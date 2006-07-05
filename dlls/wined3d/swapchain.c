/*
 *IDirect3DSwapChain9 implementation
 *
 *Copyright 2002-2003 Jason Edmeades
 *Copyright 2002-2003 Raphael Junqueira
 *Copyright 2005 Oliver Stieber
 *
 *This library is free software; you can redistribute it and/or
 *modify it under the terms of the GNU Lesser General Public
 *License as published by the Free Software Foundation; either
 *version 2.1 of the License, or (at your option) any later version.
 *
 *This library is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *Lesser General Public License for more details.
 *
 *You should have received a copy of the GNU Lesser General Public
 *License along with this library; if not, write to the Free Software
 *Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wined3d_private.h"


/* TODO: move to shared header (or context manager )*/
/* x11drv GDI escapes */
#define X11DRV_ESCAPE 6789
enum x11drv_escape_codes
{
    X11DRV_GET_DISPLAY,   /* get X11 display for a DC */
    X11DRV_GET_DRAWABLE,  /* get current drawable for a DC */
    X11DRV_GET_FONT,      /* get current X font for a DC */
};

/* retrieve the X display to use on a given DC */
inline static Display *get_display( HDC hdc )
{
    Display *display;
    enum x11drv_escape_codes escape = X11DRV_GET_DISPLAY;

    if (!ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPCSTR)&escape,
                    sizeof(display), (LPSTR)&display )) display = NULL;
    return display;
}

/*TODO: some of the additional parameters may be required to
    set the gamma ramp (for some weird reason microsoft have left swap gammaramp in device
    but it operates on a swapchain, it may be a good idea to move it to IWineD3DSwapChain for IWineD3D)*/


WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(fps);


/* IDirect3DSwapChain IUnknown parts follow: */
static ULONG WINAPI IWineD3DSwapChainImpl_AddRef(IWineD3DSwapChain *iface) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    DWORD refCount = InterlockedIncrement(&This->ref);
    TRACE("(%p) : AddRef increasing from %ld\n", This, refCount - 1);
    return refCount;
}

static HRESULT WINAPI IWineD3DSwapChainImpl_QueryInterface(IWineD3DSwapChain *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DSwapChain)){
        IWineD3DSwapChainImpl_AddRef(iface);
        if(ppobj == NULL){
            ERR("Query interface called but now data allocated\n");
            return E_NOINTERFACE;
        }
        *ppobj = This;
        return WINED3D_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}


static ULONG WINAPI IWineD3DSwapChainImpl_Release(IWineD3DSwapChain *iface) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    DWORD refCount;
    refCount = InterlockedDecrement(&This->ref);
    TRACE("(%p) : ReleaseRef to %ld\n", This, refCount);
    if (refCount == 0) {
        IUnknown* bufferParent;

        /* release the ref to the front and back buffer parents */
        if(This->frontBuffer) {
            IWineD3DSurface_SetContainer(This->frontBuffer, 0);
            IWineD3DSurface_GetParent(This->frontBuffer, &bufferParent);
            IUnknown_Release(bufferParent); /* once for the get parent */
            if(IUnknown_Release(bufferParent) > 0){
                FIXME("(%p) Something's still holding the front buffer\n",This);
            }
        }

        if(This->backBuffer) {
            int i;
            for(i = 0; i < This->presentParms.BackBufferCount; i++) {
                IWineD3DSurface_SetContainer(This->backBuffer[i], 0);
                IWineD3DSurface_GetParent(This->backBuffer[i], &bufferParent);
                IUnknown_Release(bufferParent); /* once for the get parent */
                if(IUnknown_Release(bufferParent) > 0){
                    FIXME("(%p) Something's still holding the back buffer\n",This);
                }
            }
        }

        /* Clean up the context */
        /* check that we are the current context first */
        if(glXGetCurrentContext() == This->glCtx){
            glXMakeCurrent(This->display, None, NULL);
        }
        glXDestroyContext(This->display, This->glCtx);
        /* IUnknown_Release(This->parent); This should only apply to the primary swapchain,
         all others are crated by the caller, so releasing the parent should cause
         the child to be released, not the other way around!
         */
        HeapFree(GetProcessHeap(), 0, This);
    }
    return refCount;
}

