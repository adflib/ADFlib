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

#include "unadf_fs.h"

#include "unadf_util.h"

#include <assert.h>
#include <sys/stat.h>
#include <time.h>

#ifdef WIN32
//#if !defined(__GNUC__)
#  if !defined(__MINGW32__) && !defined(_CYGWIN)
#    include <io.h>         // for open(), write(), ...
#    define PATH_MAX _MAX_PATH
#  endif
#  if defined _MSC_VER
#    include <direct.h>     // for _mkdir()
#  endif
#  define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#  include "unadf_win32.h"
#else
#  include <sys/time.h>
#  include <unistd.h>

#  ifndef O_BINARY
#  define O_BINARY 0
#  endif

#endif  // WIN32


/* set amiga file date on output file */
void set_file_date( const char *const             out,
                    const struct AdfEntry * const e )
{
    struct tm tm;
    tm.tm_sec   = e->secs;
    tm.tm_min   = e->mins;
    tm.tm_hour  = e->hour;
    tm.tm_mday  = e->days;
    tm.tm_mon   = e->month - 1;
    tm.tm_year  = e->year - 1900;
    tm.tm_isdst = -1;

    time_t time = mktime(&tm);

#ifdef WIN32
    /* on Windows, don't allow dates earlier than 1 Jan 1980 */
    if (time < 315532800) {
        time = 315532800;
    }

    struct utimbuf times;
    times.actime = times.modtime = time;

#  if !defined(_CYGWIN)
    // utime() does not work on directories on Windows,
    // need a custom implementation
    if (utimeWin32(out, &times) != 0) {
        print_error(out);
    }
#  else
    if (utime(out, &times) != 0) {
        print_error(out);
    }
#  endif

#else
    struct timeval times[2];
    times[0].tv_sec  = times[1].tv_sec = time;
    times[0].tv_usec = times[1].tv_usec = 0;
    if (utimes(out, times) != 0) {
        print_error(out);
    }
#endif
}

/* OS-agnostic make directory function */
static inline int makedir( const char *const path,
                           const mode_t      perms )
{
    if ( path == NULL )
        return -1;

#ifdef DEBUG_UNADF
    fprintf (stderr, "%s: path '%s', perms 0x%08x\n",
             __func__, path, (uint32_t) perms );
#endif

#if defined _MSC_VER
    (void) perms;
    return _mkdir(path);
#else
#if defined(__MINGW32__)
    (void) perms;
    return mkdir(path);
#else
    return mkdir(path, perms);
#endif
#endif
}

/* create directory tree, if needed
   return value: 0 - success, -1 - failure */
int make_dir_tree( const char * const dirpath,
                   const mode_t       perms )
{
    if ( dirpath == NULL )
        return -1;

#ifdef DEBUG_UNADF
    fprintf( stderr, "%s: '%s'\n", __func__, dirpath );
#endif

    if ( strlen( dirpath ) < 1 ||        // or: *dirpath == '\0' ||
         strlen( dirpath ) > PATH_MAX )
        return -1;

    char path[PATH_MAX + 1];
    strncpy( path, dirpath, PATH_MAX );
    path[ PATH_MAX ] = '\0';

    for ( char *p = path + 1; ; p++ ) {
        if ( *p == '\0' )
            return mkdir_if_needed( path, perms );

        if ( *p == DIRSEP ) {
                *p = 0;
                if ( mkdir_if_needed( path, perms ) != 0 )
                    return -1;
                *p = DIRSEP;
        }
    }

/*  (variant 2)
    char *p = path;
    for ( ; *p == DIRSEP; p++ );   // skip all leading separators

    while ( true ) {
        p = strchr( p, DIRSEP );

        if ( p == NULL )
            return mkdir_if_needed( path, perms );

        *p = '\0';
        int rc = mkdir_if_needed( path, perms );
        if ( rc != 0 )
            return rc;
        *p = DIRSEP;

        p++;
    }
*/
    assert( false );
    return -1;
}

/* creates directory, if it does not already exist */
int mkdir_if_needed( const char * const path,
                     const mode_t       perms )
{
#ifdef DEBUG_UNADF
    fprintf( stderr, "%s: '%s': ", __func__, path );
#endif
    struct stat st;
    if ( stat(path, &st) == 0 ) {
        if ( S_ISDIR(st.st_mode) ) {
#ifdef DEBUG_UNADF
            fprintf( stderr, "already exists.\n" );
#endif
            return 0;   // (directory already there - success)
        } else {
#ifdef DEBUG_UNADF
            fprintf( stderr, "something else with the same name - error!\n" );
#endif
            return -1;  // (something else there (???) - error!)
        }
    }

    if ( makedir(path, perms) != 0 ) {
        print_error(path);
        return -1;
    }
#ifdef DEBUG_UNADF
    else
        fprintf( stderr, "OK.\n" );
#endif
    return 0;
}

/* create directory path (if needed) leading to the basename
   (the last name in the given path).

   return value: 0 - success, -1 - failure */
int make_parent_dir_path( char * const path,
                          const mode_t perms )
{
#ifdef DEBUG_UNADF
    fprintf( stderr, "%s: '%s'\n", __func__, path );
#endif
    char *last_sep = strrchr( path, DIRSEP );
    int rc = 0;
    if ( last_sep ) {
        *last_sep = '\0';                 // temp. cut off leaf name
        rc = make_dir_tree( path, perms );
        *last_sep = DIRSEP;
    }

    return rc;
}

/* convert amiga permissions to unix permissions */
mode_t permissions( const struct AdfEntry * const e )
{
    return (mode_t)
        (!(e->access & 4) ? 0600 : 0400) | /* rw for user */
        (e->type == ADF_ST_DIR || !(e->access & 2) ? 0100 : 0) | /* x for user */
        ((e->access >> 13) & 0007) | /* rwx for others */
        ((e->access >> 5) & 0070); /* rwx for group */
}


#ifdef HAVE_CHMOD
#include <sys/stat.h>

void set_permissions( const char * const path,
                      const mode_t       perms )
{
    if ( chmod( path, perms ) != 0 )
        perror( path );
//    fprintf (stderr, "%s: real\n", __func__ );
}
#else
//#warning "No chmod() - unadf won't set permissions."
void set_permissions( const char * const path,
                      const mode_t       perms )
{
    (void) path, (void) perms;
//    fprintf (stderr, "%s: fake\n", __func__ );
}
#endif
