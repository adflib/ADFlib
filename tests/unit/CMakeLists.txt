
find_package( CHECK 0.11.0 QUIET )
if ( NOT CHECK_FOUND )
    find_package( PkgConfig QUIET )
    if ( PkgConfig_FOUND )
        set( PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON )
        pkg_check_modules( CHECK check>=0.11.0 )
    endif ( PkgConfig_FOUND )
endif ( NOT CHECK_FOUND )

if ( NOT CHECK_FOUND )
    # Automatic fetching external dependency
    # ( https://cmake.org/cmake/help/latest/module/FetchContent.html
    #   https://cmake.org/cmake/help/latest/guide/using-dependencies/ )
    message( STATUS "Fetching Check library" )
    include( FetchContent )
    FetchContent_Declare(
      CHECK
      GIT_REPOSITORY https://github.com/libcheck/check.git
      GIT_TAG        0.15.2
    )
    FetchContent_MakeAvailable( CHECK )
endif()

#if (NOT Check_FOUND AND NOT DEFINED Check_QUIET)
#    message(FATAL_ERROR "Could NOT find Check")
#endif (NOT Check_FOUND AND NOT DEFINED Check_QUIET)

#add_compile_options(-pthread)

include_directories( ${PROJECT_SOURCE_DIR}/src )
include_directories( ${PROJECT_BINARY_DIR}/src )

message( STATUS "Check CFLAGS: ${CHECK_CFLAGS}" )
message( STATUS "Check LDFLAGS: ${CHECK_LDFLAGS}" )
#message ( STATUS "Check include dir: ${CHECK_INCLUDE_DIR}, libraries: ${CHECK_LIBRARIES}")
#if ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
#include_directories(${CHECK_INCLUDE_DIR})
#else()
add_compile_options( ${CHECK_CFLAGS} )
#endif()

add_subdirectory( examples )

add_executable( test_adflib
                test_adflib.c )

add_executable( test_adfenv
                test_adfenv.c )


add_executable( test_test_util
                test_test_util.c
                test_util.c )

add_executable( test_adf_util
                test_adf_util.c )

add_executable( test_adfPos2DataBlock
                test_adfPos2DataBlock.c )

add_executable( test_adfDays2Date
                test_adfDays2Date.c
                ${PROJECT_SOURCE_DIR}/src/adf_util.c )

add_executable( test_adf_vector
                test_adf_vector.c
                ${PROJECT_SOURCE_DIR}/src/adf_vector.c
                ${PROJECT_SOURCE_DIR}/src/adf_vector.h )

add_executable( test_adfSwapEndian
                test_adfSwapEndian.c
                test_util.c )
#                ${PROJECT_SOURCE_DIR}/src/adf_raw.c
#                ${PROJECT_SOURCE_DIR}/src/adf_raw.h )

add_executable( test_dev_open
                test_dev_open.c )

add_executable( test_dev_mount
                test_dev_mount.c )

add_executable( test_adf_file_util
                test_adf_file_util.c )

add_executable( test_file_create
                test_file_create.c )

add_executable( test_file_append
                test_file_append.c )

add_executable( test_file_write
                test_file_write.c
	        test_util.c )

add_executable( test_file_write_chunks
                test_file_write_chunks.c
	        test_util.c )

add_executable( test_file_overwrite
                test_file_overwrite.c )

add_executable( test_file_overwrite2
                test_file_overwrite2.c
                test_util.c )

add_executable( test_file_seek
                test_file_seek.c
                test_util.c )

add_executable( test_file_seek_after_write
                test_file_seek_after_write.c
                test_util.c )

add_executable( test_file_truncate
                test_file_truncate.c
                test_util.c )

add_executable( test_file_truncate2
                test_file_truncate2.c
                test_util.c )


if ( "${CHECK_LIBRARIES}" STREQUAL "" )
  set( CHECK_LIBRARIES Check::check )
else()
  #set( CHECK_LIBRARIES "${CHECK_LIBRARIES} -pthread" )
  set( CHECK_LIBRARIES "${CHECK_LDFLAGS}" )
endif()

target_link_libraries( test_adflib                PUBLIC adf ${CHECK_LIBRARIES} )
target_link_libraries( test_adfenv                PUBLIC adf ${CHECK_LIBRARIES} )

target_link_libraries( test_test_util             PUBLIC adf ${CHECK_LIBRARIES} )
target_link_libraries( test_adf_util              PUBLIC adf ${CHECK_LIBRARIES} )
target_link_libraries( test_adfPos2DataBlock      PUBLIC adf ${CHECK_LIBRARIES} )
target_link_libraries( test_adfDays2Date          PUBLIC     ${CHECK_LIBRARIES} )
target_link_libraries( test_adf_vector            PUBLIC     ${CHECK_LIBRARIES} )

target_link_libraries( test_adfSwapEndian         PUBLIC adf ${CHECK_LIBRARIES} )
target_link_libraries( test_dev_open              PUBLIC adf ${CHECK_LIBRARIES} )
target_link_libraries( test_dev_mount             PUBLIC adf ${CHECK_LIBRARIES} )
target_link_libraries( test_adf_file_util         PUBLIC     ${CHECK_LIBRARIES} )
target_link_libraries( test_file_create           PUBLIC adf ${CHECK_LIBRARIES} )
target_link_libraries( test_file_append           PUBLIC adf ${CHECK_LIBRARIES} )
target_link_libraries( test_file_write            PUBLIC adf ${CHECK_LIBRARIES} )

target_link_libraries( test_file_write_chunks     PUBLIC adf ${CHECK_LIBRARIES} )
target_link_libraries( test_file_overwrite        PUBLIC adf ${CHECK_LIBRARIES} )
target_link_libraries( test_file_overwrite2       PUBLIC adf ${CHECK_LIBRARIES} )
target_link_libraries( test_file_seek             PUBLIC adf ${CHECK_LIBRARIES} )
target_link_libraries( test_file_seek_after_write PUBLIC adf ${CHECK_LIBRARIES} )
target_link_libraries( test_file_truncate         PUBLIC adf ${CHECK_LIBRARIES} )
target_link_libraries( test_file_truncate2        PUBLIC adf ${CHECK_LIBRARIES} )

# this should be done first
add_test( test_adflib test_adflib )
add_test( test_adfenv test_adfenv )

# utils (independent code)
add_test( test_test_util        test_test_util )
add_test( test_adfPos2DataBlock test_adfPos2DataBlock )
add_test( test_adfDays2Date     test_adfDays2Date )
add_test( test_adf_vector       test_adf_vector )

# library functions
add_test( test_adfSwapEndian         test_adfSwapEndian )
add_test( test_dev_open              test_dev_open )
add_test( test_dev_mount             test_dev_mount )
add_test( test_adf_file_util         test_adf_file_util )
add_test( test_file_create           test_file_create )
add_test( test_file_append           test_file_append )
add_test( test_file_write            test_file_write )
add_test( test_file_write_chunks     test_file_write_chunks )
add_test( test_file_overwrite        test_file_overwrite )
add_test( test_file_overwrite2       test_file_overwrite2 )
add_test( test_file_seek             test_file_seek )
add_test( test_file_seek_after_write test_file_seek_after_write )
add_test( test_file_truncate         test_file_truncate )
add_test( test_file_truncate2        test_file_truncate2 )
