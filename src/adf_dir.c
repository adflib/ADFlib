/*
 *  adf_dir.c - directory code
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

#include "adf_dir.h"

#include "adf_bitm.h"
#include "adf_byteorder.h"
#include "adf_cache.h"
#include "adf_env.h"
#include "adf_file_block.h"
#include "adf_raw.h"
#include "adf_util.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void adfStrToUpper( uint8_t * const        nstr,
                           const uint8_t * const  ostr,
                           const unsigned         nlen,
                           const bool             intl );

static unsigned adfGetHashValue( const uint8_t * const  name,
                                 const bool             intl );


/*
 * adfToRootDir
 *
 */
ADF_RETCODE adfToRootDir( struct AdfVolume * const  vol )
{
    vol->curDirPtr = vol->rootBlock;
    return ADF_RC_OK;
}

/*
 * adfChangeDir
 *
 */
ADF_RETCODE adfChangeDir( struct AdfVolume * const  vol,
                          const char * const        name )
{
    struct AdfEntryBlock entry;

    ADF_RETCODE rc = adfReadEntryBlock( vol, vol->curDirPtr, &entry );
    if ( rc != ADF_RC_OK )
        return rc;

    ADF_SECTNUM nSect = adfNameToEntryBlk( vol, entry.hashTable, name, &entry, NULL );
    if ( nSect == -1 )
        return ADF_RC_ERROR;

    // if current entry is a hard-link - load entry of the hard-linked directory
    rc = adfReadEntryBlock( vol, nSect, &entry );
    if ( rc != ADF_RC_OK )
        return rc;
    if ( entry.realEntry )  {
        nSect = entry.realEntry;
    }

/*printf("adfChangeDir=%d\n",nSect);*/
    if ( nSect != -1 ) {
        vol->curDirPtr = nSect;
/*        if (*adfEnv.useNotify)
            (*adfEnv.notifyFct)( 0, ADF_ST_ROOT );*/
        return ADF_RC_OK;
    } else
        return ADF_RC_ERROR;
}

/*
 * adfParentDir
 *
 */
ADF_SECTNUM adfParentDir( struct AdfVolume * const  vol )
{
    if ( vol->curDirPtr != vol->rootBlock ) {
        struct AdfEntryBlock entry;
        ADF_RETCODE rc = adfReadEntryBlock( vol, vol->curDirPtr, &entry );
        if ( rc != ADF_RC_OK )
            return rc;
        vol->curDirPtr = entry.parent;
    }
    return ADF_RC_OK;
}


/*
 * adfGetDirEnt
 *
 */
struct AdfList * adfGetDirEnt( const struct AdfVolume * const  vol,
                               const ADF_SECTNUM               nSect )
{
    return adfGetRDirEnt( vol, nSect, false );
}

/*
 * adfGetRDirEnt
 *
 */
struct AdfList * adfGetRDirEnt( const struct AdfVolume * const  vol,
                                const ADF_SECTNUM               nSect,
                                const bool                      recurs )
{
    struct AdfList *cell, *head;
    struct AdfEntry * entry;
    int32_t *hashTable;
    struct AdfEntryBlock parent, entryBlk;

    if ( adfEnv.useDirCache && adfVolHasDIRCACHE( vol ) )
        return adfGetDirEntCache( vol, nSect, recurs );

    if ( adfReadEntryBlock( vol, nSect, &parent ) != ADF_RC_OK )
        return NULL;

    hashTable = parent.hashTable;
    cell = head = NULL;
    for ( int i = 0 ; i < ADF_HT_SIZE ; i++ ) {
        if ( hashTable[ i ] != 0 ) {
             entry = ( struct AdfEntry * ) malloc( sizeof(struct AdfEntry) );
             if ( ! entry ) {
                 adfFreeDirList( head );
                 adfEnv.eFct( "%s: malloc", __func__ );
                 return NULL;
             }
             if ( adfReadEntryBlock( vol, hashTable[ i ], &entryBlk ) != ADF_RC_OK ) {
                 free( entry );
                 adfFreeDirList( head );
                 return NULL;
             }
             if ( adfEntBlock2Entry( &entryBlk, entry ) != ADF_RC_OK ) {
                 free( entry );
                 adfFreeDirList( head );
                 return NULL;
             }
             entry->sector = hashTable[ i ];
	
             if ( head == NULL )
                 head = cell = adfListNewCell( NULL, (void *) entry );
             else
                 cell = adfListNewCell( cell, (void *) entry );
             if ( cell == NULL ) {
                 free( entry );
                 adfFreeDirList( head );
                 return NULL;
             }

             if ( recurs && entry->type == ADF_ST_DIR )
                 cell->subdir = adfGetRDirEnt( vol, entry->sector, recurs );

             /* same hashcode linked list */
             ADF_SECTNUM nextSector = entryBlk.nextSameHash;
             while ( nextSector != 0 ) {
                 entry = ( struct AdfEntry * ) malloc( sizeof(struct AdfEntry) );
                 if ( ! entry ) {
                     adfFreeDirList( head );
                     adfEnv.eFct( "%s: malloc", __func__ );
                     return NULL;
                 }
                 if ( adfReadEntryBlock( vol, nextSector, &entryBlk ) != ADF_RC_OK ) {
                     free( entry );
                     adfFreeDirList( head );
                     return NULL;
                 }

                 if ( adfEntBlock2Entry( &entryBlk, entry ) != ADF_RC_OK ) {
                     free( entry );
                     adfFreeDirList( head );
                     return NULL;
                 }
                 entry->sector = nextSector;
	
                 cell = adfListNewCell( cell, (void *) entry );
                 if ( cell == NULL ) {
                     adfFreeDirList( head );
                     return NULL;
                 }
				 
                 if ( recurs && entry->type == ADF_ST_DIR )
                     cell->subdir = adfGetRDirEnt( vol, entry->sector, recurs );
				 
                 nextSector = entryBlk.nextSameHash;
             }
        }
    }

/*    if (parent.extension && isDIRCACHE(vol->fs.type) )
        adfReadDirCache(vol,parent.extension);
*/
    return head;
}

