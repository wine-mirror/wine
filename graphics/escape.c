/*
 * Escape() function.
 *
 * Copyright 1994  Bob Amstadt
 */

#include <string.h>
#include "wingdi.h"
#include "gdi.h"
#include "heap.h"
#include "ldt.h"
#include "dc.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(driver)

/***********************************************************************
 *            Escape16  [GDI.38]
 */
INT16 WINAPI Escape16( HDC16 hdc, INT16 nEscape, INT16 cbInput,
                       SEGPTR lpszInData, SEGPTR lpvOutData )
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc || !dc->funcs->pEscape) return 0;
    if(nEscape == SETABORTPROC) SetAbortProc16(hdc, lpszInData);
    return dc->funcs->pEscape( dc, nEscape, cbInput, lpszInData, lpvOutData );
}

/************************************************************************
 *             Escape  [GDI32.200]
 */
INT WINAPI Escape( HDC hdc, INT nEscape, INT cbInput,
		   LPCSTR lpszInData, LPVOID lpvOutData )
{
    SEGPTR	segin,segout;
    INT	ret;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc || !dc->funcs->pEscape) return 0;

    segin	= (SEGPTR)lpszInData;
    segout	= (SEGPTR)lpvOutData;
    switch (nEscape) {
    	/* Escape(hdc,QUERYESCSUPPORT,LPINT,NULL) */
        /* Escape(hdc,SETLINECAP,LPINT,NULL) */
    case QUERYESCSUPPORT:
    case SETLINECAP:
    case SETLINEJOIN:
      {
    	LPINT16 x = (LPINT16)SEGPTR_NEW(INT16);
	*x = *(INT*)lpszInData;
	segin = SEGPTR_GET(x);
 	cbInput = sizeof(INT16);
	break;
      }

    	/* Escape(hdc,GETSCALINGFACTOR,NULL,LPPOINT32) */
    	/* Escape(hdc,GETPHYSPAGESIZE,NULL,LPPOINT32) */
    	/* Escape(hdc,GETPRINTINGOFFSET,NULL,LPPOINT32) */

    case GETSCALINGFACTOR:
    case GETPHYSPAGESIZE:
    case GETPRINTINGOFFSET:
	segout = SEGPTR_GET(SEGPTR_NEW(POINT16));
	cbInput = sizeof(POINT16);
	break;

        /* Escape(hdc,EXT_DEVICE_CAPS,LPINT,LPDWORD) */
    case EXT_DEVICE_CAPS:
      {
    	LPINT16 lpIndex = (LPINT16)SEGPTR_NEW(INT16);
	LPDWORD lpCaps = (LPDWORD)SEGPTR_NEW(DWORD);
	*lpIndex = *(INT*)lpszInData;
	
	segin = SEGPTR_GET(lpIndex);
	segout = SEGPTR_GET(lpCaps);
 	cbInput = sizeof(INT16);
	break;
      }

      /* Escape(hdc,GETTECHNOLOGY,NULL,LPSTR); */
    case GETTECHNOLOGY: {
        segout = SEGPTR_GET(SEGPTR_ALLOC(200)); /* enough I hope */
        break;

    }

      /* Escape(hdc,ENABLEPAIRKERNING,LPINT16,LPINT16); */

    case ENABLEPAIRKERNING: {
        LPINT16 enab = SEGPTR_NEW(INT16);
        segout = SEGPTR_GET(SEGPTR_NEW(INT16));
        segin = SEGPTR_GET(enab);
        *enab = *(INT*)lpszInData;
	cbInput = sizeof(INT16);
        break;
    }

      /* Escape(hdc,GETFACENAME,NULL,LPSTR); */

    case GETFACENAME: {
        segout = SEGPTR_GET(SEGPTR_ALLOC(200));
        break;
    }

      /* Escape(hdc,STARTDOC,LPSTR,NULL); */

    case STARTDOC: /* string may not be \0 terminated */
        if(lpszInData) {
	    char *cp = SEGPTR_ALLOC(cbInput);
	    memcpy(cp, lpszInData, cbInput);
	    segin = SEGPTR_GET(cp);
	} else
	    segin = 0;
	break;

    case SETABORTPROC:
        SetAbortProc(hdc, (ABORTPROC)lpszInData);
	break;

    default:
        break;

    }

    ret = dc->funcs->pEscape( dc, nEscape, cbInput, segin, segout );

    switch(nEscape) {
    case QUERYESCSUPPORT:
    	if (ret)
		TRACE("target DC implements Escape %d\n",nEscape);
    	SEGPTR_FREE(PTR_SEG_TO_LIN(segin));
	break;
    case SETLINECAP:
    case SETLINEJOIN:
        SEGPTR_FREE(PTR_SEG_TO_LIN(segin));
	break;
    case GETSCALINGFACTOR:
    case GETPRINTINGOFFSET:
    case GETPHYSPAGESIZE: {
    	LPPOINT16 x = (LPPOINT16)PTR_SEG_TO_LIN(segout);
	CONV_POINT16TO32(x,(LPPOINT)lpvOutData);
	SEGPTR_FREE(x);
	break;
    }
    case EXT_DEVICE_CAPS:
        *(LPDWORD)lpvOutData = *(LPDWORD)PTR_SEG_TO_LIN(segout);
        SEGPTR_FREE(PTR_SEG_TO_LIN(segin));
        SEGPTR_FREE(PTR_SEG_TO_LIN(segout));
	break;

    case GETTECHNOLOGY: {
        LPSTR x=PTR_SEG_TO_LIN(segout);
        lstrcpyA(lpvOutData,x);
        SEGPTR_FREE(x);
	break;
    }
    case ENABLEPAIRKERNING: {
        LPINT16 enab = (LPINT16)PTR_SEG_TO_LIN(segout);

        *(LPINT)lpvOutData = *enab;
        SEGPTR_FREE(enab);
        SEGPTR_FREE(PTR_SEG_TO_LIN(segin));
	break;
    }
    case GETFACENAME: {
        LPSTR x = (LPSTR)PTR_SEG_TO_LIN(segout);
        lstrcpyA(lpvOutData,x);
        SEGPTR_FREE(x);
        break;
    }
    case STARTDOC:
        SEGPTR_FREE(PTR_SEG_TO_LIN(segin));
	break;

    default:
    	break;
    }
    return ret;
}

