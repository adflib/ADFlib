
#include <stdio.h>
#include <stdlib.h>
#include"adflib.h"
#include "log.h"

#define TEST_VERBOSITY 3

typedef struct check_s {
    unsigned int  offset;
    unsigned char value;
} check_t;

typedef struct reading_test_s {
    char *       info;
    char *       image_filename;

    char *       hlink_dir;
    char *       hlink_name;

    char *       real_file;

    unsigned int nchecks;
    check_t      checks[100];
} reading_test_t;


int test_hlink_read ( reading_test_t * test_data );

int test_single_read ( struct AdfFile * const file_adf,
                       unsigned int           offset,
                       unsigned char          expected_value );


int main ( int argc, char * argv[] )
{ 
    (void) argc;

    log_init( stderr, TEST_VERBOSITY );

    adfEnvInitDefault();

//	adfEnvSetFct(0,0,MyVer,0);
    int status = 0;

    // setup
    reading_test_t test_hlink = {
        .info       = "hard link file reads",
        .hlink_dir  = NULL, //"dir_1",
        .hlink_name = "hlink_blue",
        .real_file  = "dir_2/blue2c.gif",
        .nchecks    = 8,
        .checks     = {
            { 0, 0x47 },
            { 1, 0x49 },
            { 2, 0x46 },
            { 3, 0x38 },

            // the last 4 bytes of the file
            { 0xcfe, 0x42 },
            { 0xcff, 0x04 },
            { 0xd00, 0x00 },
            { 0xd01, 0x3B }
        }
    };

    reading_test_t test_chained_hlink = {
        .info       = "chained hard link file reads",
        .hlink_dir  = "hardlinks_file",
        .hlink_name = "hl2hl2hl2testfile1",
        .real_file  = "dir1/dir1_1/testfile.txt",
        .nchecks    = 8,
        .checks     = {
            { 0, 'T' },
            { 1, 'h' },
            { 2, 'i' },
            { 3, 's' },

            // the last 4 bytes of the file
            { 0x3e, '.' },  // 0x2e
            { 0x3f, '.' },
            { 0x40, '\n' },  // 0x0a
            { 0x41, '\n' }
        }
    };

    test_hlink.image_filename = argv[1];
    test_chained_hlink.image_filename = argv[2];

    // run tests
    log_info( "*** Test reading a file opened using a hardlink\n" );
    status += test_hlink_read ( &test_hlink );
    status += test_hlink_read ( &test_chained_hlink );
    log_info( status ? " -> ERROR\n" : " -> PASSED\n" );

    // clean-up
    adfEnvCleanUp();

    return status;
}


int test_hlink_read ( reading_test_t * test_data )
{
    log_info( "\n*** Testing %s"
             "\n\timage file:\t%s\n\tdirectory:\t%s\n\thlink:\t\t%s\n\treal file:\t%s\n",
             test_data->info,
             test_data->image_filename,
             test_data->hlink_dir,
             test_data->hlink_name,
             test_data->real_file );

    struct AdfDevice * const dev = adfDevOpen ( test_data->image_filename,
                                                ADF_ACCESS_MODE_READONLY );
    if ( ! dev ) {
        log_error( "Cannot open file/device '%s' - aborting...\n",
                   test_data->image_filename );
        adfEnvCleanUp();
        exit(1);
    }

    ADF_RETCODE rc = adfDevMount ( dev );
    if ( rc != ADF_RC_OK ) {
        log_error( "Cannot mount image %s - aborting the test...\n",
                   test_data->image_filename );
        adfDevClose ( dev );
        return 1;
    }

    struct AdfVolume * const vol = adfVolMount ( dev, 0, ADF_ACCESS_MODE_READONLY );
    if ( ! vol ) {
        log_error( "Cannot mount volume 0 from image %s - aborting the test...\n",
                   test_data->image_filename );
        adfDevUnMount ( dev );
        adfDevClose ( dev );
        return 1;
    }

#if TEST_VERBOSITY > 0
    //showVolInfo ( vol );
#endif

    int status = 0;
    adfToRootDir ( vol );
    char * dir = test_data->hlink_dir;
    if ( dir ) {
        log_info( "Entering directory %s...\n", dir );

        int chdir_st = adfChangeDir ( vol, dir );
        if ( chdir_st != ADF_RC_OK ) {
            log_error( " -> Cannot chdir to %s, status %d - aborting...\n",
                       dir, chdir_st );
            adfToRootDir ( vol );
            status = 1;
            goto clean_up;
        }
    }

    struct AdfFile * const file_adf = adfFileOpen ( vol, test_data->hlink_name,
                                                    ADF_FILE_MODE_READ );
    if ( ! file_adf ) {
        log_error( " -> Cannot open hard link file %s - aborting...\n",
                   test_data->hlink_name );
        status = 1;
        goto clean_up;
    }

    for ( unsigned int i = 0 ; i < test_data->nchecks ; ++i ) {
        status += test_single_read ( file_adf,
                                     test_data->checks[i].offset,
                                     test_data->checks[i].value );
    }

    adfFileClose ( file_adf );

    // clean-up
clean_up:
    //adfToRootDir ( vol );
    adfVolUnMount ( vol );
    adfDevUnMount ( dev );
    adfDevClose ( dev );

    return status;
}


int test_single_read ( struct AdfFile * const file_adf,
                       unsigned int           offset,
                       unsigned char          expected_value )
{
    log_info( "  Reading data after seek to position 0x%x (%d)...",
              offset, offset );

    adfFileSeek ( file_adf, offset );

    unsigned char c;
    unsigned n = adfFileRead ( file_adf, 1, &c );

    if ( n != 1 ) {
        log_error( " -> Reading data failed!!!\n" );
        return 1;
    }
    
    if ( c != expected_value ) {
        log_error( " -> Incorrect data read:  expected 0x%x != read 0x%x\n",
                   (int) expected_value, (int) c );
        return 1;
    }

    log_info( " -> OK.\n" );

    return 0;
}
