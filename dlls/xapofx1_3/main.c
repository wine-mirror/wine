/*
 * Copyright 2015 Andrey Gusev
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

#include "initguid.h"
#include "windef.h"
#include "winbase.h"
#include "compobj.h"
#include "xapofx.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xaudio2);

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(instance);
            break;
    }

    return TRUE;
}

HRESULT CDECL CreateFX(REFCLSID clsid, IUnknown **out)
{
    const GUID *class = clsid;

    TRACE("%s %p\n", debugstr_guid(clsid), out);

    if(IsEqualGUID(clsid, &CLSID_FXReverb27) ||
            IsEqualGUID(clsid, &CLSID_FXReverb))
        class = &CLSID_WINE_FXReverb13;

    return CoCreateInstance(class, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void**)out);
}
