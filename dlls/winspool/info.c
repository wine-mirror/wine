/* 
 * WINSPOOL functions
 * 
 * Copyright 1996 John Harvey
 * Copyright 1998 Andreas Mohr
 * Copyright 1999 Klaas van Gend, Huw D M Davies
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "winspool.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "debugtools.h"
#include "heap.h"
#include "commctrl.h"

DEFAULT_DEBUG_CHANNEL(winspool)

CRITICAL_SECTION PRINT32_RegistryBlocker;

typedef struct _OPENEDPRINTERA
{
    LPSTR lpsPrinterName;
    HANDLE hPrinter;
    LPPRINTER_DEFAULTSA lpDefault;
} OPENEDPRINTERA, *LPOPENEDPRINTERA;

/* The OpenedPrinter Table dynamic array */
static HDPA pOpenedPrinterDPA = NULL;

extern HDPA   (WINAPI* WINSPOOL_DPA_CreateEx) (INT, HANDLE);
extern LPVOID (WINAPI* WINSPOOL_DPA_GetPtr) (const HDPA, INT);
extern INT    (WINAPI* WINSPOOL_DPA_InsertPtr) (const HDPA, INT, LPVOID);

static char Printers[] =
 "System\\CurrentControlSet\\control\\Print\\Printers\\";
static char Drivers[] =
"System\\CurrentControlSet\\control\\Print\\Environments\\Windows 4.0\\Drivers\\"; /* Hmm, well */

/******************************************************************
 *  WINSPOOL_GetOpenedPrinterEntryA
 *  Get the first place empty in the opened printer table
 */
static LPOPENEDPRINTERA WINSPOOL_GetOpenedPrinterEntryA()
{
    int i;
    LPOPENEDPRINTERA pOpenedPrinter;

    /*
     * Create the opened printers' handle dynamic array.
     */
    if (!pOpenedPrinterDPA)
    {
        pOpenedPrinterDPA = WINSPOOL_DPA_CreateEx(10, GetProcessHeap());
        for (i = 0; i < 10; i++)
        {
            pOpenedPrinter = HeapAlloc(GetProcessHeap(),
                                       HEAP_ZERO_MEMORY,
                                       sizeof(OPENEDPRINTERA));
            pOpenedPrinter->hPrinter = -1;
            WINSPOOL_DPA_InsertPtr(pOpenedPrinterDPA, i, pOpenedPrinter);
        }
    }

    /*
     * Search for a handle not yet allocated.
     */
    for (i = 0; i < pOpenedPrinterDPA->nItemCount; i++)
    {
        pOpenedPrinter = WINSPOOL_DPA_GetPtr(pOpenedPrinterDPA, i);

        if (pOpenedPrinter->hPrinter == -1)
        {
            pOpenedPrinter->hPrinter = i + 1;
            return pOpenedPrinter;
        }
    }

    /*
     * Didn't find one, insert new element in the array.
     */
    if (i == pOpenedPrinterDPA->nItemCount)
    {
        pOpenedPrinter = HeapAlloc(GetProcessHeap(),
                                   HEAP_ZERO_MEMORY,
                                   sizeof(OPENEDPRINTERA));
        pOpenedPrinter->hPrinter = i + 1;
        WINSPOOL_DPA_InsertPtr(pOpenedPrinterDPA, i, pOpenedPrinter);
        return pOpenedPrinter;
    }

    return NULL;
}

/******************************************************************
 *  WINSPOOL_GetOpenedPrinterA
 *  Get the pointer to the opened printer referred by the handle
 */
static LPOPENEDPRINTERA WINSPOOL_GetOpenedPrinterA(int printerHandle)
{
    LPOPENEDPRINTERA pOpenedPrinter;

    if(!pOpenedPrinterDPA) return NULL;
    if((printerHandle <=0) || 
       (printerHandle > (pOpenedPrinterDPA->nItemCount - 1)))
        return NULL;

    pOpenedPrinter = WINSPOOL_DPA_GetPtr(pOpenedPrinterDPA, printerHandle-1);

    return pOpenedPrinter;
}

/******************************************************************
 *              DeviceCapabilities32A    [WINSPOOL.151]
 *
 */
INT WINAPI DeviceCapabilitiesA(LPCSTR pDeivce,LPCSTR pPort, WORD cap,
			       LPSTR pOutput, LPDEVMODEA lpdm)
{
    INT ret;
    ret = GDI_CallDeviceCapabilities16(pDeivce, pPort, cap, pOutput, lpdm);

    /* If DC_PAPERSIZE map POINT16s to POINTs */
    if(ret != -1 && cap == DC_PAPERSIZE && pOutput) {
        POINT16 *tmp = HeapAlloc( GetProcessHeap(), 0, ret * sizeof(POINT16) );
	INT i;
	memcpy(tmp, pOutput, ret * sizeof(POINT16));
	for(i = 0; i < ret; i++)
	    CONV_POINT16TO32(tmp + i, (POINT*)pOutput + i);
	HeapFree( GetProcessHeap(), 0, tmp );
    }
    return ret;
}


/*****************************************************************************
 *          DeviceCapabilities32W 
 */
INT WINAPI DeviceCapabilitiesW(LPCWSTR pDevice, LPCWSTR pPort,
			       WORD fwCapability, LPWSTR pOutput,
			       const DEVMODEW *pDevMode)
{
    FIXME("(%p,%p,%d,%p,%p): stub\n",
          pDevice, pPort, fwCapability, pOutput, pDevMode);
    return -1;
}

/******************************************************************
 *              DocumentProperties32A   [WINSPOOL.155]
 *
 */
LONG WINAPI DocumentPropertiesA(HWND hWnd,HANDLE hPrinter,
                                LPSTR pDeviceName, LPDEVMODEA pDevModeOutput,
				LPDEVMODEA pDevModeInput,DWORD fMode )
{
    LPOPENEDPRINTERA lpOpenedPrinter;
    LPSTR lpName = pDeviceName;

    TRACE("(%d,%d,%s,%p,%p,%ld)\n",
	hWnd,hPrinter,pDeviceName,pDevModeOutput,pDevModeInput,fMode
    );

    if(!pDeviceName) {
        lpOpenedPrinter = WINSPOOL_GetOpenedPrinterA(hPrinter);
	if(!lpOpenedPrinter) {
	    SetLastError(ERROR_INVALID_HANDLE);
	    return -1;
	}
	lpName = lpOpenedPrinter->lpsPrinterName;
    }

    return GDI_CallExtDeviceMode16(hWnd, pDevModeOutput, lpName, "LPT1:",
				   pDevModeInput, NULL, fMode);

}


/*****************************************************************************
 *          DocumentProperties32W 
 */
LONG WINAPI DocumentPropertiesW(HWND hWnd, HANDLE hPrinter,
                                  LPWSTR pDeviceName,
                                  LPDEVMODEW pDevModeOutput,
                                  LPDEVMODEW pDevModeInput, DWORD fMode)
{
    FIXME("(%d,%d,%s,%p,%p,%ld): stub\n",
          hWnd,hPrinter,debugstr_w(pDeviceName),pDevModeOutput,pDevModeInput,
	  fMode);
    return -1;
}


