/*
 *  adfsalvage - show and restore deleted files on Amiga Disk Files (ADFs)
 *
 *  Copyright (C) 2023-2025 Tomasz Wolak
 *
 *  adfsalvage is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  adfsalvage is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Foobar; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <adflib.h>
#include <errno.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef HAVE_GETOPT
#include "getopt.h"      // use custom getopt
#else
#include <unistd.h>
#endif

#ifndef WIN32
#include <libgen.h>
#endif

typedef struct CmdlineOptions {
    char *                   adfDevName;
    unsigned                 volidx;
    struct AdfVectorSectors  entries;
    bool                     verbose,
                             help,
                             version;
} CmdlineOptions;


bool parse_args( const int * const     argc,
                 char * const * const  argv,
                 CmdlineOptions *      options );

void showDeletedEntries( struct AdfVolume * const      vol,
                         const struct AdfList * const  list );
char * getBlockTypeStr( const int  block2ndaryType );

ADF_RETCODE checkEntriesToUndelete(
    struct AdfVolume * const               vol,
    const struct AdfList * const           deletedEntriesList,
    const struct AdfVectorSectors * const  entriesHeaderBlocks );

bool entryIsDeleted( const ADF_SECTNUM             entryBlockIdx,
                     const struct AdfList * const  deletedEntriesList );


ADF_RETCODE undeleteFiles(
    struct AdfVolume * const                vol,
    const struct AdfVectorSectors * const   entriesHeaderBlocks );

ADF_RETCODE undeleteFile( struct AdfVolume * const  vol,
                          ADF_SECTNUM               block );


void usage(void)
{
    printf( "\nUsage:  adfsalvage  [-p volume] adf_device [file header block to undelete]...\n\n"
            "Salvage files from an ADF (Amiga Disk File) or an HDF (Hard Disk File) volume.\n\n"
            "Options:\n"
            "  -p volume  volume/partition index, counting from 0, default: 0\n"
            "  -v         be more verbose\n\n"
            "  -h         show help\n"
            "  -V         show version\n\n" );
}


int main( const int     argc,
          char * const argv[] )
{
    CmdlineOptions options;
    if ( ! parse_args( &argc, argv, &options ) ) {
        fprintf( stderr, "Usage info:  adfsalvage -h\n" );
        exit( EXIT_FAILURE );
    }

    if ( options.help ) {
        usage();
        exit( EXIT_SUCCESS );
    }

    if ( options.version ) {
        printf( "%s, powered by ADFlib: build   v%s (%s)\n"
                "                           runtime v%s (%s)\n",
                ADFLIB_VERSION,
                ADFLIB_VERSION, ADFLIB_DATE,
                adfGetVersionNumber(), adfGetVersionDate() );
        exit( EXIT_SUCCESS );
    }

    int status = 0;

    adfLibInit();
    adfEnvSetProperty( ADF_PR_USEDIRC, true );
 
    struct AdfDevice * const dev = adfDevOpen( options.adfDevName,
                                               ADF_ACCESS_MODE_READWRITE );
    if ( dev == NULL ) {
        fprintf( stderr, "Error opening device '%s' - aborting...\n",
                 options.adfDevName );
        status = 1;
        goto clean_up_env;
    }

    ADF_RETCODE rc = adfDevMount( dev );
    if ( rc != ADF_RC_OK ) {
        fprintf( stderr, "Error mounting device '%s' - aborting...\n",
                 options.adfDevName );
        status = 2;
        goto clean_up_dev_close;
    }

    //adfDevInfo ( dev );

    struct AdfVolume * const vol = adfVolMount( dev, (int) options.volidx,
                                                ADF_ACCESS_MODE_READWRITE );
    if ( vol == NULL ) {
        fprintf( stderr, "Error mounting volume %d of '%s' - aborting...\n",
                 options.volidx, options.adfDevName );
        status = 3;
        goto clean_up_dev_unmount;
    }

    /* check block allocation bitmap - if invalid, cannot salvage (dangerous)! */
    if ( ! adfVolBitmapIsMarkedValid ( vol ) ) {
        fprintf( stderr, "Block allocation bitmap of the volume is marked as invalid "
                 "-> cannot reliably salvage - aborting...\n" );
        status = 4;
        goto clean_up_volume;
    }

    /* get list of deleted entries on the volume */
    struct AdfList * const deletedEntries = adfGetDelEnt( vol );
    //if ( list == NULL )
    //    goto clean_up_volume;

    if ( options.entries.sectors == NULL ) {
        /* nothing specified to salvage - just show deleted entries */
        showDeletedEntries( vol, deletedEntries );
        goto clean_up_free_del_list;
    }

    //adfVolUnMount(vol);

    /* try to salvage / undelete specified files */
    rc = undeleteFiles( vol, &options.entries );
    if ( rc != ADF_RC_OK )
        status = 5;

