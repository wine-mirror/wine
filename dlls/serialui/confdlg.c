/*
 * This DLL contains the user interface for the serial driver.
 *    a dialog box to configure the specified COMM port
 *    an interface to the control panel (??)
 *    functions to load and save default configuration
 *
 * Eventually the 32 bit comm port driver could be moved into here
 * and interfaced to KERNEL32 using the WIN95 or WINNT comm driver interface.
 * This way, different driver DLLS could be written to support other
 * serial interfaces, such as X.25, etc.
 *
 * Basic structure copied from COMCTL32 code.
 *
 * Copyright 2000, 2004 Mike McCormack
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
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"
#include "serialui.h"
#include "winerror.h"

WINE_DEFAULT_DEBUG_CHANNEL(comm);

static HMODULE SERIALUI_hModule;

/***********************************************************************
 * DllMain [Internal] Initializes the internal 'SERIALUI.DLL'.
 *
 * PARAMS
 *     hinstDLL    [I] handle to the DLL's instance
 *     fdwReason   [I]
 *     lpvReserved [I] reserved, must be NULL
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p,%lx,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            SERIALUI_hModule = hinstDLL;
	    break;
    }

    return TRUE;
}


/***********************************************************************
 * EnumPropPages (SERIALUI.2)
 *
 * Called by the device manager to add prop sheets in Control Panel ???
 * Pointed to in Win98 registry by
 * \System\CurrentControlSet\Services\Class\ports\0000\EnumPropPages =
 *  "serialui.dll,EnumPropPages"
 */
typedef LPVOID LPDEVICE_INFO;
typedef LPVOID LPFNADDPROPSHEETPAGE;
BOOL WINAPI EnumPropPages(LPDEVICE_INFO pdi, LPFNADDPROPSHEETPAGE pfnAdd, LPARAM lParam )
{
    FIXME("(%p %p %Ix)\n",pdi,pfnAdd,lParam);
    return FALSE;
}

/*
 * These data structures are convert from values used in fields of a DCB
 * to strings used in the CommConfigDialog.
 */
typedef struct tagPARAM2STRDATA
{
    DWORD        val;
    const CHAR  *name;
} PARAM2STRDATA, *LPPARAM2STRDATA;

typedef struct tagPARAM2STR
{
    DWORD         dwSize;
    LPPARAM2STRDATA data;
} PARAM2STR, *LPPARAM2STR;
typedef const PARAM2STR *LPCPARAM2STR;

static PARAM2STRDATA SERIALUI_Baud2StrData[]={
  {110, "110"}, {300, "300"}, {600, "600"}, {1200, "1200"},
  {2400, "2400"}, {4800, "4800"}, {9600, "9600"}, {14400, "14400"},
  {19200, "19200"}, {38400L, "38400"}, {56000L, "56000"}, {57600L, "57600"},
  {115200L, "115200"}, {128000L, "128000"}, {256000L, "256000"}
};
static PARAM2STR SERIALUI_Baud2Str={ ARRAY_SIZE(SERIALUI_Baud2StrData), SERIALUI_Baud2StrData };

static PARAM2STRDATA SERIALUI_Parity2StrData[]={
  {NOPARITY,"None"}, {ODDPARITY,"Odd"}, {EVENPARITY,"Even"}, {MARKPARITY,"Mark"},
  {SPACEPARITY,"Space"}
};
static PARAM2STR SERIALUI_Parity2Str={ ARRAY_SIZE(SERIALUI_Parity2StrData), SERIALUI_Parity2StrData };

static PARAM2STRDATA SERIALUI_Stop2StrData[]={
  {ONESTOPBIT,"1"}, {ONE5STOPBITS,"1.5"}, {TWOSTOPBITS,"2"}
};
static PARAM2STR SERIALUI_Stop2Str={ ARRAY_SIZE(SERIALUI_Stop2StrData), SERIALUI_Stop2StrData };

