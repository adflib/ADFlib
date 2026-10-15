
find_package( CHECK 0.11.0 QUIET )
if ( NOT CHECK_FOUND )
    find_package( PkgConfig QUIET )
    if ( PkgConfig_FOUND )
        set( PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON )
        pkg_check_modules( CHECK check>=0.11.0 )
    endif ( PkgConfig_FOUND )
endif ( NOT CHECK_FOUND )

include_directories( ${PROJECT_SOURCE_DIR}/examples )

add_executable( test_unadf_path
                test_unadf_path.c
                ${PROJECT_SOURCE_DIR}/examples/unadf_arg.h
                ${PROJECT_SOURCE_DIR}/examples/unadf_arg.c
                ${PROJECT_SOURCE_DIR}/examples/unadf_path.h
                ${PROJECT_SOURCE_DIR}/examples/unadf_path.c
                ${PROJECT_SOURCE_DIR}/examples/unadf_util.h
                ${PROJECT_SOURCE_DIR}/examples/unadf_util.c )

target_link_libraries( test_unadf_path PUBLIC adf ${CHECK_LIBRARIES} )

add_test( test_unadf_path test_unadf_path )
