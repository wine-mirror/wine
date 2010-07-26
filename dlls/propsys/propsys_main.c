/*
 * propsys main
 *
 * Copyright 2008 James Hawkins
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
#include "objbase.h"
#include "propsys.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(propsys);

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
        case DLL_PROCESS_DETACH:
            break;
        default:
            break;
    }

    return TRUE;
}

HRESULT WINAPI PSRegisterPropertySchema(PCWSTR path)
{
    FIXME("%s stub\n", debugstr_w(path));

    return S_OK;
}

HRESULT WINAPI PSUnregisterPropertySchema(PCWSTR path)
{
    FIXME("%s stub\n", debugstr_w(path));

    return E_NOTIMPL;
}

HRESULT WINAPI PSStringFromPropertyKey(REFPROPERTYKEY pkey, LPWSTR psz, UINT cch)
{
    static const WCHAR guid_fmtW[] = {'{','%','0','8','X','-','%','0','4','X','-',
                                      '%','0','4','X','-','%','0','2','X','%','0','2','X','-',
                                      '%','0','2','X','%','0','2','X','%','0','2','X',
                                      '%','0','2','X','%','0','2','X','%','0','2','X','}',0};
    static const WCHAR pid_fmtW[] = {'%','u',0};

    WCHAR pidW[PKEY_PIDSTR_MAX + 1];
    LPWSTR p = psz;
    int len;

    TRACE("(%p, %p, %u)\n", pkey, psz, cch);

    if (!psz)
        return E_POINTER;

    /* GUIDSTRING_MAX accounts for null terminator, +1 for space character. */
    if (cch <= GUIDSTRING_MAX + 1)
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

    if (!pkey)
    {
        psz[0] = '\0';
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    sprintfW(psz, guid_fmtW, pkey->fmtid.Data1, pkey->fmtid.Data2,
             pkey->fmtid.Data3, pkey->fmtid.Data4[0], pkey->fmtid.Data4[1],
             pkey->fmtid.Data4[2], pkey->fmtid.Data4[3], pkey->fmtid.Data4[4],
             pkey->fmtid.Data4[5], pkey->fmtid.Data4[6], pkey->fmtid.Data4[7]);

    /* Overwrite the null terminator with the space character. */
    p += GUIDSTRING_MAX - 1;
    *p++ = ' ';
    cch -= GUIDSTRING_MAX - 1 + 1;

    len = sprintfW(pidW, pid_fmtW, pkey->pid);

    if (cch >= len + 1)
    {
        strcpyW(p, pidW);
        return S_OK;
    }
    else
    {
        WCHAR *ptr = pidW + len - 1;

        psz[0] = '\0';
        *p++ = '\0';
        cch--;

        /* Replicate a quirk of the native implementation where the contents
         * of the property ID string are written backwards to the output
         * buffer, skipping the rightmost digit. */
        if (cch)
        {
            ptr--;
            while (cch--)
                *p++ = *ptr--;
        }

        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }
}
