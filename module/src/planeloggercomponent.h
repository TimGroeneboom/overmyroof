#pragma once

#include <databasetable.h>
#include <database.h>
#include <component.h>
#include <nap/resourceptr.h>
#include <restclient.h>
#include <entity.h>
#include <rtti/factory.h>
#include <statescache.h>

#include "flightstate.h"
#include "rect.h"

namespace nap
{

    // Forward declarations
    class PlaneLoggerComponentInstance;


    class NAPAPI PlaneLoggerComponent : public Component
    {
        RTTI_ENABLE(Component)
        DECLARE_COMPONENT(PlaneLoggerComponent, PlaneLoggerComponentInstance)
    public:
        ResourcePtr<RestClient> mRestClient;
        ResourcePtr<DatabaseTableResource> mFlightStatesDatabase;
        ResourcePtr<StatesCache> mStatesCache;
        std::string mFlightStatesTableName = "states";
        float mInterval = 10.0f;
        int mRetainHours = 768;
        int mCacheHours = 24;
        std::string mAdress = "/zones/fcgi/feed.js";
        glm::vec4 mBounds = {53.445884435606054, 50.749405057563486, 3.5163031843031223, 7.9136148705580505};
    };

    class NAPAPI PlaneLoggerComponentInstance : public ComponentInstance
    {
    RTTI_ENABLE(ComponentInstance)
    public:
        PlaneLoggerComponentInstance(EntityInstance& entityInstance, Component& component);

        void update(double deltaTime) override;

        bool init(utility::ErrorState &errorState) override;

        void clear();
    private:
        RestClient* mRestClient;
        DatabaseTable* mFlightStatesTable;
        StatesCache* mStatesCache;

        float mInterval = 10.0f;
        double mTime = 0.0;
        int mRetainHours = 768;
        int mCacheHours = 24;
        std::string mFlightStatesTableName = "states";
        bool mQuerying = false;
        std::string mAddress = "/zones/fcgi/feed.js";
        glm::vec4 mBounds = {53.445884435606054, 50.749405057563486, 3.5163031843031223, 7.9136148705580505};
    };
}