static PARAM2STRDATA SERIALUI_Data2StrData[]={
  {5,"5"}, {6,"6"}, {7,"7"}, {8, "8"}, {16,"16"}
};
static PARAM2STR SERIALUI_Data2Str={ ARRAY_SIZE(SERIALUI_Data2StrData), SERIALUI_Data2StrData };

static PARAM2STRDATA SERIALUI_Flow2StrData[]={
  {0,"None"}, {1,"Hardware (RTS/CTS)"}, {2,"Software (XON/XOFF)"}
};
static PARAM2STR SERIALUI_Flow2Str={ ARRAY_SIZE(SERIALUI_Flow2StrData), SERIALUI_Flow2StrData };

/*
 * Add all the fields to a combo box and highlight the current value
 */
static void SERIALUI_AddConfItems(HWND hDlg, DWORD id, LPCPARAM2STR table, DWORD dwVal)
{
    unsigned int i;
    int n;
    HWND hControl = GetDlgItem(hDlg,id);

    if(!hControl)
        return;

    for(i=0; i<table->dwSize; i++)
    {
        n = SendMessageA(hControl, CB_ADDSTRING, 0L, (LPARAM)table->data[i].name);
        if(dwVal == table->data[i].val)
	{
            SendMessageA(hControl, CB_SETCURSEL, n, 0);
	}
    }
}

/*
 * Get the current selection of the given combo box and set a DCB field to
 * the value matching that selection.
 */
static BOOL SERIALUI_GetConfItems(HWND hDlg, DWORD id, LPCPARAM2STR table, LPDWORD lpdwVal)
{
    DWORD i;
    CHAR lpEntry[20];
    HWND hControl = GetDlgItem(hDlg,id);

    if( (!hControl) || (!lpdwVal))
    {
        TRACE("Couldn't get window handle for item %lx\n",id);
        return FALSE;
    }

    if(!GetWindowTextA(hControl, &lpEntry[0], sizeof(lpEntry)))
    {
        TRACE("Couldn't get window text for item %lx\n",id);
        return FALSE;
    }
    /* TRACE("%ld contains %s\n",id, lpEntry); */

    for(i=0; i<table->dwSize; i++)
    {
        if(!lstrcmpA(table->data[i].name,lpEntry))
	{
            *lpdwVal = table->data[i].val;
            return TRUE;
	}
    }

    return FALSE;
}

/*
 * Both the enumerated values CBR_XXXX and integer baud rates are valid
 * dcb.BaudRate. This code is to convert back and forth between CBR_ style
 * and integers. The dialog box uses integer values.
 */
static const DWORD SERIALUI_BaudConvertTable[] =  {
  CBR_110, 110, CBR_300, 300, CBR_600, 600, CBR_1200, 1200,
  CBR_2400, 2400, CBR_4800, 4800, CBR_9600, 9600, CBR_14400, 14400,
  CBR_19200, 19200, CBR_38400, 38400, CBR_56000, 56000, CBR_57600, 57600,
  CBR_115200, 115200, CBR_128000, 128000, CBR_256000, 256000
};

