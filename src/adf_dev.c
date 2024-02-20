/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_dev.c
 *
 *  $Id$
 *
 *  device code
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
 *  along with Foobar; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "adf_dev.h"

#include "adf_dev_drivers.h"
#include "adf_dev_flop.h"
#include "adf_dev_hd.h"
#include "adf_env.h"

#include <stdlib.h>
#include <string.h>


static RETCODE adfDevSetCalculatedGeometry_ ( struct AdfDevice * const dev );
static BOOL adfDevIsGeometryValid_ ( const struct AdfDevice * const dev );


struct AdfDevice * adfDevCreate ( const char * const driverName,
                                  const char * const name,
                                  const uint32_t     cylinders,
                                  const uint32_t     heads,
                                  const uint32_t     sectors )
{
    const struct AdfDeviceDriver * const driver = adfGetDeviceDriverByName ( driverName );
    if ( driver == NULL || driver->createDev == NULL )
        return NULL;
    return driver->createDev ( name, cylinders, heads, sectors );
}


static struct AdfDevice *
    adfDevOpenWithDrv_ ( const struct AdfDeviceDriver * const driver,
                         const char * const  name,
                         const AdfAccessMode mode )
{
    if ( driver == NULL || driver->openDev == NULL )
        return NULL;

    struct AdfDevice * const dev = driver->openDev ( name, mode );
    if ( dev == NULL )
        return NULL;

    dev->devType = adfDevType ( dev );

    if ( ! dev->drv->isNative() ) {
        if ( adfDevSetCalculatedGeometry_ ( dev ) != RC_OK ) {
            dev->drv->closeDev ( dev );
            return NULL;
        }
    }

    if ( ! adfDevIsGeometryValid_ ( dev ) ) {
        adfEnv.eFct ( "adfDevOpen : invalid geometry: cyliders %u, "
                      "heads: %u, sectors: %u, size: %u, device: %s",
                      dev->cylinders, dev->heads, dev->sectors, dev->size, dev->name );
        dev->drv->closeDev ( dev );
        return NULL;
    }

    if ( dev->devType != DEVTYPE_HARDDISK )
        return dev;

    struct bRDSKblock rdsk;
    RETCODE rc = adfReadRDSKblock ( dev, &rdsk );
    if ( rc == RC_OK ) {
        /* rigid block exists -> check geometry */
        //if ( ! adfDevIsRDSKGeometryValid_ ( dev, &rdsk ) ) {
        if  ( dev->cylinders != rdsk.cylinders ||
              dev->heads     != rdsk.heads     ||
              dev->sectors   != rdsk.sectors )
        {
            adfEnv.wFct (
                "adfDevOpen : using geometry from Rigid Block, "
                "different than detected/calculated(!):\n"
                "              detected             rdsk block\n"
                " cyliders:      %8u                  %8u\n"
                " heads:         %8u                  %8u\n"
                " sectors:       %8u                  %8u\n"
                " size:        %10llu               %10llu",
                dev->cylinders, rdsk.cylinders,
                dev->heads,     rdsk.heads,
                dev->sectors,   rdsk.sectors,
                dev->size,
                (long long unsigned) rdsk.cylinders *
                (long long unsigned) rdsk.heads *
                (long long unsigned) rdsk.sectors * 512LLU );
            dev->cylinders = rdsk.cylinders;
            dev->heads     = rdsk.heads;
            dev->sectors   = rdsk.sectors;
            if ( ! adfDevIsGeometryValid_ ( dev ) ) {
                adfEnv.eFct ( "adfDevOpen : invalid geometry: cyliders %u, "
                              "heads: %u, sectors: %u, size: %u, device: %s",
                              dev->cylinders, dev->heads, dev->sectors, dev->size, dev->name );
                dev->drv->closeDev ( dev );
                return NULL;
            }
        }
    }

    return dev;
}


struct AdfDevice * adfDevOpen ( const char * const  name,
                                const AdfAccessMode mode )
{
    return adfDevOpenWithDrv_ ( adfGetDeviceDriverByDevName ( name ), name, mode );
}


