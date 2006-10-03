/*
 * OpenGL function forwarding to the display driver
 *
 * Copyright (c) 1999 Lionel Ulmer
 * Copyright (c) 2005 Raphael Junqueira
 * Copyright (c) 2006 Roderick Colenbrander
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "gdi.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wgl);

/***********************************************************************
 *		wglCreateContext (OPENGL32.@)
 */
HGLRC WINAPI wglCreateContext(HDC hdc)
{
    HGLRC ret = 0;
    DC * dc = DC_GetDCPtr( hdc );

    TRACE("(%p)\n",hdc);

    if (!dc) return 0;

    if (!dc->funcs->pwglCreateContext) FIXME(" :stub\n");
    else ret = dc->funcs->pwglCreateContext(dc->physDev);

    GDI_ReleaseObj( hdc );
    return ret;
}

/***********************************************************************
 *		wglMakeCurrent (OPENGL32.@)
 */
BOOL WINAPI wglMakeCurrent(HDC hdc, HGLRC hglrc)
{
    BOOL ret = FALSE;
    DC * dc = DC_GetDCPtr( hdc );

    TRACE("hdc: (%p), hglrc: (%p)\n", hdc, hglrc);

    if (!dc) return FALSE;

    if (!dc->funcs->pwglMakeCurrent) FIXME(" :stub\n");
    else ret = dc->funcs->pwglMakeCurrent(dc->physDev,hglrc);

    GDI_ReleaseObj( hdc);
    return ret;
}

/***********************************************************************
 *		wglUseFontBitmapsA (OPENGL32.@)
 */
BOOL WINAPI wglUseFontBitmapsA(HDC hdc, DWORD first, DWORD count, DWORD listBase)
{
    BOOL ret = FALSE;
    DC * dc = DC_GetDCPtr( hdc );

    TRACE("(%p, %ld, %ld, %ld)\n", hdc, first, count, listBase);

    if (!dc) return FALSE;

    if (!dc->funcs->pwglUseFontBitmapsA) FIXME(" :stub\n");
    else ret = dc->funcs->pwglUseFontBitmapsA(dc->physDev, first, count, listBase);

    GDI_ReleaseObj( hdc);
    return ret;
}

/***********************************************************************
 *		wglUseFontBitmapsW (OPENGL32.@)
 */
BOOL WINAPI wglUseFontBitmapsW(HDC hdc, DWORD first, DWORD count, DWORD listBase)
{
    BOOL ret = FALSE;
    DC * dc = DC_GetDCPtr( hdc );

    TRACE("(%p, %ld, %ld, %ld)\n", hdc, first, count, listBase);

    if (!dc) return FALSE;

    if (!dc->funcs->pwglUseFontBitmapsW) FIXME(" :stub\n");
    else ret = dc->funcs->pwglUseFontBitmapsW(dc->physDev, first, count, listBase);

    GDI_ReleaseObj( hdc);
    return ret;
}
