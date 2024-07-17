#include "fetchflightscall.h"
#include "flightstate.h"
#include "nap/logger.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "restutils.h"
#include "restcontenttypes.h"
#include "addresscachedata.h"

#include <math.h>

RTTI_BEGIN_CLASS(nap::FetchFlightsCall)
    RTTI_PROPERTY("FlightStatesDatabase", &nap::FetchFlightsCall::mFlightStatesDatabase, nap::rtti::EPropertyMetaData::Required)
    RTTI_PROPERTY("StatesCache", &nap::FetchFlightsCall::mStatesCache, nap::rtti::EPropertyMetaData::Required)
    RTTI_PROPERTY("FlightStatesTableName", &nap::FetchFlightsCall::mFlightStatesTableName, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("AddressCacheRetentionDays", &nap::FetchFlightsCall::mAdressCacheRetentionDays, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("Pro6ppClient", &nap::FetchFlightsCall::mPro6ppClient, nap::rtti::EPropertyMetaData::Required | nap::rtti::EPropertyMetaData::Embedded)
    RTTI_PROPERTY("Pro6ppAddress", &nap::FetchFlightsCall::mPro6ppAddress, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("Pro6ppKeyFile", &nap::FetchFlightsCall::mPro6ppKeyFile, nap::rtti::EPropertyMetaData::Default | nap::rtti::EPropertyMetaData::FileLink)
    RTTI_PROPERTY("Pro6ppPostalCodeDescription", &nap::FetchFlightsCall::mPro6ppPostalCodeDescription, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("Pro6ppStreetNumberAndPremiseDescription", &nap::FetchFlightsCall::mPro6ppStreetNumberAndPremiseDescription, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("Pro6ppAuthKeyDescription", &nap::FetchFlightsCall::mPro6ppAuthKeyDescription, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("Pro6ppLatitudeDescription", &nap::FetchFlightsCall::mPro6ppLatitudeDescription, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("Pro6ppLongitudeDescription", &nap::FetchFlightsCall::mPro6ppLongitudeDescription, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

#define ENABLE_DEBUG_LOG 0
#if ENABLE_DEBUG_LOG
#define DEBUG_LOG(...) nap::Logger::info(__VA_ARGS__)
#else
#define DEBUG_LOG(...)
#endif

namespace nap
{
    double calcGPSDistance(double latitude_new, double longitude_new, double latitude_old, double longitude_old);

    bool FetchFlightsCall::init(utility::ErrorState &errorState)
    {
        mDatabaseTable = mFlightStatesDatabase->getDatabaseTable<FlightStatesData>(mFlightStatesTableName);

        return true;
    }


    RestResponse FetchFlightsCall::call(const std::unordered_map<std::string, std::unique_ptr<APIBaseValue>> &values)
    {
        SteadyTimer timer;
        timer.start();

        float lat, lon, altitude, radius;
        std::string begin, end, postal_code, streetnumber_and_premise;
        utility::ErrorState error_state;

        // If postal_code is not provided, lat and lon should be provided
        if(!extractValue("postal_code", values, postal_code, error_state))
        {
            if(!extractValue("lat", values, lat, error_state))
            {
                return utility::generateErrorResponse(error_state.toString());
            }
            if(!extractValue("lon", values, lon, error_state))
            {
                return utility::generateErrorResponse(error_state.toString());
            }
        }else
        {
            // Get the streetnumber_and_premise
            if(!extractValue("streetnumber_and_premise", values, streetnumber_and_premise, error_state))
            {
                return utility::generateErrorResponse(error_state.toString());
            }

            // All values required for pro6pp are present, but first check the cache if the address is present
            std::vector<std::unique_ptr<rtti::Object>> objects;
            rtti::Factory factory;
            auto* address_cache_table = mFlightStatesDatabase->getDatabaseTable<AddressCacheData>(mAddressCacheTableName);
            bool acquired_lat_lon_from_cache = false;
            if(address_cache_table != nullptr)
            {
                if(address_cache_table->query(utility::stringFormat("%s = '%s' AND %s = '%s'", "postalCode",
                                                                 postal_code.c_str(),
                                                                 "streetNumberAndPremise",
                                                                 streetnumber_and_premise.c_str()),
                                              objects, factory, error_state))
                {
                    if(!objects.empty())
                    {
                        auto valid_ts = DateTime(SystemClock::now() - std::chrono::hours(mAdressCacheRetentionDays * 24));
                        std::string valid_ts_format = utility::stringFormat("%d%02d%02d%02d%02d%02d",
                                                                            valid_ts.getYear(),
                                                                            valid_ts.getMonth(),
                                                                            valid_ts.getDayInTheMonth(),
                                                                            valid_ts.getHour(),
                                                                            valid_ts.getMinute(),
                                                                            valid_ts.getSecond());
                        uint64 valid_ts_uint64 = std::stoull(valid_ts_format);

                        assert(objects[0]->get_type().is_derived_from<AddressCacheData>());

                        auto* data = static_cast<AddressCacheData*>(objects[0].get());
                        if(data->mTimeStamp > valid_ts_uint64)
                        {
                            DEBUG_LOG(*this, "Acquired lat and lon from cache");
                            lat = data->mLat;
                            lon = data->mLon;
                            acquired_lat_lon_from_cache = true;
                        }else
                        {
                            DEBUG_LOG(*this, "Cache data is too old, removing it");
                            if(!address_cache_table->remove(utility::stringFormat("%s = '%s' AND %s = '%s'", "postalCode",
                                                                                  postal_code.c_str(),
                                                                                  "streetNumberAndPremise",
                                                                                  streetnumber_and_premise.c_str()), error_state))
                            {
                                nap::Logger::error(*this, "Failed to remove address cache data from the database : %s", error_state.toString().c_str());
                            }
                        }
                    }
                }
            }

            if(!acquired_lat_lon_from_cache)
            {
                // We have all the values we need to make a call to pro6pp, construct the values and make the call
                DEBUG_LOG(*this, "Making a call to pro6pp");

                // Get the lat and lon from the pro6pp client
                std::vector<std::unique_ptr<APIBaseValue>> pro6pp_values;
                pro6pp_values.emplace_back(std::make_unique<APIValue<std::string>>(mPro6ppPostalCodeDescription, postal_code));
                pro6pp_values.emplace_back(std::make_unique<APIValue<std::string>>(mPro6ppStreetNumberAndPremiseDescription, streetnumber_and_premise));

                // try and get the pro6pp key from the file
                std::string mPro6ppKey;
                if(!utility::readFileToString(mPro6ppKeyFile, mPro6ppKey, error_state))
                {
                    return utility::generateErrorResponse(
                            utility::stringFormat("Failed to read pro6pp key from file : %s", error_state.toString().c_str()));
                }

                pro6pp_values.emplace_back(std::make_unique<APIValue<std::string>>(mPro6ppAuthKeyDescription, mPro6ppKey));
                RestResponse pro6pp_response;
                if(!mPro6ppClient->getBlocking(mPro6ppAddress, pro6pp_values, pro6pp_response, error_state))
                {
                    return utility::generateErrorResponse(
                            utility::stringFormat("pro6pp error : %s", error_state.toString().c_str()));
                }

                // try and parse the response
                // return error if parsing fails

                rapidjson::Document document;
                document.Parse(pro6pp_response.mData.c_str());
                if(document.HasParseError())
                {
                    return utility::generateErrorResponse(
                            "Failed to parse pro6pp response, document contents : " + pro6pp_response.mData);
                }

                // finally, we can extract the lat and lon from the response, return error if they are not present
                DEBUG_LOG(*this, "Parsed pro6pp response");

                if(!document.HasMember(mPro6ppLatitudeDescription.c_str()) ||
                   !document[mPro6ppLatitudeDescription.c_str()].IsFloat())
                {
                    return utility::generateErrorResponse(
                            "Failed to parse pro6pp response, document contents : " + pro6pp_response.mData);
                }
                if(!document.HasMember(mPro6ppLongitudeDescription.c_str()) ||
                   !document[mPro6ppLongitudeDescription.c_str()].IsFloat())
                {
                    return utility::generateErrorResponse(
                            "Failed to parse pro6pp response, document contents : " + pro6pp_response.mData);
                }

                // done
                lat = document[mPro6ppLatitudeDescription.c_str()].GetFloat();
                lon = document[mPro6ppLongitudeDescription.c_str()].GetFloat();

                // Save the lat and lon to the cache
                if(address_cache_table != nullptr)
                {
                    DEBUG_LOG(*this, "Saving lat and lon to cache");

                    auto valid_ts = DateTime(SystemClock::now());
                    std::string valid_ts_format = utility::stringFormat("%d%02d%02d%02d%02d%02d",
                                                                        valid_ts.getYear(),
                                                                        valid_ts.getMonth(),
                                                                        valid_ts.getDayInTheMonth(),
                                                                        valid_ts.getHour(),
                                                                        valid_ts.getMinute(),
                                                                        valid_ts.getSecond());

                    AddressCacheData address_cache;
                    address_cache.mPostalCode = postal_code;
                    address_cache.mStreetNumberAndPremise = streetnumber_and_premise;
                    address_cache.mLat = lat;
                    address_cache.mLon = lon;
                    address_cache.mTimeStamp = std::stoull(valid_ts_format);

                    if(!address_cache_table->add(address_cache, error_state))
                    {
                        nap::Logger::error(*this, "Failed to add address cache data to the database : %s", error_state.toString().c_str());
                    }
                }
            }
        }

        if(!extractValue("altitude", values, altitude, error_state))
        {
            return utility::generateErrorResponse(error_state.toString());
        }
        if(!extractValue("radius", values, radius, error_state))
        {
            return utility::generateErrorResponse(error_state.toString());
        }
        if(!extractValue("begin", values, begin, error_state))
        {
            return utility::generateErrorResponse(error_state.toString());
        }
        if(!extractValue("end", values, end, error_state))
        {
            return utility::generateErrorResponse(error_state.toString());
        }

        std::vector<std::unique_ptr<rtti::Object>> objects;
        rtti::Factory factory;
        std::unordered_map<std::string, std::string> callsigns_to_ignore;
        std::vector<FlightState> filtered_states;
        std::unordered_map<std::string, uint64> timestamps;
        std::unordered_map<std::string, float> distances;

        // Determine how many states we need to fetch from the database and cache
        uint64 begin_timestamp_db = std::stoull(begin);
        uint64 end_timestamp_db = std::stoull(end);
        uint64 begin_timestamp_cache = mStatesCache->getOldestTimeStamp();
        uint64 end_timestamp_cache = mStatesCache->getMostRecentTimeStamp();
        bool ignore_cache = false;
        bool ignore_database = false;

        // Completely ignore the cache if timestamps are not overlapping with the cache
        if(begin_timestamp_db < begin_timestamp_cache && end_timestamp_db < begin_timestamp_cache)
        {
            ignore_cache = true;
        }else if(begin_timestamp_db < begin_timestamp_cache && end_timestamp_db > begin_timestamp_cache)
        {
            // Fetch from cache and database
            if(end_timestamp_db < end_timestamp_cache)
            {
                end_timestamp_cache = end_timestamp_db;
            }
            end_timestamp_db = begin_timestamp_cache;
        }else if(begin_timestamp_db > begin_timestamp_cache)
        {
            // Fetch only from cache
            begin_timestamp_cache = begin_timestamp_db;
            end_timestamp_cache = end_timestamp_db;
            ignore_database = true;
        }

        if(!ignore_database)
        {
            if(mDatabaseTable->query(utility::stringFormat("%s > %s AND %s < %s", "TimeStamp",
                                                           std::to_string(begin_timestamp_db).c_str(),
                                                           "TimeStamp",
                                                           std::to_string(end_timestamp_db).c_str()),
                                     objects, factory, error_state))
            {
                // Iterate over all the objects
                for(auto& object : objects)
                {
                    assert(object->get_type().is_derived_from<FlightStatesData>());

                    // Cast the object to the correct type
                    auto* data = static_cast<FlightStatesData*>(object.get());
                    std::vector<FlightState> states;

                    // Parse the data
                    if(data->ParseData(states, altitude, error_state))
                    {
                        for(const auto& state : states)
                        {
                            if(callsigns_to_ignore.find(state.mICAO) != callsigns_to_ignore.end())
                            {
                                continue;
                            }

                            double distance = calcGPSDistance(lat, lon, state.mLatitude, state.mLongitude);
                            if(distance < radius)
                            {
                                filtered_states.push_back(state);
                                callsigns_to_ignore[state.mICAO] = state.mICAO;
                                timestamps[state.mICAO] = data->mTimeStamp;
                                distances[state.mICAO] = distance;
                            }
                        }
                    }else
                    {
                        return utility::generateErrorResponse(error_state.toString());
                    }
                }
            }
        }

        std::vector<FlightStates> states;
        if(!ignore_cache)
        {
            if(mStatesCache->getStates(begin_timestamp_cache, end_timestamp_cache, altitude, states))
            {
                for(const auto& state : states)
                {
                    for(const auto& flight : state.mStates)
                    {
                        if(callsigns_to_ignore.find(flight.mICAO) != callsigns_to_ignore.end())
                        {
                            continue;
                        }

                        double distance = calcGPSDistance(lat, lon, flight.mLatitude, flight.mLongitude);
                        if(distance < radius)
                        {
                            filtered_states.push_back(flight);
                            callsigns_to_ignore[flight.mICAO] = flight.mICAO;
                            timestamps[flight.mICAO] = state.mTimeStamp;
                            distances[flight.mICAO] = distance;
                        }
                    }
                }
            }
        }

        DEBUG_LOG(*this, "Got %d states from database and %d states from cache", objects.size(), states.size());
        DEBUG_LOG(*this, "Filtered %d states", filtered_states.size());

        // Create the json document
        rapidjson::Document document(rapidjson::kObjectType);
        rapidjson::Value data(rapidjson::kObjectType);
        document.AddMember("status", "ok", document.GetAllocator());

        // Add found flights to the response
        rapidjson::Value flights(rapidjson::kArrayType);
        for(const auto& state : filtered_states)
        {
            rapidjson::Value flight(rapidjson::kObjectType);
            flight.AddMember("icao", rapidjson::Value(state.mICAO.c_str(), document.GetAllocator()), document.GetAllocator());
            flight.AddMember("reg", rapidjson::Value(state.mRegistration.c_str(), document.GetAllocator()), document.GetAllocator());
            flight.AddMember("aircraft_type", rapidjson::Value(state.mAircraftType.c_str(), document.GetAllocator()), document.GetAllocator());
            flight.AddMember("lat", state.mLatitude, document.GetAllocator());
            flight.AddMember("lon", state.mLongitude, document.GetAllocator());
            flight.AddMember("altitude", state.mAltitude, document.GetAllocator());
            flight.AddMember("timestamp", timestamps[state.mICAO], document.GetAllocator());
            flight.AddMember("distance", distances[state.mICAO], document.GetAllocator());

            flights.PushBack(flight, document.GetAllocator());
        }
        data.AddMember("flights", flights, document.GetAllocator());
        data.AddMember("ms", timer.getMillis().count(), document.GetAllocator());
        data.AddMember("states", objects.size() + states.size(), document.GetAllocator());
        document.AddMember("data", data, document.GetAllocator());

        // Serialize the response
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        writer.SetMaxDecimalPlaces(4);
        document.Accept(writer);

        // Create the response
        RestResponse response;
        response.mData = buffer.GetString();
        response.mContentType = rest::contenttypes::json;

        return response;
    }


    double toRad(double degree) {
        return degree/180 * M_PI;
    }

    #define PI 3.14159265358979323846
    #define RADIO_TERRESTRE 6372797.56085
    #define GRADOS_RADIANES PI / 180
    #define RADIANES_GRADOS 180 / PI

    double calcGPSDistance(double latitude_new, double longitude_new, double latitude_old, double longitude_old)
    {
        double lat_new = latitude_old * GRADOS_RADIANES;
        double lat_old = latitude_new * GRADOS_RADIANES;
        double lat_diff = (latitude_new - latitude_old) * GRADOS_RADIANES;
        double lng_diff = (longitude_new - longitude_old) * GRADOS_RADIANES;

        double a = sin(lat_diff / 2) * sin(lat_diff / 2) +
                   cos(lat_new) * cos(lat_old) *
                   sin(lng_diff / 2) * sin(lng_diff / 2);
        double c = 2 * atan2(sqrt(a), sqrt(1 - a));

        double distance = RADIO_TERRESTRE * c;


        return distance;
    }
}