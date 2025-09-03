/*
 *  adf_device_drivers.c - device drivers management
 *
 *  Copyright (C) 2024-2025 Tomasz Wolak
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
#include "adf_dev_drivers.h"

#include <stdlib.h>
#include <string.h>


struct AdfDeviceDriverListNode {
    struct AdfDeviceDriverListNode * next;
    const struct AdfDeviceDriver *   driver;
};

static struct AdfDeviceDriverListNode * adfDeviceDrivers = NULL;


ADF_RETCODE adfAddDeviceDriver( const struct AdfDeviceDriver * const driver )
{
    struct AdfDeviceDriverListNode * newNode =
        malloc( sizeof( struct AdfDeviceDriverListNode ) );
    if ( newNode == NULL )
        return ADF_RC_MALLOC;

    newNode->next   = NULL;
    newNode->driver = driver;

    if ( adfDeviceDrivers == NULL )
        adfDeviceDrivers = newNode;
    else {
        struct AdfDeviceDriverListNode * node = adfDeviceDrivers;
        for ( ; node->next != NULL; node = node->next );
        node->next = newNode;
    }
    
    //fprintf (stderr, "Added driver %s\n", newNode->driver->name );

    return ADF_RC_OK;
}


ADF_RETCODE adfRemoveDeviceDriver( const struct AdfDeviceDriver * const driver )
{
    struct AdfDeviceDriverListNode
        *node = adfDeviceDrivers,
        *prev = NULL;

    for ( ; node != NULL;  prev = node,  node = node->next )  {
        if ( node->driver == driver ) {
            if ( prev == NULL ) {
                adfDeviceDrivers = node->next;
            } else {
                prev->next = node->next;
            }
            free( node );
            return ADF_RC_OK;
        }
    }

    return ADF_RC_ERROR;
}


void adfRemoveDeviceDrivers(void)
{
    struct AdfDeviceDriverListNode
        *node = adfDeviceDrivers,
        *next = NULL;

    for ( ; node != NULL; node = next )  {
        //fprintf (stderr, "Removing driver %s\n", node->driver->name );
        //fflush(stderr);
        next = node->next;
        free( node );
    }

    adfDeviceDrivers = NULL;
}


const struct AdfDeviceDriver * adfGetDeviceDriverByName( const char * const name )
{
    for ( struct AdfDeviceDriverListNode * node = adfDeviceDrivers;
          node != NULL; node = node->next )
        if ( strcmp( node->driver->name, name ) == 0 )
            return node->driver;
    return NULL;
}


const struct AdfDeviceDriver * adfGetDeviceDriverByDevName( const char * const name )
{
    for ( struct AdfDeviceDriverListNode * node = adfDeviceDrivers;
          node != NULL; node = node->next )
    {
        if ( node->driver->isDevice == NULL )
            continue;

        if ( node->driver->isDevice( name ) ) {
            //adfEnv.vFct( "%s: matched device %s for name %s",
            //             __func__, node->driver->name, name );
            return node->driver;
        }
    }

    /* if nothing matched -> default to dump file */
    return adfGetDeviceDriverByName( "dump" );
}
