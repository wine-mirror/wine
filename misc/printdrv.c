/* 
 * Implementation of some printer driver bits
 * 
 * Copyright 1996 John Harvey
 * Copyright 1998 Huw Davies
 * Copyright 1998 Andreas Mohr
 * Copyright 1999 Klaas van Gend
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "ldt.h"
#include "winbase.h"
#include "wine/wingdi16.h"
#include "winspool.h"
#include "winerror.h"
#include "winreg.h"
#include "debugtools.h"
#include "gdi.h"
#include "dc.h"
#include "callback.h"
#include "options.h"
#include "heap.h"

DEFAULT_DEBUG_CHANNEL(print)

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
    DOCINFOA docA;

    docA.cbSize = lpdoc->cbSize;
    docA.lpszDocName = PTR_SEG_TO_LIN(lpdoc->lpszDocName);
    docA.lpszOutput = PTR_SEG_TO_LIN(lpdoc->lpszOutput);

    if(lpdoc->cbSize >= 14)
        docA.lpszDatatype = PTR_SEG_TO_LIN(lpdoc->lpszDatatype);
    else
        docA.lpszDatatype = NULL;

    if(lpdoc->cbSize >= 18)
        docA.fwType = lpdoc->fwType;
    else
        docA.fwType = 0;

    return StartDocA(hdc, &docA);
}

/******************************************************************
 *                  StartDocA  [GDI32.347]
 *
 * StartDoc calls the STARTDOC Escape with the input data pointing to DocName
 * and the output data (which is used as a second input parameter).pointing at
 * the whole docinfo structure.  This seems to be an undocumented feature of
 * the STARTDOC Escape. 
 */
INT WINAPI StartDocA(HDC hdc, const DOCINFOA* doc)
{
    DC *dc = DC_GetDCPtr( hdc );

    TRACE("DocName = '%s' Output = '%s' Datatype = '%s'\n",
	  doc->lpszDocName, doc->lpszOutput, doc->lpszDatatype);

    if(!dc) return 0;

    if(dc->funcs->pStartDoc)
        return dc->funcs->pStartDoc( dc, doc );
    else
        return Escape(hdc, STARTDOC, strlen(doc->lpszDocName),
		      doc->lpszDocName, (LPVOID)doc);
}

/*************************************************************************
 *                  StartDocW [GDI32.348]
 * 
 */
INT WINAPI StartDocW(HDC hdc, const DOCINFOW* doc)
{
    DOCINFOA docA;
    INT ret;

    docA.cbSize = doc->cbSize;
    docA.lpszDocName = doc->lpszDocName ? 
      HEAP_strdupWtoA( GetProcessHeap(), 0, doc->lpszDocName ) : NULL;
    docA.lpszOutput = doc->lpszOutput ?
      HEAP_strdupWtoA( GetProcessHeap(), 0, doc->lpszOutput ) : NULL;
    docA.lpszDatatype = doc->lpszDatatype ?
      HEAP_strdupWtoA( GetProcessHeap(), 0, doc->lpszDatatype ) : NULL;
    docA.fwType = doc->fwType;

    ret = StartDocA(hdc, &docA);

    if(docA.lpszDocName)
        HeapFree( GetProcessHeap(), 0, (LPSTR)docA.lpszDocName );
    if(docA.lpszOutput)
        HeapFree( GetProcessHeap(), 0, (LPSTR)docA.lpszOutput );
    if(docA.lpszDatatype)
        HeapFree( GetProcessHeap(), 0, (LPSTR)docA.lpszDatatype );

    return ret;
}

/******************************************************************
 *                  EndDoc16  [GDI.378]
 *
 */
INT16 WINAPI EndDoc16(HDC16 hdc)
{
    return EndDoc(hdc);
}

/******************************************************************
 *                  EndDoc  [GDI32.76]
 *
 */
INT WINAPI EndDoc(HDC hdc)
{
    DC *dc = DC_GetDCPtr( hdc );
    if(!dc) return 0;

    if(dc->funcs->pEndDoc)
        return dc->funcs->pEndDoc( dc );
    else
        return Escape(hdc, ENDDOC, 0, 0, 0);
}

/******************************************************************
 *                  StartPage16  [GDI.379]
 *
 */
INT16 WINAPI StartPage16(HDC16 hdc)
{
    return StartPage(hdc);
}

/******************************************************************
 *                  StartPage  [GDI32.349]
 *
 */
INT WINAPI StartPage(HDC hdc)
{
    DC *dc = DC_GetDCPtr( hdc );
    if(!dc) return 0;

    if(dc->funcs->pStartPage)
        return dc->funcs->pStartPage( dc );

    FIXME("stub\n");
    return 1;
}

/******************************************************************
 *                  EndPage16  [GDI.380]
 *
 */
