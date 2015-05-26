/*
 * Copyright (C) 2007 Alexandre Julliard
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

#include "msvcirt.h"
#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcirt);

typedef struct {
    LPVOID VTable;
} class_ios;

typedef struct {
    LPVOID VTable;
} class_ostream;

typedef struct {
    LPVOID VTable;
} class_strstreambuf;

/******************************************************************
 *		 ??1ios@@UAE@XZ (MSVCRTI.@)
 *        class ios & __thiscall ios::-ios<<(void)
 */
DEFINE_THISCALL_WRAPPER(MSVCIRT_ios_sl_void,4)
void * __thiscall MSVCIRT_ios_sl_void(class_ios * _this)
{
   FIXME("(%p) stub\n", _this);
   return _this;
}

/******************************************************************
 *		 ??0ostrstream@@QAE@XZ (MSVCRTI.@)
 *        class ostream & __thiscall ostrstream::ostrstream<<(void)
 */
DEFINE_THISCALL_WRAPPER(MSVCIRT_ostrstream_sl_void,4)
void * __thiscall MSVCIRT_ostrstream_sl_void(class_ostream * _this)
{
   FIXME("(%p) stub\n", _this);
   return _this;
}

/******************************************************************
 *		??6ostream@@QAEAAV0@E@Z (MSVCRTI.@)
 *    class ostream & __thiscall ostream::operator<<(unsigned char)
 */
DEFINE_THISCALL_WRAPPER(MSVCIRT_operator_sl_uchar,8)
void * __thiscall MSVCIRT_operator_sl_uchar(class_ostream * _this, unsigned char ch)
{
   FIXME("(%p)->(%c) stub\n", _this, ch);
   return _this;
}

/******************************************************************
 *		 ??6ostream@@QAEAAV0@H@Z (MSVCRTI.@)
 *        class ostream & __thiscall ostream::operator<<(int)
 */
DEFINE_THISCALL_WRAPPER(MSVCIRT_operator_sl_int,8)
void * __thiscall MSVCIRT_operator_sl_int(class_ostream * _this, int integer)
{
   FIXME("(%p)->(%d) stub\n", _this, integer);
   return _this;
}

/******************************************************************
 *		??6ostream@@QAEAAV0@PBD@Z (MSVCRTI.@)
 *    class ostream & __thiscall ostream::operator<<(char const *)
 */
DEFINE_THISCALL_WRAPPER(MSVCIRT_operator_sl_pchar,8)
void * __thiscall MSVCIRT_operator_sl_pchar(class_ostream * _this, const char * string)
{
   FIXME("(%p)->(%s) stub\n", _this, debugstr_a(string));
   return _this;
}

/******************************************************************
 *		??6ostream@@QAEAAV0@P6AAAV0@AAV0@@Z@Z (MSVCRTI.@)
 *    class ostream & __thiscall ostream::operator<<(class ostream & (__cdecl*)(class ostream &))
 */
DEFINE_THISCALL_WRAPPER(MSVCIRT_operator_sl_callback,8)
void * __thiscall MSVCIRT_operator_sl_callback(class_ostream * _this, class_ostream * (__cdecl*func)(class_ostream*))
{
   TRACE("%p, %p\n", _this, func);
   return func(_this);
}

/******************************************************************
 *		?endl@@YAAAVostream@@AAV1@@Z (MSVCRTI.@)
 *           class ostream & __cdecl endl(class ostream &)
 */
void * CDECL MSVCIRT_endl(class_ostream * _this)
{
   FIXME("(%p)->() stub\n", _this);
   return _this;
}

/******************************************************************
 *		?ends@@YAAAVostream@@AAV1@@Z (MSVCRTI.@)
 *           class ostream & __cdecl ends(class ostream &)
 */
void * CDECL MSVCIRT_ends(class_ostream * _this)
{
   FIXME("(%p)->() stub\n", _this);
   return _this;
}

/******************************************************************
 *		?str@strstreambuf@@QAEPADXZ (MSVCRTI.@)
 *           class strstreambuf & __thiscall strstreambuf::str(class strstreambuf &)
 */
DEFINE_THISCALL_WRAPPER(MSVCIRT_str_sl_void,4)
char * __thiscall MSVCIRT_str_sl_void(class_strstreambuf * _this)
{
   FIXME("(%p)->() stub\n", _this);
   return 0;
}

void (__cdecl *MSVCRT_operator_delete)(void*);

static void init_cxx_funcs(void)
{
    HMODULE hmod = GetModuleHandleA("msvcrt.dll");

    if (sizeof(void *) > sizeof(int))  /* 64-bit has different names */
    {
        MSVCRT_operator_delete = (void*)GetProcAddress(hmod, "??3@YAXPEAX@Z");
    }
    else
    {
        MSVCRT_operator_delete = (void*)GetProcAddress(hmod, "??3@YAXPAX@Z");
    }
}

BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
   switch (reason)
   {
   case DLL_WINE_PREATTACH:
       return FALSE;  /* prefer native version */
   case DLL_PROCESS_ATTACH:
       init_cxx_funcs();
       init_exception(inst);
       DisableThreadLibraryCalls( inst );
       break;
   }
   return TRUE;
}
