/*
 * Copyright (C) 2006 Dmitry Timoshkov
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "ntdsapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdsapi);

/*****************************************************
 *      DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    TRACE("(%p, %ld, %p)\n", hinst, reason, reserved);

    switch(reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */

    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinst );
        break;
    }
    return TRUE;
}

DWORD WINAPI DsMakeSpnW(LPCWSTR svc_class, LPCWSTR svc_name,
                        LPCWSTR inst_name, USHORT inst_port,
                        LPCWSTR ref, DWORD *spn_length, LPWSTR spn)
{
    FIXME("(%s,%s,%s,%d,%s,%p,%p): stub!\n", debugstr_w(svc_class),
            debugstr_w(svc_name), debugstr_w(inst_name), inst_port,
            debugstr_w(ref), spn_length, spn);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI DsMakeSpnA(LPCSTR svc_class, LPCSTR svc_name,
                        LPCSTR inst_name, USHORT inst_port,
                        LPCSTR ref, DWORD *spn_length, LPSTR spn)
{
    FIXME("(%s,%s,%s,%d,%s,%p,%p): stub!\n", debugstr_a(svc_class),
            debugstr_a(svc_name), debugstr_a(inst_name), inst_port,
            debugstr_a(ref), spn_length, spn);

    return ERROR_CALL_NOT_IMPLEMENTED;
}
