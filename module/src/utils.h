#pragma once

#include <nap/core.h>
#include <nap/datetime.h>

namespace nap
{
    namespace utility
    {
        bool NAPAPI dateTimeFromUINT64(uint64 timestamp, DateTime& dt, utility::ErrorState errorState);
    }
}