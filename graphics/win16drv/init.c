/*
 * Windows Device Context initialisation functions
 *
 * Copyright 1996 John Harvey
 *           1998 Huw Davies
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "windows.h"
#include "win16drv.h"
#include "gdi.h"
#include "bitmap.h"
#include "heap.h"
#include "color.h"
#include "font.h"
#include "callback.h"
#include "options.h"
#include "debug.h"
#include "dc.h"

#define SUPPORT_REALIZED_FONTS 1
#pragma pack(1)
typedef struct
{
  SHORT nSize;
  SEGPTR lpindata;
  SEGPTR lpFont;
  SEGPTR lpXForm;
  SEGPTR lpDrawMode;
} EXTTEXTDATA, *LPEXTTEXTDATA;
#pragma pack(4)

SEGPTR		win16drv_SegPtr_TextXForm;
LPTEXTXFORM16 	win16drv_TextXFormP;
SEGPTR		win16drv_SegPtr_DrawMode;
LPDRAWMODE 	win16drv_DrawModeP;


static BOOL32 WIN16DRV_CreateDC( DC *dc, LPCSTR driver, LPCSTR device,
                                 LPCSTR output, const DEVMODE16* initData );
static INT32 WIN16DRV_Escape( DC *dc, INT32 nEscape, INT32 cbInput, 
                              SEGPTR lpInData, SEGPTR lpOutData );

static const DC_FUNCTIONS WIN16DRV_Funcs =
{
    NULL,                            /* pArc */
    NULL,                            /* pBitBlt */
    NULL,                            /* pChord */
    WIN16DRV_CreateDC,               /* pCreateDC */
    NULL,                            /* pDeleteDC */
    NULL,                            /* pDeleteObject */
    WIN16DRV_Ellipse,                /* pEllipse */
    WIN16DRV_EnumDeviceFonts,        /* pEnumDeviceFonts */
    WIN16DRV_Escape,                 /* pEscape */
    NULL,                            /* pExcludeClipRect */
    NULL,                            /* pExcludeVisRect */
    NULL,                            /* pExtFloodFill */
    WIN16DRV_ExtTextOut,             /* pExtTextOut */
    WIN16DRV_GetCharWidth,           /* pGetCharWidth */
    NULL,                            /* pGetPixel */
    WIN16DRV_GetTextExtentPoint,     /* pGetTextExtentPoint */
    WIN16DRV_GetTextMetrics,         /* pGetTextMetrics */
    NULL,                            /* pIntersectClipRect */
    NULL,                            /* pIntersectVisRect */
    WIN16DRV_LineTo,                 /* pLineTo */
    WIN16DRV_MoveToEx,               /* pMoveToEx */
    NULL,                            /* pOffsetClipRgn */
    NULL,                            /* pOffsetViewportOrgEx */
    NULL,                            /* pOffsetWindowOrgEx */
    NULL,                            /* pPaintRgn */
    WIN16DRV_PatBlt,                 /* pPatBlt */
    NULL,                            /* pPie */
    NULL,                            /* pPolyPolygon */
    NULL,                            /* pPolyPolyline */
    WIN16DRV_Polygon,                /* pPolygon */
    WIN16DRV_Polyline,               /* pPolyline */
    NULL,                            /* pPolyBezier */
    NULL,                            /* pRealizePalette */
    WIN16DRV_Rectangle,              /* pRectangle */
    NULL,                            /* pRestoreDC */
    NULL,                            /* pRoundRect */
    NULL,                            /* pSaveDC */
    NULL,                            /* pScaleViewportExtEx */
    NULL,                            /* pScaleWindowExtEx */
    NULL,                            /* pSelectClipRgn */
    WIN16DRV_SelectObject,           /* pSelectObject */
    NULL,                            /* pSelectPalette */
    NULL,                            /* pSetBkColor */
    NULL,                            /* pSetBkMode */
    NULL,                            /* pSetDeviceClipping */
    NULL,                            /* pSetDIBitsToDevice */
    NULL,                            /* pSetMapMode */
    NULL,                            /* pSetMapperFlags */
    NULL,                            /* pSetPixel */
    NULL,                            /* pSetPolyFillMode */
    NULL,                            /* pSetROP2 */
    NULL,                            /* pSetRelAbs */
    NULL,                            /* pSetStretchBltMode */
    NULL,                            /* pSetTextAlign */
    NULL,                            /* pSetTextCharacterExtra */
    NULL,                            /* pSetTextColor */
    NULL,                            /* pSetTextJustification */
    NULL,                            /* pSetViewportExtEx */
    NULL,                            /* pSetViewportOrgEx */
    NULL,                            /* pSetWindowExtEx */
    NULL,                            /* pSetWindowOrgEx */
    NULL,                            /* pStretchBlt */
    NULL                             /* pStretchDIBits */
};





