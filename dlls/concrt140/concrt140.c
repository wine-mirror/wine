/*
 * Copyright 2021 Piotr Caban for CodeWeavers
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

#include <malloc.h>
#include <stdarg.h>

#include "msvcrt.h"
#include "windef.h"
#include "winternl.h"
#include "wine/debug.h"
#include "cxx.h"

WINE_DEFAULT_DEBUG_CHANNEL(concrt);

CREATE_TYPE_INFO_VTABLE

static HMODULE msvcp140;

extern const vtable_ptr exception_vtable;

int CDECL _callnewh(size_t size);
void (__cdecl *_Xbad_alloc)(void);

void* __cdecl operator_new(size_t size)
{
    void *retval;
    int freed;

    do
    {
        retval = malloc(size);
        if (retval)
        {
            TRACE("(%Iu) returning %p\n", size, retval);
            return retval;
        }
        freed = _callnewh(size);
    } while (freed);

    TRACE("(%Iu) out of memory\n", size);
    _Xbad_alloc();
    return NULL;
}

void __cdecl operator_delete(void *mem)
{
    free(mem);
}

static BOOL init_cxx_funcs(void)
{
    msvcp140 = LoadLibraryA("msvcp140.dll");
    if (!msvcp140)
    {
        FIXME("Failed to load msvcp140.dll\n");
        return FALSE;
    }

    _Xbad_alloc = (void*)GetProcAddress(msvcp140, "?_Xbad_alloc@std@@YAXXZ");
    if (!_Xbad_alloc)
    {
        FIXME("Failed to get address of ?_Xbad_alloc@std@@YAXXZ\n");
        FreeLibrary(msvcp140);
        return FALSE;
    }

    return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
   switch (reason)
   {
   case DLL_PROCESS_ATTACH:
       if (!init_cxx_funcs()) return FALSE;
#ifdef __x86_64__
       init_type_info_rtti((char*)inst);
#endif
       msvcrt_init_concurrency(inst);
       break;
   case DLL_PROCESS_DETACH:
       if (reserved) break;
       FreeLibrary(msvcp140);
       break;
   }
   return TRUE;
}
