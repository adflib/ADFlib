/*
 *  pathutils - common path utilities
 *
 *  Copyright (C) 2023-2025 Tomasz Wolak
 *
 *  This file is part of ADFLib.
 *
 *  ADFlib is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  ADFlib is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Foobar; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#pragma once

#if defined(_WIN32) && !defined(_CYGWIN)
char * dirname( char *  path );
char * basename( char *  path );
#else
#include <libgen.h>
#endif
