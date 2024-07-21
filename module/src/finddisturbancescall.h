#pragma once

#include <restfunction.h>
#include <restclient.h>
#include <database.h>
#include <databasetable.h>

#include "fetchflightscall.h"

namespace nap
{
    class NAPAPI FindDisturbancesCall : public RestFunction
    {
    RTTI_ENABLE(RestFunction)
    public:
        bool init(utility::ErrorState &errorState) final;

        RestResponse call(const RestValueMap &values) override;

        ResourcePtr<FetchFlightsCall> mFetchFlightsCall;
        int mMaxPeriod = 2880; ///< Property "MaxPeriod" : Maximum period in minutes to search for disturbances
        int mMinPeriod = 10; ///< Property "MinPeriod" : Minimum period in minutes to search for disturbances
    private:

    };
}
