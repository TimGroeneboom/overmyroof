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
    /**
     * Pro6pp interface contains the Pro6pp description and the Pro6pp client
     * Pro6pp client is used to fetch the lat, lon from the postal code and street number premise
     */
    class NAPAPI Pro6ppInterface : public RestFunction
    {
        RTTI_ENABLE(RestFunction)
    public:
        ResourcePtr<Pro6ppDescription> mPro6ppDescription; ///< Property "Pro6ppDescription" : Pro6pp description
        ResourcePtr<RestClient> mPro6ppClient; ///< Property "Pro6ppClient" : Pro6pp client
    };

    /**
     * FetchFlightsCall is a RestFunction that fetches the flights from the database and/or cache
     * It can also fetches the lat, lon from the postal code and street number premise using Pro6pp client
     * which makes a call to the Pro6pp API
     * Lat, lon coordinates are cached in the address cache
     */
    class NAPAPI FetchFlightsCall : public Pro6ppInterface
    {
    RTTI_ENABLE(Pro6ppInterface)
    public:
        bool init(utility::ErrorState &errorState) final;

        RestResponse call(const RestValueMap &values) override;

        bool getFlights(const RestValueMap &values,
                        std::vector<FlightState> &filteredStates,
                        std::unordered_map<std::string, uint64> &timeStamps,
                        std::unordered_map<std::string, float> &distances,
                        utility::ErrorState& errorState);

        // Properties
        ResourcePtr<DatabaseTableResource> mFlightStatesDatabase; ///< Property "FlightStatesDatabase" : Flight states database
        ResourcePtr<StatesCache> mStatesCache; ///< Property "StatesCache" : States cache
        int mAddressCacheRetentionDays = 180; ///< Property "AddressCacheRetentionDays" : Address cache retention days
        std::string mFlightStatesTableName = "states"; ///< Property "FlightStatesTableName" : Flight states table name
        std::string mAddressCacheTableName = "addressCache"; ///< Property "AddressCacheTableName" : Address cache table name
        int mMaxDurationHours = 48; ///< Property "MaxDurationHours" : Maximum duration in hours to search for flights
    protected:
        DatabaseTable* mDatabaseTable;
        std::string mPro6ppKey;
    };
}
