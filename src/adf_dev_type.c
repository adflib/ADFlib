
#include "adf_dev_type.h"

#include <assert.h>
#include <stddef.h>

// https://en.wikipedia.org/wiki/List_of_floppy_disk_formats
static struct AdfDevMedium {
    const char * const           name;         // string id
    const struct AdfDevGeometry  geometry;
    const AdfDevClass            class;
    const char * const           descr;
} adfDevMedia[ ADF_DEVTYPE_NUMTYPES + 1 ] = {
    { "unknown",{  0, 0,  0, 512 }, ADF_DEVCLASS_UNKNOWN,  "unknown"                         },

    { "dd",     { 80, 2, 11, 512 }, ADF_DEVCLASS_FLOP,     "880 KiB floppy"                  },

    { "sd",     { 40, 2, 11, 512 }, ADF_DEVCLASS_FLOP,     "440 KiB floppy (SD, 5.25\")"     },
    { "hd1520", { 80, 2, 19, 512 }, ADF_DEVCLASS_FLOP,     "1520 KiB floppy (HD)"            },
    { "hd1760", { 80, 2, 22, 512 }, ADF_DEVCLASS_FLOP,     "1760 KiB floppy (HD)"            },

    { "dd81",   { 81, 2, 11, 512 }, ADF_DEVCLASS_FLOP,     "891 KiB floppy (81 tracks)"      },
    { "dd82",   { 82, 2, 11, 512 }, ADF_DEVCLASS_FLOP,     "902 KiB floppy (82 tracks)"      },
    { "dd83",   { 83, 2, 11, 512 }, ADF_DEVCLASS_FLOP,     "913 KiB floppy (83 tracks)"      },

    { "hd81",   { 81, 2, 22, 512 }, ADF_DEVCLASS_FLOP,     "1782 KiB floppy (HD, 81 tracks)" },
    { "hd82",   { 82, 2, 22, 512 }, ADF_DEVCLASS_FLOP,     "1804 KiB floppy (HD, 82 tracks)" },
    { "hd83",   { 83, 2, 22, 512 }, ADF_DEVCLASS_FLOP,     "1826 KiB floppy (HD, 83 tracks)" },

    { "pc360",  { 40, 2,  9, 512 }, ADF_DEVCLASS_FLOP,     "PC 360 KiB floppy 5.25\""        },
    { "pc1200", { 80, 2, 15, 512 }, ADF_DEVCLASS_FLOP,     "PC 1,2 MiB floppy 5.25\""        },
    { "pc720",  { 80, 2,  9, 512 }, ADF_DEVCLASS_FLOP,     "PC 720 KiB floppy 3.5\""         },
    { "pc1440", { 80, 2, 18, 512 }, ADF_DEVCLASS_FLOP,     "PC 1.440 MiB floppy 3.5\""       },
    { "pc2880", { 80, 2, 36, 512 }, ADF_DEVCLASS_FLOP,     "PC 2.80 MiB floppy 3.5\""        },

    { "adf",    {  0, 0,  0, 512 }, ADF_DEVCLASS_FLOP,     "Amiga disk file (ADF)"           },
    { "hdf",    {  0, 0,  0, 512 }, ADF_DEVCLASS_HARDFILE, "hard disk file (HDF)"            },
    { "hd",     {  0, 0,  0, 512 }, ADF_DEVCLASS_HARDDISK, "hard disk"                       },

    { "zip",  { 2891, 1, 68, 512 }, ADF_DEVCLASS_HARDDISK, "Zip Disk"                        },

    { "a590",  { 782, 2, 27, 512 }, ADF_DEVCLASS_HARDDISK, "Western Digital WD93028-X A (A590)" },
    { "wdac280", { 980, 10, 17, 512 }, ADF_DEVCLASS_HARDDISK, "Western Digital WDAC280"      },

    { NULL,     {  0, 0,  0, 512 }, ADF_DEVCLASS_UNKNOWN,  NULL                              }
};


AdfDevClass adfDevTypeGetClass( AdfDevType type )
{
    assert( type >= 0 && type < ADF_DEVTYPE_NUMTYPES );
    return adfDevMedia[ type ].class;
}

struct AdfDevGeometry adfDevTypeGetGeometry( AdfDevType type )
{
    assert( type >= 0 && type < ADF_DEVTYPE_NUMTYPES );
    return adfDevMedia[ type ].geometry;
}

