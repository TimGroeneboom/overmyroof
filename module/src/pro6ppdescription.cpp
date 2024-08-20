#include "pro6ppdescription.h"

RTTI_BEGIN_CLASS(nap::Pro6ppDescription)
    RTTI_PROPERTY("Pro6ppAddress", &nap::Pro6ppDescription::mPro6ppAddress, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("Pro6ppKeyFile", &nap::Pro6ppDescription::mPro6ppKeyFile, nap::rtti::EPropertyMetaData::Default | nap::rtti::EPropertyMetaData::FileLink)
    RTTI_PROPERTY("Pro6ppPostalCodeDescription", &nap::Pro6ppDescription::mPro6ppPostalCodeDescription, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("Pro6ppStreetNumberAndPremiseDescription", &nap::Pro6ppDescription::mPro6ppStreetNumberAndPremiseDescription, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("Pro6ppAuthKeyDescription", &nap::Pro6ppDescription::mPro6ppAuthKeyDescription, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("Pro6ppLatitudeDescription", &nap::Pro6ppDescription::mPro6ppLatitudeDescription, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("Pro6ppLongitudeDescription", &nap::Pro6ppDescription::mPro6ppLongitudeDescription, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS