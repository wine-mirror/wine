/* 
 * WINSPOOL functions
 * 
 * Copyright 1996 John Harvey
 * Copyright 1998 Andreas Mohr
 * Copyright 1999 Klaas van Gend
 * Copyright 1999, 2000 Huw D M Davies
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include "winspool.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "debugtools.h"
#include "heap.h"
#include "commctrl.h"
#include "winnls.h"

DEFAULT_DEBUG_CHANNEL(winspool)

CRITICAL_SECTION PRINT32_RegistryBlocker;

typedef struct _OPENEDPRINTER
{
    LPWSTR lpsPrinterName;
    HANDLE hPrinter;
} OPENEDPRINTER, *LPOPENEDPRINTER;

/* The OpenedPrinter Table dynamic array */
static HDPA pOpenedPrinterDPA = NULL;

extern HDPA   (WINAPI* WINSPOOL_DPA_CreateEx) (INT, HANDLE);
extern LPVOID (WINAPI* WINSPOOL_DPA_GetPtr) (const HDPA, INT);
extern INT    (WINAPI* WINSPOOL_DPA_InsertPtr) (const HDPA, INT, LPVOID);

static char Printers[] =
"System\\CurrentControlSet\\control\\Print\\Printers\\";
static char Drivers[] =
"System\\CurrentControlSet\\control\\Print\\Environments\\Windows 4.0\\Drivers\\"; /* Hmm, well */

static WCHAR DefaultEnvironmentW[] = {'W','i','n','e',0};

static WCHAR Configuration_FileW[] = {'C','o','n','f','i','g','u','r','a','t',
				      'i','o','n',' ','F','i','l','e',0};
static WCHAR DatatypeW[] = {'D','a','t','a','t','y','p','e',0};
static WCHAR Data_FileW[] = {'D','a','t','a',' ','F','i','l','e',0};
static WCHAR Default_DevModeW[] = {'D','e','f','a','u','l','t',' ','D','e','v',
				   'M','o','d','e',0};
static WCHAR Dependent_FilesW[] = {'D','e','p','e','n','d','e','n','t',' ','F',
				   'i','l','e','s',0};
static WCHAR DescriptionW[] = {'D','e','s','c','r','i','p','t','i','o','n',0};
static WCHAR DriverW[] = {'D','r','i','v','e','r',0};
static WCHAR Help_FileW[] = {'H','e','l','p',' ','F','i','l','e',0};
static WCHAR LocationW[] = {'L','o','c','a','t','i','o','n',0};
static WCHAR MonitorW[] = {'M','o','n','i','t','o','r',0};
static WCHAR NameW[] = {'N','a','m','e',0};
static WCHAR ParametersW[] = {'P','a','r','a','m','e','t','e','r','s',0}; 
static WCHAR PortW[] = {'P','o','r','t',0};
static WCHAR Print_ProcessorW[] = {'P','r','i','n','t',' ','P','r','o','c','e',
				   's','s','o','r',0};
static WCHAR Printer_DriverW[] = {'P','r','i','n','t','e','r',' ','D','r','i',
				  'v','e','r',0};
static WCHAR Separator_FileW[] = {'S','e','p','a','r','a','t','o','r',' ','F',
				  'i','l','e',0};
static WCHAR Share_NameW[] = {'S','h','a','r','e',' ','N','a','m','e',0};
static WCHAR WinPrintW[] = {'W','i','n','P','r','i','n','t',0};

/******************************************************************
 *  WINSPOOL_GetOpenedPrinterEntry
 *  Get the first place empty in the opened printer table
 */
static LPOPENEDPRINTER WINSPOOL_GetOpenedPrinterEntry()
{
    int i;
    LPOPENEDPRINTER pOpenedPrinter;

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
                                       sizeof(OPENEDPRINTER));
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
                                   sizeof(OPENEDPRINTER));
        pOpenedPrinter->hPrinter = i + 1;
        WINSPOOL_DPA_InsertPtr(pOpenedPrinterDPA, i, pOpenedPrinter);
        return pOpenedPrinter;
    }

    return NULL;
}

/******************************************************************
 *  WINSPOOL_GetOpenedPrinter
 *  Get the pointer to the opened printer referred by the handle
 */
static LPOPENEDPRINTER WINSPOOL_GetOpenedPrinter(int printerHandle)
{
    LPOPENEDPRINTER pOpenedPrinter;

    if(!pOpenedPrinterDPA) return NULL;
    if((printerHandle <=0) || 
       (printerHandle > (pOpenedPrinterDPA->nItemCount - 1)))
        return NULL;

    pOpenedPrinter = WINSPOOL_DPA_GetPtr(pOpenedPrinterDPA, printerHandle-1);

    return pOpenedPrinter;
}

/***********************************************************
 *      DEVMODEcpyAtoW
 */
static LPDEVMODEW DEVMODEcpyAtoW(DEVMODEW *dmW, const DEVMODEA *dmA)
{
    BOOL Formname;
    ptrdiff_t off_formname = (char *)dmA->dmFormName - (char *)dmA;
    DWORD size;

    Formname = (dmA->dmSize > off_formname);
    size = dmA->dmSize + CCHDEVICENAME + (Formname ? CCHFORMNAME : 0);
    MultiByteToWideChar(CP_ACP, 0, dmA->dmDeviceName, -1, dmW->dmDeviceName,
			CCHDEVICENAME);
    if(!Formname) {
      memcpy(&dmW->dmSpecVersion, &dmA->dmSpecVersion,
	     dmA->dmSize - CCHDEVICENAME);
    } else {
      memcpy(&dmW->dmSpecVersion, &dmA->dmSpecVersion,
	     off_formname - CCHDEVICENAME);
      MultiByteToWideChar(CP_ACP, 0, dmA->dmFormName, -1, dmW->dmFormName,
			  CCHFORMNAME);
      memcpy(&dmW->dmLogPixels, &dmA->dmLogPixels, dmA->dmSize -
	     (off_formname + CCHFORMNAME));
    }
    dmW->dmSize = size;
    memcpy((char *)dmW + dmW->dmSize, (char *)dmA + dmA->dmSize,
	   dmA->dmDriverExtra);
    return dmW;
}

/***********************************************************
 *      DEVMODEdupAtoW
 * Creates a unicode copy of supplied devmode on heap
 */
static LPDEVMODEW DEVMODEdupAtoW(HANDLE heap, const DEVMODEA *dmA)
{
    LPDEVMODEW dmW;
    DWORD size;
    BOOL Formname;
    ptrdiff_t off_formname;

    TRACE("\n");
    if(!dmA) return NULL;

    off_formname = (char *)dmA->dmFormName - (char *)dmA;
    Formname = (dmA->dmSize > off_formname);
    size = dmA->dmSize + CCHDEVICENAME + (Formname ? CCHFORMNAME : 0);
    dmW = HeapAlloc(heap, HEAP_ZERO_MEMORY, size + dmA->dmDriverExtra);
    return DEVMODEcpyAtoW(dmW, dmA);
}

/***********************************************************
 *      DEVMODEdupWtoA
 * Creates an ascii copy of supplied devmode on heap
 */
