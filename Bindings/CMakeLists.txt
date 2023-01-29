if(BUILD_PYTHON_WRAPPING OR BUILD_JAVA_WRAPPING)
    if(POLICY CMP0078)
        cmake_policy(SET CMP0078 NEW)
    endif()

    if(POLICY CMP0086)
        cmake_policy(SET CMP0086 NEW)
    endif()

    set(swig_components)
    if(BUILD_PYTHON_WRAPPING)
        list(APPEND swig_components python)
    endif()
    if(BUILD_JAVA_WRAPPING)
        list(APPEND swig_components java)
    endif()

    find_package(SWIG 4.0.2 REQUIRED COMPONENTS ${swig_components})
    include(UseSWIG)

    set(UseSWIG_MODULE_VERSION 2)
    set(SWIG_USE_SWIG_DEPENDENCIES ON)
    set(SWIG_USE_TARGET_INCLUDE_DIRECTORIES ON)
endif()

# Flags are both Python and Java bindings will use.
if(WITH_EZC3D)
    set(SWIG_COMPILE_OPTIONS "-DWITH_EZC3D")
endif()
#flag to indicate whether to let SWIG process Doxygen comments
# Will be on for releases but too many warnings/errors for regular dev.
set(SWIG_DOXYGEN ON CACHE BOOL "Carry Doxygen comments to bindings")
mark_as_advanced(SWIG_DOXYGEN)
set(SWIG_DOXYGEN_STRING "")
if (SWIG_DOXYGEN)
    set(SWIG_DOXYGEN_STRING "-doxygen")
    list(APPEND SWIG_COMPILE_OPTIONS ${SWIG_DOXYGEN_STRING})
endif()

if(BUILD_PYTHON_WRAPPING)
    add_subdirectory(Python)
endif()
if(BUILD_JAVA_WRAPPING)
    add_subdirectory(Java)
endif()
