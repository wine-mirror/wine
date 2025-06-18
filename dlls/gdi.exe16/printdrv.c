/*
 * Implementation of some printer driver bits
 *
 * Copyright 1996 John Harvey
 * Copyright 1998 Huw Davies
 * Copyright 1998 Andreas Mohr
 * Copyright 1999 Klaas van Gend
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
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wine/winbase16.h"
#include "wine/wingdi16.h"
#include "winspool.h"
#include "winerror.h"
#include "winreg.h"
#include "wownt32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(print);

static const char PrinterModel[]      = "Printer Model";
static const char DefaultDevMode[]    = "Default DevMode";
static const char PrinterDriverData[] = "PrinterDriverData";
static const char Printers[]          = "System\\CurrentControlSet\\Control\\Print\\Printers\\";

/****************** misc. printer related functions */

/*
 * The following function should implement a queuing system
 */
struct hpq
{
    struct hpq 	*next;
    int		 tag;
    int		 key;
};

static struct hpq *hpqueue;

/**********************************************************************
 *           CreatePQ   (GDI.230)
 *
 */
HPQ16 WINAPI CreatePQ16(INT16 size)
{
#if 0
    HGLOBAL16 hpq = 0;
    WORD tmp_size;
    LPWORD pPQ;

    tmp_size = size << 2;
    if (!(hpq = GlobalAlloc16(GMEM_SHARE|GMEM_MOVEABLE, tmp_size + 8)))
       return 0xffff;
    pPQ = GlobalLock16(hpq);
    *pPQ++ = 0;
    *pPQ++ = tmp_size;
    *pPQ++ = 0;
    *pPQ++ = 0;
    GlobalUnlock16(hpq);

    return (HPQ16)hpq;
#else
    FIXME("(%d): stub\n",size);
    return 1;
#endif
}

/**********************************************************************
 *           DeletePQ   (GDI.235)
 *
 */
INT16 WINAPI DeletePQ16(HPQ16 hPQ)
{
    return GlobalFree16(hPQ);
}

/**********************************************************************
 *           ExtractPQ   (GDI.232)
 *
 */
INT16 WINAPI ExtractPQ16(HPQ16 hPQ)
{
    struct hpq *queue, *prev, *current, *currentPrev;
    int key = 0, tag = -1;
    prev = NULL;
    queue = current = hpqueue;
    if (current)
        key = current->key;

    while (current)
    {
        currentPrev = current;
        current = current->next;
        if (current)
        {
            if (current->key < key)
            {
                queue = current;
                prev = currentPrev;
            }
        }
    }
    if (queue)
    {
        tag = queue->tag;

        if (prev)
            prev->next = queue->next;
        else
            hpqueue = queue->next;
        HeapFree(GetProcessHeap(), 0, queue);
    }

    TRACE("%x got tag %d key %d\n", hPQ, tag, key);

    return tag;
}

/**********************************************************************
 *           InsertPQ   (GDI.233)
 *
 */
INT16 WINAPI InsertPQ16(HPQ16 hPQ, INT16 tag, INT16 key)
{
    struct hpq *queueItem = HeapAlloc(GetProcessHeap(), 0, sizeof(struct hpq));
    if(queueItem == NULL) {
        ERR("Memory exhausted!\n");
        return FALSE;
    }
    queueItem->next = hpqueue;
    hpqueue = queueItem;
    queueItem->key = key;
    queueItem->tag = tag;

    FIXME("(%x %d %d): stub???\n", hPQ, tag, key);
    return TRUE;
}

/**********************************************************************
 *           MinPQ   (GDI.231)
 *
 */
INT16 WINAPI MinPQ16(HPQ16 hPQ)
{
    FIXME("(%x): stub\n", hPQ);
    return 0;
}

/**********************************************************************
 *           SizePQ   (GDI.234)
 *
 */
INT16 WINAPI SizePQ16(HPQ16 hPQ, INT16 sizechange)
{
    FIXME("(%x %d): stub\n", hPQ, sizechange);
    return -1;
}