/**********************************************************************
 *	     WIN16DRV_Init
 */
BOOL32 WIN16DRV_Init(void)
{
    return DRIVER_RegisterDriver( NULL /* generic driver */, &WIN16DRV_Funcs );
        
}

/* Tempory functions, for initialising structures */
/* These values should be calculated, not hardcoded */
void InitTextXForm(LPTEXTXFORM16 lpTextXForm)
{
    lpTextXForm->txfHeight 	= 0x0001;
    lpTextXForm->txfWidth 	= 0x000c;
    lpTextXForm->txfEscapement 	= 0x0000;
    lpTextXForm->txfOrientation = 0x0000;
    lpTextXForm->txfWeight 	= 0x0190;
    lpTextXForm->txfItalic 	= 0x00;
    lpTextXForm->txfUnderline 	= 0x00;
    lpTextXForm->txfStrikeOut 	= 0x00; 
    lpTextXForm->txfOutPrecision = 0x02;
    lpTextXForm->txfClipPrecision = 0x01;
    lpTextXForm->txfAccelerator = 0x0001;
    lpTextXForm->txfOverhang 	= 0x0000;
}


void InitDrawMode(LPDRAWMODE lpDrawMode)
{
    lpDrawMode->Rop2		= 0x000d;       
    lpDrawMode->bkMode		= 0x0001;     
    lpDrawMode->bkColor		= 0x3fffffff;    
    lpDrawMode->TextColor	= 0x20000000;  
    lpDrawMode->TBreakExtra	= 0x0000;
    lpDrawMode->BreakExtra	= 0x0000; 
    lpDrawMode->BreakErr	= 0x0000;   
    lpDrawMode->BreakRem	= 0x0000;   
    lpDrawMode->BreakCount	= 0x0000; 
    lpDrawMode->CharExtra	= 0x0000;  
    lpDrawMode->LbkColor	= 0x00ffffff;   
    lpDrawMode->LTextColor	= 0x00000000;     
}