/*
 * adfFreeDirList
 *
 */
void adfFreeDirList( struct AdfList * const  list )
{
    struct AdfList *root, *cell;

    root = cell = list;
    while ( cell != NULL ) {
        adfFreeEntry( cell->content );
        if ( cell->subdir != NULL )
            adfFreeDirList( cell->subdir );
        cell = cell->next;
    }
    adfListFree( root );
}

/*
 * adfFreeEntry
 *
 */
void adfFreeEntry( struct AdfEntry * const  entry )
{
    if ( entry == NULL )
       return;
    if ( entry->name )
        free( entry->name );
    if ( entry->comment )
        free( entry->comment );
    free( entry );
}


/*
 * adfGetEntry
 *
 */
ADF_RETCODE adfGetEntry( struct AdfVolume * const  vol,
                         const ADF_SECTNUM         dirPtr,
                         const char * const        name,
                         struct AdfEntry * const   entry )
{
    struct AdfEntryBlock  entryBlock;
    //printf ("%s: name '%s'\n", __func__, name );
    if ( adfGetEntryBlock( vol, dirPtr, name, &entryBlock ) == -1 ||
         adfEntBlock2Entry( &entryBlock, entry ) != ADF_RC_OK )
    {
        return ADF_RC_ERROR;
    }

    return ADF_RC_OK;
}


/*
 * adfGetEntryBlock
 *
 */
ADF_SECTNUM adfGetEntryBlock( struct AdfVolume * const      vol,
                              const ADF_SECTNUM             dirPtr,
                              const char * const            name,
                              struct AdfEntryBlock * const  entryBlock )
{
    // get parent
    struct AdfEntryBlock parent;
    ADF_RETCODE rc = adfReadEntryBlock( vol, dirPtr, &parent );
    if ( rc != ADF_RC_OK ) {
        adfEnv.eFct( "%s: error reading parent entry "
                     "(block %d)\n", __func__, dirPtr );
        return -1;
    }

    // get entry
    ADF_SECTNUM nUpdSect;
    ADF_SECTNUM sectNum = adfNameToEntryBlk( vol, parent.hashTable, name,
                                             entryBlock, &nUpdSect );
    return sectNum;
}


/*
 * adfGetEntryBlockNum
 *
 */
ADF_SECTNUM adfGetEntryBlockNum( struct AdfVolume * const  vol,
                                 const ADF_SECTNUM         dirPtr,
                                 const char * const        name )
{
    struct AdfEntryBlock entryBlock;
    return adfGetEntryBlock( vol, dirPtr, name, &entryBlock );
}


/*
 * adfCreateFile
 *
 */
ADF_RETCODE adfCreateFile( struct AdfVolume * const           vol,
                           const ADF_SECTNUM                  nParent,
                           const char * const                 name,
                           struct AdfFileHeaderBlock * const  fhdr )
{
    struct AdfEntryBlock parent;
/*puts("adfCreateFile in");*/

    ADF_RETCODE rc = adfReadEntryBlock( vol, nParent, &parent );
    if ( rc != ADF_RC_OK )
        return rc;

    /* -1 : do not use a specific, already allocated sector */
    const ADF_SECTNUM nSect = adfCreateEntry( vol, &parent, name, -1 );
    if ( nSect == -1 ) return ADF_RC_ERROR;
/*printf("new fhdr=%d\n",nSect);*/
    memset( fhdr, 0, 512 );
    fhdr->nameLen = (uint8_t) min( (unsigned) ADF_MAX_NAME_LEN,
                                   (unsigned) strlen( name ) );
    memcpy( fhdr->fileName, name, fhdr->nameLen );
    fhdr->headerKey = nSect;
    if ( parent.secType == ADF_ST_ROOT )
        fhdr->parent = vol->rootBlock;
    else if ( parent.secType == ADF_ST_DIR )
        fhdr->parent = parent.headerKey;
    else
        adfEnv.wFct("%s: unknown parent secType", __func__ );
    adfTime2AmigaTime( adfGiveCurrentTime(), &fhdr->days, &fhdr->mins, &fhdr->ticks );

    rc = adfWriteFileHdrBlock( vol, nSect, fhdr );
    if ( rc != ADF_RC_OK )
        return rc;

    if ( adfVolHasDIRCACHE( vol ) ) {
        rc = adfAddInCache( vol, &parent, (struct AdfEntryBlock *) fhdr );
        if ( rc != ADF_RC_OK )
            return rc;
    }

    rc = adfUpdateBitmap( vol );

    if ( adfEnv.useNotify )
        adfEnv.notifyFct( nParent, ADF_ST_FILE );

    return rc;
}

/*
 * adfCreateDir
 *
 */
ADF_RETCODE adfCreateDir( struct AdfVolume * const  vol,
                          const ADF_SECTNUM         nParent,
                          const char * const        name )
{
    struct AdfEntryBlock parent;

    ADF_RETCODE rc = adfReadEntryBlock( vol, nParent, &parent );
    if ( rc != ADF_RC_OK )
        return rc;

    /* -1 : do not use a specific, already allocated sector */
    const ADF_SECTNUM nSect = adfCreateEntry( vol, &parent, name, -1 );
    if ( nSect == -1 ) {
        adfEnv.wFct("%s: no sector available", __func__ );
        return ADF_RC_ERROR;
    }

    struct AdfDirBlock dir;
    memset( &dir, 0, sizeof(struct AdfDirBlock) );
    dir.nameLen = (uint8_t) min( (unsigned) ADF_MAX_NAME_LEN,
                                 (unsigned) strlen ( name ) );
    memcpy( dir.dirName, name, dir.nameLen );
    dir.headerKey = nSect;

    if ( parent.secType == ADF_ST_ROOT )
        dir.parent = vol->rootBlock;
    else
        dir.parent = parent.headerKey;
    adfTime2AmigaTime( adfGiveCurrentTime(), &dir.days, &dir.mins, &dir.ticks );

    if ( adfVolHasDIRCACHE( vol ) ) {
        /* for adfCreateEmptyCache, will be added by adfWriteDirBlock */
        dir.secType = ADF_ST_DIR;
        rc = adfAddInCache( vol, &parent, (struct AdfEntryBlock *) &dir );
        if ( rc != ADF_RC_OK )
            return rc;
        rc = adfCreateEmptyCache( vol, (struct AdfEntryBlock *) &dir, -1 );
        if ( rc != ADF_RC_OK )
            return rc;
    }

    /* writes the dirblock, with the possible dircache assiocated */
    rc = adfWriteDirBlock( vol, nSect, &dir );
    if ( rc != ADF_RC_OK )
        return rc;

    rc = adfUpdateBitmap( vol );

    if ( adfEnv.useNotify )
        adfEnv.notifyFct( nParent, ADF_ST_DIR );

    return rc;
}


/*
 * adfCreateEntry
 *
 * if 'thisSect'==-1, allocate a sector, and insert its pointer into the hashTable of 'dir', using the 
 * name 'name'. if 'thisSect'!=-1, insert this sector pointer  into the hashTable 
 * (here 'thisSect' must be allocated before in the bitmap).
 */
ADF_SECTNUM adfCreateEntry( struct AdfVolume * const      vol,
                            struct AdfEntryBlock * const  dir,
                            const char * const            name,
                            const ADF_SECTNUM             thisSect )
{
    const unsigned len = (unsigned) strlen( name );
    if ( len > ADF_MAX_NAME_LEN ) {
        adfEnv.eFct( "%s: name '%s' is too long (%u > max. %d characters).",
                     __func__, name, len, ADF_MAX_NAME_LEN );
        return -1;
    }

    ADF_RETCODE  rc;
    ADF_SECTNUM  newSect;
    char  name2[ ADF_MAX_NAME_LEN + 1 ],
          name3[ ADF_MAX_NAME_LEN + 1 ];

/*puts("adfCreateEntry in");*/

    bool intl = adfVolHasINTL( vol ) ||
                adfVolHasDIRCACHE( vol );

    adfStrToUpper( (uint8_t *) name2,
                   (uint8_t *) name, len, intl );
    unsigned hashValue = adfGetHashValue( (uint8_t *) name, intl );
    ADF_SECTNUM nSect = dir->hashTable[ hashValue ];

    if ( nSect == 0 ) {
        /* the first entry with this hash */

        if ( thisSect != -1 )
            newSect = thisSect;
        else {
            newSect = adfGet1FreeBlock( vol );
            if ( newSect == -1 ) {
                adfEnv.wFct( "%s: nSect == -1", __func__ );
                return -1;
            }
        }

        dir->hashTable[ hashValue ] = newSect;
        if ( dir->secType == ADF_ST_ROOT ) {
            struct AdfRootBlock * const root = (struct AdfRootBlock *) dir;
            adfTime2AmigaTime( adfGiveCurrentTime(),
                               &root->cDays, &root->cMins, &root->cTicks );
            rc = adfWriteRootBlock( vol, (uint32_t) vol->rootBlock, root );
        }
        else {
            adfTime2AmigaTime( adfGiveCurrentTime(), &dir->days,&dir->mins,&dir->ticks );
            rc = adfWriteDirBlock( vol, dir->headerKey, (struct AdfDirBlock *) dir );
        }
/*puts("adfCreateEntry out, dir");*/

    } else {
        /* at least already one entry with this hash */

        struct AdfEntryBlock updEntry;

        /* find the last on the list */
        do {
            if ( adfReadEntryBlock( vol, nSect, &updEntry ) != ADF_RC_OK )
                return -1;

            /* check if the entry with the same name is not present */
            if ( updEntry.nameLen == len ) {
                adfStrToUpper( (uint8_t *) name3,
                               (uint8_t *) updEntry.name,
                               updEntry.nameLen, intl );
                if ( strncmp( name3, name2, len) == 0 ) {
                    adfEnv.wFct( "%s: entry already exists", __func__ );
                    return -1;
                }
            }
            nSect = updEntry.nextSameHash;
        } while ( nSect != 0 );

        /* set sector of the new entry */
        if ( thisSect != -1 )
            newSect = thisSect;
        else {
            newSect = adfGet1FreeBlock( vol );
            if ( newSect == -1 ) {
                adfEnv.wFct( "%s: nSect==-1", __func__ );
                return -1;
            }
        }

        /* add the new entry at the end of the list */
        rc = ADF_RC_OK;
        updEntry.nextSameHash = newSect;

        /* write changes */
        if ( updEntry.secType == ADF_ST_DIR )
            rc = adfWriteDirBlock( vol, updEntry.headerKey,
                                   (struct AdfDirBlock *) &updEntry );
        else if ( updEntry.secType == ADF_ST_FILE )
            rc = adfWriteFileHdrBlock( vol, updEntry.headerKey,
                                       (struct AdfFileHeaderBlock *) &updEntry );
        else {
            adfEnv.eFct( "%s: entry '%s' has unknown type %d",
                         __func__, updEntry.name, updEntry.secType );
            rc = ADF_RC_ERROR;
        }

/*puts("adfCreateEntry out, hash");*/
    }

    if ( rc != ADF_RC_OK ) {
        adfSetBlockFree( vol, newSect );
        return -1;
    }
    else
        return newSect;
}


