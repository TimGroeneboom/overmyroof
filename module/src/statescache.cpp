#include "statescache.h"

RTTI_BEGIN_CLASS(nap::StatesCache)
    RTTI_PROPERTY("MaxEntries", &nap::StatesCache::mMaxEntries, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

namespace nap
{
    void StatesCache::addStates(uint64 timestamp, const std::vector<FlightState>& states)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mStates[timestamp].mStates = states;
        mStates[timestamp].mTimeStamp = timestamp;
        mNewestTimeStamp = timestamp;
        while(mStates.size() > mMaxEntries)
        {
            mStates.erase(mOldestTimeStamp);
            mOldestTimeStamp = mStates.begin()->first;
        }
        mOldestTimeStamp = mStates.begin()->first;
    }


    bool StatesCache::getStates(uint64 begin, uint64 end, std::vector<FlightStates>& states)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        auto it = mStates.lower_bound(begin);
        while(it != mStates.end() && it->first <= end)
        {
            states.push_back(it->second);
            ++it;
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