/******************************************************************************
 *		ExtEscape	[GDI32.95]
 *
 * PARAMS
 *    hdc         [I] Handle to device context
 *    nEscape     [I] Escape function
 *    cbInput     [I] Number of bytes in input structure
 *    lpszInData  [I] Pointer to input structure
 *    cbOutput    [I] Number of bytes in output structure
 *    lpszOutData [O] Pointer to output structure
 *
 * RETURNS
 *    Success: >0
 *    Not implemented: 0
 *    Failure: <0
 */
INT WINAPI ExtEscape( HDC hdc, INT nEscape, INT cbInput, 
		      LPCSTR lpszInData, INT cbOutput, LPSTR lpszOutData )
{
    char *inBuf, *outBuf;
    INT ret;

    inBuf = SEGPTR_ALLOC(cbInput);
    memcpy(inBuf, lpszInData, cbInput);
    outBuf = cbOutput ? SEGPTR_ALLOC(cbOutput) : NULL;
    ret = Escape16( hdc, nEscape, cbInput, SEGPTR_GET(inBuf),
		    SEGPTR_GET(outBuf) );
    SEGPTR_FREE(inBuf);
    if(outBuf) {
	memcpy(lpszOutData, outBuf, cbOutput);
	SEGPTR_FREE(outBuf);
    }
    return ret;
}

/*******************************************************************
 *      DrawEscape [GDI32.74]
 *
 *
 */
INT WINAPI DrawEscape(HDC hdc, INT nEscape, INT cbInput, LPCSTR lpszInData)
{
    FIXME("DrawEscape, stub\n");
    return 0;
}