/*
 * adfRemoveEntry
 *
 */
ADF_RETCODE adfRemoveEntry( struct AdfVolume * const  vol,
                            const ADF_SECTNUM         pSect,
                            const char * const        name )
{
    struct AdfEntryBlock parent, previous, entry;

    ADF_RETCODE rc = adfReadEntryBlock( vol, pSect, &parent );
    if ( rc != ADF_RC_OK )
        return rc;

    ADF_SECTNUM nSect2;
    const ADF_SECTNUM nSect =
        adfNameToEntryBlk( vol, parent.hashTable, name, &entry, &nSect2 );
    if ( nSect == -1 ) {
        adfEnv.wFct( "%s: entry '%s' not found", __func__, name );
        return ADF_RC_ERROR;
    }
    /* if it is a directory, is it empty ? */
    if ( entry.secType == ADF_ST_DIR &&
         ! adfIsDirEmpty ( (struct AdfDirBlock *) &entry ) )
    {
        adfEnv.wFct( "%s: directory '%s' not empty", __func__, name );
        return ADF_RC_ERROR;
    }
/*    printf("name=%s  nSect2=%ld\n",name, nSect2);*/

    /* in parent hashTable */
    if ( nSect2 == 0 ) {
        bool intl = adfVolHasINTL( vol ) ||
                    adfVolHasDIRCACHE( vol );
        unsigned hashVal = adfGetHashValue( (uint8_t *) name, intl );
/*printf("hashTable=%d nexthash=%d\n",parent.hashTable[hashVal],
 entry.nextSameHash);*/
        parent.hashTable[ hashVal ] = entry.nextSameHash;
        rc = adfWriteEntryBlock( vol, pSect, &parent );
        if ( rc != ADF_RC_OK )
            return rc;
    }
    /* in linked list */
    else {
        rc = adfReadEntryBlock( vol, nSect2, &previous );
        if ( rc != ADF_RC_OK )
            return rc;
        previous.nextSameHash = entry.nextSameHash;
        rc = adfWriteEntryBlock( vol, nSect2, &previous );
        if ( rc != ADF_RC_OK )
            return rc;
    }

    if ( entry.secType == ADF_ST_FILE ) {
        rc = adfFreeFileBlocks( vol, (struct AdfFileHeaderBlock*) &entry );
        if ( rc != ADF_RC_OK )
            return rc;
        adfSetBlockFree( vol, nSect ); //marks the FileHeaderBlock as free in BitmapBlock
        if ( adfEnv.useNotify )
             adfEnv.notifyFct( pSect, ADF_ST_FILE );
    }
    else if ( entry.secType == ADF_ST_DIR ) {
        adfSetBlockFree( vol, nSect );
        /* free dir cache block : the directory must be empty, so there's only one cache block */
        if ( adfVolHasDIRCACHE( vol ) )
            adfSetBlockFree( vol, entry.extension );

        if ( adfEnv.useNotify )
            adfEnv.notifyFct( pSect, ADF_ST_DIR );
    }
    else {
        adfEnv.wFct( "%s: secType %d not supported", __func__, entry.secType );
        return ADF_RC_ERROR;
    }

    if ( adfVolHasDIRCACHE( vol ) ) {
        rc = adfDelFromCache( vol, &parent, entry.headerKey );
        if ( rc != ADF_RC_OK )
            return rc;
    }

    rc = adfUpdateBitmap( vol );

    return rc;
}


/*
 * adfRenameEntry
 *
 */
