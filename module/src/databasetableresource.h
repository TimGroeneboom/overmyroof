#pragma once

#include <nap/resource.h>
#include <database.h>
#include <databasetable.h>
#include <rtti/factory.h>

namespace nap
{
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

            return true;
        }

        std::string mDatabaseName = "test.db";

        template<typename T>
        DatabaseTable* getDatabaseTable(const std::string& tableName);
    private:
        std::unique_ptr<Database> mDatabase;
        rtti::Factory mDatabaseFactory;
        std::unordered_map<std::string, DatabaseTable*> mTables;
    };

    template<typename T>
    DatabaseTable* DatabaseTableResource::getDatabaseTable(const std::string& tableName)
    {
        auto it = mTables.find(tableName);
        if(it != mTables.end())
        {
            return it->second;
        }


        utility::ErrorState error_state;
        DatabaseTable* table = mDatabase->getOrCreateTable(tableName, RTTI_OF(T), {}, error_state);
        mTables[tableName] = table;
        return table;
    }
}














