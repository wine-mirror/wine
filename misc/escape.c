/*
 * Escape() function.
 *
 * Copyright 1994  Bob Amstadt
 */

#include <stdio.h>
#include "windows.h"
#include "gdi.h"
#include "dc.h"

INT16 Escape16( HDC16 hdc, INT16 nEscape, INT16 cbInput,
                SEGPTR lpszInData, SEGPTR lpvOutData )
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc || !dc->funcs->pEscape) return 0;
    return dc->funcs->pEscape( dc, nEscape, cbInput, lpszInData, lpvOutData );
}

INT32 Escape32( HDC32 hdc, INT32 nEscape, INT32 cbInput,
                LPVOID lpszInData, LPVOID lpvOutData )
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc || !dc->funcs->pEscape) return 0;
    return dc->funcs->pEscape( dc, nEscape, cbInput,
                               (SEGPTR)lpszInData, (SEGPTR)lpvOutData );
}
