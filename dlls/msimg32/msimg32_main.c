/*
 * Copyright 2002 Uwe Bonnes
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

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msimg32);

/******************************************************************************
 *           AlphaBlend   (MSIMG32.@)
 */
BOOL WINAPI AlphaBlend( HDC hdcDest, int xDest, int yDest, int widthDest, int heightDst,
                        HDC hdcSrc, int xSrc, int ySrc, int widthSrc, int heightSrc,
                        BLENDFUNCTION func )
{
    FIXME("stub: AlphaBlend from %p to %p\n", hdcSrc, hdcDest );
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/******************************************************************************
 *           TransparentBlt   (MSIMG32.@)
 */
BOOL WINAPI TransparentBlt( HDC hdcDest, int xDest, int yDest, int widthDest, int heightDst,
                            HDC hdcSrc, int xSrc, int ySrc, int widthSrc, int heightSrc,
                            UINT crTransparent )
{
    FIXME("stub: TransparentBlt from %p to %p\n", hdcSrc, hdcDest );
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/******************************************************************************
 *           vSetDdrawflag   (MSIMG32.@)
 */
void WINAPI vSetDdrawflag(void)
{
    static unsigned int vDrawflag=1;
    FIXME("stub: vSetDrawFlag %u\n", vDrawflag);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}