static LPDEVMODEA DEVMODEdupWtoA(HANDLE heap, const DEVMODEW *dmW)
{
    LPDEVMODEA dmA;
    DWORD size;
    BOOL Formname;
    ptrdiff_t off_formname = (char *)dmW->dmFormName - (char *)dmW;

    if(!dmW) return NULL;
    Formname = (dmW->dmSize > off_formname);
    size = dmW->dmSize - CCHDEVICENAME - (Formname ? CCHFORMNAME : 0);
    dmA = HeapAlloc(heap, HEAP_ZERO_MEMORY, size + dmW->dmDriverExtra);
    WideCharToMultiByte(CP_ACP, 0, dmW->dmDeviceName, -1, dmA->dmDeviceName,
			CCHDEVICENAME, NULL, NULL);
    if(!Formname) {
      memcpy(&dmA->dmSpecVersion, &dmW->dmSpecVersion,
	     dmW->dmSize - CCHDEVICENAME * sizeof(WCHAR));
    } else {
      memcpy(&dmA->dmSpecVersion, &dmW->dmSpecVersion,
	     off_formname - CCHDEVICENAME * sizeof(WCHAR));
      WideCharToMultiByte(CP_ACP, 0, dmW->dmFormName, -1, dmA->dmFormName,
			  CCHFORMNAME, NULL, NULL);
      memcpy(&dmA->dmLogPixels, &dmW->dmLogPixels, dmW->dmSize -
	     (off_formname + CCHFORMNAME * sizeof(WCHAR)));
    }
    dmA->dmSize = size;
    memcpy((char *)dmA + dmA->dmSize, (char *)dmW + dmW->dmSize,
	   dmW->dmDriverExtra);
    return dmA;
}

/***********************************************************
 *             PRINTER_INFO_2AtoW
 * Creates a unicode copy of PRINTER_INFO_2A on heap
 */
static LPPRINTER_INFO_2W PRINTER_INFO_2AtoW(HANDLE heap, LPPRINTER_INFO_2A piA)
{
    LPPRINTER_INFO_2W piW;
    if(!piA) return NULL;
    piW = HeapAlloc(heap, 0, sizeof(*piW));
    memcpy(piW, piA, sizeof(*piW)); /* copy everything first */
    piW->pServerName = HEAP_strdupAtoW(heap, 0, piA->pServerName);
    piW->pPrinterName = HEAP_strdupAtoW(heap, 0, piA->pPrinterName);
    piW->pShareName = HEAP_strdupAtoW(heap, 0, piA->pShareName);
    piW->pPortName = HEAP_strdupAtoW(heap, 0, piA->pPortName);
    piW->pDriverName = HEAP_strdupAtoW(heap, 0, piA->pDriverName);
    piW->pComment = HEAP_strdupAtoW(heap, 0, piA->pComment);
    piW->pLocation = HEAP_strdupAtoW(heap, 0, piA->pLocation);
    piW->pDevMode = DEVMODEdupAtoW(heap, piA->pDevMode);
    piW->pSepFile = HEAP_strdupAtoW(heap, 0, piA->pSepFile);
    piW->pPrintProcessor = HEAP_strdupAtoW(heap, 0, piA->pPrintProcessor);
    piW->pDatatype = HEAP_strdupAtoW(heap, 0, piA->pDatatype);
    piW->pParameters = HEAP_strdupAtoW(heap, 0, piA->pParameters);
    return piW;
}

/***********************************************************
 *       FREE_PRINTER_INFO_2W
 * Free PRINTER_INFO_2W and all strings
 */
static void FREE_PRINTER_INFO_2W(HANDLE heap, LPPRINTER_INFO_2W piW)
{
    if(!piW) return;

    HeapFree(heap,0,piW->pServerName);
    HeapFree(heap,0,piW->pPrinterName);
    HeapFree(heap,0,piW->pShareName);
    HeapFree(heap,0,piW->pPortName);
    HeapFree(heap,0,piW->pDriverName);
    HeapFree(heap,0,piW->pComment);
    HeapFree(heap,0,piW->pLocation);
    HeapFree(heap,0,piW->pDevMode);
    HeapFree(heap,0,piW->pSepFile);
    HeapFree(heap,0,piW->pPrintProcessor);
    HeapFree(heap,0,piW->pDatatype);
    HeapFree(heap,0,piW->pParameters);
    HeapFree(heap,0,piW);
    return;
}

/******************************************************************
 *              DeviceCapabilitiesA    [WINSPOOL.150 & WINSPOOL.151]
 *
 */
