/*
 * Copyright (C) 2006 Hans Leidekker
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

#ifndef __ERRORS__
#define __ERRORS__

#ifdef __cplusplus
extern "C" {
#endif

#define VFW_FIRST_CODE      0x200
#define MAX_ERROR_TEXT_LEN  160

#include <vfwmsgs.h>

DWORD WINAPI AMGetErrorTextA(HRESULT hr, char *buffer, DWORD maxlen);
DWORD WINAPI AMGetErrorTextW(HRESULT hr, WCHAR *buffer, DWORD maxlen);

#define AMGetErrorText WINELIB_NAME_AW(AMGetErrorText)

#ifdef __cplusplus
}
#endif

#endif /* __ERRORS__ */
