/*
 * The program performs a test of updating the block allocation bitmap
 * on the given image.
 *
 * It does the following:
 * 1. makes a copy of the original images
 * 2. mounts and enforces block allocation bitmap update
 * 3. compares if the updated bitmap is the same as the original
 *    (every bit different counts as an error)
 * Obviously, the test can be performed only on images with already correct
 * block allocation bitmap (which should not be changed by the enforced update).
 *
 * More details on bitmap issues:
 * https://github.com/adflib/ADFlib/issues/63
 */


#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifndef _WIN32
#include <unistd.h>   // for unlink()
#endif

#include "adflib.h"

#define TEST_VERBOSITY 1


void log_error ( FILE * const       file,
                 const char * const format, ... );

void log_warning ( FILE * const       file,
                   const char * const format, ... );

void log_info ( FILE * const       file,
                const char * const format, ... );

ADF_RETCODE copy_file ( const char * const dst_fname,
                    const char * const src_fname );

unsigned compare_bitmaps ( struct AdfVolume * const volOrig,
                           struct AdfVolume * const volUpdate );

char * num32_to_bit_str ( uint32_t num,
                          char     str[33] );


int main ( const int          argc,
           const char * const argv[] )
{
    if ( argc < 2 ) {
        fprintf (stderr, "Usage: %s <ADF_IMAGE>\n", argv[0] );
        return 1;
    }

    /* set filenames */
    const char * const adfOrig         = argv[1];
    char               adfUpdate[1024] = "\0";
    strcat ( adfUpdate, adfOrig );
    strcat ( adfUpdate, "-bitmap_updated.adf" );

    bool error_status = false;

    /* make a copy for updating */
    if ( copy_file ( adfUpdate, adfOrig ) != ADF_RC_OK ) {
        error_status = true;
        goto delete_adf_copy;
    }

    /* init adflib */
    adfEnvInitDefault();
    adfEnvSetProperty ( ADF_PR_IGNORE_CHECKSUM_ERRORS, true );
//	adfEnvSetFct(0,0,MyVer,0);

    /* mount the original image */
    struct AdfDevice * devOrig = adfDevOpen ( adfOrig, ADF_ACCESS_MODE_READONLY );
    if ( ! devOrig ) {
        fprintf ( stderr, "Cannot open file/device '%s' - aborting...\n",
                  adfOrig );
        error_status = true;
        goto clean_up;
    }

    ADF_RETCODE rc = adfDevMount ( devOrig );
    if ( rc != ADF_RC_OK ) {
        log_error ( stderr, "can't mount device %s\n", adfOrig );
        error_status = true;
        goto close_dev_orig;
    }

    struct AdfVolume * const volOrig = adfVolMount ( devOrig, 0,
                                                     ADF_ACCESS_MODE_READONLY );
    if ( volOrig == NULL ) {
        log_error ( stderr, "can't mount volume %d\n", 0 );
        error_status = true;
        goto umount_dev_orig;
    }

    
    /* mount the image copy (for updating) */
    struct AdfDevice * const devUpdate = adfDevOpen ( adfUpdate,
                                                      ADF_ACCESS_MODE_READWRITE );
    if ( ! devUpdate ) {
        fprintf ( stderr, "Cannot open file/device '%s' - aborting...\n",
                  adfUpdate );
        error_status = true;
        goto umount_vol_orig;
    }

    rc = adfDevMount ( devUpdate );
    if ( rc != ADF_RC_OK ) {
        log_error ( stderr, "can't mount device %s\n", adfUpdate );
        error_status = true;
        goto close_dev_updated;
    }

    struct AdfVolume * volUpdate = adfVolMount ( devUpdate, 0,
                                                 ADF_ACCESS_MODE_READONLY );
    if ( volUpdate == NULL ) {
        log_error ( stderr, "can't mount volume %d\n", 0 );
        error_status = true;
        goto umount_dev_updated;
    }


    /* update the block allocation bitmap */
    rc = adfVolRemount ( volUpdate, ADF_ACCESS_MODE_READWRITE );
    if ( rc != ADF_RC_OK ) {
        log_error (
            stderr, "error remounting read-write, volume %d\n", 0 );
        error_status = true;
        goto umount_vol_updated;
    }

    struct AdfRootBlock root;
    rc = adfReadRootBlock ( volUpdate, (uint32_t) volUpdate->rootBlock, &root );
    if ( rc != ADF_RC_OK ) {
        adfEnv.eFct ( "Invalid RootBlock, volume %s, sector %u - aborting...",
                      volUpdate->volName, volUpdate->rootBlock );
        error_status = true;
        goto umount_vol_updated;
    }

    rc = adfReconstructBitmap ( volUpdate, &root );
    if ( rc != ADF_RC_OK ) {
        adfEnv.eFct ( "Error rebuilding the bitmap (%d), volume %s",
                      rc, volUpdate->volName );
        error_status = true;
        goto umount_vol_updated;
    }

    /*
    rc = adfUpdateBitmap ( vol );
    if ( rc != ADF_RC_OK ) {
        adfEnv.eFct ( "Error writing updated bitmap (%d), volume %s",
                      rc, volUpdate->volName );
        return rc;
    }

    adfVolUnMount ( volUpdate );

    volUpdate = adfVolMount ( devUpdate, 0,
                              ADF_ACCESS_MODE_READWRITE );
    if ( volUpdate == NULL ) {
        log_error ( stderr, "can't mount volume %d\n", 0 );
        error_status = true;
        goto umount_dev_updated;
    }
    */

    /* compare the original and reconstructed */
    unsigned nerrors = compare_bitmaps ( volOrig, volUpdate );
    if ( nerrors != 0 )
        log_error ( stderr, "Bitmap update of %s: %u errors\n", adfOrig, nerrors );
    
    error_status = ( nerrors != 0 );
    
    //printf( "%s", adfVolGetInfo( vol ) );
    //putchar('\n');

    /* cleanup */

umount_vol_updated:
    adfVolUnMount ( volUpdate );
umount_dev_updated:
    adfDevUnMount ( devUpdate );
close_dev_updated:
    adfDevClose ( devUpdate );

umount_vol_orig:
    adfVolUnMount ( volOrig );
umount_dev_orig:
    adfDevUnMount ( devOrig );
close_dev_orig:
    adfDevClose ( devOrig );

clean_up:
    adfEnvCleanUp();
    
delete_adf_copy:
    log_info ( stdout, "Removing %s\n", adfUpdate );
    unlink ( adfUpdate );
    
    return ( error_status ? 1 : 0 );
}



