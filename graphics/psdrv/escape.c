/*
 *	PostScript driver Escape function
 *
 *	Copyright 1998  Huw D M Davies
 */
#include "wine/winuser16.h"
#include "psdrv.h"
#include "debugtools.h"
#include "winspool.h"
#include "heap.h"

DEFAULT_DEBUG_CHANNEL(psdrv)


INT PSDRV_Escape( DC *dc, INT nEscape, INT cbInput, 
		  SEGPTR lpInData, SEGPTR lpOutData )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;

    switch(nEscape) {

    case NEXTBAND: {
        RECT16 *r = (RECT16 *)PTR_SEG_TO_LIN(lpOutData);
	if(!physDev->job.banding) {
	    physDev->job.banding = TRUE;
	    SetRect16( r, 0, 0, dc->w.devCaps->horzRes,
		                     dc->w.devCaps->vertRes );
	    TRACE("NEXTBAND returning %d,%d - %d,%d\n", r->left,
		  r->top, r->right, r->bottom );
	    return 1;
	}
        SetRect16( r, 0, 0, 0, 0 );
	TRACE("NEXTBAND rect to 0,0 - 0,0\n" );
	physDev->job.banding = FALSE;
    }	/* Fall through */

    case NEWFRAME:
        TRACE("NEWFRAME\n");

        if(!physDev->job.hJob) {
	    FIXME("hJob == 0. Now what?\n");
	    return SP_ERROR;
	}
	
	if(!PSDRV_EndPage( dc ))
	    return SP_ERROR;
	return 1;
          
    case QUERYESCSUPPORT:
        if(cbInput < 2) {
	    WARN("cbInput < 2 (=%d) for QUERYESCSUPPORT\n", cbInput);
	    return 0;
	} else {
	    UINT16 num = *(UINT16 *)PTR_SEG_TO_LIN(lpInData);
	    TRACE("QUERYESCSUPPORT for %d\n", num);	   

	    switch(num) {
	    case NEWFRAME:
	    case NEXTBAND:
	    case QUERYESCSUPPORT:
	    case SETABORTPROC:
	    case STARTDOC:
	    case ENDDOC:
	    case GETPHYSPAGESIZE:
	    case GETPRINTINGOFFSET:
	    case GETSCALINGFACTOR:
	    case SETCOPYCOUNT:
	    case GETTECHNOLOGY:
	    case SETLINECAP:
	    case SETLINEJOIN:
	    case SETMITERLIMIT:
	    case SETCHARSET:
	    case EXT_DEVICE_CAPS:
	    case SET_BOUNDS:
	        return TRUE;

	    default:
	        return FALSE;
	    }
	}

    case SETABORTPROC:
        TRACE("SETABORTPROC\n");
	dc->w.spfnPrint = (FARPROC16)lpInData;
	return 1;

    case STARTDOC:
      {
	DOCINFOA doc;
	char *name = NULL;
	INT16 ret;

        TRACE("STARTDOC\n");

	/* lpInData may not be 0 terminated so we must copy it */
	if(lpInData) {
	    name = HeapAlloc( GetProcessHeap(), 0, cbInput+1 );
	    memcpy(name, PTR_SEG_TO_LIN(lpInData), cbInput);
	    name[cbInput] = '\0';
	}
	doc.cbSize = sizeof(doc);
	doc.lpszDocName = name;
	doc.lpszOutput = doc.lpszDatatype = NULL;
	doc.fwType = 0;

	ret = PSDRV_StartDoc(dc, &doc);
	if(name) HeapFree( GetProcessHeap(), 0, name );
	if(ret <= 0) return -1;
	ret = PSDRV_StartPage(dc);
	if(ret <= 0) return -1;
	return ret;
      }

    case ENDDOC:
        TRACE("ENDDOC\n");
	return PSDRV_EndDoc( dc );

    case GETPHYSPAGESIZE:
        {
	    POINT16 *p  = (POINT16 *)PTR_SEG_TO_LIN(lpOutData);
	    
	    p->x = dc->w.devCaps->horzRes;
	    p->y = dc->w.devCaps->vertRes;
	    TRACE("GETPHYSPAGESIZE: returning %dx%d\n", p->x, p->y);
	    return 1;
	}

    case GETPRINTINGOFFSET:
        {
	    POINT16 *p = (POINT16 *)PTR_SEG_TO_LIN(lpOutData);
        
	    p->x = p->y = 0;
	    TRACE("GETPRINTINGOFFSET: returning %dx%d\n", p->x, p->y);
	    return 1;
	}
      
    case GETSCALINGFACTOR:
        {
	    POINT16 *p = (POINT16 *)PTR_SEG_TO_LIN(lpOutData);
        
	    p->x = p->y = 0;
	    TRACE("GETSCALINGFACTOR: returning %dx%d\n", p->x, p->y);
	    return 1;
	}

    case SETCOPYCOUNT:
        {
	    INT16 *NumCopies = (INT16 *)PTR_SEG_TO_LIN(lpInData);
	    INT16 *ActualCopies = (INT16 *)PTR_SEG_TO_LIN(lpOutData);
	    if(cbInput != 2) {
	        WARN("cbInput != 2 (=%d) for SETCOPYCOUNT\n", cbInput);
		return 0;
	    }
	    TRACE("SETCOPYCOUNT %d\n", *NumCopies);
	    *ActualCopies = 1;
	    return 1;
	}

    case GETTECHNOLOGY:
        {
	    LPSTR p = (LPSTR)PTR_SEG_TO_LIN(lpOutData);
	    strcpy(p, "PostScript");
	    *(p + strlen(p) + 1) = '\0'; /* 2 '\0's at end of string */
	    return 1;
	}

    case SETLINECAP:
        {
	    INT16 newCap = *(INT16 *)PTR_SEG_TO_LIN(lpInData);
	    if(cbInput != 2) {
	        WARN("cbInput != 2 (=%d) for SETLINECAP\n", cbInput);
		return 0;
	    }	        	
	    TRACE("SETLINECAP %d\n", newCap);
	    return 0;
	}
	    
    case SETLINEJOIN:
        {
	    INT16 newJoin = *(INT16 *)PTR_SEG_TO_LIN(lpInData);
	    if(cbInput != 2) {
	        WARN("cbInput != 2 (=%d) for SETLINEJOIN\n", cbInput);
		return 0;
	    }	        
	    TRACE("SETLINEJOIN %d\n", newJoin);
	    return 0;
	}

    case SETMITERLIMIT:
        {
	    INT16 newLimit = *(INT16 *)PTR_SEG_TO_LIN(lpInData);
	    if(cbInput != 2) {
	        WARN("cbInput != 2 (=%d) for SETMITERLIMIT\n", cbInput);
		return 0;
	    }	        
	    TRACE("SETMITERLIMIT %d\n", newLimit);
	    return 0;
	}

    case SETCHARSET: 
      /* Undocumented escape used by winword6.
	 Switches between ANSI and a special charset.
	 If *lpInData == 1 we require that
	 0x91 is quoteleft
	 0x92 is quoteright
	 0x93 is quotedblleft
	 0x94 is quotedblright
	 0x95 is bullet
	 0x96 is endash
	 0x97 is emdash
	 0xa0 is non break space - yeah right.
	 
	 If *lpInData == 0 we get ANSI.
	 Since there's nothing else there, let's just make these the default
	 anyway and see what happens...
      */
        return 1;

    case EXT_DEVICE_CAPS:
        {
	    UINT16 cap = *(UINT16 *)PTR_SEG_TO_LIN(lpInData);
	    if(cbInput != 2) {
	        WARN("cbInput != 2 (=%d) for EXT_DEVICE_CAPS\n",
		     cbInput);
		return 0;
	    }	        
	    TRACE("EXT_DEVICE_CAPS %d\n", cap);
	    return 0;
	}

    case SET_BOUNDS:
        {
	    RECT16 *r = (RECT16 *)PTR_SEG_TO_LIN(lpInData);
	    if(cbInput != 8) {
	        WARN("cbInput != 8 (=%d) for SET_BOUNDS\n", cbInput);
		return 0;
	    }	        
	    TRACE("SET_BOUNDS (%d,%d) - (%d,%d)\n", r->left, r->top,
		  r->right, r->bottom);
	    return 0;
	}

    default:
        FIXME("Unimplemented code 0x%x\n", nEscape);
	return 0;
    }
}

