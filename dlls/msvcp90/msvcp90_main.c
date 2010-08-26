/*
 * Copyright 2010 Piotr Caban for CodeWeavers
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

#include "msvcp90.h"

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcp90);

void* (__cdecl *MSVCRT_operator_new)(size_t);
void (__cdecl *MSVCRT_operator_delete)(void*);
static void init_cxx_funcs(void)
{
    HMODULE hmod = GetModuleHandleA("msvcrt.dll");

    if (sizeof(void *) > sizeof(int))  /* 64-bit has different names */
    {
        MSVCRT_operator_new = (void*)GetProcAddress(hmod, "??2@YAPEAX_K@Z");
        MSVCRT_operator_delete = (void*)GetProcAddress(hmod, "??3@YAXPEAX@Z");
    }
    else
    {
        MSVCRT_operator_new = (void*)GetProcAddress(hmod, "??2@YAPAXI@Z");
        MSVCRT_operator_delete = (void*)GetProcAddress(hmod, "??3@YAXPAX@Z");
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            init_cxx_funcs();
            set_exception_vtable();
            break;
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}