ADF_RETCODE adfRenameEntry( struct AdfVolume * const  vol,
                            const ADF_SECTNUM         pSect,
                            const char * const        oldName,
                            const ADF_SECTNUM         nPSect,
                            const char * const        newName )
{
    const unsigned len = (unsigned) strlen( newName );
    if ( len > ADF_MAX_NAME_LEN ) {
        adfEnv.eFct( "%s: name '%s' is too long (%u > max. %d characters).",
                     __func__, newName, len, ADF_MAX_NAME_LEN );
        return ADF_RC_NAME_TOO_LONG;
    }

    struct AdfEntryBlock parent, previous, entry, nParent;
    char name2[ ADF_MAX_NAME_LEN + 1 ],
         name3[ ADF_MAX_NAME_LEN + 1 ];

    if ( pSect == nPSect  &&
         strcmp( oldName, newName ) == 0 )
    {
        return ADF_RC_OK;
    }

    bool intl = adfVolHasINTL( vol ) ||
                adfVolHasDIRCACHE( vol );
    adfStrToUpper( (uint8_t *) name2, (uint8_t*) newName, len, intl );
    adfStrToUpper( (uint8_t *) name3, (uint8_t*) oldName, (unsigned) strlen(oldName), intl );
    /* newName == oldName ? */

    ADF_RETCODE rc = adfReadEntryBlock( vol, pSect, &parent );
    if ( rc != ADF_RC_OK )
        return rc;

    unsigned hashValueO = adfGetHashValue( (uint8_t *) oldName, intl );

    ADF_SECTNUM prevSect = -1;
    const ADF_SECTNUM nSect =
        adfNameToEntryBlk( vol, parent.hashTable, oldName, &entry, &prevSect );
    if ( nSect == -1 ) {
        adfEnv.wFct( "%s: entry '%s' not found", __func__, oldName );
        return ADF_RC_ERROR;
    }

    /* change name and parent dir */
    entry.nameLen = (uint8_t) min( 31u, len );
    memcpy( entry.name, newName, entry.nameLen );
    entry.parent = nPSect;
    const ADF_SECTNUM tmpSect = entry.nextSameHash;

    entry.nextSameHash = 0;
    rc = adfWriteEntryBlock( vol, nSect, &entry );
    if ( rc != ADF_RC_OK )
        return rc;

    /* del from the oldname list */

    /* in hashTable */
    if ( prevSect == 0 ) {
        parent.hashTable[ hashValueO ] = tmpSect;
    }
    else {
        /* in linked list */
        rc = adfReadEntryBlock( vol, prevSect, &previous );
        if ( rc != ADF_RC_OK )
            return rc;
        /* entry.nextSameHash (tmpSect) could be == 0 */
        previous.nextSameHash = tmpSect;
        rc = adfWriteEntryBlock( vol, prevSect, &previous );
        if ( rc != ADF_RC_OK )
            return rc;
    }

    // update old parent's ctime and write its block
    adfTime2AmigaTime( adfGiveCurrentTime(),
                       &parent.days,
                       &parent.mins,
                       &parent.ticks );

    if ( parent.secType == ADF_ST_ROOT )
        rc = adfWriteRootBlock( vol, (uint32_t) pSect, (struct AdfRootBlock*) &parent );
    else
        rc = adfWriteDirBlock( vol, pSect, (struct AdfDirBlock*) &parent );
    if ( rc != ADF_RC_OK )
        return rc;

    rc = adfReadEntryBlock( vol, nPSect, &nParent );
    if ( rc != ADF_RC_OK )
        return rc;

    unsigned hashValueN = adfGetHashValue( (uint8_t * ) newName, intl );
    ADF_SECTNUM nSect2 = nParent.hashTable[ hashValueN ];
    /* no list */
    if ( nSect2 == 0 ) {
        nParent.hashTable[ hashValueN ] = nSect;
    }
    else {
        /* a list exists : addition at the end */
        /* len = strlen(newName);
                   * name2 == newName
                   */
        do {
            rc = adfReadEntryBlock( vol, nSect2, &previous );
            if ( rc != ADF_RC_OK )
                return rc;
            if ( previous.nameLen == len ) {
                adfStrToUpper( (uint8_t *) name3,
                               (uint8_t *) previous.name,
                               previous.nameLen, intl );
                if ( strncmp( name3, name2, len ) == 0 ) {
                    adfEnv.wFct( "%s: entry already exists", __func__ );
                    return ADF_RC_ERROR;
                }
            }
            nSect2 = previous.nextSameHash;
/*printf("sect=%ld\n",nSect2);*/
        } while ( nSect2 != 0 );

        previous.nextSameHash = nSect;
        if ( previous.secType == ADF_ST_DIR )
            rc = adfWriteDirBlock( vol, previous.headerKey,
                                   (struct AdfDirBlock *) &previous );
        else if ( previous.secType == ADF_ST_FILE )
            rc = adfWriteFileHdrBlock( vol, previous.headerKey,
                                       (struct AdfFileHeaderBlock *) &previous );
        else {
            adfEnv.wFct( "%s: unknown entry type", __func__ );
            rc = ADF_RC_ERROR;
        }
        if ( rc != ADF_RC_OK )
            return rc;
    }

    // update new parent's time and write its block
    adfTime2AmigaTime( adfGiveCurrentTime(),
                       &nParent.days,
                       &nParent.mins,
                       &nParent.ticks );

    if ( nParent.secType == ADF_ST_ROOT )
        rc = adfWriteRootBlock( vol, (uint32_t) nPSect, (struct AdfRootBlock *) &nParent );
    else
        rc = adfWriteDirBlock( vol, nPSect, (struct AdfDirBlock *) &nParent );
    if ( rc != ADF_RC_OK )
        return rc;

    // update dircache
    if ( adfVolHasDIRCACHE( vol ) ) {
        if ( pSect == nPSect ) {
            rc = adfUpdateCache( vol, &parent,
                                 (struct AdfEntryBlock *) &entry, true );
        }
        else {
            rc = adfDelFromCache( vol, &parent, entry.headerKey );
            if ( rc != ADF_RC_OK )
                return rc;
            rc = adfAddInCache( vol, &nParent, &entry );
        }
    }
/*
    if (isDIRCACHE(vol->fs.type) && pSect!=nPSect) {
        adfUpdateCache ( vol, &nParent, (struct AdfEntryBlock *) &entry, true );
    }
*/
    return rc;
}


/*
 * adfSetEntryAccess
 *
 */
ADF_RETCODE adfSetEntryAccess( struct AdfVolume * const  vol,
                               const ADF_SECTNUM         parSect,
                               const char * const        name,
                               const int32_t             newAcc )
{
    struct AdfEntryBlock parent, entry;

    ADF_RETCODE rc = adfReadEntryBlock( vol, parSect, &parent );
    if ( rc != ADF_RC_OK )
        return rc;

    const ADF_SECTNUM
        nSect = adfNameToEntryBlk( vol, parent.hashTable, name, &entry, NULL );
    if ( nSect == -1 ) {
        adfEnv.wFct( "%s: entry not found", __func__ );
        return ADF_RC_ERROR;
    }

    entry.access = newAcc;
    if ( entry.secType == ADF_ST_DIR ) {
        rc = adfWriteDirBlock( vol, nSect, (struct AdfDirBlock *) &entry );
        if ( rc != ADF_RC_OK )
            return rc;
    }
    else if ( entry.secType == ADF_ST_FILE) {
        adfWriteFileHdrBlock( vol, nSect, (struct AdfFileHeaderBlock *) &entry );
        if ( rc != ADF_RC_OK )
            return rc;
    }
    else {
        adfEnv.wFct( "%s: entry secType incorrect", __func__ );
        // abort here?
    }

    if ( adfVolHasDIRCACHE( vol ) )
        rc = adfUpdateCache( vol, &parent, (struct AdfEntryBlock *) &entry, false );

    return rc;
}


