/*
 *  adfinfo_dir - directory metadata code
 *
 *  Copyright (C) 2023-2025 Tomasz Wolak
 *
 *  This file is part of adfinfo, an utility program showing low-level
 *  filesystem metadata of Amiga Disk Files (ADFs).
 *
 *  adfinfo is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  adfinfo is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Foobar; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "adfinfo_dir.h"

#include "adfinfo_common.h"
#include "adfinfo_dircache.h"

#include <adf_dir.h>
#include <stdio.h>
#include <string.h>


void show_directory_metadata( struct AdfVolume * const  vol,
                              const ADF_SECTNUM         dir_sector )
{
    struct AdfDirBlock //AdfEntryBlock
        dir_block;
    if ( adfReadEntryBlock( vol, dir_sector,
                            ( struct AdfEntryBlock * ) &dir_block ) != ADF_RC_OK )
    {
        fprintf( stderr, "Error reading directory entry block (%d)\n",
                 dir_sector );
        return;
    }

    uint8_t dirblock_orig_endian[ 512 ];
    memcpy( dirblock_orig_endian, &dir_block, 512 );
    adfSwapEndian( dirblock_orig_endian, ADF_SWBL_DIR );
    uint32_t checksum_calculated = adfNormalSum( dirblock_orig_endian, 0x14,
                                                 sizeof( struct AdfDirBlock ) );
    printf( "\nDirectory block:\n"
            //"  Offset  field\t\tvalue\n"
            "  0x000  type\t\t0x%x\t\t%u\n"
            "  0x004  headerKey\t0x%x\t\t%u\n"
            "  0x008  highSeq\t0x%x\t\t%u\n"
            "  0x00c  hashTableSize\t0x%x\t\t%u\n"
            "  0x010  r1\t\t0x%x\n"
            "  0x014  checkSum\t0x%x\n"
            "     ->  calculated:\t0x%x%s\n"
            "  0x018  hashTable[%u]:\t(see below)\n"
            "  0x138  r2[2]:\t\t(see below)\n"
            "  0x140  access\t\t0x%x\n"
            "  0x144  r4\t\t0x%x\n"
            "  0x148  commLen\t0x%x\t\t%u\n"
            "  0x149  comment [ %u ]:\t%s\n"
            "  0x199  r5[%u]:\t(see below)\n"
            "  0x1a4  days\t\t0x%x\t\t%u\n"
            "  0x1a8  mins\t\t0x%x\t\t%u\n"
            "  0x1ac  ticks\t\t0x%x\t\t%u\n"
            "  0x1b0  nameLen\t0x%x\t\t%u\n"
            "  0x1b1  dirName:\t%s\n"
            "  0x1d0  r6\t\t0x%x\n"
            "  0x1d4  real\t\t0x%x\t\t%u\n"
            "  0x1d8  nextLink\t0x%x\t\t%u\n"
            "  0x1dc  r7[5]:\t\t(see below)\n"
            "  0x1f0  nextSameHash\t0x%x\t\t%u\n"
            "  0x1f4  parent\t\t0x%x\t\t%u\n"
            "  0x1f8  extension\t0x%x\t\t%u\n"
            "  0x1fc  secType\t0x%x\t\t%d\n",
            dir_block.type, dir_block.type,
            dir_block.headerKey, dir_block.headerKey,
            dir_block.highSeq, dir_block.highSeq,
            dir_block.hashTableSize, dir_block.hashTableSize,
            dir_block.r1,
            dir_block.checkSum,
            checksum_calculated,
            dir_block.checkSum == checksum_calculated ? " -> OK" : " -> different(!)",
            ADF_HT_SIZE, //hashTable[ADF_HT_SIZE],
            //dir_block.r2[2],
            dir_block.access,
            dir_block.r4,
            dir_block.commLen, dir_block.commLen,
            ADF_MAX_COMMENT_LEN + 1, dir_block.comment,
            91 - ( ADF_MAX_COMMENT_LEN + 1 ), //dir_block.r5[ 91 - (ADF_MAX_COMMENT_LEN + 1)],
            dir_block.days, dir_block.days,
            dir_block.mins, dir_block.mins,
            dir_block.ticks, dir_block.ticks,
            dir_block.nameLen, dir_block.nameLen,
            dir_block.dirName,
            dir_block.r6,
            dir_block.real, dir_block.real,
            dir_block.nextLink, dir_block.nextLink,
            //dir_block.r7[5],
            dir_block.nextSameHash, dir_block.nextSameHash,
            dir_block.parent, dir_block.parent,
            dir_block.extension, dir_block.extension,
            dir_block.secType, dir_block.secType
        );

    show_hashtable( (const uint32_t * const) dir_block.hashTable );
    show_dircache_metadata( vol, dir_block.extension );
}

