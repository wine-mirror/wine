/*
 * Copyright (C) 2001 Hidenori Takeshima
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

#ifndef	WINE_DSHOW_ERRORS_H
#define	WINE_DSHOW_ERRORS_H

#include "vfwmsgs.h"

#define MAX_ERROR_TEXT_LEN	160

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

DWORD WINAPI AMGetErrorTextA(HRESULT hr, LPSTR pszbuf, DWORD dwBufLen);
DWORD WINAPI AMGetErrorTextW(HRESULT hr, LPWSTR pwszbuf, DWORD dwBufLen);
#define AMGetErrorText	WINELIB_NAME_AW(AMGetErrorText)

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif	/* WINE_DSHOW_ERRORS_H */