/*
 * adfSetEntryComment
 *
 */
ADF_RETCODE adfSetEntryComment( struct AdfVolume * const  vol,
                                const ADF_SECTNUM         parSect,
                                const char * const        name,
                                const char * const        newCmt )
{
    struct AdfEntryBlock parent, entry;

    ADF_RETCODE rc = adfReadEntryBlock( vol, parSect, &parent );
    if ( rc != ADF_RC_OK )
        return rc;

    const ADF_SECTNUM nSect =
        adfNameToEntryBlk( vol, parent.hashTable, name, &entry, NULL );
    if ( nSect == -1 ) {
        adfEnv.wFct( "%s: entry not found", __func__ );
        return ADF_RC_ERROR;
    }

    entry.commLen = (uint8_t) min( (unsigned) ADF_MAX_COMMENT_LEN,
                                   strlen ( newCmt ) );
    memcpy( entry.comment, newCmt, entry.commLen );

    if ( entry.secType == ADF_ST_DIR ) {
        rc = adfWriteDirBlock( vol, nSect, (struct AdfDirBlock*) &entry );
        if ( rc != ADF_RC_OK )
            return rc;
    }
    else if ( entry.secType == ADF_ST_FILE ) {
        rc = adfWriteFileHdrBlock( vol, nSect, (struct AdfFileHeaderBlock *) &entry );
        if ( rc != ADF_RC_OK )
            return rc;
    }
    else {
        adfEnv.wFct("%s: entry secType incorrect", __func__ );
        // abort here?
    }

    if ( adfVolHasDIRCACHE( vol ) )
        rc = adfUpdateCache( vol, &parent, (struct AdfEntryBlock *) &entry, true );

    return rc;
}


/*
 * adfIsDirEmpty
 *
 */
bool adfIsDirEmpty( const struct AdfDirBlock * const  dir )
{
    for ( int i = 0 ; i < ADF_HT_SIZE ; i++ )
        if ( dir->hashTable[ i ] != 0 )
           return false;
    return true;
}


/*
 * adfDirCountEntries
 *
 */
int adfDirCountEntries( struct AdfVolume * const  vol,
                        const ADF_SECTNUM         dirPtr )
{
    struct AdfList *list, *cell;

    int nentries = 0;
    cell = list = adfGetDirEnt( vol, dirPtr );
    while ( cell ) {
        //adfEntryPrint ( cell->content );
        cell = cell->next;
        nentries++;
    }
    adfFreeDirList( list );
    return nentries;
}


/*
 * adfReadEntryBlock
 *
 */
ADF_RETCODE adfReadEntryBlock( const struct AdfVolume * const  vol,
                               const ADF_SECTNUM               nSect,
                               struct AdfEntryBlock * const    ent )
{
    uint8_t  buf[ 512 ];

    ADF_RETCODE rc = adfVolReadBlock( vol, (uint32_t) nSect, buf );
    if ( rc != ADF_RC_OK )
        return rc;

    memcpy( ent, buf, 512 );
#ifdef LITT_ENDIAN
    int32_t  secType = (int32_t) swapUint32fromPtr( (uint8_t *) &ent->secType );
    if ( secType == ADF_ST_LFILE ||
         secType == ADF_ST_LDIR ||
         secType == ADF_ST_LSOFT  )
    {
        adfSwapEndian( (uint8_t *) ent, ADF_SWBL_LINK );
    } else {
        adfSwapEndian( (uint8_t *) ent, ADF_SWBL_ENTRY );
    }
#endif
/*printf("readentry=%d\n",nSect);*/

    const uint32_t  checksumCalculated = adfNormalSum( (uint8_t *) buf, 20, 512 );
    if ( ent->checkSum != checksumCalculated ) {
        const char  msg[] = "%s: invalid checksum 0x%x != "
            "0x%x (calculated), block %d, volume '%s'";
        if ( adfEnv.ignoreChecksumErrors ) {
            adfEnv.wFct( msg, __func__, ent->checkSum, checksumCalculated,
                         nSect, vol->volName );
        } else {
            adfEnv.eFct( msg, __func__, ent->checkSum, checksumCalculated,
                         nSect, vol->volName );
            return ADF_RC_BLOCKSUM;
        }
    }

    if ( ent->type != ADF_T_HEADER)  {
        adfEnv.wFct( "%s: ADF_T_HEADER id not found, volume '%s', block %u",
                     __func__, vol->volName, nSect );
        return ADF_RC_ERROR;
    }
    if ( ent->nameLen > ADF_MAX_NAME_LEN ) {
        adfEnv.wFct( "%s: nameLen (%d) incorrect, volume '%s', block %u, entry %s",
                     __func__, ent->nameLen, vol->volName, nSect, ent->name );
        //printf("nameLen=%d, commLen=%d, name=%s sector%d\n",
        //    ent->nameLen,ent->commLen,ent->name, ent->headerKey);
    }
    if ( ent->commLen > ADF_MAX_COMMENT_LEN ) {
        adfEnv.wFct( "%s: commLen (%d) incorrect, volume '%s', block %u, entry %s",
                     __func__, ent->commLen, vol->volName, nSect, ent->name);
        //printf("nameLen=%d, commLen=%d, name=%s sector%d\n",
        //    ent->nameLen, ent->commLen, ent->name, ent->headerKey);
    }

