/*
 * Escape() function.
 *
 * Copyright 1994  Bob Amstadt
 */

#include <stdio.h>
#include "windows.h"
#include "gdi.h"
#include "heap.h"
#include "ldt.h"
#include "dc.h"

INT16 WINAPI Escape16( HDC16 hdc, INT16 nEscape, INT16 cbInput,
                       SEGPTR lpszInData, SEGPTR lpvOutData )
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc || !dc->funcs->pEscape) return 0;
    return dc->funcs->pEscape( dc, nEscape, cbInput, lpszInData, lpvOutData );
}

INT32 WINAPI Escape32( HDC32 hdc, INT32 nEscape, INT32 cbInput,
                       LPVOID lpszInData, LPVOID lpvOutData )
{
    DC		*dc = DC_GetDCPtr( hdc );
    SEGPTR	segin,segout;
    INT32	ret;

    if (!dc || !dc->funcs->pEscape) return 0;
    segin	= (SEGPTR)lpszInData;
    segout	= (SEGPTR)lpvOutData;
    switch (nEscape) {
    	/* Escape(hdc,QUERYESCSUPPORT,LPINT32,NULL) */
    case QUERYESCSUPPORT: {
    	LPINT16 x = (LPINT16)SEGPTR_NEW(INT16);
	*x = *(INT32*)lpszInData;
	segin = SEGPTR_GET(x);
	break;
    }

    	/* Escape(hdc,GETSCALINGFACTOR,NULL,LPPOINT32) */
    	/* Escape(hdc,GETPHYSPAGESIZE,NULL,LPPOINT32) */
    	/* Escape(hdc,GETPRINTINGOFFSET,NULL,LPPOINT32) */

    case GETSCALINGFACTOR:
    case GETPHYSPAGESIZE:
    case GETPRINTINGOFFSET:
	segout = SEGPTR_GET(SEGPTR_NEW(POINT16));
	break;

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
        *enab = *(INT32*)lpszInData;
        break;
    }

      /* Escape(hdc,GETFACENAME,NULL,LPSTR); */

    case GETFACENAME: {
        segout = SEGPTR_GET(SEGPTR_ALLOC(200));
        break;
    }
    }
    ret = dc->funcs->pEscape( dc, nEscape, cbInput, segin, segout );
    switch(nEscape) {
    case QUERYESCSUPPORT:
    	if (ret)
		fprintf(stderr,"target DC implements Escape %d\n",nEscape);
    	SEGPTR_FREE(PTR_SEG_TO_LIN(segin));
	break;
    case GETSCALINGFACTOR:
    case GETPRINTINGOFFSET:
    case GETPHYSPAGESIZE: {
    	LPPOINT16 x = (LPPOINT16)PTR_SEG_TO_LIN(segout);
	CONV_POINT16TO32(x,(LPPOINT32)lpvOutData);
	SEGPTR_FREE(x);
	break;
    }
    case GETTECHNOLOGY: {
        LPSTR x=PTR_SEG_TO_LIN(segout);
        lstrcpy32A(lpvOutData,x);
        SEGPTR_FREE(x);
	break;
    }
    case ENABLEPAIRKERNING: {
        LPINT16 enab = (LPINT16)PTR_SEG_TO_LIN(segout);

        *(LPINT32)lpvOutData = *enab;
        SEGPTR_FREE(enab);
        SEGPTR_FREE(PTR_SEG_TO_LIN(segin));
	break;
    }
    case GETFACENAME: {
        LPSTR x = (LPSTR)PTR_SEG_TO_LIN(segout);
        lstrcpy32A(lpvOutData,x);
        SEGPTR_FREE(x);
        break;
    }
    default:
    	break;
    }
    return ret;
}

INT32 WINAPI ExtEscape32(HDC32 hdc,INT32 nEscape,INT32 cbInput,LPCSTR x,INT32 cbOutput,LPSTR out) {
	fprintf(stderr,"ExtEscape32(0x%04x,0x%x,%d,%s,%d,%p),stub!\n",
		hdc,nEscape,cbInput,x,cbOutput,out
	);
	return 1;
}
