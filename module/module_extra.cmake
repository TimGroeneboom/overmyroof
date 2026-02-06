# ENABLE DEBUG LOG pre-processor macro for debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DENABLE_DEBUG_LOG)
endif()