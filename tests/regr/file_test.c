/*
 * file_test.c
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include"adflib.h"
#include "common.h"
#include "log.h"

#define TEST_VERBOSITY 1


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
    log_init( stderr, TEST_VERBOSITY );

    if ( argc < 2 ) {
        log_error( "missingparameter (image/device) - aborting...\n" );
        return 1;
    }

    int status = 0;
 
    adfLibInit();

//	adfEnvSetFct(0,0,MyVer,0);

    /* open and mount existing device : FFS */
    struct AdfDevice * hd = adfDevOpen( argv[1], ADF_ACCESS_MODE_READONLY );
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

    struct AdfVolume * vol = adfVolMount( hd, 0, ADF_ACCESS_MODE_READONLY );
    if ( ! vol ) {
        log_error( "can't mount volume\n" );
        status = 1;
        goto cleanup_dev;
    }

    showVolInfo( vol );

    struct AdfFile * file = adfFileOpen( vol, "mod.and.distantcall", ADF_FILE_MODE_READ );
    if ( ! file ) {
        log_error( "can't open file mod.and.distantcall\n" );
        status = 1;
        goto cleanup_vol;
    }
    FILE * out = fopen( "mod.distant", "wb" );
    if ( ! out ) {
        log_error( "can't open file mod.distant\n" );
        status = 1;
        adfFileClose( file );
        goto cleanup_vol;
    }
    
    unsigned len = 600;
    unsigned char buf[600];
    unsigned n = adfFileRead( file, len, buf );
    while ( ! adfFileAtEOF( file ) ) {
        fwrite( buf, sizeof(unsigned char), n, out );
        n = adfFileRead( file, len, buf );
    }
    if ( n > 0 )
        fwrite( buf, sizeof(unsigned char), n, out );

    fclose( out );

    adfFileClose( file );

    file = adfFileOpen( vol, "emptyfile", ADF_FILE_MODE_READ );
    if ( ! file ) { 
        log_error( "can't open file emptyfile\n" );
        status = 1;
        goto cleanup_vol;
    }
 
    n = adfFileRead( file, 2, buf );

    adfFileClose( file );

    adfVolUnMount( vol );
    adfDevUnMount( hd );
    adfDevClose( hd );


    /* ofs */

    hd = adfDevOpen( argv[2], ADF_ACCESS_MODE_READONLY );
    if ( ! hd ) {
        log_error( "Cannot open file/device '%s' - aborting...\n", argv[2] );
        status = 1;
        goto cleanup_env;
    }

    rc = adfDevMount( hd );
    if ( rc != ADF_RC_OK ) {
        log_error( "can't mount device\n" );
        status = 1;
        goto cleanup_dev;
    }

    vol = adfVolMount( hd, 0, ADF_ACCESS_MODE_READONLY );
    if ( ! vol ) {
        log_error( "can't mount volume\n" );
        status = 1;
        goto cleanup_dev;
    }

    showVolInfo( vol );

    file = adfFileOpen( vol, "moon.gif", ADF_FILE_MODE_READ );
    if ( ! file ) {
        log_error( "can't open file moon.gif for reading\n" );
        status = 1;
        goto cleanup_vol;
    }
    out = fopen( "moon_gif", "wb" );
    if ( ! out ) {
        log_error( "can't open file moon.gif for writing\n" );
        status = 1;
        adfFileClose( file );
        goto cleanup_vol;
    }

    len = 300;
    n = adfFileRead( file, len, buf );
    while ( ! adfFileAtEOF( file ) ) {
        fwrite( buf, sizeof(unsigned char), n, out );
        n = adfFileRead( file, len, buf );
    }
    if ( n > 0 )
        fwrite( buf, sizeof(unsigned char), n, out );

    fclose( out );

    adfFileClose( file );

cleanup_vol:
    adfVolUnMount( vol );

cleanup_dev:
    adfDevUnMount( hd );
    adfDevClose( hd );

cleanup_env:
    adfLibCleanUp();

    return status;
}
