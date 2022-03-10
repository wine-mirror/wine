/* WinRT Windows.Gaming.Input implementation
 *
 * Copyright 2022 Rémi Bernon for CodeWeavers
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

struct handler_entry
{
    struct list entry;
    EventRegistrationToken token;
    IEventHandler_IInspectable *handler;
};

HRESULT event_handlers_append( struct list *list, IEventHandler_IInspectable *handler, EventRegistrationToken *token )
{
    struct handler_entry *entry;

    if (!(entry = calloc( 1, sizeof(*entry) ))) return E_OUTOFMEMORY;
    IEventHandler_IInspectable_AddRef( (entry->handler = handler) );

    EnterCriticalSection( &handlers_cs );

    *token = entry->token = next_token;
    next_token.value++;
    list_add_tail( list, &entry->entry );

    LeaveCriticalSection( &handlers_cs );

    return S_OK;
}

HRESULT event_handlers_remove( struct list *list, EventRegistrationToken *token )
{
    struct handler_entry *entry;
    BOOL found = FALSE;

    EnterCriticalSection( &handlers_cs );

    LIST_FOR_EACH_ENTRY( entry, list, struct handler_entry, entry )
        if ((found = !memcmp( &entry->token, token, sizeof(*token) ))) break;
    if (found) list_remove( &entry->entry );

    LeaveCriticalSection( &handlers_cs );

    if (found)
    {
        IEventHandler_IInspectable_Release( entry->handler );
        free( entry );
    }

    return S_OK;
}

void event_handlers_notify( struct list *list, IInspectable *element )
{
    struct handler_entry *entry;

    EnterCriticalSection( &handlers_cs );

    LIST_FOR_EACH_ENTRY( entry, list, struct handler_entry, entry )
        IEventHandler_IInspectable_Invoke( entry->handler, NULL, element );

    LeaveCriticalSection( &handlers_cs );
}