BOOL32 WIN16DRV_CreateDC( DC *dc, LPCSTR driver, LPCSTR device, LPCSTR output,
                          const DEVMODE16* initData )
{
    LOADED_PRINTER_DRIVER *pLPD;
    WORD wRet;
    DeviceCaps *printerDevCaps;
    int nPDEVICEsize;
    PDEVICE_HEADER *pPDH;
    WIN16DRV_PDEVICE *physDev;
    char printerEnabled[20];
    PROFILE_GetWineIniString( "wine", "printer", "off",
                             printerEnabled, sizeof(printerEnabled) );
    if (lstrcmpi32A(printerEnabled,"on"))
    {
        MSG("Printing disabled in wine.conf or .winerc file\n");
        MSG("Use \"printer=on\" in the \"[wine]\" section to enable it.\n");
        return FALSE;
    }

    TRACE(win16drv, "In creatdc for (%s,%s,%s) initData 0x%p\n",
	  driver, device, output, initData);

    physDev = (WIN16DRV_PDEVICE *)HeapAlloc( SystemHeap, 0, sizeof(*physDev) );
    if (!physDev) return FALSE;
    dc->physDev = physDev;

    pLPD = LoadPrinterDriver(driver);
    if (pLPD == NULL)
    {
	WARN(win16drv, "Failed to find printer driver\n");
        HeapFree( SystemHeap, 0, physDev );
        return FALSE;
    }
    TRACE(win16drv, "windevCreateDC pLPD 0x%p\n", pLPD);

    /* Now Get the device capabilities from the printer driver */
    
    printerDevCaps = (DeviceCaps *) malloc(sizeof(DeviceCaps));
    memset(printerDevCaps, 0, sizeof(DeviceCaps));

    /* Get GDIINFO which is the same as a DeviceCaps structure */
    wRet = PRTDRV_Enable(printerDevCaps, GETGDIINFO, device, driver, output,NULL); 

    /* Add this to the DC */
    dc->w.devCaps = printerDevCaps;
    dc->w.hVisRgn = CreateRectRgn32(0, 0, dc->w.devCaps->horzRes, dc->w.devCaps->vertRes);
    dc->w.bitsPerPixel = dc->w.devCaps->bitsPixel;
    
    TRACE(win16drv, "Got devcaps width %d height %d bits %d planes %d\n",
	  dc->w.devCaps->horzRes, dc->w.devCaps->vertRes, 
	  dc->w.devCaps->bitsPixel, dc->w.devCaps->planes);

    /* Now we allocate enough memory for the PDEVICE structure */
    /* The size of this varies between printer drivers */
    /* This PDEVICE is used by the printer DRIVER not by the GDI so must */
    /* be accessable from 16 bit code */
    nPDEVICEsize = dc->w.devCaps->pdeviceSize + sizeof(PDEVICE_HEADER);

    /* TTD Shouldn't really do pointer arithmetic on segment points */
    physDev->segptrPDEVICE = WIN16_GlobalLock16(GlobalAlloc16(GHND, nPDEVICEsize))+sizeof(PDEVICE_HEADER);
    *((BYTE *)PTR_SEG_TO_LIN(physDev->segptrPDEVICE)+0) = 'N'; 
    *((BYTE *)PTR_SEG_TO_LIN(physDev->segptrPDEVICE)+1) = 'B'; 

    /* Set up the header */
    pPDH = (PDEVICE_HEADER *)((BYTE*)PTR_SEG_TO_LIN(physDev->segptrPDEVICE) - sizeof(PDEVICE_HEADER)); 
    pPDH->pLPD = pLPD;
    
    TRACE(win16drv, "PDEVICE allocated %08lx\n",(DWORD)(physDev->segptrPDEVICE));
    
    /* Now get the printer driver to initialise this data */
    wRet = PRTDRV_Enable((LPVOID)physDev->segptrPDEVICE, INITPDEVICE, device, driver, output, NULL); 

    physDev->FontInfo = NULL;
    physDev->BrushInfo = NULL;
    physDev->PenInfo = NULL;
    win16drv_SegPtr_TextXForm = WIN16_GlobalLock16(GlobalAlloc16(GHND, sizeof(TEXTXFORM16)));
    win16drv_TextXFormP = PTR_SEG_TO_LIN(win16drv_SegPtr_TextXForm);
    
    InitTextXForm(win16drv_TextXFormP);

    /* TTD Lots more to do here */
    win16drv_SegPtr_DrawMode = WIN16_GlobalLock16(GlobalAlloc16(GHND, sizeof(DRAWMODE)));
    win16drv_DrawModeP = PTR_SEG_TO_LIN(win16drv_SegPtr_DrawMode);
    
    InitDrawMode(win16drv_DrawModeP);

    return TRUE;
}

BOOL32 WIN16DRV_PatBlt( struct tagDC *dc, INT32 left, INT32 top,
			INT32 width, INT32 height, DWORD rop )
{
  
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    BOOL32 bRet = 0;

    bRet = PRTDRV_StretchBlt( physDev->segptrPDEVICE, left, top, width, height, (SEGPTR)NULL, 0, 0, width, height,
                       PATCOPY, physDev->BrushInfo, win16drv_SegPtr_DrawMode, NULL);

    return bRet;
}
/* 
 * Escape (GDI.38)
 */