INT16 WINAPI EndPage16( HDC16 hdc )
{
    return EndPage(hdc);
}

/******************************************************************
 *                  EndPage  [GDI32.77]
 *
 */
INT WINAPI EndPage(HDC hdc)
{
    DC *dc = DC_GetDCPtr( hdc );
    if(!dc) return 0;

    if(dc->funcs->pEndPage)
        return dc->funcs->pEndPage( dc );
    else
        return Escape(hdc, NEWFRAME, 0, 0, 0);
}

/******************************************************************************
 *                 AbortDoc16  [GDI.382]
 */
INT16 WINAPI AbortDoc16(HDC16 hdc)
{
    return AbortDoc(hdc);
}

/******************************************************************************
 *                 AbortDoc  [GDI32.105]
 */
INT WINAPI AbortDoc(HDC hdc)
{
    DC *dc = DC_GetDCPtr( hdc );
    if(!dc) return 0;

    if(dc->funcs->pAbortDoc)
        return dc->funcs->pAbortDoc( dc );
    else
        return Escape(hdc, ABORTDOC, 0, 0, 0);
}

/**********************************************************************
 *           QueryAbort16   (GDI.155)
 *
 *  Calls the app's AbortProc function if avail.
 *
 * RETURNS
 * TRUE if no AbortProc avail or AbortProc wants to continue printing.
 * FALSE if AbortProc wants to abort printing.
 */
BOOL16 WINAPI QueryAbort16(HDC16 hdc, INT16 reserved)
{
    DC *dc = DC_GetDCPtr( hdc );

    if(!dc) {
        ERR("Invalid hdc %04x\n", hdc);
	return FALSE;
    }

    if(!dc->w.pAbortProc)
        return TRUE;
    return dc->w.pAbortProc(hdc, 0);
}

/* ### start build ### */
extern WORD CALLBACK PRTDRV_CallTo16_word_ww(FARPROC16,WORD,WORD);
/* ### stop build ### */

/**********************************************************************
 *           SetAbortProc16   (GDI.381)
 *
 */
INT16 WINAPI SetAbortProc16(HDC16 hdc, SEGPTR abrtprc)
{
    ABORTPROC proc32 = (ABORTPROC)THUNK_Alloc((FARPROC16)abrtprc,
					      (RELAY)PRTDRV_CallTo16_word_ww);
    return SetAbortProc(hdc, proc32);
}

/**********************************************************************
 *           SetAbortProc32   (GDI32.301)
 *
 */
INT WINAPI SetAbortProc(HDC hdc, ABORTPROC abrtprc)
{
    DC *dc = DC_GetDCPtr( hdc );

    if(dc->w.pAbortProc) THUNK_Free((FARPROC)dc->w.pAbortProc);
    dc->w.pAbortProc = abrtprc;
    return TRUE;
}


/****************** misc. printer related functions */

/*
 * The following function should implement a queing system
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
    return GlobalFree16((HGLOBAL16)hPQ);
}

/**********************************************************************
 *           ExtractPQ   (GDI.232)
 *
 */
INT16 WINAPI ExtractPQ16(HPQ16 hPQ) 
{ 
    struct hpq *queue, *prev, *current, *currentPrev;
    int key = 0, tag = -1;
    currentPrev = prev = NULL;
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
        ERR("Memory exausted!");
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
    char	*pszOutput;
    char 	*pszTitle;
    HDC16  	hDC;
    HANDLE16 	hHandle;
    int		nIndex;
    int		fd;
} PRINTJOB, *PPRINTJOB;

#define MAX_PRINT_JOBS 1
#define SP_OK 1

PPRINTJOB gPrintJobsTable[MAX_PRINT_JOBS];


static PPRINTJOB FindPrintJobFromHandle(HANDLE16 hHandle)
{
    return gPrintJobsTable[0];
}

/* TTD Need to do some DOS->UNIX file conversion here */
static int CreateSpoolFile(LPCSTR pszOutput)
{
    int fd=-1;
    char psCmd[1024];
    char *psCmdP = psCmd;

    /* TTD convert the 'output device' into a spool file name */

    if (pszOutput == NULL || *pszOutput == '\0')
      return -1;

    PROFILE_GetWineIniString( "spooler", pszOutput, "", psCmd, sizeof(psCmd) );
    TRACE("Got printerSpoolCommand '%s' for output device '%s'\n",
	  psCmd, pszOutput);
    if (!*psCmd)
        psCmdP = (char *)pszOutput;
    else
    {
        while (*psCmdP && isspace(*psCmdP))
        {
            psCmdP++;
        };
        if (!*psCmdP)
            return -1;
    }
    if (*psCmdP == '|')
    {
        int fds[2];
        if (pipe(fds))
            return -1;
        if (fork() == 0)
        {
            psCmdP++;

            TRACE("In child need to exec %s\n",psCmdP);
            close(0);
            dup2(fds[0],0);
            close (fds[1]);
            system(psCmdP);
            exit(0);
            
        }
        close (fds[0]);
        fd = fds[1];
        TRACE("Need to execute a cmnd and pipe the output to it\n");
    }
    else
    {
        TRACE("Just assume its a file\n");

        if ((fd = open(psCmdP, O_CREAT | O_TRUNC | O_WRONLY , 0600)) < 0)
        {
            ERR("Failed to create spool file %s, errno = %d\n", 
		psCmdP, errno);
        }
    }
    return fd;
}

