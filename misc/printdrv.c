/* 
 * Implementation of some printer driver bits
 * 
 * Copyright 1996 John Harvey
 * Copyright 1998 Andreas Mohr
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "winspool.h"
#include "ldt.h"
#include "winerror.h"
#include "winreg.h"
#include "debug.h"

static char PrinterModel[]	= "Printer Model";
static char DefaultDevMode[]	= "Default DevMode";
static char PrinterDriverData[] = "PrinterDriverData";
static char Printers[]		= "System\\CurrentControlSet\\Control\\Print\\Printers\\";

/******************************************************************
 *                  StartDoc16  [GDI.377]
 *
 */
INT16 WINAPI StartDoc16( HDC16 hdc, const DOCINFO16 *lpdoc )
{
  INT16 retVal;
  TRACE(print,"(%p)\n", lpdoc );
  TRACE(print,"%d 0x%lx:0x%p 0x%lx:0x%p\n",lpdoc->cbSize,
	lpdoc->lpszDocName,PTR_SEG_TO_LIN(lpdoc->lpszDocName),
	lpdoc->lpszOutput,PTR_SEG_TO_LIN(lpdoc->lpszOutput));
  TRACE(print, "%d %s %s\n",lpdoc->cbSize,
	(LPSTR)PTR_SEG_TO_LIN(lpdoc->lpszDocName),
	(LPSTR)PTR_SEG_TO_LIN(lpdoc->lpszOutput));
  retVal =  Escape16(hdc, STARTDOC,
    strlen((LPSTR)PTR_SEG_TO_LIN(lpdoc->lpszDocName)), lpdoc->lpszDocName, 0);
  TRACE(print,"Escape16 returned %d\n",retVal);
  return retVal;
}

/******************************************************************
 *                  EndPage16  [GDI.380]
 *
 */
INT16 WINAPI EndPage16( HDC16 hdc )
{
  INT16 retVal;
  retVal =  Escape16(hdc, NEWFRAME, 0, 0, 0);
  TRACE(print,"Escape16 returned %d\n",retVal);
  return retVal;
}

/******************************************************************
 *                  StartDoc32A  [GDI32.347]
 *
 */
