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

#define DEFAULT_DIR_MODE 0777  // temp. directory permissions (before extracting
                               // its contents ("w" is needed for extraction)

/* extracts all files, recursing into directories */
ADF_RETCODE extract_tree( struct AdfVolume * const vol,
                          const struct AdfList    *node,
                          const char * const       path )
{
#ifdef DEBUG_UNADF
    fprintf( stderr, "%s: path '%s'\n", __func__, path );
#endif

    unsigned nerrors = 0;
    for (; node; node = node->next) {
        const struct AdfEntry *e = node->content;

        // get destination path name
        char * const out = output_name(path, e->name);
#ifdef DEBUG_UNADF
        fprintf( stderr, "%s: out '%s'\n", __func__, out );
#endif

        // create destination directory
        if ( !args.pipe_mode &&
             make_parent_dir_path( out, DEFAULT_DIR_MODE ) != 0 )
        {
            nerrors++;
            // if parent dir cannot be created - cannot extract
            free(out);
            continue;
        }

        // extract file or create directory
        if (e->type == ADF_ST_DIR) {
            if ( !args.pipe_mode ) {
                printf("x - %s/\n", out);
                mkdir_if_needed( out, DEFAULT_DIR_MODE );
            }
        }
        else if (e->type == ADF_ST_FILE) {
            if ( extract_file(vol, e->name, out, permissions(e) ) != ADF_RC_OK )
                nerrors++;
        }

        /* recurse into subdirectories */
        if (node->subdir) {
            char *newpath = join_path(path, e->name);
            if (adfChangeDir(vol, e->name) == ADF_RC_OK) {
                if ( extract_tree(vol, node->subdir, newpath) != ADF_RC_OK )
                    nerrors++;
                adfParentDir(vol);
            }
            else {
                fprintf(stderr, "%s: can't find directory %s in volume\n",
                        args.adf_file, newpath);
                nerrors++;
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

    return ( nerrors == 0 ? ADF_RC_OK : ADF_RC_ERROR );
}

/* follows a path to a file on disk and extracts just that file */
ADF_RETCODE extract_filepath( struct AdfVolume * const vol,
                              char * const             filepath )
{
#ifdef DEBUG_UNADF
    fprintf( stderr, "%s: path '%s'\n", __func__, filepath );
#endif
    assert( filepath != NULL );
    assert( vol != NULL );

    ADF_RETCODE rc = ADF_RC_OK;

    /* skip any leading slashes */
    char *p;
    for ( p = filepath; *p == DIRSEP; p++ );

    /* switch to root directory */
    adfToRootDir(vol);

    /* find each path element and adfChangeDir() to each element */
    char *element;
    for (element = p; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (*element) {
                if (adfChangeDir(vol, element) != ADF_RC_OK) {
                    fprintf(stderr, "%s: can't find directory %s in volume\n",
                            args.adf_file, filepath);
                    return ADF_RC_ERROR;
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
            return ADF_RC_ERROR;
        }

        char *out;
        if (element == filepath) {
            /* no directory parts, just a filename */
            out = output_name("", element);
        }
        else {
            element[-1] = '\0'; /* split path and file */
            out = output_name(filepath, element);
        }

        if ( !args.pipe_mode &&
             make_parent_dir_path( out, DEFAULT_DIR_MODE ) != 0 )
        {
            adfFreeEntry(entry);
            free(out);
            return ADF_RC_ERROR;
        }

        rc = extract_file(vol, element, out, permissions(entry));
        adfFreeEntry(entry);
        free(out);
    }

    return rc;
}

/* copies a file from the volume to a given output filename */
ADF_RETCODE extract_file( struct AdfVolume * const vol,
                          const char * const       filename,
                          const char * const       out,
                          const mode_t             perms )
{
#ifdef DEBUG_UNADF
    fprintf( stderr, "%s: filename '%s', destination '%s'\n",
             __func__, filename, out );
#endif

    ADF_RETCODE rc = ADF_RC_OK;

    struct AdfFile * const f = adfFileOpen(vol, filename, ADF_FILE_MODE_READ);
    if ( f == NULL ) {
        fprintf(stderr, "%s: can't find file %s in volume\n", args.adf_file, filename);
        rc = ADF_RC_ERROR;
        goto clean_up;
    }

    /* open local file for writing */
    int fd = 0;
    if ( args.pipe_mode ) {
#ifdef _WIN32
        fd = _fileno( stdout );
        const int result = _setmode( fd, _O_BINARY );
        if ( result == -1 ) {
            perror( "Cannot set binary mode" );
            rc = ADF_RC_ERROR;
            goto clean_up;
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
            rc = ADF_RC_ERROR;
            goto clean_up;
        }
    }

    /* copy from volume to local file until EOF */
    uint8_t * const buf = malloc( EXTRACT_BUFFER_SIZE );
    if ( buf == NULL ) {
        perror(__func__);
        rc = ADF_RC_ERROR;
        goto clean_up;
    }

    while (!adfFileAtEOF(f)) {
        unsigned n = adfFileRead(f, sizeof(buf), buf);
        if (n != sizeof(buf) && !adfFileAtEOF(f)) {
            fprintf(stderr, "%s: error reading %s at %u\n",
                    args.adf_file, filename, adfFileGetPos(f));
            rc = ADF_RC_ERROR;
            break;
        }

        if (write(fd, buf, n) != n) {
            print_error(out);
            rc = ADF_RC_ERROR;
            break;
        }
    }

    free( buf );

clean_up:
    if ( fd && !args.pipe_mode ) close(fd);
    if (f) adfFileClose(f);

    return rc;
}
