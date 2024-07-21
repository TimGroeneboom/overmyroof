#include "utils.h"
#include <iomanip>

namespace nap
{
    namespace utility
    {
        bool dateTimeFromUINT64(uint64 timestamp, DateTime& dt, utility::ErrorState errorState)
        {
            std::tm t{};
            std::istringstream ss(std::to_string(timestamp));

            ss >> std::get_time(&t, "%Y%m%d%H%M%S");
            if(!errorState.check(!ss.fail(), "failed to parse time string"))
                return false;

            dt = DateTime(std::chrono::system_clock::from_time_t(mktime(&t)));

            return true;
        }
    }
}