static BOOL SERIALUI_MakeBaudDword(LPDWORD lpdwBaudRate)
{
    unsigned int i;

    for(i=0; i<ARRAY_SIZE(SERIALUI_BaudConvertTable); i+=2)
    {
        if(*lpdwBaudRate == SERIALUI_BaudConvertTable[i])
        {
            *lpdwBaudRate = SERIALUI_BaudConvertTable[i+1];
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL SERIALUI_MakeBaudEnum(LPDWORD lpdwBaudRate)
{
    unsigned int i;

    for(i=0; i<ARRAY_SIZE(SERIALUI_BaudConvertTable); i+=2)
    {
        if(*lpdwBaudRate == SERIALUI_BaudConvertTable[i+1])
        {
            *lpdwBaudRate = SERIALUI_BaudConvertTable[i];
            return TRUE;
        }
    }
    return FALSE;
}

typedef struct tagSERIALUI_DialogInfo
{
    LPCWSTR lpszDevice;
    LPCOMMCONFIG lpCommConfig;
    BOOL bConvert; /* baud rate was converted to a DWORD */
    DWORD dwFlowControl; /* old flow control */
} SERIALUI_DialogInfo;

static void SERIALUI_DCBToDialogInfo(HWND hDlg, SERIALUI_DialogInfo *info)
{
    DWORD dwBaudRate, dwStopBits, dwParity, dwByteSize, dwFlowControl;
    LPDCB lpdcb = &info->lpCommConfig->dcb;

    /* pass integer pointers to SERIALUI_ dialog config fns */
    dwBaudRate    = lpdcb->BaudRate;
    dwStopBits    = lpdcb->StopBits;
    dwParity      = lpdcb->Parity;
    dwByteSize    = lpdcb->ByteSize;

    /* map flow control state, if it looks normal */
    if((lpdcb->fRtsControl == RTS_CONTROL_HANDSHAKE) ||
       (lpdcb->fOutxCtsFlow)) {
        dwFlowControl = 1;
    } else if(lpdcb->fOutX || lpdcb->fInX) {
        dwFlowControl = 2;
    } else {
        dwFlowControl = 0;
    }

    info->bConvert = SERIALUI_MakeBaudDword(&dwBaudRate);

    SERIALUI_AddConfItems( hDlg, IDC_BAUD, &SERIALUI_Baud2Str ,dwBaudRate);
    SERIALUI_AddConfItems( hDlg, IDC_STOP, &SERIALUI_Stop2Str ,dwStopBits);
    SERIALUI_AddConfItems( hDlg, IDC_PARITY, &SERIALUI_Parity2Str ,dwParity);
    SERIALUI_AddConfItems( hDlg, IDC_DATA, &SERIALUI_Data2Str ,dwByteSize);
    SERIALUI_AddConfItems( hDlg, IDC_FLOW, &SERIALUI_Flow2Str, dwFlowControl );

    info->dwFlowControl = dwFlowControl;
}

static void SERIALUI_DialogInfoToDCB(HWND hDlg, SERIALUI_DialogInfo *info)
{
    DWORD dwBaudRate, dwStopBits, dwParity, dwByteSize, dwFlowControl;
    LPDCB lpdcb = &info->lpCommConfig->dcb;

    SERIALUI_GetConfItems( hDlg, IDC_BAUD, &SERIALUI_Baud2Str, &dwBaudRate);
    SERIALUI_GetConfItems( hDlg, IDC_STOP, &SERIALUI_Stop2Str, &dwStopBits);
    SERIALUI_GetConfItems( hDlg, IDC_PARITY, &SERIALUI_Parity2Str, &dwParity);
    SERIALUI_GetConfItems( hDlg, IDC_DATA, &SERIALUI_Data2Str, &dwByteSize);
    SERIALUI_GetConfItems( hDlg, IDC_FLOW, &SERIALUI_Flow2Str, &dwFlowControl );

    TRACE("baud=%ld stop=%ld parity=%ld data=%ld flow=%ld\n",
          dwBaudRate, dwStopBits, dwParity, dwByteSize, dwFlowControl);

    lpdcb->BaudRate = dwBaudRate;
    lpdcb->StopBits = dwStopBits;
    lpdcb->Parity   = dwParity;
    lpdcb->ByteSize = dwByteSize;

    /* try not to change flow control if the user didn't change it */
    if(info->dwFlowControl != dwFlowControl)
    {
        switch(dwFlowControl)
        {
        case 0:
            lpdcb->fOutxCtsFlow = FALSE;
            lpdcb->fOutxDsrFlow = FALSE;
            lpdcb->fDtrControl  = DTR_CONTROL_DISABLE;
            lpdcb->fOutX        = FALSE;
            lpdcb->fInX         = FALSE;
            lpdcb->fRtsControl  = RTS_CONTROL_DISABLE;
            break;
        case 1: /* CTS/RTS */
            lpdcb->fOutxCtsFlow = TRUE;
            lpdcb->fOutxDsrFlow = FALSE;
            lpdcb->fDtrControl  = DTR_CONTROL_DISABLE;
            lpdcb->fOutX        = FALSE;
            lpdcb->fInX         = FALSE;
            lpdcb->fRtsControl  = RTS_CONTROL_HANDSHAKE;
            break;
        case 2:
            lpdcb->fOutxCtsFlow = FALSE;
            lpdcb->fOutxDsrFlow = FALSE;
            lpdcb->fDtrControl  = DTR_CONTROL_DISABLE;
            lpdcb->fOutX        = TRUE;
            lpdcb->fInX         = TRUE;
            lpdcb->fRtsControl  = RTS_CONTROL_DISABLE;
            break;
        }
    }

    if(info->bConvert)
        SERIALUI_MakeBaudEnum(&lpdcb->BaudRate);
}

/***********************************************************************
 * SERIALUI_ConfigDialogProc
 *
 * Shows a dialog for configuring a COMM port
 */
static INT_PTR CALLBACK SERIALUI_ConfigDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WCHAR szTitle[128], format[40];
    SERIALUI_DialogInfo *info;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        info = (SERIALUI_DialogInfo*) lParam;
        if(!info)
            return FALSE;
        SetWindowLongPtrW(hWnd, DWLP_USER, lParam);
        GetWindowTextW(hWnd, format, ARRAY_SIZE(format));
        swprintf(szTitle, ARRAY_SIZE(szTitle), format, info->lpszDevice);
        SetWindowTextW(hWnd, szTitle);
        SERIALUI_DCBToDialogInfo(hWnd, info);
        return TRUE;

    case WM_COMMAND:
    {
        WORD wID = LOWORD(wParam);

        info = (SERIALUI_DialogInfo *) GetWindowLongPtrW(hWnd, DWLP_USER);
        if(!info)
            EndDialog(hWnd,0);
        switch (wID)
        {
        case IDOK:
            SERIALUI_DialogInfoToDCB(hWnd,info);
            EndDialog(hWnd, ERROR_SUCCESS);
            return TRUE;
        case IDCANCEL:
            EndDialog(hWnd, ERROR_CANCELLED);
            return TRUE;
/* test code for Get/SetDefaultCommConfig begins */
        case ID_GETDEFAULT:
            {
                DWORD r,dwConfSize = sizeof (COMMCONFIG);
                r = GetDefaultCommConfigW(info->lpszDevice,
                          info->lpCommConfig, &dwConfSize);
                if(!r)
                    MessageBoxA(hWnd,"Failed","GetDefaultCommConfig",MB_OK);
            }
            SERIALUI_DCBToDialogInfo(hWnd, info);
            break;
        case ID_SETDEFAULT:
            {
                DWORD r;
                SERIALUI_DialogInfoToDCB(hWnd,info);
                r = SetDefaultCommConfigW(info->lpszDevice,
                          info->lpCommConfig, sizeof (COMMCONFIG));
                if(!r)
                    MessageBoxA(hWnd,"Failed","GetDefaultCommConfig",MB_OK);
            }
            break;
/* test code for Get/SetDefaultCommConfig ends */
        }
    }
    default:
        return FALSE;
    }
}

static LPWSTR SERIALUI_strdup( LPCSTR str )
{
    DWORD len;
    LPWSTR strW;

    if (!str)
        return NULL;
    len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
    strW = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, str, -1, strW, len );
    return strW;
}