struct AdfDevice * adfDevOpenWithDriver ( const char * const  driverName,
                                          const char * const  name,
                                          const AdfAccessMode mode )
{
    return adfDevOpenWithDrv_ ( adfGetDeviceDriverByName ( driverName ), name, mode );
}



/*
 * adfDevClose
 *
 * Closes/releases an opened device.
 */
void adfDevClose ( struct AdfDevice * const dev )
{
    if ( dev == NULL )
        return;

    if ( dev->mounted )
        adfDevUnMount ( dev );

    dev->drv->closeDev ( dev );
}


/*
 * adfDevType
 *
 * returns the type of a device
 * only based of the field 'dev->size'
 */
int adfDevType ( const struct AdfDevice * const dev )
{
    if( (dev->size==512*11*2*80) ||		/* BV */
        (dev->size==512*11*2*81) ||		/* BV */
        (dev->size==512*11*2*82) || 	/* BV */
        (dev->size==512*11*2*83) )		/* BV */
        return(DEVTYPE_FLOPDD);
    else if (dev->size==512*22*2*80)
        return(DEVTYPE_FLOPHD);
    else if (dev->size>512*22*2*80)
        return(DEVTYPE_HARDDISK);
    else {
        (*adfEnv.eFct)("adfDevType : unknown device type");
        return(-1);
    }
}


/*
 * adfDevInfo
 *
 * display information about the device and its volumes
 * for demonstration purpose only since the output is stdout !
 *
 * can be used before adfVolCreate() or adfVolMount()
 */
void adfDevInfo ( const struct AdfDevice * const dev )
{
    const char * devTypeInfo = NULL;
    switch ( dev->devType ) {
    case DEVTYPE_FLOPDD:
        devTypeInfo = "floppy dd";
        break;
    case DEVTYPE_FLOPHD:
        devTypeInfo = "floppy hd";
        break;
    case DEVTYPE_HARDDISK:
        devTypeInfo = "harddisk";
        break;
    case DEVTYPE_HARDFILE:
        devTypeInfo = "hardfile";
        break;
    default:
        devTypeInfo = "unknown device type!";
    }

    printf ( "\nADF device info:\n  Type:\t\t%s\n  Driver:\t%s\n",
             devTypeInfo, dev->drv->name );

    printf ( "  Geometry:\n"
             "    Cylinders\t%d\n"
             "    Heads\t%d\n"
             "    Sectors\t%d\n\n",
             dev->cylinders, dev->heads, dev->sectors );

    printf ( "  Volumes (%d):\n"
             "   idx  first bl.     last bl.    name\n",
             dev->nVol );

    for ( int i = 0 ; i < dev->nVol ; i++ ) {
        printf ( "    %2d  %9d    %9d    \"%s\"", i,
                 dev->volList[i]->firstBlock,
                 dev->volList[i]->lastBlock,
                 dev->volList[i]->volName ? dev->volList[i]->volName : "" );
        if ( dev->volList[i]->mounted )
            printf("    mounted");
        putchar('\n');
    }
    putchar('\n');
}


/*
 * adfDevMount
 *
 * mount a dump file (.adf) or a real device (uses adf_nativ.c and .h)
 *
 * adfInitDevice() must fill dev->size !
 */