static HRESULT WINAPI IWineD3DSwapChainImpl_GetParent(IWineD3DSwapChain *iface, IUnknown ** ppParent){
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    *ppParent = This->parent;
    IUnknown_AddRef(*ppParent);
    TRACE("(%p) returning %p\n", This , *ppParent);
    return WINED3D_OK;
}

/*IWineD3DSwapChain parts follow: */
static HRESULT WINAPI IWineD3DSwapChainImpl_Present(IWineD3DSwapChain *iface, CONST RECT *pSourceRect, CONST RECT *pDestRect, HWND hDestWindowOverride, CONST RGNDATA *pDirtyRegion, DWORD dwFlags) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;

    ENTER_GL();

    if (pSourceRect || pDestRect) FIXME("Unhandled present options %p/%p\n", pSourceRect, pDestRect);
    /* TODO: If only source rect or dest rect are supplied then clip the window to match */
    TRACE("preseting display %p, drawable %ld\n", This->display, This->drawable);

    /* Don't call checkGLcall, as glGetError is not applicable here */
    if (hDestWindowOverride && This->win_handle != hDestWindowOverride) {
        /* Set this swapchain up to point to the new destination.. */
#ifdef USE_CONTEXT_MANAGER
            /* TODO: use a context mamager */
#endif

            /* FIXME: Never access */
            IWineD3DSwapChainImpl *swapChainImpl;
            IWineD3DDevice_GetSwapChain((IWineD3DDevice *)This->wineD3DDevice, 0 , (IWineD3DSwapChain **)&swapChainImpl);
            FIXME("Unable to render to a destination window %p\n", hDestWindowOverride );
            if(This == swapChainImpl){
                /* FIXME: this will be fixed by moving to a context management system */
                FIXME("Cannot change the target of the implicit swapchain\n");
            }else{
                HDC               hDc;
                XVisualInfo       template;
                int               num;
                Display          *oldDisplay = This->display;
                GLXContext        oldContext = This->glCtx;
                IUnknown*         tmp;
                GLXContext        currentContext;
                Drawable          currentDrawable;
                hDc                          = GetDC(hDestWindowOverride);
                This->win_handle             = hDestWindowOverride;
                This->win                    = (Window)GetPropA( hDestWindowOverride, "__wine_x11_whole_window" );

                TRACE("Creating a new context for the window %p\n", hDestWindowOverride);
                ENTER_GL();
                TRACE("Desctroying context %p %p\n", This->display, This->render_ctx);



                LEAVE_GL();
                ENTER_GL();

                This->display    = get_display(hDc);
                TRACE("Got display%p  for  %p %p\n",  This->display, hDc, hDestWindowOverride);
                ReleaseDC(hDestWindowOverride, hDc);
                template.visualid = (VisualID)GetPropA(GetDesktopWindow(), "__wine_x11_visual_id");
                This->visInfo   = XGetVisualInfo(This->display, VisualIDMask, &template, &num);
                if (NULL == This->visInfo) {
                    ERR("cannot really get XVisual\n");
                    LEAVE_GL();
                    return WINED3DERR_NOTAVAILABLE;
                }
                /* Now we have problems? well not really we just need to know what the implicit context is */
                /* now destroy the old context and create a new one (we should really copy the buffers over, and do the whole make current thing! */
                /* destroy the active context?*/
                TRACE("Creating new context for %p %p %p\n",This->display, This->visInfo, swapChainImpl->glCtx);
                This->glCtx = glXCreateContext(This->display, This->visInfo, swapChainImpl->glCtx, GL_TRUE);

                if (NULL == This->glCtx) {
                    ERR("cannot create glxContext\n");
                }
                This->drawable     = This->win;
                This->render_ctx   = This->glCtx;
                /* Setup some default states TODO: apply the stateblock to the new context */
                /** save current context and drawable **/
                currentContext  =   glXGetCurrentContext();
                currentDrawable =   glXGetCurrentDrawable();

                if (glXMakeCurrent(This->display, This->win, This->glCtx) == False) {
                    ERR("Error in setting current context (display %p context %p drawable %ld)!\n", This->display, This->glCtx, This->win);
                }

                checkGLcall("glXMakeCurrent");

                /* Clear the screen */
                glClearColor(0.0, 0.0, 0.0, 0.0);
                checkGLcall("glClearColor");
                glClearIndex(0);
                glClearDepth(1);
                glClearStencil(0);

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
                checkGLcall("glClear");

                glColor3f(1.0, 1.0, 1.0);
                checkGLcall("glColor3f");

                glEnable(GL_LIGHTING);
                checkGLcall("glEnable");

                glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
                checkGLcall("glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);");

                glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
                checkGLcall("glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);");

                glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
                checkGLcall("glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);");

                /* If this swapchain is currently the active context then make this swapchain active */
                if(IWineD3DSurface_GetContainer(This->wineD3DDevice->renderTarget, &IID_IWineD3DSwapChain, (void **)&tmp) == WINED3D_OK){
                    if(tmp != (IUnknown *)This){
                        glXMakeCurrent(This->display, currentDrawable, currentContext);
                        checkGLcall("glXMakeCurrent");
                    }
                    IUnknown_Release(tmp);
                }else{
                    /* reset the context */
                    glXMakeCurrent(This->display, currentDrawable, currentContext);
                    checkGLcall("glXMakeCurrent");
                }
                /* delete the old contxt*/
                glXDestroyContext(oldDisplay, oldContext); /* Should this happen on an active context? seems a bad idea */
                LEAVE_GL();
            }
            IWineD3DSwapChain_Release((IWineD3DSwapChain *)swapChainImpl);

        }


        /* TODO: The slow way, save the data to memory, create a new context for the destination window, transfer the data cleanup, it may be a good idea to the move this swapchain over to the using the target winows context so that it runs faster in feature. */

    glXSwapBuffers(This->display, This->drawable); /* TODO: cycle through the swapchain buffers */

    TRACE("glXSwapBuffers called, Starting new frame\n");
    /* FPS support */
    if (TRACE_ON(fps))
    {
        static long prev_time, frames;

        DWORD time = GetTickCount();
        frames++;
        /* every 1.5 seconds */
        if (time - prev_time > 1500) {
            TRACE_(fps)("@ approx %.2ffps\n", 1000.0*frames/(time - prev_time));
            prev_time = time;
            frames = 0;
        }
    }