static VOID SERIALUI_strfree( LPWSTR strW )
{
    HeapFree( GetProcessHeap(), 0, strW );
}

/***********************************************************************
 * drvCommConfigDialogW (SERIALUI.@)
 *
 * Show a dialog for configuring a Serial Port.
 *
 */
DWORD WINAPI drvCommConfigDialogW(LPCWSTR lpszName, HWND hWndParent, LPCOMMCONFIG lpCommConfig)
{
    SERIALUI_DialogInfo info;
    INT res;

    info.lpCommConfig  = lpCommConfig;
    info.lpszDevice    = lpszName;
    info.bConvert      = FALSE;
    info.dwFlowControl = 0;

    if ((!lpCommConfig) || (!lpszName))
        return ERROR_INVALID_PARAMETER;

    if (lpCommConfig->dwSize < sizeof(COMMCONFIG))
        return ERROR_INSUFFICIENT_BUFFER;

    if (!lpszName[0])
        return ERROR_BADKEY;

    res = DialogBoxParamW( SERIALUI_hModule,
                           MAKEINTRESOURCEW(IDD_SERIALUICONFIG),
                           hWndParent,
                           SERIALUI_ConfigDialogProc,
                           (LPARAM)&info);

    return (res == -1) ? GetLastError() : res ;
}

