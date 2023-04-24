/*
 * winewayland.drv entry points
 *
 * Copyright 2022 Alexandros Frantzis for Collabora Ltd
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

#include "waylanddrv_dll.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(waylanddrv);

static DWORD WINAPI wayland_read_events_thread(void *arg)
{
    WAYLANDDRV_UNIX_CALL(read_events, NULL);
    /* This thread terminates only if an unrecoverable error occurred
     * during event reading (e.g., the connection to the Wayland
     * compositor is broken). */
    ERR("Failed to read events from the compositor, terminating process\n");
    TerminateProcess(GetCurrentProcess(), 1);
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    DWORD tid;

    if (reason != DLL_PROCESS_ATTACH) return TRUE;

    DisableThreadLibraryCalls(instance);
    if (__wine_init_unix_call()) return FALSE;

    if (WAYLANDDRV_UNIX_CALL(init, NULL))
        return FALSE;

    /* Read wayland events from a dedicated thread. */
    CloseHandle(CreateThread(NULL, 0, wayland_read_events_thread, NULL, 0, &tid));

    return TRUE;
}
