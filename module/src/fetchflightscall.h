#pragma once

#include <restfunction.h>
#include <restclient.h>
#include <database.h>
#include <databasetable.h>

#include "statescache.h"
#include "flightstate.h"


namespace nap
{
    class NAPAPI FetchFlightsCall : public RestFunction
    {
    RTTI_ENABLE(RestFunction)
    public:
        bool init(utility::ErrorState &errorState) override;

        RestResponse call(const std::unordered_map<std::string, std::unique_ptr<APIBaseValue>> &values) override;

        ResourcePtr<DatabaseTableResource> mFlightStatesDatabase;
        ResourcePtr<StatesCache> mStatesCache;
        ResourcePtr<RestClient> mPro6ppClient;
        int mAdressCacheRetentionDays = 180;
        std::string mFlightStatesTableName = "states";
        std::string mAddressCacheTableName = "addressCache";
        std::string mPro6ppAddress = "https://api.pro6pp.nl/v1/autocomplete";
        std::string mPro6ppKeyFile = "pro6pp.key";
        std::string mPro6ppPostalCodeDescription = "postalCode";
        std::string mPro6ppStreetNumberAndPremiseDescription = "streetNumberAndPremise";
        std::string mPro6ppAuthKeyDescription = "authKey";
        std::string mPro6ppLatitudeDescription = "lat";
        std::string mPro6ppLongitudeDescription = "lng";
    private:
        DatabaseTable* mDatabaseTable;
    };
}