/*
 * The following functions implement part of the spooling process to
 * print manager.  I would like to see wine have a version of print managers
 * that used LPR/LPD.  For simplicity print jobs will be sent to a file for
 * now.
 */
typedef struct PRINTJOB
{
    HDC16  	hDC;
    HANDLE16 	hHandle;
    int		nIndex;
    int		id;
} PRINTJOB, *PPRINTJOB;

#define MAX_PRINT_JOBS 1
#define SP_OK 1

static PPRINTJOB gPrintJobsTable[MAX_PRINT_JOBS];


static PPRINTJOB FindPrintJobFromHandle(HANDLE16 hHandle)
{
    return gPrintJobsTable[0];
}

/**********************************************************************
 *           OpenJob   (GDI.240)
 *
 */
HPJOB16 WINAPI OpenJob16(LPCSTR lpOutput, LPCSTR lpTitle, HDC16 hDC)
{
    HPJOB16 hHandle = (HPJOB16)SP_ERROR;
    PPRINTJOB pPrintJob;

    TRACE("'%s' '%s' %04x\n", lpOutput, lpTitle, hDC);

    pPrintJob = gPrintJobsTable[0];
    if (pPrintJob == NULL)
    {
        DOCINFOA info = { sizeof(info), lpTitle, lpOutput, NULL, 0 };
        int id;

        id = StartDocA( HDC_32(hDC), &info );
        if (id > 0)
	{
	    pPrintJob = HeapAlloc(GetProcessHeap(), 0, sizeof(PRINTJOB));
            if(pPrintJob == NULL) {
                WARN("Memory exhausted!\n");
                return hHandle;
            }

            hHandle = 1;

	    pPrintJob->hDC = hDC;
	    pPrintJob->id = id;
	    pPrintJob->nIndex = 0;
	    pPrintJob->hHandle = hHandle;
	    gPrintJobsTable[pPrintJob->nIndex] = pPrintJob;
	}
    }
    TRACE("return %04x\n", hHandle);
    return hHandle;
}

/**********************************************************************
 *           CloseJob   (GDI.243)
 *
 */
INT16 WINAPI CloseJob16(HPJOB16 hJob)
{
    int nRet = SP_ERROR;
    PPRINTJOB pPrintJob;

    TRACE("%04x\n", hJob);

    pPrintJob = FindPrintJobFromHandle(hJob);
    if (pPrintJob != NULL) nRet = EndDoc( HDC_32(pPrintJob->hDC) );
    return nRet;
}

/**********************************************************************
 *           WriteSpool   (GDI.241)
 *
 */
INT16 WINAPI WriteSpool16(HPJOB16 hJob, LPSTR lpData, INT16 cch)
{
    int nRet = SP_ERROR;
    PPRINTJOB pPrintJob;

    TRACE("%04x %p %04x\n", hJob, lpData, cch);

    pPrintJob = FindPrintJobFromHandle(hJob);
    if (pPrintJob != NULL && cch)
    {
        WORD *data = HeapAlloc( GetProcessHeap(), 0, cch + 2 );

        if (!data) return SP_OUTOFDISK;
        *data = cch;
        memcpy( data + 1, lpData, cch );
        ExtEscape( HDC_32(pPrintJob->hDC), PASSTHROUGH, cch + 2, (char *)data, 0, NULL );
        HeapFree( GetProcessHeap(), 0, data );
        nRet = cch;
    }
    return nRet;
}

typedef INT (WINAPI *MSGBOX_PROC)( HWND, LPCSTR, LPCSTR, UINT );

/**********************************************************************
 *           WriteDialog   (GDI.242)
 *
 */
