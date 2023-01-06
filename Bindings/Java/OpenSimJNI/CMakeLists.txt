set(KIT JavaJNI)
set(UKIT JAVAJNI)

# For building C++ wrapper.
find_package(JNI 1.8 REQUIRED)

# Avoid excessive compiler warnings. We set these COMPILE_OPTIONS in the root
# CMakeLists.txt.
set_directory_properties(PROPERTIES COMPILE_OPTIONS "")

set(SWIG_JAVA_INTERFACE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/../swig/java_simbody.i"
    "${CMAKE_CURRENT_SOURCE_DIR}/../swig/java_common.i"
    "${CMAKE_CURRENT_SOURCE_DIR}/../swig/java_simulation.i"
    "${CMAKE_CURRENT_SOURCE_DIR}/../swig/java_actuators.i"
    "${CMAKE_CURRENT_SOURCE_DIR}/../swig/java_examplecomponents.i"
    "${CMAKE_CURRENT_SOURCE_DIR}/../swig/java_moco.i")
cmake_path(NORMAL_PATH SWIG_JAVA_INTERFACE_FILES)

set_property(SOURCE ${SWIG_JAVA_INTERFACE_FILES} PROPERTY CPLUSPLUS ON)
set_property(SOURCE ${SWIG_JAVA_INTERFACE_FILES} PROPERTY INCLUDE_DIRECTORIES
    "${OpenSim_SOURCE_DIR}"
    "${OpenSim_SOURCE_DIR}/Bindings"
    "${Simbody_INCLUDE_DIR}"
)
set_property(SOURCE ${SWIG_JAVA_INTERFACE_FILES} PROPERTY COMPILE_OPTIONS
    -package ${OPENSIM_JAVA_WRAPPING_PACKAGE} ${SWIG_COMPILE_OPTIONS})

swig_add_library(osim${KIT}
    TYPE SHARED
    LANGUAGE java
    OUTPUT_DIR ${SWIG_JAVA_SOURCE_BUILD_OUTPUT_DIR}
    SOURCES ${SWIG_JAVA_INTERFACE_FILES}
        OpenSimContext.cpp
        OpenSimContext.h
)

set_target_properties(osim${KIT} PROPERTIES SWIG_COMPILE_OPTIONS ${SWIG_COMPILE_OPTIONS})

set(EXPORT_MACRO OSIM${UKIT}_EXPORTS)

target_include_directories(osim${KIT} PRIVATE
                           ${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2})

set_target_properties(osim${KIT} PROPERTIES
   DEFINE_SYMBOL ${EXPORT_MACRO}
   PROJECT_LABEL "Java - osim${KIT}"
   FOLDER "Bindings"
)

# Compiler flags.
if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU" OR
        ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    # We purposefully wrap deprecated functions, so no need to see such warnings.
    set(COMPILE_FLAGS "-Wno-deprecated-declarations")
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "MSVC")
    # Don't warn about:
    # 4996: deprecated functions.
    # 4114: "const const T"
    set(COMPILE_FLAGS "/wd4996 /wd4114")
endif()
set_target_properties(osim${KIT}
        PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS}")

target_link_libraries(osim${KIT} osimTools osimExampleComponents osimMoco)

# RPATH: The osim libraries that osimJavaJNI needs are in the same folder as
# osimJavaJNI.
OpenSimAddInstallRPATHSelf(TARGET osim${KIT} LOADER)
OpenSimAddInstallRPATHSimbody(TARGET osim${KIT} LOADER
    FROM "${CMAKE_INSTALL_LIBDIR}")

# Shared libraries are needed at runtime for applications, so we put them
# at the top level in OpenSim/bin/*.dll (Windows) or OpenSim/lib/*.so (Linux)
# or OpemSim/lib/*.dylib (Mac). Windows .lib files, and Linux/Mac .a static
# archives are only needed at link time so go in sdk/lib.
install(TARGETS osim${KIT}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${OPENSIM_INSTALL_ARCHIVEDIR}")

# Don't install .class files.
install(DIRECTORY ${SWIG_JAVA_SOURCE_BUILD_OUTPUT_DIR}/
    DESTINATION "${OPENSIM_INSTALL_JAVASRCDIR}/${SWIG_JAVA_PACKAGE_PATH}"
    PATTERN "*.class" EXCLUDE
    )

if(BUILD_TESTING)
    subdirs(Test)
endif()