#if defined(FRAME_DEBUGGING)
{
    if (GetFileAttributesA("C:\\D3DTRACE") != INVALID_FILE_ATTRIBUTES) {
        if (!isOn) {
            isOn = TRUE;
            FIXME("Enabling D3D Trace\n");
            __WINE_SET_DEBUGGING(__WINE_DBCL_TRACE, __wine_dbch_d3d, 1);
#if defined(SHOW_FRAME_MAKEUP)
            FIXME("Singe Frame snapshots Starting\n");
            isDumpingFrames = TRUE;
            glClear(GL_COLOR_BUFFER_BIT);
#endif

#if defined(SINGLE_FRAME_DEBUGGING)
        } else {
#if defined(SHOW_FRAME_MAKEUP)
            FIXME("Singe Frame snapshots Finishing\n");
            isDumpingFrames = FALSE;
#endif
            FIXME("Singe Frame trace complete\n");
            DeleteFileA("C:\\D3DTRACE");
            __WINE_SET_DEBUGGING(__WINE_DBCL_TRACE, __wine_dbch_d3d, 0);
#endif
        }
    } else {
        if (isOn) {
            isOn = FALSE;
#if defined(SHOW_FRAME_MAKEUP)
            FIXME("Single Frame snapshots Finishing\n");
            isDumpingFrames = FALSE;
#endif
            FIXME("Disabling D3D Trace\n");
            __WINE_SET_DEBUGGING(__WINE_DBCL_TRACE, __wine_dbch_d3d, 0);
        }
    }
}
#endif

    LEAVE_GL();
    /* Although this is not strictly required, a simple demo showed this does occur
       on (at least non-debug) d3d                                                  */
    if (This->presentParms.SwapEffect == WINED3DSWAPEFFECT_DISCARD) {

        TRACE("Clearing\n");

        IWineD3DDevice_Clear((IWineD3DDevice*)This->wineD3DDevice, 0, NULL, D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER|D3DCLEAR_TARGET, 0x00, 1.0, 0);

    } else {
        TRACE("Clearing z/stencil buffer\n");

        IWineD3DDevice_Clear((IWineD3DDevice*)This->wineD3DDevice, 0, NULL, D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER, 0x00, 1.0, 0);
    }

    ((IWineD3DSurfaceImpl *) This->frontBuffer)->Flags |= SFLAG_GLDIRTY;
    ((IWineD3DSurfaceImpl *) This->backBuffer[0])->Flags |= SFLAG_GLDIRTY;

    TRACE("returning\n");
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DSwapChainImpl_GetFrontBufferData(IWineD3DSwapChain *iface, IWineD3DSurface *pDestSurface) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    WINED3DFORMAT d3dformat;
    UINT width;
    UINT height;
    WINED3DSURFACE_DESC desc;
    glDescriptor *glDescription;

    TRACE("(%p) : iface(%p) pDestSurface(%p)\n", This, iface, pDestSurface);
    ENTER_GL();
    memset(&desc, 0, sizeof(desc));
    desc.Width =  &width;
    desc.Height = &height;
    desc.Format = &d3dformat;
