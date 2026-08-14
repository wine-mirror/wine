/*
 * MSCMS - Color Management System for Wine
 *
 * Copyright 2004, 2005, 2008 Hans Leidekker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "icm.h"
#include "wine/debug.h"

#include "mscms_priv.h"

WINE_DEFAULT_DEBUG_CHANNEL(mscms);

static CRITICAL_SECTION mscms_handle_cs;
static CRITICAL_SECTION_DEBUG mscms_handle_cs_debug =
{
    0, 0, &mscms_handle_cs,
    { &mscms_handle_cs_debug.ProcessLocksList,
      &mscms_handle_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": mscms_handle_cs") }
};
static CRITICAL_SECTION mscms_handle_cs = { &mscms_handle_cs_debug, -1, 0, 0, 0, 0 };

static struct object **handle_table;
static ULONG_PTR next_handle;
static ULONG_PTR max_handles;

struct object *grab_object( HANDLE handle, enum object_type type )
{
    struct object *obj = NULL;
    ULONG_PTR index = (ULONG_PTR)handle;

    EnterCriticalSection( &mscms_handle_cs );
    if (index > 0 && index <= max_handles)
    {
        index--;
        if (handle_table[index] && handle_table[index]->type == type)
        {
            obj = handle_table[index];
            InterlockedIncrement( &obj->refs );
        }
    }
    LeaveCriticalSection( &mscms_handle_cs );

    TRACE( "handle %p -> %p\n", handle, obj );
    return obj;
}

void release_object( struct object *obj )
{
    ULONG refs = InterlockedDecrement( &obj->refs );
    if (!refs)
    {
        if (obj->close) obj->close( obj );
        TRACE( "destroying object %p\n", obj );
        free( obj );
    }
}

#define HANDLE_TABLE_SIZE 4
HANDLE alloc_handle( struct object *obj )
{
    struct object **ptr;
    ULONG_PTR index, count;

    EnterCriticalSection( &mscms_handle_cs );
    if (!max_handles)
    {
        count = HANDLE_TABLE_SIZE;
        if (!(ptr = calloc( 1, sizeof(*ptr) * count )))
        {
            LeaveCriticalSection( &mscms_handle_cs );
            return 0;
        }
        handle_table = ptr;
        max_handles = count;
    }
    if (max_handles == next_handle)
    {
        size_t new_size, old_size = max_handles * sizeof(*ptr);
        count = max_handles * 2;
        new_size = count * sizeof(*ptr);
        if (!(ptr = realloc( handle_table, new_size )))
        {
            LeaveCriticalSection( &mscms_handle_cs );
            return 0;
        }
        memset( (char *)ptr + old_size, 0, new_size - old_size );
        handle_table = ptr;
        max_handles = count;
    }
    index = next_handle;
    if (handle_table[index]) ERR( "handle isn't free but should be\n" );

    handle_table[index] = obj;
    InterlockedIncrement( &obj->refs );
    while (next_handle < max_handles && handle_table[next_handle]) next_handle++;

    LeaveCriticalSection( &mscms_handle_cs );
    TRACE( "object %p -> %Ix\n", obj, index + 1 );
    return (HANDLE)(index + 1);
}

void free_handle( HANDLE handle )
{
    struct object *obj = NULL;
    ULONG_PTR index = (ULONG_PTR)handle;

    EnterCriticalSection( &mscms_handle_cs );
    if (index > 0 && index <= max_handles)
    {
        index--;
        if (handle_table[index])
        {
            obj = handle_table[index];
            TRACE( "destroying handle %p for object %p\n", handle, obj );
            handle_table[index] = NULL;
        }
    }
    LeaveCriticalSection( &mscms_handle_cs );

    if (obj) release_object( obj );

    EnterCriticalSection( &mscms_handle_cs );
    if (next_handle > index && !handle_table[index]) next_handle = index;
    LeaveCriticalSection( &mscms_handle_cs );
}
