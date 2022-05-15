/*
 * Raw Input
 *
 * Copyright 2012 Henri Verbeet
 * Copyright 2018 Zebediah Figura for CodeWeavers
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winioctl.h"
#include "winnls.h"
#include "winreg.h"
#include "winuser.h"
#include "ddk/hidclass.h"
#include "wine/debug.h"
#include "wine/server.h"
#include "wine/hid.h"

#include "user_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(rawinput);

/***********************************************************************
 *              GetRawInputDeviceInfoA   (USER32.@)
 */
UINT WINAPI GetRawInputDeviceInfoA(HANDLE device, UINT command, void *data, UINT *data_size)
{
    TRACE("device %p, command %#x, data %p, data_size %p.\n",
            device, command, data, data_size);

    /* RIDI_DEVICENAME data_size is in chars, not bytes */
    if (command == RIDI_DEVICENAME)
    {
        WCHAR *nameW;
        UINT ret, nameW_sz;

        if (!data_size) return ~0U;

        nameW_sz = *data_size;

        if (data && nameW_sz > 0)
            nameW = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * nameW_sz);
        else
            nameW = NULL;

        ret = NtUserGetRawInputDeviceInfo( device, command, nameW, &nameW_sz );

        if (ret && ret != ~0U)
            WideCharToMultiByte(CP_ACP, 0, nameW, -1, data, *data_size, NULL, NULL);

        *data_size = nameW_sz;

        HeapFree(GetProcessHeap(), 0, nameW);

        return ret;
    }

    return NtUserGetRawInputDeviceInfo( device, command, data, data_size );
}

/***********************************************************************
 *              DefRawInputProc   (USER32.@)
 */
LRESULT WINAPI DefRawInputProc(RAWINPUT **data, INT data_count, UINT header_size)
{
    FIXME("data %p, data_count %d, header_size %u stub!\n", data, data_count, header_size);

    return 0;
}