clean_up_free_del_list:
    options.entries.destroy( &options.entries );
    adfFreeDelList( deletedEntries );

clean_up_volume:
    adfVolUnMount( vol );

clean_up_dev_unmount:
    adfDevUnMount( dev );

clean_up_dev_close:
    adfDevClose( dev );

clean_up_env:
    adfLibCleanUp();


    return status;
}


/* return value: true - valid, false - invalid */
bool parse_args( const int * const     argc,
                 char * const * const  argv,
                 CmdlineOptions *      options )
{
    // set default options
    memset( options, 0, sizeof( CmdlineOptions ) );
    options->volidx  = 0;
    options->verbose =
    options->help    =
    options->version = false;
    options->entries = adfVectorSectorsCreate( 0 );

    const char * valid_options = "p:hvV";
    int opt;
    while ( ( opt = getopt( *argc, (char * const *) argv, valid_options ) ) != -1 ) {
//        printf ( "optind %d, opt %c, optarg %s\n", optind, ( char ) opt, optarg );
        switch ( opt ) {
        case 'p': {
            // partition (volume) number (index)
            char * endptr = NULL;
            options->volidx = ( unsigned int ) strtoul( optarg, &endptr, 10 );
            if ( endptr == optarg ||
                 options->volidx > 255 )  // is there a limit for max. number of partitions???
            {
                fprintf( stderr, "Invalid volume/partition %u.\n", options->volidx );
                return false;
            }
            continue;
        }

        case 'v':
            options->verbose = true;
            continue;

        case 'h':
            options->help = true;
            return true;

        case 'V':
            options->version = true;
            return true;

        default:
            return false;
        }
    }

    /* the name of the adf device - required */
    if ( optind > *argc - 1 ) {
        fprintf( stderr, "Missing the name of an adf file/device.\n" );
        return false;
    }
    options->adfDevName = argv[ optind++ ];

    if ( optind >= *argc )
        return true;

    /* (optional) list of files (given as sectors of their file header blocks)
       to undelete */
    options->entries = adfVectorSectorsCreate( (unsigned)( *argc - optind ) );
    if ( options->entries.sectors == NULL ) {
        fprintf( stderr, "Memory allocation error.");
        return false;
    }

    for ( unsigned i = 0 ; i < options->entries.nItems ; i++ ) {
        char * endptr = NULL;
        errno = 0;
        //printf ("Adding %s\n", argv[ optind ] );
        const char * const blockStr = argv[ optind++ ];
        unsigned long blockIdx = strtoul( blockStr, &endptr, 10 );
        if ( errno != 0 ||
             endptr == blockStr ||
             blockIdx > 0x7fffffff )          /* overflow / negative */
        {
            fprintf( stderr, "Invalid file header block number '%s'.\n", blockStr );
            options->entries.destroy( &options->entries );
            return false;
        }
        options->entries.sectors[ i ] = (ADF_SECTNUM) blockIdx;
    }

    return true;
}


void showDeletedEntries( struct AdfVolume * const      vol,
                         const struct AdfList * const  list )
{
    if ( list == NULL ) {
        printf ( "No deleted entries found.\n" );
        return;
    }
    const char * const  line =
        "-----------------------------------------------------------||-------------------------------\n";
    printf(
        //"\nDeleted entries found:\n%s"
        "%s"
        "                     deleted entry                         ||          parent\n"
        "%s"
        "               name               |     type      | sector || sector |     type      | name\n"
        "%s", line, line, line );
    unsigned nDeletedEntries = 0;
    for ( const struct AdfList * cell = list; cell != NULL; cell = cell->next ) {
        struct GenBlock * const block = (struct GenBlock *) cell->content;

        struct AdfEntryBlock  parent;
        const ADF_RETCODE rc = adfReadEntryBlock( vol, block->parent, &parent );
        const char * const parentName =
            ( rc == ADF_RC_OK ) ? parent.name : "error getting entry name";
        printf( "%33s | %9s,%3d | %6d || %6d | %9s,%3d | %s\n",
                block->name,
                //block->type,
                getBlockTypeStr ( block->secType ),
                block->secType,
                block->sect,
                block->parent,
                getBlockTypeStr ( parent.secType ),
                parent.secType,
                parentName );
        nDeletedEntries++;
    }
    printf( "%s\nNumber of deleted entries: %u\n", line, nDeletedEntries );
}


