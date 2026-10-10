/*
 *  adf_str.c
 *
 *  Copyright (C) 1997-2022 Laurent Clevy
 *                2023-2026 Tomasz Wolak
 *
 *  This file is part of ADFLib.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "adf_str.h"

#include "adf_env.h"

#include <stdlib.h>

/*
 * adfListNewCell
 *
 * adds a cell at the end the list
 */
struct AdfList * adfListNewCell( struct AdfList * const  list,
                                 const void * const      content )
{
    struct AdfList * const cell = ( struct AdfList * )
        malloc( sizeof ( struct AdfList ) );
    if ( ! cell ) {
        adfEnv.eFct( "%s: malloc", __func__ );
        return NULL;
    }
    cell->content = content;
    cell->next    = cell->subdir = 0;
    if ( list != NULL )
        list->next = cell;

    return cell;
}


/*
 * adfListFree
 *
 */
void adfListFree( struct AdfList * const  list )
{
    if ( list == NULL )
        return;
    
    if ( list->next )
        adfListFree( list->next );
    free( list );
}
