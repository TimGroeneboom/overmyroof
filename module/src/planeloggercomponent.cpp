#include "planeloggercomponent.h"
#include "flightstate.h"

#include <nap/logger.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>

RTTI_BEGIN_CLASS(nap::PlaneLoggerComponent)
    RTTI_PROPERTY("RestClient", &nap::PlaneLoggerComponent::mRestClient, nap::rtti::EPropertyMetaData::Required)
    RTTI_PROPERTY("Interval", &nap::PlaneLoggerComponent::mInterval, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("FlightStatesTable", &nap::PlaneLoggerComponent::mFlightStatesTable, nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::PlaneLoggerComponentInstance)
RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component&)
RTTI_END_CLASS

namespace nap
{
    PlaneLoggerComponentInstance::PlaneLoggerComponentInstance(nap::EntityInstance &entityInstance, nap::Component &component)
        : ComponentInstance(entityInstance, component)
    {}


    bool PlaneLoggerComponentInstance::init(utility::ErrorState &errorState)
    {
        auto* resource = getComponent<PlaneLoggerComponent>();
        mRestClient = resource->mRestClient.get();
        mInterval = resource->mInterval;
        mFlightStatesTable = resource->mFlightStatesTable->getDatabaseTable();

        return true;
    }


    void PlaneLoggerComponentInstance::update(double deltaTime)
    {
        mTime += deltaTime;
        if(mTime > mInterval)
        {
            mTime = 0.0;

            std::vector<std::unique_ptr<APIBaseValue>> params;
            params.emplace_back(std::make_unique<APIValue<int>>("faa", 1));
            params.emplace_back(std::make_unique<APIValue<int>>("satellite", 1));
            params.emplace_back(std::make_unique<APIValue<int>>("mlat", 1));
            params.emplace_back(std::make_unique<APIValue<int>>("flarm", 1));
            params.emplace_back(std::make_unique<APIValue<int>>("adsb", 1));
            params.emplace_back(std::make_unique<APIValue<int>>("gnd", 1));
            params.emplace_back(std::make_unique<APIValue<int>>("air", 1));
            params.emplace_back(std::make_unique<APIValue<int>>("vehicles", 1));
            params.emplace_back(std::make_unique<APIValue<int>>("estimated", 1));
            params.emplace_back(std::make_unique<APIValue<int>>("maxage", 14400));
            params.emplace_back(std::make_unique<APIValue<int>>("gliders", 1));
            params.emplace_back(std::make_unique<APIValue<int>>("stats", 1));
            params.emplace_back(std::make_unique<APIValue<int>>("limit", 5000));
            params.emplace_back(std::make_unique<APIDoubleArray>("bounds", std::vector<double>{53.445884435606054, 50.749405057563486, 3.5163031843031223, 7.9136148705580505}));
            mRestClient->get("/zones/fcgi/feed.js", params, [this](const RestResponse& response)
            {
                FlightStatesData state;

                auto now = getCurrentDateTime();
                std::string now_format = utility::stringFormat("%d%02d%02d%02d%02d%02d", now.getYear(), now.getMonth(), now.getDayInTheMonth(), now.getHour(), now.getMinute(), now.getSecond());
                uint64 now_uint64 = std::stoull(now_format);
                state.mTimeStamp = now_uint64;

                //
                rapidjson::Document document(rapidjson::kObjectType);
                rapidjson::Value flights(rapidjson::kArrayType);

                // parse fetched
                rapidjson::Document fetched_data(rapidjson::kObjectType);
                fetched_data.Parse(response.mData.c_str());

                //
                std::vector<std::unique_ptr<std::string>> strings;
                for (auto p = fetched_data.MemberBegin(); p != fetched_data.MemberEnd(); ++p)
                {
                    if(p->value.IsArray())
                    {
                        //
                        float lat = 0.0f;
                        float lon = 0.0f;
                        std::unique_ptr<std::string> icao = std::make_unique<std::string>("");
                        std::unique_ptr<std::string> reg = std::make_unique<std::string>("");
                        std::unique_ptr<std::string> aircraft_type = std::make_unique<std::string>("");
                        float altitude = 0.0f;

                        //
                        int idx = 0;
                        bool add_flight = true;
                        for (auto q = p->value.Begin(); q != p->value.End(); ++q)
                        {
                            if(idx==1) // lat
                            {
                                if(q->IsFloat())
                                {
                                    lat = q->GetFloat();
                                }else
                                {
                                    nap::Logger::error("lat is not float");
                                    add_flight = false;
                                    break;
                                }
                            }else if(idx==2) // lon
                            {
                                if(q->IsFloat())
                                {
                                    lon = q->GetFloat();
                                }else
                                {
                                    nap::Logger::error("lon is not float");
                                    add_flight = false;
                                    break;
                                }
                            }else if(idx==4)
                            {
                                if(q->IsInt())
                                {
                                    // to feet
                                    altitude = q->GetInt() * 0.3048f;
                                    if(altitude <= 0.0f)
                                    {
                                        add_flight = false;
                                        break;
                                    }
                                }else
                                {
                                    nap::Logger::error("altitude is not float");
                                    add_flight = false;
                                    break;
                                }
                            }else if(idx==8)
                            {
                                if(q->IsString())
                                {
                                    aircraft_type = std::make_unique<std::string>(q->GetString());
                                }else
                                {
                                    nap::Logger::error("aircraft_type is not string");
                                    add_flight = false;
                                    break;
                                }
                            }else if(idx==16)// icao
                            {
                                if(q->IsString())
                                {
                                    icao = std::make_unique<std::string>(q->GetString());
                                }else
                                {
                                    nap::Logger::error("icao is not string");
                                    add_flight = false;
                                    break;
                                }
                            }else if(idx==18)// reg
                            {
                                if(q->IsString())
                                {
                                    reg = std::make_unique<std::string>(q->GetString());
                                }else
                                {
                                    nap::Logger::error("reg is not string");
                                    add_flight = false;
                                    break;
                                }
                            }

                            idx++;
                        }

                        if(add_flight)
                        {
                            rapidjson::Value flight(rapidjson::kObjectType);
                            flight.AddMember("lat", lat, document.GetAllocator());
                            flight.AddMember("lon", lon, document.GetAllocator());
                            flight.AddMember("icao", rapidjson::StringRef(icao->c_str()), document.GetAllocator());
                            flight.AddMember("reg", rapidjson::StringRef(reg->c_str()), document.GetAllocator());
                            flight.AddMember("aircraft_type", rapidjson::StringRef(aircraft_type->c_str()), document.GetAllocator());
                            flight.AddMember("altitude", altitude, document.GetAllocator());
                            flights.PushBack(flight, document.GetAllocator());

                            strings.push_back(std::move(icao));
                            strings.push_back(std::move(reg));
                            strings.push_back(std::move(aircraft_type));
                        }
                    }
                }

                document.AddMember("flights", flights, document.GetAllocator());

                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                document.Accept(writer);

                // nap::Logger::info("json: %s", buffer.GetString());
                state.mData = buffer.GetString();
                utility::ErrorState err;
                if(!mFlightStatesTable->add(state, err))
                {
                    nap::Logger::error("error! %s", err.toString().c_str());
                }else
                {
                    nap::Logger::info("Successfully wrote flight data to database");
                }
            }, [](const utility::ErrorState& error)
            {
                nap::Logger::error("error! %s", error.toString().c_str());
            });
        }
    }
}