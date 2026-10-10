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

#include "unadf_extract.h"

#include "unadf_arg.h"
#include "unadf_info.h"
#include "unadf_pathutil.h"
#include "unadf_util.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#define EXTRACT_BUFFER_SIZE 8192

#ifdef WIN32

 //#if !defined(__GNUC__)
#if !defined(__MINGW32__) && !defined(_CYGWIN)
#include <io.h>         // for open(), write(), ...
#endif

#else
# include <unistd.h>

# ifndef O_BINARY
# define O_BINARY 0
# endif

#endif  // WIN32


/* extracts all files, recursing into directories */
void extract_tree( struct AdfVolume * const vol,
                   const struct AdfList    *node,
                  const char * const        path )
{
#ifdef DEBUG_UNADF
    fprintf( stderr, "%s: path '%s'\n", __func__, path );
#endif

    for (; node; node = node->next) {
        const struct AdfEntry *e = node->content;

        /* extract file or create directory */
        char * const out = output_name(path, e->name);
#ifdef DEBUG_UNADF
        fprintf( stderr, "%s: out '%s'\n", __func__, out );
#endif
        if (e->type == ADF_ST_DIR) {
            if ( !args.pipe_mode ) {
                printf("x - %s/\n", out);
                mkdir_if_needed(out, 0777 ); // temp. permissionss
                                             // ("w" is needed for extraction)
            }
        }
        else if (e->type == ADF_ST_FILE) {
            extract_file(vol, e->name, out, permissions(e));
        }

        /* recurse into subdirectories */
        if (node->subdir) {
            char *newpath = join_path(path, e->name);
            if (adfChangeDir(vol, e->name) == ADF_RC_OK) {
                extract_tree(vol, node->subdir, newpath);
                adfParentDir(vol);
            }
            else {
                fprintf(stderr, "%s: can't find directory %s in volume\n",
                        args.adf_file, newpath);
            }
            free(newpath);
        }

        // set timestamp
        if ( !args.pipe_mode ) {
            set_file_date(out, e);
            set_permissions( out, permissions(e) ); // real permissions
        }

        free(out);
    }
}

/* follows a path to a file on disk and extracts just that file */
void extract_filepath( struct AdfVolume * const vol,
                       char * const             filepath )
{
    assert( filepath != NULL );
    assert( vol != NULL );

    /* skip any leading slashes */
    char *p;
    for (p = filepath; *p == '/';) p++;

    /* switch to root directory */
    adfToRootDir(vol);

    /* find each path element and adfChangeDir() to each element */
    char *element;
    for (element = p; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (*element) {
                if (adfChangeDir(vol, element) != ADF_RC_OK) {
                    fprintf(stderr, "%s: can't find directory %s in volume\n",
                            args.adf_file, filepath);
                    return;
                }
            }
            *p = '/';
            element = p + 1;
        }
    }

    /* extract the file using the final path element */
    if (*element) {
        struct AdfEntry * const entry = malloc(sizeof(struct AdfEntry));
        if (adfGetEntry(vol, vol->curDirPtr, element, entry) != ADF_RC_OK) {
            fprintf(stderr, "%s: can't get file permissions for %s\n",
                    args.adf_file, filepath);
            return;
        }

        char *out;
        if (element == filepath) {
            /* no directory parts, just a filename */
            out = output_name("", element);
        }
        else {
            element[-1] = 0; /* split path and file */
            out = output_name(filepath, element);
        }

        extract_file(vol, element, out, permissions(entry));
        adfFreeEntry(entry);
        free(out);
    }
}

/* copies a file from the volume to a given output filename */
void extract_file( struct AdfVolume * const vol,
                   const char * const       filename,
                   const char * const       out,
                   const mode_t             perms )
{
    struct AdfFile *f = NULL;
    uint8_t buf[EXTRACT_BUFFER_SIZE];
    int fd = 0;

    if ((f = adfFileOpen(vol, filename, ADF_FILE_MODE_READ)) == NULL)  {
        fprintf(stderr, "%s: can't find file %s in volume\n", args.adf_file, filename);
        goto error_handler;
    }

    /* open local file for writing */
    if ( args.pipe_mode ) {
#ifdef _WIN32
        fd = _fileno( stdout );
        const int result = _setmode( fd, _O_BINARY );
        if ( result == -1 ) {
            perror( "Cannot set binary mode" );
            goto error_handler;
        }
#else
        fd = fileno( stdout );
#endif
    }
    else {
        printf("x - %s\n", out);
        fd = open(out, O_CREAT | O_WRONLY | O_TRUNC | O_BINARY, perms);
        if (fd < 0) {
            print_error(out);
            goto error_handler;
        }
    }

    /* copy from volume to local file until EOF */
    while (!adfFileAtEOF(f)) {
        unsigned n = adfFileRead(f, sizeof(buf), buf);
        if (n != sizeof(buf) && !adfFileAtEOF(f)) {
            fprintf(stderr, "%s: error reading %s at %u\n",
                    args.adf_file, filename, adfFileGetPos(f));
            goto error_handler;
        }

        if (write(fd, buf, n) != n) {
            print_error(out);
            goto error_handler;
        }
    }

error_handler:
    if ( fd && !args.pipe_mode ) close(fd);
    if (f) adfFileClose(f);
}
