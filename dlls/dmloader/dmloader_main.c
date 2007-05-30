/* DirectMusicLoader Main
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "dmloader_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmloader);

LONG dwDirectMusicContainer = 0;
LONG dwDirectMusicLoader = 0;

/******************************************************************
 *		DllMain
 */
BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hinstDLL);
		/* FIXME: Initialisation */
	} else if (fdwReason == DLL_PROCESS_DETACH) {
		/* FIXME: Cleanup */
	}
	return TRUE;
}


/******************************************************************
 *		DllCanUnloadNow (DMLOADER.@)
 */
HRESULT WINAPI DllCanUnloadNow (void)
{
    TRACE("(void)\n");
	/* if there are no instances left, it's safe to release */
	if (!dwDirectMusicContainer && !dwDirectMusicLoader)
		return S_OK;
	else
    	return S_FALSE;
}


/******************************************************************
 *		DllGetClassObject (DMLOADER.@)
 */
HRESULT WINAPI DllGetClassObject (REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("(%s, %s, %p)\n", debugstr_dmguid(rclsid), debugstr_dmguid(riid), ppv);
    if (IsEqualCLSID (rclsid, &CLSID_DirectMusicLoader) && IsEqualIID (riid, &IID_IClassFactory)) {
		return DMUSIC_CreateDirectMusicLoaderCF (riid, ppv, NULL);
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicContainer) && IsEqualIID (riid, &IID_IClassFactory)) {
		return DMUSIC_CreateDirectMusicContainerCF (riid, ppv, NULL);		
	}
	
    WARN(": no class found\n");
    return CLASS_E_CLASSNOTAVAILABLE;
}
