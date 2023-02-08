#ifndef ADF_DIR_H
#define ADF_DIR_H 1

/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_dir.h
 *
 *  $Id$
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

#include "adf_blk.h"
#include"adf_str.h"
#include"adf_err.h"
#include"adf_defs.h"
#include "adf_vol.h"

#include"prefix.h"


/* ----- ENTRY ---- */

struct AdfEntry {
    int type;
    char* name;
    SECTNUM sector;
    SECTNUM real;
    SECTNUM parent;
    char* comment;
    uint32_t size;
    int32_t access;
    int year, month, days;
    int hour, mins, secs;
};


PREFIX RETCODE adfToRootDir ( struct AdfVolume * vol );
BOOL isDirEmpty(struct bDirBlock *dir);

PREFIX RETCODE adfRemoveEntry ( struct AdfVolume * vol,
                                SECTNUM            pSect,
                                char *             name );

PREFIX struct List * adfGetDirEnt ( struct AdfVolume * vol,
                                    SECTNUM            nSect );

PREFIX struct List * adfGetRDirEnt ( struct AdfVolume * vol,
                                     SECTNUM            nSect,
                                     BOOL               recurs );

PREFIX void adfFreeDirList(struct List* list);

PREFIX int adfDirCountEntries ( struct AdfVolume * const vol,
                                SECTNUM                  dirPtr );

RETCODE adfEntBlock2Entry ( struct bEntryBlock * entryBlk,
                            struct AdfEntry *    entry );

PREFIX void adfFreeEntry (struct AdfEntry * entry );

RETCODE adfCreateFile ( struct AdfVolume *        vol,
                        SECTNUM                   parent,
                        char *                    name,
                        struct bFileHeaderBlock * fhdr );

PREFIX RETCODE adfCreateDir ( struct AdfVolume * vol,
                              SECTNUM            parent,
                              char *             name );

SECTNUM adfCreateEntry ( struct AdfVolume *   vol,
                         struct bEntryBlock * dir,
                         char *               name,
                         SECTNUM              thisSect );

PREFIX RETCODE adfRenameEntry ( struct AdfVolume * vol,
                                SECTNUM            pSect,
                                char *             oldName,
                                SECTNUM            nPSect,
                                char *             newName );

PREFIX RETCODE adfReadEntryBlock ( struct AdfVolume *   vol,
                                   SECTNUM              nSect,
                                   struct bEntryBlock * ent );

RETCODE adfWriteDirBlock ( struct AdfVolume * vol,
                           SECTNUM            nSect,
                           struct bDirBlock * dir );

RETCODE adfWriteEntryBlock ( struct AdfVolume *   vol,
                             SECTNUM              nSect,
                             struct bEntryBlock * ent );

char* adfAccess2String(int32_t acc);
uint8_t adfIntlToUpper(uint8_t c);
int adfGetHashValue(uint8_t *name, BOOL intl);
void myToUpper( uint8_t *ostr, uint8_t *nstr, int,BOOL intl );
PREFIX RETCODE adfChangeDir ( struct AdfVolume * vol,
                              char *             name );
PREFIX RETCODE adfParentDir ( struct AdfVolume * vol );

PREFIX RETCODE adfSetEntryAccess ( struct AdfVolume * vol,
                                   SECTNUM            parSect,
                                   char *             name,
                                   int32_t            newAcc );

PREFIX RETCODE adfSetEntryComment ( struct AdfVolume * vol,
                                    SECTNUM            parSect,
                                    char *             name,
                                    char *             newCmt );

PREFIX SECTNUM adfNameToEntryBlk ( struct AdfVolume *   vol,
                                   int32_t              ht[],
                                   char *               name,
                                   struct bEntryBlock * entry,
                                   SECTNUM *            nUpdSect );

PREFIX void printEntry ( struct AdfEntry * entry );

#endif /* ADF_DIR_H */

