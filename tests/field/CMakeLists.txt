cmake_minimum_required(VERSION 3.16)
project(agt_field_hmi_tests)
set(CMAKE_CXX_STANDARD 17)
find_package(Qt5 COMPONENTS Core Gui Widgets Network REQUIRED)
add_executable(field_hmi_logic_test field_hmi_logic_test.cpp)
target_link_libraries(field_hmi_logic_test Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network)
enable_testing()
add_test(NAME field_hmi_logic COMMAND field_hmi_logic_test)
set_tests_properties(field_hmi_logic PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
find_package(Eigen3 REQUIRED)
find_package(Boost COMPONENTS filesystem system REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDLIMAGE REQUIRED IMPORTED_TARGET SDL2_image)
add_executable(occupancy_export_test occupancy_export_test.cpp
  ../../src/common/logger/logger.cc ../../src/common/logger/easylogging++.cc)
target_include_directories(occupancy_export_test PRIVATE ../../src/basic ../../src/common)
target_link_libraries(occupancy_export_test Qt5::Core Eigen3::Eigen yaml-cpp
  Boost::filesystem Boost::system PkgConfig::SDLIMAGE pthread)
add_test(NAME occupancy_export COMMAND occupancy_export_test)

# Requires a separately started Mock runtime and activated fixture; not a standalone CTest.
add_library(qt_save_probe SHARED qt_save_probe.cpp)
target_link_libraries(qt_save_probe Qt5::Widgets)