    return ADF_RC_OK;
}


/*
 * adfWriteEntryBlock
 *
 */
ADF_RETCODE adfWriteEntryBlock( const struct AdfVolume * const      vol,
                                const ADF_SECTNUM                   nSect,
                                const struct AdfEntryBlock * const  ent )
{
    uint8_t   buf[ 512 ];
    uint32_t  newSum;

    memcpy( buf, ent, sizeof(struct AdfEntryBlock) );

#ifdef LITT_ENDIAN
    adfSwapEndian( buf, ADF_SWBL_ENTRY );
#endif
    newSum = adfNormalSum( buf, 20, sizeof(struct AdfEntryBlock) );
    swapUint32ToPtr( buf + 20, newSum );

    return adfVolWriteBlock( vol, (uint32_t) nSect, buf );
}


/*
 * adfWriteDirBlock
 *
 */
ADF_RETCODE adfWriteDirBlock( const struct AdfVolume * const  vol,
                              const ADF_SECTNUM               nSect,
                              struct AdfDirBlock * const      dir )
{
    uint8_t buf[ 512 ];
    uint32_t newSum;
    

/*printf("wdirblk=%d\n",nSect);*/
    dir->type          = ADF_T_HEADER;
    dir->highSeq       = 0;
    dir->hashTableSize = 0;
    dir->secType       = ADF_ST_DIR;

    memcpy( buf, dir, sizeof(struct AdfDirBlock) );
#ifdef LITT_ENDIAN
    adfSwapEndian( buf, ADF_SWBL_DIR );
#endif
    newSum = adfNormalSum( buf, 20, sizeof(struct AdfDirBlock) );
    swapUint32ToPtr( buf + 20, newSum );

    if ( adfVolWriteBlock( vol, (uint32_t) nSect, buf ) != ADF_RC_OK )
        return ADF_RC_ERROR;

    return ADF_RC_OK;
}


/*
 * adfEntBlock2Entry
 *
 */
ADF_RETCODE adfEntBlock2Entry( const struct AdfEntryBlock * const  entryBlk,
                               struct AdfEntry * const             entry )
{
    entry->type   = entryBlk->secType;
    entry->parent = entryBlk->parent;

    entry->name = strndup( entryBlk->name,
                           min( entryBlk->nameLen,
                                (unsigned) ADF_MAX_NAME_LEN ) );
    if ( entry->name == NULL )
        return ADF_RC_MALLOC;

/*printf("len=%d name=%s parent=%ld\n",entryBlk->nameLen, entry->name,entry->parent );*/
    adfDays2Date( entryBlk->days, &entry->year, &entry->month, &entry->days );
    entry->hour = entryBlk->mins / 60;
    entry->mins = entryBlk->mins % 60;
    entry->secs = entryBlk->ticks / 50;

    entry->access  = -1;
    entry->size    = 0L;
    entry->comment = NULL;
    entry->real    = 0L;
    switch ( entryBlk->secType ) {
    case ADF_ST_ROOT:
        break;
    case ADF_ST_DIR:
        entry->access  = entryBlk->access;
        entry->comment = strndup( entryBlk->comment,
                                  min( entryBlk->commLen,
                                       (unsigned) ADF_MAX_COMMENT_LEN ) );
        if ( entry->comment == NULL ) {
            free( entry->name );
            entry->name = NULL;
            return ADF_RC_MALLOC;
        }
        break;
    case ADF_ST_FILE:
        entry->access  = entryBlk->access;
        entry->size    = entryBlk->byteSize;
        entry->comment = strndup( entryBlk->comment,
                                  min( entryBlk->commLen,
                                       (unsigned) ADF_MAX_COMMENT_LEN ) );
        if ( entry->comment == NULL ) {
            free( entry->name );
            entry->name = NULL;
            return ADF_RC_MALLOC;
        }
        break;
    case ADF_ST_LFILE:
    case ADF_ST_LDIR:
        entry->real = entryBlk->realEntry;
    case ADF_ST_LSOFT:
        break;
    default:
        adfEnv.wFct( "%s: unknown type %u for entry '%s', sector %u",
                     __func__, entry->type, entry->name, entry->sector );
    }

    return ADF_RC_OK;
}


/*
 * adfNameToEntryBlk
 *
 */
