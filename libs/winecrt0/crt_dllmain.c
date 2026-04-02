/*
 * DllMainCRTStartup default entry point
 *
 * Copyright 2019 Jacek Caban for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * In addition to the permissions in the GNU Lesser General Public License,
 * the authors give you unlimited permission to link the compiled version
 * of this file with other programs, and to distribute those programs
 * without any restriction coming from the use of this file.  (The GNU
 * Lesser General Public License restrictions do apply in other respects;
 * for example, they cover modification of the file, and distribution when
 * not linked into another program.)
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

#ifdef __WINE_PE_BUILD

#include <stdarg.h>
#include <stdio.h>
#include <process.h>
#include "windef.h"
#include "winbase.h"

extern _PIFV __xi_a[];
extern _PIFV __xi_z[];
extern _PVFV __xc_a[];
extern _PVFV __xc_z[];
extern _PVFV __xt_a[];
extern _PVFV __xt_z[];

static void fallback_initterm( _PVFV *table,_PVFV *end )
{
    for ( ; table < end; table++)
        if (*table) (*table)();
}

static int fallback_initterm_e( _PIFV *table, _PIFV *end )
{
    int res;
    for (res = 0; !res && table < end; table++)
        if (*table) res = (*table)();
    return res;
}

BOOL WINAPI DllMainCRTStartup( HINSTANCE inst, DWORD reason, void *reserved )
{
    BOOL ret;

    if (reason == DLL_PROCESS_ATTACH)
    {
        if (fallback_initterm_e( __xi_a, __xi_z ) != 0) return FALSE;
        fallback_initterm( __xc_a, __xc_z );
    }

    ret = DllMain( inst, reason, reserved );

    if (reason == DLL_PROCESS_DETACH) fallback_initterm( __xt_a, __xt_z );
    return ret;
}

#endif
