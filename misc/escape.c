/*
 * Escape() function.
 *
 * Copyright 1994  Bob Amstadt
 */

#include <stdlib.h>
#include <stdio.h>
#include "windows.h"
#include "gdi.h"

INT Escape( HDC hdc, INT nEscape, INT cbInput,
            SEGPTR lpszInData, SEGPTR lpvOutData )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0;
    }

    if (!dc->funcs->pEscape) return 0;
    return dc->funcs->pEscape( dc, nEscape, cbInput, lpszInData, lpvOutData );
}
