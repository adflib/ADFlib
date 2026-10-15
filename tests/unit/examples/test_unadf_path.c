
#include <check.h>
#include <stdio.h>
#include <stdlib.h>

//#include "adflib.h"
#include "unadf_path.h"

#define MAX_TEST_STR_SIZE 64

//#define DEBUG_TEST_UNADF_PATH

START_TEST( test_check_framework ) {
    ck_assert( 1 );
}
END_TEST


START_TEST( test_fix_win32_name  )
{
    typedef struct test_data_s {
        const char
            *const input,
            *const output;
    } test_data_t;

    test_data_t test_data[] = {
        { "nochange", "nochange" },

        // forbidden characters
        { "char\1-lt32", "char_-lt32" },
        { "char""\x1f""lt32", "char_lt32" },         // < 32 changed
        { "char""\x20""eq32", "char""\x20""eq32" },  // 32 unchanged
        { "co:lon", "co_lon" },
        { "star*char", "star_char" },
        { "double\"quote", "double_quote" },
        { "question?mark", "question_mark" },
        { "less<than", "less_than" },
        { "more>than", "more_than" },
        { "pipe|char", "pipe_char" },
        { "slash/char", "slash_char" },
        { "back\\slash", "back_slash" },

        // extension
        { "mul.tiple.dots", "mul.tiple.dots" },

        // forbidden names
        { "CON\0", "CON_" },
        { "PRN\0", "PRN_" },
        { "AUX\0", "AUX_" },
        { "NUL\0", "NUL_" },
        { "CON\0", "CON_" },
        { "COM9\0", "COM9_" },
        { "LPT7\0", "LPT7_" },
        { "lpt5.ext\0", "lpt5_.ext" },
        { "com3.com3.ext\0", "com3_.com3.ext" },  // ???
        { "com3.com4.ext\0", "com3_.com4.ext" },  // ???
    };
    const unsigned NTESTS = sizeof( test_data ) / sizeof( test_data_t );

    char mangled[ MAX_TEST_STR_SIZE + 1 ];
    for ( unsigned i = 0; i < NTESTS; ++i ) {
#ifdef DEBUG_TEST_UNADF_PATH
        fprintf( stderr, "case %d:\n  input:    %s\n",
                 i, test_data[ i ].input );
#endif
        strncpy( mangled, test_data[ i ].input, MAX_TEST_STR_SIZE );
        mangled[ MAX_TEST_STR_SIZE ] = '\0';

        fix_win32_name( mangled );

#ifdef DEBUG_TEST_UNADF_PATH
        fprintf( stderr, "  output:   %s\n  expected: %s\n",
                 mangled, test_data[ i ].output );
#endif
        ck_assert_str_eq( mangled, test_data[ i ].output );
    }
}
END_TEST

Suite * unadf_suite(void)
{
    Suite *const suite = suite_create( "unadf" );
    
    TCase *tcase = tcase_create( "check framework" );
    tcase_add_test( tcase, test_check_framework );
    suite_add_tcase( suite, tcase );

    tcase = tcase_create( "unadf fix_win32_name" );
    tcase_add_test( tcase, test_fix_win32_name );
    suite_add_tcase( suite, tcase );

    return suite;
}

int main(void)
{
    Suite *const   suite   = unadf_suite();
    SRunner *const srunner = srunner_create( suite );

    srunner_run_all( srunner, CK_VERBOSE ); //CK_NORMAL );

    const int number_failed = srunner_ntests_failed( srunner );
    srunner_free( srunner );

    return ( number_failed == 0 ) ? EXIT_SUCCESS : EXIT_FAILURE;
}