static INT32 WIN16DRV_Escape( DC *dc, INT32 nEscape, INT32 cbInput, 
                              SEGPTR lpInData, SEGPTR lpOutData )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    int nRet = 0;

    /* We should really process the nEscape parameter, but for now just
       pass it all to the driver */
    if (dc != NULL && physDev->segptrPDEVICE != 0)
    {
	switch(nEscape)
          {
	  case ENABLEPAIRKERNING:
	    FIXME(win16drv,"Escape: ENABLEPAIRKERNING ignored.\n");
            nRet = 1;
	    break;
	  case GETPAIRKERNTABLE:
	    FIXME(win16drv,"Escape: GETPAIRKERNTABLE ignored.\n");
            nRet = 0;
	    break;
          case SETABORTPROC:
		/* FIXME: The AbortProc should be called:
		- After every write to printer port or spool file
		- Several times when no more disk space
		- Before every metafile record when GDI does banding
		*/ 
/*	    dc->w.lpfnPrint = (FARPROC16)lpInData; FIXME! */
            nRet = 1;
	    break;

          case NEXTBAND:
            {
              LPPOINT16 newInData =  SEGPTR_NEW(POINT16);

              nRet = PRTDRV_Control(physDev->segptrPDEVICE, nEscape,
                                    SEGPTR_GET(newInData), lpOutData);
              SEGPTR_FREE(newInData);
              break;
            }

	  case GETEXTENDEDTEXTMETRICS:
            {
	      EXTTEXTDATA *textData = SEGPTR_NEW(EXTTEXTDATA);

              textData->nSize = cbInput;
              textData->lpindata = lpInData;
              textData->lpFont = SEGPTR_GET( physDev->FontInfo );
              textData->lpXForm = win16drv_SegPtr_TextXForm;
              textData->lpDrawMode = win16drv_SegPtr_DrawMode;
              nRet = PRTDRV_Control(physDev->segptrPDEVICE, nEscape,
                                    SEGPTR_GET(textData), lpOutData);
              SEGPTR_FREE(textData);
            }
          break;
          case STARTDOC:
	    nRet = PRTDRV_Control(physDev->segptrPDEVICE, nEscape,
				  lpInData, lpOutData);
            if (nRet != -1)
            {
              HDC32 *tmpHdc = SEGPTR_NEW(HDC32);

#define SETPRINTERDC SETABORTPROC

              *tmpHdc = dc->hSelf;
              PRTDRV_Control(physDev->segptrPDEVICE, SETPRINTERDC,
                             SEGPTR_GET(tmpHdc), (SEGPTR)NULL);
              SEGPTR_FREE(tmpHdc);
            }
            break;
	  default:
	    nRet = PRTDRV_Control(physDev->segptrPDEVICE, nEscape,
				  lpInData, lpOutData);
            break;
	}
    }
    else
	WARN(win16drv, "Escape(nEscape = %04x) - ???\n", nEscape);      
    return nRet;
}


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
    return dc->w.lpfnPrint(hdc, 0);
}

/**********************************************************************
 *           SetAbortProc   (GDI.381)
 *
 */
INT16 WINAPI SetAbortProc(HDC16 hdc, FARPROC16 abrtprc)
{
    DC *dc = DC_GetDCPtr( hdc );

    if (dc) {
	dc->w.lpfnPrint = abrtprc;
	return 1;
    }
    return -1;
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
    FIXME(win16drv, "(%d): stub\n",size);
    return 1;
#endif
}

int WINAPI DeletePQ(HPQ hPQ) 
{
    return GlobalFree16((HGLOBAL16)hPQ);
}

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
    
    TRACE(win16drv, "%x got tag %d key %d\n", hPQ, tag, key); 

    return tag;
}

