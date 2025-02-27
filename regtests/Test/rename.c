/*
 * rename.c
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
    (void) argc, (void) argv;
    struct AdfVolume *vol;
    struct AdfFile *fic;
    unsigned char buf[1];
    struct AdfList *list, *cell;
 
    adfEnvInitDefault();

    /* create and mount one device */
    struct AdfDevice * const hd = adfDevCreate ( "dump", "rename-newdev", 80, 2, 11 );
    if (!hd) {
        fprintf(stderr, "can't mount device\n");
        adfEnvCleanUp(); exit(1);
    }

    adfDevInfo(hd);

    if ( adfCreateFlop ( hd, "empty", ADF_DOSFS_FFS |
                                      ADF_DOSFS_DIRCACHE ) != ADF_RC_OK )
    {
		fprintf(stderr, "can't create floppy\n");
        adfDevUnMount ( hd );
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

    fic = adfFileOpen ( vol, "file_1a", ADF_FILE_MODE_WRITE );
    if (!fic) {
        adfVolUnMount(vol);
        adfDevUnMount ( hd );
        adfDevClose ( hd );
        adfEnvCleanUp();
        exit(1);
    }
    adfFileWrite ( fic, 1, buf );
    adfFileClose ( fic );

    fic = adfFileOpen ( vol, "file_24", ADF_FILE_MODE_WRITE );
    if (!fic) {
        adfVolUnMount(vol);
        adfDevUnMount ( hd );
        adfDevClose ( hd );
        adfEnvCleanUp();
        exit(1);
    }
    adfFileWrite ( fic, 1, buf );
    adfFileClose ( fic );

    fic = adfFileOpen ( vol, "dir_1a", ADF_FILE_MODE_WRITE );
    if (!fic) {
        adfVolUnMount(vol);
        adfDevUnMount ( hd );
        adfDevClose ( hd );
        adfEnvCleanUp();
        exit(1);
    }
    adfFileWrite ( fic, 1, buf );
    adfFileClose ( fic );

    fic = adfFileOpen ( vol, "dir_5u", ADF_FILE_MODE_WRITE );
    if (!fic) {
        adfVolUnMount(vol);
        adfDevUnMount ( hd );
        adfDevClose ( hd );
        adfEnvCleanUp();
        exit(1);
    }
    adfFileWrite ( fic, 1, buf );
    adfFileClose ( fic );

    puts("Create file_1a, file_24, dir_1a, dir_5u (with this order)");

    cell = list = adfGetDirEnt(vol,vol->curDirPtr);
    while(cell) {
        adfEntryPrint ( cell->content );
        cell = cell->next;
    }
    adfFreeDirList(list);

    putchar('\n');

    puts("Rename dir_5u into file_5u");

    adfRenameEntry(vol, vol->curDirPtr, "dir_5u", vol->curDirPtr, "file_5u");

    cell = list = adfGetDirEnt(vol,vol->curDirPtr);
    while(cell) {
        adfEntryPrint ( cell->content );
        cell = cell->next;
    }
    adfFreeDirList(list);

    putchar('\n');

    puts("Rename file_1a into dir_3");

    adfRenameEntry(vol, vol->curDirPtr,"file_1a", vol->curDirPtr,"dir_3");

    cell = list = adfGetDirEnt(vol,vol->curDirPtr);
    while(cell) {
        adfEntryPrint ( cell->content );
        cell = cell->next;
    }
    adfFreeDirList(list);

    puts("Create dir_5u, Rename dir_3 into toto");
/*
    fic = adfOpenFile ( vol, "dir_5u", ADF_FILE_MODE_WRITE );
    if (!fic) {
        adfVolUnMount(vol);
        adfUnMountDev(hd);
        adfCloseDev(hd);
        adfEnvCleanUp();
        exit(1);
    }
    adfWriteFile(fic,1,buf);
    adfCloseFile(fic);
*/
    adfCreateDir(vol,vol->curDirPtr,"dir_5u");

    cell = list = adfGetDirEnt(vol, vol->curDirPtr);
    while(cell) {
        adfEntryPrint ( cell->content );
        cell = cell->next;
    }
    adfFreeDirList(list);

    adfRenameEntry(vol, vol->curDirPtr,"dir_1a", vol->curDirPtr,"longfilename");

    cell = list = adfGetDirEnt(vol,vol->curDirPtr);
    while(cell) {
        adfEntryPrint ( cell->content );
        cell = cell->next;
    }
    adfFreeDirList(list);

    adfVolUnMount(vol);
    adfDevUnMount ( hd );
    adfDevClose ( hd );

    adfEnvCleanUp();

    return 0;
}
