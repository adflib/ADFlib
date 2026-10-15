
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
