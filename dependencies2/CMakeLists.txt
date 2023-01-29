cmake_minimum_required(VERSION 3.24)

include(FetchContent)

set(SIMBODY_VERSION_TO_USE 3.7)
set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE NEVER)

# Simbody uses the same variable name for building tests. Create a temporary cache variable
# to use for restoring BUILD_TESTING to its original value after overwriting BUILD_TESTING
# to configure the Simbody project.
set(_build_testing ${BUILD_TESTING})
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

set(BUILD_EXAMPLES OFF CACHE BOOL INTERNAL)
set(INSTALL_DOCS OFF CACHE BOOL INTERNAL)

# Setting CMAKE_ARGS or CMAKE_CACHE_ARGS doesn't seem to work (at least for BUILD_TESTING)
FetchContent_Declare(Simbody
    GIT_REPOSITORY https://github.com/halleysfifthinc/simbody.git
    GIT_TAG master
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    SYSTEM
    FIND_PACKAGE_ARGS ${SIMBODY_VERSION_TO_USE}
        PATHS "${SIMBODY_HOME}" NO_MODULE)

FetchContent_MakeAvailable(Simbody)

# Restore BUILD_TESTING to original value
set(BUILD_TESTING ${_build_testing} CACHE BOOL "" FORCE)

set(Simbody_LIBRARIES SimTKcommon SimTKmath SimTKsimbody PARENT_SCOPE)

foreach(mod SimTKcommon SimTKmath SimTKsimbody simbody-visualizer)
    target_compile_options(${mod}
        PRIVATE
            "$<$<C_COMPILER_ID:GNU,Clang>:-w>"
            "$<$<CXX_COMPILER_ID:GNU,Clang>:-w>"
            "$<$<C_COMPILER_ID:MSVC>:/w>"
            "$<$<CXX_COMPILER_ID:MSVC>:/w>"
    )
endforeach()

get_target_property(SimTKcommon_INCLUDE_DIRS SimTKcommon INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(SimTKmath_INCLUDE_DIRS SimTKmath INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(SimTKsimbody_INCLUDE_DIRS SimTKsimbody INTERFACE_INCLUDE_DIRECTORIES)

set(Simbody_INCLUDE_DIR ${SimTKcommon_INCLUDE_DIRS} ${SimTKmath_INCLUDE_DIRS} ${SimTKsimbody_INCLUDE_DIRS} PARENT_SCOPE)

# if(NOT SIMBODY_HOME AND OPENSIM_DEPENDENCIES_DIR)
#     set(SIMBODY_HOME "${OPENSIM_DEPENDENCIES_DIR}/simbody")
# endif()

# # Find Simbody freshly by unsetting this CMake-generated variable.
# unset(Simbody_DIR CACHE)
# if("${SIMBODY_HOME}" STREQUAL "")
#     # We assume the only case in which the user
#     # wants us to search for Simbody is if they left SIMBODY_HOME empty.
#     # If the user specifies an invalid SIMBODY_HOME by accident,
#     # we shouldn't let that fail silently and still search for
#     # Simbody elsewhere; they may never realize
#     # we are not using their requested installation of Simbody.
#     find_package(Simbody ${SIMBODY_VERSION_TO_USE}
#         NO_MODULE)
# else()
#     # Find the package using the user-specified path.
#     # NO_DEFAULT_PATH will cause find_package to only
#     # look in the provided PATHS.
#     find_package(Simbody ${SIMBODY_VERSION_TO_USE}
#         PATHS "${SIMBODY_HOME}" NO_MODULE NO_DEFAULT_PATH)
# endif()
# # This variable appears in the CMake GUI and could confuse users,
# # since this variable can't be changed manually.
# mark_as_advanced(Simbody_DIR)

# # If Simbody is not found, Simbody_FOUND is false.
# if(NOT Simbody_FOUND)
#     message(FATAL_ERROR "
#         Simbody ${SIMBODY_VERSION_TO_USE} not found. Install Simbody and set
#         SIMBODY_HOME to the installation directory of Simbody.")
# endif()

# Unnecessary when building with FetchContent
###########################################
# # Check if Simbody targets were built using same BUILD_TYPE/CONFIGURATION as
# # current OpenSim build. If build/configuration types mismatch, stop the build.
# # ------------------------------------------------------------------------------
# get_property(Simbody_CONFIGURATION
#     TARGET   SimTKcommon
#     PROPERTY IMPORTED_CONFIGURATIONS)
# 
# string(CONCAT MESSAGE
#     "Checking if Simbody was built with same BUILD_TYPE or CONFIGURATION as "
#     "the current OpenSim BUILD_TYPE or CONFIGURATION.")
# 
# if(WIN32)
#     string(TOLOWER "${Simbody_CONFIGURATION}" Simbody_CONFIGURATION_STR)
#     add_custom_target(Simbody_CONFIG_check ALL
#         COMMAND "set" "Simbody_CONFIG=${Simbody_CONFIGURATION_STR}"
#         COMMAND "if"
#            "\"%Simbody_CONFIG:$<LOWER_CASE:$<CONFIG>>=%\"==\"%Simbody_CONFIG%\""
#            "exit" "1"
#         COMMENT ${MESSAGE})
# endif()
###########################################

# Copy Simbody DLLs to build tree.
# --------------------------------
# On Windows, we must copy Simbody libraries to the OpenSim build directory
# so that the tests and examples can run without modifying the PATH
# (that is, put Simbody's dll's in the same directory as OpenSim's
# executables and libraries).
OpenSimCopyDependencyDLLsForWin(DEP_NAME Simbody
    DEP_BIN_DIR "${simbody_BINARY_DIR}")
# if(WIN32)
#     add_dependencies(Copy_Simbody_DLLs Simbody_CONFIG_check)
# endif()