const char * adfDevTypeGetDescription( AdfDevType type )
{
    assert( type >= 0 && type < ADF_DEVTYPE_NUMTYPES );
    return adfDevMedia[ type ].descr;
}


AdfDevType adfDevGetTypeBySizeBlocks( uint32_t sizeBlocks )
{
    for ( unsigned i = 0 ; i < ADF_DEVTYPE_NUMTYPES ; i++ ) {
        const struct AdfDevGeometry * const geometry = &adfDevMedia[i].geometry;
        if ( geometry->cylinders *
             geometry->heads *
             geometry->sectors == sizeBlocks )
        {
            return i;
        }
    }
    return ADF_DEVTYPE_UNKNOWN;
}

/*
struct AdfDevGeometry adfDevGetGeometryBySize( uint64_t size )
{
    for ( unsigned i = 0 ; i < ADF_DEVTYPE_NUMTYPES ; i++ ) {
        const AdfDevMedium * const dm = &adfDevMedia[i];
        if ( (uint64_t) dm->geometry.tracks *
             (uint64_t) dm->geometry.heads *
             (uint64_t) dm->geometry.sectors * 512 == size )
        {
            return dm->geometry;
        }
    }
    return ADF_DEVTYPE_UNKNOWN;
}
*/

AdfDevType adfDevGetTypeByGeometry( const struct AdfDevGeometry * const geometry )
{
    for ( unsigned i = 0 ; i < ADF_DEVTYPE_NUMTYPES ; i++ ) {
        const struct AdfDevMedium * const dm = &adfDevMedia[i];
        if ( dm->geometry.cylinders == geometry->cylinders &&
             dm->geometry.heads     == geometry->heads     &&
             dm->geometry.sectors   == geometry->sectors   )
        {
            return i;
        }
    }
    return ADF_DEVTYPE_UNKNOWN;
}



static const struct AdfDevMedium * adfDevGetMediumBySizeBlocks( uint32_t sizeBlocks );

AdfDevClass adfDevGetClassBySizeBlocks( uint32_t sizeBlocks )
{
    // check the predefined list first
    const struct AdfDevMedium * const dm = adfDevGetMediumBySizeBlocks( sizeBlocks );
    if ( dm != NULL )
        return dm->class;

    // if not found on the list - maybe it's an HD or HDF?
    const struct AdfDevMedium * const largestFlop = &adfDevMedia[ ADF_DEVTYPE_FHD83 ];
    if ( sizeBlocks > largestFlop->geometry.cylinders *
                      largestFlop->geometry.heads *
                      largestFlop->geometry.sectors )
    {
        // means also HDF(!), cannot distinguish them by size
        return ADF_DEVCLASS_HARDDISK;
    } else {
        // is this an error?
        ///adfEnv.eFct("%: unknown device type", __func__ ); 
        //return -1;
        return ADF_DEVCLASS_UNKNOWN;
    }
}


static const struct AdfDevMedium * adfDevGetMediumBySizeBlocks( uint32_t sizeBlocks )
{
    for ( const struct AdfDevMedium * dm = &adfDevMedia[0]; dm->name ; dm++ ) {
        if ( dm->geometry.cylinders *
             dm->geometry.heads *
             dm->geometry.sectors == sizeBlocks )
        {
            return dm;
        }
    }
    return NULL;
}







////  LEGACY
#if 0

/*
 * adfDevType
 *
 * returns the type of a device
 * only based of the field 'dev->size'
 */
int adfDevType( const struct AdfDevice * const  dev )
{
    if ( ( dev->size == 512 * 11 * 2 * 80 ) ||		/* BV */
         ( dev->size == 512 * 11 * 2 * 81 ) ||		/* BV */
         ( dev->size == 512 * 11 * 2 * 82 ) || 	/* BV */
         ( dev->size == 512 * 11 * 2 * 83 ) )		/* BV */
        return ADF_DEVTYPE_FLOPDD;
    else if ( dev->size == 512 * 22 * 2 * 80 )
        return ADF_DEVTYPE_FLOPHD;
    else if ( dev->size > 512 * 22 * 2 * 80 )
        return ADF_DEVTYPE_HARDDISK;
    else {
        adfEnv.eFct("%: unknown device type", __func__ );
        return -1;
    }
}

#endif
