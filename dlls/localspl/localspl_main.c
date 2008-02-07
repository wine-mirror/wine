/*
 * Implementation of the Local Printmonitor
 *
 * Copyright 2006-2008 Detlef Riekenberg
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

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "winspool.h"
#include "ddk/winsplp.h"

#include "wine/debug.h"
#include "localspl_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(localspl);

/* ############################### */

typedef struct {
    LPCWSTR  envname;
    LPCWSTR  subdir;
    DWORD    driverversion;
    LPCWSTR  versionregpath;
    LPCWSTR  versionsubdir;
} printenv_t;

/* ############################### */

HINSTANCE LOCALSPL_hInstance = NULL;

static const PRINTPROVIDOR * pp = NULL;


static const WCHAR spooldriversW[] = {'\\','s','p','o','o','l','\\','d','r','i','v','e','r','s','\\',0};

static const WCHAR win40_envnameW[] = {'W','i','n','d','o','w','s',' ','4','.','0',0};
static const WCHAR win40_subdirW[] = {'w','i','n','4','0',0};
static const WCHAR version0_regpathW[] = {'\\','V','e','r','s','i','o','n','-','0',0};
static const WCHAR version0_subdirW[] = {'\\','0',0};

static const WCHAR x86_envnameW[] = {'W','i','n','d','o','w','s',' ','N','T',' ','x','8','6',0};
static const WCHAR x86_subdirW[] = {'w','3','2','x','8','6',0};
static const WCHAR version3_regpathW[] = {'\\','V','e','r','s','i','o','n','-','3',0};
static const WCHAR version3_subdirW[] = {'\\','3',0};


static const printenv_t env_x86 =   {x86_envnameW, x86_subdirW, 3,
                                     version3_regpathW, version3_subdirW};

static const printenv_t env_win40 = {win40_envnameW, win40_subdirW, 0,
                                     version0_regpathW, version0_subdirW};

static const printenv_t * const all_printenv[] = {&env_x86, &env_win40};

/******************************************************************
 * validate_envW [internal]
 *
 * validate the user-supplied printing-environment
 *
 * PARAMS
 *  env  [I] PTR to Environment-String or NULL
 *
 * RETURNS
 *  Success:  PTR to printenv_t
 *  Failure:  NULL and ERROR_INVALID_ENVIRONMENT
 *
 * NOTES
 *  An empty string is handled the same way as NULL.
 *
 */

static const  printenv_t * validate_envW(LPCWSTR env)
{
    const printenv_t *result = NULL;
    unsigned int i;

    TRACE("(%s)\n", debugstr_w(env));
    if (env && env[0])
    {
        for (i = 0; i < sizeof(all_printenv)/sizeof(all_printenv[0]); i++)
        {
            if (lstrcmpiW(env, all_printenv[i]->envname) == 0)
            {
                result = all_printenv[i];
                break;
            }
        }
        if (result == NULL) {
            FIXME("unsupported Environment: %s\n", debugstr_w(env));
            SetLastError(ERROR_INVALID_ENVIRONMENT);
        }
        /* on win9x, only "Windows 4.0" is allowed, but we ignore this */
    }
    else
    {
        result = (GetVersion() & 0x80000000) ? &env_win40 : &env_x86;
    }

    TRACE("=> using %p: %s\n", result, debugstr_w(result ? result->envname : NULL));
    return result;
}

/*****************************************************************************
 * fpGetPrinterDriverDirectory [exported through PRINTPROVIDOR]
 *
 * Return the PATH for the Printer-Drivers
 *
 * PARAMS
 *   pName            [I] Servername (NT only) or NULL (local Computer)
 *   pEnvironment     [I] Printing-Environment (see below) or NULL (Default)
 *   Level            [I] Structure-Level (must be 1)
 *   pDriverDirectory [O] PTR to Buffer that receives the Result
 *   cbBuf            [I] Size of Buffer at pDriverDirectory
 *   pcbNeeded        [O] PTR to DWORD that receives the size in Bytes used /
 *                        required for pDriverDirectory
 *
 * RETURNS
 *   Success: TRUE  and in pcbNeeded the Bytes used in pDriverDirectory
 *   Failure: FALSE and in pcbNeeded the Bytes required for pDriverDirectory,
 *            if cbBuf is too small
 *
 *   Native Values returned in pDriverDirectory on Success:
 *|  NT(Windows NT x86):  "%winsysdir%\\spool\\DRIVERS\\w32x86"
 *|  NT(Windows 4.0):     "%winsysdir%\\spool\\DRIVERS\\win40"
 *|  win9x(Windows 4.0):  "%winsysdir%"
 *
 *   "%winsysdir%" is the Value from GetSystemDirectoryW()
 *
 */
