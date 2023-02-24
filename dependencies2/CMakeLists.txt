cmake_minimum_required(VERSION 3.24)

include(FetchContent)
set(fetch_libraries)

FetchContent_Declare(docopt
    GIT_REPOSITORY https://github.com/docopt/docopt.cpp.git
    GIT_TAG 3dd23e3280f213bacefdf5fcb04857bf52e90917
    GIT_PROGRESS ON
    SYSTEM
    FIND_PACKAGE_ARGS # NAMES libdocopt-dev
    )
list(APPEND fetch_libraries docopt)

FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.4.1
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    CMAKE_CACHE_ARGS -DSPDLOG_INSTALL:BOOL=ON
    SYSTEM
    FIND_PACKAGE_ARGS # NAMES libspdlog-dev
    )
list(APPEND fetch_libraries spdlog)

## EZC3D
if(${OPENSIM_C3D_PARSER} STREQUAL "ezc3d")
    set(BUILD_EXAMPLE OFF CACHE BOOL INTERNAL) # ezc3d

    FetchContent_Declare(ezc3d
        GIT_REPOSITORY https://github.com/pyomeca/ezc3d.git
        GIT_TAG Release_1.5.1
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        SYSTEM)
    list(APPEND fetch_libraries ezc3d)
endif()


## SIMBODY
set(SIMBODY_VERSION_TO_USE 3.7)

# Simbody uses the same variable name for building tests. Create a temporary cache variable
# to use for restoring BUILD_TESTING to its original value after overwriting BUILD_TESTING
# to configure the Simbody project.
set(_build_testing ${BUILD_TESTING})
set(BUILD_TESTING OFF CACHE BOOL "" FORCE) # Simbody

set(BUILD_EXAMPLES OFF CACHE BOOL INTERNAL) # Simbody
set(INSTALL_DOCS OFF CACHE BOOL INTERNAL) # Simbody

# Setting CMAKE_ARGS or CMAKE_CACHE_ARGS doesn't seem to work (at least for BUILD_TESTING)
FetchContent_Declare(Simbody
    GIT_REPOSITORY https://github.com/halleysfifthinc/simbody.git
    GIT_TAG master
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    SYSTEM
    FIND_PACKAGE_ARGS ${SIMBODY_VERSION_TO_USE}
        GLOBAL PATHS "${SIMBODY_HOME}" NO_MODULE)
list(APPEND fetch_libraries Simbody)

FetchContent_MakeAvailable(${fetch_libraries})

# Restore BUILD_TESTING to original value
set(BUILD_TESTING ${_build_testing} CACHE BOOL "" FORCE)

set(docopt PARENT_SCOPE)
set(spdlog PARENT_SCOPE)
set_property(TARGET spdlog PROPERTY POSITION_INDEPENDENT_CODE ON)
install(TARGETS spdlog EXPORT OpenSimTargets)

foreach(mod docopt spdlog)
    target_compile_options(${mod}
        PRIVATE
            "$<$<C_COMPILER_ID:GNU,Clang>:-w>"
            "$<$<CXX_COMPILER_ID:GNU,Clang>:-w>"
            "$<$<C_COMPILER_ID:MSVC>:/w>"
            "$<$<CXX_COMPILER_ID:MSVC>:/w>"
    )
endforeach()

if("${OPENSIM_C3D_PARSER}" STREQUAL "ezc3d")
    target_compile_options(ezc3d
        PRIVATE
            "$<$<C_COMPILER_ID:GNU,Clang>:-w>"
            "$<$<CXX_COMPILER_ID:GNU,Clang>:-w>"
            "$<$<C_COMPILER_ID:MSVC>:/w>"
            "$<$<CXX_COMPILER_ID:MSVC>:/w>"
    )

    set(ezc3d_LIBRARY ezc3d PARENT_SCOPE)
endif()

if(${Simbody_DIR} STREQUAL ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR})
    # Silence build warnings from Simbody files
    foreach(mod SimTKcommon SimTKmath SimTKsimbody simbody-visualizer)
        if(TARGET ${mod})
            target_compile_options(${mod}
                PRIVATE
                    "$<$<C_COMPILER_ID:GNU,Clang>:-w>"
                    "$<$<CXX_COMPILER_ID:GNU,Clang>:-w>"
                    "$<$<C_COMPILER_ID:MSVC>:/w>"
                    "$<$<CXX_COMPILER_ID:MSVC>:/w>"
            )
        endif()
    endforeach()

    set(Simbody_LIBRARIES SimTKcommon SimTKmath SimTKsimbody PARENT_SCOPE)
    get_target_property(SimTKcommon_INCLUDE_DIRS SimTKcommon INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(SimTKmath_INCLUDE_DIRS SimTKmath INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(SimTKsimbody_INCLUDE_DIRS SimTKsimbody INTERFACE_INCLUDE_DIRECTORIES)
    set(Simbody_INCLUDE_DIR ${SimTKcommon_INCLUDE_DIRS} ${SimTKmath_INCLUDE_DIRS} ${SimTKsimbody_INCLUDE_DIRS} PARENT_SCOPE)
else()
    set(Simbody_LIBRARIES ${Simbody_LIBRARIES} PARENT_SCOPE)
    set(Simbody_INCLUDE_DIR ${Simbody_INCLUDE_DIR} PARENT_SCOPE)
endif()

# Copy Simbody DLLs to build tree.
# --------------------------------
# On Windows, we must copy Simbody libraries to the OpenSim build directory
# so that the tests and examples can run without modifying the PATH
# (that is, put Simbody's dll's in the same directory as OpenSim's
# executables and libraries).
# OpenSimCopyDependencyDLLsForWin(DEP_NAME Simbody
#     DEP_BIN_DIR "${simbody_BINARY_DIR}")

