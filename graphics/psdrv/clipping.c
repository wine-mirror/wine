/*
 *	PostScript clipping functions
 *
 *	Copyright 1999  Luc Tourangau 
 *
 */

#include "gdi.h"
#include "psdrv.h"
#include "region.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(psdrv)

/***********************************************************************
 *           PSDRV_SetDeviceClipping
 */
VOID PSDRV_SetDeviceClipping( DC *dc )
{
    CHAR szArrayName[] = "clippath";
    RGNOBJ *obj = (RGNOBJ *)GDI_GetObjPtr(dc->w.hGCClipRgn, REGION_MAGIC);
    if (!obj)
    {
        ERR_(psdrv)("Rgn is 0. Please report this.\n");
	return;
    }

    TRACE_(psdrv)("dc=%p", dc);
    
    if (obj->rgn->numRects > 0)
    {
        INT i;
        RECT *pRect = obj->rgn->rects;

        PSDRV_WriteArrayDef(dc, szArrayName, obj->rgn->numRects * 4);

        for (i = 0; i < obj->rgn->numRects; i++, pRect++)
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
    }
    
    PSDRV_WriteRectClip(dc, szArrayName);
}
