#include "planeloggercomponent.h"
#include "flightstate.h"

#include <nap/logger.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>

RTTI_BEGIN_CLASS(nap::PlaneLoggerComponent)
    RTTI_PROPERTY("RestClient", &nap::PlaneLoggerComponent::mRestClient, nap::rtti::EPropertyMetaData::Required)
    RTTI_PROPERTY("Interval", &nap::PlaneLoggerComponent::mInterval, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("FlightStatesDatabase", &nap::PlaneLoggerComponent::mFlightStatesDatabase, nap::rtti::EPropertyMetaData::Required)
    RTTI_PROPERTY("StatesCache", &nap::PlaneLoggerComponent::mStatesCache, nap::rtti::EPropertyMetaData::Required)
    RTTI_PROPERTY("RetainHours", &nap::PlaneLoggerComponent::mRetainHours, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("CacheHours", &nap::PlaneLoggerComponent::mCacheHours, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("Adress", &nap::PlaneLoggerComponent::mAdress, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("Bounds", &nap::PlaneLoggerComponent::mBounds, nap::rtti::EPropertyMetaData::Default)
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
        mFlightStatesTableName = resource->mFlightStatesTableName;
        mFlightStatesTable = resource->mFlightStatesDatabase->getDatabaseTable<FlightStatesData>(mFlightStatesTableName);
        mStatesCache = resource->mStatesCache.get();
        mRetainHours = resource->mRetainHours;
        mCacheHours = resource->mCacheHours;
        mAddress = resource->mAdress;
        mBounds = resource->mBounds;

        mTime = mInterval;

        // Fill cache with data from the last cache hours
        nap::Logger::info(*this, "Filling cache with data from the last 24 hours");
        auto now = getCurrentDateTime();
        std::string now_format = utility::stringFormat("%d%02d%02d%02d%02d%02d", now.getYear(), now.getMonth(), now.getDayInTheMonth(), now.getHour(), now.getMinute(), now.getSecond());
        uint64 now_uint64 = std::stoull(now_format);

        auto yes = DateTime(SystemClock::now() - std::chrono::hours(mCacheHours));
        std::string yes_format = utility::stringFormat("%d%02d%02d%02d%02d%02d", yes.getYear(), yes.getMonth(), yes.getDayInTheMonth(), yes.getHour(), yes.getMinute(), yes.getSecond());
        uint64 yes_uint64 = std::stoull(yes_format);

        utility::ErrorState e;
        rtti::Factory factory;
        std::vector<std::unique_ptr<rtti::Object>> objects;
        if(mFlightStatesTable->query(utility::stringFormat("%s > %s AND %s < %s", "TimeStamp",
                                                       std::to_string(yes_uint64).c_str(),
                                                       "TimeStamp",
                                                       std::to_string(now_uint64).c_str()),
                                     objects, factory, e))
        {
            // Iterate over all the objects
            for(auto &object: objects)
            {
                assert(object->get_type().is_derived_from<FlightStatesData>());

                // Cast the object to the correct type
                auto* data = static_cast<FlightStatesData*>(object.get());
                std::vector<FlightState> states;

                // Parse the data
                if(data->ParseData(states, -1, e))
                {
                    // sort by altitude
                    std::sort(states.begin(), states.end(), [](const FlightState& a, const FlightState& b)
                    {
                        return a.mAltitude < b.mAltitude;
                    });

                    for(const auto &state: states)
                    {
                        mStatesCache->addStates(data->mTimeStamp, states);
                    }
                }else
                {
                    nap::Logger::error(*this, "Error parsing data : %s", e.toString().c_str());
                }
            }
        }else
        {
            nap::Logger::error(*this, "Error querying database : %s", e.toString().c_str());
        }

        nap::Logger::info(*this, "Cache filled with %i states", objects.size());

        return true;
    }


    void PlaneLoggerComponentInstance::update(double deltaTime)
    {
        if(mQuerying)
            return;

        mTime += deltaTime;
        if(mTime > mInterval)
        {
            mQuerying = true;

            mTime = 0.0;

            nap::Logger::info(*this, "Getting flight states from url %s address %s", mRestClient->mURL.c_str(), mAddress.c_str());

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
            params.emplace_back(std::make_unique<APIDoubleArray>("bounds", std::vector<double>{mBounds[0], mBounds[1], mBounds[2], mBounds[3]}));
            mRestClient->get(mAddress, params, [this](const RestResponse& response)
            {
                FlightStatesData state;

                // set timestamp
                auto now = getCurrentDateTime();
                std::string now_format = utility::stringFormat("%d%02d%02d%02d%02d%02d", now.getYear(), now.getMonth(), now.getDayInTheMonth(), now.getHour(), now.getMinute(), now.getSecond());
                uint64 now_uint64 = std::stoull(now_format);
                state.mTimeStamp = now_uint64;

                // create json document
                rapidjson::Document document(rapidjson::kObjectType);
                rapidjson::Value flights(rapidjson::kArrayType);

                // parse fetched data
                rapidjson::Document fetched_data(rapidjson::kObjectType);
                fetched_data.Parse(response.mData.c_str());

                //
                FlightStates states;
                states.mTimeStamp = now_uint64;

                // We make unique pointers to string because rapidjson::Value is not copyable
                std::vector<std::unique_ptr<std::string>> strings;
                int states_added = 0;
                for (auto p = fetched_data.MemberBegin(); p != fetched_data.MemberEnd(); ++p)
                {
                    if(p->value.IsArray())
                    {
                        // Following are the indexes that represent some of the data returned in the array by the FlightRadars24 API
                        static const int lat_index = 1;
                        static const int lon_index = 2;
                        static const int altitude_index = 4;
                        static const int aircraft_type_index = 8;
                        static const int icao_index = 16;
                        static const int reg_index = 18;

                        // The following data is extracted from the array and stored in the database
                        // We make unique pointers to string because rapidjson::Value is not copyable
                        float lat = 0.0f;
                        float lon = 0.0f;
                        std::unique_ptr<std::string> icao = std::make_unique<std::string>("");
                        std::unique_ptr<std::string> reg = std::make_unique<std::string>("");
                        std::unique_ptr<std::string> aircraft_type = std::make_unique<std::string>("");
                        float altitude = 0.0f;

                        // Proceed to extract the data from the array
                        int idx = 0;
                        int added = 0;
                        bool add_flight = true;
                        for (auto q = p->value.Begin(); q != p->value.End(); ++q)
                        {
                            if(idx==lat_index) // lat
                            {
                                if(q->IsFloat())
                                {
                                    lat = q->GetFloat();
                                    added++;
                                }else
                                {
                                    nap::Logger::error("lat is not float");
                                    add_flight = false;
                                    break;
                                }
                            }else if(idx==lon_index) // lon
                            {
                                if(q->IsFloat())
                                {
                                    lon = q->GetFloat();
                                    added++;
                                }else
                                {
                                    nap::Logger::error("lon is not float");
                                    add_flight = false;
                                    break;
                                }
                            }else if(idx==altitude_index)
                            {
                                if(q->IsInt())
                                {
                                    // to feet
                                    altitude = q->GetInt() * 0.3048f;
                                    if(altitude <= 0.0f)
                                    {
                                        add_flight = false;
                                        break;
                                    }else
                                    {
                                        added++;
                                    }
                                }else
                                {
                                    nap::Logger::error("altitude is not float");
                                    add_flight = false;
                                    break;
                                }
                            }else if(idx==aircraft_type_index)
                            {
                                if(q->IsString())
                                {
                                    aircraft_type = std::make_unique<std::string>(q->GetString());
                                    added++;
                                }else
                                {
                                    nap::Logger::error("aircraft_type is not string");
                                    add_flight = false;
                                    break;
                                }
                            }else if(idx==icao_index)// icao
                            {
                                if(q->IsString())
                                {
                                    icao = std::make_unique<std::string>(q->GetString());
                                    added++;
                                }else
                                {
                                    nap::Logger::error("icao is not string");
                                    add_flight = false;
                                    break;
                                }
                            }else if(idx==reg_index)// reg
                            {
                                if(q->IsString())
                                {
                                    reg = std::make_unique<std::string>(q->GetString());
                                    added++;
                                }else
                                {
                                    nap::Logger::error("reg is not string");
                                    add_flight = false;
                                    break;
                                }
                            }

                            idx++;
                        }

                        // Some data is missing, so we don't add the flight
                        if(added != 6)
                            add_flight = false;

                        if(add_flight)
                        {
                            FlightState flight_state;
                            flight_state.mAircraftType = *aircraft_type;
                            flight_state.mAltitude = altitude;
                            flight_state.mICAO = *icao;
                            flight_state.mLatitude = lat;
                            flight_state.mLongitude = lon;
                            flight_state.mRegistration = *reg;
                            states.mStates.emplace_back(flight_state);

                            states_added++;
                        }
                    }
                }

                // sort by altitude
                std::sort(states.mStates.begin(), states.mStates.end(), [](const FlightState& a, const FlightState& b)
                {
                    return a.mAltitude < b.mAltitude;
                });

                // add to cache
                mStatesCache->addStates(now_uint64, states.mStates);

                // add to database
                for(const auto &state: states.mStates)
                {
                    rapidjson::Value flight(rapidjson::kObjectType);
                    rapidjson::Value data(rapidjson::kArrayType);

                    data.PushBack(state.mLatitude, document.GetAllocator());
                    data.PushBack(state.mLongitude, document.GetAllocator());
                    data.PushBack(state.mAltitude, document.GetAllocator());
                    data.PushBack(rapidjson::StringRef(state.mICAO.c_str()), document.GetAllocator());
                    data.PushBack(rapidjson::StringRef(state.mRegistration.c_str()), document.GetAllocator());
                    data.PushBack(rapidjson::StringRef(state.mAircraftType.c_str()), document.GetAllocator());

                    flight.AddMember("data", data, document.GetAllocator());
                    flights.PushBack(flight, document.GetAllocator());
                }

                // Add the flights to the document
                document.AddMember("flights", flights, document.GetAllocator());

                // Serialize the document
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                document.Accept(writer);

                // Write the data to the database
                state.mData = buffer.GetString();
                utility::ErrorState err;
                if(!mFlightStatesTable->add(state, err))
                {
                    nap::Logger::error(*this, "Error writing to database : %s", err.toString().c_str());
                }else
                {
                    nap::Logger::info(*this, "Successfully wrote %i states to database", states_added);
                }

                // Remove entries older than retain hours property
                auto past = DateTime(SystemClock::now() - std::chrono::hours(mRetainHours));
                uint64 past_uint64 = std::stoull(utility::stringFormat("%d%02d%02d%02d%02d%02d", past.getYear(), past.getMonth(), past.getDayInTheMonth(), past.getHour(), past.getMinute(), past.getSecond()));
                nap::Logger::info(*this, "Removing entries older than %s", past.toString().c_str());
                if(!mFlightStatesTable->remove(utility::stringFormat("%s < %s", "TimeStamp", std::to_string(past_uint64).c_str()), err))
                {
                    nap::Logger::error(*this, "Error removing old entries : %s", err.toString().c_str());
                }

                mQuerying = false;
            }, [this](const utility::ErrorState& error)
            {
                nap::Logger::error(*this, "Error getting flight states : %s", error.toString().c_str());
                mQuerying = false;
            });
        }
    }


    void PlaneLoggerComponentInstance::clear()
    {
        utility::ErrorState err;
        if(!mFlightStatesTable->clear(err))
        {
            nap::Logger::error(*this, "Error clearing database : %s", err.toString().c_str());
        }
    }
}