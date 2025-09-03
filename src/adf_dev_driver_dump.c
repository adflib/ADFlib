/*
 *  adf_dev_driver_dump.c - dump file device driver
 *
 *  Copyright (C) 1997-2022 Laurent Clevy
 *                2023-2025 Tomasz Wolak
 *
 *  This file is part of ADFLib.
 *
 *  ADFLib is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  ADFLib is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with ADFLib; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "adf_dev_driver_dump.h"

#include "adf_blk.h"
#include "adf_dev_type.h"
#include "adf_env.h"
#include "adf_err.h"
#include "adf_limits.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct DevDumpData {
    FILE * fd;
};


/*
 * adfCreateDumpDevice
 *
 * returns NULL if failed
 */ 
static struct AdfDevice * adfCreateDumpDevice( const char * const  filename,
                                               const uint32_t      cylinders,
                                               const uint32_t      heads,
                                               const uint32_t      sectors )
{
    uint8_t buf[ ADF_LOGICAL_BLOCK_SIZE ];
/*    int32_t i;*/
    int r;
	
    struct AdfDevice * dev = (struct AdfDevice *)
        malloc( sizeof(struct AdfDevice) );
    if ( ! dev ) {
        adfEnv.eFct("%s: malloc dev", __func__ );
        return NULL;
    }

    dev->drvData = malloc( sizeof(struct DevDumpData) );
    if ( dev->drvData == NULL ) {
        adfEnv.eFct( "%s: malloc data error", __func__ );
        free( dev );
        return NULL;
    }

    FILE ** fd = &( ( (struct DevDumpData *) dev->drvData )->fd );

    *fd = fopen( filename, "wb" );
    if ( *fd == NULL ) {
        free( dev->drvData );
        free( dev );
        adfEnv.eFct("%s: fopen", __func__ );
        return NULL;
    }

/*    for(i=0; i<cylinders*heads*sectors; i++)
        fwrite(buf, sizeof(uint8_t), 512 , nDev->fd);
*/
    long lastBlockOffset = ( ( cylinders * heads * sectors ) - 1 ) *
        ADF_LOGICAL_BLOCK_SIZE;
    r = fseek( *fd, lastBlockOffset, SEEK_SET );
    if ( r == -1 ) {
        fclose( *fd );
        free( dev->drvData );
        free( dev );
        adfEnv.eFct("%s: fseek", __func__ );
        return NULL;
    }

    memset( buf, 0, ADF_LOGICAL_BLOCK_SIZE );
    size_t blocksWritten = fwrite( buf, ADF_LOGICAL_BLOCK_SIZE, 1, *fd );
    if ( blocksWritten != 1 ) {
        fclose( *fd );
        free( dev->drvData );
        free( dev );
        adfEnv.eFct("%s: fwrite", __func__ );
        return NULL;
    }

    fclose( *fd );

    *fd = fopen( filename, "rb+" );
    if ( ! *fd ) {
        free( dev->drvData );
        free( dev );
        adfEnv.eFct("%s: fopen", __func__ );
        return NULL;
    }
    dev->geometry.cylinders = cylinders;
    dev->geometry.heads     = heads;
    dev->geometry.sectors   = sectors;
    dev->geometry.blockSize = ADF_DEV_BLOCK_SIZE;
    dev->sizeBlocks         = cylinders * heads * sectors;

    dev->type  = adfDevGetTypeByGeometry( &dev->geometry );
    dev->class = ( dev->type != ADF_DEVTYPE_UNKNOWN ) ?
        adfDevTypeGetClass( dev->type ) :
        adfDevGetClassBySizeBlocks( dev->sizeBlocks );

    dev->nVol     = 0;
    dev->readOnly = false;
    dev->mounted  = false;
    dev->drv      = &adfDeviceDriverDump;
    dev->name     = strdup( filename );

    return dev;
}


/*
 * adfDevDumpOpen
 *
 */