static int FreePrintJob(HANDLE16 hJob)
{
    int nRet = SP_ERROR;
    PPRINTJOB pPrintJob;

    pPrintJob = FindPrintJobFromHandle(hJob);
    if (pPrintJob != NULL)
    {
	gPrintJobsTable[pPrintJob->nIndex] = NULL;
	HeapFree(GetProcessHeap(), 0, pPrintJob->pszOutput);
	HeapFree(GetProcessHeap(), 0, pPrintJob->pszTitle);
	if (pPrintJob->fd >= 0) close(pPrintJob->fd);
	HeapFree(GetProcessHeap(), 0, pPrintJob);
	nRet = SP_OK;
    }
    return nRet;
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
	int fd;

	/* Try an create a spool file */
	fd = CreateSpoolFile(lpOutput);
	if (fd >= 0)
	{
	    pPrintJob = HeapAlloc(GetProcessHeap(), 0, sizeof(PRINTJOB));
            if(pPrintJob == NULL) {
                WARN("Memory exausted!");
                return hHandle;
            }
            
            hHandle = 1;

	    pPrintJob->pszOutput = HEAP_strdupA(GetProcessHeap(), 0, lpOutput);
	    if(lpTitle)
	        pPrintJob->pszTitle = HEAP_strdupA(GetProcessHeap(), 0, lpTitle);
	    pPrintJob->hDC = hDC;
	    pPrintJob->fd = fd;
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
    PPRINTJOB pPrintJob = NULL;

    TRACE("%04x\n", hJob);

    pPrintJob = FindPrintJobFromHandle(hJob);
    if (pPrintJob != NULL)
    {
	/* Close the spool file */
	close(pPrintJob->fd);
	FreePrintJob(hJob);
	nRet  = 1;
    }
    return nRet;
}

/**********************************************************************
 *           WriteSpool   (GDI.241)
 *
 */
INT16 WINAPI WriteSpool16(HPJOB16 hJob, LPSTR lpData, INT16 cch)
{
    int nRet = SP_ERROR;
    PPRINTJOB pPrintJob = NULL;

    TRACE("%04x %08lx %04x\n", hJob, (DWORD)lpData, cch);

    pPrintJob = FindPrintJobFromHandle(hJob);
    if (pPrintJob != NULL && pPrintJob->fd >= 0 && cch)
    {
	if (write(pPrintJob->fd, lpData, cch) != cch)
	  nRet = SP_OUTOFDISK;
	else
	  nRet = cch;
	if (pPrintJob->hDC == 0) {
	    TRACE("hDC == 0 so no QueryAbort\n");
	}
        else if (!(QueryAbort16(pPrintJob->hDC, (nRet == SP_OUTOFDISK) ? nRet : 0 )))
	{
	    CloseJob16(hJob); /* printing aborted */
	    nRet = SP_APPABORT;
	}
    }
    return nRet;
}

/**********************************************************************
 *           WriteDialog   (GDI.242)
 *
 */
INT16 WINAPI WriteDialog16(HPJOB16 hJob, LPSTR lpMsg, INT16 cchMsg)
{
    int nRet = 0;

    TRACE("%04x %04x '%s'\n", hJob,  cchMsg, lpMsg);

    nRet = MessageBox16(0, lpMsg, "Printing Error", MB_OKCANCEL);
    return nRet;
}


/**********************************************************************
 *           DeleteJob  (GDI.244)
 *
 */
INT16 WINAPI DeleteJob16(HPJOB16 hJob, INT16 nNotUsed)
{
    int nRet;

    TRACE("%04x\n", hJob);

    nRet = FreePrintJob(hJob);
    return nRet;
}

/* 
 * The following two function would allow a page to be sent to the printer
 * when it has been processed.  For simplicity they havn't been implemented.
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
    DWORD retval = 0;
    TRACE("In GetSpoolJob param 0x%lx noption %d\n",param, nOption);
    return retval;
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
            TRACE("printer %s\n",lpPrinter);
    else
            TRACE("printer %p\n",lpPrinter);
    if (HIWORD(lpProfile))
            TRACE("profile %s\n",lpProfile);
    else
            TRACE("profile %p\n",lpProfile);
    TRACE("lpType %08lx\n",lpType);

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
