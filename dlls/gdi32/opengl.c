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

static HDC default_hdc = 0;

typedef struct opengl_context
{
    HDC hdc;
} *OPENGL_Context;

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
 *		wglCopyContext (OPENGL32.@)
 */
BOOL WINAPI wglCopyContext(HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask)
{
    DC *dc;
    BOOL ret = FALSE;
    OPENGL_Context ctx = (OPENGL_Context)hglrcSrc;

    TRACE("hglrcSrc: (%p), hglrcDst: (%p), mask: %#x\n", hglrcSrc, hglrcDst, mask);
    /* If no context is set, this call doesn't have a purpose */
    if(!hglrcSrc || !hglrcDst)
        return FALSE;

    /* Retrieve the HDC associated with the context to access the display driver */
    dc = get_dc_ptr(ctx->hdc);
    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pwglCopyContext );
        ret = physdev->funcs->pwglCopyContext(hglrcSrc, hglrcDst, mask);
        release_dc_ptr( dc );
    }
    return ret;
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
 *		wglDeleteContext (OPENGL32.@)
 */
BOOL WINAPI wglDeleteContext(HGLRC hglrc)
{
    DC *dc;
    BOOL ret = FALSE;
    OPENGL_Context ctx = (OPENGL_Context)hglrc;

    TRACE("hglrc: (%p)\n", hglrc);
    if(ctx == NULL)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    /* Retrieve the HDC associated with the context to access the display driver */
    dc = get_dc_ptr(ctx->hdc);
    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pwglDeleteContext );
        ret = physdev->funcs->pwglDeleteContext( hglrc );
        release_dc_ptr( dc );
    }
    else SetLastError(ERROR_INVALID_HANDLE);
    return ret;
}

/***********************************************************************
 *		wglGetCurrentContext (OPENGL32.@)
 */
HGLRC WINAPI wglGetCurrentContext(void)
{
    HGLRC ret = NtCurrentTeb()->glContext;
    TRACE(" returning %p\n", ret);
    return ret;
}

/***********************************************************************
 *		wglGetCurrentDC (OPENGL32.@)
 */
HDC WINAPI wglGetCurrentDC(void)
{
    OPENGL_Context ctx = (OPENGL_Context)wglGetCurrentContext();

    TRACE(" found context: %p\n", ctx);
    if(ctx == NULL)
        return NULL;

    /* Retrieve the current DC from the active context */
    TRACE(" returning hdc: %p\n", ctx->hdc);
    return ctx->hdc;
}

/***********************************************************************
 *		wglGetPbufferDCARB
 */
static HDC WINAPI wglGetPbufferDCARB(void *pbuffer)
{
    HDC ret = 0;

    /* Create a device context to associate with the pbuffer */
    HDC hdc = CreateDCA("DISPLAY", NULL, NULL, NULL);
    DC *dc = get_dc_ptr(hdc);

    TRACE("(%p)\n", pbuffer);

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pwglGetPbufferDCARB );

        /* The display driver has to do the rest of the work because
         * we need access to lowlevel datatypes which we can't access here
         */
        ret = physdev->funcs->pwglGetPbufferDCARB( physdev, pbuffer );
        release_dc_ptr( dc );
    }
    TRACE("(%p), hdc=%p\n", pbuffer, ret);
    if (!ret) DeleteDC( hdc );
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
        if( hdc == NULL && !wglGetCurrentContext() )
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
 *		wglShareLists (OPENGL32.@)
 */
BOOL WINAPI wglShareLists(HGLRC hglrc1, HGLRC hglrc2)
{
    DC *dc;
    BOOL ret = FALSE;
    OPENGL_Context ctx = (OPENGL_Context)hglrc1;

    TRACE("hglrc1: (%p); hglrc: (%p)\n", hglrc1, hglrc2);
    if(ctx == NULL || hglrc2 == NULL)
        return FALSE;

    /* Retrieve the HDC associated with the context to access the display driver */
    dc = get_dc_ptr(ctx->hdc);
    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pwglShareLists );
        ret = physdev->funcs->pwglShareLists( hglrc1, hglrc2 );
        release_dc_ptr( dc );
    }
    return ret;
}

/***********************************************************************
 *		wglUseFontBitmapsA (OPENGL32.@)
 */
BOOL WINAPI wglUseFontBitmapsA(HDC hdc, DWORD first, DWORD count, DWORD listBase)
{
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );

    TRACE("(%p, %d, %d, %d)\n", hdc, first, count, listBase);

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pwglUseFontBitmapsA );
        ret = physdev->funcs->pwglUseFontBitmapsA( physdev, first, count, listBase );
        release_dc_ptr( dc );
    }
    return ret;
}

/***********************************************************************
 *		wglUseFontBitmapsW (OPENGL32.@)
 */
BOOL WINAPI wglUseFontBitmapsW(HDC hdc, DWORD first, DWORD count, DWORD listBase)
{
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );

    TRACE("(%p, %d, %d, %d)\n", hdc, first, count, listBase);

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pwglUseFontBitmapsW );
        ret = physdev->funcs->pwglUseFontBitmapsW( physdev, first, count, listBase );
        release_dc_ptr( dc );
    }
    return ret;
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
    else if(ret && strcmp(func, "wglGetPbufferDCARB") == 0)
        return (PROC)wglGetPbufferDCARB;
    else if(ret && strcmp(func, "wglSetPixelFormatWINE") == 0)
        return (PROC)wglSetPixelFormatWINE;

    return ret;
}
