# Find CUnit - Unit testing framework for C
#
# This module defines
#  CUNIT_INCLUDE_DIRS - where to find CUnit headers
#  CUNIT_LIBRARIES    - List of libraries when using CUnit
#  CUNIT_FOUND       - True if CUnit found

find_path(CUNIT_INCLUDE_DIR NAMES CUnit/CUnit.h)
find_library(CUNIT_LIBRARY NAMES cunit libcunit)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CUnit DEFAULT_MSG
    CUNIT_LIBRARY CUNIT_INCLUDE_DIR)

if(CUNIT_FOUND)
    set(CUNIT_LIBRARIES ${CUNIT_LIBRARY})
    set(CUNIT_INCLUDE_DIRS ${CUNIT_INCLUDE_DIR})
endif()

mark_as_advanced(CUNIT_INCLUDE_DIR CUNIT_LIBRARY) 