/******************************************************************
 *              OpenPrinter32A        [WINSPOOL.196]
 *
 */
BOOL WINAPI OpenPrinterA(LPSTR lpPrinterName,HANDLE *phPrinter,
			     LPPRINTER_DEFAULTSA pDefault)
{
  /* Not implemented: use the DesiredAccess of pDefault to set 
     the access rights to the printer */

    LPOPENEDPRINTERA lpOpenedPrinter;
    HKEY hkeyPrinters, hkeyPrinter;

    if (!lpPrinterName) {
       WARN("(printerName: NULL, pDefault %p Ret: False\n", pDefault);
       SetLastError(ERROR_INVALID_PARAMETER);
       return FALSE;
    }

    TRACE("(printerName: %s, pDefault %p\n", lpPrinterName, pDefault);

    /* Check Printer exists */
    if(RegCreateKeyA(HKEY_LOCAL_MACHINE, Printers, &hkeyPrinters) !=
       ERROR_SUCCESS) {
        ERR("Can't create Printers key\n");
	SetLastError(ERROR_FILE_NOT_FOUND); /* ?? */
	return FALSE;
    }

    if(RegOpenKeyA(hkeyPrinters, lpPrinterName, &hkeyPrinter)
       != ERROR_SUCCESS) {
        WARN("Can't find printer `%s' in registry\n", lpPrinterName);
	RegCloseKey(hkeyPrinters);
        SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
    RegCloseKey(hkeyPrinter);
    RegCloseKey(hkeyPrinters);

    if(!phPrinter) /* This seems to be what win95 does anyway */
        return TRUE;

    /* Get a place in the opened printer buffer*/
    lpOpenedPrinter = WINSPOOL_GetOpenedPrinterEntryA();
    if(!lpOpenedPrinter) {
        ERR("Can't allocate printer slot\n");
	SetLastError(ERROR_OUTOFMEMORY);
	return FALSE;
    }

    /* Get the name of the printer */
    lpOpenedPrinter->lpsPrinterName = 
      HEAP_strdupA( GetProcessHeap(), 0, lpPrinterName );

    /* Get the unique handle of the printer*/
    *phPrinter = lpOpenedPrinter->hPrinter;

    if (pDefault != NULL) {
        lpOpenedPrinter->lpDefault = 
	  HeapAlloc(GetProcessHeap(), 0, sizeof(PRINTER_DEFAULTSA));
	lpOpenedPrinter->lpDefault->pDevMode =
	  HeapAlloc(GetProcessHeap(), 0, sizeof(DEVMODEA));
	memcpy(lpOpenedPrinter->lpDefault->pDevMode, pDefault->pDevMode,
	       sizeof(DEVMODEA));
	lpOpenedPrinter->lpDefault->pDatatype = 
	  HEAP_strdupA( GetProcessHeap(), 0, pDefault->pDatatype );
	lpOpenedPrinter->lpDefault->DesiredAccess =
	  pDefault->DesiredAccess;
    }
    
    return TRUE;

}

/******************************************************************
 *              OpenPrinter32W        [WINSPOOL.197]
 *
 */
BOOL WINAPI OpenPrinterW(LPWSTR lpPrinterName,HANDLE *phPrinter,
			     LPPRINTER_DEFAULTSW pDefault)
{
    FIXME("(%s,%p,%p):stub\n",debugstr_w(lpPrinterName), phPrinter,
          pDefault);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************
 *              AddMonitor32A        [WINSPOOL.107]
 *
 */
BOOL WINAPI AddMonitorA(LPSTR pName, DWORD Level, LPBYTE pMonitors)
{
    FIXME("(%s,%lx,%p):stub!\n", pName, Level, pMonitors);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************
 *              DeletePrinterDriver32A        [WINSPOOL.146]
 *
 */
BOOL WINAPI
DeletePrinterDriverA (LPSTR pName, LPSTR pEnvironment, LPSTR pDriverName)
{
    FIXME("(%s,%s,%s):stub\n",debugstr_a(pName),debugstr_a(pEnvironment),
          debugstr_a(pDriverName));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/******************************************************************
 *              DeleteMonitor32A        [WINSPOOL.135]
 *
 */
BOOL WINAPI
DeleteMonitorA (LPSTR pName, LPSTR pEnvironment, LPSTR pMonitorName)
{
    FIXME("(%s,%s,%s):stub\n",debugstr_a(pName),debugstr_a(pEnvironment),
          debugstr_a(pMonitorName));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/******************************************************************
 *              DeletePort32A        [WINSPOOL.137]
 *
 */
BOOL WINAPI
DeletePortA (LPSTR pName, HWND hWnd, LPSTR pPortName)
{
    FIXME("(%s,0x%08x,%s):stub\n",debugstr_a(pName),hWnd,
          debugstr_a(pPortName));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************************
 *    SetPrinter32W  [WINSPOOL.214]
 */
BOOL WINAPI
SetPrinterW(
  HANDLE  hPrinter,
  DWORD     Level,
  LPBYTE    pPrinter,
  DWORD     Command) {

       FIXME("():stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
       return FALSE;
}

/******************************************************************************
 *    WritePrinter32  [WINSPOOL.223]
 */
BOOL WINAPI
WritePrinter( 
  HANDLE  hPrinter,
  LPVOID  pBuf,
  DWORD   cbBuf,
  LPDWORD pcWritten) {

       FIXME("():stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
       return FALSE;
}

/*****************************************************************************
 *          AddForm32A  [WINSPOOL.103]
 */
BOOL WINAPI AddFormA(HANDLE hPrinter, DWORD Level, LPBYTE pForm)
{
    FIXME("(%d,%ld,%p): stub\n", hPrinter, Level, pForm);
    return 1;
}

/*****************************************************************************
 *          AddForm32W  [WINSPOOL.104]
 */
BOOL WINAPI AddFormW(HANDLE hPrinter, DWORD Level, LPBYTE pForm)
{
    FIXME("(%d,%ld,%p): stub\n", hPrinter, Level, pForm);
    return 1;
}

/*****************************************************************************
 *          AddJob32A  [WINSPOOL.105]
 */
BOOL WINAPI AddJobA(HANDLE hPrinter, DWORD Level, LPBYTE pData,
                        DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME("(%d,%ld,%p,%ld,%p): stub\n", hPrinter, Level, pData, cbBuf,
          pcbNeeded);
    return 1;
}

/*****************************************************************************
 *          AddJob32W  [WINSPOOL.106]
 */
BOOL WINAPI AddJobW(HANDLE hPrinter, DWORD Level, LPBYTE pData, DWORD cbBuf,
                        LPDWORD pcbNeeded)
{
    FIXME("(%d,%ld,%p,%ld,%p): stub\n", hPrinter, Level, pData, cbBuf,
          pcbNeeded);
    return 1;
}

/*****************************************************************************
 *          AddPrinter32A  [WINSPOOL.117]
 */
HANDLE WINAPI AddPrinterA(LPSTR pName, DWORD Level, LPBYTE pPrinter)
{
    PRINTER_INFO_2A *pi = (PRINTER_INFO_2A *) pPrinter;

    HANDLE retval;
    HKEY hkeyPrinter, hkeyPrinters, hkeyDriver, hkeyDrivers;

    TRACE("(%s,%ld,%p)\n", pName, Level, pPrinter);
    
    if(pName != NULL) {
        FIXME("pName = `%s' - unsupported\n", pName);
	SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }
    if(Level != 2) {
        WARN("Level = %ld\n", Level);
	SetLastError(ERROR_INVALID_LEVEL);
	return 0;
    }
    if(!pPrinter) {
        SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }
    if(RegCreateKeyA(HKEY_LOCAL_MACHINE, Printers, &hkeyPrinters) !=
       ERROR_SUCCESS) {
        ERR("Can't create Printers key\n");
	return 0;
    }
    if(RegOpenKeyA(hkeyPrinters, pi->pPrinterName, &hkeyPrinter) ==
       ERROR_SUCCESS) {
        SetLastError(ERROR_PRINTER_ALREADY_EXISTS);
	RegCloseKey(hkeyPrinter);
	RegCloseKey(hkeyPrinters);
	return 0;
    }
    if(RegCreateKeyA(HKEY_LOCAL_MACHINE, Drivers, &hkeyDrivers) !=
       ERROR_SUCCESS) {
        ERR("Can't create Drivers key\n");
	RegCloseKey(hkeyPrinters);
	return 0;
    }
    if(RegOpenKeyA(hkeyDrivers, pi->pDriverName, &hkeyDriver) != 
       ERROR_SUCCESS) {
        WARN("Can't find driver `%s'\n", pi->pDriverName);
	RegCloseKey(hkeyPrinters);
	RegCloseKey(hkeyDrivers);
	SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER);
	return 0;
    }
    RegCloseKey(hkeyDriver);
    RegCloseKey(hkeyDrivers);
    if(strcasecmp(pi->pPrintProcessor, "WinPrint")) {  /* FIXME */
        WARN("Can't find processor `%s'\n", pi->pPrintProcessor);
	SetLastError(ERROR_UNKNOWN_PRINTPROCESSOR);
	RegCloseKey(hkeyPrinters);
	return 0;
    }
    if(RegCreateKeyA(hkeyPrinters, pi->pPrinterName, &hkeyPrinter) !=
       ERROR_SUCCESS) {
        WARN("Can't create printer `%s'\n", pi->pPrinterName);
	SetLastError(ERROR_INVALID_PRINTER_NAME);
	RegCloseKey(hkeyPrinters);
	return 0;
    }
    RegSetValueExA(hkeyPrinter, "Attributes", 0, REG_DWORD,
		   (LPSTR)&pi->Attributes, sizeof(DWORD));
    RegSetValueExA(hkeyPrinter, "Default DevMode", 0, REG_BINARY,
		   (LPSTR)&pi->pDevMode,
		   pi->pDevMode ? pi->pDevMode->dmSize : 0);
    RegSetValueExA(hkeyPrinter, "Description", 0, REG_SZ, pi->pComment, 0);
    RegSetValueExA(hkeyPrinter, "Location", 0, REG_SZ, pi->pLocation, 0);
    RegSetValueExA(hkeyPrinter, "Name", 0, REG_SZ, pi->pPrinterName, 0);
    RegSetValueExA(hkeyPrinter, "Parameters", 0, REG_SZ, pi->pParameters, 0);
    RegSetValueExA(hkeyPrinter, "Port", 0, REG_SZ, pi->pPortName, 0);
    RegSetValueExA(hkeyPrinter, "Print Processor", 0, REG_SZ,
		   pi->pPrintProcessor, 0);
    RegSetValueExA(hkeyPrinter, "Printer Driver", 0, REG_SZ, pi->pDriverName,
		   0);
    RegSetValueExA(hkeyPrinter, "Priority", 0, REG_DWORD,
		   (LPSTR)&pi->Priority, sizeof(DWORD));
    RegSetValueExA(hkeyPrinter, "Separator File", 0, REG_SZ, pi->pSepFile, 0);
    RegSetValueExA(hkeyPrinter, "Share Name", 0, REG_SZ, pi->pShareName, 0);
    RegSetValueExA(hkeyPrinter, "Start Time", 0, REG_DWORD,
		   (LPSTR)&pi->StartTime, sizeof(DWORD));
    RegSetValueExA(hkeyPrinter, "Status", 0, REG_DWORD,
		   (LPSTR)&pi->Status, sizeof(DWORD));
    RegSetValueExA(hkeyPrinter, "Until Time", 0, REG_DWORD,
		   (LPSTR)&pi->UntilTime, sizeof(DWORD));

    RegCloseKey(hkeyPrinter);
    RegCloseKey(hkeyPrinters);
    if(!OpenPrinterA(pi->pPrinterName, &retval, NULL)) {
        ERR("OpenPrinter failing\n");
	return 0;
    }
    return retval;
}

/*****************************************************************************
 *          AddPrinter32W  [WINSPOOL.122]
 */
HANDLE WINAPI AddPrinterW(LPWSTR pName, DWORD Level, LPBYTE pPrinter)
{
    FIXME("(%p,%ld,%p): stub\n", pName, Level, pPrinter);
    return 0;
}


/*****************************************************************************
 *          ClosePrinter32  [WINSPOOL.126]
 */
BOOL WINAPI ClosePrinter(HANDLE hPrinter)
{
    LPOPENEDPRINTERA lpOpenedPrinter;

    TRACE("Handle %d\n", hPrinter);

    if (!pOpenedPrinterDPA)
        return FALSE;

    if ((hPrinter != -1) && (hPrinter < (pOpenedPrinterDPA->nItemCount - 1)))
    {
	lpOpenedPrinter = WINSPOOL_GetOpenedPrinterA(hPrinter);
	HeapFree(GetProcessHeap(), 0, lpOpenedPrinter->lpsPrinterName);
	lpOpenedPrinter->lpsPrinterName = NULL;
	
	/* Free the memory of lpDefault if it has been initialized*/
	if(lpOpenedPrinter->lpDefault != NULL)
	{
	    HeapFree(GetProcessHeap(), 0,
		     lpOpenedPrinter->lpDefault->pDevMode);
	    HeapFree(GetProcessHeap(), 0,
		     lpOpenedPrinter->lpDefault->pDatatype);
	    HeapFree(GetProcessHeap(), 0,
		     lpOpenedPrinter->lpDefault);
	    lpOpenedPrinter->lpDefault = NULL;
	}
	
	lpOpenedPrinter->hPrinter = -1;

	return TRUE;
    }
    return FALSE;
}

/*****************************************************************************
 *          DeleteForm32A  [WINSPOOL.133]
 */
BOOL WINAPI DeleteFormA(HANDLE hPrinter, LPSTR pFormName)
{
    FIXME("(%d,%s): stub\n", hPrinter, pFormName);
    return 1;
}

/*****************************************************************************
 *          DeleteForm32W  [WINSPOOL.134]
 */
BOOL WINAPI DeleteFormW(HANDLE hPrinter, LPWSTR pFormName)
{
    FIXME("(%d,%s): stub\n", hPrinter, debugstr_w(pFormName));
    return 1;
}

/*****************************************************************************
 *          DeletePrinter32  [WINSPOOL.143]
 */
BOOL WINAPI DeletePrinter(HANDLE hPrinter)
{
    FIXME("(%d): stub\n", hPrinter);
    return 1;
}

/*****************************************************************************
 *          SetPrinter32A  [WINSPOOL.211]
 */
BOOL WINAPI SetPrinterA(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
                           DWORD Command)
{
    FIXME("(%d,%ld,%p,%ld): stub\n",hPrinter,Level,pPrinter,Command);
    return FALSE;
}

/*****************************************************************************
 *          SetJob32A  [WINSPOOL.209]
 */
BOOL WINAPI SetJobA(HANDLE hPrinter, DWORD JobId, DWORD Level,
                       LPBYTE pJob, DWORD Command)
{
    FIXME("(%d,%ld,%ld,%p,%ld): stub\n",hPrinter,JobId,Level,pJob,
         Command);
    return FALSE;
}

/*****************************************************************************
 *          SetJob32W  [WINSPOOL.210]
 */
BOOL WINAPI SetJobW(HANDLE hPrinter, DWORD JobId, DWORD Level,
                       LPBYTE pJob, DWORD Command)
{
    FIXME("(%d,%ld,%ld,%p,%ld): stub\n",hPrinter,JobId,Level,pJob,
         Command);
    return FALSE;
}

/*****************************************************************************
 *          GetForm32A  [WINSPOOL.181]
 */
BOOL WINAPI GetFormA(HANDLE hPrinter, LPSTR pFormName, DWORD Level,
                 LPBYTE pForm, DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME("(%d,%s,%ld,%p,%ld,%p): stub\n",hPrinter,pFormName,
         Level,pForm,cbBuf,pcbNeeded); 
    return FALSE;
}

/*****************************************************************************
 *          GetForm32W  [WINSPOOL.182]
 */
BOOL WINAPI GetFormW(HANDLE hPrinter, LPWSTR pFormName, DWORD Level,
                 LPBYTE pForm, DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME("(%d,%s,%ld,%p,%ld,%p): stub\n",hPrinter,
	  debugstr_w(pFormName),Level,pForm,cbBuf,pcbNeeded);
    return FALSE;
}

/*****************************************************************************
 *          SetForm32A  [WINSPOOL.207]
 */
BOOL WINAPI SetFormA(HANDLE hPrinter, LPSTR pFormName, DWORD Level,
                        LPBYTE pForm)
{
    FIXME("(%d,%s,%ld,%p): stub\n",hPrinter,pFormName,Level,pForm);
    return FALSE;
}

/*****************************************************************************
 *          SetForm32W  [WINSPOOL.208]
 */
BOOL WINAPI SetFormW(HANDLE hPrinter, LPWSTR pFormName, DWORD Level,
                        LPBYTE pForm)
{
    FIXME("(%d,%p,%ld,%p): stub\n",hPrinter,pFormName,Level,pForm);
    return FALSE;
}

/*****************************************************************************
 *          ReadPrinter32  [WINSPOOL.202]
 */
BOOL WINAPI ReadPrinter(HANDLE hPrinter, LPVOID pBuf, DWORD cbBuf,
                           LPDWORD pNoBytesRead)
{
    FIXME("(%d,%p,%ld,%p): stub\n",hPrinter,pBuf,cbBuf,pNoBytesRead);
    return FALSE;
}

/*****************************************************************************
 *          ResetPrinter32A  [WINSPOOL.203]
 */
BOOL WINAPI ResetPrinterA(HANDLE hPrinter, LPPRINTER_DEFAULTSA pDefault)
{
    FIXME("(%d, %p): stub\n", hPrinter, pDefault);
    return FALSE;
}

/*****************************************************************************
 *          ResetPrinter32W  [WINSPOOL.204]
 */
BOOL WINAPI ResetPrinterW(HANDLE hPrinter, LPPRINTER_DEFAULTSW pDefault)
{
    FIXME("(%d, %p): stub\n", hPrinter, pDefault);
    return FALSE;
}


/*****************************************************************************
 *    WINSPOOL_GetStringFromRegA
 *
 * Get ValueName from hkey storing result in str.  buflen is space left in str
 */ 
static BOOL WINSPOOL_GetStringFromRegA(HKEY hkey, LPCSTR ValueName, LPSTR ptr,
				       DWORD buflen, DWORD *needed)
{
    DWORD sz = buflen, type;
    LONG ret;

    ret = RegQueryValueExA(hkey, ValueName, 0, &type, ptr, &sz);

    if(ret != ERROR_SUCCESS && ret != ERROR_MORE_DATA) {
        ERR("Got ret = %ld\n", ret);
	return FALSE;
    }
    *needed = sz;
    return TRUE;
}

/*********************************************************************
 *    WINSPOOL_GetPrinter_2A
 *
 * Fills out a PRINTER_INFO_2A struct storing the strings in buf.
 */
static BOOL WINSPOOL_GetPrinter_2A(HKEY hkeyPrinter, PRINTER_INFO_2A *pi2,
				   LPBYTE buf, DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD size, left = cbBuf;
    BOOL space = (cbBuf > 0);
    LPBYTE ptr = buf;

    *pcbNeeded = 0;

    WINSPOOL_GetStringFromRegA(hkeyPrinter, "Name", ptr, left, &size);
    if(space && size <= left) {
        pi2->pPrinterName = ptr;
	ptr += size;
	left -= size;
    } else
        space = FALSE;
    *pcbNeeded += size;

    WINSPOOL_GetStringFromRegA(hkeyPrinter, "Port", ptr, left, &size);
    if(space && size <= left) {
        pi2->pPortName = ptr;
	ptr += size;
	left -= size;
    } else
        space = FALSE;
    *pcbNeeded += size;

    WINSPOOL_GetStringFromRegA(hkeyPrinter, "Printer Driver", ptr, left,
			       &size);
    if(space && size <= left) {
        pi2->pDriverName = ptr;
	ptr += size;
	left -= size;
    } else
        space = FALSE;
    *pcbNeeded += size;

    WINSPOOL_GetStringFromRegA(hkeyPrinter, "Default DevMode", ptr, left,
			       &size);
    if(space && size <= left) {
        pi2->pDevMode = (LPDEVMODEA)ptr;
	ptr += size;
	left -= size;
    } else
        space = FALSE;
    *pcbNeeded += size;

    WINSPOOL_GetStringFromRegA(hkeyPrinter, "Print Processor", ptr, left,
			       &size);
    if(space && size <= left) {
        pi2->pPrintProcessor = ptr;
	ptr += size;
	left -= size;
    } else
	space = FALSE;
    *pcbNeeded += size;

    if(!space && pi2) /* zero out pi2 if we can't completely fill buf */
        memset(pi2, 0, sizeof(*pi2));

    return space;
}

/*********************************************************************
 *    WINSPOOL_GetPrinter_4A
 *
 * Fills out a PRINTER_INFO_4A struct storing the strings in buf.
 */
static BOOL WINSPOOL_GetPrinter_4A(HKEY hkeyPrinter, PRINTER_INFO_4A *pi4,
				   LPBYTE buf, DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD size, left = cbBuf;
    BOOL space = (cbBuf > 0);
    LPBYTE ptr = buf;

    *pcbNeeded = 0;

    WINSPOOL_GetStringFromRegA(hkeyPrinter, "Name", ptr, left, &size);
    if(space && size <= left) {
        pi4->pPrinterName = ptr;
	ptr += size;
	left -= size;
    } else
        space = FALSE;

    *pcbNeeded += size;

    if(!space && pi4) /* zero out pi4 if we can't completely fill buf */
        memset(pi4, 0, sizeof(*pi4));

    return space;
}

/*********************************************************************
 *    WINSPOOL_GetPrinter_5A
 *
 * Fills out a PRINTER_INFO_5A struct storing the strings in buf.
 */
static BOOL WINSPOOL_GetPrinter_5A(HKEY hkeyPrinter, PRINTER_INFO_5A *pi5,
				   LPBYTE buf, DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD size, left = cbBuf;
    BOOL space = (cbBuf > 0);
    LPBYTE ptr = buf;

    *pcbNeeded = 0;

    WINSPOOL_GetStringFromRegA(hkeyPrinter, "Name", ptr, left, &size);
    if(space && size <= left) {
        pi5->pPrinterName = ptr;
	ptr += size;
	left -= size;
    } else
        space = FALSE;
    *pcbNeeded += size;

    WINSPOOL_GetStringFromRegA(hkeyPrinter, "Port", ptr, left, &size);
    if(space && size <= left) {
        pi5->pPortName = ptr;
	ptr += size;
	left -= size;
    } else
	space = FALSE;
    *pcbNeeded += size;

    if(!space && pi5) /* zero out pi5 if we can't completely fill buf */
        memset(pi5, 0, sizeof(*pi5));

    return space;
}

/*****************************************************************************
 *          GetPrinterA  [WINSPOOL.187]
 */
BOOL WINAPI GetPrinterA(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
			DWORD cbBuf, LPDWORD pcbNeeded)
{
    OPENEDPRINTERA *lpOpenedPrinter;
    DWORD size, needed = 0;
    LPBYTE ptr = NULL;
    HKEY hkeyPrinter, hkeyPrinters;
    BOOL ret;

    TRACE("(%d,%ld,%p,%ld,%p)\n",hPrinter,Level,pPrinter,cbBuf, pcbNeeded);

    lpOpenedPrinter = WINSPOOL_GetOpenedPrinterA(hPrinter);
    if(!lpOpenedPrinter) {
        SetLastError(ERROR_INVALID_HANDLE);
	return FALSE;
    }
    if(RegCreateKeyA(HKEY_LOCAL_MACHINE, Printers, &hkeyPrinters) !=
       ERROR_SUCCESS) {
        ERR("Can't create Printers key\n");
	return FALSE;
    }
    if(RegOpenKeyA(hkeyPrinters, lpOpenedPrinter->lpsPrinterName, &hkeyPrinter)
       != ERROR_SUCCESS) {
        ERR("Can't find opened printer `%s' in registry\n",
	    lpOpenedPrinter->lpsPrinterName);
	RegCloseKey(hkeyPrinters);
        SetLastError(ERROR_INVALID_PRINTER_NAME); /* ? */
	return FALSE;
    }

    switch(Level) {
    case 2:
      {
        PRINTER_INFO_2A *pi2 = (PRINTER_INFO_2A *)pPrinter;

        size = sizeof(PRINTER_INFO_2A);
	if(size <= cbBuf) {
	    ptr = pPrinter + size;
	    cbBuf -= size;
	    memset(pPrinter, 0, size);
	} else {
	    pi2 = NULL;
	    cbBuf = 0;
	}
	ret = WINSPOOL_GetPrinter_2A(hkeyPrinter, pi2, ptr, cbBuf, &needed);
	needed += size;
	break;
      }
      
    case 4:
      {
	PRINTER_INFO_4A *pi4 = (PRINTER_INFO_4A *)pPrinter;
	
        size = sizeof(PRINTER_INFO_4A);
	if(size <= cbBuf) {
	    ptr = pPrinter + size;
	    cbBuf -= size;
	    memset(pPrinter, 0, size);
	} else {
	    pi4 = NULL;
	    cbBuf = 0;
	}
	ret = WINSPOOL_GetPrinter_4A(hkeyPrinter, pi4, ptr, cbBuf, &needed);
	needed += size;
	break;
      }


    case 5:
      {
        PRINTER_INFO_5A *pi5 = (PRINTER_INFO_5A *)pPrinter;

        size = sizeof(PRINTER_INFO_5A);
	if(size <= cbBuf) {
	    ptr = pPrinter + size;
	    cbBuf -= size;
	    memset(pPrinter, 0, size);
	} else {
	    pi5 = NULL;
	    cbBuf = 0;
	}

	ret = WINSPOOL_GetPrinter_5A(hkeyPrinter, pi5, ptr, cbBuf, &needed);
	needed += size;
	break;
      }

    default:
        FIXME("Unimplemented level %ld\n", Level);
        SetLastError(ERROR_INVALID_LEVEL);
	RegCloseKey(hkeyPrinters);
	RegCloseKey(hkeyPrinter);
	return FALSE;
    }

    RegCloseKey(hkeyPrinter);
    RegCloseKey(hkeyPrinters);

    if(pcbNeeded) *pcbNeeded = needed;
    if(!ret)
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return ret;
}


/*****************************************************************************
 *          GetPrinterW  [WINSPOOL.194]
 */
BOOL WINAPI GetPrinterW(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
                    DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME("(%d,%ld,%p,%ld,%p): stub\n", hPrinter, Level, pPrinter,
          cbBuf, pcbNeeded);
    return FALSE;
}

/******************************************************************
 *              EnumPrintersA        [WINSPOOL.174]
 *
 *    Enumerates the available printers, print servers and print
 *    providers, depending on the specified flags, name and level.
 *
 * RETURNS:
 *
 *    If level is set to 1:
 *      Not implemented yet! 
 *      Returns TRUE with an empty list.
 *
 *    If level is set to 2:
 *		Possible flags: PRINTER_ENUM_CONNECTIONS, PRINTER_ENUM_LOCAL.
 *      Returns an array of PRINTER_INFO_2 data structures in the 
 *      lpbPrinters buffer. Note that according to MSDN also an 
 *      OpenPrinter should be performed on every remote printer.
 *
 *    If level is set to 4 (officially WinNT only):
 *		Possible flags: PRINTER_ENUM_CONNECTIONS, PRINTER_ENUM_LOCAL.
 *      Fast: Only the registry is queried to retrieve printer names,
 *      no connection to the driver is made.
 *      Returns an array of PRINTER_INFO_4 data structures in the 
 *      lpbPrinters buffer.
 *
 *    If level is set to 5 (officially WinNT4/Win9x only):
 *      Fast: Only the registry is queried to retrieve printer names,
 *      no connection to the driver is made.
 *      Returns an array of PRINTER_INFO_5 data structures in the 
 *      lpbPrinters buffer.
 *
 *    If level set to 3 or 6+:
 *	    returns zero (faillure!)
 *      
 *    Returns nonzero (TRUE) on succes, or zero on faillure, use GetLastError
 *    for information.
 *
 * BUGS:
 *    - Only PRINTER_ENUM_LOCAL and PRINTER_ENUM_NAME are implemented.
 *    - Only levels 2, 4 and 5 are implemented at the moment.
 *    - 16-bit printer drivers are not enumerated.
 *    - Returned amount of bytes used/needed does not match the real Windoze 
 *      implementation (as in this implementation, all strings are part 
 *      of the buffer, whereas Win32 keeps them somewhere else)
 *    - At level 2, EnumPrinters should also call OpenPrinter for remote printers.
 *
 * NOTE:
 *    - In a regular Wine installation, no registry settings for printers
 *      exist, which makes this function return an empty list.
 */
BOOL  WINAPI EnumPrintersA(
		DWORD dwType,        /* Types of print objects to enumerate */
                LPSTR lpszName,      /* name of objects to enumerate */
	        DWORD dwLevel,       /* type of printer info structure */
                LPBYTE lpbPrinters,  /* buffer which receives info */
		DWORD cbBuf,         /* max size of buffer in bytes */
		LPDWORD lpdwNeeded,  /* pointer to var: # bytes used/needed */
		LPDWORD lpdwReturned /* number of entries returned */
		)

{
    HKEY hkeyPrinters, hkeyPrinter;
    char PrinterName[255];
    DWORD needed = 0, number = 0;
    DWORD used, i, left;
    PBYTE pi, buf;

    if(lpbPrinters)
        memset(lpbPrinters, 0, cbBuf);
    if(lpdwReturned)
        *lpdwReturned = 0;

    if (!((dwType & PRINTER_ENUM_LOCAL) || (dwType & PRINTER_ENUM_NAME))) {
        FIXME("dwType = %08lx\n", dwType);
	SetLastError(ERROR_INVALID_FLAGS);
	return FALSE;
    }

    if(RegCreateKeyA(HKEY_LOCAL_MACHINE, Printers, &hkeyPrinters) !=
       ERROR_SUCCESS) {
        ERR("Can't create Printers key\n");
	return FALSE;
    }
  
    if(RegQueryInfoKeyA(hkeyPrinters, NULL, NULL, NULL, &number, NULL, NULL,
			NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
        RegCloseKey(hkeyPrinters);
	ERR("Can't query Printers key\n");
	return FALSE;
    }
    TRACE("Found %ld printers\n", number);

    switch(dwLevel) {
    case 1:
        RegCloseKey(hkeyPrinters);
	if (lpdwReturned)
	    *lpdwReturned = number;
	return TRUE;

    case 2:
        used = number * sizeof(PRINTER_INFO_2A);
	break;
    case 4:
        used = number * sizeof(PRINTER_INFO_4A);
	break;
    case 5:
        used = number * sizeof(PRINTER_INFO_5A);
	break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
	RegCloseKey(hkeyPrinters);
	return FALSE;
    }
    pi = (used <= cbBuf) ? lpbPrinters : NULL;

    for(i = 0; i < number; i++) {
        if(RegEnumKeyA(hkeyPrinters, i, PrinterName, sizeof(PrinterName)) != 
	   ERROR_SUCCESS) {
	    ERR("Can't enum key number %ld\n", i);
	    RegCloseKey(hkeyPrinters);
	    return FALSE;
	}
	if(RegOpenKeyA(hkeyPrinters, PrinterName, &hkeyPrinter) !=
	   ERROR_SUCCESS) {
	    ERR("Can't open key '%s'\n", PrinterName);
	    RegCloseKey(hkeyPrinters);
	    return FALSE;
	}

	if(cbBuf > used) {
	    buf = lpbPrinters + used;
	    left = cbBuf - used;
	} else {
	    buf = NULL;
	    left = 0;
	}

	switch(dwLevel) {
	case 2:
	    WINSPOOL_GetPrinter_2A(hkeyPrinter, (PRINTER_INFO_2A *)pi, buf,
				   left, &needed);
	    used += needed;
	    if(pi) pi += sizeof(PRINTER_INFO_2A);
	    break;
	case 4:
	    WINSPOOL_GetPrinter_4A(hkeyPrinter, (PRINTER_INFO_4A *)pi, buf,
				   left, &needed);
	    used += needed;
	    if(pi) pi += sizeof(PRINTER_INFO_4A);
	    break;
	case 5:
	    WINSPOOL_GetPrinter_5A(hkeyPrinter, (PRINTER_INFO_5A *)pi, buf,
				   left, &needed);
	    used += needed;
	    if(pi) pi += sizeof(PRINTER_INFO_5A);
	    break;
	default:
	    ERR("Shouldn't be here!\n");
	    RegCloseKey(hkeyPrinter);
	    RegCloseKey(hkeyPrinters);
	    return FALSE;
	}
	RegCloseKey(hkeyPrinter);
    }
    RegCloseKey(hkeyPrinters);

    if(lpdwNeeded)
        *lpdwNeeded = used;
    if(used > cbBuf) {
        if(lpbPrinters)
	    memset(lpbPrinters, 0, cbBuf);
	SetLastError(ERROR_INSUFFICIENT_BUFFER);
	return FALSE;
    }
    if(lpdwReturned)
        *lpdwReturned = number;  
    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

/******************************************************************
 *              EnumPrintersW        [WINSPOOL.175]
 *
 */
BOOL  WINAPI EnumPrintersW(DWORD dwType, LPWSTR lpszName,
			       DWORD dwLevel, LPBYTE lpbPrinters,
			       DWORD cbBuf, LPDWORD lpdwNeeded,
			       LPDWORD lpdwReturned)
{
    FIXME("Nearly empty stub\n");
    *lpdwReturned=0;
    *lpdwNeeded = 0;
    return TRUE;
}


/*****************************************************************************
 *          GetPrinterDriver32A  [WINSPOOL.190]
 */
BOOL WINAPI GetPrinterDriverA(HANDLE hPrinter, LPSTR pEnvironment,
			      DWORD Level, LPBYTE pDriverInfo,
			      DWORD cbBuf, LPDWORD pcbNeeded)
{
    OPENEDPRINTERA *lpOpenedPrinter;
    char DriverName[100];
    DWORD ret, type, size, dw, needed = 0;
    LPBYTE ptr = NULL;
    HKEY hkeyPrinter, hkeyPrinters, hkeyDriver, hkeyDrivers;
    
    TRACE("(%d,%s,%ld,%p,%ld,%p)\n",hPrinter,pEnvironment,
         Level,pDriverInfo,cbBuf, pcbNeeded);

    ZeroMemory(pDriverInfo, cbBuf);

    lpOpenedPrinter = WINSPOOL_GetOpenedPrinterA(hPrinter);
    if(!lpOpenedPrinter) {
        SetLastError(ERROR_INVALID_HANDLE);
	return FALSE;
    }
    if(pEnvironment) {
        FIXME("pEnvironment = `%s'\n", pEnvironment);
	SetLastError(ERROR_INVALID_ENVIRONMENT);
	return FALSE;
    }
    if(Level < 1 || Level > 3) {
        SetLastError(ERROR_INVALID_LEVEL);
	return FALSE;
    }
    if(RegCreateKeyA(HKEY_LOCAL_MACHINE, Printers, &hkeyPrinters) !=
       ERROR_SUCCESS) {
        ERR("Can't create Printers key\n");
	return FALSE;
    }
    if(RegOpenKeyA(hkeyPrinters, lpOpenedPrinter->lpsPrinterName, &hkeyPrinter)
       != ERROR_SUCCESS) {
        ERR("Can't find opened printer `%s' in registry\n",
	    lpOpenedPrinter->lpsPrinterName);
	RegCloseKey(hkeyPrinters);
        SetLastError(ERROR_INVALID_PRINTER_NAME); /* ? */
	return FALSE;
    }
    size = sizeof(DriverName);
    ret = RegQueryValueExA(hkeyPrinter, "Printer Driver", 0, &type, DriverName,
			   &size);
    RegCloseKey(hkeyPrinter);
    RegCloseKey(hkeyPrinters);
    if(ret != ERROR_SUCCESS) {
        ERR("Can't get DriverName for printer `%s'\n",
	    lpOpenedPrinter->lpsPrinterName);
	return FALSE;
    }
    if(RegCreateKeyA(HKEY_LOCAL_MACHINE, Drivers, &hkeyDrivers) !=
       ERROR_SUCCESS) {
        ERR("Can't create Drivers key\n");
	return FALSE;
    }
    if(RegOpenKeyA(hkeyDrivers, DriverName, &hkeyDriver)
       != ERROR_SUCCESS) {
        ERR("Can't find driver `%s' in registry\n", DriverName);
	RegCloseKey(hkeyDrivers);
        SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER); /* ? */
	return FALSE;
    }

    switch(Level) {
    case 1:
        size = sizeof(DRIVER_INFO_1A);
	break;
    case 2:
        size = sizeof(DRIVER_INFO_2A);
	break;
    case 3:
        size = sizeof(DRIVER_INFO_3A);
	break;
    default:
        ERR("Invalid level\n");
	return FALSE;
    }

    if(size <= cbBuf) {
        ptr = pDriverInfo + size;
	cbBuf -= size;
    } else
        cbBuf = 0;
    needed = size;

    size = strlen(DriverName) + 1;
    if(size <= cbBuf) {
        cbBuf -= size;
        strcpy(ptr, DriverName);
        if(Level == 1)
	    ((DRIVER_INFO_1A *)pDriverInfo)->pName = ptr;
	else
	    ((DRIVER_INFO_2A *)pDriverInfo)->pName = ptr;    
	ptr += size;
    }
    needed += size;

    if(Level > 1) {
        DRIVER_INFO_2A *di2 = (DRIVER_INFO_2A *)pDriverInfo;

	size = sizeof(dw);
        if(RegQueryValueExA(hkeyDriver, "Version", 0, &type, (PBYTE)&dw,
			    &size) !=
	   ERROR_SUCCESS)
	    WARN("Can't get Version\n");
	else if(cbBuf)
	    di2->cVersion = dw;

	size = strlen("Wine") + 1;  /* FIXME */
	if(size <= cbBuf) {
	    cbBuf -= size;
	    strcpy(ptr, "Wine");
	    di2->pEnvironment = ptr;
	    ptr += size;
	} else 
	    cbBuf = 0;
	needed += size;

	WINSPOOL_GetStringFromRegA(hkeyDriver, "Driver", ptr, cbBuf, &size);
	if(cbBuf && size <= cbBuf) {
	    di2->pDriverPath = ptr;
	    ptr += size;
	} else
	    cbBuf = 0;
	needed += size;

	WINSPOOL_GetStringFromRegA(hkeyDriver, "Data File", ptr, cbBuf, &size);
	if(cbBuf && size <= cbBuf) {
	    di2->pDataFile = ptr;
	    ptr += size;
	} else
	    cbBuf = 0;
	needed += size;

	WINSPOOL_GetStringFromRegA(hkeyDriver, "Configuration File", ptr,
				   cbBuf, &size);
	if(cbBuf && size <= cbBuf) {
	    di2->pConfigFile = ptr;
	    ptr += size;
	} else
	    cbBuf = 0;
	needed += size;
    }

    if(Level > 2) {
        DRIVER_INFO_3A *di3 = (DRIVER_INFO_3A *)pDriverInfo;

	WINSPOOL_GetStringFromRegA(hkeyDriver, "Help File", ptr, cbBuf, &size);
	if(cbBuf && size <= cbBuf) {
	    di3->pHelpFile = ptr;
	    ptr += size;
	} else
	    cbBuf = 0;
	needed += size;

	WINSPOOL_GetStringFromRegA(hkeyDriver, "Dependent Files", ptr, cbBuf,
				   &size);
	if(cbBuf && size <= cbBuf) {
	    di3->pDependentFiles = ptr;
	    ptr += size;
	} else
	    cbBuf = 0;
	needed += size;

	WINSPOOL_GetStringFromRegA(hkeyDriver, "Monitor", ptr, cbBuf, &size);
	if(cbBuf && size <= cbBuf) {
	    di3->pMonitorName = ptr;
	    ptr += size;
	} else
	    cbBuf = 0;
	needed += size;

	WINSPOOL_GetStringFromRegA(hkeyDriver, "DataType", ptr, cbBuf, &size);
	if(cbBuf && size <= cbBuf) {
	    di3->pDefaultDataType = ptr;
	    ptr += size;
	} else
	    cbBuf = 0;
	needed += size;
    }
    RegCloseKey(hkeyDriver);
    RegCloseKey(hkeyDrivers);

    if(pcbNeeded) *pcbNeeded = needed;
    if(cbBuf) return TRUE;
    SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return FALSE;
}

/*****************************************************************************
 *          GetPrinterDriver32W  [WINSPOOL.193]
 */
BOOL WINAPI GetPrinterDriverW(HANDLE hPrinter, LPWSTR pEnvironment,
                                  DWORD Level, LPBYTE pDriverInfo, 
                                  DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME("(%d,%p,%ld,%p,%ld,%p): stub\n",hPrinter,pEnvironment,
          Level,pDriverInfo,cbBuf, pcbNeeded);
    return FALSE;
}

/*****************************************************************************
 *       GetPrinterDriverDirectoryA  [WINSPOOL.191]
 */
BOOL WINAPI GetPrinterDriverDirectoryA(LPSTR pName, LPSTR pEnvironment,
				       DWORD Level, LPBYTE pDriverDirectory,
				       DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD needed;

    TRACE("(%s, %s, %ld, %p, %ld, %p)\n", pName, pEnvironment, Level,
	  pDriverDirectory, cbBuf, pcbNeeded);
    if(pName != NULL) {
        FIXME("pName = `%s' - unsupported\n", pName);
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
    if(pEnvironment != NULL) {
        FIXME("pEnvironment = `%s' - unsupported\n", pEnvironment);
	SetLastError(ERROR_INVALID_ENVIRONMENT);
	return FALSE;
    }
    if(Level != 1)  /* win95 ignores this so we just carry on */
        WARN("Level = %ld - assuming 1\n", Level);
    
    /* FIXME should read from registry */
    needed = GetSystemDirectoryA(pDriverDirectory, cbBuf);
    needed++;
    if(pcbNeeded)
        *pcbNeeded = needed;
    if(needed > cbBuf) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
	return FALSE;
    }
    return TRUE;
}


/*****************************************************************************
 *       GetPrinterDriverDirectoryW  [WINSPOOL.192]
 */
BOOL WINAPI GetPrinterDriverDirectoryW(LPWSTR pName, LPWSTR pEnvironment,
				       DWORD Level, LPBYTE pDriverDirectory,
				       DWORD cbBuf, LPDWORD pcbNeeded)
{
    LPSTR pNameA = NULL, pEnvironmentA = NULL;
    BOOL ret;

    if(pName)
        pNameA = HEAP_strdupWtoA( GetProcessHeap(), 0, pName );
    if(pEnvironment)
        pEnvironmentA = HEAP_strdupWtoA( GetProcessHeap(), 0, pEnvironment );
    ret = GetPrinterDriverDirectoryA( pNameA, pEnvironmentA, Level,
				      pDriverDirectory, cbBuf, pcbNeeded );
    if(pNameA)
        HeapFree( GetProcessHeap(), 0, pNameA );
    if(pEnvironmentA)
        HeapFree( GetProcessHeap(), 0, pEnvironment );

    return ret;
}

/*****************************************************************************
 *          AddPrinterDriver32A  [WINSPOOL.120]
 */
BOOL WINAPI AddPrinterDriverA(LPSTR pName, DWORD level, LPBYTE pDriverInfo)
{
    DRIVER_INFO_3A di3;
    HKEY hkeyDrivers, hkeyName;

    TRACE("(%s,%ld,%p)\n",pName,level,pDriverInfo);

    if(level != 2 && level != 3) {
        SetLastError(ERROR_INVALID_LEVEL);
	return FALSE;
    }
    if(pName != NULL) {
        FIXME("pName= `%s' - unsupported\n", pName);
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
    if(!pDriverInfo) {
        WARN("pDriverInfo == NULL");
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
    
    if(level == 3)
        di3 = *(DRIVER_INFO_3A *)pDriverInfo;
    else {
        memset(&di3, 0, sizeof(di3));
        *(DRIVER_INFO_2A *)&di3 = *(DRIVER_INFO_2A *)pDriverInfo;
    }

    if(!di3.pName || !di3.pDriverPath || !di3.pConfigFile ||
       !di3.pDataFile) {
        SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
    if(!di3.pDefaultDataType) di3.pDefaultDataType = "";
    if(!di3.pDependentFiles) di3.pDependentFiles = "\0";
    if(!di3.pHelpFile) di3.pHelpFile = "";
    if(!di3.pMonitorName) di3.pMonitorName = "";

    if(di3.pEnvironment) {
        FIXME("pEnvironment = `%s'\n", di3.pEnvironment);
	SetLastError(ERROR_INVALID_ENVIRONMENT);
	return FALSE;
    }
    if(RegCreateKeyA(HKEY_LOCAL_MACHINE, Drivers, &hkeyDrivers) !=
       ERROR_SUCCESS) {
        ERR("Can't create Drivers key\n");
	return FALSE;
    }

    if(level == 2) { /* apparently can't overwrite with level2 */
        if(RegOpenKeyA(hkeyDrivers, di3.pName, &hkeyName) == ERROR_SUCCESS) {
	    RegCloseKey(hkeyName);
	    RegCloseKey(hkeyDrivers);
	    WARN("Trying to create existing printer driver `%s'\n", di3.pName);
	    SetLastError(ERROR_PRINTER_DRIVER_ALREADY_INSTALLED);
	    return FALSE;
	}
    }
    if(RegCreateKeyA(hkeyDrivers, di3.pName, &hkeyName) != ERROR_SUCCESS) {
	RegCloseKey(hkeyDrivers);
	ERR("Can't create Name key\n");
	return FALSE;
    }
    RegSetValueExA(hkeyName, "Configuration File", 0, REG_SZ, di3.pConfigFile,
		   0);
    RegSetValueExA(hkeyName, "Data File", 0, REG_SZ, di3.pDataFile, 0);
    RegSetValueExA(hkeyName, "Driver", 0, REG_SZ, di3.pDriverPath, 0);
    RegSetValueExA(hkeyName, "Version", 0, REG_DWORD, (LPSTR)&di3.cVersion, 
		   sizeof(DWORD));
    RegSetValueExA(hkeyName, "Datatype", 0, REG_SZ, di3.pDefaultDataType, 0);
    RegSetValueExA(hkeyName, "Dependent Files", 0, REG_MULTI_SZ,
		   di3.pDependentFiles, 0);
    RegSetValueExA(hkeyName, "Help File", 0, REG_SZ, di3.pHelpFile, 0);
    RegSetValueExA(hkeyName, "Monitor", 0, REG_SZ, di3.pMonitorName, 0);
    RegCloseKey(hkeyName);
    RegCloseKey(hkeyDrivers);

    return TRUE;
}
/*****************************************************************************
 *          AddPrinterDriver32W  [WINSPOOL.121]
 */
BOOL WINAPI AddPrinterDriverW(LPWSTR printerName,DWORD level, 
				   LPBYTE pDriverInfo)
{
    FIXME("(%s,%ld,%p): stub\n",debugstr_w(printerName),
	  level,pDriverInfo);
    return FALSE;
}


/*****************************************************************************
 *          PrinterProperties  [WINSPOOL.201]
 *
 *     Displays a dialog to set the properties of the printer.
 *
 * RETURNS 
 *     nonzero on succes or zero on faillure
 *
 * BUGS
 *	   implemented as stub only
 */
BOOL WINAPI PrinterProperties(HWND hWnd,      /* handle to parent window */
                              HANDLE hPrinter /* handle to printer object */
){
    FIXME("(%d,%d): stub\n", hWnd, hPrinter);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*****************************************************************************
 *          EnumJobsA [WINSPOOL.162]
 *
 */
BOOL WINAPI EnumJobsA(HANDLE hPrinter, DWORD FirstJob, DWORD NoJobs,
		      DWORD Level, LPBYTE pJob, DWORD cbBuf, LPDWORD pcbNeeded,
		      LPDWORD pcReturned)
{
    FIXME("stub\n");
    if(pcbNeeded) *pcbNeeded = 0;
    if(pcReturned) *pcReturned = 0;
    return TRUE;
}


/*****************************************************************************
 *          EnumJobsW [WINSPOOL.163]
 *
 */
BOOL WINAPI EnumJobsW(HANDLE hPrinter, DWORD FirstJob, DWORD NoJobs,
		      DWORD Level, LPBYTE pJob, DWORD cbBuf, LPDWORD pcbNeeded,
		      LPDWORD pcReturned)
{
    FIXME("stub\n");
    if(pcbNeeded) *pcbNeeded = 0;
    if(pcReturned) *pcReturned = 0;
    return TRUE;
}