#if 0 /* TODO: make sure that this swapchains context is active */
    IWineD3DDevice_ActivateSwapChainContext(This->wineD3DDevice, iface);
#endif
    IWineD3DSurface_GetDesc(pDestSurface, &desc);
    /* make sure that the front buffer is the active read buffer */
    glReadBuffer(GL_FRONT);
    /* Read the pixels from the buffer into the surfaces memory */
    IWineD3DSurface_GetGlDesc(pDestSurface, &glDescription);
    glReadPixels(glDescription->target,
                glDescription->level,
                width,
                height,
                glDescription->glFormat,
                glDescription->glType,
                (void *)IWineD3DSurface_GetData(pDestSurface));
    LEAVE_GL();
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DSwapChainImpl_GetBackBuffer(IWineD3DSwapChain *iface, UINT iBackBuffer, WINED3DBACKBUFFER_TYPE Type, IWineD3DSurface **ppBackBuffer) {

    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;

    if (iBackBuffer > This->presentParms.BackBufferCount - 1) {
        TRACE("Back buffer count out of range\n");
        /* Native d3d9 doesn't set NULL here, just as wine's d3d9. But set it here
         * in wined3d to avoid problems in other libs
         */
        *ppBackBuffer = NULL;
        return WINED3DERR_INVALIDCALL;
    }

    *ppBackBuffer = This->backBuffer[iBackBuffer];
    TRACE("(%p) : BackBuf %d Type %d  returning %p\n", This, iBackBuffer, Type, *ppBackBuffer);

    /* Note inc ref on returned surface */
    if(*ppBackBuffer) IWineD3DSurface_AddRef(*ppBackBuffer);
    return WINED3D_OK;

}

