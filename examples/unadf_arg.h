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

#include <stdbool.h>

/* command-line arguments */
extern bool
    list_mode,
    list_all,
    use_dircache,
    show_sectors,
    show_comments,
    pipe_mode,
    win32_mangle;

extern char
    *adf_file,
    *extract_dir;

extern int vol_number;

extern struct AdfList *file_list;


void parse_args(int argc, char *argv[]);
void help(FILE * const stream);
