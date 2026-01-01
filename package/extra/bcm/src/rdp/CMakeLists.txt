# Top level CMake file for creating the build trees

#====
# Indicate the minimum version of CMake required
#====
cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

#====
# cmake must be started in project's target directory
#====
get_filename_component(_TARGET ${CMAKE_BINARY_DIR} NAME)
if(NOT _TARGET STREQUAL "target")
    message(FATAL_ERROR "cmake must be started in project's target directory")
endif()
get_filename_component(_DIR_NO_TARGET ${CMAKE_BINARY_DIR} DIRECTORY)
get_filename_component(_PROJECT ${_DIR_NO_TARGET} NAME)
if(PROJECT)
    if (NOT ${PROJECT} STREQUAL ${_PROJECT})
        message(FATAL_ERROR "project '${PROJECT}' doesn't match the directory cmake is started in '${CMAKE_BINARY_DIR}'")
    endif()
else()
    set(PROJECT ${_PROJECT} CACHE STRING "Project directory" FORCE)
endif()

#====
# Identify where to find the modules
#====
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/tools/cmake/modules;${CMAKE_SOURCE_DIR}/projects/${PROJECT}/target)

#====
# Define the project we are building for
#====
project(bcm968xxx
        LANGUAGES CXX C ASM)

#====
# Set global variables used here
#====
set(SOURCE_TOP ${CMAKE_CURRENT_SOURCE_DIR})
set(BUILD_TOP ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_INSTALL_PREFIX ${BUILD_TOP})

#====
# Include top level CMake modules. All these are found in cmake/modules
#====
include(target_prepare)            # Set project variables and add targets to create project directories
include(xml_utils)                 # Macros and targets for code generation from xml
include(fw_utils)                  # Macros and targets for fw symbolic links creation and coding style check
include(fw_bin_utils)              # Macros and targets for binary fw image generation
