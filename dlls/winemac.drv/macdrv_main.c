/*
 * MACDRV initialization code
 *
 * Copyright 1998 Patrik Stridvall
 * Copyright 2000 Alexandre Julliard
 * Copyright 2011, 2012, 2013 Ken Thomases for CodeWeavers Inc.
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

#include <Security/AuthSession.h>

#include "macdrv.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(macdrv);


DWORD thread_data_tls_index = TLS_OUT_OF_INDEXES;


/**************************************************************************
 *              debugstr_cf
 */
const char* debugstr_cf(CFTypeRef t)
{
    CFStringRef s;
    const char* ret;

    if (!t) return "(null)";

    if (CFGetTypeID(t) == CFStringGetTypeID())
        s = t;
    else
        s = CFCopyDescription(t);
    ret = CFStringGetCStringPtr(s, kCFStringEncodingUTF8);
    if (ret) ret = debugstr_a(ret);
    if (!ret)
    {
        const UniChar* u = CFStringGetCharactersPtr(s);
        if (u)
            ret = debugstr_wn((const WCHAR*)u, CFStringGetLength(s));
    }
    if (!ret)
    {
        UniChar buf[200];
        int len = min(CFStringGetLength(s), sizeof(buf)/sizeof(buf[0]));
        CFStringGetCharacters(s, CFRangeMake(0, len), buf);
        ret = debugstr_wn(buf, len);
    }
    if (s != t) CFRelease(s);
    return ret;
}


/***********************************************************************
 *              process_attach
 */
static BOOL process_attach(void)
{
    SessionAttributeBits attributes;
    OSStatus status;

    assert(NUM_EVENT_TYPES <= sizeof(macdrv_event_mask) * 8);

    status = SessionGetInfo(callerSecuritySession, NULL, &attributes);
    if (status != noErr || !(attributes & sessionHasGraphicAccess))
        return FALSE;

    if ((thread_data_tls_index = TlsAlloc()) == TLS_OUT_OF_INDEXES) return FALSE;

    macdrv_err_on = ERR_ON(macdrv);
    if (macdrv_start_cocoa_app(GetTickCount64()))
    {
        ERR("Failed to start Cocoa app main loop\n");
        return FALSE;
    }

    macdrv_clipboard_process_attach();

    return TRUE;
}


/***********************************************************************
 *              thread_detach
 */
static void thread_detach(void)
{
    struct macdrv_thread_data *data = macdrv_thread_data();

    if (data)
    {
        macdrv_destroy_event_queue(data->queue);
        if (data->keyboard_layout_uchr)
            CFRelease(data->keyboard_layout_uchr);
        HeapFree(GetProcessHeap(), 0, data);
    }
}


/***********************************************************************
 *              set_queue_display_fd
 *
 * Store the event queue fd into the message queue
 */
static void set_queue_display_fd(int fd)
{
    HANDLE handle;
    int ret;

    if (wine_server_fd_to_handle(fd, GENERIC_READ | SYNCHRONIZE, 0, &handle))
    {
        MESSAGE("macdrv: Can't allocate handle for event queue fd\n");
        ExitProcess(1);
    }
    SERVER_START_REQ(set_queue_fd)
    {
        req->handle = wine_server_obj_handle(handle);
        ret = wine_server_call(req);
    }
    SERVER_END_REQ;
    if (ret)
    {
        MESSAGE("macdrv: Can't store handle for event queue fd\n");
        ExitProcess(1);
    }
    CloseHandle(handle);
}


/***********************************************************************
 *              macdrv_init_thread_data
 */
struct macdrv_thread_data *macdrv_init_thread_data(void)
{
    struct macdrv_thread_data *data = macdrv_thread_data();

    if (data) return data;

    if (!(data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*data))))
    {
        ERR("could not create data\n");
        ExitProcess(1);
    }

    if (!(data->queue = macdrv_create_event_queue(macdrv_handle_event)))
    {
        ERR("macdrv: Can't create event queue.\n");
        ExitProcess(1);
    }

    data->keyboard_layout_uchr = macdrv_copy_keyboard_layout(&data->keyboard_type, &data->iso_keyboard);
    macdrv_compute_keyboard_layout(data);

    set_queue_display_fd(macdrv_get_event_queue_fd(data->queue));
    TlsSetValue(thread_data_tls_index, data);

    return data;
}


/***********************************************************************
 *              DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    BOOL ret = TRUE;

    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        ret = process_attach();
        break;
    case DLL_THREAD_DETACH:
        thread_detach();
        break;
    }
    return ret;
}
