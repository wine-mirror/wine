/*
 * Server-side USER handles
 *
 * Copyright (C) 2001 Alexandre Julliard
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "thread.h"
#include "file.h"
#include "user.h"
#include "file.h"
#include "process.h"
#include "request.h"

typedef volatile struct user_entry user_entry_t;

static void *server_objects[MAX_USER_HANDLES];
static mem_size_t freelist = -1;
static int nb_handles;

static void *get_server_object( const user_entry_t *entry )
{
    const user_entry_t *handles = shared_session->user_entries;
    return server_objects[entry - handles];
}

static void *set_server_object( const user_entry_t *entry, void *ptr )
{
    const user_entry_t *handles = shared_session->user_entries;
    void *prev = server_objects[entry - handles];
    server_objects[entry - handles] = ptr;
    return prev;
}

static user_entry_t *handle_to_entry( user_handle_t handle )
{
    user_entry_t *entry, *handles = shared_session->user_entries;
    unsigned short generation = handle >> 16;
    int index;

    index = ((handle & 0xffff) - FIRST_USER_HANDLE) >> 1;
    if (index < 0 || index >= nb_handles) return NULL;
    entry = handles + index;

    if (!entry->type) return NULL;
    if (!generation || generation == 0xffff) return entry;
    if (generation == entry->generation) return entry;
    return NULL;
}

static user_handle_t entry_to_handle( const user_entry_t *entry )
{
    const user_entry_t *handles = shared_session->user_entries;
    unsigned int index = entry - handles;
    return (index << 1) + FIRST_USER_HANDLE + (entry->generation << 16);
}

static const user_entry_t *alloc_user_entry( struct obj_locator locator, unsigned short type )
{
    user_entry_t *entry, *handles = shared_session->user_entries;
    unsigned short generation;

    if (freelist != -1)
    {
        entry = handles + freelist;
        generation = entry->generation + 1;
        freelist = entry->offset;
    }
    else
    {
        entry = handles + nb_handles;
        generation = 1;
        nb_handles++;
    }

    if (generation == 0 || generation == 0xffff) generation = 1;

    entry->offset = locator.offset;
    entry->tid = get_thread_id( current );
    entry->pid = get_process_id( current->process );
    entry->id = locator.id;
    WriteRelease64( &entry->uniq, MAKELONG(type, generation) );
    return entry;
}

static void free_user_entry( user_entry_t *entry )
{
    user_entry_t *handles = shared_session->user_entries;
    size_t index = entry - handles;

    WriteRelease64( &entry->uniq, MAKELONG(0, entry->generation) );
    entry->offset = freelist;
    freelist = index;
}

/* allocate a user handle for a given object */
user_handle_t alloc_user_handle( void *ptr, volatile void *shared, unsigned short type )
{
    struct obj_locator locator = {0};
    const user_entry_t *entry;

    if (shared) locator = get_shared_object_locator( shared );
    if (!(entry = alloc_user_entry( locator, type ))) return 0;
    set_server_object( entry, ptr );
    return entry_to_handle( entry );
}

/* return a pointer to a user object from its handle */
void *get_user_object( user_handle_t handle, unsigned short type )
{
    const user_entry_t *entry;

    if (!(entry = handle_to_entry( handle )) || entry->type != type) return NULL;
    return get_server_object( entry );
}

/* get the full handle for a possibly truncated handle */
user_handle_t get_user_full_handle( user_handle_t handle )
{
    const user_entry_t *entry;

    if (handle >> 16) return handle;
    if (!(entry = handle_to_entry( handle ))) return handle;
    return entry_to_handle( entry );
}

/* same as get_user_object plus set the handle to the full 32-bit value */
void *get_user_object_handle( user_handle_t *handle, unsigned short type )
{
    const user_entry_t *entry;

    if (!(entry = handle_to_entry( *handle )) || entry->type != type) return NULL;
    *handle = entry_to_handle( entry );
    return get_server_object( entry );
}

/* free a user handle and return a pointer to the object */
void *free_user_handle( user_handle_t handle )
{
    user_entry_t *entry;
    void *ret;

    if (!(entry = handle_to_entry( handle )))
    {
        set_error( STATUS_INVALID_HANDLE );
        return NULL;
    }

    ret = set_server_object( entry, NULL );
    free_user_entry( entry );
    return ret;
}

/* return the next user handle after 'handle' that is of a given type */
void *next_user_handle( user_handle_t *handle, unsigned short type )
{
    const user_entry_t *entry, *handles = shared_session->user_entries;

    if (!*handle) entry = handles;
    else
    {
        int index = ((*handle & 0xffff) - FIRST_USER_HANDLE) >> 1;
        if (index < 0 || index >= nb_handles) return NULL;
        entry = handles + index + 1;  /* start from the next one */
    }
    while (entry < handles + nb_handles)
    {
        if (!type || entry->type == type)
        {
            *handle = entry_to_handle( entry );
            return get_server_object( entry );
        }
        entry++;
    }
    return NULL;
}

/* free client-side user handles managed by the process */
void free_process_user_handles( struct process *process )
{
    user_entry_t *handles = shared_session->user_entries;
    unsigned int i, pid = get_process_id( process );

    for (i = 0; i < nb_handles; i++)
    {
        user_entry_t *entry = handles + i;
        switch (entry->type)
        {
        case NTUSER_OBJ_MENU:
        case NTUSER_OBJ_ICON:
        case NTUSER_OBJ_WINPOS:
        case NTUSER_OBJ_ACCEL:
        case NTUSER_OBJ_IMC:
            if (entry->pid == pid) free_user_entry( entry );
            break;
        case NTUSER_OBJ_HOOK:
        case NTUSER_OBJ_WINDOW:
        default:
            continue;
        }
    }
}

/* allocate an arbitrary user handle */
DECL_HANDLER(alloc_user_handle)
{
    reply->handle = alloc_user_handle( (void *)-1 /* never used */, NULL, req->type );
}


/* free an arbitrary user handle */
DECL_HANDLER(free_user_handle)
{
    user_entry_t *entry;

    if ((entry = handle_to_entry( req->handle )) && entry->type == req->type)
        free_user_entry( entry );
    else
        set_error( STATUS_INVALID_HANDLE );
}
