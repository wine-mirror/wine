/*
 * Copyright 2023 Rémi Bernon for CodeWeavers
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

#ifndef __WINE_IME_TEST_H
#define __WINE_IME_TEST_H

#include <stdarg.h>
#include <stddef.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"

#include "imm.h"
#include "immdev.h"

struct ime_functions
{
    BOOL (*WINAPI pImeConfigure)(HKL,HWND,DWORD,void *);
    DWORD (*WINAPI pImeConversionList)(HIMC,const WCHAR *,CANDIDATELIST *,DWORD,UINT);
    BOOL (*WINAPI pImeDestroy)(UINT);
    UINT (*WINAPI pImeEnumRegisterWord)(REGISTERWORDENUMPROCW,const WCHAR *,DWORD,const WCHAR *,void *);
    LRESULT (*WINAPI pImeEscape)(HIMC,UINT,void *);
    DWORD (*WINAPI pImeGetImeMenuItems)(HIMC,DWORD,DWORD,IMEMENUITEMINFOW *,IMEMENUITEMINFOW *,DWORD);
    UINT (*WINAPI pImeGetRegisterWordStyle)(UINT,STYLEBUFW *);
    BOOL (*WINAPI pImeInquire)(IMEINFO *,WCHAR *,DWORD);
    BOOL (*WINAPI pImeProcessKey)(HIMC,UINT,LPARAM,BYTE *);
    BOOL (*WINAPI pImeRegisterWord)(const WCHAR *,DWORD,const WCHAR *);
    BOOL (*WINAPI pImeSelect)(HIMC,BOOL);
    BOOL (*WINAPI pImeSetActiveContext)(HIMC,BOOL);
    BOOL (*WINAPI pImeSetCompositionString)(HIMC,DWORD,const void *,DWORD,const void *,DWORD);
    UINT (*WINAPI pImeToAsciiEx)(UINT,UINT,BYTE *,TRANSMSGLIST *,UINT,HIMC);
    BOOL (*WINAPI pImeUnregisterWord)(const WCHAR *,DWORD,const WCHAR *);
    BOOL (*WINAPI pNotifyIME)(HIMC,DWORD,DWORD,DWORD);
    BOOL (*WINAPI pDllMain)(HINSTANCE,DWORD,void *);
};

#endif /* __WINE_IME_TEST_H */
