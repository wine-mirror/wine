/*
 * Print spooler and PQ functions
 *
 * Copyright 1996 John Harvey
 *           1998 Huw Davies
 *
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "callback.h"
#include "dc.h"
#include "debug.h"
#include "gdi.h"
#include "options.h"
#include "wine/winuser16.h"
#include "winerror.h"
#include "xmalloc.h"

/**********************************************************************
 *           QueryAbort   (GDI.155)
 *
 *  Calls the app's AbortProc function if avail.
 *
 * RETURNS
 * TRUE if no AbortProc avail or AbortProc wants to continue printing.
 * FALSE if AbortProc wants to abort printing.
 */
BOOL16 WINAPI QueryAbort(HDC16 hdc, INT16 reserved)
{
    DC *dc = DC_GetDCPtr( hdc );

    if ((!dc) || (!dc->w.lpfnPrint))
	return TRUE;
    return Callbacks->CallDrvAbortProc(dc->w.lpfnPrint, hdc, 0);
}

/**********************************************************************
 *           SetAbortProc16   (GDI.381)
 *
 */
INT16 WINAPI SetAbortProc16(HDC16 hdc, SEGPTR abrtprc)
{
    return Escape16(hdc, SETABORTPROC, 0, abrtprc, (SEGPTR)0);
} 

/**********************************************************************
 *           SetAbortProc32   (GDI32.301)
 *
 */
INT32 WINAPI SetAbortProc32(HDC32 hdc, FARPROC32 abrtprc)
{
    FIXME(print, "stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return SP_ERROR;
}


/****************** misc. printer related functions */

/*
 * The following function should implement a queing system
 */
#ifndef HPQ 
#define HPQ WORD
#endif
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
HPQ WINAPI CreatePQ(int size) 
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

    return (HPQ)hpq;
#else
    FIXME(print, "(%d): stub\n",size);
    return 1;
#endif
}

/**********************************************************************
 *           DeletePQ   (GDI.235)
 *
 */
int WINAPI DeletePQ(HPQ hPQ) 
{
    return GlobalFree16((HGLOBAL16)hPQ);
}

/**********************************************************************
 *           ExtractPQ   (GDI.232)
 *
 */
int WINAPI ExtractPQ(HPQ hPQ) 
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
        free(queue);
    }
    
    TRACE(print, "%x got tag %d key %d\n", hPQ, tag, key); 

    return tag;
}

/**********************************************************************
 *           InsertPQ   (GDI.233)
 *
 */
int WINAPI InsertPQ(HPQ hPQ, int tag, int key) 
{
    struct hpq *queueItem = xmalloc(sizeof(struct hpq));
    queueItem->next = hpqueue;
    hpqueue = queueItem;
    queueItem->key = key;
    queueItem->tag = tag;
    
    FIXME(print, "(%x %d %d): stub???\n", hPQ, tag, key);
    return TRUE;
}

/**********************************************************************
 *           MinPQ   (GDI.231)
 *
 */
int WINAPI MinPQ(HPQ hPQ) 
{
    FIXME(print, "(%x): stub\n", hPQ); 
    return 0;
}

/**********************************************************************
 *           SizePQ   (GDI.234)
 *
 */
