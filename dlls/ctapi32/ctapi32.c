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

#include <string.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#include "unixlib.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ctapi32);

#define FALLBACK_LIBCTAPI "libctapi.so"

#define CTAPI_CALL( func, params ) WINE_UNIX_CALL( unix_ ## func, params )


static BOOL load_functions(void) {
	char soname[MAX_PATH] = FALLBACK_LIBCTAPI;
	LONG result;
	HKEY key_handle;

	/* Try to get name of low level library from registry */
        /* @@ Wine registry key: HKCU\Software\Wine\ctapi32 */
	result = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Wine\\ctapi32", 0, KEY_READ, &key_handle);
	if (result == ERROR_SUCCESS) {
		DWORD type, size;
		WCHAR buffer_w[MAX_PATH];

		size = sizeof(buffer_w) - sizeof(WCHAR);  /* Leave space for null termination */
		result = RegQueryValueExW(key_handle, L"library", NULL, &type, (LPBYTE)buffer_w, &size);
		if ((result == ERROR_SUCCESS) && (type == REG_SZ)) {
			/* Null termination */
			buffer_w[size / sizeof(WCHAR)] = '\0';
			WideCharToMultiByte(CP_UNIXCP, 0, buffer_w, -1, soname, sizeof(soname), NULL, NULL);
		}
		RegCloseKey(key_handle);
	}

	TRACE("Loading library '%s'\n", soname);
        if (!CTAPI_CALL( attach, soname )) return TRUE;

	MESSAGE("Wine cannot find any usable hardware library, ctapi32.dll not working.\n");
	MESSAGE("Please create the key \"HKEY_CURRENT_USER\\Software\\Wine\\ctapi32\" in your registry\n");
	MESSAGE("and set the value \"library\" to your library name (e.g. \"libctapi-cyberjack.so.1\" or \"/usr/lib/readers/libctapi.so\").\n");
	return FALSE;
}

/*
 *  ct-API specific functions
 */

IS8 WINAPI CT_init(IU16 ctn, IU16 pn)
{
    struct ct_init_params params = { ctn, pn };

    return CTAPI_CALL( ct_init, &params );
}

IS8 WINAPI CT_data(IU16 ctn, IU8 *dad, IU8 *sad, IU16 lenc, IU8 *command, IU16 *lenr, IU8 *response)
{
    struct ct_data_params params = { ctn, dad, sad, lenc, command, lenr, response };

    return CTAPI_CALL( ct_data, &params );
}

IS8 WINAPI CT_close(IU16 ctn)
{
    struct ct_close_params params = { ctn };

    return CTAPI_CALL( ct_close, &params );
}

/*
 *  Dll Main function
 */
BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p,%lx,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            if (__wine_init_unix_call())
                return FALSE;
            if (!load_functions())
		return FALSE;
            break;
        case DLL_PROCESS_DETACH:
            if (lpvReserved) break;
            CTAPI_CALL( detach, NULL );
            break;
    }

    return TRUE;
}
