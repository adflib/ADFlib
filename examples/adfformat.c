/*
 *  adfformat - an utility for creating filesystem (OFS/FFS) on Amiga Disk
 *              Files (ADFs)
 *
 *  Copyright (C) 2023-2025 Tomasz Wolak
 *
 *  adfformat is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  adfformat is distributed in the hope that it will be useful,
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
#include <stdlib.h>
#include <string.h>

#ifndef HAVE_GETOPT
#include "getopt.h"      // use custom getopt
#else
#include <unistd.h>
#endif

#ifndef WIN32
#include <libgen.h>
#endif

typedef struct CmdlineOptions {
    char     *adfName,
             *label;
    unsigned  volidx;
    uint8_t   fsType;
    bool      force,
              verbose,
              help,
              version;
} CmdlineOptions;


bool parse_args( const int * const     argc,
                 char * const * const  argv,
                 CmdlineOptions *      options );

bool bootblockEmpty( const struct AdfDevice * const dev );


void usage(void)
{
    printf( "\nUsage:  adfformat [-f] [-l label] [-p volume] [-t fstype] adf_device\n\n"
            "Quick-format an ADF (Amiga Disk File) or an HDF (Hard Disk File) volume.\n\n"
            "Options:\n"
            "  -f         force formatting even if a filesystem already present\n"
            "             (WARNING: know what you're doing, irreversible data loss!)\n"
            "  -l label   set volume name/label (1-%d characters), default: \"Empty\"\n"
            "  -p volume  volume/partition index, counting from 0, default: 0\n"
            "  -t fstype  set A. DOS filesystem type (OFS/FFS + INTL, DIR_CACHE)\n"
            "  -v         be more verbose\n\n"
            "  -h         show help\n"
            "  -V         show version\n\n"

            "  fstype can be 0-7: flags = 3 least significant bits\n"
            "         bit  set         clr\n"
            "         0    FFS         OFS\n"
            "         1    INTL ONLY   NO_INTL ONLY\n"
            "         2    DIRC&INTL   NO_DIRC&INTL\n\n", ADF_MAX_NAME_LEN );
}


int main( const int            argc,
          char * const * const argv )
{
    CmdlineOptions options;
    if ( ! parse_args( &argc, argv, &options ) ) {
        fprintf( stderr, "Usage info:  adfformat -h\n" );
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

    adfLibInit();

    struct AdfDevice * const device = adfDevOpen( options.adfName,
                                                  ADF_ACCESS_MODE_READWRITE );
    if ( device == NULL ) {
        fprintf( stderr, "Cannot open '%s' - aborting...\n", options.adfName );
        return 1;
    }

    ADF_RETCODE rc = adfDevMount( device );
    if ( rc != ADF_RC_OK ) {
        fprintf( stderr, "adfDevMount failed on %s - aborting...\n",
                 options.adfName );
        adfDevClose( device );
        exit( EXIT_FAILURE );
    }

    if ( options.verbose ) {
        char * const devInfo = adfDevGetInfo( device );
        printf( "%s", devInfo );
        free( devInfo );
    }

    if ( device->nVol <= (int) options.volidx ) {
        fprintf( stderr, "Invalid volume index %u, %s contains %d volume%s.\n",
                 options.volidx, options.adfName, device->nVol,
                 device->nVol > 1 ? "s" : "");
        adfDevClose( device );
        exit( EXIT_FAILURE );
    }

    if ( ! options.force ) {
        if ( adfVolIsFsValid( device->volList[ options.volidx ] ) ) {
            fprintf( stderr, "Volume %u of %s already contains a filesystem (%s)"
                     " - risk of data loss, aborting...\n"
                     "(use -f to enforce formatting, ONLY IF 100%% SURE!)\n",
                     options.volidx, options.adfName,
                     adfVolGetFsStr( device->volList[ options.volidx ] ) );
            adfDevClose( device );
            exit( EXIT_FAILURE );
        }

        // check for any other possible filesystem (any data in bootblock)
        if ( ! bootblockEmpty( device ) ) {
            fprintf( stderr, "Non-zero data found in bootblock area (assuming"
                     "first 2K of the volume).\n"
                     "Volume %u of %s may contain a filesystem - risk of data "
                     "loss, aborting...\n"
                     "(use -f to enforce formatting, ONLY IF 100%% SURE!)\n",
                     options.volidx, options.adfName );
            adfDevClose( device );
            exit( EXIT_FAILURE );
        }
    }
    adfDevUnMount( device );

    const char *devtype_str;
    if ( device->type != ADF_DEVTYPE_UNKNOWN )
        devtype_str = adfDevTypeGetDescription( device->type );
/*  else  if ( device->devType == ADF_DEVTYPE_FLOPDD ) {
        devtype_str     = "DD floppy (880k)";
    } else if ( device->devType == ADF_DEVTYPE_FLOPHD ) {
        devtype_str     = "HD floppy (1760k)";
        } else*/
    else if ( device->class == ADF_DEVCLASS_HARDFILE ) {
        devtype_str = "Hardfile (hdf)";
    } else { //if ( dev->class == DEVCLASS_HARDDISK ) {
        fprintf( stderr, "Devices with RDB (partitioned) are not supported "
                 "(yet...) - aborting...\n" );
        return 1;
    }

    printf( "Formatting %s '%s', volume %d, DOS fstype %d, label '%s'... ",
            devtype_str, options.adfName, options.volidx, options.fsType, options.label );
    fflush( stdout );

    rc = ( device->class == ADF_DEVCLASS_HARDFILE ) ?
        adfCreateHdFile( device, options.label, options.fsType ) :
        adfCreateFlop  ( device, options.label, options.fsType );
    if ( rc != ADF_RC_OK ) {
        fprintf( stderr, "Error formatting '%s'!", options.adfName );
        adfDevClose( device );
        adfLibCleanUp();
        return 1;
    }
    printf( "Done!\n" );

    if ( options.verbose ) {
        char * const devInfo = adfDevGetInfo( device );
        printf( "%s", devInfo );
        free( devInfo );
    }

    adfDevClose( device );
    adfLibCleanUp();

    return 0;
}



