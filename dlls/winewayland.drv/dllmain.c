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

#include "ntuser.h"
#include "winuser.h"

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

static LRESULT CALLBACK clipboard_wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_NCCREATE:
    case WM_CLIPBOARDUPDATE:
    case WM_RENDERFORMAT:
    case WM_TIMER:
    case WM_DESTROYCLIPBOARD:
    case WM_USER:
        return NtUserMessageCall(hwnd, msg, wp, lp, 0, NtUserClipboardWindowProc, FALSE);
    }

    return DefWindowProcW(hwnd, msg, wp, lp);
}

static DWORD WINAPI clipboard_thread(void *arg)
{
    static const WCHAR clipboard_classname[] = L"__winewayland_clipboard_manager";
    WNDCLASSW class;
    ATOM atom;
    MSG msg;
    HWND hwnd;

    memset(&class, 0, sizeof(class));
    class.lpfnWndProc = clipboard_wndproc;
    class.lpszClassName = clipboard_classname;

    if (!(atom = RegisterClassW(&class)) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        ERR("could not register clipboard window class err %lu\n", GetLastError());
        return 0;
    }
    /* The HWND_MESSAGE parent window may not have been created yet. It will be
     * created eventually, so keep trying. */
    while (!(hwnd = CreateWindowW(clipboard_classname, NULL, 0, 0, 0, 0, 0,
                                  HWND_MESSAGE, 0, 0, NULL)) &&
           GetLastError() == ERROR_INVALID_WINDOW_HANDLE)
    {
        SwitchToThread();
    }

    if (!hwnd)
    {
        TRACE("failed to create clipboard window err %lu\n", GetLastError());
        UnregisterClassW(MAKEINTRESOURCEW(atom), NULL);
        return 0;
    }

    TRACE("created per-process clipboard window hwnd=%p\n", hwnd);

    while (GetMessageW(&msg, 0, 0, 0)) DispatchMessageW(&msg);
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
    /* Handle clipboard events in a dedicated thread, if needed. */
    if (!WAYLANDDRV_UNIX_CALL(init_clipboard, NULL))
        CloseHandle(CreateThread(NULL, 0, clipboard_thread, NULL, 0, &tid));

    return TRUE;
}
