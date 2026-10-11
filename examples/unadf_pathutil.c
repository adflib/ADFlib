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

#include "unadf_pathutil.h"

#include "unadf_arg.h"
#include "unadf_fs.h"
#include "unadf_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#  if defined _MSC_VER
#    define strcasecmp _stricmp
#  endif
#  define DIRSEP '\\'
#else
#  define DIRSEP '/'
#endif  // WIN32

static void fix_win32_path( char *const name );


/* combines path and file to "path/file", or just "file" if path is blank */
char * join_path( const char *const path,
                  const char *const file )
{
    char *newpath = malloc(strlen(path) + strlen(file) + 2);
    if (!newpath) {
        print_error( args.adf_file );
        exit(1);
    }
    sprintf(newpath, "%s%s%s", path, *path ? "/" : "", file);
    return newpath;
}

/* creates a suitable output filename from the amiga filename */
char * output_name( const char *const path,
                    const char *const name )
{
    /* maybe (extract_dir + "/") + path + "/" + name + maybe "_" + "\0" */
    const size_t dirlen = ( args.extract_dir ? strlen(args.extract_dir) + 1 : 0 );
    char *const out = malloc(dirlen + strlen(path) + strlen(name) + 3);
    if (!out) {
        print_error( args.adf_file );
        exit(1);
    }

    /* copy user's extract directory if needed */
    if (dirlen) {
        strcpy(out, args.extract_dir );
        out[dirlen - 1] = DIRSEP;
    }

    /* copy path and name, translating '/' to platform's separator */
    char *o = &out[dirlen];
    for ( const char *s = path; *s; s++ ) {
        *o++ = (*s == '/') ? DIRSEP : *s;
    }
    if (*path) {
        *o++ = DIRSEP;
    }
    strcpy(o, name);

    /* search for "../" and change to "xx" to stop directory traversal */
    for (o = &out[dirlen]; *o; o++) {
        if (o[0] == '.' && o[1] == '.' && (o[2] == '/' || o[2] == '\\')) {
            o[0] = o[1] = 'x';
            o += 2;
        }
    }

    /* alter paths that are a problem for Windows */
    if ( args.win32_mangle )
        fix_win32_path( out );

    if ( !args.pipe_mode ) {
        /* create directories leading up to the leaf name, if needed */
        char *last_slash = strrchr( out, DIRSEP );
        if ( last_slash ) {
            *last_slash = '\0';                 // temp. cut off leaf name
            make_dir_tree( out, DIRSEP );
            *last_slash = DIRSEP;
        }
    }

    return out;
}


/* alter file- and directory names that would cause problems on Windows:
 * https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file
 * 1. replace characters : * " ? < > | / \ and chars 1-31 with underscore
 * 2. replace any trailng dot or space with underscore (breaks Windows File Explorer)
 * 3. if filename is CON PRN AUX NUL COM[1-9] LPT[1-9], regardless of case or
 *    extension, insert an underscore (vestiges of CP/M retained by Windows)
 *
 * In this last case, the name will be _extended_ by 1 character!
 */
static void fix_win32_name( char *const name )
{
    /* work forwards through name, replacing forbidden characters */
    char *o;
    for (o = name; *o; o++) {
        if (*o < 32   || *o == ':' || *o == '*' || *o == '"' || *o == '?' ||
            *o == '<' || *o == '>' || *o == '|' || *o == '/' || *o == '\\')
        {
            *o = '_';
        }
    }
    /* work backwards through name, replacing forbidden trailing chars */
    for (o--; o >= name && (*o == '.' || *o == ' '); o--) {
       *o = '_';
    }

    /* temporarily remove file extension */
    char *ext = strchr(name, '.');
    if (ext) {
        *ext = 0;
    }

    /* is the name (excluding extension) a reserved name in Windows? */
    const char *reserved_names[] = {
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };
    for (unsigned int i = 0; i < sizeof(reserved_names)/sizeof(char **); i++) {
       if (strcasecmp(reserved_names[i], name) == 0) {
           if (ext) {
               /* restore extension and insert underscore, PRN.txt -> PRN_.txt */
               *ext = '.';
               memmove(&ext[1], ext, strlen(ext) + 1);
               *ext = '_';
           }
           else {
               /* no extension: append underscore, e.g. CON -> CON_ */
               o = &name[strlen(name)];
               *o++ = '_';
               *o++ = 0;
           }
           return;
       }
    }

    /* not a reserved name, restore file extension */
    if (ext) {
        *ext = '.';
    }
}

static void fix_win32_path( char *const path )
{
    for ( char *name = path; name; ) {
        char *nextsep = strchr( name, DIRSEP );
        if ( nextsep ) {
            *nextsep = '\0';
            fix_win32_name( name );
            *nextsep = DIRSEP;
            name = nextsep + 1;
        } else {
            fix_win32_name( name );
            break;
        }
    }
}
