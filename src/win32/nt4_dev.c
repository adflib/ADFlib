/*
 *  nt4_dev.c - routines for direct drive access in Windows NT 4.0+
 *
 *  These routines only currently work with drives <2GB and 512 bytes per sector
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
 *  along with Foobar; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include "nt4_dev.h"

/*
 * Open a physical disk device on Windows
 *
 * lpstrDrive[3]  - a 2-letter identification of the device:
 *    [0]  - type (only 'H', meaning 'PhysicalDrive" in Win32 namespace,
 *           is implemented)
 *    [1]  - a numeric id of the disk drive (as, for instance, listed in
 *           "Disk Management")
 *    [2]  - '\0'
 *
 * Useful documentation:
 * - https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file
 * - https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea
 */
HANDLE NT4OpenDrive( const char * const  lpstrDrive )
{
    char    strDriveFile[ 40 ];
    HANDLE  hDrv;
    DWORD   dwRet;

    switch ( lpstrDrive[ 0 ] ) {
    case 'H':
        sprintf( strDriveFile, "\\\\.\\PhysicalDrive%c", lpstrDrive[ 1 ] );
        break;
        /* add support for other device types here */
    default:
        return NULL;
    }

    hDrv = CreateFile( strDriveFile, GENERIC_READ | GENERIC_WRITE,
                      FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                      0, NULL );

    if ( hDrv == INVALID_HANDLE_VALUE )
        return NULL;

    if ( ! DeviceIoControl( hDrv, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0,
                           &dwRet, NULL ) )
        return NULL;

    return hDrv;
}

bool NT4CloseDrive( const HANDLE  hDrv )
{
    DWORD dwRet;

    if ( hDrv == NULL ) return true;				/* BV */

    if ( ! DeviceIoControl( hDrv, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0,
                            &dwRet,	NULL ) )
        return false;

    if ( ! CloseHandle( hDrv ) )
        return false;

    return true;
}

bool NT4ReadSector( const HANDLE  hDrv,
                    const long    iSect,
                    const size_t  iSize,
                    void * const  lpvoidBuf )
{
    void  *lpvoidTempBuf;
    DWORD  dwActual;

    lpvoidTempBuf = VirtualAlloc( NULL, 512, MEM_COMMIT, PAGE_READWRITE );

    if ( SetFilePointer( hDrv, iSect * 512, NULL, FILE_BEGIN ) == 0xFFFFFFFF ) {
        VirtualFree( lpvoidTempBuf, 0, MEM_RELEASE );
        return false;
    }

    if ( ! ReadFile( hDrv, lpvoidTempBuf, 512, &dwActual, NULL ) ) {
        VirtualFree( lpvoidTempBuf, 0, MEM_RELEASE );
        return false;
    }

    memcpy( lpvoidBuf, lpvoidTempBuf, iSize );
    VirtualFree( lpvoidTempBuf, 0, MEM_RELEASE );

    return true;
}

bool NT4WriteSector( const HANDLE        hDrv,
                     const long          iSect,
                     const size_t        iSize,
                     const void * const  lpvoidBuf )
{
    void  *lpvoidTempBuf;
    DWORD  dwActual;

    if ( iSize != 512 )
        return false;

    lpvoidTempBuf = VirtualAlloc( NULL, 512, MEM_COMMIT, PAGE_READWRITE );

    if ( SetFilePointer( hDrv, iSect * 512, NULL, FILE_BEGIN ) == 0xFFFFFFFF ) {
        VirtualFree( lpvoidTempBuf, 0, MEM_RELEASE );
        return false;
    }

    memcpy( lpvoidTempBuf, lpvoidBuf, iSize );

    if ( ! WriteFile( hDrv, lpvoidTempBuf, 512, &dwActual, NULL ) ) {
        VirtualFree( lpvoidTempBuf, 0, MEM_RELEASE );
        return false;
    }
	
    VirtualFree( lpvoidTempBuf, 0, MEM_RELEASE );

    return true;
}

ULONG NT4GetDriveSize( const HANDLE  hDrv )
{
    DWORD          dwActual;
    DISK_GEOMETRY  dgGeom;
    long           size;

    DeviceIoControl( hDrv, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
                     &dgGeom, sizeof(DISK_GEOMETRY), &dwActual, NULL );

    size = dgGeom.Cylinders.LowPart *
           dgGeom.TracksPerCylinder *
           dgGeom.SectorsPerTrack *
           dgGeom.BytesPerSector;
/* BV */
/*	printf("Total sectors: %i\n", dgGeom.Cylinders.LowPart * dgGeom.TracksPerCylinder * dgGeom.SectorsPerTrack);
**	printf("Byte size: %i\n", size);
*/
    return size;
}


bool NT4GetDriveGeometry( const HANDLE                hDrv,
                          NT4DriveGeometry_t * const  geometry )
{
    DWORD          dwActual;
    DISK_GEOMETRY  dgGeom;

    bool status = DeviceIoControl (
        hDrv, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
        &dgGeom, sizeof(DISK_GEOMETRY), &dwActual, NULL );
    if ( ! status )
        return false;

    geometry->cylinders         = dgGeom.Cylinders.LowPart;
    geometry->tracksPerCylinder = dgGeom.TracksPerCylinder;
    geometry->sectorsPerTrack   = dgGeom.SectorsPerTrack;
    geometry->bytesPerSector    = dgGeom.BytesPerSector;

    return true;
}
