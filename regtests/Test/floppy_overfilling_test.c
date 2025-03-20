
//
// regr. test for the issue:
//   https://github.com/adflib/ADFlib/issues/9
//

#include "adflib.h"

#include "log.h"

#define TEST_VERBOSITY 2

#include <stdio.h>
#include <stdlib.h>

typedef struct test_data_s {
    char * const          adfname;
    char * const          filename;
    unsigned char * const buffer;
    const unsigned        bufsize;
    unsigned              blocksize;
    const unsigned char   fstype;
    const int             max_errors;
} test_data_t;


int test_floppy_overfilling( test_data_t * const  tdata );

int verify_file_data( struct AdfVolume * const  vol,
                      char * const              filename,
                      unsigned char * const     buffer,
                      const unsigned            bytes_written,
                      const int                 max_errors );

void pattern_AMIGAMIG( unsigned char *  buf,
                       const unsigned   bufsize );

void pattern_random( unsigned char *  buf,
                     const unsigned   bufsize );


int main(void)
{
    log_init( stderr, TEST_VERBOSITY );

    adfLibInit();

#ifdef _MSC_VER   // visual studio do not understand that const is const...
#define BUF_SIZE 1024 * 1024
#else
    const unsigned BUF_SIZE = 1024 * 1024;
#endif
    unsigned char * const buf = malloc( BUF_SIZE );
    if ( buf == NULL )
        return 1;

    //pattern_AMIGAMIG ( buf, BUFSIZE );
    pattern_random( buf, BUF_SIZE );

    const unsigned test_blocksize[] = { 4096, 4095, 2049, 2048, 1025, 1024, 1023,
        513, 512, 511, 257, 256, 128, 32, 16, 8, 4, 2, 1 };
    const unsigned nblocksizes = sizeof(test_blocksize) / sizeof(unsigned);

    test_data_t tdata_ofs = {
        .adfname    = "test_floppy_overfilling_ofs.adf",
        .filename   = "testfile1.dat",
        .buffer     = buf,
        .bufsize    = BUF_SIZE,
        //.blocksize = ,
        .fstype     =  0,     // OFS
        .max_errors = 10
    };

    test_data_t tdata_ffs = {
        .adfname    = "test_floppy_overfilling_ffs.adf",
        .filename   = "testfile1.dat",
        .buffer     = buf,
        .bufsize    = BUF_SIZE,
        //.blocksize = ,
        .fstype     =  1,     // FFS
        .max_errors = 10
    };

    int status = 0;
    for ( unsigned i = 0 ; i < nblocksizes ; ++i ) {

        tdata_ofs.blocksize = test_blocksize[i];
        status += test_floppy_overfilling( &tdata_ofs );

        tdata_ffs.blocksize = test_blocksize[i];
        status += test_floppy_overfilling( &tdata_ffs );
    }

    free( buf );
    adfLibCleanUp();
    return status;
}


