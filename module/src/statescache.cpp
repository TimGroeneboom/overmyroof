#include "statescache.h"

RTTI_BEGIN_CLASS(nap::StatesCache)
    RTTI_PROPERTY("MaxEntries", &nap::StatesCache::mMaxEntries, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

namespace nap
{
    bool StatesCache::init(utility::ErrorState &errorState)
    {
        if(!errorState.check(mMaxEntries <= 0, "MaxEntries must be greater than 0"))
            return false;

        return true;
    }


    void StatesCache::addStates(uint64 timestamp, const std::vector<FlightState>& states)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mStates[timestamp].mStates = states;
        mStates[timestamp].mTimeStamp = timestamp;
        mNewestTimeStamp = timestamp;
        while(mStates.size() > mMaxEntries)
            mStates.erase(mStates.begin());
        mOldestTimeStamp = mStates.begin()->first;
    }


    bool StatesCache::getStates(uint64 begin, uint64 end, float altitude, std::vector<FlightStates>& states)
    {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            auto it = mStates.lower_bound(begin);
            while(it != mStates.end() && it->first <= end)
            {
                states.push_back(it->second);
                ++it;
            }
        }

        if(altitude <= 0)
        {
            return true;
        }

        for(auto& state : states)
        {
            // remove all flight below the altitude
            auto itr = std::find_if(state.mStates.begin(), state.mStates.end(), [altitude](const FlightState& flight){ return flight.mAltitude > altitude; });
            if(itr != state.mStates.end())
            {
                state.mStates.erase(itr, state.mStates.end());
            }
        }

        return true;
    }


    uint64 StatesCache::getClosestTimeStamp(uint64 timestamp)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        auto it = mStates.lower_bound(timestamp);
        if(it != mStates.end())
        {
            return it->first;
        }
        return 0;
    }
}
