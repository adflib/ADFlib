
cmake_minimum_required ( VERSION 3.11 )

# version details - use utils/bump-version to update it
set ( ADFLIB_VERSION 0.10.4 )
set ( ADFLIB_DATE 2025-09-03 )

project ( adflib
    VERSION      ${ADFLIB_VERSION}
    DESCRIPTION  "A free, portable and open implementation of the Amiga filesystem"
    HOMEPAGE_URL "https://gitlab.com/adflib/ADFlib"
    LANGUAGES    C
)

message ( STATUS "Building version: ${PROJECT_VERSION}, ${ADFLIB_DATE}" )

set ( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake )

# setting NDEBUG
# ( https://lists.debian.org/debian-mentors/2018/04/msg00244.html )
#set ( CMAKE_BUILD_TYPE Release )

message ( STATUS "Compiler ID: ${CMAKE_C_COMPILER_ID}" )


#
# Compile options
#

add_compile_options (
    -Wall
    $<$<CONFIG:Release>:-O2>
#    -DADFLIB_VERSION="${ADFLIB_VERSION}"
#    -DADFLIB_DATE="${ADFLIB_DATE}"
)

if ( NOT "${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC" )
    message ( STATUS "Setting additional compiler options/checks (mainly for gcc and clang)" )
    add_compile_options (
      -Wextra
      -Wconversion
      -Wsign-conversion
      -pedantic
      -Werror-implicit-function-declaration
      -Werror=format-security
    #  -pedantic-errors
    #  $<$<CONFIG:Debug>:-g3>
    #  $<$<CONFIG:Debug>:-Og>
      $<$<CONFIG:Debug>:-ggdb>
    #  $<$<CONFIG:Debug>:-pg>
    )
endif()

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "AppleClang")
    message ( STATUS "Setting additional clang options/checks" )
    add_compile_options (
      -Wno-gnu-statement-expression-from-macro-expansion
    )
endif()

#
# Address sanitizer
#

option ( ADFLIB_ENABLE_ADDRESS_SANITIZER "Enable address sanitizer" OFF )

if ( ADFLIB_ENABLE_ADDRESS_SANITIZER )

    if ( "${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" AND NOT MINGW AND NOT CYGWIN )
        message ( STATUS "Enabling GNU address sanitizer" )
        add_compile_options ( -fsanitize=address )
        add_link_options (
            -fsanitize=address
	# for library need either this or LD_PRELOAD (-lasan does not work )
            -static-libasan
	)
    endif()

    if ( MINGW OR CYGWIN )
        message ( STATUS "No address sanitizer for CygWin or MinGW (not available)")
    endif()

    if ( "${CMAKE_C_COMPILER_ID}" MATCHES ".*Clang" )
        message ( STATUS "Enabling Clang address sanitizer" )
        # https://releases.llvm.org/10.0.0/tools/clang/docs/AddressSanitizer.html
        add_compile_options ( -fsanitize=address )
        add_link_options ( -fsanitize=address )
    endif()

    if ( "${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC" )
        # https://learn.microsoft.com/en-us/cpp/sanitizers/asan?view=msvc-170
        message ( STATUS "Enabling MSVC address sanitizer" )
        add_compile_options ( /fsanitize=address /Zi )
        add_link_options ( /fsanitize=address /Zi)
    endif()

else()
    message ( STATUS "Address sanitizer disabled - not configuring" )
endif()

#
# Build type options
#
add_compile_definitions ( BUILDING_WITH_CMAKE=1 )

option ( ADFLIB_BUILD_DLL "Build Windows DLL" OFF )
if ( ADFLIB_BUILD_DLL )
#if(BUILD_SHARED_LIBS)
    message ( STATUS "Building a Windows DLL" )
    add_definitions ( -DBUILD_DLL )
endif ( ADFLIB_BUILD_DLL )


#
# Check available APIs
#

include ( CheckFunctionExists )
include ( CheckSymbolExists )


# Check filesystem functions

check_symbol_exists ( chmod "sys/stat.h" HAVE_CHMOD )
if ( ${HAVE_CHMOD} )
  add_compile_definitions ( HAVE_CHMOD=1 )
endif()

check_symbol_exists ( getopt "unistd.h" HAVE_GETOPT )
if ( ${HAVE_GETOPT} )
  add_compile_definitions ( HAVE_GETOPT=1 )
endif()

# Check backtrace

check_function_exists ( backtrace HAVE_BACKTRACE )
check_function_exists ( backtrace_symbols HAVE_BACKTRACE_SYMBOLS )
#include(CheckCXXSymbolExists)
#check_cxx_symbol_exists(backtrace features HAVE_BACKTRACE)
#check_cxx_symbol_exists(backtrace_symbols features HAVE_BACKTRACE_SYMBOLS)
#message ( STATUS "HAVE_BACKTRACE: ${HAVE_BACKTRACE}" )
#message ( STATUS "HAVE_BACKTRACE_SYMBOLS: ${HAVE_BACKTRACE_SYMBOLS}" )
if ( ${HAVE_BACKTRACE} )
    message ( STATUS "backtrace_symbols() available")
    add_link_options ( $<$<CONFIG:DEBUG>:-rdynamic> ) # for backtrace_symbols()
endif()


# Check string functions

check_symbol_exists ( strnlen "string.h" HAVE_STRNLEN )
if ( ${HAVE_STRNLEN} )
  add_compile_definitions ( HAVE_STRNLEN=1 )
endif()

check_symbol_exists ( strndup "string.h" HAVE_STRNDUP )
if ( ${HAVE_STRNDUP} )
  add_compile_definitions ( HAVE_STRNDUP=1 )
endif()

check_symbol_exists ( mempcpy "string.h" HAVE_MEMPCPY )
if ( ${HAVE_MEMPCPY} )
  add_compile_definitions ( HAVE_MEMPCPY=1 )
endif()

check_symbol_exists ( stpncpy "string.h" HAVE_STPNCPY )
if ( ${HAVE_STPNCPY} )
  add_compile_definitions ( HAVE_STPNCPY=1 )
endif()

#
# ADFlib config
#
option ( ADFLIB_ENABLE_NATIVE_DEV "Enable native devices" ON )

option ( ADFLIB_ENABLE_SALVAGE_DIRCACHE
         "Allow file undelete on volumes with dircache (EXPERIMENTAL)" ON )
if ( ADFLIB_ENABLE_SALVAGE_DIRCACHE )
  message ( STATUS "Enabling salvage for volumes with dircache (EXPERIMENTAL)." )
  add_definitions ( -DADFLIB_ENABLE_SALVAGE_DIRCACHE )
else()
  message ( STATUS "Disabling salvage for volumes with dircache." )
endif()


#
# Subdirectories
#

add_subdirectory ( src )
add_subdirectory ( examples )


#
# Tests
#

option ( ADFLIB_ENABLE_TESTS "Enable tests" ON )

if ( ADFLIB_ENABLE_TESTS )
    message ( STATUS "Testing enabled" )
    enable_testing()
    add_subdirectory( tests )
else()
    message ( STATUS "Testing disabled" )
endif()

include( GNUInstallDirs )

# install documentation
install( FILES
    doc/adfbitmap.1
    doc/adfformat.1
    doc/adfimgcreate.1
    doc/adfinfo.1
    doc/adfsalvage.1
    doc/unadf.1
#    TYPE MAN   --> installs to /usr/local/share/man, not to .../man/man1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1 )
