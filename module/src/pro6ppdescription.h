#pragma once

#include <nap/core.h>
#include <nap/resource.h>

namespace nap
{
    class NAPAPI Pro6ppDescription : public Resource
    {
    RTTI_ENABLE(Resource)
    public:

        std::string mPro6ppAddress = "https://api.pro6pp.nl/v1/autocomplete";
        std::string mPro6ppKeyFile = "pro6pp.key";
        std::string mPro6ppPostalCodeDescription = "postalCode";
        std::string mPro6ppStreetNumberAndPremiseDescription = "streetNumberAndPremise";
        std::string mPro6ppAuthKeyDescription = "authKey";
        std::string mPro6ppLatitudeDescription = "lat";
        std::string mPro6ppLongitudeDescription = "lng";
    };
}