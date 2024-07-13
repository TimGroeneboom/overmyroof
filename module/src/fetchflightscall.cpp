#include "fetchflightscall.h"
#include "flightstate.h"
#include "nap/logger.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "restutils.h"
#include <math.h>

RTTI_BEGIN_CLASS(nap::FetchFlightsCall)
    RTTI_PROPERTY("FlightStatesTable", &nap::FetchFlightsCall::mFlightStatesTable, nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

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
        std::vector<std::string> callsigns_to_ignore;
        std::vector<FlightState> filtered_states;
        std::unordered_map<std::string, uint64> timestamps;
        std::unordered_map<std::string, float> distances;
        if(mDatabaseTable->query(utility::stringFormat("%s > %s AND %s < %s", "TimeStamp", begin.c_str(), "TimeStamp", end.c_str()),
                                 objects, factory, error_state))
        {
            //nap::Logger::info("Got %d objects", objects.size());

            for(auto& object : objects)
            {
                if(object->get_type().is_derived_from<FlightStatesData>())
                {
                    auto* data = static_cast<FlightStatesData*>(object.get());
                    std::vector<FlightState> states;
                    if(data->ParseData(states, error_state))
                    {
                        for(const auto& state : states)
                        {
                            if(std::find(callsigns_to_ignore.begin(), callsigns_to_ignore.end(), state.mICAO) != callsigns_to_ignore.end())
                            {
                                continue;
                            }

                            double distance = calcGPSDistance(lat, lon, state.mLatitude, state.mLongitude);
                            if(state.mAltitude < altitude && distance < radius)
                            {
                                filtered_states.push_back(state);
                                callsigns_to_ignore.emplace_back(state.mICAO);
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
        }else
        {
            return utility::generateErrorResponse(error_state.toString());
        }

        rapidjson::Document document(rapidjson::kObjectType);
        rapidjson::Value data(rapidjson::kObjectType);
        document.AddMember("status", "ok", document.GetAllocator());


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
        document.AddMember("data", data, document.GetAllocator());

        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        writer.SetMaxDecimalPlaces(4);
        document.Accept(writer);

        RestResponse response;
        response.mData = buffer.GetString();
        response.mContentType = "application/json";

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

        // std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "

        return distance;
    }
}