/*
 * Copyright 2002 Dmitry Timoshkov for Codeweavers
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

#include "winbase.h"
#include "ntdef.h"
#include "winnt.h"
#include "ntddk.h"

#include "module.h"
#include "wine/debug.h"


WINE_DEFAULT_DEBUG_CHANNEL(ntdll);

NTSTATUS WINAPI LdrDisableThreadCalloutsForDll(HANDLE hModule)
{
    if (DisableThreadLibraryCalls(hModule))
	return STATUS_SUCCESS;
    else
	return STATUS_DLL_NOT_FOUND;
}

/* FIXME : MODULE_FindModule should depend on LdrGetDllHandle, not vice-versa */

NTSTATUS WINAPI LdrGetDllHandle(ULONG x, LONG y, PUNICODE_STRING name, PVOID *base)
{
    STRING str;
    WINE_MODREF *wm;

    FIXME("%08lx %08lx %s %p : partial stub\n",x,y,debugstr_wn(name->Buffer,name->Length),base);

    *base = 0;

    RtlUnicodeStringToAnsiString(&str, name, TRUE);
    wm = MODULE_FindModule(str.Buffer);
    if(!wm)
        return STATUS_DLL_NOT_FOUND;
    *base = (PVOID) wm->module;

    return STATUS_SUCCESS;
}

/* FIXME : MODULE_GetProcAddress should depend on LdrGetProcedureAddress, not vice-versa */

NTSTATUS WINAPI LdrGetProcedureAddress(PVOID base, PANSI_STRING name, ULONG ord, PVOID *address)
{
    WARN("%p %s %ld %p\n",base, debugstr_an(name->Buffer,name->Length), ord, address);

    if(name)
        *address = MODULE_GetProcAddress( (HMODULE) base, name->Buffer, -1, FALSE);
    else
        *address = MODULE_GetProcAddress( (HMODULE) base, (LPSTR) ord, -1, FALSE);

    return (*address) ? STATUS_SUCCESS : STATUS_DLL_NOT_FOUND;
}
