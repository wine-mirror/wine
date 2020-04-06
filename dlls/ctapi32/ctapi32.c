/*
 * WINE ct-api wrapper
 *
 * Copyright 2007 Christian Eggers
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
#include "wine/port.h"
#include <string.h>
#include "wine/debug.h"
#include "windef.h"
#include "winreg.h"
#include "winnls.h"
#include "ctapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(ctapi32);

#define FALLBACK_LIBCTAPI "libctapi.so"
static const WCHAR value_name[] = {'l','i','b','r','a','r','y',0};


static IS8 (*pCT_init)(IU16 ctn, IU16 pn) = NULL;
static IS8 (*pCT_data)(IU16 ctn, IU8 *dad, IU8 *sad, IU16 lenc, IU8 *command,
	IU16 *lenr, IU8 *response) = NULL;
static IS8 (*pCT_close)(IU16 ctn) = NULL;

static void *ctapi_handle = NULL;


static BOOL load_functions(void) {
	char soname[MAX_PATH] = FALLBACK_LIBCTAPI, buffer[MAX_PATH];
	LONG result;
	HKEY key_handle;

	if (pCT_init) /* loaded already */
                return TRUE;

	/* Try to get name of low level library from registry */
        /* @@ Wine registry key: HKCU\Software\Wine\ctapi32 */
	result = RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Wine\\ctapi32", 0, KEY_READ, &key_handle);
	if (result == ERROR_SUCCESS) {
		DWORD type, size;
		WCHAR buffer_w[MAX_PATH];

		size = sizeof(buffer_w) - sizeof(WCHAR);  /* Leave space for null termination */
		result = RegQueryValueExW(key_handle, value_name, NULL, &type, (LPBYTE)buffer_w, &size);
		if ((result == ERROR_SUCCESS) && (type == REG_SZ)) {
			int len;

			/* Null termination */
			buffer_w[size / sizeof(WCHAR)] = '\0';
			len = WideCharToMultiByte(CP_UNIXCP, 0, buffer_w, -1, buffer, sizeof(buffer), NULL, NULL);
			if (len)
				memcpy(soname, buffer, len);
		}
		RegCloseKey(key_handle);
	}

	TRACE("Loading library '%s'\n", soname);
	ctapi_handle = dlopen(soname, RTLD_NOW);
	if (ctapi_handle) {
		TRACE("Successfully loaded '%s'\n", soname);
	}
	else {
		MESSAGE("Wine cannot find any usable hardware library, ctapi32.dll not working.\n");
		MESSAGE("Please create the key \"HKEY_CURRENT_USER\\Software\\Wine\\ctapi32\" in your registry\n");
		MESSAGE("and set the value \"library\" to your library name (e.g. \"libctapi-cyberjack.so.1\" or \"/usr/lib/readers/libctapi.so\").\n");
                return FALSE;
	}

#define LOAD_FUNCPTR(f) if((p##f = dlsym(ctapi_handle, #f)) == NULL){WARN("Can't find symbol %s\n", #f); return FALSE;}
LOAD_FUNCPTR(CT_init);
LOAD_FUNCPTR(CT_data);
LOAD_FUNCPTR(CT_close);
#undef LOAD_FUNCPTR

        return TRUE;
}

/*
 *  ct-API specific functions
 */

IS8 WINAPI WIN_CT_init(IU16 ctn, IU16 pn)
{
	if (!pCT_init)
		return ERR_HOST;
	return pCT_init(ctn, pn);
}

IS8 WINAPI WIN_CT_data(IU16 ctn, IU8 *dad, IU8 *sad, IU16 lenc, IU8 *command, IU16 *lenr, IU8 *response)
{
	if (!pCT_data)
		return ERR_HOST;
	return pCT_data(ctn, dad, sad, lenc, command, lenr, response);
}

IS8 WINAPI WIN_CT_close(IU16 ctn)
{
	if (!pCT_close)
		return ERR_HOST;
	return pCT_close(ctn);
}

/*
 *  Dll Main function
 */
BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p,%x,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            /* Try to load low-level library */
            if (!load_functions())
		return FALSE;  /* error */
            break;
        case DLL_PROCESS_DETACH:
            if (lpvReserved) break;
            if (ctapi_handle) dlclose(ctapi_handle);
            break;
    }

    return TRUE;
}
