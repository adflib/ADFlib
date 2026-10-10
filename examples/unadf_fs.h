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

#pragma once

#include "adflib.h"

#ifdef WIN32

#  include <windows.h>

 //#if !defined(__GNUC__)
#  if !defined(__MINGW32__) && !defined(_CYGWIN)
//#    include <io.h>         // for open(), write(), ...
     typedef uint32_t mode_t;
#  endif

#else
#  include <sys/types.h>
#endif

void set_file_date( const char *const            out,
                    const struct AdfEntry *const e );

void mkdir_if_needed( const char *const path,
                      const mode_t      perms );

mode_t permissions( const struct AdfEntry * const e );

void set_permissions( const char * const path,
                      const mode_t       perms );
