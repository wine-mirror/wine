/*
 *	PostScript driver Escape function
 *
 *	Copyright 1998  Huw D M Davies
 */
#include "windows.h"
#include "psdrv.h"
#include "debug.h"
#include "print.h"


INT32 PSDRV_Escape( DC *dc, INT32 nEscape, INT32 cbInput, 
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
          
    case STARTDOC:
        TRACE(psdrv, "STARTDOC\n");
        if(physDev->job.hJob) {
	    FIXME(psdrv, "hJob != 0. Now what?\n");
	    return 0;
	}

	physDev->job.hJob = OpenJob(physDev->job.output,
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

    case QUERYESCSUPPORT:
        if(cbInput != 2) {
	    WARN(psdrv, "cbInput != 2 (=%d) for QUERYESCSUPPORT\n", cbInput);
	    return 0;
	} else {
	    UINT16 num = *(UINT16 *)PTR_SEG_TO_LIN(lpInData);
	    TRACE(psdrv, "QUERYESCSUPPORT for %d\n", num);	   
	    return 0;
	}

    case SETABORTPROC:
        FIXME(psdrv, "SETABORTPROC: ignoring\n");
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
	    POINT16 *p  = (POINT16 *)PTR_SEG_TO_LIN(lpOutData);
        
	    p->x = p->y = 0;
	    TRACE(psdrv, "GETPRINTINGOFFSET: returning %dx%d\n", p->x, p->y);
	    return 1;
	}
      
    case GETSCALINGFACTOR:
        {
	    POINT16 *p  = (POINT16 *)PTR_SEG_TO_LIN(lpOutData);
        
	    p->x = p->y = 0;
	    TRACE(psdrv, "GETSCALINGFACTOR: returning %dx%d\n", p->x, p->y);
	    return 1;
	}

    case ENDDOC:
        TRACE(psdrv, "ENDDOC\n");
        if(!physDev->job.hJob) {
	    FIXME(psdrv, "hJob == 0. Now what?\n");
	    return 0;
	}

	physDev->job.NeedPageHeader = FALSE;

	if(!PSDRV_WriteFooter( dc ))
	    return 0;

        if( CloseJob( physDev->job.hJob ) == SP_ERROR ) {
	    WARN(psdrv, "CloseJob error\n");
	    return 0;
	}
	physDev->job.hJob = 0;
	return 1;

    default:
        FIXME(psdrv, "Unimplemented code 0x%x\n", nEscape);
	return 0;
    }
}