#define BUFSIZE 1024

ADF_RETCODE copy_file ( const char * const dst_fname,
                    const char * const src_fname )
{
    log_info ( stdout, "Copying %s to %s... ", src_fname, dst_fname );
    FILE * const src = fopen ( src_fname, "rb" );
    if ( src == NULL )
        return ADF_RC_ERROR;

    FILE * const dst = fopen ( dst_fname, "wb" );
    if ( src == NULL ) {
        fclose ( src );
        return ADF_RC_ERROR;
    }

    ADF_RETCODE status = ADF_RC_OK;
    char buffer[BUFSIZE];
    size_t bytes_read;
    while ( ( bytes_read = fread ( buffer, 1, BUFSIZE, src ) ) > 0 ) {
        size_t bytes_written = fwrite ( buffer, 1, bytes_read, dst );
        if ( bytes_written != bytes_read ) {
            log_error ( stderr, "error writing copy to %s\n", dst_fname );
            status = ADF_RC_ERROR;
            break;
        }
    }
    fclose ( src );
    fclose ( dst );
    log_info ( stdout, "Done!\n" );
    return status;
}


// returns number of errors
unsigned compare_bitmaps ( struct AdfVolume * const volOrig,
                           struct AdfVolume * const volUpdate )
{
    struct AdfRootBlock   rbOrig, rbUpdate;
    struct AdfBitmapBlock bmOrig, bmUpdate;

    if ( adfReadRootBlock ( volOrig, (uint32_t) volOrig->rootBlock,
                            &rbOrig ) != ADF_RC_OK )
    {
        log_error ( stderr, "invalid RootBlock on orig. volume, sector %u\n",
                    volOrig->rootBlock );

        return 1;
    }

