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
  GIT_REPOSITORY ${websocketpp_GIT_REPOSITORY}
  GIT_TAG ${websocketpp_GIT_TAG}
  GIT_SHALLOW FALSE)

FetchContent_GetProperties(websocketpp)
if(NOT websocketpp_POPULATED)
  FetchContent_MakeAvailable(websocketpp)
endif()

set(WEBSOCKETPP_INCLUDE_DIRS ${websocketpp_SOURCE_DIR} CACHE PATH "websocketpp include directory") 