#include "flightstate.h"
#include "nap/logger.h"

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>

RTTI_BEGIN_CLASS(nap::FlightStatesData)
    RTTI_PROPERTY("TimeStamp", &nap::FlightStatesData::mTimeStamp, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("Data", &nap::FlightStatesData::mData, nap::rtti::EPropertyMetaData::Default)
RTTI_END_STRUCT

RTTI_BEGIN_CLASS(nap::FlightStatesTable)
    RTTI_PROPERTY("DatabaseName", &nap::FlightStatesTable::mDatabaseName, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("TableName", &nap::FlightStatesTable::mTableName, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

namespace nap
{
    bool FlightStatesData::ParseData(std::vector<FlightState>& states, utility::ErrorState& errorState)
    {
        FlightState state;
        bool found_flights = false;

        rapidjson::Document d(rapidjson::kObjectType);
        d.Parse(mData.c_str());
        for (rapidjson::Value::ConstMemberIterator p = d.MemberBegin(); p != d.MemberEnd(); ++p)
        {
            std::string name = p->name.GetString();
            // strip empty spaces
            name.erase(std::remove_if(name.begin(), name.end(), ::isspace), name.end());

            if(p->value.IsArray() && name == "flights")
            {
                found_flights = true;
                for (rapidjson::Value::ConstValueIterator q = p->value.Begin(); q != p->value.End(); ++q)
                {
                    if( q->HasMember("icao") &&
                        q->HasMember("aircraft_type") &&
                        q->HasMember("reg") &&
                        q->HasMember("lat") &&
                        q->HasMember("lon") &&
                        q->HasMember("altitude"))
                    {
                        state.mICAO = q->FindMember("icao")->value.GetString();
                        state.mRegistration = q->FindMember("reg")->value.GetString();
                        state.mAircraftType = q->FindMember("aircraft_type")->value.GetString();
                        state.mLatitude = q->FindMember("lat")->value.GetFloat();
                        state.mLongitude = q->FindMember("lon")->value.GetFloat();
                        state.mAltitude = q->FindMember("altitude")->value.GetFloat();
                        states.push_back(state);
                    }else
                    {
                        errorState.fail("Missing required fields in flight data");
                        return false;
                    }
                }
            }
        }

        if(!found_flights)
        {
            errorState.fail("No flights found in data");
            return false;
        }

        return true;
    }
}