int test_floppy_overfilling( test_data_t * const  tdata )
{
    const char * const fstype_info[] = { "OFS", "FFS" };
    log_info( "Test floppy overfilling, filesystem: %s, blocksize: %d",
              fstype_info[ tdata->fstype ], tdata->blocksize );

    struct AdfDevice * device = adfDevCreate( "dump", tdata->adfname, 80, 2, 11 );
    if ( ! device )
        return 1;

    int status = 1;  // default status: error

    if ( adfCreateFlop( device, "OverfillTest", tdata->fstype ) != ADF_RC_OK ) {
        goto test_floppy_overfilling_cleanup_dev;
    }

    struct AdfVolume * const vol = adfVolMount( device, 0, ADF_ACCESS_MODE_READWRITE );
    if ( ! vol ) {
        goto test_floppy_overfilling_cleanup_dev;
    }

    log_info( "\nFree blocks: %d\n", adfCountFreeBlocks( vol ) );

    struct AdfFile * const output = adfFileOpen( vol, tdata->filename, ADF_FILE_MODE_WRITE );
    if ( ! output ) {
        goto test_floppy_overfilling_cleanup_vol;
    }

    unsigned char *  bufferp       = tdata->buffer;
    unsigned         bytes_written = 0;
    while ( bytes_written + tdata->blocksize < tdata->bufsize ) {  /* <- assumption:
                                                                      bufsize must be larger than
                                                                      floppy size + blocksize */
        const unsigned block_bytes_written = adfFileWrite( output, tdata->blocksize, bufferp );
        bytes_written += block_bytes_written;
        if ( block_bytes_written != tdata->blocksize ) {
            // OK, end of the disk space hit and not all bytes written
            status = 0;
            break;
        }
        bufferp += block_bytes_written; //tdata->blocksize;
        //printf ( "\nFree blocks: %d\n", adfCountFreeBlocks ( vol ) );
    }

    if ( output->pos != bytes_written ) {
        log_error( "written_file->pos (%u) != bytes_written (%u) - ERROR!\n",
                   output->pos, bytes_written );
        status++;
    }

    //adfFlushFile ( output );
    adfFileClose( output );

    log_info( "\nFree blocks: %d\n", adfCountFreeBlocks( vol ) );

    const unsigned free_blocks = adfCountFreeBlocks( vol );
    if ( free_blocks != 0 ) {
        log_error( "\n%d blocks still free after 'overfilling'!\n", free_blocks );
        status++;
    }

    status += verify_file_data( vol, tdata->filename, tdata->buffer,
                                bytes_written, tdata->max_errors );

test_floppy_overfilling_cleanup_vol:
    adfVolUnMount( vol );

test_floppy_overfilling_cleanup_dev:
    adfDevUnMount( device );
    adfDevClose( device );

    log_info( " -> %s\n", status ? "ERROR" : "OK" );

    return status;
}


int verify_file_data( struct AdfVolume * const  vol,
                      char * const              filename,
                      unsigned char * const     buffer,
                      const unsigned            bytes_written,
                      const int                 max_errors )
{
    struct AdfFile * const output = adfFileOpen( vol, filename, ADF_FILE_MODE_READ );
    if ( ! output )
        return 1;

#ifdef _MSC_VER   // visual studio do not understand that const is const...
#define READ_BUFSIZE 1024
#else
    const unsigned READ_BUFSIZE = 1024;
#endif
    unsigned char readbuf[ READ_BUFSIZE ];

    unsigned bytes_read = 0,
             block_bytes_read,
             offset = 0;
    int nerrors = 0;
    do {
        block_bytes_read = adfFileRead( output, READ_BUFSIZE, readbuf );
        bytes_read += block_bytes_read;
        for ( unsigned i = 0 ; i < block_bytes_read ; ++i ) {
            if ( readbuf[ offset % READ_BUFSIZE ] != buffer[ offset ] ) {
                log_error( "Data differ at %u ( 0x%x ): orig. 0x%x, read 0x%x\n",
                           offset, offset,
                           buffer[ offset ],
                           readbuf[ offset % READ_BUFSIZE ] );
                nerrors++;
                if ( nerrors > max_errors )
                    break;
            }
            offset++;
        }
    } while ( block_bytes_read == READ_BUFSIZE &&
              nerrors <= max_errors );

    adfFileClose( output );

    if ( bytes_read != bytes_written ) {
        log_error( "bytes read (%u) != bytes written (%u) -> ERROR!!!\n",
                   bytes_read, bytes_written );
        nerrors++;
    }

    return nerrors;
}


void pattern_AMIGAMIG( unsigned char *  buf,
                       const unsigned   bufsize )
{
    for ( unsigned i = 0 ; i < bufsize ; i += 4 ) {
        buf[i]   = 'A';
        buf[i+1] = 'M';
        buf[i+2] = 'I';
        buf[i+3] = 'G';
     }
}

void pattern_random( unsigned char *  buf,
                     const unsigned   bufsize )
{
    for ( unsigned i = 0 ; i < bufsize ; ++i ) {
        buf[i] = (unsigned char) ( rand() & 0xff );
    }
}
