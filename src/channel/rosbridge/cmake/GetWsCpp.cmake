include(FetchContent)

message(STATUS "get websocketpp ...")

set(websocketpp_GIT_TAG
    "56123c87598f8b1dd471be83ca841ceae07f95ba"
    CACHE STRING "websocketpp git tag")

set(websocketpp_GIT_REPOSITORY 
   "https://github.com/zaphoyd/websocketpp.git"
   CACHE STRING "websocketpp git repository")

FetchContent_Declare(
    websocketpp
    URL "https://codeload.github.com/zaphoyd/websocketpp/tar.gz/56123c87598f8b1dd471be83ca841ceae07f95ba"
    URL_HASH SHA256=56a040dd99e7e58e0513204030e5af5e94750a3d80b176b16bf8e3a837b40f0a)

FetchContent_GetProperties(websocketpp)
if(NOT websocketpp_POPULATED)
  FetchContent_MakeAvailable(websocketpp)
endif()

set(WEBSOCKETPP_INCLUDE_DIRS ${websocketpp_SOURCE_DIR} CACHE PATH "websocketpp include directory") 