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

INT16 WINAPI EndDoc16(HDC16 hdc)
{
  return  Escape16(hdc, ENDDOC, 0, 0, 0);
}

DWORD DrvGetPrinterDataInternal(LPSTR RegStr_Printer, LPBYTE lpPrinterData, int cbData)
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


INT32 WINAPI DeviceCapabilities32A(LPCSTR printer,LPCSTR target,WORD z,
                                   LPSTR a,LPDEVMODE32A b)
{
    FIXME(print,"(%s,%s,%d,%p,%p):stub.\n",printer,target,z,a,b);
    return 1;   	
}

LONG WINAPI DocumentProperties32A(HWND32 hWnd,HANDLE32 hPrinter,
                                LPSTR pDeviceName, LPDEVMODE32A pDevModeOutput,
                                  LPDEVMODE32A pDevModeInput,DWORD fMode )
{
    FIXME(print,"(%d,%d,%s,%p,%p,%ld):stub.\n",
	hWnd,hPrinter,pDeviceName,pDevModeOutput,pDevModeInput,fMode
    );
    return 1;
}

BOOL32 WINAPI OpenPrinter32A(LPSTR lpPrinterName,HANDLE32 *phPrinter,
			     LPPRINTER_DEFAULTS32A pDefault)
{
    FIXME(print,"(%s,%p,%p):stub\n",debugstr_a(lpPrinterName), phPrinter,
          pDefault);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL32  WINAPI EnumPrinters32A(DWORD dwType, LPSTR lpszName,
			       DWORD dwLevel, LPBYTE lpbPrinters,
			       DWORD cbBuf, LPDWORD lpdwNeeded,
			       LPDWORD lpdwReturned)
{
    FIXME(print,"Nearly empty stub\n");
    *lpdwReturned=0;
    return TRUE;
}


BOOL32 WINAPI AddMonitor32A(LPCSTR pName, DWORD Level, LPBYTE pMonitors)
{
    FIXME(print, "(%s,%lx,%p):stub!\n", pName, Level, pMonitors);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOL32 WINAPI
DeletePrinterDriver32A (LPSTR pName, LPSTR pEnvironment, LPSTR pDriverName)
{
    FIXME(print,"(%s,%s,%s):stub\n",debugstr_a(pName),debugstr_a(pEnvironment),
          debugstr_a(pDriverName));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOL32 WINAPI
DeleteMonitor32A (LPSTR pName, LPSTR pEnvironment, LPSTR pMonitorName)
{
    FIXME(print,"(%s,%s,%s):stub\n",debugstr_a(pName),debugstr_a(pEnvironment),
          debugstr_a(pMonitorName));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOL32 WINAPI
DeletePort32A (LPSTR pName, HWND32 hWnd, LPSTR pPortName)
{
    FIXME(print,"(%s,0x%08x,%s):stub\n",debugstr_a(pName),hWnd,
          debugstr_a(pPortName));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

