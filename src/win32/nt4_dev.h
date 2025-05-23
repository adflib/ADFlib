/*
 *  nt4_dev.h - prototypes for NT4 direct drive access functions
 *
 *  Copyright (C) 1999      Dan Sutherland
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
 */

#ifndef NT4_DEV_H
#define NT4_DEV_H

#include <stdbool.h>

typedef struct NT4DriveGeometry_s {
    unsigned
        cylinders,
        tracksPerCylinder,
        sectorsPerTrack,
        bytesPerSector;
} NT4DriveGeometry_t;

HANDLE NT4OpenDrive( const char * const  strDrive );

bool NT4CloseDrive( const HANDLE  hDrv );

bool NT4ReadSector( const HANDLE  hDrv,
                    const long    iSect,
                    const size_t  iSize,
                    void * const  lpvoidBuf );

bool NT4WriteSector( const HANDLE        hDrv,
                     const long          iSect,
                     const size_t        iSize,
                     const void * const  lpvoidBuf );

ULONG NT4GetDriveSize( const HANDLE  hDrv );

bool NT4GetDriveGeometry( const HANDLE                hDrv,
                          NT4DriveGeometry_t * const  geometry );

#endif  /* NT4_DEV_H */
