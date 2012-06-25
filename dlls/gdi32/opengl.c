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
#include "winternl.h"
#include "winnt.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wgl);

static const WCHAR opengl32W[] = {'o','p','e','n','g','l','3','2','.','d','l','l',0};
static HMODULE opengl32;
static INT (WINAPI *wglChoosePixelFormat)(HDC,const PIXELFORMATDESCRIPTOR *);
static INT (WINAPI *wglDescribePixelFormat)(HDC,INT,UINT,PIXELFORMATDESCRIPTOR*);
static BOOL (WINAPI *wglSetPixelFormat)(HDC,INT,const PIXELFORMATDESCRIPTOR*);
static BOOL (WINAPI *wglSwapBuffers)(HDC);

static HDC default_hdc = 0;

/* We route all wgl functions from opengl32.dll through gdi32.dll to
 * the display driver. Various wgl calls have a hDC as one of their parameters.
 * Using get_dc_ptr we get access to the functions exported by the driver.
 * Some functions don't receive a hDC. This function creates a global hdc and
 * if there's already a global hdc, it returns it.
 */
static DC* OPENGL_GetDefaultDC(void)
{
    if(!default_hdc)
        default_hdc = CreateDCA("DISPLAY", NULL, NULL, NULL);

    return get_dc_ptr(default_hdc);
}

/***********************************************************************
 *		wglCreateContext (OPENGL32.@)
 */
HGLRC WINAPI wglCreateContext(HDC hdc)
{
    HGLRC ret = 0;
    DC * dc = get_dc_ptr( hdc );

    TRACE("(%p)\n",hdc);

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pwglCreateContext );
        update_dc( dc );
        ret = physdev->funcs->pwglCreateContext( physdev );
        release_dc_ptr( dc );
    }
    return ret;
}

/***********************************************************************
 *      wglCreateContextAttribsARB
 */
static HGLRC WINAPI wglCreateContextAttribsARB(HDC hdc, HGLRC hShareContext, const int *attributeList)
{
    HGLRC ret = 0;
    DC * dc = get_dc_ptr( hdc );

    TRACE("(%p)\n",hdc);

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pwglCreateContextAttribsARB );
        update_dc( dc );
        ret = physdev->funcs->pwglCreateContextAttribsARB( physdev, hShareContext, attributeList );
        release_dc_ptr( dc );
    }
    return ret;
}

/***********************************************************************
 *		wglMakeCurrent (OPENGL32.@)
 */
BOOL WINAPI wglMakeCurrent(HDC hdc, HGLRC hglrc)
{
    BOOL ret = FALSE;
    DC * dc = NULL;

    /* When the context hglrc is NULL, the HDC is ignored and can be NULL.
     * In that case use the global hDC to get access to the driver.  */
    if(hglrc == NULL)
    {
        if (hdc == NULL && !NtCurrentTeb()->glContext)
        {
            WARN( "Current context is NULL\n");
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }
        dc = OPENGL_GetDefaultDC();
    }
    else
        dc = get_dc_ptr( hdc );

    TRACE("hdc: (%p), hglrc: (%p)\n", hdc, hglrc);

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pwglMakeCurrent );
        update_dc( dc );
        ret = physdev->funcs->pwglMakeCurrent( physdev, hglrc );
        release_dc_ptr( dc );
    }
    return ret;
}

/***********************************************************************
 *		wglMakeContextCurrentARB
 */
static BOOL WINAPI wglMakeContextCurrentARB(HDC hDrawDC, HDC hReadDC, HGLRC hglrc)
{
    BOOL ret = FALSE;
    PHYSDEV draw_physdev, read_physdev;
    DC *DrawDC;
    DC *ReadDC;

    TRACE("hDrawDC: (%p), hReadDC: (%p) hglrc: (%p)\n", hDrawDC, hReadDC, hglrc);

    /* Both hDrawDC and hReadDC need to be valid */
    DrawDC = get_dc_ptr( hDrawDC );
    if (!DrawDC) return FALSE;

    ReadDC = get_dc_ptr( hReadDC );
    if (!ReadDC) {
        release_dc_ptr( DrawDC );
        return FALSE;
    }

    update_dc( DrawDC );
    update_dc( ReadDC );
    draw_physdev = GET_DC_PHYSDEV( DrawDC, pwglMakeContextCurrentARB );
    read_physdev = GET_DC_PHYSDEV( ReadDC, pwglMakeContextCurrentARB );
    if (draw_physdev->funcs == read_physdev->funcs)
        ret = draw_physdev->funcs->pwglMakeContextCurrentARB(draw_physdev, read_physdev, hglrc);
    release_dc_ptr( DrawDC );
    release_dc_ptr( ReadDC );
    return ret;
}

