/*
 * Copyright 2011 André Hentschel
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
#include <appcompatapi.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(apphelp);

/* FIXME: don't know where to place that enum */
typedef enum _PATH_TYPE {
    DOS_PATH,
    NT_PATH
} PATH_TYPE;

/* FIXME: don't know where to place that typedef */
typedef HANDLE PDB;
typedef HANDLE HSDB;

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    TRACE("%p, %u, %p\n", hinst, reason, reserved);

    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls( hinst );
            break;
    }
    return TRUE;
}

BOOL WINAPI ApphelpCheckInstallShieldPackage( void* ptr, LPCWSTR path )
{
    FIXME("stub: %p %s\n", ptr, debugstr_w(path));
    return TRUE;
}

BOOL WINAPI ApphelpCheckMsiPackage( void* ptr, LPCWSTR path )
{
    FIXME("stub: %p %s\n", ptr, debugstr_w(path));
    return TRUE;
}

PDB WINAPI SdbCreateDatabase( LPCWSTR path, PATH_TYPE type )
{
    FIXME("stub: %s %u\n", debugstr_w(path), type);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}

BOOL WINAPI ApphelpCheckShellObject( REFCLSID clsid, BOOL shim, ULONGLONG *flags )
{
    TRACE("(%s, %d, %p)\n", debugstr_guid(clsid), shim, flags);
    if (flags) *flags = 0;
    return TRUE;
}

BOOL WINAPI ShimFlushCache( HWND hwnd, HINSTANCE instance, LPCSTR cmdline, int cmd )
{
    FIXME("stub: %p %p %s %u\n", hwnd, instance, debugstr_a(cmdline), cmd);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return TRUE;
}


HSDB WINAPI SdbInitDatabase(DWORD flags, LPCWSTR path)
{
    FIXME("stub: %08x %s\n", flags, debugstr_w(path));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}
