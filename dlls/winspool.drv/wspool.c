/******************************************************************************
 * Print Spooler Functions
 *
 *
 * Copyright 1999 Thuy Nguyen
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

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winspool.h"
#include "winreg.h"
#include "winternl.h"
#include "ddk/winsplp.h"
#include "wine/debug.h"
#include "wine/unixlib.h"

#include "wspool.h"

WINE_DEFAULT_DEBUG_CHANNEL(winspool);

/* ############################### */

static CRITICAL_SECTION backend_cs;
static CRITICAL_SECTION_DEBUG backend_cs_debug =
{
    0, 0, &backend_cs,
    { &backend_cs_debug.ProcessLocksList, &backend_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": backend_cs") }
};
static CRITICAL_SECTION backend_cs = { &backend_cs_debug, -1, 0, 0, 0, 0 };

/* ############################### */

HINSTANCE WINSPOOL_hInstance = NULL;

static HMODULE hlocalspl;
static BOOL (WINAPI *pInitializePrintProvidor)(LPPRINTPROVIDOR, DWORD, LPWSTR);
PRINTPROVIDOR *backend = NULL;

/******************************************************************************
 * load_backend [internal]
 *
 * load and init our backend (the local printprovider: "localspl.dll")
 *
 * PARAMS
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE and RPC_S_SERVER_UNAVAILABLE
 *
 * NOTES
 *  In windows, winspool.drv use RPC to interact with the spooler service
 *  (spoolsv.exe with spoolss.dll) and the spooler router (spoolss.dll) interact
 *  with the correct printprovider (localspl.dll for the local system)
 *
 */
BOOL load_backend(void)
{
    static PRINTPROVIDOR mybackend;
    DWORD res;

    EnterCriticalSection(&backend_cs);
    hlocalspl = LoadLibraryA("localspl.dll");
    if (hlocalspl) {
        pInitializePrintProvidor = (void *) GetProcAddress(hlocalspl, "InitializePrintProvidor");
        if (pInitializePrintProvidor) {

            /* native localspl does not clear unused entries */
            memset(&mybackend, 0, sizeof(mybackend));
            res = pInitializePrintProvidor(&mybackend, sizeof(mybackend), NULL);
            if (res) {
                backend = &mybackend;
                LeaveCriticalSection(&backend_cs);
                TRACE("backend: %p (%p)\n", backend, hlocalspl);
                return TRUE;
            }
        }
        FreeLibrary(hlocalspl);
    }

    LeaveCriticalSection(&backend_cs);

    WARN("failed to load the backend: %lu\n", GetLastError());
    SetLastError(RPC_S_SERVER_UNAVAILABLE);
    return FALSE;
}

/******************************************************************************
 *  DllMain
 *
 * Winspool entry point.
 *
 */
BOOL WINAPI DllMain( HINSTANCE instance, DWORD reason, void *reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        WINSPOOL_hInstance = instance;
        DisableThreadLibraryCalls( instance );
        if (!__wine_init_unix_call())
            UNIX_CALL( process_attach, NULL );
        WINSPOOL_LoadSystemPrinters();
        break;

    case DLL_PROCESS_DETACH:
        if (reserved) break;
        DeleteCriticalSection(&backend_cs);
        FreeLibrary(hlocalspl);
        break;
    }

    return TRUE;
}
