/*
 * Copyright 2012 Austin English
 * Copyright 2012 André Hentschel
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

#include "windef.h"
#include "winbase.h"
#include "winevt.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wevtapi);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

    return TRUE;
}

EVT_HANDLE WINAPI EvtOpenSession(EVT_LOGIN_CLASS login_class, void *login, DWORD timeout, DWORD flags)
{
    FIXME("(%u %p %u %u) stub\n", login_class, login, timeout, flags);
    return NULL;
}

EVT_HANDLE WINAPI EvtOpenLog(EVT_HANDLE session, const WCHAR *path, DWORD flags)
{
    FIXME("(%p %s %u) stub\n", session, debugstr_w(path), flags);
    return NULL;
}

BOOL WINAPI EvtGetChannelConfigProperty(EVT_HANDLE ChannelConfig,
                                        EVT_CHANNEL_CONFIG_PROPERTY_ID PropertyId,
                                        DWORD Flags,
                                        DWORD PropertyValueBufferSize,
                                        PEVT_VARIANT PropertyValueBuffer,
                                        DWORD *PropertyValueBufferUsed)
{
    FIXME("(%p %i %u %u %p %p) stub\n", ChannelConfig, PropertyId, Flags, PropertyValueBufferSize,
          PropertyValueBuffer, PropertyValueBufferUsed);
    return FALSE;
}

BOOL WINAPI EvtSetChannelConfigProperty(EVT_HANDLE ChannelConfig,
                                        EVT_CHANNEL_CONFIG_PROPERTY_ID PropertyId,
                                        DWORD Flags,
                                        PEVT_VARIANT PropertyValue)
{
    FIXME("(%p %i %u %p) stub\n", ChannelConfig, PropertyId, Flags, PropertyValue);
    return FALSE;
}

EVT_HANDLE WINAPI EvtSubscribe(EVT_HANDLE Session, HANDLE SignalEvent, LPCWSTR ChannelPath,
                               LPCWSTR Query, EVT_HANDLE Bookmark, PVOID context,
                               EVT_SUBSCRIBE_CALLBACK Callback, DWORD Flags)
{
    FIXME("(%p %p %s %s %p %p %p %u) stub\n", Session, SignalEvent, debugstr_w(ChannelPath),
          debugstr_w(Query), Bookmark, context, Callback, Flags);
    return NULL;
}

EVT_HANDLE WINAPI EvtOpenChannelEnum(EVT_HANDLE session, DWORD flags)
{
    FIXME("(%p %u) stub\n", session, flags);
    return NULL;
}

BOOL WINAPI EvtNextChannelPath(EVT_HANDLE channel_enum, DWORD buffer_len, WCHAR *buffer, DWORD *used)
{
    FIXME("(%p %u %p %p) stub\n", channel_enum, buffer_len, buffer, used);
    return FALSE;
}

EVT_HANDLE WINAPI EvtOpenChannelConfig(EVT_HANDLE Session, LPCWSTR ChannelPath, DWORD Flags)
{
    FIXME("(%p %s %u) stub\n", Session, debugstr_w(ChannelPath), Flags);
    return NULL;
}

EVT_HANDLE WINAPI EvtQuery(EVT_HANDLE session, const WCHAR *path, const WCHAR *query, DWORD flags)
{
    FIXME("(%p %s %s %u) stub\n", session, debugstr_w(path), debugstr_w(query), flags);
    return NULL;
}

BOOL WINAPI EvtClose(EVT_HANDLE handle)
{
    FIXME("(%p) stub\n", handle);
    return TRUE;
}

BOOL WINAPI EvtNext(EVT_HANDLE result_set, DWORD size, EVT_HANDLE *array, DWORD timeout, DWORD flags, DWORD *ret_count)
{
    FIXME("(%p %u %p %u %#x %p) stub!\n", result_set, size, array, timeout, flags, ret_count);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI EvtExportLog(EVT_HANDLE session, const WCHAR *path, const WCHAR *query, const WCHAR *file, DWORD flags)
{
    FIXME("(%p %s %s %s %#x) stub!\n", session, debugstr_w(path), debugstr_w(query), debugstr_w(file), flags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
