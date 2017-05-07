cmake_minimum_required(VERSION 3.1)

project(QNvFanControl)

# Set default build type as Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_CXX_STANDARD 11)

find_package(PkgConfig REQUIRED)
find_package(Qt5Core)

pkg_check_modules(LIBNOTIFY_PKGCONFIG REQUIRED libnotify)
pkg_check_modules(X11_PKGCONFIG REQUIRED x11)

set(FILES
    "Main.cpp"
    "Notify.cpp"
    "Notify.hpp"
    "Nvidia.cpp"
    "Nvidia.hpp")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang") # GCC or Clang
    option(USE_LINK_TIME_OPTIMIZATION "Enable link time optimization for release builds" ON)

    set(CMAKE_CXX_FLAGS_RELEASE    "-fno-exceptions ${CMAKE_CXX_FLAGS_RELEASE}")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-fno-exceptions ${CMAKE_CXX_FLAGS_MINSIZEREL}")

    if(USE_LINK_TIME_OPTIMIZATION)
        set(CMAKE_CXX_FLAGS_RELEASE    "${CMAKE_CXX_FLAGS_RELEASE} -flto")
        set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_RELEASE} -flto")
    endif()

    if(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
        string(REPLACE "-rdynamic" "" CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS ${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS})
    endif()
endif()

add_executable(${PROJECT_NAME}
    ${FILES})

link_directories(${PROJECT_NAME}
    ${X11_PKGCONFIG_LIBRARY_DIRS}
    ${LIBNOTIFY_PKGCONFIG_LIBRARY_DIRS})
include_directories(${PROJECT_NAME}
    ${X11_PKGCONFIG_INCLUDE_DIRS}
    ${LIBNOTIFY_PKGCONFIG_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}
    ${X11_PKGCONFIG_LIBRARIES}
    ${LIBNOTIFY_PKGCONFIG_LIBRARIES}
    "XNVCtrl"
    Qt5::Core)

# Installation
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
