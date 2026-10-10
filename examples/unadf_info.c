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

#include "unadf_info.h"

#include "unadf_arg.h"
#include "unadf_pathutil.h"

#include <stdio.h>
#include <stdlib.h>

/* prints one line of information about a device */
void print_device( const struct AdfDevice * const dev )
{
    printf("Device : %s. Cylinders = %d, Heads = %d, Sectors = %d. Volumes = %d\n",
        dev->type != ADF_DEVTYPE_UNKNOWN    ? adfDevTypeGetDescription( dev->type ) :
        dev->class == ADF_DEVCLASS_HARDDISK ? "Harddisk"  :
        dev->class == ADF_DEVCLASS_HARDFILE ? "Hardfile"  : "???",
        dev->geometry.cylinders, dev->geometry.heads, dev->geometry.sectors, dev->nVol);
}

/* prints one line of information about a volume */
void print_volume( const struct AdfVolume * const vol )
{
    ADF_SECTNUM num_blocks = vol->lastBlock - vol->firstBlock + 1;

    switch (vol->dev->class) {
    case ADF_DEVCLASS_FLOP:
        printf("Volume : %s,", adfDevTypeGetDescription( vol->dev->type ) );
        break;
    case ADF_DEVCLASS_HARDDISK:
        printf("Volume : HD partition #%d %3.1f KBytes,", args.vol_number, num_blocks / 2.0);
        break;
    case ADF_DEVCLASS_HARDFILE:
        printf("Volume : HardFile %3.1f KBytes,", num_blocks / 2.0);
        break;
    default:
        printf("???,");
    }

    if (vol->volName) {
        printf(" \"%s\"", vol->volName);
    }

    printf(" between sectors [%d-%d]. %s%s%s. Filled at %2.1f%%.\n",
        vol->firstBlock,
        vol->lastBlock,
        adfVolIsFFS(vol) ? "FFS" : "OFS",
        adfVolHasINTL(vol) ? " INTL" : "",
        adfVolHasDIRCACHE(vol) ? " DIRCACHE" : "",
        100.0 - ((adfCountFreeBlocks(vol) * 100.0) / num_blocks));
}

/* recurses through all directories/files and calls print_entry() on them */
void print_tree( const struct AdfList * node,
                 const char * const     path )
{
    for (; node; node = node->next) {
        print_entry(node->content, path);
        if (node->subdir) {
            struct AdfEntry *e = (struct AdfEntry *) node->content;
            char *newpath = join_path(path, e->name);
            print_tree(node->subdir, newpath);
            free(newpath);
        }
    }
}

/* prints one line of information about a directory/file entry */
void print_entry( const struct AdfEntry * const e,
                  const char * const            path )
{
    bool is_dir = e->type == ADF_ST_DIR;
    bool print_comment = args.show_comments && e->comment && *e->comment;

    /* do not print the links entries, ADFlib do not support them yet properly */
    if (e->type == ADF_ST_LFILE || e->type == ADF_ST_LDIR || e->type == ADF_ST_LSOFT) {
        return;
    }

    /* print "size date time [ sector ] path/filename [, comment]" */
    printf(is_dir ? "       " : "%7d", e->size);
    printf("  %4d/%02d/%02d  %2d:%02d:%02d ",
        e->year, e->month, e->days, e->hour, e->mins, e->secs);
    if ( args.show_sectors ) {
        printf(" %06d ", e->sector);
    }
    printf(" %s%s%s%s%s%s\n",
        path,
        *path ? "/" : "",
        e->name,
        is_dir ? "/" : "",
        print_comment ? ", " : "",
        print_comment ? e->comment : "");
}
