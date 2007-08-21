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
 * Using DC_GetDCPtr we get access to the functions exported by the driver.
 * Some functions don't receive a hDC. This function creates a global hdc and
 * if there's already a global hdc, it returns it.
 */
static DC* OPENGL_GetDefaultDC(void)
{
    if(!default_hdc)
        default_hdc = CreateDCA("DISPLAY", NULL, NULL, NULL);
        
    return DC_GetDCPtr(default_hdc);
}

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

    DC_ReleaseDCPtr( dc );
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
        return FALSE;

    /* Retrieve the HDC associated with the context to access the display driver */
    dc = DC_GetDCPtr(ctx->hdc);
    if (!dc) return FALSE;

    if (!dc->funcs->pwglDeleteContext) FIXME(" :stub\n");
    else ret = dc->funcs->pwglDeleteContext(hglrc);

    DC_ReleaseDCPtr( dc );
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
    DC *dc = DC_GetDCPtr(hdc);

    TRACE("(%p)\n", pbuffer);

    if (!dc) return FALSE;

    /* The display driver has to do the rest of the work because
     * we need access to lowlevel datatypes which we can't access here
     */
    if (!dc->funcs->pwglGetPbufferDCARB) FIXME(" :stub\n");
    else ret = dc->funcs->pwglGetPbufferDCARB(dc->physDev, pbuffer);

    TRACE("(%p), hdc=%p\n", pbuffer, ret);
    
    DC_ReleaseDCPtr( dc );
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
        dc = OPENGL_GetDefaultDC();
    else
        dc = DC_GetDCUpdate( hdc );

    TRACE("hdc: (%p), hglrc: (%p)\n", hdc, hglrc);

    if (!dc) return FALSE;

    if (!dc->funcs->pwglMakeCurrent) FIXME(" :stub\n");
    else ret = dc->funcs->pwglMakeCurrent(dc->physDev,hglrc);

    DC_ReleaseDCPtr( dc );
    return ret;
}

/***********************************************************************
 *		wglMakeContextCurrentARB
 */
static BOOL WINAPI wglMakeContextCurrentARB(HDC hDrawDC, HDC hReadDC, HGLRC hglrc)
{
    BOOL ret = FALSE;
    DC *DrawDC;
    DC *ReadDC;

    TRACE("hDrawDC: (%p), hReadDC: (%p) hglrc: (%p)\n", hDrawDC, hReadDC, hglrc);

    /* Both hDrawDC and hReadDC need to be valid */
    DrawDC = DC_GetDCPtr( hDrawDC);
    if (!DrawDC) return FALSE;

    ReadDC = DC_GetDCPtr( hReadDC);
    if (!ReadDC) {
        DC_ReleaseDCPtr(DrawDC);
        return FALSE;
    }

    if (!DrawDC->funcs->pwglMakeContextCurrentARB) FIXME(" :stub\n");
    else ret = DrawDC->funcs->pwglMakeContextCurrentARB(DrawDC->physDev, ReadDC->physDev, hglrc);

    DC_ReleaseDCPtr(DrawDC);
    DC_ReleaseDCPtr(ReadDC);

    return ret;
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
    if(ctx == NULL)
        return FALSE;
    
    /* Retrieve the HDC associated with the context to access the display driver */
    dc = DC_GetDCPtr(ctx->hdc);
    if (!dc) return FALSE;

    if (!dc->funcs->pwglShareLists) FIXME(" :stub\n");
    else ret = dc->funcs->pwglShareLists(hglrc1, hglrc2);

    DC_ReleaseDCPtr( dc );
    return ret;
}

/***********************************************************************
 *		wglUseFontBitmapsA (OPENGL32.@)
 */
BOOL WINAPI wglUseFontBitmapsA(HDC hdc, DWORD first, DWORD count, DWORD listBase)
{
    BOOL ret = FALSE;
    DC * dc = DC_GetDCPtr( hdc );

    TRACE("(%p, %d, %d, %d)\n", hdc, first, count, listBase);

    if (!dc) return FALSE;

    if (!dc->funcs->pwglUseFontBitmapsA) FIXME(" :stub\n");
    else ret = dc->funcs->pwglUseFontBitmapsA(dc->physDev, first, count, listBase);

    DC_ReleaseDCPtr( dc );
    return ret;
}

/***********************************************************************
 *		wglUseFontBitmapsW (OPENGL32.@)
 */
BOOL WINAPI wglUseFontBitmapsW(HDC hdc, DWORD first, DWORD count, DWORD listBase)
{
    BOOL ret = FALSE;
    DC * dc = DC_GetDCPtr( hdc );

    TRACE("(%p, %d, %d, %d)\n", hdc, first, count, listBase);

    if (!dc) return FALSE;

    if (!dc->funcs->pwglUseFontBitmapsW) FIXME(" :stub\n");
    else ret = dc->funcs->pwglUseFontBitmapsW(dc->physDev, first, count, listBase);

    DC_ReleaseDCPtr( dc );
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

    TRACE("func: '%p'\n", func);

    /* Retrieve the global hDC to get access to the driver.  */
    dc = OPENGL_GetDefaultDC();
    if (!dc) return FALSE;

    if (!dc->funcs->pwglGetProcAddress) FIXME(" :stub\n");
    else ret = dc->funcs->pwglGetProcAddress(func);

    DC_ReleaseDCPtr( dc );

    /* At the moment we implement one WGL extension which requires a HDC. When we
     * are looking up this call and when the Extension is available (that is the case
     * when a non-NULL value is returned by wglGetProcAddress), we return the address
     * of a wrapper function which will handle the HDC->PhysDev conversion.
     */
    if(ret && strcmp(func, "wglMakeContextCurrentARB") == 0)
        return (PROC)wglMakeContextCurrentARB;
    else if(ret && strcmp(func, "wglGetPbufferDCARB") == 0)
        return (PROC)wglGetPbufferDCARB;

    return ret;
}
