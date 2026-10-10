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

#include "unadf_util.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG_UNADF
void adfPrintBacktrace(void);
#endif

void print_error(const char *s)
{
#ifdef DEBUG_UNADF
    adfPrintBacktrace();
#endif

    perror(s);
}

#ifdef DEBUG_UNADF
#include <execinfo.h>    // for backtrace()

void adfPrintBacktrace(void)
{
    const unsigned BUFSIZE = 100;
    void *buffer[ BUFSIZE ];

    int size = backtrace( buffer, (int) BUFSIZE );
    const char * const * const strings =
        (const char * const * const) backtrace_symbols( buffer, size );

    if ( strings == NULL ) {
        perror( "error getting symbols" );
        return;
    }

    fprintf( stderr, "Obtained %d stack frames.\n", size );
    for ( int i = 0; i < size; i++ )
        fprintf( stderr, "%s\n", strings[i] );

    free( (void *) strings );
}
#endif