ADF_SECTNUM adfNameToEntryBlk( struct AdfVolume * const      vol,
                               const int32_t                 ht[],
                               const char * const            name,
                               struct AdfEntryBlock * const  entry,
                               ADF_SECTNUM * const           nUpdSect )
{
    const unsigned nameLen = (unsigned) strlen( name );
    if ( nameLen > ADF_MAX_NAME_LEN ) {
        adfEnv.eFct( "%s: name '%s' is too long (%u > max. %d characters).",
                     __func__, name, nameLen, ADF_MAX_NAME_LEN );
        return -1;
    }

    uint8_t upperName[ ADF_MAX_NAME_LEN + 1 ];
    uint8_t upperName2[ ADF_MAX_NAME_LEN + 1 ];

    bool intl = adfVolHasINTL( vol ) ||
                adfVolHasDIRCACHE( vol );
    unsigned hashVal = adfGetHashValue( (uint8_t *) name, intl );
    adfStrToUpper( upperName, (uint8_t *) name, nameLen, intl );

    ADF_SECTNUM nSect = ht[ hashVal ];
/*printf("name=%s ht[%d]=%d upper=%s len=%d\n",name,hashVal,nSect,upperName,nameLen);
printf("hashVal=%u\n",adfGetHashValue(upperName, intl ));
if (!strcmp("españa.country",name)) {
for ( int i = 0 ; i < ADF_HT_SIZE ; i++ )  printf("ht[%d]=%d    ", i, ht[i]);
}*/
    if ( nSect == 0 )
        return -1;

    ADF_SECTNUM updSect = 0;
    bool found = false;
    do {
        if ( adfReadEntryBlock( vol, nSect, entry ) != ADF_RC_OK )
            return -1;
        if ( nameLen == entry->nameLen ) {
            adfStrToUpper( upperName2, (uint8_t *) entry->name, nameLen, intl );
/*printf("2=%s %s\n",upperName2,upperName);*/
            found = ( strncmp( (char *) upperName,
                               (char *) upperName2, nameLen ) == 0 );
        }
        if ( ! found ) {
            updSect = nSect;
            nSect = entry->nextSameHash;
        }
    } while ( ! found && nSect != 0 );

    if ( nSect == 0 && ! found )
        return -1;
    else {
        if ( nUpdSect != NULL )
            *nUpdSect = updSect;
        return nSect;
    }
}


/*
 * Access2String
 *
 */
static void adfAccess2String( int32_t  acc,
                              char     accStr[ 8 + 1 ] )
{
    strcpy( accStr, "----rwed" );
    if ( adfAccHasD( acc ) )  accStr[ 7 ] = '-';
    if ( adfAccHasE( acc ) )  accStr[ 6 ] = '-';
    if ( adfAccHasW( acc ) )  accStr[ 5 ] = '-';
    if ( adfAccHasR( acc ) )  accStr[ 4 ] = '-';
    if ( adfAccHasA( acc ) )  accStr[ 3 ] = 'a';
    if ( adfAccHasP( acc ) )  accStr[ 2 ] = 'p';
    if ( adfAccHasS( acc ) )  accStr[ 1 ] = 's';
    if ( adfAccHasH( acc ) )  accStr[ 0 ] = 'h';
}




/*
 * adfIntlToUpper
 *
 */
static inline uint8_t adfIntlToUpper( const uint8_t  c )
{
    return ( ( c >= 'a' && c <= 'z' ) ||
             ( c >= 224 && c <= 254 && c != 247 ) ) ? c - ('a'-'A') : c ;
}

static inline uint8_t adfToUpper( const uint8_t  c )
{
    return ( c >= 'a' && c <= 'z' ) ? c - ( 'a' - 'A' ) : c ;
}

/*
 * adfStrToUpper
 *
 */
void adfStrToUpper( uint8_t * const        nstr,
                    const uint8_t * const  ostr,
                    const unsigned         nlen,
                    const bool             intl )
{
    if ( intl )
        for ( unsigned i = 0 ; i < nlen ; i++ )
            nstr[ i ] = adfIntlToUpper( ostr[ i ] );
    else
        for ( unsigned i = 0 ; i < nlen ; i++ )
            nstr[ i ] = adfToUpper( ostr[ i ] );
    nstr[ nlen ] = '\0';
}


/*
 * adfGetHashValue
 *
 */
unsigned adfGetHashValue( const uint8_t * const  name,
                          const bool             intl )
{
    uint32_t      hash, len;
    unsigned int  i;
    uint8_t       upper;

    len = hash = (uint32_t) strlen( (const char * const) name );
    for ( i = 0; i < len; i++ ) {
        if ( intl )
            upper = adfIntlToUpper( name[ i ] );
        else
            upper = (uint8_t) toupper( name[ i ] );
        hash = ( hash * 13 + upper ) & 0x7ff;
    }
    hash = hash % ADF_HT_SIZE;

    return hash;
}


/*
 * adfEntryGetInfo
 *
 */

#define ENTRYINFO_SIZE 512

char * adfEntryGetInfo( const struct AdfEntry * const  entry )
{
    char * const info = malloc( ENTRYINFO_SIZE + 1 );
    if ( info == NULL )
        return NULL;

    char *infoptr = info;
    infoptr += snprintf( infoptr, (size_t)( ENTRYINFO_SIZE - ( infoptr - info ) ),
                         "%-30s %2d %6d "
                         "%2d/%02d/%04d %2d:%02d:%02d",
                         entry->name, entry->type, entry->sector,
                         entry->days, entry->month, entry->year,
                         entry->hour, entry->mins, entry->secs );


   if ( entry->type == ADF_ST_FILE )
       infoptr += snprintf( infoptr, (size_t)( ENTRYINFO_SIZE - ( infoptr - info ) ),
                            "%8d ",entry->size );
   else
       infoptr += snprintf( infoptr, (size_t)( ENTRYINFO_SIZE - ( infoptr - info ) ),
                            "         " );

    if ( entry->type == ADF_ST_FILE ||
         entry->type == ADF_ST_DIR )
    {
        char accessStr[ 8 + 1 ];
        adfAccess2String( entry->access, accessStr );

        infoptr += snprintf( infoptr, (size_t)( ENTRYINFO_SIZE - ( infoptr - info ) ),
                             "%-s ", accessStr );
    }

    if ( entry->comment != NULL )
        infoptr += snprintf( infoptr, (size_t)( ENTRYINFO_SIZE - ( infoptr - info ) ),
                             "%s ",entry->comment );
    infoptr += snprintf( infoptr, (size_t)( ENTRYINFO_SIZE - ( infoptr - info ) ), "\n" );

    assert( infoptr - info < ENTRYINFO_SIZE );
    return info;
}
