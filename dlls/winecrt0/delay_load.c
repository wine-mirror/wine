/*
 * Support for delayed imports
 *
 * Copyright 2005 Alexandre Julliard
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
#include "delayloadhandler.h"

WINBASEAPI void *WINAPI DelayLoadFailureHook( LPCSTR name, LPCSTR function );

#ifdef __WINE_PE_BUILD

extern IMAGE_DOS_HEADER __ImageBase;

WINBASEAPI void *WINAPI ResolveDelayLoadedAPI( void* base, const IMAGE_DELAYLOAD_DESCRIPTOR* desc,
                                               PDELAYLOAD_FAILURE_DLL_CALLBACK dllhook,
                                               PDELAYLOAD_FAILURE_SYSTEM_ROUTINE syshook,
                                               IMAGE_THUNK_DATA* addr, ULONG flags );

FARPROC WINAPI __delayLoadHelper2( const IMAGE_DELAYLOAD_DESCRIPTOR *descr, IMAGE_THUNK_DATA *addr )
{
    return ResolveDelayLoadedAPI( &__ImageBase, descr, NULL, DelayLoadFailureHook, addr, 0 );
}

#else /* __WINE_PE_BUILD */

struct ImgDelayDescr
{
    DWORD_PTR               grAttrs;
    LPCSTR                  szName;
    HMODULE                *phmod;
    IMAGE_THUNK_DATA       *pIAT;
    const IMAGE_THUNK_DATA *pINT;
    const IMAGE_THUNK_DATA *pBoundIAT;
    const IMAGE_THUNK_DATA *pUnloadIAT;
    DWORD_PTR               dwTimeStamp;
};

extern struct ImgDelayDescr __wine_spec_delay_imports[];

FARPROC WINAPI DECLSPEC_HIDDEN __wine_spec_delay_load( unsigned int id )
{
    struct ImgDelayDescr *descr = __wine_spec_delay_imports + HIWORD(id);
    WORD func = LOWORD(id);
    FARPROC proc;

    if (!*descr->phmod) *descr->phmod = LoadLibraryA( descr->szName );
    if (!*descr->phmod ||
        !(proc = GetProcAddress( *descr->phmod, (LPCSTR)descr->pINT[func].u1.Function )))
        proc = DelayLoadFailureHook( descr->szName, (LPCSTR)descr->pINT[func].u1.Function );
    descr->pIAT[func].u1.Function = (ULONG_PTR)proc;
    return proc;
}

#endif /* __WINE_PE_BUILD */
