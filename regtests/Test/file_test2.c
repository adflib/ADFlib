/*
 * file_test2.c
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include"adflib.h"


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
    if ( argc < 2 ) {
        fprintf ( stderr,
                  "required parameter (image/device) absent - aborting...\n");
        return 1;
    }

    struct AdfVolume *vol;
    struct AdfFile *file;
    unsigned char buf[600];
    FILE *out;
    struct AdfList *list;
 
    adfEnvInitDefault();

//	adfEnvSetFct(0,0,MyVer,0);

    /* open and mount existing device : FFS */
    struct AdfDevice * hd = adfDevOpen ( argv[1], ADF_ACCESS_MODE_READWRITE );
    if ( ! hd ) {
        fprintf ( stderr, "Cannot open file/device '%s' - aborting...\n",
                  argv[1] );
        adfEnvCleanUp();
        exit(1);
    }

    ADF_RETCODE rc = adfDevMount ( hd );
    if ( rc != ADF_RC_OK ) {
        fprintf(stderr, "can't mount device\n");
        adfDevClose ( hd );
        adfEnvCleanUp(); exit(1);
    }

    vol = adfVolMount ( hd, 0, ADF_ACCESS_MODE_READWRITE );
    if (!vol) {
        adfDevUnMount ( hd );
        adfDevClose ( hd );
        fprintf(stderr, "can't mount volume\n");
        adfEnvCleanUp(); exit(1);
    }

    adfVolInfo(vol);


    /* write one file */
    file = adfFileOpen ( vol, "moon_gif", ADF_FILE_MODE_WRITE );
    if (!file) return 1;
    out = fopen( argv[2],"rb");
    if (!out) return 1;
    
    unsigned len = 600;
    unsigned n = (unsigned) fread ( buf, sizeof(unsigned char), len, out );
    while(!feof(out)) {
        adfFileWrite ( file, n, buf );
        n = (unsigned) fread ( buf, sizeof(unsigned char), len, out );
    }
    if (n>0)
        adfFileWrite ( file, n, buf );

    fclose(out);

    adfFileClose ( file );

    /* the directory */
    list = adfGetDirEnt(vol,vol->curDirPtr);
    while(list) {
        adfEntryPrint ( list->content );
        adfFreeEntry(list->content);
        list = list->next;
    }
    adfListFree ( list );


    /* re read this file */
    file = adfFileOpen ( vol, "moon_gif", ADF_FILE_MODE_READ );
    if (!file) return 1;
    out = fopen("moon__gif","wb");
    if (!out) return 1;

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
