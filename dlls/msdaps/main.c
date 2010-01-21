/*
 * msdaps initialisation.
 *
 * Copyright 2010 Huw Davies
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
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "objbase.h"
#include "oleauto.h"
#include "oledb.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(oledb);

extern BOOL WINAPI msdaps_DllMain(HINSTANCE, DWORD, LPVOID) DECLSPEC_HIDDEN;
extern HRESULT WINAPI msdaps_DllGetClassObject(REFCLSID, REFIID, LPVOID *) DECLSPEC_HIDDEN;
extern HRESULT WINAPI msdaps_DllCanUnloadNow(void) DECLSPEC_HIDDEN;

/*****************************************************************************
 *              DllMain
 */
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    return msdaps_DllMain(instance, reason, reserved);
}

/***********************************************************************
 *              DllGetClassObject
 */
HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID *obj)
{
    return msdaps_DllGetClassObject(clsid, iid, obj);
}

/***********************************************************************
 *              DllCanUnloadNow
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return msdaps_DllCanUnloadNow();
}
