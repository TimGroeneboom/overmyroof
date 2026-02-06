#pragma once

#ifdef ENABLE_DEBUG_LOG
#define DEBUG_LOG(...) nap::Logger::info(__VA_ARGS__)
#else
#define DEBUG_LOG(...)
#endif