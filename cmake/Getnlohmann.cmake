include(FetchContent)
message(STATUS "get nlohmann_json ...")

set(nlohmann_json_GIT_REPOSITORY
    "https://github.com/nlohmann/json.git"
    CACHE STRING "nlohmann_json git repository")

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY ${nlohmann_json_GIT_REPOSITORY}
    GIT_TAG "55f93686c01528224f448c19128836e7df245f72"
    GIT_SHALLOW FALSE)

FetchContent_GetProperties(nlohmann_json)
if(NOT nlohmann_json_POPULATED)
    
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# target: nlohmann_json::nlohmann_json