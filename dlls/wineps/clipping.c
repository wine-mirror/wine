/*
 *	PostScript clipping functions
 *
 *	Copyright 1999  Luc Tourangau 
 *
 */

#include "gdi.h"
#include "psdrv.h"
#include "debugtools.h"
#include "winbase.h"

DEFAULT_DEBUG_CHANNEL(psdrv)

/***********************************************************************
 *           PSDRV_SetDeviceClipping
 */
VOID PSDRV_SetDeviceClipping( DC *dc )
{
    CHAR szArrayName[] = "clippath";
    DWORD size;
    RGNDATA *rgndata;

    TRACE("hdc=%04x\n", dc->hSelf);

    if (dc->w.hGCClipRgn == 0) {
        ERR("Rgn is 0. Please report this.\n");
	return;
    }

    size = GetRegionData(dc->w.hGCClipRgn, 0, NULL);
    if(!size) {
        ERR("Invalid region\n");
	return;
    }

    rgndata = HeapAlloc( GetProcessHeap(), 0, size );
    if(!rgndata) {
        ERR("Can't allocate buffer\n");
	return;
    }

    GetRegionData(dc->w.hGCClipRgn, size, rgndata);

    PSDRV_WriteInitClip(dc);

    /* check for NULL region */
    if (rgndata->rdh.nCount == 0)
    {
        /* set an empty clip path. */
        PSDRV_WriteRectClip(dc, 0, 0, 0, 0);
    }
    /* optimize when it is a simple region */
    else if (rgndata->rdh.nCount == 1)
    {
        RECT *pRect = (RECT *)rgndata->Buffer;

        PSDRV_WriteRectClip(dc, pRect->left, pRect->top, 
                            pRect->right - pRect->left, 
                            pRect->bottom - pRect->top);        
    }
    else
    {
        INT i;
        RECT *pRect = (RECT *)rgndata->Buffer;

        PSDRV_WriteArrayDef(dc, szArrayName, rgndata->rdh.nCount * 4);

        for (i = 0; i < rgndata->rdh.nCount; i++, pRect++)
        {
            PSDRV_WriteArrayPut(dc, szArrayName, i * 4,
                                pRect->left);
            PSDRV_WriteArrayPut(dc, szArrayName, i * 4 + 1,
                                pRect->top);
            PSDRV_WriteArrayPut(dc, szArrayName, i * 4 + 2, 
                                pRect->right - pRect->left);
            PSDRV_WriteArrayPut(dc, szArrayName, i * 4 + 3, 
                                pRect->bottom - pRect->top);
        }

        PSDRV_WriteRectClip2(dc, szArrayName);
    }
    
    HeapFree( GetProcessHeap(), 0, rgndata );
    return;
}
