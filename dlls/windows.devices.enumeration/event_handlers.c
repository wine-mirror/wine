/* WinRT Windows.Devices.Enumeration implementation
 *
 * Copyright 2022 Bernhard Kölbl
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

#include "private.h"

static CRITICAL_SECTION handlers_cs;
static CRITICAL_SECTION_DEBUG handlers_cs_debug =
{
    0, 0, &handlers_cs,
    { &handlers_cs_debug.ProcessLocksList, &handlers_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": handlers_cs") }
};
static CRITICAL_SECTION handlers_cs = { &handlers_cs_debug, -1, 0, 0, 0, 0 };
static EventRegistrationToken next_token = {.value = 1};

struct typed_event_handler_entry
{
    struct list entry;
    EventRegistrationToken token;
    ITypedEventHandler_IInspectable_IInspectable *handler;
};

static struct typed_event_handler_entry *typed_event_handler_entry_create( ITypedEventHandler_IInspectable_IInspectable *handler )
{
    struct typed_event_handler_entry *entry;
    if (!(entry = calloc( 1, sizeof(*entry) ))) return NULL;
    ITypedEventHandler_IInspectable_IInspectable_AddRef( (entry->handler = handler) );
    return entry;
}

static void typed_event_handler_entry_destroy( struct typed_event_handler_entry *entry )
{
    ITypedEventHandler_IInspectable_IInspectable_Release( entry->handler );
    free( entry );
}

HRESULT typed_event_handlers_append( struct list *list, ITypedEventHandler_IInspectable_IInspectable *handler, EventRegistrationToken *token )
{
    struct typed_event_handler_entry *entry;

    if (!(entry = calloc( 1, sizeof(*entry) ))) return E_OUTOFMEMORY;
    ITypedEventHandler_IInspectable_IInspectable_AddRef( (entry->handler = handler) );

    EnterCriticalSection( &handlers_cs );

    *token = entry->token = next_token;
    next_token.value++;
    list_add_tail( list, &entry->entry );

    LeaveCriticalSection( &handlers_cs );

    return S_OK;
}

HRESULT typed_event_handlers_remove( struct list *list, EventRegistrationToken *token )
{
    struct typed_event_handler_entry *entry;
    BOOL found = FALSE;

    EnterCriticalSection( &handlers_cs );

    LIST_FOR_EACH_ENTRY( entry, list, struct typed_event_handler_entry, entry )
        if ((found = !memcmp( &entry->token, token, sizeof(*token) ))) break;
    if (found) list_remove( &entry->entry );

    LeaveCriticalSection( &handlers_cs );

    if (found)
    {
        ITypedEventHandler_IInspectable_IInspectable_Release( entry->handler );
        free( entry );
    }

    return S_OK;
}

HRESULT typed_event_handlers_notify( struct list *list, IInspectable *sender, IInspectable *args )
{
    struct typed_event_handler_entry *entry, *next, *copy;
    struct list copies = LIST_INIT(copies);

    EnterCriticalSection( &handlers_cs );

    LIST_FOR_EACH_ENTRY( entry, list, struct typed_event_handler_entry, entry )
    {
        if (!(copy = typed_event_handler_entry_create( entry->handler ))) continue;
        list_add_tail( &copies, &copy->entry );
    }

    LeaveCriticalSection( &handlers_cs );

    LIST_FOR_EACH_ENTRY_SAFE( entry, next, &copies, struct typed_event_handler_entry, entry )
    {
        ITypedEventHandler_IInspectable_IInspectable_Invoke( entry->handler, sender, args );
        list_remove( &entry->entry );
        typed_event_handler_entry_destroy( entry );
    }

    return S_OK;
}

HRESULT typed_event_handlers_clear( struct list *list )
{
    struct typed_event_handler_entry *entry, *entry_cursor2;

    EnterCriticalSection( &handlers_cs );

    LIST_FOR_EACH_ENTRY_SAFE( entry, entry_cursor2, list, struct typed_event_handler_entry, entry )
    {
        list_remove( &entry->entry );
        ITypedEventHandler_IInspectable_IInspectable_Release( entry->handler );
        free( entry );
    }

    LeaveCriticalSection( &handlers_cs );

    return S_OK;
}
