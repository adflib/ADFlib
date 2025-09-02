/*
 *  adf_err.h - error codes
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

#ifndef ADF_ERR_H
#define ADF_ERR_H

#include "adf_types.h"

typedef enum {
    ADF_RC_OK              = 0,
    ADF_RC_ERROR           = -1,
    ADF_RC_ERROR_INTERNAL  = -2,

    ADF_RC_MALLOC          = 1,
    ADF_RC_VOLFULL         = 2,

    ADF_RC_FOPEN           = 1 << 10,
    ADF_RC_NULLPTR         = 1 << 12,
    ADF_RC_NAME_TOO_LONG   = 1 << 13,

/* adfRead*Block() */

    ADF_RC_BLOCKTYPE       = 1,
    ADF_RC_BLOCKSTYPE      = 1 << 1,
    ADF_RC_BLOCKSUM        = 1 << 2,
    ADF_RC_HEADERKEY       = 1 << 3,
    ADF_RC_BLOCKREAD       = 1 << 4,

/* adfWrite*Block */
    ADF_RC_BLOCKWRITE      = 1 << 4,

/* adfVolReadBlock() */
    ADF_RC_BLOCKOUTOFRANGE = 1,
    ADF_RC_BLOCKNATREAD    = 1 << 1,

/* adfVolWriteBlock() */
/* ADF_RC_BLOCKOUTOFRANGE */
    ADF_RC_BLOCKNATWRITE   = 1 << 1,
    ADF_RC_BLOCKREADONLY   = 1 << 2,

/* adfInitDumpDevice() */
/* ADF_RC_FOPEN */
/* ADF_RC_MALLOC */

/* adfNativeReadBlock(), adfReadDumpSector() */

    ADF_RC_BLOCKSHORTREAD  = 1,
    ADF_RC_BLOCKFSEEK      = 1 << 1,

/* adfNativeWriteBlock(), adfWriteDumpSector() */

    ADF_RC_BLOCKSHORTWRITE = 1,
/* ADF_RC_BLOCKFSEEK */


/*-- adfReadRDSKblock --*/
    ADF_RC_BLOCKID         = 1 << 5,

/*-- adfWriteRDSKblock() --*/
/*ADF_RC_BLOCKREADONLY*/
} ADF_RETCODE;

//#define hasRC(rc,c) ((rc)&(c))
static inline bool adfHasRC ( ADF_RETCODE mask,
                              ADF_RETCODE code )
{
    return ( mask & code );
}

#endif  /* ADF_ERR_H */
