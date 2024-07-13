#pragma once

#include <restfunction.h>
#include <database.h>
#include <databasetable.h>

#include "flightstate.h"

namespace nap
{
    class NAPAPI FetchFlightsCall : public RestFunction
    {
    RTTI_ENABLE(RestFunction)
    public:
        bool init(utility::ErrorState &errorState) override;

        RestResponse call(const std::unordered_map<std::string, std::unique_ptr<APIBaseValue>> &values) override;

        ResourcePtr<FlightStatesTable> mFlightStatesTable;
    private:
        DatabaseTable* mDatabaseTable;
    };
}