/**************************************************************************************
 *      WINE-specific wglSetPixelFormat which can set the iPixelFormat multiple times
 *
 */
static BOOL WINAPI wglSetPixelFormatWINE(HDC hdc, int iPixelFormat, const PIXELFORMATDESCRIPTOR *ppfd)
{
    INT bRet = FALSE;
    DC * dc = get_dc_ptr( hdc );

    TRACE("(%p,%d,%p)\n", hdc, iPixelFormat, ppfd);

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pwglSetPixelFormatWINE );
        update_dc( dc );
        bRet = physdev->funcs->pwglSetPixelFormatWINE( physdev, iPixelFormat, ppfd );
        release_dc_ptr( dc );
    }
    return bRet;
}

/***********************************************************************
 *		Internal wglGetProcAddress for retrieving WGL extensions
 */
PROC WINAPI wglGetProcAddress(LPCSTR func)
{
    PROC ret = NULL;
    DC *dc;

    if(!func)
	return NULL;

    TRACE("func: '%s'\n", func);

    /* Retrieve the global hDC to get access to the driver.  */
    dc = OPENGL_GetDefaultDC();
    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pwglGetProcAddress );
        ret = physdev->funcs->pwglGetProcAddress(func);
        release_dc_ptr( dc );
    }

    /* At the moment we implement one WGL extension which requires a HDC. When we
     * are looking up this call and when the Extension is available (that is the case
     * when a non-NULL value is returned by wglGetProcAddress), we return the address
     * of a wrapper function which will handle the HDC->PhysDev conversion.
     */
    if(ret && strcmp(func, "wglCreateContextAttribsARB") == 0)
        return (PROC)wglCreateContextAttribsARB;
    else if(ret && strcmp(func, "wglMakeContextCurrentARB") == 0)
        return (PROC)wglMakeContextCurrentARB;
    else if(ret && strcmp(func, "wglSetPixelFormatWINE") == 0)
        return (PROC)wglSetPixelFormatWINE;

    return ret;
}

/******************************************************************************
 *		ChoosePixelFormat (GDI32.@)
 */
INT WINAPI ChoosePixelFormat( HDC hdc, const PIXELFORMATDESCRIPTOR *pfd )
{
    if (!wglChoosePixelFormat)
    {
        if (!opengl32) opengl32 = LoadLibraryW( opengl32W );
        if (!(wglChoosePixelFormat = (void *)GetProcAddress( opengl32, "wglChoosePixelFormat" )))
            return 0;
    }
    return wglChoosePixelFormat( hdc, pfd );
}

/******************************************************************************
 *		DescribePixelFormat (GDI32.@)
 */
INT WINAPI DescribePixelFormat( HDC hdc, INT fmt, UINT size, PIXELFORMATDESCRIPTOR *pfd )
{
    if (!wglDescribePixelFormat)
    {
        if (!opengl32) opengl32 = LoadLibraryW( opengl32W );
        if (!(wglDescribePixelFormat = (void *)GetProcAddress( opengl32, "wglDescribePixelFormat" )))
            return 0;
    }
    return wglDescribePixelFormat( hdc, fmt, size, pfd );
}

/******************************************************************************
 *		SetPixelFormat (GDI32.@)
 */
BOOL WINAPI SetPixelFormat( HDC hdc, INT fmt, const PIXELFORMATDESCRIPTOR *pfd )
{
    if (!wglSetPixelFormat)
    {
        if (!opengl32) opengl32 = LoadLibraryW( opengl32W );
        if (!(wglSetPixelFormat = (void *)GetProcAddress( opengl32, "wglSetPixelFormat" )))
            return 0;
    }
    return wglSetPixelFormat( hdc, fmt, pfd );
}

/******************************************************************************
 *		SwapBuffers (GDI32.@)
 */
BOOL WINAPI SwapBuffers( HDC hdc )
{
    if (!wglSwapBuffers)
    {
        if (!opengl32) opengl32 = LoadLibraryW( opengl32W );
        if (!(wglSwapBuffers = (void *)GetProcAddress( opengl32, "wglSwapBuffers" )))
            return 0;
    }
    return wglSwapBuffers( hdc );
}
