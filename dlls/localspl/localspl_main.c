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

HINSTANCE LOCALSPL_hInstance = NULL;

static const PRINTPROVIDOR * pp = NULL;

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
        NULL,   /* fpGetPrinterDriverDirectory */
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
    TRACE("=> %p (%u byte for %u entries)\n", &backend, sizeof(backend), sizeof(backend) / sizeof(VOID *));
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
