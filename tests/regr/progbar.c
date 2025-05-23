/*
 *  dispsect.c
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include"adflib.h"
#include "common.h"


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
    struct AdfVolume *vol;
 
    adfLibInit();

    adfEnvSetProperty ( ADF_PR_USEDIRC, true );

    /* use or not the progress bar callback */
    adfEnvSetProperty ( ADF_PR_USE_PROGBAR, true );
 
    /* create and mount one device */
puts("\ncreate dumpdevice");
    struct AdfDevice * const hd = adfDevCreate ( "dump", "progbar-newdev", 80, 2, 11 );
    if (!hd) {
        fprintf(stderr, "can't mount device\n");
        adfLibCleanUp(); exit(1);
    }

    showDevInfo( hd );

puts("\ncreate floppy");
    if ( adfCreateFlop ( hd, "empty", ADF_DOSFS_FFS |
                                      ADF_DOSFS_DIRCACHE ) != ADF_RC_OK )
    {
		fprintf(stderr, "can't create floppy\n");
        adfDevUnMount ( hd );
        adfDevClose ( hd );
        adfLibCleanUp(); exit(1);
    }

    vol = adfVolMount ( hd, 0, ADF_ACCESS_MODE_READWRITE );
    if (!vol) {
        adfDevUnMount ( hd );
        adfDevClose ( hd );
        fprintf(stderr, "can't mount volume\n");
        adfLibCleanUp(); exit(1);
    }

    showVolInfo( vol );

    adfVolUnMount(vol);
    adfDevUnMount ( hd );
    adfDevClose ( hd );

    adfLibCleanUp();

    return 0;
}