char * getBlockTypeStr( const int  block2ndaryType )
{
    switch ( block2ndaryType ) {
    case ADF_ST_ROOT:       return "root";        
    case ADF_ST_DIR:        return "dir";
    case ADF_ST_FILE:       return "file";
    case ADF_ST_LFILE:      return "file link";
    case ADF_ST_LDIR:       return "dir link";
    case ADF_ST_LSOFT:      return "soft link";
    case ADF_T_LIST:        return "t list";
    case ADF_T_DATA:        return "t data";
    case ADF_T_DIRC:        return "t dirc";
    }
    return "unknown";
}


ADF_RETCODE checkEntriesToUndelete(
    struct AdfVolume * const               vol,
    const struct AdfList * const           deletedEntriesList,
    const struct AdfVectorSectors * const  entriesHeaderBlocks )
{
    for ( unsigned i = 0 ; i < entriesHeaderBlocks->nItems ; i++ ) {
        const ADF_SECTNUM block = entriesHeaderBlocks->sectors[ i ];

        /* check if block number (index) is valid */
        if ( block < 2 ||               /* exclude bootblock */
             (unsigned) block > adfVolGetSizeInBlocks( vol ) )
        {
            fprintf( stderr, "Invalid file header block number %d.\n", block );
            return ADF_RC_ERROR;
        }

        /* check if the entry at the given block exists and is valid */
        struct AdfFileHeaderBlock fhb;
        ADF_RETCODE rc = adfReadEntryBlock( vol, (ADF_SECTNUM) block,
                                            (struct AdfEntryBlock * const) &fhb );
        if ( rc != ADF_RC_OK ) {
            fprintf( stderr, "Error reading entry (file header block) at %d.\n",
                     block );
            return rc;
        }
        if ( fhb.secType != ADF_ST_FILE ) {
            fprintf( stderr, "Entry at block %d is not a file but %s.\n",
                     block, getBlockTypeStr ( fhb.secType ) );
            return ADF_RC_ERROR;
        }

        /* check if the entry at the given block is a deleted entry */
        if ( ! entryIsDeleted( block, deletedEntriesList ) ) {
            fprintf( stderr, "Entry at block %d is not deleted.\n",
                     block );
            return ADF_RC_ERROR;
        }

        /* check if file metadata is valid */
        //rc = adfCheckFile ( vol, block, &fhb, 0 );
        rc = adfCheckEntry( vol, block, 0 );
        if ( rc != ADF_RC_OK ) {
            fprintf( stderr, "Error checking file at block %d - cannot undelete it.\n",
                     block );
            return rc;
        }

        /* check if parent dir. is valid */
        /*rc = adfCheckparent ( vol, fhb->parent );
        if ( rc != ADF_RC_OK ) {
            fprintf ( stderr, "Entry at %d: invalid parent %d.\n",
                      block, fhb->parent );
            return rc;
            }*/
    }
    return ADF_RC_OK;
}


bool entryIsDeleted( const ADF_SECTNUM             entryBlockIdx,
                     const struct AdfList * const  deletedEntriesList )
{
    for ( const struct AdfList * deletedEntry = deletedEntriesList ;
          deletedEntry != NULL ;
          deletedEntry = deletedEntry->next )
        {
            const struct GenBlock * const block =
                ( const struct GenBlock * const ) deletedEntry->content;
            if ( block->sect == entryBlockIdx )
                return true;
        }
    return false;
}


ADF_RETCODE undeleteFiles (
    struct AdfVolume * const               vol,
    const struct AdfVectorSectors * const  entriesHeaderBlocks )
{
    for ( unsigned i = 0 ; i < entriesHeaderBlocks->nItems ; i++ ) {
        ADF_RETCODE rc = undeleteFile( vol, entriesHeaderBlocks->sectors[ i ] );
        if ( rc != ADF_RC_OK )
            return rc;
    }
    return ADF_RC_OK;
}


ADF_RETCODE undeleteFile( struct AdfVolume * const  vol,
                          ADF_SECTNUM               block )
{
    printf( "Undeleting entry at %d... ", block );
    fflush( stdout );

    struct AdfFileHeaderBlock fhb;
    ADF_RETCODE rc = adfReadEntryBlock( vol, block,
                                        (struct AdfEntryBlock * const) &fhb );
    if ( rc != ADF_RC_OK ) {
        fprintf( stderr, "Error reading entry (file header block) at %d.\n",
                 block );
        return rc;
    }

    rc = adfUndelEntry( vol, fhb.parent, block );
    if ( rc != ADF_RC_OK ) {
        fprintf( stderr, "Error %d undeleting file at %d.\n", rc, block );
        return rc;
    }
    printf("Done!\n");
    return ADF_RC_OK;
}
