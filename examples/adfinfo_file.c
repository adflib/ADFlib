/*
 *  adfinfo_file - file metadata code
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

#include "adfinfo_file.h"

#include <adf_file_util.h>
#include <stdio.h>
#include <string.h>


static void show_data_blocks_array(
    const int32_t datablocks[ ADF_MAX_DATABLK ] );


void show_file_metadata( struct AdfVolume * const  vol,
                         const ADF_SECTNUM         fheader_sector )
{
    struct AdfFileHeaderBlock fheader_block;
    if ( adfReadEntryBlock( vol, fheader_sector,
                            ( struct AdfEntryBlock * ) &fheader_block ) != ADF_RC_OK )
    {
        fprintf( stderr, "Error reading file header block (sector: %d).\n",
                 fheader_sector );
        return;
    }
    show_file_header_block( &fheader_block );
    show_file_ext_blocks( vol, &fheader_block );
    //struct File * file = adfOpenFile ( vol, entry_name, "r" );
}


void show_file_header_block( const struct AdfFileHeaderBlock * const  block )
{
    uint8_t file_header_block_orig_endian[ 512 ];
    memcpy( file_header_block_orig_endian, block, 512 );
    adfSwapEndian( file_header_block_orig_endian, ADF_SWBL_FILE );
    uint32_t checksum_calculated = adfNormalSum( file_header_block_orig_endian, 0x14,
                                                 sizeof (struct AdfFileHeaderBlock ) );
    printf( "\nFile Header Block:\n"
            "  0x000  type:\t\t0x%x\t\t%u\n"
            "  0x004  headerKey:\t0x%x\t\t%u\n"
            "  0x008  highSeq:\t0x%x\t\t%u\n"
            "  0x00c  dataSize:\t0x%x\t\t%u\n"
            "  0x010  firstData:\t0x%x\t\t%u\n"
            "  0x014  checkSum:\t0x%x\n"
            "     ->  calculated:\t0x%x%s\n"
            "  0x018  dataBlocks [ %u ]: (see below)\n"
            "  0x138  r1:\t\t%d\n"
            "  0x13c  r2:\t\t%d\n"
            "  0x140  access:\t0x%x\t\t%u\n"
            "  0x144  byteSize:\t0x%x\t\t%u\n"
            "  0x148  commLen:\t0x%x\t\t%u\n"
            "  0x149  comment[ %u ]:\t%s\n"
            "  0x199  r3 [ %u ]:\t\t%s\n"
            "  0x1a4  days:\t\t%d\n"
            "  0x1a8  mins:\t\t%d\n"
            "  0x1ac  ticks:\t\t%d\n"
            "  0x1b0  nameLen:\t0x%x\t\t%u\n"
            "  0x1b1  fileName:\t%s\n"
            "  0x1d0  r4:\t\t%d\n"
            "  0x1d4  real:\t\t0x%x\t\t%u\n"
            "  0x1d8  nextLink:\t0x%x\t\t%u\n"
            "  0x1dc  r5 [ 5 ]:\t(see below)\n"
            "  0x1f0  nextSameHash:\t0x%x\t\t%u\n"
            "  0x1f4  parent:\t0x%x\t\t%u\n"
            "  0x1f8  extension:\t0x%x\t\t%u\n"
            "  0x1fc  secType:\t0x%x\t%d\n",
            block->type, block->type,
            block->headerKey, block->headerKey,
            block->highSeq, block->highSeq,
            block->dataSize, block->dataSize,
            block->firstData, block->firstData,
            block->checkSum,
            checksum_calculated,
            block->checkSum == checksum_calculated ? " -> OK" : " -> different(!)",
            ADF_MAX_DATABLK, //dataBlocks[ADF_MAX_DATABLK],
            block->r1,
            block->r2,
            block->access, block->access,
            block->byteSize, block->byteSize,
            block->commLen, block->commLen,
            ADF_MAX_COMMENT_LEN + 1, block->comment,
            91 - ( ADF_MAX_COMMENT_LEN + 1 ), block->r3,
            block->days,
            block->mins,
            block->ticks,
            block->nameLen, block->nameLen,
            block->fileName,
            block->r4,
            block->real, block->real,
            block->nextLink, block->nextLink,
            //block->r5[],
            block->nextSameHash, block->nextSameHash,
            block->parent, block->parent,
            block->extension, block->extension,
            block->secType, block->secType );

    show_data_blocks_array( block->dataBlocks );
    /*puts ( "\ndatablocks (non-zero):" );
    for ( unsigned i = 0 ; i < ADF_MAX_DATABLK ; ++i ) {
        uint32_t dblock_sector = block->dataBlocks[i];
        if ( dblock_sector )
            printf ( "  dataBlocks[ %d ]:  0x%x (%d)\n",
                     i, dblock_sector, dblock_sector );
                     } */

    printf( "\n  r3 (non-zero):" );
    for ( unsigned i = 0 ; i < 91 - ( ADF_MAX_COMMENT_LEN + 1 ) ; ++i ) {
        char r3_i = block->r3[ i ];
        if ( r3_i )
            printf( "\n    r3[ %d ]:  0x%x (%d)",
                    i, r3_i, r3_i );
    }
    
    printf( "\n  r5 (non-zero):\n" );
    for ( unsigned i = 0 ; i < 5 ; ++i ) {
        int32_t r5_i = block->r5[ i ];
        if ( r5_i )
            printf( "    r5[ %d ]:  0x%x (%d)\n",
                    i, r5_i, r5_i );
    }
}


