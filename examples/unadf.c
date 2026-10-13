/*
 *  unadf - an unzip-like utility for Amiga disk images (ADF)
 *
 *  Copyright (C) 1999-2018 Laurent Clevy
 *                2019      Stuart Caie
 *                2023      Stuart Caie, Tomasz Wolak
 *                2024-2026 Tomasz Wolak
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 */


#include "unadf_arg.h"
#include "unadf_extract.h"
#include "unadf_info.h"

#include "adf_dev_driver_nativ.h"

#include <stdlib.h>

int main( const int          argc,
          const char * const argv[] )
{
    if ( adfLibInit() != ADF_RC_OK ) {
        fprintf( stderr, "Error initializing ADFlib - aborting...\n" );
        return EXIT_FAILURE;
    }

    parse_args(argc, argv);

    adfAddDeviceDriver( &adfDeviceDriverNative );

    /* open device */
    struct AdfDevice * const dev = adfDevOpen( args.adf_file,
                                               ADF_ACCESS_MODE_READONLY );
    if ( !dev ) {
        fprintf(stderr, "%s: can't open device\n", args.adf_file);
        return EXIT_FAILURE;
    }

    int rc = EXIT_SUCCESS;

    /* mount device */
    if (adfDevMount(dev) != ADF_RC_OK) {
        fprintf(stderr, "%s: can't mount device\n", args.adf_file);
        rc = EXIT_FAILURE;
        goto clean_up;
    }
    if ( !args.pipe_mode ) {
        print_device(dev);
    }

    /* mount volume */
    if ( args.vol_number < 0 ||
         args.vol_number >= dev->nVol )
    {
        fprintf(stderr, "%s: volume %d is invalid (device has %d volume(s))\n",
                args.adf_file, args.vol_number, dev->nVol);
        rc = EXIT_FAILURE;
        goto clean_up;
    }

    struct AdfVolume * const vol = adfVolMount( dev, args.vol_number,
                                                ADF_ACCESS_MODE_READONLY );
    if ( !vol ) {
        fprintf(stderr, "%s: can't mount volume %d\n",
                args.adf_file, args.vol_number);
        rc = EXIT_FAILURE;
        goto clean_up;
    }
    if ( !args.pipe_mode ) {
        print_volume(vol);
    }

    if ( args.pipe_mode && !args.file_list ) {
        fprintf(stderr, "can't use pipe mode without giving a file name to extract\n");
        rc = EXIT_FAILURE;
        goto clean_up;
    }

    if ( args.list_mode || args.list_all ) {
        /* list files */
        if ( args.use_dircache && adfVolHasDIRCACHE(vol) ) {
            adfEnvSetProperty(ADF_PR_USEDIRC, true);
            puts("Using dir cache blocks.");
        }
        if ( args.list_all ) {
            /* list all files recursively */
            struct AdfList * const list = adfGetRDirEnt(vol, vol->curDirPtr, true);
            print_tree(list, "");
            adfFreeDirList(list);
        }
        else {
            /* list contents of root directory */
            struct AdfList * const list = adfGetDirEnt(vol, vol->curDirPtr);
            for ( struct AdfList *node = list; node; node = node->next ) {
                print_entry(node->content, "");
            }
            adfFreeDirList(list);
        }
    }
    else {
        /* extract files */
        if ( args.file_list ) {
            /* extract list of files given on command line */
            char filepath[PATH_MAX + 1];
            unsigned nerrors = 0;
            for ( struct AdfList *node = args.file_list; node; node = node->next ) {
                if ( strlen( node->content ) > PATH_MAX ) {
                    fprintf( stderr, "Path too long, error: '%s'\n",
                             (const char *) node->content );
                    nerrors++;
                    continue;
                }
                strncpy( filepath, node->content, PATH_MAX );
                filepath[ PATH_MAX ] = '\0';

                if ( extract_filepath( vol, filepath ) != ADF_RC_OK )
                    nerrors++;

                if ( args.pipe_mode ) {
                    /* only extract one file in pipe mode */
                    break;
                }
            }

            if ( nerrors > 0 )
                rc = EXIT_FAILURE;
        }
        else {
            /* extract all files */
            struct AdfList * const list = adfGetRDirEnt(vol, vol->curDirPtr, true);
            if ( extract_tree(vol, list, "") != ADF_RC_OK )
                rc = EXIT_FAILURE;
            adfFreeDirList(list);
        }
    }

clean_up:
    if (vol) adfVolUnMount(vol);
    if (dev && dev->mounted) adfDevUnMount(dev);
    if (dev) adfDevClose(dev);
    if ( args.file_list ) adfListFree( args.file_list );
    adfLibCleanUp();

    return rc;
}
