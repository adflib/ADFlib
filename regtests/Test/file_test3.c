/*
 * dir_test.c
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
        log_error( "missing parameter (image/device) - aborting...\n" );
        return 1;
    }

    struct AdfVolume *vol;
    struct AdfFile *file;
    unsigned char buf[600];
    FILE *out;
 
    adfEnvInitDefault();

//	adfEnvSetFct(0,0,MyVer,0);

    /* open and mount existing device : OFS */
    struct AdfDevice * hd = adfDevOpen ( argv[1], ADF_ACCESS_MODE_READWRITE );
    if ( ! hd ) {
        log_error( "Cannot open file/device '%s' - aborting...\n", argv[1] );
        adfEnvCleanUp();
        exit(1);
    }

    ADF_RETCODE rc = adfDevMount ( hd );
    if ( rc != ADF_RC_OK ) {
        log_error( "can't mount device\n" );
        adfDevClose ( hd );
        adfEnvCleanUp(); exit(1);
    }

    vol = adfVolMount ( hd, 0, ADF_ACCESS_MODE_READWRITE );
    if (!vol) {
        log_error( "can't mount volume\n" );
        adfDevUnMount ( hd );
        adfDevClose ( hd );
        adfEnvCleanUp(); exit(1);
    }

    showVolInfo( vol );


    /* write one file */
    file = adfFileOpen ( vol, "moon_gif", ADF_FILE_MODE_WRITE );
    if (!file) {
        log_error( "can't open file 'moon_gif' for writing\n" );
        return 1;
    }
    out = fopen( argv[2],"rb");
    if ( ! out ) {
        log_error( "can't open file '%s' for reading\n", argv[2] );
        return 1;
    }
    
    unsigned len = 600;
    unsigned n = (unsigned) fread(buf,sizeof(unsigned char),len,out);
    while(!feof(out)) {
        adfFileWrite ( file, n, buf );
        n = (unsigned) fread(buf,sizeof(unsigned char),len,out);
    }
    if (n>0)
        adfFileWrite ( file, n, buf );

    fclose(out);

    adfFileClose ( file );

    /* the directory */
    showDirEntries( vol, vol->curDirPtr );

    /* re read this file */
    file = adfFileOpen ( vol, "moon_gif", ADF_FILE_MODE_READ );
    if ( ! file ) {
        log_error( "can't open file 'moon_gif' for reading\n" );
        return 1;
    }
    out = fopen("moon__gif","wb");
    if ( ! out ) {
        log_error( "can't open file 'moon__gif' for writing\n" );
        return 1;
    }

    len = 300;
    n = adfFileRead ( file, len, buf );
    while(!adfFileAtEOF(file)) {
        fwrite(buf,sizeof(unsigned char),n,out);
        n = adfFileRead ( file, len, buf );
    }
    if (n>0)
        fwrite(buf,sizeof(unsigned char),n,out);

    fclose(out);

    adfFileClose ( file );

    adfVolUnMount(vol);
    adfDevUnMount ( hd );
    adfDevClose ( hd );

    adfEnvCleanUp();

    return 0;
}
