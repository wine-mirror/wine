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

#include <stdarg.h>

#include "msvcp90.h"

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcp);

#ifdef __ASM_USE_THISCALL_WRAPPER

#define DEFINE_VTBL_WRAPPER(off)            \
    __ASM_GLOBAL_FUNC(vtbl_wrapper_ ## off, \
        "popl %eax\n\t"                     \
        "popl %ecx\n\t"                     \
        "pushl %eax\n\t"                    \
        "movl 0(%ecx), %eax\n\t"            \
        "jmp *" #off "(%eax)\n\t")

DEFINE_VTBL_WRAPPER(0);
DEFINE_VTBL_WRAPPER(4);
DEFINE_VTBL_WRAPPER(8);
DEFINE_VTBL_WRAPPER(12);
DEFINE_VTBL_WRAPPER(16);
DEFINE_VTBL_WRAPPER(20);
DEFINE_VTBL_WRAPPER(24);
DEFINE_VTBL_WRAPPER(28);
DEFINE_VTBL_WRAPPER(32);
DEFINE_VTBL_WRAPPER(36);
DEFINE_VTBL_WRAPPER(40);
DEFINE_VTBL_WRAPPER(44);
DEFINE_VTBL_WRAPPER(48);
DEFINE_VTBL_WRAPPER(52);
DEFINE_VTBL_WRAPPER(56);

#endif

void* (__cdecl *MSVCRT_set_new_handler)(void*);

#if _MSVCP_VER >= 110
critical_section* (__thiscall *critical_section_ctor)(critical_section*);
void (__thiscall *critical_section_dtor)(critical_section*);
void (__thiscall *critical_section_lock)(critical_section*);
void (__thiscall *critical_section_unlock)(critical_section*);
bool (__thiscall *critical_section_trylock)(critical_section*);
#endif

#if _MSVCP_VER >= 100
bool (__cdecl *Context_IsCurrentTaskCollectionCanceling)(void);
#endif

#define VERSION_STRING(ver) #ver
#if _MSVCP_VER >= 140
#define MSVCRT_NAME(ver) "ucrtbase.dll"
#define CONCRT_NAME(ver) "concrt" VERSION_STRING(ver) ".dll"
#else
#define MSVCRT_NAME(ver) "msvcr" VERSION_STRING(ver) ".dll"
#endif

#if _MSVCP_VER >= 140
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
    _Xmem();
}

void __cdecl operator_delete(void *mem)
{
    TRACE("(%p)\n", mem);
    free(mem);
}

void __cdecl _invalid_parameter(const wchar_t *expr, const wchar_t *func, const wchar_t *file, unsigned int line, uintptr_t arg)
{
   _invalid_parameter_noinfo();
}
#else
static void* (__cdecl *MSVCRT_operator_new)(size_t);
static void (__cdecl *MSVCRT_operator_delete)(void*);

void* __cdecl operator_new(size_t size)
{
    void *ret = MSVCRT_operator_new(size);
#if _MSVCP_VER < 80
    if (!ret) _Xmem();
#endif
    return ret;
}

void __cdecl operator_delete(void *mem)
{
    MSVCRT_operator_delete(mem);
}
#endif

