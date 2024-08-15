#pragma once

#include <nap/resourceptr.h>
#include <nap/numeric.h>
#include <databasetableresource.h>

namespace nap
{
    struct NAPAPI FlightState
    {
    public:
        // Properties
        std::string mICAO;
        std::string mRegistration;
        std::string mAircraftType;
        float mLatitude;
        float mLongitude;
        float mAltitude;
    };


    class NAPAPI FlightStatesData : public rtti::Object
    {
    RTTI_ENABLE(rtti::Object)
    public:
        static constexpr const char* kTimeStampPropertyName = "TimeStamp";

        // Properties
        std::string mData;
        nap::uint64 mTimeStamp;

        bool ParseData(std::vector<FlightState>& states, float altitude, utility::ErrorState& errorState) const;
    };
}