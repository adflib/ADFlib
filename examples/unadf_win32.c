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

#include "unadf_win32.h"

#if defined(WIN32) && !defined(_CYGWIN)

// utime() does not work on directories on Windows,
// need a custom implementation

#include <time.h>
#include <windows.h>

static FILETIME getFileTime( time_t time );

int utimeWin32( const char * const            pathname,
                const struct utimbuf * const  times )
{
    if ( times == NULL )
        return -1;

    const HANDLE handle = CreateFileA(
        pathname, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
        OPEN_EXISTING,
        FILE_WRITE_ATTRIBUTES |
        FILE_FLAG_BACKUP_SEMANTICS, NULL );

    if ( handle == INVALID_HANDLE_VALUE )
        return -1;

    int status = 0;

    FILETIME timeAccess = getFileTime( times->actime );
    FILETIME timeWrite  = getFileTime( times->modtime );

    if ( ( timeAccess.dwHighDateTime == 0 &&
           timeAccess.dwLowDateTime == 0 ) ||
         ( timeWrite.dwHighDateTime == 0 &&
           timeWrite.dwLowDateTime == 0 ) )
    {
        status = -1;
        goto win32_utime_clean_up;
    }

    //if ( ! SetFileTime( handle, &timeWrite, &timeAccess, &timeWrite ) ) {
    if ( ! SetFileTime( handle, NULL, &timeAccess, &timeWrite ) ) {
        //fprintf( stderr,"%s: SetFileTime failed.\n", __func__ );
        status = -1;
        goto win32_utime_clean_up;
    }

    //fprintf( stderr,"%s: SetFileTime OK.\n", __func__ );

win32_utime_clean_up:
    CloseHandle( handle );
    return status;
}


FILETIME getFileTime( time_t time )
{
    FILETIME fileTime = {
        .dwHighDateTime = 0,
        .dwLowDateTime  = 0
    };

    struct tm tm;
    if ( gmtime_s( &tm, &time ) != 0 )
        return fileTime;

    SYSTEMTIME sysTime = {
        .wYear         = (short unsigned) tm.tm_year + 1900,
        .wMonth        = (short unsigned) tm.tm_mon + 1,
        //WORD .wDayOfWeek;
        .wDay          = (short unsigned) tm.tm_mday,
        .wHour         = (short unsigned) tm.tm_hour,
        .wMinute       = (short unsigned) tm.tm_min,
        .wSecond       = (short unsigned) tm.tm_sec,
        .wMilliseconds = 0
    };

    if ( ! SystemTimeToFileTime( &sysTime, &fileTime ) )
        fileTime.dwHighDateTime =
        fileTime.dwLowDateTime  = 0;

    return fileTime;
}

#endif