void show_file_ext_blocks( struct AdfVolume * const                 vol,
                           const struct AdfFileHeaderBlock * const  fheader_block )
{
    // check if there are any ext blocks
    uint32_t posInExtBlk, posInDataBlk, curDataN;
    if ( adfFilePos2DataBlock( fheader_block->byteSize,
                               vol->datablockSize,
                               &posInExtBlk, &posInDataBlk, &curDataN ) == -1 )
    {
        printf( "No ext blocks.\n" );
        return;
    }
    
    struct AdfFileExtBlock extblock;
    extblock.extension = fheader_block->extension; // copy initial extension pointer (sector)
    while ( extblock.extension ) {
        int32_t extblock_sector = extblock.extension;
        if ( adfReadFileExtBlock( vol, extblock_sector, &extblock ) != ADF_RC_OK ) {
            fprintf( stderr, "Error reading ext block from sector 0x%x (%u).\n",
                     extblock_sector, extblock_sector );
            return;
        }
        show_ext_block( &extblock );
    }
}


void show_ext_block( const struct AdfFileExtBlock * const block )
{
    uint8_t file_ext_block_orig_endian[ 512 ];
    memcpy( file_ext_block_orig_endian, block, 512 );
    adfSwapEndian( file_ext_block_orig_endian, ADF_SWBL_FILE );
    uint32_t checksum_calculated = adfNormalSum( file_ext_block_orig_endian, 0x14,
                                                 sizeof (struct AdfFileExtBlock ) );
    printf( "\nFile extension block:\n"
            "  0x000  type:\t\t0x%x\t\t%u\n"
            "  0x004  headerKey:\t0x%x\t\t%u\n"
            "  0x008  highSeq:\t0x%x\t\t%u\n"
            "  0x00c  dataSize:\t0x%x\t\t%u\n"
            "  0x010  firstData:\t0x%x\t\t%u\n"
            "  0x014  checkSum:\t0x%x\n"
            "     ->  calculated:\t0x%x%s\n"
            "  0x018  dataBlocks [ %u ]:\t(see below)\n"
            "  0x138  r[45]:\t\t\t(not used)\n"
            "  0x1ec  info:\t\t0x%x\t\t%u\n"
            "  0x1f0  nextSameHash:\t0x%x\t\t%u\n"
            "  0x1f4  parent:\t0x%x\t\t%u\n"
            "  0x1f8  extension:\t0x%x\t\t%u\n"
            "  0x1fc  secType:\t0x%x\t%d\n",
            block->type, block->type,
            block->headerKey, block->headerKey,
            block->highSeq, block->highSeq,
            block->dataSize, block->dataSize,
            block->firstData, block->firstData,
            block->checkSum,
            checksum_calculated,
            block->checkSum == checksum_calculated ? " -> OK" : " -> different(!)",
            ADF_MAX_DATABLK, //block->dataBlocks[ADF_MAX_DATABLK],
            //r[45]
            block->info, block->info,
            block->nextSameHash, block->nextSameHash,
            block->parent, block->parent,
            block->extension, block->extension,
            block->secType, block->secType );
    
    show_data_blocks_array( block->dataBlocks );
}

static void show_data_blocks_array(
    const int32_t datablocks[ ADF_MAX_DATABLK ] )
{
    printf ( "\n  data blocks (non-zero):\n" );
    for ( unsigned i = 0 ; i < ADF_MAX_DATABLK ; ++i ) {
        int32_t dblock_i = datablocks[ i ];
        if ( dblock_i )
            printf( "    dataBlocks [ %2d ]:  0x%x\t%d\n",
                    i, dblock_i, dblock_i );
    }
}


/*
static void show_File ( const struct File * const file )
{
    printf ( "\nstruct File:\n"
             //"volume:\t0x%x
             //fileHdr;
             //currentData;
             //struct AdfFileExtBlock* currentExt;
             "  nDataBlock:\t0x%x\t\t%u\n"
             "  curDataPtr:\t0x%x\t\t%u\n"
             "  pos:\t\t0x%x\t\t%u\n"
             "  posInDataBlk:\t0x%x\t\t%u\n"
             "  posInExtBlk:\t0x%x\t\t%u\n"
             "  eof:\t\t0x%x\t\t%u\n"
             "  writeMode:\t0x%x\t\t%u\n",
             //volume;
             //fileHdr;
             //currentData;
             //struct AdfFileExtBlock* currentExt;
             file->nDataBlock,
             file->nDataBlock,
             file->curDataPtr,
             file->curDataPtr,
             file->pos,
             file->pos,
             file->posInDataBlk,
             file->posInDataBlk,
             file->posInExtBlk,
             file->posInExtBlk,
             file->eof,
             file->eof,
             file->writeMode,
             file->writeMode );
}
*/
