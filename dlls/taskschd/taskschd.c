/*
 * Task Scheduler
 *
 * Copyright 2013 Dmitry Timoshkov
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

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(taskschd);

/******************************************************************
 *      DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE; /* prefer native version */

    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinst);
        break;
    }
    return TRUE;
}

const char *debugstr_variant(const VARIANT *v)
{
    if (!v) return "(null)";

    switch (V_VT(v))
    {
    case VT_EMPTY:
        return "{VT_EMPTY}";
    case VT_NULL:
        return "{VT_NULL}";
    case VT_I2:
        return wine_dbg_sprintf("{VT_I2: %d}", V_I2(v));
    case VT_I4:
        return wine_dbg_sprintf("{VT_I4: %d}", V_I4(v));
    case VT_R8:
        return wine_dbg_sprintf("{VT_R8: %lf}", V_R8(v));
    case VT_BSTR:
        return wine_dbg_sprintf("{VT_BSTR: %s}", debugstr_w(V_BSTR(v)));
    case VT_DISPATCH:
        return wine_dbg_sprintf("{VT_DISPATCH: %p}", V_DISPATCH(v));
    case VT_ERROR:
        return wine_dbg_sprintf("{VT_ERROR: %08x}", V_ERROR(v));
    case VT_BOOL:
        return wine_dbg_sprintf("{VT_BOOL: %x}", V_BOOL(v));
    case VT_UINT:
        return wine_dbg_sprintf("{VT_UINT: %u}", V_UINT(v));
    default:
        return wine_dbg_sprintf("{vt %d}", V_VT(v));
    }
}
