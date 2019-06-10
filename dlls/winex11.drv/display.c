/*
 * X11DRV display device functions
 *
 * Copyright 2019 Zhiyi Zhang for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "wine/debug.h"
#include "x11drv.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

static const WCHAR video_keyW[] = {
    'H','A','R','D','W','A','R','E','\\',
    'D','E','V','I','C','E','M','A','P','\\',
    'V','I','D','E','O',0};

static struct x11drv_display_device_handler handler;

void X11DRV_DisplayDevices_SetHandler(const struct x11drv_display_device_handler *new_handler)
{
    if (new_handler->priority > handler.priority)
    {
        handler = *new_handler;
        TRACE("Display device functions are now handled by: %s\n", handler.name);
    }
}

void X11DRV_DisplayDevices_Init(void)
{
    static const WCHAR init_mutexW[] = {'d','i','s','p','l','a','y','_','d','e','v','i','c','e','_','i','n','i','t',0};
    HANDLE mutex;
    HKEY video_hkey = NULL;
    DWORD disposition = 0;

    mutex = CreateMutexW(NULL, FALSE, init_mutexW);
    WaitForSingleObject(mutex, INFINITE);

    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, video_keyW, 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &video_hkey,
                        &disposition))
    {
        ERR("Failed to create video device key\n");
        goto done;
    }

    /* Avoid unnecessary reinit */
    if (disposition != REG_CREATED_NEW_KEY)
        goto done;

    TRACE("via %s\n", wine_dbgstr_a(handler.name));

done:
    RegCloseKey(video_hkey);
    ReleaseMutex(mutex);
    CloseHandle(mutex);
}