BOOL WINAPI fpGetPrinterDriverDirectory(LPWSTR pName, LPWSTR pEnvironment,
            DWORD Level, LPBYTE pDriverDirectory, DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD needed;
    const printenv_t * env;

    TRACE("(%s, %s, %d, %p, %d, %p)\n", debugstr_w(pName),
          debugstr_w(pEnvironment), Level, pDriverDirectory, cbBuf, pcbNeeded);

    if (pName != NULL && pName[0]) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    env = validate_envW(pEnvironment);
    if (!env) return FALSE;  /* pEnvironment invalid or unsupported */


    /* GetSystemDirectoryW returns number of WCHAR including the '\0' */
    needed = GetSystemDirectoryW(NULL, 0);
    /* add the Size for the Subdirectories */
    needed += lstrlenW(spooldriversW);
    needed += lstrlenW(env->subdir);
    needed *= sizeof(WCHAR);  /* return-value is size in Bytes */

    *pcbNeeded = needed;

    if (needed > cbBuf) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    if (pDriverDirectory == NULL) {
        /* ERROR_INVALID_USER_BUFFER is NT, ERROR_INVALID_PARAMETER is win9x */
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    GetSystemDirectoryW((LPWSTR) pDriverDirectory, cbBuf/sizeof(WCHAR));
    /* add the Subdirectories */
    lstrcatW((LPWSTR) pDriverDirectory, spooldriversW);
    lstrcatW((LPWSTR) pDriverDirectory, env->subdir);

    TRACE("=> %s\n", debugstr_w((LPWSTR) pDriverDirectory));
    return TRUE;
}

/*****************************************************
 *  get_backend [internal]
 */
static const PRINTPROVIDOR * get_backend(void)
{
    static const PRINTPROVIDOR backend = {
        NULL,   /* fpOpenPrinter */
        NULL,   /* fpSetJob */
        NULL,   /* fpGetJob */
        NULL,   /* fpEnumJobs */
        NULL,   /* fpAddPrinter */
        NULL,   /* fpDeletePrinter */
        NULL,   /* fpSetPrinter */
        NULL,   /* fpGetPrinter */
        NULL,   /* fpEnumPrinters */
        NULL,   /* fpAddPrinterDriver */
        NULL,   /* fpEnumPrinterDrivers */
        NULL,   /* fpGetPrinterDriver */
        fpGetPrinterDriverDirectory,
        NULL,   /* fpDeletePrinterDriver */
        NULL,   /* fpAddPrintProcessor */
        NULL,   /* fpEnumPrintProcessors */
        NULL,   /* fpGetPrintProcessorDirectory */
        NULL,   /* fpDeletePrintProcessor */
        NULL,   /* fpEnumPrintProcessorDatatypes */
        NULL,   /* fpStartDocPrinter */
        NULL,   /* fpStartPagePrinter */
        NULL,   /* fpWritePrinter */
        NULL,   /* fpEndPagePrinter */
        NULL,   /* fpAbortPrinter */
        NULL,   /* fpReadPrinter */
        NULL,   /* fpEndDocPrinter */
        NULL,   /* fpAddJob */
        NULL,   /* fpScheduleJob */
        NULL,   /* fpGetPrinterData */
        NULL,   /* fpSetPrinterData */
        NULL,   /* fpWaitForPrinterChange */
        NULL,   /* fpClosePrinter */
        NULL,   /* fpAddForm */
        NULL,   /* fpDeleteForm */
        NULL,   /* fpGetForm */
        NULL,   /* fpSetForm */
        NULL,   /* fpEnumForms */
        NULL,   /* fpEnumMonitors */
        NULL,   /* fpEnumPorts */
        NULL,   /* fpAddPort */
        NULL,   /* fpConfigurePort */
        NULL,   /* fpDeletePort */
        NULL,   /* fpCreatePrinterIC */
        NULL,   /* fpPlayGdiScriptOnPrinterIC */
        NULL,   /* fpDeletePrinterIC */
        NULL,   /* fpAddPrinterConnection */
        NULL,   /* fpDeletePrinterConnection */
        NULL,   /* fpPrinterMessageBox */
        NULL,   /* fpAddMonitor */
        NULL,   /* fpDeleteMonitor */
        NULL,   /* fpResetPrinter */
        NULL,   /* fpGetPrinterDriverEx */
        NULL,   /* fpFindFirstPrinterChangeNotification */
        NULL,   /* fpFindClosePrinterChangeNotification */
        NULL,   /* fpAddPortEx */
        NULL,   /* fpShutDown */
        NULL,   /* fpRefreshPrinterChangeNotification */
        NULL,   /* fpOpenPrinterEx */
        NULL,   /* fpAddPrinterEx */
        NULL,   /* fpSetPort */
        NULL,   /* fpEnumPrinterData */
        NULL,   /* fpDeletePrinterData */
        NULL,   /* fpClusterSplOpen */
        NULL,   /* fpClusterSplClose */
        NULL,   /* fpClusterSplIsAlive */
        NULL,   /* fpSetPrinterDataEx */
        NULL,   /* fpGetPrinterDataEx */
        NULL,   /* fpEnumPrinterDataEx */
        NULL,   /* fpEnumPrinterKey */
        NULL,   /* fpDeletePrinterDataEx */
        NULL,   /* fpDeletePrinterKey */
        NULL,   /* fpSeekPrinter */
        NULL,   /* fpDeletePrinterDriverEx */
        NULL,   /* fpAddPerMachineConnection */
        NULL,   /* fpDeletePerMachineConnection */
        NULL,   /* fpEnumPerMachineConnections */
        NULL,   /* fpXcvData */
        NULL,   /* fpAddPrinterDriverEx */
        NULL,   /* fpSplReadPrinter */
        NULL,   /* fpDriverUnloadComplete */
        NULL,   /* fpGetSpoolFileInfo */
        NULL,   /* fpCommitSpoolData */
        NULL,   /* fpCloseSpoolFileHandle */
        NULL,   /* fpFlushPrinter */
        NULL,   /* fpSendRecvBidiData */
        NULL    /* fpAddDriverCatalog */
    };
    TRACE("=> %p\n", &backend);
    return &backend;

}

/*****************************************************
 *      DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(%p, %d, %p)\n",hinstDLL, fdwReason, lpvReserved);

    switch(fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;           /* prefer native version */

        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls( hinstDLL );
            LOCALSPL_hInstance = hinstDLL;
            pp = get_backend();
            break;
    }
    return TRUE;
}


/*****************************************************
 * InitializePrintProvidor     (localspl.@)
 *
 * Initialize the Printprovider
 *
 * PARAMS
 *  pPrintProvidor    [I] Buffer to fill with a struct PRINTPROVIDOR
 *  cbPrintProvidor   [I] Size of Buffer in Bytes
 *  pFullRegistryPath [I] Registry-Path for the Printprovidor
 *
 * RETURNS
 *  Success: TRUE and pPrintProvidor filled
 *  Failure: FALSE
 *
 * NOTES
 *  The RegistryPath should be:
 *  "System\CurrentControlSet\Control\Print\Providers\<providername>",
 *  but this Parameter is ignored in "localspl.dll".
 *
 */

BOOL WINAPI InitializePrintProvidor(LPPRINTPROVIDOR pPrintProvidor,
                                    DWORD cbPrintProvidor, LPWSTR pFullRegistryPath)
{

    TRACE("(%p, %u, %s)\n", pPrintProvidor, cbPrintProvidor, debugstr_w(pFullRegistryPath));
    memcpy(pPrintProvidor, pp, (cbPrintProvidor < sizeof(PRINTPROVIDOR)) ? cbPrintProvidor : sizeof(PRINTPROVIDOR));

    return TRUE;
}