static struct AdfDevice * adfDevDumpOpen( const char * const   name,
                                          const AdfAccessMode  mode )
{
    struct AdfDevice * const  dev = ( struct AdfDevice * )
        malloc( sizeof ( struct AdfDevice ) );
    if ( dev == NULL ) {
        adfEnv.eFct( "%s: malloc error", __func__ );
        return NULL;
    }

    dev->readOnly = ( mode != ADF_ACCESS_MODE_READWRITE );

    dev->drvData = malloc( sizeof ( struct DevDumpData ) );
    if ( dev->drvData == NULL ) {
        adfEnv.eFct("%s: malloc data error", __func__ );
        free( dev );
        return NULL;
    }

    FILE ** const fd = &( ( (struct DevDumpData *) dev->drvData )->fd );
    *fd = fopen( name, dev->readOnly ? "rb" : "rb+" );
    if ( *fd == NULL ) {
        adfEnv.eFct( "%s: fopen", __func__ );
        free( dev->drvData );
        free( dev );
        return NULL;
    }

    dev->geometry.blockSize = ADF_DEV_BLOCK_SIZE;

    /* determines size */
    fseek( *fd, 0, SEEK_END );
    dev->sizeBlocks = (uint32_t)( ftell( *fd ) / dev->geometry.blockSize );
    fseek( *fd, 0, SEEK_SET );

    dev->class = adfDevGetClassBySizeBlocks( dev->sizeBlocks );
    dev->type  = ADF_DEVTYPE_UNKNOWN; // geometry unknown
    dev->nVol    = 0;
    dev->volList = NULL;
    dev->mounted = false;
    dev->drv     = &adfDeviceDriverDump;
    dev->name    = strdup( name );

    return dev;
}

/*
 * adfReleaseDumpDevice
 *
 */
static ADF_RETCODE adfReleaseDumpDevice( struct AdfDevice * const  dev )
{
    fclose( ( (struct DevDumpData *) dev->drvData )->fd );

    if ( dev->mounted )
        adfDevUnMount( dev );

    free( dev->drvData );
    free( dev->name );
    free( dev );

    return ADF_RC_OK;
}


/*
 * adfReadDumpSectors
 *
 */
static ADF_RETCODE adfReadDumpSectors( const struct AdfDevice * const  dev,
                                       const uint32_t                  block,
                                       const uint32_t                  lenBlocks,
                                       uint8_t * const                 buf )
{
    if ( block + lenBlocks > dev->sizeBlocks )
        return ADF_RC_ERROR;

    FILE * const fd = ( (struct DevDumpData *) dev->drvData )->fd;
    int pos = fseek( fd, dev->geometry.blockSize * block, SEEK_SET );
    if ( pos == -1 )
        return ADF_RC_ERROR;

    if ( fread( buf, dev->geometry.blockSize, lenBlocks, fd ) != lenBlocks )
        return ADF_RC_ERROR;

    return ADF_RC_OK;
}


/*
 * adfWriteDumpSectors
 *
 */
static ADF_RETCODE adfWriteDumpSectors( const struct AdfDevice * const  dev,
                                        const uint32_t                  block,
                                        const uint32_t                  lenBlocks,
                                        const uint8_t * const           buf )
{
    if ( block + lenBlocks > dev->sizeBlocks )
        return ADF_RC_ERROR;

    FILE * const fd = ( (struct DevDumpData *) dev->drvData )->fd;
    int r = fseek( fd, dev->geometry.blockSize * block, SEEK_SET );
    if ( r == -1 )
        return ADF_RC_ERROR;

    if ( fwrite( buf, dev->geometry.blockSize, lenBlocks, fd ) != lenBlocks )
        return ADF_RC_ERROR;

    return ADF_RC_OK;
}


static bool adfDevDumpIsNativeDevice( void )
{
    return false;
}

const struct AdfDeviceDriver adfDeviceDriverDump = {
    .name         = "dump",
    .data         = NULL,
    .createDev    = adfCreateDumpDevice,
    .openDev      = adfDevDumpOpen,       // adfOpenDev + adfInitDumpDevice
    .closeDev     = adfReleaseDumpDevice,
    .readSectors  = adfReadDumpSectors,
    .writeSectors = adfWriteDumpSectors,
    .isNative     = adfDevDumpIsNativeDevice,
    .isDevice     = NULL
};

/*##################################################################################*/