/************************************************************************
 *           PSDRV_StartPage
 */
INT PSDRV_StartPage( DC *dc )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;

    if(!physDev->job.OutOfPage) {
        FIXME("Already started a page?\n");
	return 1;
    }
    physDev->job.PageNo++;
    if(!PSDRV_WriteNewPage( dc ))
        return 0;
    physDev->job.OutOfPage = FALSE;
    return 1;
}

	
/************************************************************************
 *           PSDRV_EndPage
 */
INT PSDRV_EndPage( DC *dc )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;

    if(physDev->job.OutOfPage) {
        FIXME("Already ended a page?\n");
	return 1;
    }
    if(!PSDRV_WriteEndPage( dc ))
        return 0;
    physDev->job.OutOfPage = TRUE;
    return 1;
}


/************************************************************************
 *           PSDRV_StartDoc
 */
INT PSDRV_StartDoc( DC *dc, const DOCINFOA *doc )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;

    if(physDev->job.hJob) {
        FIXME("hJob != 0. Now what?\n");
	return 0;
    }

    if(doc->lpszOutput) {
        HeapFree( PSDRV_Heap, 0, physDev->job.output );
	physDev->job.output = HEAP_strdupA( PSDRV_Heap, 0, doc->lpszOutput );
    }
    physDev->job.hJob = OpenJob16(physDev->job.output,  doc->lpszDocName,
				  dc->hSelf);
    if(!physDev->job.hJob) {
        WARN("OpenJob failed\n");
	return 0;
    }
    physDev->job.banding = FALSE;
    physDev->job.OutOfPage = TRUE;
    physDev->job.PageNo = 0;
    if(!PSDRV_WriteHeader( dc, doc->lpszDocName ))
        return 0;

    return physDev->job.hJob;
}


/************************************************************************
 *           PSDRV_EndDoc
 */
INT PSDRV_EndDoc( DC *dc )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;

    if(!physDev->job.hJob) {
        FIXME("hJob == 0. Now what?\n");
	return 0;
    }

    if(!physDev->job.OutOfPage) {
        WARN("Somebody forgot a EndPage\n");
	PSDRV_EndPage( dc );
    }
    if(!PSDRV_WriteFooter( dc ))
        return 0;

    if( CloseJob16( physDev->job.hJob ) == SP_ERROR ) {
        WARN("CloseJob error\n");
	return 0;
    }
    physDev->job.hJob = 0;
    return 1;
}