INT WINAPI DeviceCapabilitiesA(LPCSTR pDevice,LPCSTR pPort, WORD cap,
			       LPSTR pOutput, LPDEVMODEA lpdm)
{
    INT ret;
    ret = GDI_CallDeviceCapabilities16(pDevice, pPort, cap, pOutput, lpdm);

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
 *          DeviceCapabilitiesW        [WINSPOOL.152]
 *
 * Call DeviceCapabilitiesA since we later call 16bit stuff anyway
 *
 */
INT WINAPI DeviceCapabilitiesW(LPCWSTR pDevice, LPCWSTR pPort,
			       WORD fwCapability, LPWSTR pOutput,
			       const DEVMODEW *pDevMode)
{
    LPDEVMODEA dmA = DEVMODEdupWtoA(GetProcessHeap(), pDevMode);
    LPSTR pDeviceA = HEAP_strdupWtoA(GetProcessHeap(),0,pDevice);
    LPSTR pPortA = HEAP_strdupWtoA(GetProcessHeap(),0,pPort);
    INT ret;

    if(pOutput && (fwCapability == DC_BINNAMES ||
		   fwCapability == DC_FILEDEPENDENCIES ||
		   fwCapability == DC_PAPERNAMES)) {
      /* These need A -> W translation */
        INT size = 0, i;
	LPSTR pOutputA;
        ret = DeviceCapabilitiesA(pDeviceA, pPortA, fwCapability, NULL,
				  dmA);
	if(ret == -1)
	    return ret;
	switch(fwCapability) {
	case DC_BINNAMES:
	    size = 24;
	    break;
	case DC_PAPERNAMES:
	case DC_FILEDEPENDENCIES:
	    size = 64;
	    break;
	}
	pOutputA = HeapAlloc(GetProcessHeap(), 0, size * ret);
	ret = DeviceCapabilitiesA(pDeviceA, pPortA, fwCapability, pOutputA,
				  dmA);
	for(i = 0; i < ret; i++)
	    MultiByteToWideChar(CP_ACP, 0, pOutputA + (i * size), -1,
				pOutput + (i * size), size);
	HeapFree(GetProcessHeap(), 0, pOutputA);
    } else {
        ret = DeviceCapabilitiesA(pDeviceA, pPortA, fwCapability,
				  (LPSTR)pOutput, dmA);
    }
    HeapFree(GetProcessHeap(),0,pPortA);
    HeapFree(GetProcessHeap(),0,pDeviceA);
    HeapFree(GetProcessHeap(),0,dmA);
    return ret;
}

/******************************************************************
 *              DocumentPropertiesA   [WINSPOOL.155]
 *
 */
LONG WINAPI DocumentPropertiesA(HWND hWnd,HANDLE hPrinter,
                                LPSTR pDeviceName, LPDEVMODEA pDevModeOutput,
				LPDEVMODEA pDevModeInput,DWORD fMode )
{
    LPOPENEDPRINTER lpOpenedPrinter;
    LPSTR lpName = pDeviceName;
    LONG ret;

    TRACE("(%d,%d,%s,%p,%p,%ld)\n",
	hWnd,hPrinter,pDeviceName,pDevModeOutput,pDevModeInput,fMode
    );

    if(!pDeviceName) {
        LPWSTR lpNameW;
        lpOpenedPrinter = WINSPOOL_GetOpenedPrinter(hPrinter);
	if(!lpOpenedPrinter) {
	    SetLastError(ERROR_INVALID_HANDLE);
	    return -1;
	}
	lpNameW = lpOpenedPrinter->lpsPrinterName;
	lpName = HEAP_strdupWtoA(GetProcessHeap(),0,lpNameW);
    }

    ret = GDI_CallExtDeviceMode16(hWnd, pDevModeOutput, lpName, "LPT1:",
				  pDevModeInput, NULL, fMode);

    if(!pDeviceName)
        HeapFree(GetProcessHeap(),0,lpName);
    return ret;
}


/*****************************************************************************
 *          DocumentPropertiesW 
 */
LONG WINAPI DocumentPropertiesW(HWND hWnd, HANDLE hPrinter,
				LPWSTR pDeviceName,
				LPDEVMODEW pDevModeOutput,
				LPDEVMODEW pDevModeInput, DWORD fMode)
{

    LPSTR pDeviceNameA = HEAP_strdupWtoA(GetProcessHeap(),0,pDeviceName);
    LPDEVMODEA pDevModeInputA = DEVMODEdupWtoA(GetProcessHeap(),pDevModeInput);
    LPDEVMODEA pDevModeOutputA = NULL;
    LONG ret;

    TRACE("(%d,%d,%s,%p,%p,%ld)\n",
          hWnd,hPrinter,debugstr_w(pDeviceName),pDevModeOutput,pDevModeInput,
	  fMode);
    if(pDevModeOutput) {
        ret = DocumentPropertiesA(hWnd, hPrinter, pDeviceNameA, NULL, NULL, 0);
	if(ret < 0) return ret;
	pDevModeOutputA = HeapAlloc(GetProcessHeap(), 0, ret);
    }
    ret = DocumentPropertiesA(hWnd, hPrinter, pDeviceNameA, pDevModeOutputA,
			      pDevModeInputA, fMode);
    if(pDevModeOutput) {
        DEVMODEcpyAtoW(pDevModeOutput, pDevModeOutputA);
	HeapFree(GetProcessHeap(),0,pDevModeOutputA);
    }
    if(fMode == 0 && ret > 0)
        ret += (CCHDEVICENAME + CCHFORMNAME);
    HeapFree(GetProcessHeap(),0,pDevModeInputA);
    HeapFree(GetProcessHeap(),0,pDeviceNameA);    
    return ret;
}

/******************************************************************
 *              OpenPrinterA        [WINSPOOL.196]
 *
 */
BOOL WINAPI OpenPrinterA(LPSTR lpPrinterName,HANDLE *phPrinter,
			 LPPRINTER_DEFAULTSA pDefault)
{
    LPWSTR lpPrinterNameW = HEAP_strdupAtoW(GetProcessHeap(),0,lpPrinterName);
    PRINTER_DEFAULTSW DefaultW, *pDefaultW = NULL;
    BOOL ret;

    if(pDefault) {
        DefaultW.pDatatype = HEAP_strdupAtoW(GetProcessHeap(), 0,
					     pDefault->pDatatype);
	DefaultW.pDevMode = DEVMODEdupAtoW(GetProcessHeap(),
					   pDefault->pDevMode);
	DefaultW.DesiredAccess = pDefault->DesiredAccess;
	pDefaultW = &DefaultW;
    }
    ret = OpenPrinterW(lpPrinterNameW, phPrinter, pDefaultW);
    if(pDefault) {
        HeapFree(GetProcessHeap(), 0, DefaultW.pDatatype);
	HeapFree(GetProcessHeap(), 0, DefaultW.pDevMode);
    }
    HeapFree(GetProcessHeap(), 0, lpPrinterNameW);
    return ret;
}

/******************************************************************
 *              OpenPrinterW        [WINSPOOL.197]
 *
 */
BOOL WINAPI OpenPrinterW(LPWSTR lpPrinterName,HANDLE *phPrinter,
			 LPPRINTER_DEFAULTSW pDefault)
{
    LPOPENEDPRINTER lpOpenedPrinter;
    HKEY hkeyPrinters, hkeyPrinter;

    if (!lpPrinterName) {
       WARN("(printerName: NULL, pDefault %p Ret: False\n", pDefault);
       SetLastError(ERROR_INVALID_PARAMETER);
       return FALSE;
    }

    TRACE("(printerName: %s, pDefault %p)\n", debugstr_w(lpPrinterName),
	  pDefault);

    /* Check Printer exists */
    if(RegCreateKeyA(HKEY_LOCAL_MACHINE, Printers, &hkeyPrinters) !=
       ERROR_SUCCESS) {
        ERR("Can't create Printers key\n");
	SetLastError(ERROR_FILE_NOT_FOUND); /* ?? */
	return FALSE;
    }

    if(RegOpenKeyW(hkeyPrinters, lpPrinterName, &hkeyPrinter)
       != ERROR_SUCCESS) {
        WARN("Can't find printer %s in registry\n", debugstr_w(lpPrinterName));
	RegCloseKey(hkeyPrinters);
        SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
    RegCloseKey(hkeyPrinter);
    RegCloseKey(hkeyPrinters);

    if(!phPrinter) /* This seems to be what win95 does anyway */
        return TRUE;

    /* Get a place in the opened printer buffer*/
    lpOpenedPrinter = WINSPOOL_GetOpenedPrinterEntry();
    if(!lpOpenedPrinter) {
        ERR("Can't allocate printer slot\n");
	SetLastError(ERROR_OUTOFMEMORY);
	return FALSE;
    }

    /* Get the name of the printer */
    lpOpenedPrinter->lpsPrinterName = 
      HEAP_strdupW( GetProcessHeap(), 0, lpPrinterName );

    /* Get the unique handle of the printer*/
    *phPrinter = lpOpenedPrinter->hPrinter;

    if (pDefault != NULL)
        FIXME("Not handling pDefault\n");

    return TRUE;
}

/******************************************************************
 *              AddMonitorA        [WINSPOOL.107]
 *
 */
BOOL WINAPI AddMonitorA(LPSTR pName, DWORD Level, LPBYTE pMonitors)
{
    FIXME("(%s,%lx,%p):stub!\n", pName, Level, pMonitors);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************
 *              DeletePrinterDriverA        [WINSPOOL.146]
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
 *              DeleteMonitorA        [WINSPOOL.135]
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
 *              DeletePortA        [WINSPOOL.137]
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
 *    SetPrinterW  [WINSPOOL.214]
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
 *    WritePrinter  [WINSPOOL.223]
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
 *          AddFormA  [WINSPOOL.103]
 */
BOOL WINAPI AddFormA(HANDLE hPrinter, DWORD Level, LPBYTE pForm)
{
    FIXME("(%d,%ld,%p): stub\n", hPrinter, Level, pForm);
    return 1;
}

/*****************************************************************************
 *          AddFormW  [WINSPOOL.104]
 */
BOOL WINAPI AddFormW(HANDLE hPrinter, DWORD Level, LPBYTE pForm)
{
    FIXME("(%d,%ld,%p): stub\n", hPrinter, Level, pForm);
    return 1;
}

/*****************************************************************************
 *          AddJobA  [WINSPOOL.105]
 */
BOOL WINAPI AddJobA(HANDLE hPrinter, DWORD Level, LPBYTE pData,
                        DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME("(%d,%ld,%p,%ld,%p): stub\n", hPrinter, Level, pData, cbBuf,
          pcbNeeded);
    return 1;
}

/*****************************************************************************
 *          AddJobW  [WINSPOOL.106]
 */
BOOL WINAPI AddJobW(HANDLE hPrinter, DWORD Level, LPBYTE pData, DWORD cbBuf,
                        LPDWORD pcbNeeded)
{
    FIXME("(%d,%ld,%p,%ld,%p): stub\n", hPrinter, Level, pData, cbBuf,
          pcbNeeded);
    return 1;
}

/*****************************************************************************
 *          AddPrinterW  [WINSPOOL.122]
 */
HANDLE WINAPI AddPrinterW(LPWSTR pName, DWORD Level, LPBYTE pPrinter)
{
    PRINTER_INFO_2W *pi = (PRINTER_INFO_2W *) pPrinter;
    LPDEVMODEA dmA;
    LPDEVMODEW dmW;
    HANDLE retval;
    HKEY hkeyPrinter, hkeyPrinters, hkeyDriver, hkeyDrivers;
    LONG size;

    TRACE("(%s,%ld,%p)\n", debugstr_w(pName), Level, pPrinter);
    
    if(pName != NULL) {
        FIXME("pName = %s - unsupported\n", debugstr_w(pName));
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
    if(RegOpenKeyW(hkeyPrinters, pi->pPrinterName, &hkeyPrinter) ==
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
    if(RegOpenKeyW(hkeyDrivers, pi->pDriverName, &hkeyDriver) != 
       ERROR_SUCCESS) {
        WARN("Can't find driver %s\n", debugstr_w(pi->pDriverName));
	RegCloseKey(hkeyPrinters);
	RegCloseKey(hkeyDrivers);
	SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER);
	return 0;
    }
    RegCloseKey(hkeyDriver);
    RegCloseKey(hkeyDrivers);

    if(lstrcmpiW(pi->pPrintProcessor, WinPrintW)) {  /* FIXME */
        WARN("Can't find processor %s\n", debugstr_w(pi->pPrintProcessor));
	SetLastError(ERROR_UNKNOWN_PRINTPROCESSOR);
	RegCloseKey(hkeyPrinters);
	return 0;
    }

    /* See if we can load the driver.  We may need the devmode structure anyway
     */
    size = DocumentPropertiesW(0, -1, pi->pPrinterName, NULL, NULL, 0);
    if(size < 0) {
        WARN("DocumentProperties fails\n");
	SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER);
	return 0;
    }
    if(pi->pDevMode) {
        dmW = pi->pDevMode;
    } else {
	dmW = HeapAlloc(GetProcessHeap(), 0, size);
	DocumentPropertiesW(0, -1, pi->pPrinterName, dmW, NULL, DM_OUT_BUFFER);
    }

    if(RegCreateKeyW(hkeyPrinters, pi->pPrinterName, &hkeyPrinter) !=
       ERROR_SUCCESS) {
        WARN("Can't create printer %s\n", debugstr_w(pi->pPrinterName));
	SetLastError(ERROR_INVALID_PRINTER_NAME);
	RegCloseKey(hkeyPrinters);
	if(!pi->pDevMode)
	    HeapFree(GetProcessHeap(), 0, dmW);
	return 0;
    }
    RegSetValueExA(hkeyPrinter, "Attributes", 0, REG_DWORD,
		   (LPBYTE)&pi->Attributes, sizeof(DWORD));
    RegSetValueExW(hkeyPrinter, DatatypeW, 0, REG_SZ, (LPBYTE)pi->pDatatype,
		   0);

    /* Write DEVMODEA not DEVMODEW into reg.  This is what win9x does
       and we support these drivers.  NT writes DEVMODEW so somehow
       we'll need to distinguish between these when we support NT
       drivers */
    dmA = DEVMODEdupWtoA(GetProcessHeap(), dmW);
    RegSetValueExA(hkeyPrinter, "Default DevMode", 0, REG_BINARY, (LPBYTE)dmA,
		   dmA->dmSize + dmA->dmDriverExtra);
    HeapFree(GetProcessHeap(), 0, dmA);
    if(!pi->pDevMode)
        HeapFree(GetProcessHeap(), 0, dmW);
    RegSetValueExW(hkeyPrinter, DescriptionW, 0, REG_SZ, (LPBYTE)pi->pComment,
		   0);
    RegSetValueExW(hkeyPrinter, LocationW, 0, REG_SZ, (LPBYTE)pi->pLocation,
		   0);
    RegSetValueExW(hkeyPrinter, NameW, 0, REG_SZ, (LPBYTE)pi->pPrinterName, 0);
    RegSetValueExW(hkeyPrinter, ParametersW, 0, REG_SZ,
		   (LPBYTE)pi->pParameters, 0);
    RegSetValueExW(hkeyPrinter, PortW, 0, REG_SZ, (LPBYTE)pi->pPortName, 0);
    RegSetValueExW(hkeyPrinter, Print_ProcessorW, 0, REG_SZ,
		   (LPBYTE)pi->pPrintProcessor, 0);
    RegSetValueExW(hkeyPrinter, Printer_DriverW, 0, REG_SZ,
		   (LPBYTE)pi->pDriverName, 0);
    RegSetValueExA(hkeyPrinter, "Priority", 0, REG_DWORD,
		   (LPBYTE)&pi->Priority, sizeof(DWORD));
    RegSetValueExW(hkeyPrinter, Separator_FileW, 0, REG_SZ,
		   (LPBYTE)pi->pSepFile, 0);
    RegSetValueExW(hkeyPrinter, Share_NameW, 0, REG_SZ, (LPBYTE)pi->pShareName,
		   0);
    RegSetValueExA(hkeyPrinter, "StartTime", 0, REG_DWORD,
		   (LPBYTE)&pi->StartTime, sizeof(DWORD));
    RegSetValueExA(hkeyPrinter, "Status", 0, REG_DWORD,
		   (LPBYTE)&pi->Status, sizeof(DWORD));
    RegSetValueExA(hkeyPrinter, "UntilTime", 0, REG_DWORD,
		   (LPBYTE)&pi->UntilTime, sizeof(DWORD));

    RegCloseKey(hkeyPrinter);
    RegCloseKey(hkeyPrinters);
    if(!OpenPrinterW(pi->pPrinterName, &retval, NULL)) {
        ERR("OpenPrinter failing\n");
	return 0;
    }
    return retval;
}

/*****************************************************************************
 *          AddPrinterA  [WINSPOOL.117]
 */
HANDLE WINAPI AddPrinterA(LPSTR pName, DWORD Level, LPBYTE pPrinter)
{
    WCHAR *pNameW;
    PRINTER_INFO_2W *piW;
    PRINTER_INFO_2A *piA = (PRINTER_INFO_2A*)pPrinter;
    HANDLE ret;

    TRACE("(%s,%ld,%p): stub\n", debugstr_a(pName), Level, pPrinter);
    if(Level != 2) {
        WARN("Level = %ld\n", Level);
	SetLastError(ERROR_INVALID_LEVEL);
	return 0;
    }
    pNameW = HEAP_strdupAtoW(GetProcessHeap(), 0, pName);
    piW = PRINTER_INFO_2AtoW(GetProcessHeap(), piA);

    ret = AddPrinterW(pNameW, Level, (LPBYTE)piW);

    FREE_PRINTER_INFO_2W(GetProcessHeap(), piW);
    HeapFree(GetProcessHeap(),0,pNameW);
    return ret;
}


/*****************************************************************************
 *          ClosePrinter  [WINSPOOL.126]
 */
BOOL WINAPI ClosePrinter(HANDLE hPrinter)
{
    LPOPENEDPRINTER lpOpenedPrinter;

    TRACE("Handle %d\n", hPrinter);

    if (!pOpenedPrinterDPA)
        return FALSE;

    if ((hPrinter != -1) && (hPrinter < (pOpenedPrinterDPA->nItemCount - 1)))
    {
	lpOpenedPrinter = WINSPOOL_GetOpenedPrinter(hPrinter);
	HeapFree(GetProcessHeap(), 0, lpOpenedPrinter->lpsPrinterName);
	lpOpenedPrinter->lpsPrinterName = NULL;
	lpOpenedPrinter->hPrinter = -1;

	return TRUE;
    }
    return FALSE;
}

/*****************************************************************************
 *          DeleteFormA  [WINSPOOL.133]
 */
BOOL WINAPI DeleteFormA(HANDLE hPrinter, LPSTR pFormName)
{
    FIXME("(%d,%s): stub\n", hPrinter, pFormName);
    return 1;
}

/*****************************************************************************
 *          DeleteFormW  [WINSPOOL.134]
 */
BOOL WINAPI DeleteFormW(HANDLE hPrinter, LPWSTR pFormName)
{
    FIXME("(%d,%s): stub\n", hPrinter, debugstr_w(pFormName));
    return 1;
}

/*****************************************************************************
 *          DeletePrinter  [WINSPOOL.143]
 */
BOOL WINAPI DeletePrinter(HANDLE hPrinter)
{
    LPWSTR lpNameW;
    HKEY hkeyPrinters;

    LPOPENEDPRINTER lpOpenedPrinter = WINSPOOL_GetOpenedPrinter(hPrinter);
    if(!lpOpenedPrinter) {
        SetLastError(ERROR_INVALID_HANDLE);
	return FALSE;
    }
    lpNameW = lpOpenedPrinter->lpsPrinterName;
    if(RegOpenKeyA(HKEY_LOCAL_MACHINE, Printers, &hkeyPrinters) !=
       ERROR_SUCCESS) {
        ERR("Can't open Printers key\n");
	return 0;
    }

    /* This should use a recursive delete see Q142491 or SHDeleteKey */
    if(RegDeleteKeyW(hkeyPrinters, lpNameW) == ERROR_SUCCESS) {
        SetLastError(ERROR_PRINTER_NOT_FOUND); /* ?? */
	RegCloseKey(hkeyPrinters);
	return 0;
    }    

    ClosePrinter(hPrinter);
    return TRUE;
}

/*****************************************************************************
 *          SetPrinterA  [WINSPOOL.211]
 */
BOOL WINAPI SetPrinterA(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
                           DWORD Command)
{
    FIXME("(%d,%ld,%p,%ld): stub\n",hPrinter,Level,pPrinter,Command);
    return FALSE;
}

/*****************************************************************************
 *          SetJobA  [WINSPOOL.209]
 */
BOOL WINAPI SetJobA(HANDLE hPrinter, DWORD JobId, DWORD Level,
                       LPBYTE pJob, DWORD Command)
{
    FIXME("(%d,%ld,%ld,%p,%ld): stub\n",hPrinter,JobId,Level,pJob,
         Command);
    return FALSE;
}

/*****************************************************************************
 *          SetJobW  [WINSPOOL.210]
 */
BOOL WINAPI SetJobW(HANDLE hPrinter, DWORD JobId, DWORD Level,
                       LPBYTE pJob, DWORD Command)
{
    FIXME("(%d,%ld,%ld,%p,%ld): stub\n",hPrinter,JobId,Level,pJob,
         Command);
    return FALSE;
}

/*****************************************************************************
 *          GetFormA  [WINSPOOL.181]
 */
BOOL WINAPI GetFormA(HANDLE hPrinter, LPSTR pFormName, DWORD Level,
                 LPBYTE pForm, DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME("(%d,%s,%ld,%p,%ld,%p): stub\n",hPrinter,pFormName,
         Level,pForm,cbBuf,pcbNeeded); 
    return FALSE;
}

/*****************************************************************************
 *          GetFormW  [WINSPOOL.182]
 */
BOOL WINAPI GetFormW(HANDLE hPrinter, LPWSTR pFormName, DWORD Level,
                 LPBYTE pForm, DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME("(%d,%s,%ld,%p,%ld,%p): stub\n",hPrinter,
	  debugstr_w(pFormName),Level,pForm,cbBuf,pcbNeeded);
    return FALSE;
}

/*****************************************************************************
 *          SetFormA  [WINSPOOL.207]
 */
BOOL WINAPI SetFormA(HANDLE hPrinter, LPSTR pFormName, DWORD Level,
                        LPBYTE pForm)
{
    FIXME("(%d,%s,%ld,%p): stub\n",hPrinter,pFormName,Level,pForm);
    return FALSE;
}

/*****************************************************************************
 *          SetFormW  [WINSPOOL.208]
 */
BOOL WINAPI SetFormW(HANDLE hPrinter, LPWSTR pFormName, DWORD Level,
                        LPBYTE pForm)
{
    FIXME("(%d,%p,%ld,%p): stub\n",hPrinter,pFormName,Level,pForm);
    return FALSE;
}

/*****************************************************************************
 *          ReadPrinter  [WINSPOOL.202]
 */
BOOL WINAPI ReadPrinter(HANDLE hPrinter, LPVOID pBuf, DWORD cbBuf,
                           LPDWORD pNoBytesRead)
{
    FIXME("(%d,%p,%ld,%p): stub\n",hPrinter,pBuf,cbBuf,pNoBytesRead);
    return FALSE;
}

/*****************************************************************************
 *          ResetPrinterA  [WINSPOOL.203]
 */
BOOL WINAPI ResetPrinterA(HANDLE hPrinter, LPPRINTER_DEFAULTSA pDefault)
{
    FIXME("(%d, %p): stub\n", hPrinter, pDefault);
    return FALSE;
}

/*****************************************************************************
 *          ResetPrinterW  [WINSPOOL.204]
 */
BOOL WINAPI ResetPrinterW(HANDLE hPrinter, LPPRINTER_DEFAULTSW pDefault)
{
    FIXME("(%d, %p): stub\n", hPrinter, pDefault);
    return FALSE;
}

/*****************************************************************************
 *    WINSPOOL_GetDWORDFromReg
 *
 * Return DWORD associated with ValueName from hkey.
 */ 
static DWORD WINSPOOL_GetDWORDFromReg(HKEY hkey, LPCSTR ValueName)
{
    DWORD sz = sizeof(DWORD), type, value = 0;
    LONG ret;

    ret = RegQueryValueExA(hkey, ValueName, 0, &type, (LPBYTE)&value, &sz);

    if(ret != ERROR_SUCCESS) {
        WARN("Got ret = %ld on name %s\n", ret, ValueName);
	return 0;
    }
    if(type != REG_DWORD) {
        ERR("Got type %ld\n", type);
	return 0;
    }
    return value;
}

/*****************************************************************************
 *    WINSPOOL_GetStringFromReg
 *
 * Get ValueName from hkey storing result in ptr.  buflen is space left in ptr
 * String is stored either as unicode or ascii.
 * Bit of a hack here to get the ValueName if we want ascii.
 */ 
static BOOL WINSPOOL_GetStringFromReg(HKEY hkey, LPCWSTR ValueName, LPBYTE ptr,
				      DWORD buflen, DWORD *needed,
				      BOOL unicode)
{
    DWORD sz = buflen, type;
    LONG ret;

    if(unicode)
        ret = RegQueryValueExW(hkey, ValueName, 0, &type, ptr, &sz);
    else {
        LPSTR ValueNameA = HEAP_strdupWtoA(GetProcessHeap(),0,ValueName);
        ret = RegQueryValueExA(hkey, ValueNameA, 0, &type, ptr, &sz);
	HeapFree(GetProcessHeap(),0,ValueNameA);
    }
    if(ret != ERROR_SUCCESS && ret != ERROR_MORE_DATA) {
        WARN("Got ret = %ld\n", ret);
	*needed = 0;
	return FALSE;
    }
    *needed = sz;
    return TRUE;
}

/*****************************************************************************
 *    WINSPOOL_GetDevModeFromReg
 *
 * Get ValueName from hkey storing result in ptr.  buflen is space left in ptr
 * DevMode is stored either as unicode or ascii.
 */ 
static BOOL WINSPOOL_GetDevModeFromReg(HKEY hkey, LPCWSTR ValueName,
				       LPBYTE ptr,
				       DWORD buflen, DWORD *needed,
				       BOOL unicode)
{
    DWORD sz = buflen, type;
    LONG ret;

    ret = RegQueryValueExW(hkey, ValueName, 0, &type, ptr, &sz);
    if(ret != ERROR_SUCCESS && ret != ERROR_MORE_DATA) {
        WARN("Got ret = %ld\n", ret);
	*needed = 0;
	return FALSE;
    }
    if(unicode) {
	sz += (CCHDEVICENAME + CCHFORMNAME);
	if(buflen >= sz) {
	    DEVMODEW *dmW = DEVMODEdupAtoW(GetProcessHeap(), (DEVMODEA*)ptr);
	    memcpy(ptr, dmW, sz);
	    HeapFree(GetProcessHeap(),0,dmW);
	}
    }
    *needed = sz;
    return TRUE;
}

/*********************************************************************
 *    WINSPOOL_GetPrinter_2
 *
 * Fills out a PRINTER_INFO_2A|W struct storing the strings in buf.
 * The strings are either stored as unicode or ascii.
 */
static BOOL WINSPOOL_GetPrinter_2(HKEY hkeyPrinter, PRINTER_INFO_2W *pi2,
				  LPBYTE buf, DWORD cbBuf, LPDWORD pcbNeeded,
				  BOOL unicode)
{
    DWORD size, left = cbBuf;
    BOOL space = (cbBuf > 0);
    LPBYTE ptr = buf;

    *pcbNeeded = 0;

    if(WINSPOOL_GetStringFromReg(hkeyPrinter, NameW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi2->pPrinterName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, Share_NameW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi2->pShareName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, PortW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi2->pPortName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, Printer_DriverW, ptr, left,
				 &size, unicode)) {
        if(space && size <= left) {
	    pi2->pDriverName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, DescriptionW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi2->pComment = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, LocationW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi2->pLocation = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetDevModeFromReg(hkeyPrinter, Default_DevModeW, ptr, left,
				  &size, unicode)) {
        if(space && size <= left) {
	    pi2->pDevMode = (LPDEVMODEW)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, Separator_FileW, ptr, left,
				 &size, unicode)) {
        if(space && size <= left) {
            pi2->pSepFile = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, Print_ProcessorW, ptr, left,
				 &size, unicode)) {
        if(space && size <= left) {
	    pi2->pPrintProcessor = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, DatatypeW, ptr, left,
				 &size, unicode)) {
        if(space && size <= left) {
	    pi2->pDatatype = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, ParametersW, ptr, left,
				 &size, unicode)) {
        if(space && size <= left) {
	    pi2->pParameters = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(pi2) {
        pi2->Attributes = WINSPOOL_GetDWORDFromReg(hkeyPrinter, "Attributes"); 
        pi2->Priority = WINSPOOL_GetDWORDFromReg(hkeyPrinter, "Priority");
        pi2->DefaultPriority = WINSPOOL_GetDWORDFromReg(hkeyPrinter,
							"Default Priority");
        pi2->StartTime = WINSPOOL_GetDWORDFromReg(hkeyPrinter, "StartTime");
        pi2->UntilTime = WINSPOOL_GetDWORDFromReg(hkeyPrinter, "UntilTime");
    }

    if(!space && pi2) /* zero out pi2 if we can't completely fill buf */
        memset(pi2, 0, sizeof(*pi2));

    return space;
}

/*********************************************************************
 *    WINSPOOL_GetPrinter_4
 *
 * Fills out a PRINTER_INFO_4 struct storing the strings in buf.
 */
static BOOL WINSPOOL_GetPrinter_4(HKEY hkeyPrinter, PRINTER_INFO_4W *pi4,
				  LPBYTE buf, DWORD cbBuf, LPDWORD pcbNeeded,
				  BOOL unicode)
{
    DWORD size, left = cbBuf;
    BOOL space = (cbBuf > 0);
    LPBYTE ptr = buf;

    *pcbNeeded = 0;

    if(WINSPOOL_GetStringFromReg(hkeyPrinter, NameW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi4->pPrinterName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(pi4) {
        pi4->Attributes = WINSPOOL_GetDWORDFromReg(hkeyPrinter, "Attributes"); 
    }

    if(!space && pi4) /* zero out pi4 if we can't completely fill buf */
        memset(pi4, 0, sizeof(*pi4));

    return space;
}

/*********************************************************************
 *    WINSPOOL_GetPrinter_5
 *
 * Fills out a PRINTER_INFO_5 struct storing the strings in buf.
 */
static BOOL WINSPOOL_GetPrinter_5(HKEY hkeyPrinter, PRINTER_INFO_5W *pi5,
				  LPBYTE buf, DWORD cbBuf, LPDWORD pcbNeeded,
				  BOOL unicode)
{
    DWORD size, left = cbBuf;
    BOOL space = (cbBuf > 0);
    LPBYTE ptr = buf;

    *pcbNeeded = 0;

    if(WINSPOOL_GetStringFromReg(hkeyPrinter, NameW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi5->pPrinterName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, PortW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi5->pPortName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(pi5) {
        pi5->Attributes = WINSPOOL_GetDWORDFromReg(hkeyPrinter, "Attributes"); 
        pi5->DeviceNotSelectedTimeout = WINSPOOL_GetDWORDFromReg(hkeyPrinter,
								"dnsTimeout"); 
        pi5->TransmissionRetryTimeout = WINSPOOL_GetDWORDFromReg(hkeyPrinter,
								 "txTimeout"); 
    }

    if(!space && pi5) /* zero out pi5 if we can't completely fill buf */
        memset(pi5, 0, sizeof(*pi5));

    return space;
}

/*****************************************************************************
 *          WINSPOOL_GetPrinter
 *
 *    Implementation of GetPrinterA|W.  Relies on PRINTER_INFO_*W being
 *    essentially the same as PRINTER_INFO_*A. i.e. the structure itself is
 *    just a collection of pointers to strings.
 */
static BOOL WINSPOOL_GetPrinter(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
				DWORD cbBuf, LPDWORD pcbNeeded, BOOL unicode)
{
    OPENEDPRINTER *lpOpenedPrinter;
    DWORD size, needed = 0;
    LPBYTE ptr = NULL;
    HKEY hkeyPrinter, hkeyPrinters;
    BOOL ret;

    TRACE("(%d,%ld,%p,%ld,%p)\n",hPrinter,Level,pPrinter,cbBuf, pcbNeeded);

    lpOpenedPrinter = WINSPOOL_GetOpenedPrinter(hPrinter);
    if(!lpOpenedPrinter) {
        SetLastError(ERROR_INVALID_HANDLE);
	return FALSE;
    }
    if(RegCreateKeyA(HKEY_LOCAL_MACHINE, Printers, &hkeyPrinters) !=
       ERROR_SUCCESS) {
        ERR("Can't create Printers key\n");
	return FALSE;
    }
    if(RegOpenKeyW(hkeyPrinters, lpOpenedPrinter->lpsPrinterName, &hkeyPrinter)
       != ERROR_SUCCESS) {
        ERR("Can't find opened printer %s in registry\n",
	    debugstr_w(lpOpenedPrinter->lpsPrinterName));
	RegCloseKey(hkeyPrinters);
        SetLastError(ERROR_INVALID_PRINTER_NAME); /* ? */
	return FALSE;
    }

    switch(Level) {
    case 2:
      {
        PRINTER_INFO_2W *pi2 = (PRINTER_INFO_2W *)pPrinter;

        size = sizeof(PRINTER_INFO_2W);
	if(size <= cbBuf) {
	    ptr = pPrinter + size;
	    cbBuf -= size;
	    memset(pPrinter, 0, size);
	} else {
	    pi2 = NULL;
	    cbBuf = 0;
	}
	ret = WINSPOOL_GetPrinter_2(hkeyPrinter, pi2, ptr, cbBuf, &needed,
				    unicode);
	needed += size;
	break;
      }
      
    case 4:
      {
	PRINTER_INFO_4W *pi4 = (PRINTER_INFO_4W *)pPrinter;
	
        size = sizeof(PRINTER_INFO_4W);
	if(size <= cbBuf) {
	    ptr = pPrinter + size;
	    cbBuf -= size;
	    memset(pPrinter, 0, size);
	} else {
	    pi4 = NULL;
	    cbBuf = 0;
	}
	ret = WINSPOOL_GetPrinter_4(hkeyPrinter, pi4, ptr, cbBuf, &needed,
				    unicode);
	needed += size;
	break;
      }


    case 5:
      {
        PRINTER_INFO_5W *pi5 = (PRINTER_INFO_5W *)pPrinter;

        size = sizeof(PRINTER_INFO_5W);
	if(size <= cbBuf) {
	    ptr = pPrinter + size;
	    cbBuf -= size;
	    memset(pPrinter, 0, size);
	} else {
	    pi5 = NULL;
	    cbBuf = 0;
	}

	ret = WINSPOOL_GetPrinter_5(hkeyPrinter, pi5, ptr, cbBuf, &needed,
				    unicode);
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

    TRACE("returing %d needed = %ld\n", ret, needed);
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
    return WINSPOOL_GetPrinter(hPrinter, Level, pPrinter, cbBuf, pcbNeeded,
			       TRUE);
}

/*****************************************************************************
 *          GetPrinterA  [WINSPOOL.187]
 */
BOOL WINAPI GetPrinterA(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
                    DWORD cbBuf, LPDWORD pcbNeeded)
{
    return WINSPOOL_GetPrinter(hPrinter, Level, pPrinter, cbBuf, pcbNeeded,
			       FALSE);
}

/*****************************************************************************
 *          WINSPOOL_EnumPrinters
 *
 *    Implementation of EnumPrintersA|W
 */
static BOOL WINSPOOL_EnumPrinters(DWORD dwType, LPWSTR lpszName,
				  DWORD dwLevel, LPBYTE lpbPrinters,
				  DWORD cbBuf, LPDWORD lpdwNeeded,
				  LPDWORD lpdwReturned, BOOL unicode)

{
    HKEY hkeyPrinters, hkeyPrinter;
    WCHAR PrinterName[255];
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
        used = number * sizeof(PRINTER_INFO_2W);
	break;
    case 4:
        used = number * sizeof(PRINTER_INFO_4W);
	break;
    case 5:
        used = number * sizeof(PRINTER_INFO_5W);
	break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
	RegCloseKey(hkeyPrinters);
	return FALSE;
    }
    pi = (used <= cbBuf) ? lpbPrinters : NULL;

    for(i = 0; i < number; i++) {
        if(RegEnumKeyW(hkeyPrinters, i, PrinterName, sizeof(PrinterName)) != 
	   ERROR_SUCCESS) {
	    ERR("Can't enum key number %ld\n", i);
	    RegCloseKey(hkeyPrinters);
	    return FALSE;
	}
	TRACE("Printer %ld is %s\n", i, debugstr_w(PrinterName));
	if(RegOpenKeyW(hkeyPrinters, PrinterName, &hkeyPrinter) !=
	   ERROR_SUCCESS) {
	    ERR("Can't open key %s\n", debugstr_w(PrinterName));
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
	    WINSPOOL_GetPrinter_2(hkeyPrinter, (PRINTER_INFO_2W *)pi, buf,
				  left, &needed, unicode);
	    used += needed;
	    if(pi) pi += sizeof(PRINTER_INFO_2W);
	    break;
	case 4:
	    WINSPOOL_GetPrinter_4(hkeyPrinter, (PRINTER_INFO_4W *)pi, buf,
				  left, &needed, unicode);
	    used += needed;
	    if(pi) pi += sizeof(PRINTER_INFO_4W);
	    break;
	case 5:
	    WINSPOOL_GetPrinter_5(hkeyPrinter, (PRINTER_INFO_5W *)pi, buf,
				  left, &needed, unicode);
	    used += needed;
	    if(pi) pi += sizeof(PRINTER_INFO_5W);
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
BOOL  WINAPI EnumPrintersW(
		DWORD dwType,        /* Types of print objects to enumerate */
                LPWSTR lpszName,     /* name of objects to enumerate */
	        DWORD dwLevel,       /* type of printer info structure */
                LPBYTE lpbPrinters,  /* buffer which receives info */
		DWORD cbBuf,         /* max size of buffer in bytes */
		LPDWORD lpdwNeeded,  /* pointer to var: # bytes used/needed */
		LPDWORD lpdwReturned /* number of entries returned */
		)
{
    return WINSPOOL_EnumPrinters(dwType, lpszName, dwLevel, lpbPrinters, cbBuf,
				 lpdwNeeded, lpdwReturned, TRUE);
}

/******************************************************************
 *              EnumPrintersA        [WINSPOOL.174]
 *
 */
BOOL WINAPI EnumPrintersA(DWORD dwType, LPSTR lpszName,
			  DWORD dwLevel, LPBYTE lpbPrinters,
			  DWORD cbBuf, LPDWORD lpdwNeeded,
			  LPDWORD lpdwReturned)
{
    BOOL ret;
    LPWSTR lpszNameW = HEAP_strdupAtoW(GetProcessHeap(),0,lpszName);

    ret = WINSPOOL_EnumPrinters(dwType, lpszNameW, dwLevel, lpbPrinters, cbBuf,
				lpdwNeeded, lpdwReturned, FALSE);
    HeapFree(GetProcessHeap(),0,lpszNameW);
    return ret;
}


/*****************************************************************************
 *          WINSPOOL_GetPrinterDriver
 */
static BOOL WINSPOOL_GetPrinterDriver(HANDLE hPrinter, LPWSTR pEnvironment,
				      DWORD Level, LPBYTE pDriverInfo,
				      DWORD cbBuf, LPDWORD pcbNeeded,
				      BOOL unicode)
{
    OPENEDPRINTER *lpOpenedPrinter;
    WCHAR DriverName[100];
    DWORD ret, type, size, dw, needed = 0;
    LPBYTE ptr = NULL;
    HKEY hkeyPrinter, hkeyPrinters, hkeyDriver, hkeyDrivers;
    
    TRACE("(%d,%s,%ld,%p,%ld,%p)\n",hPrinter,debugstr_w(pEnvironment),
	  Level,pDriverInfo,cbBuf, pcbNeeded);

    ZeroMemory(pDriverInfo, cbBuf);

    lpOpenedPrinter = WINSPOOL_GetOpenedPrinter(hPrinter);
    if(!lpOpenedPrinter) {
        SetLastError(ERROR_INVALID_HANDLE);
	return FALSE;
    }
    if(pEnvironment) {
        FIXME("pEnvironment = %s\n", debugstr_w(pEnvironment));
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
    if(RegOpenKeyW(hkeyPrinters, lpOpenedPrinter->lpsPrinterName, &hkeyPrinter)
       != ERROR_SUCCESS) {
        ERR("Can't find opened printer %s in registry\n",
	    debugstr_w(lpOpenedPrinter->lpsPrinterName));
	RegCloseKey(hkeyPrinters);
        SetLastError(ERROR_INVALID_PRINTER_NAME); /* ? */
	return FALSE;
    }
    size = sizeof(DriverName);
    ret = RegQueryValueExW(hkeyPrinter, Printer_DriverW, 0, &type,
			   (LPBYTE)DriverName, &size);
    RegCloseKey(hkeyPrinter);
    RegCloseKey(hkeyPrinters);
    if(ret != ERROR_SUCCESS) {
        ERR("Can't get DriverName for printer %s\n",
	    debugstr_w(lpOpenedPrinter->lpsPrinterName));
	return FALSE;
    }
    if(RegCreateKeyA(HKEY_LOCAL_MACHINE, Drivers, &hkeyDrivers) !=
       ERROR_SUCCESS) {
        ERR("Can't create Drivers key\n");
	return FALSE;
    }
    if(RegOpenKeyW(hkeyDrivers, DriverName, &hkeyDriver)
       != ERROR_SUCCESS) {
        ERR("Can't find driver %s in registry\n", debugstr_w(DriverName));
	RegCloseKey(hkeyDrivers);
        SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER); /* ? */
	return FALSE;
    }

    switch(Level) {
    case 1:
        size = sizeof(DRIVER_INFO_1W);
	break;
    case 2:
        size = sizeof(DRIVER_INFO_2W);
	break;
    case 3:
        size = sizeof(DRIVER_INFO_3W);
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

    if(unicode)
        size = (lstrlenW(DriverName) + 1) * sizeof(WCHAR);
    else
        size = WideCharToMultiByte(CP_ACP, 0, DriverName, -1, NULL, 0, NULL,
				   NULL);

    if(size <= cbBuf) {
        cbBuf -= size;
	if(unicode)
	    lstrcpyW((LPWSTR)ptr, DriverName);
	else
	    WideCharToMultiByte(CP_ACP, 0, DriverName, -1, ptr, size, NULL,
				NULL);
        if(Level == 1)
	    ((DRIVER_INFO_1W *)pDriverInfo)->pName = (LPWSTR)ptr;
	else
	    ((DRIVER_INFO_2W *)pDriverInfo)->pName = (LPWSTR)ptr;
	ptr += size;
    }
    needed += size;

    if(Level > 1) {
        DRIVER_INFO_2W *di2 = (DRIVER_INFO_2W *)pDriverInfo;

	size = sizeof(dw);
        if(RegQueryValueExA(hkeyDriver, "Version", 0, &type, (PBYTE)&dw,
			    &size) !=
	   ERROR_SUCCESS)
	    WARN("Can't get Version\n");
	else if(cbBuf)
	    di2->cVersion = dw;

	if(!pEnvironment)
	    pEnvironment = DefaultEnvironmentW;
	if(unicode)
	    size = (lstrlenW(pEnvironment) + 1) * sizeof(WCHAR);
	else
	    size = WideCharToMultiByte(CP_ACP, 0, pEnvironment, -1, NULL, 0,
				       NULL, NULL);
	if(size <= cbBuf) {
	    cbBuf -= size;
	    if(unicode)
	        lstrcpyW((LPWSTR)ptr, pEnvironment);
	    else
	        WideCharToMultiByte(CP_ACP, 0, pEnvironment, -1, ptr, size,
				    NULL, NULL);
	    di2->pEnvironment = (LPWSTR)ptr;
	    ptr += size;
	} else 
	    cbBuf = 0;
	needed += size;

	if(WINSPOOL_GetStringFromReg(hkeyDriver, DriverW, ptr, cbBuf, &size,
				     unicode)) {
	    if(cbBuf && size <= cbBuf) {
	        di2->pDriverPath = (LPWSTR)ptr;
		ptr += size;
	    } else
	        cbBuf = 0;
	    needed += size;
	}
	if(WINSPOOL_GetStringFromReg(hkeyDriver, Data_FileW, ptr, cbBuf, &size,
				     unicode)) {
	    if(cbBuf && size <= cbBuf) {
	        di2->pDataFile = (LPWSTR)ptr;
		ptr += size;
	    } else
	        cbBuf = 0;
	    needed += size;
	}
	if(WINSPOOL_GetStringFromReg(hkeyDriver, Configuration_FileW, ptr,
				     cbBuf, &size, unicode)) {
	    if(cbBuf && size <= cbBuf) {
	        di2->pConfigFile = (LPWSTR)ptr;
		ptr += size;
	    } else
	        cbBuf = 0;
	    needed += size;
	}
    }
    if(Level > 2) {
        DRIVER_INFO_3W *di3 = (DRIVER_INFO_3W *)pDriverInfo;

	if(WINSPOOL_GetStringFromReg(hkeyDriver, Help_FileW, ptr, cbBuf, &size,
				     unicode)) {
	    if(cbBuf && size <= cbBuf) {
	        di3->pHelpFile = (LPWSTR)ptr;
		ptr += size;
	    } else
	        cbBuf = 0;
	    needed += size;
	}
	if(WINSPOOL_GetStringFromReg(hkeyDriver, Dependent_FilesW, ptr, cbBuf,
				     &size, unicode)) {
	    if(cbBuf && size <= cbBuf) {
	        di3->pDependentFiles = (LPWSTR)ptr;
		ptr += size;
	    } else
	        cbBuf = 0;
	    needed += size;
	}
	if(WINSPOOL_GetStringFromReg(hkeyDriver, MonitorW, ptr, cbBuf, &size,
				     unicode)) {
	    if(cbBuf && size <= cbBuf) {
	        di3->pMonitorName = (LPWSTR)ptr;
		ptr += size;
	    } else
	        cbBuf = 0;
	    needed += size;
	}
	if(WINSPOOL_GetStringFromReg(hkeyDriver, DatatypeW, ptr, cbBuf, &size,
				     unicode)) {
	    if(cbBuf && size <= cbBuf) {
	        di3->pDefaultDataType = (LPWSTR)ptr;
		ptr += size;
	    } else
	        cbBuf = 0;
	    needed += size;
	}
    }
    RegCloseKey(hkeyDriver);
    RegCloseKey(hkeyDrivers);

    if(pcbNeeded) *pcbNeeded = needed;
    if(cbBuf) return TRUE;
    SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return FALSE;
}

/*****************************************************************************
 *          GetPrinterDriverA  [WINSPOOL.190]
 */
BOOL WINAPI GetPrinterDriverA(HANDLE hPrinter, LPSTR pEnvironment,
			      DWORD Level, LPBYTE pDriverInfo,
			      DWORD cbBuf, LPDWORD pcbNeeded)
{
    BOOL ret;
    LPWSTR pEnvW = HEAP_strdupAtoW(GetProcessHeap(),0,pEnvironment);
    ret = WINSPOOL_GetPrinterDriver(hPrinter, pEnvW, Level, pDriverInfo,
				    cbBuf, pcbNeeded, FALSE);
    HeapFree(GetProcessHeap(),0,pEnvW);
    return ret;
}
/*****************************************************************************
 *          GetPrinterDriverW  [WINSPOOL.193]
 */
BOOL WINAPI GetPrinterDriverW(HANDLE hPrinter, LPWSTR pEnvironment,
                                  DWORD Level, LPBYTE pDriverInfo, 
                                  DWORD cbBuf, LPDWORD pcbNeeded)
{
    return WINSPOOL_GetPrinterDriver(hPrinter, pEnvironment, Level,
				     pDriverInfo, cbBuf, pcbNeeded, TRUE);
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
 *          AddPrinterDriverA  [WINSPOOL.120]
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
 *          AddPrinterDriverW  [WINSPOOL.121]
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
