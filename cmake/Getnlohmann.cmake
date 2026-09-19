message(STATUS "locating nlohmann_json ...")

# The HMI uses NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT, which is not
# available in the old 3.10.x package shipped by Ubuntu 22.04. Require a
# recent package so CMake does not silently select an incompatible system
# target.
find_package(nlohmann_json 3.11 CONFIG QUIET)
if(TARGET nlohmann_json::nlohmann_json)
    message(STATUS "using installed nlohmann_json ${nlohmann_json_VERSION}")
    return()
endif()

include(FetchContent)

# Reuse a source tree already fetched in this ROS workspace. This keeps a
# fresh HMI configure independent of GitHub TLS/network availability. It can
# also be overridden explicitly with -DAGT_NLOHMANN_JSON_SOURCE_DIR=....
get_filename_component(AGT_WORKSPACE_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(AGT_NLOHMANN_JSON_SOURCE_DIR
    "${AGT_WORKSPACE_ROOT}/build/agt_robot_hmi/_deps/nlohmann_json-src"
    CACHE PATH "Local nlohmann_json source tree")

if(EXISTS "${AGT_NLOHMANN_JSON_SOURCE_DIR}/CMakeLists.txt")
    message(STATUS "using local nlohmann_json source: ${AGT_NLOHMANN_JSON_SOURCE_DIR}")
    FetchContent_Declare(
        nlohmann_json
        SOURCE_DIR "${AGT_NLOHMANN_JSON_SOURCE_DIR}")
else()
    message(STATUS "nlohmann_json was not found locally; falling back to FetchContent")

    set(nlohmann_json_GIT_REPOSITORY
        "https://github.com/nlohmann/json.git"
        CACHE STRING "nlohmann_json git repository")

    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY ${nlohmann_json_GIT_REPOSITORY}
        GIT_TAG "v3.12.0"
        GIT_SHALLOW TRUE)
endif()

FetchContent_GetProperties(nlohmann_json)
if(NOT nlohmann_json_POPULATED)
    
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# target: nlohmann_json::nlohmann_json