int WINAPI InsertPQ(HPQ hPQ, int tag, int key) 
{
    struct hpq *queueItem = malloc(sizeof(struct hpq));
    queueItem->next = hpqueue;
    hpqueue = queueItem;
    queueItem->key = key;
    queueItem->tag = tag;
    
    FIXME(win16drv, "(%x %d %d): stub???\n", hPQ, tag, key);
    return TRUE;
}
int WINAPI MinPQ(HPQ hPQ) 
{
    FIXME(win16drv, "(%x): stub\n", hPQ); 
    return 0;
}
int WINAPI SizePQ(HPQ hPQ, int sizechange) 
{  
    FIXME(win16drv, "(%x %d): stub\n", hPQ, sizechange); 
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
    TRACE(win16drv, "Got printerSpoolCommand '%s' for output device '%s'\n",
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

            TRACE(win16drv, "In child need to exec %s\n",psCmdP);
            close(0);
            dup2(fds[0],0);
            close (fds[1]);
            system(psCmdP);
            exit(0);
            
        }
        close (fds[0]);
        fd = fds[1];
        TRACE(win16drv,"Need to execute a cmnd and pipe the output to it\n");
    }
    else
    {
        TRACE(win16drv, "Just assume its a file\n");

        if ((fd = open(psCmdP, O_CREAT | O_TRUNC | O_WRONLY , 0600)) < 0)
        {
            ERR(win16drv, "Failed to create spool file %s, errno = %d\n", 
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

HANDLE16 WINAPI OpenJob(LPSTR lpOutput, LPSTR lpTitle, HDC16 hDC)
{
    HANDLE16 hHandle = (HANDLE16)SP_ERROR;
    PPRINTJOB pPrintJob;

    TRACE(win16drv, "'%s' '%s' %04x\n", lpOutput, lpTitle, hDC);

    pPrintJob = gPrintJobsTable[0];
    if (pPrintJob == NULL)
    {
	int fd;

	/* Try an create a spool file */
	fd = CreateSpoolFile(lpOutput);
	if (fd >= 0)
	{
	    hHandle = 1;

	    pPrintJob = malloc(sizeof(PRINTJOB));
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
    TRACE(win16drv, "return %04x\n", hHandle);
    return hHandle;
}

int WINAPI CloseJob(HANDLE16 hJob)
{
    int nRet = SP_ERROR;
    PPRINTJOB pPrintJob = NULL;

    TRACE(win16drv, "%04x\n", hJob);

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

int WINAPI WriteSpool(HANDLE16 hJob, LPSTR lpData, WORD cch)
{
    int nRet = SP_ERROR;
    PPRINTJOB pPrintJob = NULL;

    TRACE(win16drv, "%04x %08lx %04x\n", hJob, (DWORD)lpData, cch);

    pPrintJob = FindPrintJobFromHandle(hJob);
    if (pPrintJob != NULL && pPrintJob->fd >= 0 && cch)
    {
	if (write(pPrintJob->fd, lpData, cch) != cch)
	  nRet = SP_OUTOFDISK;
	else
	  nRet = cch;
	if (pPrintJob->hDC == 0) {
	    ERR(print, "hDC == 0 !\n");
	}
        else if (!(QueryAbort(pPrintJob->hDC, (nRet == SP_OUTOFDISK) ? nRet : 0 )))
	{
	    CloseJob(hJob); /* printing aborted */
	    nRet = SP_APPABORT;
	}
    }
    return nRet;
}

int WINAPI WriteDialog(HANDLE16 hJob, LPSTR lpMsg, WORD cchMsg)
{
    int nRet = 0;

    TRACE(win16drv, "%04x %04x '%s'\n", hJob,  cchMsg, lpMsg);

    nRet = MessageBox16(0, lpMsg, "Printing Error", MB_OKCANCEL);
    return nRet;
}

int WINAPI DeleteJob(HANDLE16 hJob, WORD wNotUsed)
{
    int nRet;

    TRACE(win16drv, "%04x\n", hJob);

    nRet = FreePrintJob(hJob);
    return nRet;
}

/* 
 * The following two function would allow a page to be sent to the printer
 * when it has been processed.  For simplicity they havn't been implemented.
 * This means a whole job has to be processed before it is sent to the printer.
 */
int WINAPI StartSpoolPage(HANDLE16 hJob)
{
    FIXME(win16drv, "StartSpoolPage GDI.246 unimplemented\n");
    return 1;

}
int WINAPI EndSpoolPage(HANDLE16 hJob)
{
    FIXME(win16drv, "EndSpoolPage GDI.247 unimplemented\n");
    return 1;
}


DWORD WINAPI GetSpoolJob(int nOption, LONG param)
{
    DWORD retval = 0;
    TRACE(win16drv, "In GetSpoolJob param 0x%lx noption %d\n",param, nOption);
    return retval;
}
