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
        // Properties
        std::string mData;
        nap::uint64 mTimeStamp;

        bool ParseData(std::vector<FlightState>& states, utility::ErrorState& errorState);
    };

    using FlightStatesTable = DatabaseTableResource<FlightStatesData>;
}