/* 
 * Implementation of some printer driver bits
 * 
 * Copyright 1996 John Harvey
 * Copyright 1998 Andreas Mohr
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "windows.h"
#include "win.h"
#include "winerror.h"
#include "winreg.h"
#include "debug.h"
#include "print.h"

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
 *                  StartDoc32A  [GDI32.347]
 *
 */
INT32 WINAPI StartDoc32A(HDC32 hdc ,const DOCINFO32A* doc)
{
  FIXME(gdi,"stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED); 
  return 0; /* failure*/
}

/*************************************************************************
 *                  StartDoc32W [GDI32.348]
 * 
 */
INT32 WINAPI StartDoc32W(HDC32 hdc, const DOCINFO32W* doc) {
  FIXME(gdi,"stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED); 
  return 0; /* failure*/
}

/******************************************************************
 *                  StartPage32  [GDI32.349]
 *
 */
INT32 WINAPI StartPage32(HDC32 hdc)
{
  FIXME(gdi,"stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED); 
  return 0; /* failure*/
}

/******************************************************************
 *                  EndPage32  [GDI32.77]
 *
 */
INT32 WINAPI EndPage32(HDC32 hdc)
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
INT32 WINAPI EndDoc32(HDC32 hdc)
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
INT32 WINAPI AbortDoc32(HDC32 hdc)
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
LPBYTE lpPrinterData, int cbData)
{
    DWORD res = -1;
    HKEY hkey;
    DWORD dwType, cbQueryData;

    if (!(RegOpenKey32A(HKEY_LOCAL_MACHINE, RegStr_Printer, &hkey))) {
        if (cbData > 1) { /* "Default DevMode" */
            if (!(RegQueryValueEx32A(hkey, DefaultDevMode, 0, &dwType, 0, &cbQueryData))) {
                if (!lpPrinterData)
		    res = cbQueryData;
		else if ((cbQueryData) && (cbQueryData <= cbData)) {
		    cbQueryData = cbData;
		    if (RegQueryValueEx32A(hkey, DefaultDevMode, 0,
				&dwType, lpPrinterData, &cbQueryData))
		        res = cbQueryData;
		}
	    }
	} else { /* "Printer Driver" */
	    cbQueryData = 32;
	    RegQueryValueEx32A(hkey, "Printer Driver", 0,
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
DWORD WINAPI DrvGetPrinterData(LPSTR lpPrinter, LPSTR lpProfile,
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
	size = DrvGetPrinterDataInternal(RegStr_Printer, lpPrinterData, cbData);
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
	size = DrvGetPrinterDataInternal(RegStr_Printer, lpPrinterData, 1);
	if ((size+1) && (lpType))
	    *lpType = REG_SZ;
	else
	    res = ERROR_INVALID_PRINTER_NAME;
    }
    else
    {
	if ((res = RegOpenKey32A(HKEY_LOCAL_MACHINE, RegStr_Printer, &hkey)))
	    goto failed;
        cbPrinterAttr = 4;
        if ((res = RegQueryValueEx32A(hkey, "Attributes", 0,
                        &dwType, (LPBYTE)&PrinterAttr, &cbPrinterAttr)))
	    goto failed;
	if ((res = RegOpenKey32A(hkey, PrinterDriverData, &hkey2)))
	    goto failed;
        *lpNeeded = cbData;
        res = RegQueryValueEx32A(hkey2, lpProfile, 0,
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
	    RegSetValueEx32A(hkey2, lpProfile, 0, REG_DWORD, (LPBYTE)&SetData, 4); /* no result returned */
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
DWORD WINAPI DrvSetPrinterData(LPSTR lpPrinter, LPSTR lpProfile,
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
	if ( RegOpenKey32A(HKEY_LOCAL_MACHINE, RegStr_Printer, &hkey) 
	     != ERROR_SUCCESS ||
	     RegSetValueEx32A(hkey, DefaultDevMode, 0, REG_BINARY, 
			      lpPrinterData, dwSize) != ERROR_SUCCESS )
	        res = ERROR_INVALID_PRINTER_NAME;
    }
    else
    {
	strcat(RegStr_Printer, "\\");

	if( (res = RegOpenKey32A(HKEY_LOCAL_MACHINE, RegStr_Printer, &hkey)) ==
	    ERROR_SUCCESS ) {

	    if (!lpPrinterData) 
	        res = RegDeleteValue32A(hkey, lpProfile);
	    else
                res = RegSetValueEx32A(hkey, lpProfile, 0, lpType,
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
INT32 WINAPI DeviceCapabilities32A(LPCSTR printer,LPCSTR target,WORD z,
                                   LPSTR a,LPDEVMODE32A b)
{
    FIXME(print,"(%s,%s,%d,%p,%p):stub.\n",printer,target,z,a,b);
    return 1;   	
}


/*****************************************************************************
 *          DeviceCapabilities32W 
 */
INT32 WINAPI DeviceCapabilities32W(LPCWSTR pDevice, LPCWSTR pPort,
                                   WORD fwCapability, LPWSTR pOutput,
                                   const DEVMODE32W *pDevMode)
{
    FIXME(print,"(%p,%p,%d,%p,%p): stub\n",
          pDevice, pPort, fwCapability, pOutput, pDevMode);
    return -1;
}

/******************************************************************
 *              DocumentProperties32A   [WINSPOOL.155]
 *
 */
LONG WINAPI DocumentProperties32A(HWND32 hWnd,HANDLE32 hPrinter,
                                LPSTR pDeviceName, LPDEVMODE32A pDevModeOutput,
                                  LPDEVMODE32A pDevModeInput,DWORD fMode )
{
    FIXME(print,"(%d,%d,%s,%p,%p,%ld):stub.\n",
	hWnd,hPrinter,pDeviceName,pDevModeOutput,pDevModeInput,fMode
    );
    return 1;
}


/*****************************************************************************
 *          DocumentProperties32W 
 */
LONG WINAPI DocumentProperties32W(HWND32 hWnd, HANDLE32 hPrinter,
                                  LPWSTR pDeviceName,
                                  LPDEVMODE32W pDevModeOutput,
                                  LPDEVMODE32W pDevModeInput, DWORD fMode)
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
BOOL32 WINAPI OpenPrinter32A(LPSTR lpPrinterName,HANDLE32 *phPrinter,
			     LPPRINTER_DEFAULTS32A pDefault)
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
BOOL32 WINAPI OpenPrinter32W(LPWSTR lpPrinterName,HANDLE32 *phPrinter,
			     LPPRINTER_DEFAULTS32W pDefault)
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
BOOL32  WINAPI EnumPrinters32A(DWORD dwType, LPSTR lpszName,
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
BOOL32  WINAPI EnumPrinters32W(DWORD dwType, LPWSTR lpszName,
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
BOOL32 WINAPI AddMonitor32A(LPCSTR pName, DWORD Level, LPBYTE pMonitors)
{
    FIXME(print, "(%s,%lx,%p):stub!\n", pName, Level, pMonitors);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************
 *              DeletePrinterDriver32A        [WINSPOOL.146]
 *
 */
BOOL32 WINAPI
DeletePrinterDriver32A (LPSTR pName, LPSTR pEnvironment, LPSTR pDriverName)
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
BOOL32 WINAPI
DeleteMonitor32A (LPSTR pName, LPSTR pEnvironment, LPSTR pMonitorName)
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
BOOL32 WINAPI
DeletePort32A (LPSTR pName, HWND32 hWnd, LPSTR pPortName)
{
    FIXME(print,"(%s,0x%08x,%s):stub\n",debugstr_a(pName),hWnd,
          debugstr_a(pPortName));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************************
 *    SetPrinter32W  [WINSPOOL.214]
 */
BOOL32 WINAPI
SetPrinter32W(
  HANDLE32  hPrinter,
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
BOOL32 WINAPI
WritePrinter32( 
  HANDLE32  hPrinter,
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
BOOL32 WINAPI AddForm32A(HANDLE32 hPrinter, DWORD Level, LPBYTE pForm)
{
    FIXME(print, "(%d,%ld,%p): stub\n", hPrinter, Level, pForm);
    return 1;
}

/*****************************************************************************
 *          AddForm32W  [WINSPOOL.104]
 */
BOOL32 WINAPI AddForm32W(HANDLE32 hPrinter, DWORD Level, LPBYTE pForm)
{
    FIXME(print, "(%d,%ld,%p): stub\n", hPrinter, Level, pForm);
    return 1;
}

/*****************************************************************************
 *          AddJob32A  [WINSPOOL.105]
 */
BOOL32 WINAPI AddJob32A(HANDLE32 hPrinter, DWORD Level, LPBYTE pData,
                        DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME(print, "(%d,%ld,%p,%ld,%p): stub\n", hPrinter, Level, pData, cbBuf,
          pcbNeeded);
    return 1;
}

/*****************************************************************************
 *          AddJob32W  [WINSPOOL.106]
 */
BOOL32 WINAPI AddJob32W(HANDLE32 hPrinter, DWORD Level, LPBYTE pData, DWORD cbBuf,
                        LPDWORD pcbNeeded)
{
    FIXME(print, "(%d,%ld,%p,%ld,%p): stub\n", hPrinter, Level, pData, cbBuf,
          pcbNeeded);
    return 1;
}

/*****************************************************************************
 *          AddPrinter32A  [WINSPOOL.117]
 */
HANDLE32 WINAPI AddPrinter32A(LPSTR pName, DWORD Level, LPBYTE pPrinter)
{
    FIXME(print, "(%s,%ld,%p): stub\n", pName, Level, pPrinter);
    return 0;
}

/*****************************************************************************
 *          AddPrinter32W  [WINSPOOL.122]
 */
HANDLE32 WINAPI AddPrinter32W(LPWSTR pName, DWORD Level, LPBYTE pPrinter)
{
    FIXME(print, "(%p,%ld,%p): stub\n", pName, Level, pPrinter);
    return 0;
}


/*****************************************************************************
 *          ClosePrinter32  [WINSPOOL.126]
 */
BOOL32 WINAPI ClosePrinter32(HANDLE32 hPrinter)
{
    FIXME(print, "(%d): stub\n", hPrinter);
    return 1;
}

/*****************************************************************************
 *          DeleteForm32A  [WINSPOOL.133]
 */
BOOL32 WINAPI DeleteForm32A(HANDLE32 hPrinter, LPSTR pFormName)
{
    FIXME(print, "(%d,%s): stub\n", hPrinter, pFormName);
    return 1;
}

/*****************************************************************************
 *          DeleteForm32W  [WINSPOOL.134]
 */
BOOL32 WINAPI DeleteForm32W(HANDLE32 hPrinter, LPWSTR pFormName)
{
    FIXME(print, "(%d,%s): stub\n", hPrinter, debugstr_w(pFormName));
    return 1;
}

/*****************************************************************************
 *          DeletePrinter32  [WINSPOOL.143]
 */
BOOL32 DeletePrinter32(HANDLE32 hPrinter)
{
    FIXME(print, "(%d): stub\n", hPrinter);
    return 1;
}

/*****************************************************************************
 *          SetPrinter32A  [WINSPOOL.211]
 */
BOOL32 WINAPI SetPrinter32A(HANDLE32 hPrinter, DWORD Level, LPBYTE pPrinter,
                           DWORD Command)
{
    FIXME(print, "(%d,%ld,%p,%ld): stub\n",hPrinter,Level,pPrinter,Command);
    return FALSE;
}

/*****************************************************************************
 *          SetJob32A  [WINSPOOL.209]
 */
BOOL32 WINAPI SetJob32A(HANDLE32 hPrinter, DWORD JobId, DWORD Level,
                       LPBYTE pJob, DWORD Command)
{
    FIXME(print, "(%d,%ld,%ld,%p,%ld): stub\n",hPrinter,JobId,Level,pJob,
         Command);
    return FALSE;
}

/*****************************************************************************
 *          SetJob32W  [WINSPOOL.210]
 */
BOOL32 WINAPI SetJob32W(HANDLE32 hPrinter, DWORD JobId, DWORD Level,
                       LPBYTE pJob, DWORD Command)
{
    FIXME(print, "(%d,%ld,%ld,%p,%ld): stub\n",hPrinter,JobId,Level,pJob,
         Command);
    return FALSE;
}

/*****************************************************************************
 *          GetForm32A  [WINSPOOL.181]
 */
BOOL32 WINAPI GetForm32A(HANDLE32 hPrinter, LPSTR pFormName, DWORD Level,
                 LPBYTE pForm, DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME(print, "(%d,%s,%ld,%p,%ld,%p): stub\n",hPrinter,pFormName,
         Level,pForm,cbBuf,pcbNeeded); 
    return FALSE;
}

/*****************************************************************************
 *          GetForm32W  [WINSPOOL.182]
 */
BOOL32 WINAPI GetForm32W(HANDLE32 hPrinter, LPWSTR pFormName, DWORD Level,
                 LPBYTE pForm, DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME(print, "(%d,%s,%ld,%p,%ld,%p): stub\n",hPrinter,
	  debugstr_w(pFormName),Level,pForm,cbBuf,pcbNeeded);
    return FALSE;
}

/*****************************************************************************
 *          SetForm32A  [WINSPOOL.207]
 */
BOOL32 WINAPI SetForm32A(HANDLE32 hPrinter, LPSTR pFormName, DWORD Level,
                        LPBYTE pForm)
{
    FIXME(print, "(%d,%s,%ld,%p): stub\n",hPrinter,pFormName,Level,pForm);
    return FALSE;
}

/*****************************************************************************
 *          SetForm32W  [WINSPOOL.208]
 */
BOOL32 WINAPI SetForm32W(HANDLE32 hPrinter, LPWSTR pFormName, DWORD Level,
                        LPBYTE pForm)
{
    FIXME(print, "(%d,%p,%ld,%p): stub\n",hPrinter,pFormName,Level,pForm);
    return FALSE;
}

/*****************************************************************************
 *          ReadPrinter32  [WINSPOOL.202]
 */
BOOL32 WINAPI ReadPrinter32(HANDLE32 hPrinter, LPVOID pBuf, DWORD cbBuf,
                           LPDWORD pNoBytesRead)
{
    FIXME(print, "(%d,%p,%ld,%p): stub\n",hPrinter,pBuf,cbBuf,pNoBytesRead);
    return FALSE;
}

/*****************************************************************************
 *          ResetPrinter32A  [WINSPOOL.203]
 */
BOOL32 WINAPI ResetPrinter32A(HANDLE32 hPrinter, LPPRINTER_DEFAULTS32A pDefault)
{
    FIXME(print, "(%d, %p): stub\n", hPrinter, pDefault);
    return FALSE;
}

/*****************************************************************************
 *          ResetPrinter32W  [WINSPOOL.204]
 */
BOOL32 WINAPI ResetPrinter32W(HANDLE32 hPrinter, LPPRINTER_DEFAULTS32W pDefault)
{
    FIXME(print, "(%d, %p): stub\n", hPrinter, pDefault);
    return FALSE;
}

/*****************************************************************************
 *          GetPrinter32A  [WINSPOOL.187]
 */
BOOL32 WINAPI GetPrinter32A(HANDLE32 hPrinter, DWORD Level, LPBYTE pPrinter,
                    DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME(print, "(%d,%ld,%p,%ld,%p): stub\n", hPrinter, Level, pPrinter, 
         cbBuf, pcbNeeded);
    return FALSE;
}

/*****************************************************************************
 *          GetPrinter32W  [WINSPOOL.194]
 */
BOOL32 WINAPI GetPrinter32W(HANDLE32 hPrinter, DWORD Level, LPBYTE pPrinter,
                    DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME(print, "(%d,%ld,%p,%ld,%p): stub\n", hPrinter, Level, pPrinter,
          cbBuf, pcbNeeded);
    return FALSE;
}

/*****************************************************************************
 *          GetPrinterDriver32A  [WINSPOOL.190]
 */
BOOL32 WINAPI GetPrinterDriver32A(HANDLE32 hPrinter, LPSTR pEnvironment,
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
BOOL32 WINAPI GetPrinterDriver32W(HANDLE32 hPrinter, LPWSTR pEnvironment,
                                  DWORD Level, LPBYTE pDriverInfo, 
                                  DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME(print, "(%d,%p,%ld,%p,%ld,%p): stub\n",hPrinter,pEnvironment,
          Level,pDriverInfo,cbBuf, pcbNeeded);
    return FALSE;
}