int WINAPI SizePQ(HPQ hPQ, int sizechange) 
{  
    FIXME(print, "(%x %d): stub\n", hPQ, sizechange); 
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
static int CreateSpoolFile(LPSTR pszOutput)
{
    int fd=-1;
    char psCmd[1024];
    char *psCmdP = psCmd;

    /* TTD convert the 'output device' into a spool file name */

    if (pszOutput == NULL || *pszOutput == '\0')
      return -1;

    PROFILE_GetWineIniString( "spooler", pszOutput, "", psCmd, sizeof(psCmd) );
    TRACE(print, "Got printerSpoolCommand '%s' for output device '%s'\n",
	  psCmd, pszOutput);
    if (!*psCmd)
        psCmdP = pszOutput;
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

            TRACE(print, "In child need to exec %s\n",psCmdP);
            close(0);
            dup2(fds[0],0);
            close (fds[1]);
            system(psCmdP);
            exit(0);
            
        }
        close (fds[0]);
        fd = fds[1];
        TRACE(print,"Need to execute a cmnd and pipe the output to it\n");
    }
    else
    {
        TRACE(print, "Just assume its a file\n");

        if ((fd = open(psCmdP, O_CREAT | O_TRUNC | O_WRONLY , 0600)) < 0)
        {
            ERR(print, "Failed to create spool file %s, errno = %d\n", 
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
	free(pPrintJob->pszOutput);
	free(pPrintJob->pszTitle);
	if (pPrintJob->fd >= 0) close(pPrintJob->fd);
	free(pPrintJob);
	nRet = SP_OK;
    }
    return nRet;
}

/**********************************************************************
 *           OpenJob   (GDI.240)
 *
 */
HANDLE16 WINAPI OpenJob(LPSTR lpOutput, LPSTR lpTitle, HDC16 hDC)
{
    HANDLE16 hHandle = (HANDLE16)SP_ERROR;
    PPRINTJOB pPrintJob;

    TRACE(print, "'%s' '%s' %04x\n", lpOutput, lpTitle, hDC);

    pPrintJob = gPrintJobsTable[0];
    if (pPrintJob == NULL)
    {
	int fd;

	/* Try an create a spool file */
	fd = CreateSpoolFile(lpOutput);
	if (fd >= 0)
	{
	    hHandle = 1;

	    pPrintJob = xmalloc(sizeof(PRINTJOB));
	    memset(pPrintJob, 0, sizeof(PRINTJOB));

	    pPrintJob->pszOutput = strdup(lpOutput);
	    if(lpTitle)
	        pPrintJob->pszTitle = strdup(lpTitle);
	    pPrintJob->hDC = hDC;
	    pPrintJob->fd = fd;
	    pPrintJob->nIndex = 0;
	    pPrintJob->hHandle = hHandle; 
	    gPrintJobsTable[pPrintJob->nIndex] = pPrintJob; 
	}
    }
    TRACE(print, "return %04x\n", hHandle);
    return hHandle;
}

/**********************************************************************
 *           CloseJob   (GDI.243)
 *
 */
int WINAPI CloseJob(HANDLE16 hJob)
{
    int nRet = SP_ERROR;
    PPRINTJOB pPrintJob = NULL;

    TRACE(print, "%04x\n", hJob);

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
int WINAPI WriteSpool(HANDLE16 hJob, LPSTR lpData, WORD cch)
{
    int nRet = SP_ERROR;
    PPRINTJOB pPrintJob = NULL;

    TRACE(print, "%04x %08lx %04x\n", hJob, (DWORD)lpData, cch);

    pPrintJob = FindPrintJobFromHandle(hJob);
    if (pPrintJob != NULL && pPrintJob->fd >= 0 && cch)
    {
	if (write(pPrintJob->fd, lpData, cch) != cch)
	  nRet = SP_OUTOFDISK;
	else
	  nRet = cch;
	if (pPrintJob->hDC == 0) {
	    TRACE(print, "hDC == 0 so no QueryAbort\n");
	}
        else if (!(QueryAbort(pPrintJob->hDC, (nRet == SP_OUTOFDISK) ? nRet : 0 )))
	{
	    CloseJob(hJob); /* printing aborted */
	    nRet = SP_APPABORT;
	}
    }
    return nRet;
}

/**********************************************************************
 *           WriteDialog   (GDI.242)
 *
 */
int WINAPI WriteDialog(HANDLE16 hJob, LPSTR lpMsg, WORD cchMsg)
{
    int nRet = 0;

    TRACE(print, "%04x %04x '%s'\n", hJob,  cchMsg, lpMsg);

    nRet = MessageBox16(0, lpMsg, "Printing Error", MB_OKCANCEL);
    return nRet;
}


/**********************************************************************
 *           DeleteJob  (GDI.244)
 *
 */
int WINAPI DeleteJob(HANDLE16 hJob, WORD wNotUsed)
{
    int nRet;

    TRACE(print, "%04x\n", hJob);

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
int WINAPI StartSpoolPage(HANDLE16 hJob)
{
    FIXME(print, "StartSpoolPage GDI.246 unimplemented\n");
    return 1;

}


/**********************************************************************
 *           EndSpoolPage   (GDI.247)
 *
 */
int WINAPI EndSpoolPage(HANDLE16 hJob)
{
    FIXME(print, "EndSpoolPage GDI.247 unimplemented\n");
    return 1;
}


/**********************************************************************
 *           GetSpoolJob   (GDI.245)
 *
 */
DWORD WINAPI GetSpoolJob(int nOption, LONG param)
{
    DWORD retval = 0;
    TRACE(print, "In GetSpoolJob param 0x%lx noption %d\n",param, nOption);
    return retval;
}
