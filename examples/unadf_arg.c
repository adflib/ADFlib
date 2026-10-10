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

#include <stdlib.h>

#define UNADF_VERSION       ADFLIB_VERSION

struct Args args;

/* parses the command line arguments into global variables */
void parse_args( const int                  argc,
                 const char * const * const argv )
{
    memset( &args, 0, sizeof(args) );

    /* parse flags */
    int i;
    for (i = 1; i < argc && argv[i][0] == '-'; i++) {

        /* check options with arguments first */
        char option = argv[i][1];
        if ( option == 'v' || option == 'd' ) {
            if ( i + 1 >= argc || argv[i][2] != '\0' ) {
                help(stderr);
                exit(1);
            }

            switch (option) {
            case 'v':
                args.vol_number = atoi( argv[i + 1] );
                break;
            case 'd':
                args.extract_dir = argv[i + 1];
                break;
            //default: /* unknown option - show help and exit */
            //    help();
            //    exit(1);
            }

            i++;                            // skip to next arg
            //j = strlen(argv[i]) - 1;      // result of this not used anywhere(???)
            continue;
        }

        /* check options without arguments (can be given together) */
        for (size_t j = 1; argv[i][j]; j++) {
            switch (argv[i][j]) {
            case 'l': args.list_mode     = true; break;
            case 'r': args.list_all      = true; break;
            case 'c': args.use_dircache  = true; break;
            case 's': args.show_sectors  = true; break;
            case 'm': args.show_comments = true; break;
            case 'w': args.win32_mangle  = true; break;
            case 'p':
                args.pipe_mode = true;
                fprintf(stderr, args.list_mode
                    ? "-p option must be used with extraction, ignored\n"
                    : "sending files to pipe\n");
                break;
            case 'h':
                help(stdout);
                exit(0);
            case 'V':
                /* knowing also the build version of ADFlib is useful, in case
                   of any issues and version discrepancy (build vs. runtime) */
                printf("%s, powered by ADFlib: build   v%s (%s)\n"
                       "                           runtime v%s (%s)\n",
                       UNADF_VERSION, ADFLIB_VERSION, ADFLIB_DATE,
                       adfGetVersionNumber(), adfGetVersionDate());
                exit(0);
            default: /* unknown option - show help and exit */
                help(stderr);
                exit(1);
            }
        }
    }

    /* ADF filename must come after options */
    if (i < argc) {
        args.adf_file = argv[i++];
    }
    else {
        help(stderr);
        exit(1);
    }

    /* list of files to extract can follow ADF filename */
    for ( struct AdfList *list = NULL; i < argc; i++) {
        list = adfListNewCell(list, argv[i]);
        if ( !args.file_list ) {
            args.file_list = list;
        }
    }
}

void help(FILE * const stream)
{
    fprintf(stream,
        "\nunADF - an unzip-like utility for .ADF files.\n\n"
        "Usage:  unadf [-lrcsmpwV] [-v n] [-d extractdir] dumpname.adf [files-with-path]\n\n"
        "    -l : lists root directory contents\n"
        "    -r : lists directory tree contents\n"
        "    -c : use dircache data (must be used with -l)\n"
        "    -s : display entries logical block pointer (must be used with -l)\n"
        "    -m : display file comments, if exists (must be used with -l)\n"
        "    -p : send extracted files to pipe (unadf -p dump.adf Pics/pic1.gif | xv -)\n"
        "    -w : mangle filenames to be compatible with Windows filesystems\n"
        "    -h : show help\n"
        "    -V : show version\n\n"
        "    -v n : mount volume #n instead of default #0 volume\n"
        "    -d dir : extract to 'dir' directory\n\n");
}