RETCODE adfDevMount ( struct AdfDevice * const dev )
{
    if ( dev == NULL )
        return RC_ERROR;

    RETCODE rc;

    switch( dev->devType ) {

    case DEVTYPE_FLOPDD:
    case DEVTYPE_FLOPHD: {
        rc = adfMountFlop ( dev );
        if ( rc != RC_OK )
            return rc;
        }
        break;

    case DEVTYPE_HARDDISK: {
        uint8_t buf[512];
        rc = adfDevReadBlock ( dev, 0, 512, buf );

       /* BV ...from here*/
        if( rc != RC_OK ) {
            adfEnv.eFct ( "adfMountDev : adfReadBlockDev 0 (bootblock) failed");
            return rc;
        }

        /* a file with the first three bytes equal to 'DOS' */
        if ( ! dev->drv->isNative() &&
             strncmp ( "DOS", (char *) buf, 3 ) == 0 )
        {
            rc = adfMountHdFile ( dev );
            if ( rc != RC_OK )
                return rc;
        }
        else {
            rc = adfMountHd ( dev );
            if ( rc != RC_OK )
                return rc;								/* BV ...to here.*/
            }
        }
        break;

    default:
        (*adfEnv.eFct)("adfMountDev : unknown device type");
        return RC_ERROR;								/* BV */
    }

    dev->mounted = TRUE;
    return RC_OK;
}


/*
 * adfDevUnMount
 *
 */
void adfDevUnMount ( struct AdfDevice * const dev )
{
    if ( ! dev->mounted )
        return;

    // free volume list
    //if ( dev->volList ) {
    if ( dev->nVol > 0 ) {
        for ( int i = 0 ; i < dev->nVol ; i++ ) {
            free ( dev->volList[i]->volName );
            free ( dev->volList[i] );
        }
        free ( dev->volList );
        dev->nVol = 0;
    }

    dev->volList = NULL;
    dev->mounted = FALSE;
}


RETCODE adfDevReadBlock ( struct AdfDevice * const dev,
                          const uint32_t           pSect,
                          const uint32_t           size,
                          uint8_t * const          buf )
{
/*  printf("pSect R =%ld\n",pSect);
    RETCODE rc = dev->drv->readSector ( dev, pSect, size, buf );
    printf("rc=%ld\n",rc);
    return rc;
*/
    return dev->drv->readSector ( dev, pSect, size, buf );
}


RETCODE adfDevWriteBlock ( struct AdfDevice * const dev,
                           const uint32_t           pSect,
                           const uint32_t           size,
                           const uint8_t * const    buf )
{
/*printf("nativ=%d\n",dev->isNativeDev);*/
    return dev->drv->writeSector ( dev, pSect, size, buf );
}


static RETCODE adfDevSetCalculatedGeometry_ ( struct AdfDevice * const dev )
{
    /* set geometry (based on already set size) */
    switch ( dev->devType ) {
    case DEVTYPE_FLOPDD:
        dev->heads     = 2;
        dev->sectors   = 11;
        dev->cylinders = dev->size / ( dev->heads * dev->sectors * 512 );
        if ( dev->cylinders < 80 || dev->cylinders > 83 ) {
            adfEnv.eFct ( "adfDevSetCalculatedGeometry_: invalid size %u", dev->size );
            return RC_ERROR;
        }
        break;

    case DEVTYPE_FLOPHD:
        dev->heads     = 2;
        dev->sectors   = 22;
        dev->cylinders = dev->size / ( dev->heads * dev->sectors * 512 );
        if ( dev->cylinders != 80 ) {
            adfEnv.eFct ( "adfDevSetCalculatedGeometry_: invalid size %u", dev->size );
            return RC_ERROR;
        }
        break;

    case DEVTYPE_HARDDISK:
    case DEVTYPE_HARDFILE:
        //dev->heads = 2;
        //dev->sectors   = dev->size / 4;
        //dev->cylinders = dev->size / ( dev->sectors * dev->heads * 512 );
        dev->heads     = 1;
        dev->sectors   = 1;
        dev->cylinders = dev->size / 512;
        break;

    default:
        adfEnv.eFct ( "adfDevSetCalculatedGeometry_: invalid dev type %d", dev->devType );
        return RC_ERROR;
    }
    return RC_OK;
}


static BOOL adfDevIsGeometryValid_ ( const struct AdfDevice * const dev )
{
    return ( dev->cylinders > 0 &&
             dev->heads > 0    &&
             dev->sectors > 0  &&
             dev->cylinders * dev->heads * dev->sectors * 512 == dev->size );
}
