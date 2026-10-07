include(FetchContent)
message(STATUS "get nlohmann_json ...")

set(nlohmann_json_GIT_REPOSITORY
    "https://github.com/nlohmann/json.git"
    CACHE STRING "nlohmann_json git repository")

FetchContent_Declare(
    nlohmann_json
    URL "https://codeload.github.com/nlohmann/json/tar.gz/55f93686c01528224f448c19128836e7df245f72"
    URL_HASH SHA256=67f4cdd9ca930c9c1e130af4a437c7fc98fab77a2846fc2d2a14b4943831f8ef)

FetchContent_GetProperties(nlohmann_json)
if(NOT nlohmann_json_POPULATED)
    
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# target: nlohmann_json::nlohmann_json