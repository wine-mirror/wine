/*
 * Copyright (C) 2009 Damjan Jovanovic
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

#ifndef __WINE_STI_H
#define __WINE_STI_H

#include <objbase.h>
/* #include <stireg.h> */
/* #include <stierr.h> */

#include <pshpack8.h>

#ifdef __cplusplus
extern "C" {
#endif

DEFINE_GUID(CLSID_Sti, 0xB323F8E0L, 0x2E68, 0x11D0, 0x90, 0xEA, 0x00, 0xAA, 0x00, 0x60, 0xF8, 0x6C);

DEFINE_GUID(IID_IStillImageW, 0x641BD880, 0x2DC8, 0x11D0, 0x90, 0xEA, 0x00, 0xAA, 0x00, 0x60, 0xF8, 0x6C);

DEFINE_GUID(IID_IStillImageA, 0xA7B1F740, 0x1D7F, 0x11D1, 0xAC, 0xA9, 0x00, 0xA0, 0x24, 0x38, 0xAD, 0x48);

#define STI_VERSION_REAL         0x00000002
#define STI_VERSION_FLAG_UNICODE 0x01000000

#ifndef WINE_NO_UNICODE_MACROS
# ifdef UNICODE
#  define STI_VERSION (STI_VERSION_REAL | STI_VERSION_FLAG_UNICODE)
# else
#  define STI_VERSION (STI_VERSION_REAL)
# endif
#endif

typedef struct IStillImageA *PSTIA;
typedef struct IStillImageW *PSTIW;
DECL_WINELIB_TYPE_AW(PSTI)

HRESULT WINAPI StiCreateInstanceA(HINSTANCE hinst, DWORD dwVer, PSTIA *ppSti, LPUNKNOWN pUnkOuter);
HRESULT WINAPI StiCreateInstanceW(HINSTANCE hinst, DWORD dwVer, PSTIW *ppSti, LPUNKNOWN pUnkOuter);

#ifdef __cplusplus
};
#endif

#include <poppack.h>

#endif /* __WINE_STI_H */
