/*
 *	PostScript driver Escape function
 *
 *	Copyright 1998  Huw D M Davies
 */
#include "wine/winuser16.h"
#include "psdrv.h"
#include "debug.h"
#include "winspool.h"


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
	    TRACE(psdrv, "NEXTBAND returning %d,%d - %d,%d\n", r->left,
		  r->top, r->right, r->bottom );
	    return 1;
	}
        SetRect16( r, 0, 0, 0, 0 );
	TRACE(psdrv, "NEXTBAND rect to 0,0 - 0,0\n" );
	physDev->job.banding = FALSE;
    }	/* Fall through */

    case NEWFRAME:
        TRACE(psdrv, "NEWFRAME\n");

        if(!physDev->job.hJob) {
	    FIXME(psdrv, "hJob == 0. Now what?\n");
	    return 0;
	}

	if(!PSDRV_WriteEndPage( dc ))
	    return 0;

	physDev->job.NeedPageHeader = TRUE;
	return 1;
          
    case QUERYESCSUPPORT:
        if(cbInput != 2) {
	    WARN(psdrv, "cbInput != 2 (=%d) for QUERYESCSUPPORT\n", cbInput);
	    return 0;
	} else {
	    UINT16 num = *(UINT16 *)PTR_SEG_TO_LIN(lpInData);
	    TRACE(psdrv, "QUERYESCSUPPORT for %d\n", num);	   

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
        FIXME(psdrv, "SETABORTPROC: Ignoring\n");

/*	dc->w.lpfnPrint = (FARPROC16)lpInData;
 */
	return 1;

    case STARTDOC:
        TRACE(psdrv, "STARTDOC\n");
        if(physDev->job.hJob) {
	    FIXME(psdrv, "hJob != 0. Now what?\n");
	    return 0;
	}

	physDev->job.hJob = OpenJob16(physDev->job.output,
				    PTR_SEG_TO_LIN(lpInData), dc->hSelf);
	if(!physDev->job.hJob) {
	    WARN(psdrv, "OpenJob failed\n");
	    return 0;
	}
	physDev->job.banding = FALSE;
	physDev->job.NeedPageHeader = FALSE;
	physDev->job.PageNo = 1;
	if(!PSDRV_WriteHeader( dc, PTR_SEG_TO_LIN(lpInData), cbInput ))
	    return 0;

	if(!PSDRV_WriteNewPage( dc ))
	    return 0;
	return 1;

    case ENDDOC:
        TRACE(psdrv, "ENDDOC\n");
        if(!physDev->job.hJob) {
	    FIXME(psdrv, "hJob == 0. Now what?\n");
	    return 0;
	}

	physDev->job.NeedPageHeader = FALSE;

	if(!PSDRV_WriteFooter( dc ))
	    return 0;

        if( CloseJob16( physDev->job.hJob ) == SP_ERROR ) {
	    WARN(psdrv, "CloseJob error\n");
	    return 0;
	}
	physDev->job.hJob = 0;
	return 1;

    case GETPHYSPAGESIZE:
        {
	    POINT16 *p  = (POINT16 *)PTR_SEG_TO_LIN(lpOutData);
	    
	    p->x = dc->w.devCaps->horzRes;
	    p->y = dc->w.devCaps->vertRes;
	    TRACE(psdrv, "GETPHYSPAGESIZE: returning %dx%d\n", p->x, p->y);
	    return 1;
	}

    case GETPRINTINGOFFSET:
        {
	    POINT16 *p = (POINT16 *)PTR_SEG_TO_LIN(lpOutData);
        
	    p->x = p->y = 0;
	    TRACE(psdrv, "GETPRINTINGOFFSET: returning %dx%d\n", p->x, p->y);
	    return 1;
	}
      
    case GETSCALINGFACTOR:
        {
	    POINT16 *p = (POINT16 *)PTR_SEG_TO_LIN(lpOutData);
        
	    p->x = p->y = 0;
	    TRACE(psdrv, "GETSCALINGFACTOR: returning %dx%d\n", p->x, p->y);
	    return 1;
	}

    case SETCOPYCOUNT:
        {
	    INT16 *NumCopies = (INT16 *)PTR_SEG_TO_LIN(lpInData);
	    INT16 *ActualCopies = (INT16 *)PTR_SEG_TO_LIN(lpOutData);
	    if(cbInput != 2) {
	        WARN(psdrv, "cbInput != 2 (=%d) for SETCOPYCOUNT\n", cbInput);
		return 0;
	    }
	    TRACE(psdrv, "SETCOPYCOUNT %d\n", *NumCopies);
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
	        WARN(psdrv, "cbInput != 2 (=%d) for SETLINECAP\n", cbInput);
		return 0;
	    }	        	
	    TRACE(psdrv, "SETLINECAP %d\n", newCap);
	    return 0;
	}
	    
    case SETLINEJOIN:
        {
	    INT16 newJoin = *(INT16 *)PTR_SEG_TO_LIN(lpInData);
	    if(cbInput != 2) {
	        WARN(psdrv, "cbInput != 2 (=%d) for SETLINEJOIN\n", cbInput);
		return 0;
	    }	        
	    TRACE(psdrv, "SETLINEJOIN %d\n", newJoin);
	    return 0;
	}

    case SETMITERLIMIT:
        {
	    INT16 newLimit = *(INT16 *)PTR_SEG_TO_LIN(lpInData);
	    if(cbInput != 2) {
	        WARN(psdrv, "cbInput != 2 (=%d) for SETMITERLIMIT\n", cbInput);
		return 0;
	    }	        
	    TRACE(psdrv, "SETMITERLIMIT %d\n", newLimit);
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
	        WARN(psdrv, "cbInput != 2 (=%d) for EXT_DEVICE_CAPS\n",
		     cbInput);
		return 0;
	    }	        
	    TRACE(psdrv, "EXT_DEVICE_CAPS %d\n", cap);
	    return 0;
	}

    case SET_BOUNDS:
        {
	    RECT16 *r = (RECT16 *)PTR_SEG_TO_LIN(lpInData);
	    if(cbInput != 8) {
	        WARN(psdrv, "cbInput != 8 (=%d) for SET_BOUNDS\n", cbInput);
		return 0;
	    }	        
	    TRACE(psdrv, "SET_BOUNDS (%d,%d) - (%d,%d)\n", r->left, r->top,
		  r->right, r->bottom);
	    return 0;
	}

    default:
        FIXME(psdrv, "Unimplemented code 0x%x\n", nEscape);
	return 0;
    }
}
