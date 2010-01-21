/*
 * self-registerable dll functions for msdaps.dll
 *
 * Copyright (C) 2004 Raphael Junqueira
 *               2010 Huw Davies
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

#define COBJMACROS

#include "config.h"

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "winerror.h"

#include "ole2.h"
#include "olectl.h"
#include "oleauto.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(oledb);

extern HRESULT WINAPI msdaps_DllRegisterServer(void) DECLSPEC_HIDDEN;
extern HRESULT WINAPI msdaps_DllUnregisterServer(void) DECLSPEC_HIDDEN;

/***********************************************************************
 *                DllRegisterServer
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return msdaps_DllRegisterServer();
}

/***********************************************************************
 *                DllUnregisterServer
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return msdaps_DllUnregisterServer();
}
