/*
 * Copyright 2019 Alistair Leslie-Hughes
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
#ifndef __ATLDEF_H__
#define __ATLDEF_H__


#include "sal.h"

#ifndef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
#define _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
#endif

#ifndef ATLPREFAST_SUPPRESS
#define ATLPREFAST_SUPPRESS(x)
#endif

#ifndef ATLPREFAST_UNSUPPRESS
#define ATLPREFAST_UNSUPPRESS(x)
#endif

#ifndef ATLASSERT
#define ATLASSERT(x)
#endif

#ifndef ATLASSUME
#define ATLASSUME(x)
#endif

#ifndef ATLENSURE
#define ATLENSURE(expr)
#endif

#define ATL_NOINLINE

#define _ATL_DECLSPEC_ALLOCATOR

#ifndef _FormatMessage_format_string_
#define _FormatMessage_format_string_
#endif

#define ATLAPI HRESULT __stdcall
#define ATLAPI_(x) x __stdcall
#define ATLINLINE inline

/* Belongs elsewhere */
#define ATLTRACE(...)

#define ATL_MAKEINTRESOURCEA(i) ((LPSTR)((ULONG_PTR)((WORD)(i))))
#define ATL_MAKEINTRESOURCEW(i) ((LPWSTR)((ULONG_PTR)((WORD)(i))))
#ifndef RC_INVOKED
# ifdef WINE_NO_UNICODE_MACROS /* force using a cast */
#  define ATL_MAKEINTRESOURCE(i) ((ULONG_PTR)((WORD)(i)))
# else
#  define ATL_MAKEINTRESOURCE WINELIB_NAME_AW(ATL_MAKEINTRESOURCE)
# endif
#endif

#define ATL_IS_INTRESOURCE(x) (((ULONG_PTR)(x) >> 16) == 0)

#define ATL_RT_CURSOR           ATL_MAKEINTRESOURCEA(1)
#define ATL_RT_BITMAP           ATL_MAKEINTRESOURCEA(2)
#define ATL_RT_ICON             ATL_MAKEINTRESOURCEA(3)
#define ATL_RT_MENU             ATL_MAKEINTRESOURCEA(4)
#define ATL_RT_DIALOG           ATL_MAKEINTRESOURCEA(5)
#define ATL_RT_STRING           ATL_MAKEINTRESOURCEA(6)
#define ATL_RT_FONTDIR          ATL_MAKEINTRESOURCEA(7)
#define ATL_RT_FONT             ATL_MAKEINTRESOURCEA(8)
#define ATL_RT_ACCELERATOR      ATL_MAKEINTRESOURCEA(9)
#define ATL_RT_RCDATA           ATL_MAKEINTRESOURCEA(10)
#define ATL_RT_MESSAGETABLE     ATL_MAKEINTRESOURCEA(11)

#define ATL_DIFFERENCE          11
#define ATL_RT_GROUP_CURSOR     ATL_MAKEINTRESOURCEW((ULONG_PTR)ATL_RT_CURSOR + ATL_DIFFERENCE)
#define ATL_RT_GROUP_ICON       ATL_MAKEINTRESOURCEW((ULONG_PTR)ATL_RT_ICON + ATL_DIFFERENCE)
#define ATL_RT_VERSION          ATL_MAKEINTRESOURCEA(16)
#define ATL_RT_DLGINCLUDE       ATL_MAKEINTRESOURCEA(17)
#define ATL_RT_PLUGPLAY         ATL_MAKEINTRESOURCEA(19)
#define ATL_RT_VXD              ATL_MAKEINTRESOURCEA(20)
#define ATL_RT_ANICURSOR        ATL_MAKEINTRESOURCEA(21)
#define ATL_RT_ANIICON          ATL_MAKEINTRESOURCEA(22)
#define ATL_RT_HTML             ATL_MAKEINTRESOURCEA(23)

#ifndef AtlThrow
#ifndef _ATL_CUSTOM_THROW
#define AtlThrow ATL::AtlThrowImpl
#endif
#endif

#ifdef __cplusplus

template < typename T >
inline T* SAL_Assume_bytecap_for_opt_(T* buf, size_t dwLen)
{
    return buf;
}

namespace ATL
{
    template <typename T>
    inline T* SAL_Assume_notnull_for_opt_z_(T* buf)
    {
        ATLASSUME(buf!=0);
        return buf;
    }

#ifndef _ATL_CUSTOM_THROW
    ATL_NOINLINE DECLSPEC_NORETURN inline void WINAPI AtlThrowImpl(_In_ HRESULT hr);
#endif
    ATL_NOINLINE DECLSPEC_NORETURN inline void WINAPI AtlThrowLastWin32();
}

#endif

#endif /* __ATLDEF_H__ */