INT WINAPI StartDocA(HDC hdc ,const DOCINFOA* doc)
{
  FIXME(gdi,"stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED); 
  return 0; /* failure*/
}

/*************************************************************************
 *                  StartDoc32W [GDI32.348]
 * 
 */
INT WINAPI StartDocW(HDC hdc, const DOCINFOW* doc) {
  FIXME(gdi,"stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED); 
  return 0; /* failure*/
}

/******************************************************************
 *                  StartPage32  [GDI32.349]
 *
 */
INT WINAPI StartPage(HDC hdc)
{
  FIXME(gdi,"stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED); 
  return 0; /* failure*/
}

/******************************************************************
 *                  EndPage32  [GDI32.77]
 *
 */
INT WINAPI EndPage(HDC hdc)
{
  FIXME(gdi,"stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED); 
  return 0; /* failure*/
}

/******************************************************************
 *                  EndDoc16  [GDI.378]
 *
 */
INT16 WINAPI EndDoc16(HDC16 hdc)
{
  return  Escape16(hdc, ENDDOC, 0, 0, 0);
}

/******************************************************************
 *                  EndDoc32  [GDI32.76]
 *
 */
INT WINAPI EndDoc(HDC hdc)
{
  FIXME(gdi,"stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED); 
  return 0; /* failure*/
}

/******************************************************************************
 *                 AbortDoc16  [GDI.382]
 */
INT16 WINAPI AbortDoc16(HDC16 hdc)
{
  return Escape16(hdc, ABORTDOC, 0, 0, 0);
}

/******************************************************************************
 *                 AbortDoc32  [GDI32.0]
 */
INT WINAPI AbortDoc(HDC hdc)
{
    FIXME(gdi, "(%d): stub\n", hdc);
    return 1;
}

/******************************************************************
 *                  DrvGetPrinterDataInternal
 *
 * Helper for DrvGetPrinterData
 */
static DWORD DrvGetPrinterDataInternal(LPSTR RegStr_Printer,
LPBYTE lpPrinterData, int cbData, int what)
{
    DWORD res = -1;
    HKEY hkey;
    DWORD dwType, cbQueryData;

    if (!(RegOpenKeyA(HKEY_LOCAL_MACHINE, RegStr_Printer, &hkey))) {
        if (what == INT_PD_DEFAULT_DEVMODE) { /* "Default DevMode" */
            if (!(RegQueryValueExA(hkey, DefaultDevMode, 0, &dwType, 0, &cbQueryData))) {
                if (!lpPrinterData)
		    res = cbQueryData;
		else if ((cbQueryData) && (cbQueryData <= cbData)) {
		    cbQueryData = cbData;
		    if (RegQueryValueExA(hkey, DefaultDevMode, 0,
				&dwType, lpPrinterData, &cbQueryData))
		        res = cbQueryData;
		}
	    }
	} else { /* "Printer Driver" */
	    cbQueryData = 32;
	    RegQueryValueExA(hkey, "Printer Driver", 0,
			&dwType, lpPrinterData, &cbQueryData);
	    res = cbQueryData;
	}
    }
    if (hkey) RegCloseKey(hkey);
    return res;
}

/******************************************************************
 *                DrvGetPrinterData     [GDI.282]
 *
 */
DWORD WINAPI DrvGetPrinterData16(LPSTR lpPrinter, LPSTR lpProfile,
                               LPDWORD lpType, LPBYTE lpPrinterData,
                               int cbData, LPDWORD lpNeeded)
{
    LPSTR RegStr_Printer;
    HKEY hkey = 0, hkey2 = 0;
    DWORD res = 0;
    DWORD dwType, PrinterAttr, cbPrinterAttr, SetData, size;

    if (HIWORD(lpPrinter))
            TRACE(print,"printer %s\n",lpPrinter);
    else
            TRACE(print,"printer %p\n",lpPrinter);
    if (HIWORD(lpProfile))
            TRACE(print,"profile %s\n",lpProfile);
    else
            TRACE(print,"profile %p\n",lpProfile);
    TRACE(print,"lpType %p\n",lpType);

    if ((!lpPrinter) || (!lpProfile) || (!lpNeeded))
	return ERROR_INVALID_PARAMETER;

    RegStr_Printer = HeapAlloc(GetProcessHeap(), 0,
                               strlen(Printers) + strlen(lpPrinter) + 2);
    strcpy(RegStr_Printer, Printers);
    strcat(RegStr_Printer, lpPrinter);

    if (((DWORD)lpProfile == INT_PD_DEFAULT_DEVMODE) || (HIWORD(lpProfile) &&
    (!strcmp(lpProfile, DefaultDevMode)))) {
	size = DrvGetPrinterDataInternal(RegStr_Printer, lpPrinterData, cbData,
					 INT_PD_DEFAULT_DEVMODE);
	if (size+1) {
	    *lpNeeded = size;
	    if ((lpPrinterData) && (*lpNeeded > cbData))
		res = ERROR_MORE_DATA;
	}
	else res = ERROR_INVALID_PRINTER_NAME;
    }
    else
    if (((DWORD)lpProfile == INT_PD_DEFAULT_MODEL) || (HIWORD(lpProfile) &&
    (!strcmp(lpProfile, PrinterModel)))) {
	*lpNeeded = 32;
	if (!lpPrinterData) goto failed;
	if (cbData < 32) {
	    res = ERROR_MORE_DATA;
	    goto failed;
	}
	size = DrvGetPrinterDataInternal(RegStr_Printer, lpPrinterData, cbData,
					 INT_PD_DEFAULT_MODEL);
	if ((size+1) && (lpType))
	    *lpType = REG_SZ;
	else
	    res = ERROR_INVALID_PRINTER_NAME;
    }
    else
    {
	if ((res = RegOpenKeyA(HKEY_LOCAL_MACHINE, RegStr_Printer, &hkey)))
	    goto failed;
        cbPrinterAttr = 4;
        if ((res = RegQueryValueExA(hkey, "Attributes", 0,
                        &dwType, (LPBYTE)&PrinterAttr, &cbPrinterAttr)))
	    goto failed;
	if ((res = RegOpenKeyA(hkey, PrinterDriverData, &hkey2)))
	    goto failed;
        *lpNeeded = cbData;
        res = RegQueryValueExA(hkey2, lpProfile, 0,
                lpType, lpPrinterData, lpNeeded);
        if ((res != ERROR_CANTREAD) &&
         ((PrinterAttr &
        (PRINTER_ATTRIBUTE_ENABLE_BIDI|PRINTER_ATTRIBUTE_NETWORK))
        == PRINTER_ATTRIBUTE_NETWORK))
        {
	    if (!(res) && (*lpType == REG_DWORD) && (*(LPDWORD)lpPrinterData == -1))
	        res = ERROR_INVALID_DATA;
	}
	else
        {
	    SetData = -1;
	    RegSetValueExA(hkey2, lpProfile, 0, REG_DWORD, (LPBYTE)&SetData, 4); /* no result returned */
	}
    }
	
failed:
    if (hkey2) RegCloseKey(hkey2);
    if (hkey) RegCloseKey(hkey);
    HeapFree(GetProcessHeap(), 0, RegStr_Printer);
    return res;
}


/******************************************************************
 *                 DrvSetPrinterData     [GDI.281]
 *
 */
DWORD WINAPI DrvSetPrinterData16(LPSTR lpPrinter, LPSTR lpProfile,
                               DWORD lpType, LPBYTE lpPrinterData,
                               DWORD dwSize)
{
    LPSTR RegStr_Printer;
    HKEY hkey = 0;
    DWORD res = 0;

    if (HIWORD(lpPrinter))
            TRACE(print,"printer %s\n",lpPrinter);
    else
            TRACE(print,"printer %p\n",lpPrinter);
    if (HIWORD(lpProfile))
            TRACE(print,"profile %s\n",lpProfile);
    else
            TRACE(print,"profile %p\n",lpProfile);
    TRACE(print,"lpType %08lx\n",lpType);

    if ((!lpPrinter) || (!lpProfile) ||
    ((DWORD)lpProfile == INT_PD_DEFAULT_MODEL) || (HIWORD(lpProfile) &&
    (!strcmp(lpProfile, PrinterModel))))
	return ERROR_INVALID_PARAMETER;

    RegStr_Printer = HeapAlloc(GetProcessHeap(), 0,
			strlen(Printers) + strlen(lpPrinter) + 2);
    strcpy(RegStr_Printer, Printers);
    strcat(RegStr_Printer, lpPrinter);

    if (((DWORD)lpProfile == INT_PD_DEFAULT_DEVMODE) || (HIWORD(lpProfile) &&
    (!strcmp(lpProfile, DefaultDevMode)))) {
	if ( RegOpenKeyA(HKEY_LOCAL_MACHINE, RegStr_Printer, &hkey) 
	     != ERROR_SUCCESS ||
	     RegSetValueExA(hkey, DefaultDevMode, 0, REG_BINARY, 
			      lpPrinterData, dwSize) != ERROR_SUCCESS )
	        res = ERROR_INVALID_PRINTER_NAME;
    }
    else
    {
	strcat(RegStr_Printer, "\\");

	if( (res = RegOpenKeyA(HKEY_LOCAL_MACHINE, RegStr_Printer, &hkey)) ==
	    ERROR_SUCCESS ) {

	    if (!lpPrinterData) 
	        res = RegDeleteValueA(hkey, lpProfile);
	    else
                res = RegSetValueExA(hkey, lpProfile, 0, lpType,
				       lpPrinterData, dwSize);
	}
    }

    if (hkey) RegCloseKey(hkey);
    HeapFree(GetProcessHeap(), 0, RegStr_Printer);
    return res;
}


/******************************************************************
 *              DeviceCapabilities32A    [WINSPOOL.151]
 *
 */
INT WINAPI DeviceCapabilitiesA(LPCSTR printer,LPCSTR target,WORD z,
                                   LPSTR a,LPDEVMODEA b)
{
    FIXME(print,"(%s,%s,%d,%p,%p):stub.\n",printer,target,z,a,b);
    return 1;   	
}


/*****************************************************************************
 *          DeviceCapabilities32W 
 */
INT WINAPI DeviceCapabilitiesW(LPCWSTR pDevice, LPCWSTR pPort,
                                   WORD fwCapability, LPWSTR pOutput,
                                   const DEVMODEW *pDevMode)
{
    FIXME(print,"(%p,%p,%d,%p,%p): stub\n",
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
    FIXME(print,"(%d,%d,%s,%p,%p,%ld):stub.\n",
	hWnd,hPrinter,pDeviceName,pDevModeOutput,pDevModeInput,fMode
    );
    return 1;
}


/*****************************************************************************
 *          DocumentProperties32W 
 */
LONG WINAPI DocumentPropertiesW(HWND hWnd, HANDLE hPrinter,
                                  LPWSTR pDeviceName,
                                  LPDEVMODEW pDevModeOutput,
                                  LPDEVMODEW pDevModeInput, DWORD fMode)
{
    FIXME(print,"(%d,%d,%s,%p,%p,%ld): stub\n",
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
    FIXME(print,"(%s,%p,%p):stub\n",debugstr_a(lpPrinterName), phPrinter,
          pDefault);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************
 *              OpenPrinter32W        [WINSPOOL.197]
 *
 */
BOOL WINAPI OpenPrinterW(LPWSTR lpPrinterName,HANDLE *phPrinter,
			     LPPRINTER_DEFAULTSW pDefault)
{
    FIXME(print,"(%s,%p,%p):stub\n",debugstr_w(lpPrinterName), phPrinter,
          pDefault);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************
 *              EnumPrinters32A        [WINSPOOL.174]
 *
 */
BOOL  WINAPI EnumPrintersA(DWORD dwType, LPSTR lpszName,
			       DWORD dwLevel, LPBYTE lpbPrinters,
			       DWORD cbBuf, LPDWORD lpdwNeeded,
			       LPDWORD lpdwReturned)
{
    FIXME(print,"Nearly empty stub\n");
    *lpdwReturned=0;
    *lpdwNeeded = 0;
    return TRUE;
}

/******************************************************************
 *              EnumPrinters32W        [WINSPOOL.175]
 *
 */
BOOL  WINAPI EnumPrintersW(DWORD dwType, LPWSTR lpszName,
			       DWORD dwLevel, LPBYTE lpbPrinters,
			       DWORD cbBuf, LPDWORD lpdwNeeded,
			       LPDWORD lpdwReturned)
{
    FIXME(print,"Nearly empty stub\n");
    *lpdwReturned=0;
    *lpdwNeeded = 0;
    return TRUE;
}

/******************************************************************
 *              AddMonitor32A        [WINSPOOL.107]
 *
 */
BOOL WINAPI AddMonitorA(LPCSTR pName, DWORD Level, LPBYTE pMonitors)
{
    FIXME(print, "(%s,%lx,%p):stub!\n", pName, Level, pMonitors);
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
    FIXME(print,"(%s,%s,%s):stub\n",debugstr_a(pName),debugstr_a(pEnvironment),
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
    FIXME(print,"(%s,%s,%s):stub\n",debugstr_a(pName),debugstr_a(pEnvironment),
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
    FIXME(print,"(%s,0x%08x,%s):stub\n",debugstr_a(pName),hWnd,
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

       FIXME(print,"():stub\n");
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

       FIXME(print,"():stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
       return FALSE;
}

/*****************************************************************************
 *          AddForm32A  [WINSPOOL.103]
 */
BOOL WINAPI AddFormA(HANDLE hPrinter, DWORD Level, LPBYTE pForm)
{
    FIXME(print, "(%d,%ld,%p): stub\n", hPrinter, Level, pForm);
    return 1;
}

/*****************************************************************************
 *          AddForm32W  [WINSPOOL.104]
 */
BOOL WINAPI AddFormW(HANDLE hPrinter, DWORD Level, LPBYTE pForm)
{
    FIXME(print, "(%d,%ld,%p): stub\n", hPrinter, Level, pForm);
    return 1;
}

/*****************************************************************************
 *          AddJob32A  [WINSPOOL.105]
 */
BOOL WINAPI AddJobA(HANDLE hPrinter, DWORD Level, LPBYTE pData,
                        DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME(print, "(%d,%ld,%p,%ld,%p): stub\n", hPrinter, Level, pData, cbBuf,
          pcbNeeded);
    return 1;
}

/*****************************************************************************
 *          AddJob32W  [WINSPOOL.106]
 */
BOOL WINAPI AddJobW(HANDLE hPrinter, DWORD Level, LPBYTE pData, DWORD cbBuf,
                        LPDWORD pcbNeeded)
{
    FIXME(print, "(%d,%ld,%p,%ld,%p): stub\n", hPrinter, Level, pData, cbBuf,
          pcbNeeded);
    return 1;
}

/*****************************************************************************
 *          AddPrinter32A  [WINSPOOL.117]
 */
HANDLE WINAPI AddPrinterA(LPSTR pName, DWORD Level, LPBYTE pPrinter)
{
    FIXME(print, "(%s,%ld,%p): stub\n", pName, Level, pPrinter);
    return 0;
}

/*****************************************************************************
 *          AddPrinter32W  [WINSPOOL.122]
 */
HANDLE WINAPI AddPrinterW(LPWSTR pName, DWORD Level, LPBYTE pPrinter)
{
    FIXME(print, "(%p,%ld,%p): stub\n", pName, Level, pPrinter);
    return 0;
}


/*****************************************************************************
 *          ClosePrinter32  [WINSPOOL.126]
 */
BOOL WINAPI ClosePrinter(HANDLE hPrinter)
{
    FIXME(print, "(%d): stub\n", hPrinter);
    return 1;
}

/*****************************************************************************
 *          DeleteForm32A  [WINSPOOL.133]
 */
BOOL WINAPI DeleteFormA(HANDLE hPrinter, LPSTR pFormName)
{
    FIXME(print, "(%d,%s): stub\n", hPrinter, pFormName);
    return 1;
}

/*****************************************************************************
 *          DeleteForm32W  [WINSPOOL.134]
 */
BOOL WINAPI DeleteFormW(HANDLE hPrinter, LPWSTR pFormName)
{
    FIXME(print, "(%d,%s): stub\n", hPrinter, debugstr_w(pFormName));
    return 1;
}

/*****************************************************************************
 *          DeletePrinter32  [WINSPOOL.143]
 */
BOOL WINAPI DeletePrinter(HANDLE hPrinter)
{
    FIXME(print, "(%d): stub\n", hPrinter);
    return 1;
}

/*****************************************************************************
 *          SetPrinter32A  [WINSPOOL.211]
 */
BOOL WINAPI SetPrinterA(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
                           DWORD Command)
{
    FIXME(print, "(%d,%ld,%p,%ld): stub\n",hPrinter,Level,pPrinter,Command);
    return FALSE;
}

/*****************************************************************************
 *          SetJob32A  [WINSPOOL.209]
 */
BOOL WINAPI SetJobA(HANDLE hPrinter, DWORD JobId, DWORD Level,
                       LPBYTE pJob, DWORD Command)
{
    FIXME(print, "(%d,%ld,%ld,%p,%ld): stub\n",hPrinter,JobId,Level,pJob,
         Command);
    return FALSE;
}

/*****************************************************************************
 *          SetJob32W  [WINSPOOL.210]
 */
BOOL WINAPI SetJobW(HANDLE hPrinter, DWORD JobId, DWORD Level,
                       LPBYTE pJob, DWORD Command)
{
    FIXME(print, "(%d,%ld,%ld,%p,%ld): stub\n",hPrinter,JobId,Level,pJob,
         Command);
    return FALSE;
}

/*****************************************************************************
 *          GetForm32A  [WINSPOOL.181]
 */
BOOL WINAPI GetFormA(HANDLE hPrinter, LPSTR pFormName, DWORD Level,
                 LPBYTE pForm, DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME(print, "(%d,%s,%ld,%p,%ld,%p): stub\n",hPrinter,pFormName,
         Level,pForm,cbBuf,pcbNeeded); 
    return FALSE;
}

/*****************************************************************************
 *          GetForm32W  [WINSPOOL.182]
 */
BOOL WINAPI GetFormW(HANDLE hPrinter, LPWSTR pFormName, DWORD Level,
                 LPBYTE pForm, DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME(print, "(%d,%s,%ld,%p,%ld,%p): stub\n",hPrinter,
	  debugstr_w(pFormName),Level,pForm,cbBuf,pcbNeeded);
    return FALSE;
}

/*****************************************************************************
 *          SetForm32A  [WINSPOOL.207]
 */
BOOL WINAPI SetFormA(HANDLE hPrinter, LPSTR pFormName, DWORD Level,
                        LPBYTE pForm)
{
    FIXME(print, "(%d,%s,%ld,%p): stub\n",hPrinter,pFormName,Level,pForm);
    return FALSE;
}

/*****************************************************************************
 *          SetForm32W  [WINSPOOL.208]
 */
BOOL WINAPI SetFormW(HANDLE hPrinter, LPWSTR pFormName, DWORD Level,
                        LPBYTE pForm)
{
    FIXME(print, "(%d,%p,%ld,%p): stub\n",hPrinter,pFormName,Level,pForm);
    return FALSE;
}

/*****************************************************************************
 *          ReadPrinter32  [WINSPOOL.202]
 */
BOOL WINAPI ReadPrinter(HANDLE hPrinter, LPVOID pBuf, DWORD cbBuf,
                           LPDWORD pNoBytesRead)
{
    FIXME(print, "(%d,%p,%ld,%p): stub\n",hPrinter,pBuf,cbBuf,pNoBytesRead);
    return FALSE;
}

/*****************************************************************************
 *          ResetPrinter32A  [WINSPOOL.203]
 */
BOOL WINAPI ResetPrinterA(HANDLE hPrinter, LPPRINTER_DEFAULTSA pDefault)
{
    FIXME(print, "(%d, %p): stub\n", hPrinter, pDefault);
    return FALSE;
}

/*****************************************************************************
 *          ResetPrinter32W  [WINSPOOL.204]
 */
BOOL WINAPI ResetPrinterW(HANDLE hPrinter, LPPRINTER_DEFAULTSW pDefault)
{
    FIXME(print, "(%d, %p): stub\n", hPrinter, pDefault);
    return FALSE;
}

/*****************************************************************************
 *          GetPrinter32A  [WINSPOOL.187]
 */
BOOL WINAPI GetPrinterA(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
                    DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME(print, "(%d,%ld,%p,%ld,%p): stub\n", hPrinter, Level, pPrinter, 
         cbBuf, pcbNeeded);
    return FALSE;
}

/*****************************************************************************
 *          GetPrinter32W  [WINSPOOL.194]
 */
BOOL WINAPI GetPrinterW(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
                    DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME(print, "(%d,%ld,%p,%ld,%p): stub\n", hPrinter, Level, pPrinter,
          cbBuf, pcbNeeded);
    return FALSE;
}

/*****************************************************************************
 *          GetPrinterDriver32A  [WINSPOOL.190]
 */
BOOL WINAPI GetPrinterDriverA(HANDLE hPrinter, LPSTR pEnvironment,
                                 DWORD Level, LPBYTE pDriverInfo,
                                 DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME(print, "(%d,%s,%ld,%p,%ld,%p): stub\n",hPrinter,pEnvironment,
         Level,pDriverInfo,cbBuf, pcbNeeded);
    return FALSE;
}

/*****************************************************************************
 *          GetPrinterDriver32W  [WINSPOOL.193]
 */
BOOL WINAPI GetPrinterDriverW(HANDLE hPrinter, LPWSTR pEnvironment,
                                  DWORD Level, LPBYTE pDriverInfo, 
                                  DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME(print, "(%d,%p,%ld,%p,%ld,%p): stub\n",hPrinter,pEnvironment,
          Level,pDriverInfo,cbBuf, pcbNeeded);
    return FALSE;
}
/*****************************************************************************
 *          AddPrinterDriver32A  [WINSPOOL.120]
 */
BOOL WINAPI AddPrinterDriverA(LPSTR printerName,DWORD level, 
				   LPBYTE pDriverInfo)
{
    FIXME(print, "(%s,%ld,%p): stub\n",printerName,level,pDriverInfo);
    return FALSE;
}
/*****************************************************************************
 *          AddPrinterDriver32W  [WINSPOOL.121]
 */
BOOL WINAPI AddPrinterDriverW(LPWSTR printerName,DWORD level, 
				   LPBYTE pDriverInfo)
{
    FIXME(print, "(%s,%ld,%p): stub\n",debugstr_w(printerName),
	  level,pDriverInfo);
    return FALSE;
}



