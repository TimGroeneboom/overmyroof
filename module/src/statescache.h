#pragma once

#include <nap/resource.h>
#include <atomic>

#include "flightstate.h"

namespace nap
{
    /**
     * A collection of flight states at a certain timestamp
     */
    class NAPAPI FlightStates
    {
    public:
        std::vector<FlightState> mStates;
        uint64 mTimeStamp;
    };

    /**
     * A cache for storing flight states
     * The cache is thread safe
     * The cache will remove the oldest entries if the max number of entries is reached
     */
    class NAPAPI StatesCache : public Resource
    {
    RTTI_ENABLE(Resource)
    public:
        /**
         * Add states to the cache, thread safe
         * @param timestamp the timestamp of the states in uint64 YYYYMMDDHHMMSS
         * @param states all states
         */
        void addStates(uint64 timestamp, const std::vector<FlightState>& states);

        /**
         * Get states from the cache between begin and end, thread safe
         * @param begin the begin timestamp in uint64 YYYYMMDDHHMMSS
         * @param end the end timestamp in uint64 YYYYMMDDHHMMSS
         * @param altitude the maximum altitude of the states
         * @param states vector to store the states in
         * @return true if states were found
         */
        bool getStates(uint64 begin, uint64 end, float altitude, std::vector<FlightStates>& states);

        /**
         * Get the most recent timestamp in the cache
         * @return timestamp in uint64 YYYYMMDDHHMMSS
         */
        uint64 getMostRecentTimeStamp(){ return mNewestTimeStamp.load();}

        /**
         * Get the oldest timestamp in the cache
         * @return timestamp in uint64 YYYYMMDDHHMMSS
         */
        uint64 getOldestTimeStamp(){ return mOldestTimeStamp.load();}

        /**
         * Get the closest timestamp to the given timestamp
         * @param timestamp the timestamp to compare to
         * @return the closest timestamp in uint64 YYYYMMDDHHMMSS
         */
        uint64 getClosestTimeStamp(uint64 timestamp);

        int mMaxEntries = 8640; ///< Property: "MaxEntries" - The maximum number of entries in the cache
    private:
        std::mutex mMutex;
        std::map<uint64, FlightStates> mStates;
        std::atomic<uint64> mNewestTimeStamp;
        std::atomic<uint64> mOldestTimeStamp;
    };
}