static void init_cxx_funcs(void)
{
    HMODULE hmod = GetModuleHandleA( MSVCRT_NAME(_MSVCP_VER) );
#if _MSVCP_VER >= 100
    HMODULE hcon = hmod;
#endif

    if (!hmod) FIXME( "%s not loaded\n", MSVCRT_NAME(_MSVCP_VER) );

#if _MSVCP_VER >= 140
    MSVCRT_set_new_handler = (void*)GetProcAddress(hmod, "_set_new_handler");

    hcon = LoadLibraryA( CONCRT_NAME(_MSVCP_VER) );
    if (!hcon) FIXME( "%s not loaded\n", CONCRT_NAME(_MSVCP_VER) );
#else
    if (sizeof(void *) > sizeof(int))  /* 64-bit has different names */
    {
        MSVCRT_operator_new = (void*)GetProcAddress(hmod, "??2@YAPEAX_K@Z");
        MSVCRT_operator_delete = (void*)GetProcAddress(hmod, "??3@YAXPEAX@Z");
        MSVCRT_set_new_handler = (void*)GetProcAddress(hmod, "?_set_new_handler@@YAP6AH_K@ZP6AH0@Z@Z");
    }
    else
    {
        MSVCRT_operator_new = (void*)GetProcAddress(hmod, "??2@YAPAXI@Z");
        MSVCRT_operator_delete = (void*)GetProcAddress(hmod, "??3@YAXPAX@Z");
        MSVCRT_set_new_handler = (void*)GetProcAddress(hmod, "?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z");
    }
#endif

#if _MSVCP_VER >= 110
    if (sizeof(void *) > sizeof(int))  /* 64-bit has different names */
    {
        critical_section_ctor = (void*)GetProcAddress(hcon, "??0critical_section@Concurrency@@QEAA@XZ");
        critical_section_dtor = (void*)GetProcAddress(hcon, "??1critical_section@Concurrency@@QEAA@XZ");
        critical_section_lock = (void*)GetProcAddress(hcon, "?lock@critical_section@Concurrency@@QEAAXXZ");
        critical_section_unlock = (void*)GetProcAddress(hcon, "?unlock@critical_section@Concurrency@@QEAAXXZ");
        critical_section_trylock = (void*)GetProcAddress(hcon, "?try_lock@critical_section@Concurrency@@QEAA_NXZ");
    }
    else
    {
#ifdef __arm__
        critical_section_ctor = (void*)GetProcAddress(hcon, "??0critical_section@Concurrency@@QAA@XZ");
        critical_section_dtor = (void*)GetProcAddress(hcon, "??1critical_section@Concurrency@@QAA@XZ");
        critical_section_lock = (void*)GetProcAddress(hcon, "?lock@critical_section@Concurrency@@QAAXXZ");
        critical_section_unlock = (void*)GetProcAddress(hcon, "?unlock@critical_section@Concurrency@@QAAXXZ");
        critical_section_trylock = (void*)GetProcAddress(hcon, "?try_lock@critical_section@Concurrency@@QAA_NXZ");
#else
        critical_section_ctor = (void*)GetProcAddress(hcon, "??0critical_section@Concurrency@@QAE@XZ");
        critical_section_dtor = (void*)GetProcAddress(hcon, "??1critical_section@Concurrency@@QAE@XZ");
        critical_section_lock = (void*)GetProcAddress(hcon, "?lock@critical_section@Concurrency@@QAEXXZ");
        critical_section_unlock = (void*)GetProcAddress(hcon, "?unlock@critical_section@Concurrency@@QAEXXZ");
        critical_section_trylock = (void*)GetProcAddress(hcon, "?try_lock@critical_section@Concurrency@@QAE_NXZ");
#endif
    }
#endif /* _MSVCP_VER >= 110 */

#if _MSVCP_VER >= 100
    Context_IsCurrentTaskCollectionCanceling = (void*)GetProcAddress(hcon, "?IsCurrentTaskCollectionCanceling@Context@Concurrency@@SA_NXZ");
#endif
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %ld, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            init_cxx_funcs();
            _Init_locks__Init_locks_ctor(NULL);
            init_exception(hinstDLL);
            init_locale(hinstDLL);
            init_io(hinstDLL);
#if _MSVCP_VER >= 100
            init_misc(hinstDLL);
            init_concurrency_details(hinstDLL);
#endif
            break;
        case DLL_PROCESS_DETACH:
            if (lpvReserved) break;
            free_io();
            free_locale();
            _Init_locks__Init_locks_dtor(NULL);
#if _MSVCP_VER >= 100
            free_misc();
#endif
            break;
    }

    return TRUE;
}

/* ?_BADOFF@std@@3JB -> long const std::_BADOFF */
/* ?_BADOFF@std@@3_JB -> __int64 const std::_BADOFF */
const streamoff std_BADOFF = -1;

/* ?_BADOFF_func@std@@YAABJXZ -> long const & __cdecl std::_BADOFF_func(void) */
/* ?_BADOFF_func@std@@YAAEB_JXZ -> __int64 const & __ptr64 __cdecl std::_BADOFF_func(void) */
const streamoff * __cdecl std_BADOFF_func(void)
{
    return &std_BADOFF;
}

/* ?_Fpz@std@@3_JA  __int64 std::_Fpz */
__int64 std_Fpz = 0;

/* ?_Fpz_func@std@@YAAA_JXZ -> __int64 & __cdecl std::_Fpz_func(void) */
/* ?_Fpz_func@std@@YAAEA_JXZ -> __int64 & __ptr64 __cdecl std::_Fpz_func(void) */
__int64 * __cdecl std_Fpz_func(void)
{
    return &std_Fpz;
}
