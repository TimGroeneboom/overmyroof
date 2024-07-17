#include "addresscachedata.h"

RTTI_BEGIN_CLASS(nap::AddressCacheData)
    RTTI_PROPERTY("PostalCode", &nap::AddressCacheData::mPostalCode, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("StreetNumberAndPremise", &nap::AddressCacheData::mStreetNumberAndPremise, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("TimeStamp", &nap::AddressCacheData::mTimeStamp, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("Lat", &nap::AddressCacheData::mLat, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("Lon", &nap::AddressCacheData::mLon, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS
        