/***********************************************************************
 * drvCommConfigDialogA (SERIALUI.@)
 */
DWORD WINAPI drvCommConfigDialogA(LPCSTR lpszName, HWND hWndParent, LPCOMMCONFIG lpCommConfig)
{
    LPWSTR strW = SERIALUI_strdup( lpszName );
    DWORD r = drvCommConfigDialogW( strW, hWndParent, lpCommConfig );
    SERIALUI_strfree( strW );
    return r;
}

static const WCHAR lpszCommKey[] = L"System\\CurrentControlSet\\Services\\Class\\Ports";

/***********************************************************************
 * drvSetDefaultCommConfigW (SERIALUI.@)
 *
 * Used by Win98 KERNEL to set the default config for a COMM port
 * FIXME: uses the wrong registry key... should use a digit, not
 *        the comm port name.
 */
BOOL WINAPI drvSetDefaultCommConfigW(
	LPCWSTR lpszDevice, LPCOMMCONFIG lpCommConfig, DWORD dwSize)
{
    HKEY hKeyReg=0, hKeyPort=0;
    WCHAR szKeyName[100];
    DWORD r,dwDCBSize;

    TRACE("%p %p %lx\n",lpszDevice,lpCommConfig,dwSize);

    if(!lpCommConfig)
        return FALSE;

    if(dwSize < sizeof (COMMCONFIG))
        return FALSE;

    r = RegConnectRegistryW(NULL, HKEY_LOCAL_MACHINE, &hKeyReg);
    if(r != ERROR_SUCCESS)
        return FALSE;

    swprintf(szKeyName, ARRAY_SIZE(szKeyName), L"%s\\%s", lpszCommKey, lpszDevice);
    r = RegCreateKeyW(hKeyReg, szKeyName, &hKeyPort);
    if(r == ERROR_SUCCESS)
    {
        dwDCBSize = sizeof (DCB);
        r = RegSetValueExW(hKeyPort, L"DCB", 0, REG_BINARY, (BYTE *)&lpCommConfig->dcb, dwDCBSize);
        TRACE("write key r=%ld\n",r);
        RegCloseKey(hKeyPort);
    }

    RegCloseKey(hKeyReg);

    return (r==ERROR_SUCCESS);
}

/***********************************************************************
 * drvSetDefaultCommConfigA (SERIALUI.@)
 */
BOOL WINAPI drvSetDefaultCommConfigA(
	LPCSTR lpszDevice, LPCOMMCONFIG lpCommConfig, DWORD dwSize)
{
    LPWSTR strW = SERIALUI_strdup( lpszDevice );
    BOOL r = drvSetDefaultCommConfigW( strW, lpCommConfig, dwSize );
    SERIALUI_strfree( strW );
    return r;
}

