/*
 * fl_test.c
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "adflib.h"
#include "common.h"
#include "log.h"

#define TEST_VERBOSITY 3


void MyVer(char *msg)
{
    fprintf(stderr,"Verbose [%s]\n",msg);
}


/*
 *
 *
 */
int main(int argc, char *argv[])
{
    (void) argc, (void) argv;

    log_init( stderr, TEST_VERBOSITY );

    int status = 0;
 
    adfLibInit();

//	adfEnvSetFct(0,0,MyVer,0);

    /* open and mount existing device */
    struct AdfDevice * hd = adfDevOpen( argv[1], ADF_ACCESS_MODE_READWRITE );
    if ( ! hd ) {
        log_error( "Cannot open file/device '%s' - aborting...\n", argv[1] );
        status = 1;
        goto cleanup_env;
    }

    ADF_RETCODE rc = adfDevMount( hd );
    if ( rc != ADF_RC_OK ) {
        log_error( "can't mount device\n" );
        status = 1;
        goto cleanup_dev;
    }

    struct AdfVolume * vol = adfVolMount( hd, 0, ADF_ACCESS_MODE_READWRITE );
    if ( ! vol ) {
        log_error( "can't mount volume\n" );
        status = 1;
        goto cleanup_dev;
    }

    showVolInfo( vol );

    adfVolUnMount( vol );
    adfDevUnMount( hd );
    adfDevClose( hd );

    log_info("\n");

    /* create and mount one device */
    hd = adfDevCreate( "dump", argv[ 2 ], 80, 2, 11 );
    if ( ! hd ) {
        log_error( "can't create device\n" );
        status = 1;
        goto cleanup_env;
    }

    showDevInfo( hd );

    adfCreateFlop( hd, "empty", ADF_DOSFS_FFS |
                                ADF_DOSFS_DIRCACHE );

    vol = adfVolMount( hd, 0, ADF_ACCESS_MODE_READWRITE );
    if ( ! vol ) {
        log_error( "can't mount volume\n" );
        status = 1;
        goto cleanup_dev;
    }

    FILE * const boot = fopen( argv[ 3 ], "rb" );
    if ( ! boot ) {
        log_error( "can't open bootblock file\n" );
        status = 1;
        goto cleanup_vol;
    }
    unsigned char bootcode[1024];
    fread( bootcode, sizeof(unsigned char), 1024, boot );
    adfVolInstallBootBlock( vol, bootcode );
    fclose( boot );

    showVolInfo( vol );

cleanup_vol:
    adfVolUnMount( vol );

cleanup_dev:
    adfDevUnMount( hd );
    adfDevClose( hd );

cleanup_env:
    adfLibCleanUp();

    return status;
}
