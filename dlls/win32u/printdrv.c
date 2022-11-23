/*
 * Implementation of some printer driver bits
 *
 * Copyright 1996 John Harvey
 * Copyright 1998 Huw Davies
 * Copyright 1998 Andreas Mohr
 * Copyright 1999 Klaas van Gend
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#if 0
#pragma makedep unix
#endif

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "winspool.h"
#include "winerror.h"
#include "wine/debug.h"
#include "ntgdi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(print);

/******************************************************************
 *           NtGdiGetSpoolMessage    (win32u.@)
 */
DWORD WINAPI NtGdiGetSpoolMessage( void *ptr1, DWORD data2, void *ptr3, DWORD data4 )
{
    LARGE_INTEGER time;

    TRACE( "(%p 0x%x %p 0x%x) stub\n", ptr1, (int)data2, ptr3, (int)data4 );

    /* avoid 100% cpu usage with spoolsv.exe from w2k
      (spoolsv.exe from xp does Sleep 1000/1500/2000 in a loop) */
    time.QuadPart = 500 * -10000;
    NtDelayExecution( FALSE, &time );
    return 0;
}

/******************************************************************
 *           NtGdiInitSpool    (win32u.@)
 */
DWORD WINAPI NtGdiInitSpool(void)
{
    FIXME("stub\n");
    return TRUE;
}

/******************************************************************
 *           NtGdiStartDoc    (win32u.@)
 */
INT WINAPI NtGdiStartDoc( HDC hdc, const DOCINFOW *doc, BOOL *banding, INT job )
{
    INT ret = SP_ERROR;
    DC *dc = get_dc_ptr( hdc );

    TRACE("DocName %s, Output %s, Datatype %s, fwType %#x\n",
          debugstr_w(doc->lpszDocName), debugstr_w(doc->lpszOutput),
          debugstr_w(doc->lpszDatatype), (int)doc->fwType);

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pStartDoc );
        ret = physdev->funcs->pStartDoc( physdev, doc );
        release_dc_ptr( dc );
    }
    return ret;
}


/******************************************************************
 *           NtGdiEndDoc    (win32u.@)
 */
INT WINAPI NtGdiEndDoc( HDC hdc )
{
    INT ret = SP_ERROR;
    DC *dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pEndDoc );
        ret = physdev->funcs->pEndDoc( physdev );
        release_dc_ptr( dc );
    }
    return ret;
}


/******************************************************************
 *           NtGdiStartPage    (win32u.@)
 */
INT WINAPI NtGdiStartPage( HDC hdc )
{
    INT ret = SP_ERROR;
    DC *dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pStartPage );
        ret = physdev->funcs->pStartPage( physdev );
        release_dc_ptr( dc );
    }
    return ret;
}


/******************************************************************
 *           NtGdiEndPage    (win32u.@)
 */
INT WINAPI NtGdiEndPage( HDC hdc )
{
    INT ret = SP_ERROR;
    DC *dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pEndPage );
        ret = physdev->funcs->pEndPage( physdev );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           NtGdiAbortDoc    (win32u.@)
 */
INT WINAPI NtGdiAbortDoc( HDC hdc )
{
    INT ret = SP_ERROR;
    DC *dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pAbortDoc );
        ret = physdev->funcs->pAbortDoc( physdev );
        release_dc_ptr( dc );
    }
    return ret;
}