/***********************************************************************
 * drvGetDefaultCommConfigW (SERIALUI.@)
 *
 * Used by Win9x KERNEL to get the default config for a COMM port
 * FIXME: uses the wrong registry key... should use a digit, not
 *        the comm port name.
 */
DWORD WINAPI drvGetDefaultCommConfigW(
	LPCWSTR lpszDevice, LPCOMMCONFIG lpCommConfig, LPDWORD lpdwSize)
{
    HKEY hKeyReg, hKeyPort;
    WCHAR szKeyName[100];
    DWORD r,dwSize,dwType;

    TRACE("(%s, %p, %p) *lpdwSize: %lu\n", debugstr_w(lpszDevice), lpCommConfig, lpdwSize, lpdwSize ? *lpdwSize : 0);

    if ((!lpszDevice) || (!lpCommConfig) || (!lpdwSize)) {
        return ERROR_INVALID_PARAMETER;
    }

    if (*lpdwSize < sizeof (COMMCONFIG)) {
        *lpdwSize = sizeof (COMMCONFIG);
        return ERROR_INSUFFICIENT_BUFFER;
    }

    /* only "com1" - "com9" is allowed */
    r = ARRAY_SIZE(L"com");
    lstrcpynW(szKeyName, lpszDevice, r);    /* simulate a lstrcmpnW */
    r--;

    if (lstrcmpiW(szKeyName, L"com") ||
        (lpszDevice[r] < '1') || (lpszDevice[r] > '9') || lpszDevice[r+1]) {
        return ERROR_BADKEY;
    }

    *lpdwSize = sizeof (COMMCONFIG);
    memset(lpCommConfig, 0 , sizeof (COMMCONFIG));
    lpCommConfig->dwSize = sizeof (COMMCONFIG);
    lpCommConfig->wVersion = 1;
    lpCommConfig->dwProviderSubType = PST_RS232;

    r = RegConnectRegistryW(NULL, HKEY_LOCAL_MACHINE, &hKeyReg);
    if(r != ERROR_SUCCESS) return r;

    swprintf(szKeyName, ARRAY_SIZE(szKeyName), L"%s\\%s", lpszCommKey, lpszDevice);
    r = RegOpenKeyW(hKeyReg, szKeyName, &hKeyPort);
    if(r == ERROR_SUCCESS)
    {
        dwSize = sizeof (DCB);
        dwType = 0;
        r = RegQueryValueExW(hKeyPort, L"DCB", NULL, &dwType, (BYTE *)&lpCommConfig->dcb, &dwSize);

        RegCloseKey(hKeyPort);
        if ((r!=ERROR_SUCCESS) || (dwType != REG_BINARY) || (dwSize != sizeof(DCB))) {
            RegCloseKey(hKeyReg);
            return ERROR_INVALID_PARAMETER;
        }

    }
    else
    {
        /* FIXME: default to a hardcoded commconfig */
        lpCommConfig->dcb.DCBlength = sizeof(DCB);
        lpCommConfig->dcb.BaudRate = 9600;
        lpCommConfig->dcb.fBinary = TRUE;
        lpCommConfig->dcb.fParity = FALSE;
        lpCommConfig->dcb.ByteSize = 8;
        lpCommConfig->dcb.Parity = NOPARITY;
        lpCommConfig->dcb.StopBits = ONESTOPBIT;
        return ERROR_SUCCESS;
    }

    RegCloseKey(hKeyReg);

    return r;
}

/***********************************************************************
 * drvGetDefaultCommConfigA (SERIALUI.@)
 */
DWORD WINAPI drvGetDefaultCommConfigA(
	LPCSTR lpszDevice, LPCOMMCONFIG lpCommConfig, LPDWORD lpdwSize)
{
    LPWSTR strW = SERIALUI_strdup( lpszDevice );
    DWORD r = drvGetDefaultCommConfigW( strW, lpCommConfig, lpdwSize );
    SERIALUI_strfree( strW );
    return r;
}