/* return value: true - valid, false - invalid */
bool parse_args( const int * const     argc,
                 char * const * const  argv,
                 CmdlineOptions *      options )
{
    // set default options
    memset( options, 0, sizeof( CmdlineOptions ) );
    options->fsType  = 1;
    options->volidx  = 0;
    options->label   = "Empty";
    options->force   =
    options->verbose =
    options->help    =
    options->version = false;

    unsigned nerrors = 0;
    const char * valid_options = "l:p:t:fhvV";
    int opt;
    while ( ( opt = getopt( *argc, (char * const *) argv, valid_options ) ) != -1 ) {
//        printf ( "optind %d, opt %c, optarg %s\n", optind, ( char ) opt, optarg );
        switch ( opt ) {
        case 'f': {
            options->force = true;
            continue;
        }
        case 'l': {
            options->label = optarg;
            size_t labelLen = strlen( options->label );
            if ( labelLen < 1 ||
                 labelLen > ADF_MAX_NAME_LEN )
            {
                fprintf( stderr, "Invalid label '%s' (1 up to %d characters, "
                         "instead of given %ld).\n",
                         options->label,  ADF_MAX_NAME_LEN, labelLen );
                nerrors++;
            }
            continue;
        }

        case 'p': {
            // partition (volume) number (index)
            char * endptr = NULL;
            options->volidx = ( unsigned int ) strtoul( optarg, &endptr, 10 );
            if ( endptr == optarg ||
                 options->volidx > 255 )  // is there a limit for max. number of partitions???
            {
                fprintf( stderr, "Invalid volume/partition %u.\n", options->volidx );
                nerrors++;
            }
            continue;
        }

        case 't': {
            // filesystem type (subtype - just the number id in 'DOSn' signature)
            char * endptr = NULL;
            options->fsType = ( uint8_t ) strtoul( optarg, &endptr, 10 );
            if ( endptr == optarg ||
                 options->fsType > 7 ||
                 ( options->fsType > 0 &&
                   options->fsType % 2 == 0 ) )
            {
                fprintf( stderr, "Invalid filesystem type %u.\n", options->fsType );
                nerrors++;
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

    if ( nerrors > 0 )
        return false;

    if ( optind != *argc - 1 ) {
        fprintf( stderr, "Missing the name of an adf file/device.\n" );
        return false;
    }

    options->adfName = argv[ optind ];
    return true;
}


#define BOOTBLOCK_CHECK_SIZE 2048

/* if there is any non-zero data found in "bootblock" (first 4 512-byte blocks),
   then assume there can be some filesystem structure (return false) */
bool bootblockEmpty( const struct AdfDevice * const dev )
{
    //uint8_t bblock[ dev->geometry.blockSize ];
    uint8_t bblock[ BOOTBLOCK_CHECK_SIZE ];
    ADF_RETCODE rc = adfDevReadBlock( dev, 0, BOOTBLOCK_CHECK_SIZE, bblock );
    if ( rc != ADF_RC_OK )
        return false;

    unsigned sum = 0;
    for ( unsigned i = 0; i < BOOTBLOCK_CHECK_SIZE; i++ )
        sum += bblock[ i ];

    return ( sum == 0 );
}