INT16 WINAPI WriteDialog16(HPJOB16 hJob, LPSTR lpMsg, INT16 cchMsg)
{
    HMODULE mod;
    MSGBOX_PROC pMessageBoxA;
    INT16 ret = 0;

    TRACE("%04x %04x '%s'\n", hJob,  cchMsg, lpMsg);

    if ((mod = GetModuleHandleA("user32.dll")))
    {
        if ((pMessageBoxA = (MSGBOX_PROC)GetProcAddress( mod, "MessageBoxA" )))
            ret = pMessageBoxA(0, lpMsg, "Printing Error", MB_OKCANCEL);
    }
    return ret;
}


/**********************************************************************
 *           DeleteJob  (GDI.244)
 *
 */
INT16 WINAPI DeleteJob16(HPJOB16 hJob, INT16 nNotUsed)
{
    TRACE("%04x\n", hJob);

    return CloseJob16( hJob );
}

/*
 * The following two function would allow a page to be sent to the printer
 * when it has been processed.  For simplicity they haven't been implemented.
 * This means a whole job has to be processed before it is sent to the printer.
 */

/**********************************************************************
 *           StartSpoolPage   (GDI.246)
 *
 */
INT16 WINAPI StartSpoolPage16(HPJOB16 hJob)
{
    FIXME("StartSpoolPage GDI.246 unimplemented\n");
    return 1;

}


/**********************************************************************
 *           EndSpoolPage   (GDI.247)
 *
 */
INT16 WINAPI EndSpoolPage16(HPJOB16 hJob)
{
    FIXME("EndSpoolPage GDI.247 unimplemented\n");
    return 1;
}


/**********************************************************************
 *           GetSpoolJob   (GDI.245)
 *
 */
DWORD WINAPI GetSpoolJob16(int nOption, LONG param)
{
    TRACE("In GetSpoolJob param 0x%lx noption %d\n",param, nOption);
    return 0;
}


/******************************************************************
 *                  DrvGetPrinterDataInternal
 *
 * Helper for DrvGetPrinterData
 */
static DWORD DrvGetPrinterDataInternal(LPCSTR RegStr_Printer,
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
 *                DrvGetPrinterData     (GDI.282)
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
            TRACE("printer %s\n",lpPrinter);
    else
            TRACE("printer %p\n",lpPrinter);
    if (HIWORD(lpProfile))
            TRACE("profile %s\n",lpProfile);
    else
            TRACE("profile %p\n",lpProfile);
    TRACE("lpType %p\n",lpType);

    if ((!lpPrinter) || (!lpProfile) || (!lpNeeded))
	return ERROR_INVALID_PARAMETER;

    RegStr_Printer = HeapAlloc(GetProcessHeap(), 0,
                               strlen(Printers) + strlen(lpPrinter) + 2);
    strcpy(RegStr_Printer, Printers);
    strcat(RegStr_Printer, lpPrinter);

    if ((PtrToUlong(lpProfile) == INT_PD_DEFAULT_DEVMODE) || (HIWORD(lpProfile) &&
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
    if ((PtrToUlong(lpProfile) == INT_PD_DEFAULT_MODEL) || (HIWORD(lpProfile) &&
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
 *                 DrvSetPrinterData     (GDI.281)
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
            TRACE("printer %s\n",lpPrinter);
    else
            TRACE("printer %p\n",lpPrinter);
    if (HIWORD(lpProfile))
            TRACE("profile %s\n",lpProfile);
    else
            TRACE("profile %p\n",lpProfile);
    TRACE("lpType %08lx\n",lpType);

    if ((!lpPrinter) || (!lpProfile) ||
    (PtrToUlong(lpProfile) == INT_PD_DEFAULT_MODEL) || (HIWORD(lpProfile) &&
    (!strcmp(lpProfile, PrinterModel))))
	return ERROR_INVALID_PARAMETER;

    RegStr_Printer = HeapAlloc(GetProcessHeap(), 0,
			strlen(Printers) + strlen(lpPrinter) + 2);
    strcpy(RegStr_Printer, Printers);
    strcat(RegStr_Printer, lpPrinter);

    if ((PtrToUlong(lpProfile) == INT_PD_DEFAULT_DEVMODE) || (HIWORD(lpProfile) &&
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
