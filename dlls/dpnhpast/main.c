/* DirectPlay NAT Helper Past Main
 *
 * Copyright (C) 2003 Rok Mandeljc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dpnhpast);

/******************************************************************
 *		DllMain
 *
 *
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	TRACE("(%p, %ld, %p)\n",hinstDLL,fdwReason,lpvReserved);

	if (fdwReason == DLL_PROCESS_ATTACH)
	{
            DisableThreadLibraryCalls(hinstDLL);
		/* FIXME: Initialisation */
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
		/* FIXME: Cleanup */
	}

	return TRUE;
}


/******************************************************************
 *		DirectPlayNATHelpCreate (DPNHPAST.1)
 *
 *
 */
#if 0
HRESULT WINAPI DPNHPAST_DirectPlayNATHelpCreate()
{
	/* @stub in .spec */
}
#endif


/******************************************************************
 *		DllRegisterServer (DPNHPAST.2)
 *
 *
 */
HRESULT WINAPI DPNHPAST_DllRegisterServer(void)
{
	FIXME(":stub\n");
	return S_OK;
}


/******************************************************************
 *		DllCanUnloadNow (DPNHPAST.3)
 *
 *
 */
HRESULT WINAPI DPNHPAST_DllCanUnloadNow(void)
{
	FIXME(":stub\n");
	return S_OK;
}


/******************************************************************
 *		DllGetClassObject (DPNHPAST.4)
 *
 *
 */
HRESULT WINAPI DPNHPAST_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
	FIXME(":stub\n");
	return E_FAIL;
}


/******************************************************************
 *		DllUnregisterServer (DPNHPAST.5)
 *
 *
 */
HRESULT WINAPI DPNHPAST_DllUnregisterServer(void)
{
	FIXME(":stub\n");
	return S_OK;
}
