/*
 * COMMDLG - Color Dialog
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
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

/* BUGS : still seems to not refresh correctly
   sometimes, especially when 2 instances of the
   dialog are loaded at the same time */

#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "commdlg.h"
#include "wine/debug.h"
#include "cdlg16.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);


/***********************************************************************
 *           ColorDlgProc   (COMMDLG.8)
 */
BOOL16 CALLBACK ColorDlgProc16( HWND16 hDlg16, UINT16 message, WPARAM16 wParam, LPARAM lParam )
{
    FIXME( "%04x %04x %04x %08Ix: stub\n", hDlg16, message, wParam, lParam );
    return FALSE;
}

/***********************************************************************
 *            ChooseColor  (COMMDLG.5)
 */
BOOL16 WINAPI ChooseColor16( LPCHOOSECOLOR16 cc16 )
{
    CHOOSECOLORA cc32;
    BOOL ret;

    cc32.lStructSize  = sizeof(cc32);
    cc32.hwndOwner    = HWND_32( cc16->hwndOwner );
    cc32.rgbResult    = cc16->rgbResult;
    cc32.lpCustColors = MapSL( cc16->lpCustColors );
    cc32.Flags        = cc16->Flags & ~(CC_ENABLETEMPLATE | CC_ENABLETEMPLATEHANDLE | CC_ENABLEHOOK);
    cc32.lCustData    = cc16->lCustData;
    cc32.hInstance    = NULL;
    cc32.lpfnHook     = NULL;
    cc32.lpTemplateName = NULL;

    if (cc16->Flags & (CC_ENABLETEMPLATE | CC_ENABLETEMPLATEHANDLE))
        FIXME( "custom templates no longer supported, using default\n" );
    if (cc16->Flags & CC_ENABLEHOOK)
        FIXME( "custom hook %p no longer supported\n", cc16->lpfnHook );

    if ((ret = ChooseColorA( &cc32 )))
    {
        cc16->rgbResult = cc32.rgbResult;
    }
    return ret;
}
