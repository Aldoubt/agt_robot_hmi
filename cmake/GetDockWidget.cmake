include(FetchContent)
message(STATUS "get dockwidget ...")

set(dockwidget_GIT_REPOSITORY
    "https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System.git"
    CACHE STRING "dockwidget git repository")

FetchContent_Declare(
    dockwidget
    URL "https://codeload.github.com/githubuser0xFFFF/Qt-Advanced-Docking-System/tar.gz/a16d17a8bf375127847ac8f40af1ebcdb841b13c"
    URL_HASH SHA256=3a18ad2b2cfa521882f942ec02031718c0efad7f62ad3d39749bce7984974b13)

FetchContent_GetProperties(dockwidget)
if(NOT dockwidget_POPULATED)
    set(QT_VERSION_MAJOR 5 CACHE STRING "Qt version major" FORCE)
    set(BUILD_STATIC TRUE CACHE BOOL "Build static library" FORCE)
    set(ADS_VERSION "4.4.0" CACHE STRING "Version" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "Build examples" FORCE)
    FetchContent_MakeAvailable(dockwidget)
    add_library(dockwidget::dockwidget ALIAS "qtadvanceddocking-qt${QT_VERSION_MAJOR}")
endif()

# target: dockwidget::dockwidget