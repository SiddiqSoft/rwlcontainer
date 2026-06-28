#   SiddiqSoft CMakeLists 
#   Copyright 2024 Abdulkareem Siddiq. All rights reserved.
#   See LICENSE file
#
#   When building from our CI server, the following options must be set:
#   - RWLContainer_BUILD_TESTS=ON
#   - CI_BUILDID must be set to the gitversion
# 
cmake_minimum_required(VERSION 3.25)

# _________________
# Version detection; must preceed the declaration of the actual project()
# Figures out the value ${CURRENT_PROJECT_VERSION} and therefore the ${PROJECT_VERSION}
# Brings in the CPM module into pack/CMakeCommonHelpers.cmake
include(pack/CMakeCommonHelpers.cmake)

# ______________________
# Project Metadata block
project(RWLContainer
        VERSION         "${CURRENT_PROJECT_VERSION}"
        LANGUAGES       CXX
        HOMEPAGE_URL    "%%myurl%%"
        DESCRIPTION     "Classes to allow you to build asynchrony in your applications using standard C++ jthread, semaphore, deque")


# ____________________________________
# Options; define and declare defaults
option(${PROJECT_NAME}_BUILD_TESTS "${PROJECT_NAME} - Build tests. Uncheck for install only runs" OFF)

# ____________________________________
#  Library Definition
add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
                            INTERFACE
                            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# Require C++17 minimum (uses std::shared_mutex, std::tuple, std::shared_lock, std::unique_lock)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

# ____________________________________
# Natvis files for debugger visualization
# These files enable enhanced debugging experience in Visual Studio and VS Code
set(NATVIS_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/pack/SiddiqSoft.RWLContainer.natvis"
)

# Add natvis files to the library for IDE integration
target_sources(${PROJECT_NAME} INTERFACE FILE_SET natvis TYPE NATVIS FILES ${NATVIS_FILES})

# ____________________________________
# Dependencies
cpmaddpackage("gh:siddiqsoft/RunOnEnd#1.4.2")
target_link_libraries(${PROJECT_NAME} INTERFACE RunOnEnd::RunOnEnd)

# ____________________________________
# Testing
# Available only when building in our repo; controlled by the switch: ${PROJECT_NAME}_BUILD_TESTS = ON
if(${${PROJECT_NAME}_BUILD_TESTS} AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests" AND IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests" )
    message(STATUS "${PROJECT_NAME} - Asked to build tests.. ${PROJECT_NAME}_BUILD_TESTS = ${${PROJECT_NAME}_BUILD_TESTS}")
    enable_testing()
    add_subdirectory(tests)
endif()
