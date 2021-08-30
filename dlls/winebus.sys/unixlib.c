/*
 * Copyright 2021 Rémi Bernon for CodeWeavers
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

#include "config.h"

#include <stdarg.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "wine/unixlib.h"

#include "unix_private.h"

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    sdl_bus_init,
    sdl_bus_wait,
    sdl_bus_stop,
    udev_bus_init,
    udev_bus_wait,
    udev_bus_stop,
    iohid_bus_init,
    iohid_bus_wait,
    iohid_bus_stop,
};

void bus_event_queue_destroy(struct list *queue)
{
    struct bus_event *event, *next;

    LIST_FOR_EACH_ENTRY_SAFE(event, next, queue, struct bus_event, entry)
        HeapFree(GetProcessHeap(), 0, event);
}

BOOL bus_event_queue_device_removed(struct list *queue, const WCHAR *bus_id, void *context)
{
    ULONG size = sizeof(struct bus_event);
    struct bus_event *event = HeapAlloc(GetProcessHeap(), 0, size);
    if (!event) return FALSE;

    event->type = BUS_EVENT_TYPE_DEVICE_REMOVED;
    event->device_removed.bus_id = bus_id;
    event->device_removed.context = context;
    list_add_tail(queue, &event->entry);

    return TRUE;
}

BOOL bus_event_queue_pop(struct list *queue, struct bus_event *event)
{
    struct list *entry = list_head(queue);
    struct bus_event *tmp;

    if (!entry) return FALSE;

    tmp = LIST_ENTRY(entry, struct bus_event, entry);
    list_remove(entry);

    memcpy(event, tmp, sizeof(*event));
    HeapFree(GetProcessHeap(), 0, tmp);

    return TRUE;
}
