# FindCCache
# ----------
#
# Find ccache executable. https://ccache.dev/manual/latest.html
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables:
#
# ``CCACHE_FOUND``
#
# ``CCACHE_EXECUTABLE``
#
# ``CCACHE_VERSION``
#
include(FindPackageHandleStandardArgs)

function(validate_ccache validator_result_var item)
    try_compile(validator_result_var
        SOURCE_FROM_CONTENT test.cpp "int main() { return 0; }"
        NO_CACHE
        CMAKE_FLAGS -DCMAKE_CXX_COMPILER_LAUNCHER=${item}
        )
    set(${validator_result_var} PARENT_SCOPE)
endfunction()

find_program(CCACHE_EXECUTABLE ccache
    VALIDATOR validate_ccache)
mark_as_advanced(CCACHE_EXECUTABLE)

if(EXISTS "${CCACHE_EXECUTABLE}")
  execute_process(COMMAND ${CCACHE_EXECUTABLE} --version OUTPUT_VARIABLE CCACHE_VERSION)
  string(REGEX REPLACE "ccache version ([^\n ]+).*" "\\1" CCACHE_VERSION "${CCACHE_VERSION}")
endif()

find_package_handle_standard_args(CCache
  REQUIRED_VARS
    CCACHE_EXECUTABLE
  VERSION_VAR
    CCACHE_VERSION
)