static HRESULT WINAPI IWineD3DSwapChainImpl_GetRasterStatus(IWineD3DSwapChain *iface, WINED3DRASTER_STATUS *pRasterStatus) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    static BOOL showFixmes = TRUE;
    pRasterStatus->InVBlank = TRUE;
    pRasterStatus->ScanLine = 0;
    /* No openGL equivalent */
    if(showFixmes) {
        FIXME("(%p) : stub (once)\n", This);
        showFixmes = FALSE;
    }
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DSwapChainImpl_GetDisplayMode(IWineD3DSwapChain *iface, WINED3DDISPLAYMODE*pMode) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    HDC                 hdc;
    int                 bpp = 0;

    pMode->Width        = GetSystemMetrics(SM_CXSCREEN);
    pMode->Height       = GetSystemMetrics(SM_CYSCREEN);
    pMode->RefreshRate  = 85; /* FIXME: How to identify? */

    hdc = CreateDCA("DISPLAY", NULL, NULL, NULL);
    bpp = GetDeviceCaps(hdc, BITSPIXEL);
    DeleteDC(hdc);

    switch (bpp) {
    case  8: pMode->Format       = D3DFMT_R8G8B8; break;
    case 16: pMode->Format       = D3DFMT_R5G6B5; break;
    case 24: /*pMode->Format       = D3DFMT_R8G8B8; break; */ /* 32bpp and 24bpp can be aliased for X */
    case 32: pMode->Format       = D3DFMT_A8R8G8B8; break;
    default:
       FIXME("Unrecognized display mode format\n");
       pMode->Format       = D3DFMT_UNKNOWN;
    }

    TRACE("(%p) : returning w(%d) h(%d) rr(%d) fmt(%u,%s)\n", This, pMode->Width, pMode->Height, pMode->RefreshRate,
    pMode->Format, debug_d3dformat(pMode->Format));
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DSwapChainImpl_GetDevice(IWineD3DSwapChain *iface, IWineD3DDevice**ppDevice) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;

    *ppDevice = (IWineD3DDevice *) This->wineD3DDevice;

    /* Note  Calling this method will increase the internal reference count
       on the IDirect3DDevice9 interface. */
    IWineD3DDevice_AddRef(*ppDevice);
    TRACE("(%p) : returning %p\n", This, *ppDevice);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DSwapChainImpl_GetPresentParameters(IWineD3DSwapChain *iface, WINED3DPRESENT_PARAMETERS *pPresentationParameters) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    TRACE("(%p)\n", This);
    *pPresentationParameters->BackBufferWidth = This->presentParms.BackBufferWidth;
    *pPresentationParameters->BackBufferHeight = This->presentParms.BackBufferHeight;
    *pPresentationParameters->BackBufferFormat = This->presentParms.BackBufferFormat;
    *pPresentationParameters->BackBufferCount = This->presentParms.BackBufferCount;
    *pPresentationParameters->MultiSampleType = This->presentParms.MultiSampleType;
    *pPresentationParameters->MultiSampleQuality = This->presentParms.MultiSampleQuality;
    *pPresentationParameters->SwapEffect = This->presentParms.SwapEffect;
    *pPresentationParameters->hDeviceWindow = This->presentParms.hDeviceWindow;
    *pPresentationParameters->Windowed = This->presentParms.Windowed;
    *pPresentationParameters->EnableAutoDepthStencil = This->presentParms.EnableAutoDepthStencil;
    *pPresentationParameters->Flags = This->presentParms.Flags;
    *pPresentationParameters->FullScreen_RefreshRateInHz = This->presentParms.FullScreen_RefreshRateInHz;
    *pPresentationParameters->PresentationInterval = This->presentParms.PresentationInterval;
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DSwapChainImpl_SetGammaRamp(IWineD3DSwapChain *iface, DWORD Flags, CONST WINED3DGAMMARAMP *pRamp){

    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    HDC hDC;
    TRACE("(%p) : pRamp@%p flags(%ld)\n", This, pRamp, Flags);
    hDC = GetDC(This->win_handle);
    SetDeviceGammaRamp(hDC, (LPVOID)pRamp);
    ReleaseDC(This->win_handle, hDC);
    return WINED3D_OK;

}

static HRESULT WINAPI IWineD3DSwapChainImpl_GetGammaRamp(IWineD3DSwapChain *iface, WINED3DGAMMARAMP *pRamp){

    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    HDC hDC;
    TRACE("(%p) : pRamp@%p\n", This, pRamp);
    hDC = GetDC(This->win_handle);
    GetDeviceGammaRamp(hDC, pRamp);
    ReleaseDC(This->win_handle, hDC);
    return WINED3D_OK;

}


IWineD3DSwapChainVtbl IWineD3DSwapChain_Vtbl =
{
    /* IUnknown */
    IWineD3DSwapChainImpl_QueryInterface,
    IWineD3DSwapChainImpl_AddRef,
    IWineD3DSwapChainImpl_Release,
    /* IWineD3DSwapChain */
    IWineD3DSwapChainImpl_GetParent,
    IWineD3DSwapChainImpl_GetDevice,
    IWineD3DSwapChainImpl_Present,
    IWineD3DSwapChainImpl_GetFrontBufferData,
    IWineD3DSwapChainImpl_GetBackBuffer,
    IWineD3DSwapChainImpl_GetRasterStatus,
    IWineD3DSwapChainImpl_GetDisplayMode,
    IWineD3DSwapChainImpl_GetPresentParameters,
    IWineD3DSwapChainImpl_SetGammaRamp,
    IWineD3DSwapChainImpl_GetGammaRamp
};
