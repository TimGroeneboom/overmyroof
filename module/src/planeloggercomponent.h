#pragma once

#include <databasetable.h>
#include <database.h>
#include <component.h>
#include <nap/resourceptr.h>
#include <restclient.h>
#include <entity.h>
#include <rtti/factory.h>

#include "flightstate.h"

namespace nap
{
    class PlaneLoggerComponentInstance;

    class NAPAPI PlaneLoggerComponent : public Component
    {
        RTTI_ENABLE(Component)
        DECLARE_COMPONENT(PlaneLoggerComponent, PlaneLoggerComponentInstance)
    public:
        ResourcePtr<RestClient> mRestClient;
        ResourcePtr<FlightStatesTable> mFlightStatesTable;
        float mInterval = 10.0f;
    };

    class NAPAPI PlaneLoggerComponentInstance : public ComponentInstance
    {
    RTTI_ENABLE(ComponentInstance)
    public:
        PlaneLoggerComponentInstance(EntityInstance& entityInstance, Component& component);

        void update(double deltaTime) override;

        bool init(utility::ErrorState &errorState) override;
    private:
        RestClient* mRestClient;
        DatabaseTable* mFlightStatesTable;

        float mInterval = 10.0f;
        double mTime = 0.0;
    };
}