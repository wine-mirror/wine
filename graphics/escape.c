/*
 * Escape() function.
 *
 * Copyright 1994  Bob Amstadt
 * Copyright 2001  Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "gdi.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(driver);


/************************************************************************
 *             Escape  [GDI32.@]
 */
INT WINAPI Escape( HDC hdc, INT escape, INT in_count, LPCSTR in_data, LPVOID out_data )
{
    INT ret;
    POINT *pt;

    switch (escape)
    {
    case ABORTDOC:
        return AbortDoc( hdc );

    case ENDDOC:
        return EndDoc( hdc );

    case GETPHYSPAGESIZE:
        pt = out_data;
        pt->x = GetDeviceCaps( hdc, PHYSICALWIDTH );
        pt->y = GetDeviceCaps( hdc, PHYSICALHEIGHT );
        return 1;

    case GETPRINTINGOFFSET:
        pt = out_data;
        pt->x = GetDeviceCaps( hdc, PHYSICALOFFSETX );
        pt->y = GetDeviceCaps( hdc, PHYSICALOFFSETY );
        return 1;

    case GETSCALINGFACTOR:
        pt = out_data;
        pt->x = GetDeviceCaps( hdc, SCALINGFACTORX );
        pt->y = GetDeviceCaps( hdc, SCALINGFACTORY );
        return 1;

    case NEWFRAME:
        return EndPage( hdc );

    case SETABORTPROC:
        return SetAbortProc( hdc, (ABORTPROC)in_data );

    case STARTDOC:
        {
            DOCINFOA doc;
            char *name = NULL;

            /* in_data may not be 0 terminated so we must copy it */
            if (in_data)
            {
                name = HeapAlloc( GetProcessHeap(), 0, in_count+1 );
                memcpy( name, in_data, in_count );
                name[in_count] = 0;
            }
            /* out_data is actually a pointer to the DocInfo structure and used as
             * a second input parameter */
            if (out_data) doc = *(DOCINFOA *)out_data;
            else
            {
                doc.cbSize = sizeof(doc);
                doc.lpszOutput = NULL;
                doc.lpszDatatype = NULL;
                doc.fwType = 0;
            }
            doc.lpszDocName = name;
            ret = StartDocA( hdc, &doc );
            if (name) HeapFree( GetProcessHeap(), 0, name );
            if (ret > 0) ret = StartPage( hdc );
            return ret;
        }

    case QUERYESCSUPPORT:
        {
            INT *ptr = (INT *)in_data;
            if (in_count < sizeof(INT)) return 0;
            switch(*ptr)
            {
            case ABORTDOC:
            case ENDDOC:
            case GETPHYSPAGESIZE:
            case GETPRINTINGOFFSET:
            case GETSCALINGFACTOR:
            case NEWFRAME:
            case QUERYESCSUPPORT:
            case SETABORTPROC:
            case STARTDOC:
                return TRUE;
            }
            break;
        }
    }

    /* if not handled internally, pass it to the driver */
    return ExtEscape( hdc, escape, in_count, in_data, 0, out_data );
}


/******************************************************************************
 *		ExtEscape	[GDI32.@]
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
INT WINAPI ExtEscape( HDC hdc, INT nEscape, INT cbInput, LPCSTR lpszInData,
                      INT cbOutput, LPSTR lpszOutData )
{
    INT ret = 0;
    DC * dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        if (dc->funcs->pExtEscape)
            ret = dc->funcs->pExtEscape( dc->physDev, nEscape, cbInput, lpszInData, cbOutput, lpszOutData );
        GDI_ReleaseObj( hdc );
    }
    return ret;
}


/*******************************************************************
 *      DrawEscape [GDI32.@]
 *
 *
 */
INT WINAPI DrawEscape(HDC hdc, INT nEscape, INT cbInput, LPCSTR lpszInData)
{
    FIXME("DrawEscape, stub\n");
    return 0;
}
