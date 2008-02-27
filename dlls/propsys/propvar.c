/*
 * PropVariant implementation
 *
 * Copyright 2008 James Hawkins for CodeWeavers
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
#include <stdio.h>

#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "winuser.h"
#include "shlobj.h"
#include "propvarutil.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(propsys);

static HRESULT PROPVAR_ConvertFILETIME(PROPVARIANT *ppropvarDest,
                                       REFPROPVARIANT propvarSrc, VARTYPE vt)
{
    SYSTEMTIME time;

    FileTimeToSystemTime(&propvarSrc->u.filetime, &time);

    switch (vt)
    {
        case VT_LPSTR:
        {
            static const char format[] = "%04d/%02d/%02d:%02d:%02d:%02d.%03d";

            ppropvarDest->u.pszVal = HeapAlloc(GetProcessHeap(), 0,
                                             lstrlenA(format) + 1);
            if (!ppropvarDest->u.pszVal)
                return E_OUTOFMEMORY;

            sprintf(ppropvarDest->u.pszVal, format, time.wYear, time.wMonth,
                    time.wDay, time.wHour, time.wMinute,
                    time.wSecond, time.wMilliseconds);

            return S_OK;
        }

        default:
            FIXME("Unhandled target type: %d\n", vt);
    }

    return E_FAIL;
}

/******************************************************************
 *  PropVariantChangeType   (PROPSYS.@)
 */
HRESULT WINAPI PropVariantChangeType(PROPVARIANT *ppropvarDest, REFPROPVARIANT propvarSrc,
                                     PROPVAR_CHANGE_FLAGS flags, VARTYPE vt)
{
    FIXME("(%p, %p, %d, %d, %d): semi-stub!\n", ppropvarDest, propvarSrc,
          propvarSrc->vt, flags, vt);

    switch (propvarSrc->vt)
    {
        case VT_FILETIME:
            return PROPVAR_ConvertFILETIME(ppropvarDest, propvarSrc, vt);
        default:
            FIXME("Unhandled source type: %d\n", propvarSrc->vt);
    }

    return E_FAIL;
}
