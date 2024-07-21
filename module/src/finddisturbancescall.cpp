#include "finddisturbancescall.h"
#include "restutils.h"
#include "restcontenttypes.h"
#include "nap/logger.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include <nap/datetime.h>
#include "utils.h"

RTTI_BEGIN_CLASS(nap::FindDisturbancesCall)
RTTI_PROPERTY("FetchFlightsCall", &nap::FindDisturbancesCall::mFetchFlightsCall, nap::rtti::EPropertyMetaData::Required, "Reference to the fetch flights call")
RTTI_PROPERTY("MaxPeriod", &nap::FindDisturbancesCall::mMaxPeriod, nap::rtti::EPropertyMetaData::Default, "Maximum period in minutes to search for disturbances")
RTTI_PROPERTY("MinPeriod", &nap::FindDisturbancesCall::mMinPeriod, nap::rtti::EPropertyMetaData::Default, "Minimum period in minutes to search for disturbances")
RTTI_END_CLASS

#define ENABLE_DEBUG_LOG 0
#if ENABLE_DEBUG_LOG
#define DEBUG_LOG(...) nap::Logger::info(__VA_ARGS__)
#else
#define DEBUG_LOG(...)
#endif

namespace nap
{
    /**
     * Structure to hold a disturbance period
     */
    struct DisturbancePeriod
    {
    public:
        uint64 mBegin; ///< Begin timestamp of the disturbance period
        uint64 mEnd; ///< End timestamp of the disturbance period
        std::vector<FlightState> mStates; ///< List of flight states in the disturbance period
        std::unordered_map<std::string, uint64> mTimestamps; ///< Timestamps of the flight states in the disturbance period
        int mOccurrences; ///< Number of occurrences in the disturbance period
    };

    bool FindDisturbancesCall::init(utility::ErrorState &errorState)
    {
        return true;
    }


