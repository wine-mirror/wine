/*
 *	PostScript driver Escape function
 *
 *	Copyright 1998  Huw D M Davies
 */
#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "psdrv.h"
#include "debugtools.h"
#include "winspool.h"

DEFAULT_DEBUG_CHANNEL(psdrv);


INT PSDRV_Escape( DC *dc, INT nEscape, INT cbInput, 
		  SEGPTR lpInData, SEGPTR lpOutData )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;

    switch(nEscape) {

    case NEXTBAND: {
        RECT16 *r = MapSL(lpOutData);
	if(!physDev->job.banding) {
	    physDev->job.banding = TRUE;
            r->left   = 0;
            r->top    = 0;
            r->right  = physDev->horzRes;
            r->bottom = physDev->vertRes;
	    TRACE("NEXTBAND returning %d,%d - %d,%d\n", r->left,
		  r->top, r->right, r->bottom );
	    return 1;
	}
        r->left   = 0;
        r->top    = 0;
        r->right  = 0;
        r->bottom = 0;
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
	    UINT16 num = *(UINT16 *)MapSL(lpInData);
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
            case EPSPRINTING:
            case PASSTHROUGH:
            case POSTSCRIPT_PASSTHROUGH:
	        return TRUE;

	    default:
	        return FALSE;
	    }
	}

    case SETABORTPROC:
        TRACE("SETABORTPROC\n");
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
	    memcpy(name, MapSL(lpInData), cbInput);
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
	    PSDRV_PDEVICE   *pdev = (PSDRV_PDEVICE *)(dc->physDev);
	    POINT16 	    *p = MapSL(lpOutData);
	    
	    p->x = p->y = 0;
	    
	    if ((pdev->Devmode->dmPublic.dmFields & DM_PAPERSIZE) != 0 &&
	    	    pdev->Devmode->dmPublic.u1.s1.dmPaperSize != 0)
	    {
	    	PAGESIZE    *page = pdev->pi->ppd->PageSizes;
		
		while (page != NULL)
		{
		    if (page->WinPage ==
		    	    pdev->Devmode->dmPublic.u1.s1.dmPaperSize)
		    	break;
		    page = page->next;
		}
		
		if (page == NULL)
		{
		    ERR("No entry for papersize %u in PPD file for '%s'\n",
		    	    pdev->Devmode->dmPublic.u1.s1.dmPaperSize,
			    pdev->pi->FriendlyName);
		    return 0;
		}
		
		TRACE("Found '%s' for paper size %u\n", page->FullName,
		    	pdev->Devmode->dmPublic.u1.s1.dmPaperSize);
		
		p->x = page->PaperDimension->x * physDev->logPixelsX / 72;
		p->y = page->PaperDimension->y * physDev->logPixelsY / 72;
		
		TRACE("%fx%f PostScript points = %ix%i device units\n", 
		    	page->PaperDimension->x, page->PaperDimension->y,
			p->x, p->y);
	    }
	    
	    /* These are in tenths of a millimeter */
	    
	    if ((pdev->Devmode->dmPublic.dmFields & DM_PAPERWIDTH) != 0 &&
	    	    pdev->Devmode->dmPublic.u1.s1.dmPaperWidth != 0)
	    {
		p->x = (pdev->Devmode->dmPublic.u1.s1.dmPaperWidth *
		    	physDev->logPixelsX) / 254;
		TRACE("dmPaperWidth = %i device units\n", p->x);
	    }
	    
	    if ((pdev->Devmode->dmPublic.dmFields & DM_PAPERLENGTH) != 0 &&
	    	    pdev->Devmode->dmPublic.u1.s1.dmPaperLength != 0)
	    {
	    	p->y = (pdev->Devmode->dmPublic.u1.s1.dmPaperLength *
		    	physDev->logPixelsY) / 254;
		TRACE("dmPaperLength = %i device units\n", p->y);
	    }
			
	    if (p->x == 0 || p->y == 0)
	    {
	    	ERR("Paper size not properly set for '%s'\n",
		    	pdev->pi->FriendlyName);
		return 0;
	    }
	    
	    if ((pdev->Devmode->dmPublic.dmFields & DM_ORIENTATION) != 0 &&
	    	    pdev->Devmode->dmPublic.u1.s1.dmOrientation ==
		    DMORIENT_LANDSCAPE)
	    {
	    	register INT16	temp = p->y;
		p->y = p->x;
		p->x = temp;
	    }
	    
	    return 1;
	}

    case GETPRINTINGOFFSET:
        {
	    POINT16 *p = MapSL(lpOutData);
        
	    p->x = p->y = 0;
	    TRACE("GETPRINTINGOFFSET: returning %dx%d\n", p->x, p->y);
	    return 1;
	}
      
    case GETSCALINGFACTOR:
        {
	    POINT16 *p = MapSL(lpOutData);
        
	    p->x = p->y = 0;
	    TRACE("GETSCALINGFACTOR: returning %dx%d\n", p->x, p->y);
	    return 1;
	}

    case SETCOPYCOUNT:
        {
	    INT16 *NumCopies = MapSL(lpInData);
	    INT16 *ActualCopies = MapSL(lpOutData);
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
	    LPSTR p = MapSL(lpOutData);
	    strcpy(p, "PostScript");
	    *(p + strlen(p) + 1) = '\0'; /* 2 '\0's at end of string */
	    return 1;
	}

    case SETLINECAP:
        {
	    INT16 newCap = *(INT16 *)MapSL(lpInData);
	    if(cbInput != 2) {
	        WARN("cbInput != 2 (=%d) for SETLINECAP\n", cbInput);
		return 0;
	    }	        	
	    TRACE("SETLINECAP %d\n", newCap);
	    return 0;
	}
	    
    case SETLINEJOIN:
        {
	    INT16 newJoin = *(INT16 *)MapSL(lpInData);
	    if(cbInput != 2) {
	        WARN("cbInput != 2 (=%d) for SETLINEJOIN\n", cbInput);
		return 0;
	    }	        
	    TRACE("SETLINEJOIN %d\n", newJoin);
	    return 0;
	}

    case SETMITERLIMIT:
        {
	    INT16 newLimit = *(INT16 *)MapSL(lpInData);
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
	    UINT16 cap = *(UINT16 *)MapSL(lpInData);
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
	    RECT16 *r = MapSL(lpInData);
	    if(cbInput != 8) {
	        WARN("cbInput != 8 (=%d) for SET_BOUNDS\n", cbInput);
		return 0;
	    }	        
	    TRACE("SET_BOUNDS (%d,%d) - (%d,%d)\n", r->left, r->top,
		  r->right, r->bottom);
	    return 0;
	}

    case EPSPRINTING:
	{
	    UINT16	epsprint = *(UINT16*)MapSL(lpInData);
	    /* FIXME: In this mode we do not need to send page intros and page
	     * ends according to the doc. But I just ignore that detail
	     * for now.
	     */
	    TRACE("EPS Printing support %sable.\n",epsprint?"en":"dis");
	    return 1;
        }
    case PASSTHROUGH:
    case POSTSCRIPT_PASSTHROUGH:
        {
            /* Write directly to spool file, bypassing normal PS driver
             * processing that is done along with writing PostScript code
             * to the spool.
	     * (Usually we have a WORD before the data counting the size, but
	     * cbInput is just this +2.)
             */
            return WriteSpool16(physDev->job.hJob,((char*)lpInData)+2,cbInput-2);
        }

    case GETSETPRINTORIENT:
	{
	    /* If lpInData is present, it is a 20 byte structure, first 32
	     * bit LONG value is the orientation. if lpInData is NULL, it
	     * returns the current orientation.
	     */
	    FIXME("GETSETPRINTORIENT not implemented (lpInData %ld)!\n",lpInData);
	    return 1;
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
	physDev->job.output = HeapAlloc( PSDRV_Heap, 0, strlen(doc->lpszOutput)+1 );
	strcpy( physDev->job.output, doc->lpszOutput );
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
