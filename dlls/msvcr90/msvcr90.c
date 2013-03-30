/*
 * msvcr90 specific functions
 *
 * Copyright 2010 Detlef Riekenberg
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

#include "stdio.h"
#include "windef.h"
#include "winbase.h"

typedef void (__cdecl *MSVCRT__se_translator_function)(unsigned int code, struct _EXCEPTION_POINTERS *info);

static void* (__cdecl *MSVCRT_operator_new)(size_t);
static void (__cdecl *MSVCRT_operator_delete)(void*);
static MSVCRT__se_translator_function (__cdecl *MSVCRT__set_se_translator)(MSVCRT__se_translator_function);

static void init_cxx_funcs(void)
{
    HMODULE hmod = GetModuleHandleA("msvcrt.dll");

    if (sizeof(void *) > sizeof(int))  /* 64-bit has different names */
    {
        MSVCRT_operator_new = (void*)GetProcAddress(hmod, "??2@YAPEAX_K@Z");
        MSVCRT_operator_delete = (void*)GetProcAddress(hmod, "??3@YAXPEAX@Z");
        MSVCRT__set_se_translator = (void*)GetProcAddress(hmod,
                "?_set_se_translator@@YAP6AXIPEAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z");
    }
    else
    {
        MSVCRT_operator_new = (void*)GetProcAddress(hmod, "??2@YAPAXI@Z");
        MSVCRT_operator_delete = (void*)GetProcAddress(hmod, "??3@YAXPAX@Z");
        MSVCRT__set_se_translator = (void*)GetProcAddress(hmod,
                "?_set_se_translator@@YAP6AXIPAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z");
    }
}

/*********************************************************************
 *  DllMain (MSVCR90.@)
 */
BOOL WINAPI DllMain(HINSTANCE hdll, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hdll);
        init_cxx_funcs();
        _set_printf_count_output(0);
    }
    return TRUE;
}

/*********************************************************************
 *  _decode_pointer (MSVCR90.@)
 *
 * cdecl version of DecodePointer
 *
 */
void * CDECL MSVCR90_decode_pointer(void * ptr)
{
    return DecodePointer(ptr);
}

/*********************************************************************
 *  _encode_pointer (MSVCR90.@)
 *
 * cdecl version of EncodePointer
 *
 */
void * CDECL MSVCR90_encode_pointer(void * ptr)
{
    return EncodePointer(ptr);
}

/*********************************************************************
 *  ??2@YAPAXI@Z (MSVCR90.@)
 *
 * Naver LINE expects that this function is implemented inside msvcr90
 */
void* CDECL MSVCR90_operator_new(size_t size)
{
    return MSVCRT_operator_new(size);
}

/*********************************************************************
 *  ??3@YAXPAX@Z (MSVCR90.@)
 *
 * Naver LINE expects that this function is implemented inside msvcr90
 */
void CDECL MSVCR90_operator_delete(void *ptr)
{
    MSVCRT_operator_delete(ptr);
}

/*********************************************************************
 *  ?_set_se_translator@@YAP6AXIPAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z (MSVCR90.@)
 *
 * Naver LINE expects that this function is implemented inside msvcr90
 */
MSVCRT__se_translator_function CDECL MSVCR90__set_se_translator(MSVCRT__se_translator_function func)
{
    return MSVCRT__set_se_translator(func);
}
