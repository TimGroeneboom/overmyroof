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
        rapidjson::Document d(rapidjson::kObjectType);
        d.Parse(mData.c_str());
        for (rapidjson::Value::ConstMemberIterator p = d.MemberBegin(); p != d.MemberEnd(); ++p)
        {
            if(p->value.IsArray())
            {
                for (rapidjson::Value::ConstValueIterator q = p->value.Begin(); q != p->value.End(); ++q)
                {
                    for(auto m = q->MemberBegin(); m != q->MemberEnd(); ++m)
                    {
                        if(m->value.IsArray())
                        {
                            FlightState state;
                            state.mLatitude = m->value[0].GetFloat();
                            state.mLongitude = m->value[1].GetFloat();
                            state.mAltitude = m->value[2].GetFloat();
                            state.mICAO = m->value[3].GetString();
                            state.mRegistration = m->value[4].GetString();
                            state.mAircraftType = m->value[5].GetString();
                            states.push_back(state);
                        }
                    }
                }
            }
        }

        return true;
    }
}