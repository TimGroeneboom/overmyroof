#include "fetchflightscall.h"
#include "flightstate.h"
#include "nap/logger.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "restutils.h"
#include "restcontenttypes.h"

#include <math.h>

RTTI_BEGIN_CLASS(nap::FetchFlightsCall)
    RTTI_PROPERTY("FlightStatesTable", &nap::FetchFlightsCall::mFlightStatesTable, nap::rtti::EPropertyMetaData::Required)
    RTTI_PROPERTY("StatesCache", &nap::FetchFlightsCall::mStatesCache, nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

#define CALL_DEBUG 0
#if CALL_DEBUG
#define CALL_DEBUG_LOG(...) nap::Logger::info(__VA_ARGS__)
#else
#define CALL_DEBUG_LOG(...)
#endif

namespace nap
{
    double calcGPSDistance(double latitude_new, double longitude_new, double latitude_old, double longitude_old);

    bool FetchFlightsCall::init(utility::ErrorState &errorState)
    {
        mDatabaseTable = mFlightStatesTable->getDatabaseTable();

        return true;
    }


    RestResponse FetchFlightsCall::call(const std::unordered_map<std::string, std::unique_ptr<APIBaseValue>> &values)
    {
        SteadyTimer timer;
        timer.start();

        float lat, lon, altitude, radius;
        std::string begin, end;
        utility::ErrorState error_state;
        if(!extractValue("lat", values, lat, error_state))
        {
            return utility::generateErrorResponse(error_state.toString());
        }
        if(!extractValue("lon", values, lon, error_state))
        {
            return utility::generateErrorResponse(error_state.toString());
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
        if(begin_timestamp_db < begin_timestamp_cache && end_timestamp_db < end_timestamp_cache)
        {
            ignore_cache = true;
        }else if(begin_timestamp_db < begin_timestamp_cache && end_timestamp_db > begin_timestamp_cache)
        {
            // Fetch from cache and database
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
                    if(data->ParseData(states, error_state))
                    {
                        for(const auto& state : states)
                        {
                            if(callsigns_to_ignore.find(state.mICAO) != callsigns_to_ignore.end())
                            {
                                continue;
                            }

                            if(state.mAltitude < altitude)
                            {
                                double distance = calcGPSDistance(lat, lon, state.mLatitude, state.mLongitude);
                                if(distance < radius)
                                {
                                    filtered_states.push_back(state);
                                    callsigns_to_ignore[state.mICAO] = state.mICAO;
                                    timestamps[state.mICAO] = data->mTimeStamp;
                                    distances[state.mICAO] = distance;
                                }
                            }else
                            {
                                // We can break out the loop since the states are sorted by altitude
                                break;
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
            if(mStatesCache->getStates(begin_timestamp_cache, end_timestamp_cache, states))
            {
                for(const auto& state : states)
                {
                    for(const auto& flight : state.mStates)
                    {
                        if(callsigns_to_ignore.find(flight.mICAO) != callsigns_to_ignore.end())
                        {
                            continue;
                        }

                        if(flight.mAltitude < altitude)
                        {
                            double distance = calcGPSDistance(lat, lon, flight.mLatitude, flight.mLongitude);
                            if(distance < radius)
                            {
                                filtered_states.push_back(flight);
                                callsigns_to_ignore[flight.mICAO] = flight.mICAO;
                                timestamps[flight.mICAO] = state.mTimeStamp;
                                distances[flight.mICAO] = distance;
                            }
                        }else
                        {
                            // We can break out the loop since the states are sorted by altitude
                            break;
                        }
                    }
                }
            }
        }

        CALL_DEBUG_LOG(*this, "Got %d states from database and %d states from cache", objects.size(), states.size());
        CALL_DEBUG_LOG(*this, "Filtered %d states", filtered_states.size());

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