#pragma once

#include <restfunction.h>
#include <restclient.h>
#include <database.h>
#include <databasetable.h>

#include "statescache.h"
#include "flightstate.h"
#include "pro6ppdescription.h"

namespace nap
{
    class NAPAPI Pro6ppInterface : public RestFunction
    {
        RTTI_ENABLE(RestFunction)
    public:
        ResourcePtr<Pro6ppDescription> mPro6ppDescription;
        ResourcePtr<RestClient> mPro6ppClient;
    };


    class NAPAPI FetchFlightsCall : public Pro6ppInterface
    {
    RTTI_ENABLE(Pro6ppInterface)
    public:
        bool init(utility::ErrorState &errorState) final;

        RestResponse call(const RestValueMap &values) override;

        ResourcePtr<DatabaseTableResource> mFlightStatesDatabase;
        ResourcePtr<StatesCache> mStatesCache;
        int mAddressCacheRetentionDays = 180;
        ResourcePtr<Pro6ppDescription> mPro6ppDescription;
        std::string mFlightStatesTableName = "states";
        std::string mAddressCacheTableName = "addressCache";

        bool getFlights(const RestValueMap &values,
                        std::vector<FlightState> &filteredStates,
                        std::unordered_map<std::string, uint64> &timeStamps,
                        std::unordered_map<std::string, float> &distances,
                        utility::ErrorState& errorState);
    protected:
        DatabaseTable* mDatabaseTable;
    };
}