    RestResponse FindDisturbancesCall::call(const RestValueMap &values)
    {
        // start time to measure the time it takes to execute the call
        SteadyTimer timer;
        timer.start();

        // Errorstate
        utility::ErrorState error_state;

        // Extract find disturbances call specific values
        int occurrences, period;
        if(!extractValue("occurrences", values, occurrences, error_state))
            return utility::generateErrorResponse(error_state.toString());
        if(occurrences < 1)
            return utility::generateErrorResponse("occurrences must be greater than 0");

        if(!extractValue("period", values, period, error_state))
            return utility::generateErrorResponse(error_state.toString());
        if(period < mMinPeriod)
            return utility::generateErrorResponse(utility::stringFormat("period must be greater than %d minutes", mMinPeriod));
        if(period > mMaxPeriod)
            return utility::generateErrorResponse(utility::stringFormat("period must be less than %d minutes", mMaxPeriod));

        // Get states from referenced fetch flights call
        std::vector<FlightState> filtered_states;
        std::unordered_map<std::string, uint64> timestamps;
        std::unordered_map<std::string, float> distances;
        if(!mFetchFlightsCall->getFlights(values, filtered_states, timestamps, distances, error_state))
            return utility::generateErrorResponse(error_state.toString());

        // Find disturbances

        // sort states by timestamp
        std::sort(filtered_states.begin(), filtered_states.end(), [&timestamps](const FlightState& a, const FlightState& b) { return timestamps[a.mICAO] < timestamps[b.mICAO]; });

        // begin collecting disturbance periods
        int in_period_count = 0; // count of flights in the period
        int disturbances_count = 0; // count of total disturbances in a period
        bool currently_in_period = false; // are we currently in a disturbance period
        uint64 begin_current_period; // timestamp of the beginning of the current period
        uint64 end_current_period; // timestamp of the end of the current period
        DisturbancePeriod disturbance_period; // current disturbance period
        std::vector<DisturbancePeriod> disturbance_periods; // list of found disturbance periods
        std::vector<FlightState> disturbance_states; // list of flight states in the current disturbance period
        std::unordered_map<std::string, uint64> disturbance_timestamps; // timestamps of the flight states in the current disturbance period

        // iterate over the flight, comparing the time difference between each flight state.
        // all flight states are already filtered by altitude
        // if the time difference is less than the period, we are in a (potential) disturbance period
        // if we are in a potential disturbance period, we need to check if we have enough occurrences to register the period

        for(int i = 1; i < filtered_states.size(); i++)
        {
            const FlightState& state1 = filtered_states[i - 1];
            const FlightState& state2 = filtered_states[i];

            // Calculate time difference
            DateTime dt_1;
            if(!utility::dateTimeFromUINT64(timestamps[state1.mICAO], dt_1, error_state))
                return utility::generateErrorResponse(error_state.toString());

            DateTime dt_2;
            if(!utility::dateTimeFromUINT64(timestamps[state2.mICAO], dt_2, error_state))
                return utility::generateErrorResponse(error_state.toString());

            // if the time difference is less than the period, we are in a (potential) disturbance period
            auto dt_diff_minutes = (std::chrono::duration<double, std::milli>(dt_2.getTimeStamp()-dt_1.getTimeStamp()).count() / 1000) / 60;
            bool register_period = false;
            if(dt_diff_minutes < period)
            {
                if(in_period_count== 0 && !currently_in_period)
                {
                    DEBUG_LOG(*this, "Begin a disturbance period at %s", dt_1.toString().c_str());
                    begin_current_period = timestamps[state1.mICAO];

                    // add the first state
                    disturbance_states.emplace_back(state1);
                    disturbance_timestamps[state1.mICAO] = timestamps[state1.mICAO];
                    disturbances_count++;
                }

                // advance the count
                in_period_count++;
                disturbances_count++;

                disturbance_states.emplace_back(state2);
                disturbance_timestamps[state2.mICAO] = timestamps[state2.mICAO];

                // if we have enough occurrences, we are in a disturbance period
                if(in_period_count >= occurrences)
                {
                    currently_in_period = true;
                    in_period_count = 0;
                    end_current_period = timestamps[state2.mICAO];
                }
            }else if(currently_in_period)
            {
                // if we are in a disturbance period, but we don't have enough occurrences, we need to check if we should register the period

                DEBUG_LOG(*this, "%i flights detected in period from %ld to %ld",
                          disturbances_count,
                          begin_current_period,
                          end_current_period);

                if(disturbances_count >= occurrences)
                {
                    register_period = true;
                }else
                {
                    // discard the period
                    register_period = false;
                    currently_in_period = false;
                    in_period_count = 0;
                    disturbances_count = 0;
                    disturbance_states.clear();
                    disturbance_timestamps.clear();
                }
            }else
            {
                // discard the period
                register_period = false;
                currently_in_period = false;
                in_period_count = 0;
                disturbances_count = 0;
                disturbance_states.clear();
                disturbance_timestamps.clear();
            }

            if(register_period)
            {
                DEBUG_LOG(*this, "%i flights detected in period from %ld to %ld",
                          disturbances_count,
                          begin_current_period,
                          end_current_period);

                disturbance_period.mBegin = begin_current_period;
                disturbance_period.mEnd = end_current_period;
                disturbance_period.mStates = disturbance_states;
                disturbance_period.mTimestamps = disturbance_timestamps;
                disturbance_period.mOccurrences = disturbances_count;

                currently_in_period = false;
                in_period_count = 0;
                disturbances_count = 0;
                disturbance_states.clear();
                disturbance_timestamps.clear();
                disturbance_periods.push_back(disturbance_period);
            }
        }

        // we reached the end of the list, check if we are in a disturbance period
        // register the period if we have enough occurrences
        if(currently_in_period && disturbances_count >= occurrences)
        {
            DEBUG_LOG(*this, "%i flights detected in period from %ld to %ld",
                      disturbances_count,
                      begin_current_period,
                      end_current_period);

            disturbance_period.mBegin = begin_current_period;
            disturbance_period.mEnd = end_current_period;
            disturbance_period.mStates = disturbance_states;
            disturbance_period.mTimestamps = disturbance_timestamps;
            disturbance_period.mOccurrences = disturbances_count;

            currently_in_period = false;
            in_period_count = 0;
            disturbances_count = 0;
            disturbance_states.clear();
            disturbance_timestamps.clear();
            disturbance_periods.push_back(disturbance_period);
        }

        // Create response

        // Create the json document
        rapidjson::Document document(rapidjson::kObjectType);
        rapidjson::Value data(rapidjson::kObjectType);
        document.AddMember("status", "ok", document.GetAllocator());

        // Add found flights to the response
        rapidjson::Value periods(rapidjson::kArrayType);
        for(const auto& p : disturbance_periods)
        {
            rapidjson::Value disturbance(rapidjson::kObjectType);
            disturbance.AddMember("begin", p.mBegin, document.GetAllocator());
            disturbance.AddMember("end", p.mEnd, document.GetAllocator());
            disturbance.AddMember("flights", rapidjson::Value(rapidjson::kArrayType), document.GetAllocator());
            for(const auto& f : p.mStates)
            {
                rapidjson::Value flight(rapidjson::kObjectType);
                flight.AddMember("icao", rapidjson::Value(f.mICAO.c_str(), document.GetAllocator()), document.GetAllocator());
                flight.AddMember("reg", rapidjson::Value(f.mRegistration.c_str(), document.GetAllocator()), document.GetAllocator());
                flight.AddMember("aircraft_type", rapidjson::Value(f.mAircraftType.c_str(), document.GetAllocator()), document.GetAllocator());
                flight.AddMember("lat", f.mLatitude, document.GetAllocator());
                flight.AddMember("lon", f.mLongitude, document.GetAllocator());
                flight.AddMember("altitude", f.mAltitude, document.GetAllocator());
                flight.AddMember("timestamp", p.mTimestamps.at(f.mICAO), document.GetAllocator());
                disturbance["flights"].PushBack(flight, document.GetAllocator());
            }
            disturbance.AddMember("occurrences", p.mOccurrences, document.GetAllocator());
            periods.PushBack(disturbance, document.GetAllocator());
        }

        data.AddMember("disturbance_periods", periods, document.GetAllocator());
        data.AddMember("ms", timer.getMillis().count(), document.GetAllocator());
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
}