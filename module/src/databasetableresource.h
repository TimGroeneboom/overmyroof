#pragma once

#include <nap/resource.h>
#include <database.h>
#include <databasetable.h>
#include <rtti/factory.h>

namespace nap
{
    template<typename T>
    class NAPAPI DatabaseTableResource : public Resource
    {
    RTTI_ENABLE(Resource)
    public:
        bool init(utility::ErrorState &errorState)
        {
            mDatabase = std::make_unique<Database>(mDatabaseFactory);
            if (!mDatabase->init(mDatabaseName, errorState))
            {
                return false;
            }

            mDatabaseTable = mDatabase->getOrCreateTable(mTableName, RTTI_OF(T), {}, errorState);
            if(mDatabaseTable == nullptr)
            {
                return false;
            }

            return true;
        }

        std::string mDatabaseName = "test.db";
        std::string mTableName = "test";

        DatabaseTable* getDatabaseTable() { return mDatabaseTable; }
    private:
        std::unique_ptr<Database> mDatabase;
        DatabaseTable* mDatabaseTable = nullptr;
        rtti::Factory mDatabaseFactory;
    };
}