    if ( adfReadRootBlock ( volUpdate, (uint32_t) volUpdate->rootBlock,
                            &rbUpdate ) != ADF_RC_OK )
    {
        log_error ( stderr, "invalid RootBlock on orig. volume, sector %u\n",
                    volOrig->rootBlock );

        return 1;
    } 
    
    /* Check root bm pages  */
    unsigned nerrors = 0;
    for ( unsigned i = 0 ; i < ADF_BM_PAGES_ROOT_SIZE ; i++ ) {
        ADF_SECTNUM bmPageOrig   = rbOrig.bmPages[i],
                    bmPageUpdate = rbUpdate.bmPages[i];

        if ( bmPageOrig == 0 && bmPageUpdate == 0 )
            continue;
             
        if ( ( bmPageOrig == 0 && bmPageUpdate != 0 ) ||
             ( bmPageOrig != 0 && bmPageUpdate == 0 ) )
        {
            log_error ( stderr, "bmPages[%u]: orig (%u) != ... update (%u)"
                        " - and one of them is 0 -> error -> skipping!\n",
                        i, bmPageOrig, bmPageUpdate );
            nerrors++;
            continue;
        }
        
        if ( bmPageOrig != bmPageUpdate ) {
            // in case of relocation, this could be OK - but return a warning
            log_warning ( stderr, "bmPages[%u] differ: orig (%u) != update (%u)\n",
                          i, bmPageOrig, bmPageUpdate );
        }

        ADF_RETCODE rc = adfReadBitmapBlock ( volOrig, bmPageOrig, &bmOrig );
        if ( rc != ADF_RC_OK ) {
            log_error ( stderr, "error reading bitmap block on vol. orig, block %u\n",
                        bmPageOrig );
            nerrors++;
            continue;
        }

        rc = adfReadBitmapBlock ( volUpdate, bmPageUpdate, &bmUpdate );
        if ( rc != ADF_RC_OK ) {
            log_error ( stderr, "error reading bitmap block on vol. update, block %u\n",
                        bmPageOrig );
            nerrors++;
            continue;
        }

        char bitStrOrig[33];
        char bitStrUpdate[33];
        for ( unsigned i = 0 ; i < ADF_BM_MAP_SIZE ; i++ ) {
            if ( bmOrig.map[i] != bmUpdate.map[i] ) {
                uint32_t
                    valOrig   = bmOrig.map[i],
                    valUpdate = bmUpdate.map[i];
                log_error ( stderr,
                            "bm differ at %u:\n"
                            "  orig   0x%08x  %s\n"
                            "  update 0x%08x  %s\n",
                            i,
                            valOrig,   num32_to_bit_str ( valOrig, bitStrOrig ),
                            valUpdate, num32_to_bit_str ( valUpdate, bitStrUpdate ) );
                nerrors++;
            }
        }
    }

    /* add checking bmExt blocks! */

    return nerrors;
}



void log_error ( FILE * const       file,
                 const char * const format, ... )
{
#if TEST_VERBOSITY > 0
    va_list ap;
    va_start ( ap, format );
    fprintf ( stderr, "Error: " );
    vfprintf ( file, format, ap );
    va_end ( ap );
#else
    (void) file, (void) format;
#endif
}


void log_warning ( FILE * const       file,
                   const char * const format, ... )
{
#if TEST_VERBOSITY > 0
    va_list ap;
    va_start ( ap, format );
    fprintf ( stderr, "Warning: " );
    vfprintf ( file, format, ap );
    va_end ( ap );
#else
    (void) file, (void) format;
#endif
}


void log_info ( FILE * const       file,
                const char * const format, ... )
{
#if TEST_VERBOSITY > 1
    va_list ap;
    va_start ( ap, format );
    //fprintf ( stderr, "Warning: " );
    vfprintf ( file, format, ap );
    va_end ( ap );
#else
    (void) file, (void) format;
#endif
}




char * num32_to_bit_str ( uint32_t num,
                          char     str[33] )
{
    for (unsigned i = 0 ; i <= 31 ; i++ ) {
        uint32_t mask = 1 << i;
        uint32_t bitValue = ( num & mask ) >> i;
        str[i] = bitValue ?
            '.' :  // block free
            'o';   // block allocated
    }
    str[32] = '\0';
    return str;
}
