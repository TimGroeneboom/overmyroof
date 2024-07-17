#pragma once

#include <nap/resourceptr.h>
#include <nap/numeric.h>
#include <databasetableresource.h>

namespace nap
{
    class NAPAPI AddressCacheData : public rtti::Object
    {
    RTTI_ENABLE(rtti::Object)
    public:
        std::string mPostalCode;
        std::string mStreetNumberAndPremise;
        nap::uint64 mTimeStamp;
        float mLat;
        float mLon;
    };
}