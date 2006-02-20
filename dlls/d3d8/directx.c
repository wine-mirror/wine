/*
 * IDirect3D8 implementation
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
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

#include "config.h"

#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/* x11drv GDI escapes */
#define X11DRV_ESCAPE 6789
enum x11drv_escape_codes
{
    X11DRV_GET_DISPLAY,   /* get X11 display for a DC */
    X11DRV_GET_DRAWABLE,  /* get current drawable for a DC */
    X11DRV_GET_FONT,      /* get current X font for a DC */
};

#define NUM_FORMATS 7
static const D3DFORMAT device_formats[NUM_FORMATS] = {
  D3DFMT_P8,
  D3DFMT_R3G3B2,
  D3DFMT_R5G6B5, 
  D3DFMT_X1R5G5B5,
  D3DFMT_X4R4G4B4,
  D3DFMT_R8G8B8,
  D3DFMT_X8R8G8B8
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

/* IDirect3D IUnknown parts follow: */
HRESULT WINAPI IDirect3D8Impl_QueryInterface(LPDIRECT3D8 iface,REFIID riid,LPVOID *ppobj)
{
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3D8)) {
        IDirect3D8Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3D8Impl_AddRef(LPDIRECT3D8 iface) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %ld\n", This, ref - 1);

    return ref;
}

ULONG WINAPI IDirect3D8Impl_Release(LPDIRECT3D8 iface) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %ld\n", This, ref);

    if (ref == 0) {
        IWineD3D_Release(This->WineD3D);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3D Interface follow: */
HRESULT WINAPI IDirect3D8Impl_RegisterSoftwareDevice (LPDIRECT3D8 iface, void* pInitializeFunction) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    return IWineD3D_RegisterSoftwareDevice(This->WineD3D, pInitializeFunction);
}

UINT WINAPI IDirect3D8Impl_GetAdapterCount (LPDIRECT3D8 iface) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    return IWineD3D_GetAdapterCount(This->WineD3D);
}

HRESULT  WINAPI  IDirect3D8Impl_GetAdapterIdentifier       (LPDIRECT3D8 iface,
                                                            UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER8* pIdentifier) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    WINED3DADAPTER_IDENTIFIER adapter_id;

    /* dx8 and dx9 have different structures to be filled in, with incompatible 
       layouts so pass in pointers to the places to be filled via an internal 
       structure                                                                */
    adapter_id.Driver           = pIdentifier->Driver;          
    adapter_id.Description      = pIdentifier->Description;     
    adapter_id.DeviceName       = NULL;      
    adapter_id.DriverVersion    = &pIdentifier->DriverVersion;   
    adapter_id.VendorId         = &pIdentifier->VendorId;        
    adapter_id.DeviceId         = &pIdentifier->DeviceId;        
    adapter_id.SubSysId         = &pIdentifier->SubSysId;        
    adapter_id.Revision         = &pIdentifier->Revision;        
    adapter_id.DeviceIdentifier = &pIdentifier->DeviceIdentifier;
    adapter_id.WHQLLevel        = &pIdentifier->WHQLLevel;       

    return IWineD3D_GetAdapterIdentifier(This->WineD3D, Adapter, Flags, &adapter_id);
}

UINT WINAPI IDirect3D8Impl_GetAdapterModeCount (LPDIRECT3D8 iface,UINT Adapter) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    return IWineD3D_GetAdapterModeCount(This->WineD3D, Adapter, D3DFMT_UNKNOWN);
}

HRESULT WINAPI IDirect3D8Impl_EnumAdapterModes (LPDIRECT3D8 iface, UINT Adapter, UINT Mode, D3DDISPLAYMODE* pMode) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    return IWineD3D_EnumAdapterModes(This->WineD3D, Adapter, D3DFMT_UNKNOWN, Mode, pMode);
}

HRESULT WINAPI IDirect3D8Impl_GetAdapterDisplayMode (LPDIRECT3D8 iface, UINT Adapter, D3DDISPLAYMODE* pMode) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    return IWineD3D_GetAdapterDisplayMode(This->WineD3D, Adapter, pMode);
}

HRESULT  WINAPI  IDirect3D8Impl_CheckDeviceType            (LPDIRECT3D8 iface,
                                                            UINT Adapter, D3DDEVTYPE CheckType, D3DFORMAT DisplayFormat,
                                                            D3DFORMAT BackBufferFormat, BOOL Windowed) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    return IWineD3D_CheckDeviceType(This->WineD3D, Adapter, CheckType, DisplayFormat,
                                    BackBufferFormat, Windowed);
}

HRESULT  WINAPI  IDirect3D8Impl_CheckDeviceFormat          (LPDIRECT3D8 iface,
                                                            UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat,
                                                            DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    return IWineD3D_CheckDeviceFormat(This->WineD3D, Adapter, DeviceType, AdapterFormat,
                                    Usage, RType, CheckFormat);
}

HRESULT  WINAPI  IDirect3D8Impl_CheckDeviceMultiSampleType(LPDIRECT3D8 iface,
							   UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat,
							   BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    return IWineD3D_CheckDeviceMultiSampleType(This->WineD3D, Adapter, DeviceType, SurfaceFormat,
                                               Windowed, MultiSampleType, NULL);
}

HRESULT  WINAPI  IDirect3D8Impl_CheckDepthStencilMatch(LPDIRECT3D8 iface, 
						       UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat,
						       D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    return IWineD3D_CheckDepthStencilMatch(This->WineD3D, Adapter, DeviceType, AdapterFormat,
                                           RenderTargetFormat, DepthStencilFormat);
}

HRESULT  WINAPI  IDirect3D8Impl_GetDeviceCaps(LPDIRECT3D8 iface, UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS8* pCaps) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    HRESULT hrc = D3D_OK;
    WINED3DCAPS *pWineCaps;

    TRACE("(%p) Relay %d %u %p\n", This, Adapter, DeviceType, pCaps);

    if(NULL == pCaps){
        return D3DERR_INVALIDCALL;
    }
    pWineCaps = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINED3DCAPS));
    if(pWineCaps == NULL){
        return D3DERR_INVALIDCALL; /*well this is what MSDN says to return*/
    }
    D3D8CAPSTOWINECAPS(pCaps, pWineCaps)
    hrc = IWineD3D_GetDeviceCaps(This->WineD3D, Adapter, DeviceType, pWineCaps);
    HeapFree(GetProcessHeap(), 0, pWineCaps);
    TRACE("(%p) returning %p\n", This, pCaps);
    return hrc;
}

HMONITOR WINAPI  IDirect3D8Impl_GetAdapterMonitor(LPDIRECT3D8 iface, UINT Adapter) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    return IWineD3D_GetAdapterMonitor(This->WineD3D, Adapter);
}

/* Internal function called back during the CreateDevice to create a render target */
HRESULT WINAPI D3D8CB_CreateRenderTarget(IUnknown *device, UINT Width, UINT Height, 
                                         WINED3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, 
                                         DWORD MultisampleQuality, BOOL Lockable, 
                                         IWineD3DSurface** ppSurface, HANDLE* pSharedHandle) {
    HRESULT res = D3D_OK;
    IDirect3DSurface8Impl *d3dSurface = NULL;

    /* Note - Throw away MultisampleQuality and SharedHandle - only relevant for d3d9  */
    res = IDirect3DDevice8_CreateRenderTarget((IDirect3DDevice8 *)device, Width, Height, 
                                         (D3DFORMAT)Format, MultiSample, Lockable, 
                                         (IDirect3DSurface8 **)&d3dSurface);
    if (res == D3D_OK) {
        *ppSurface = d3dSurface->wineD3DSurface;
    }
    return res;
}

/* Callback for creating the inplicite swapchain when the device is created */
HRESULT WINAPI D3D8CB_CreateAdditionalSwapChain(IUnknown *device,
                                                WINED3DPRESENT_PARAMETERS* pPresentationParameters,
                                                IWineD3DSwapChain ** ppSwapChain){
    HRESULT res = D3D_OK;
    IDirect3DSwapChain8Impl *d3dSwapChain = NULL;
    /* We have to pass the presentation parameters back and forth */
    D3DPRESENT_PARAMETERS localParameters;
    localParameters.BackBufferWidth                = *(pPresentationParameters->BackBufferWidth);
    localParameters.BackBufferHeight               = *(pPresentationParameters->BackBufferHeight);
    localParameters.BackBufferFormat               = *(pPresentationParameters->BackBufferFormat);
    localParameters.BackBufferCount                = *(pPresentationParameters->BackBufferCount);
    localParameters.MultiSampleType                = *(pPresentationParameters->MultiSampleType);
  /* d3d9 only */
  /* localParameters.MultiSampleQuality             = *(pPresentationParameters->MultiSampleQuality); */
    localParameters.SwapEffect                     = *(pPresentationParameters->SwapEffect);
    localParameters.hDeviceWindow                  = *(pPresentationParameters->hDeviceWindow);
    localParameters.Windowed                       = *(pPresentationParameters->Windowed);
    localParameters.EnableAutoDepthStencil         = *(pPresentationParameters->EnableAutoDepthStencil);
    localParameters.AutoDepthStencilFormat         = *(pPresentationParameters->AutoDepthStencilFormat);
    localParameters.Flags                          = *(pPresentationParameters->Flags);
    localParameters.FullScreen_RefreshRateInHz     = *(pPresentationParameters->FullScreen_RefreshRateInHz);
/* not in d3d8 */
/*    localParameters.PresentationInterval           = *(pPresentationParameters->PresentationInterval); */

    TRACE("(%p) rellaying\n", device);
    /*copy the presentation parameters*/
    res = IDirect3DDevice8_CreateAdditionalSwapChain((IDirect3DDevice8 *)device, &localParameters, (IDirect3DSwapChain8 **)&d3dSwapChain);

    if (res == D3D_OK){
        *ppSwapChain = d3dSwapChain->wineD3DSwapChain;
    } else {
        FIXME("failed to create additional swap chain\n");
        *ppSwapChain = NULL;
    }
    /* Copy back the presentation parameters */
    TRACE("(%p) setting up return parameters\n", device);
   *pPresentationParameters->BackBufferWidth               = localParameters.BackBufferWidth;
   *pPresentationParameters->BackBufferHeight              = localParameters.BackBufferHeight;
   *pPresentationParameters->BackBufferFormat              = localParameters.BackBufferFormat;
   *pPresentationParameters->BackBufferCount               = localParameters.BackBufferCount;
   *pPresentationParameters->MultiSampleType               = localParameters.MultiSampleType;
/*   *pPresentationParameters->MultiSampleQuality            leave alone in case wined3d set something internally */
   *pPresentationParameters->SwapEffect                    = localParameters.SwapEffect;
   *pPresentationParameters->hDeviceWindow                 = localParameters.hDeviceWindow;
   *pPresentationParameters->Windowed                      = localParameters.Windowed;
   *pPresentationParameters->EnableAutoDepthStencil        = localParameters.EnableAutoDepthStencil;
   *pPresentationParameters->AutoDepthStencilFormat        = localParameters.AutoDepthStencilFormat;
   *pPresentationParameters->Flags                         = localParameters.Flags;
   *pPresentationParameters->FullScreen_RefreshRateInHz    = localParameters.FullScreen_RefreshRateInHz;
/*   *pPresentationParameters->PresentationInterval          leave alone in case wined3d set something internally */

   return res;
}

HRESULT  WINAPI  IDirect3D8Impl_CreateDevice               (LPDIRECT3D8 iface,
                                                            UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,
                                                            DWORD BehaviourFlags, D3DPRESENT_PARAMETERS* pPresentationParameters,
                                                            IDirect3DDevice8** ppReturnedDeviceInterface) {
    IDirect3DDevice8Impl *object;
    HWND whichHWND;
    int num;
    XVisualInfo template;
    HDC hDc;
    WINED3DPRESENT_PARAMETERS localParameters;

    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    TRACE("(%p)->(Adptr:%d, DevType: %x, FocusHwnd: %p, BehFlags: %lx, PresParms: %p, RetDevInt: %p)\n", This, Adapter, DeviceType,
          hFocusWindow, BehaviourFlags, pPresentationParameters, ppReturnedDeviceInterface);

    if (Adapter >= IDirect3D8Impl_GetAdapterCount(iface)) {
        return D3DERR_INVALIDCALL;
    }

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DDevice8Impl));
    if (NULL == object) {
      return D3DERR_OUTOFVIDEOMEMORY;
    }
    object->lpVtbl = &Direct3DDevice8_Vtbl;
    object->ref = 1;
    object->direct3d8 = This;
    /** The device AddRef the direct3d8 Interface else crash in propers clients codes */
    IDirect3D8_AddRef((LPDIRECT3D8) object->direct3d8);

    /* Allocate an associated WineD3DDevice object */
    localParameters.BackBufferWidth                = &pPresentationParameters->BackBufferWidth;             
    localParameters.BackBufferHeight               = &pPresentationParameters->BackBufferHeight;
    localParameters.BackBufferFormat               = (WINED3DFORMAT *)&pPresentationParameters->BackBufferFormat;           
    localParameters.BackBufferCount                = &pPresentationParameters->BackBufferCount;            
    localParameters.MultiSampleType                = &pPresentationParameters->MultiSampleType;            
    localParameters.MultiSampleQuality             = NULL; /* New at dx9 */
    localParameters.SwapEffect                     = &pPresentationParameters->SwapEffect;                 
    localParameters.hDeviceWindow                  = &pPresentationParameters->hDeviceWindow;              
    localParameters.Windowed                       = &pPresentationParameters->Windowed;                   
    localParameters.EnableAutoDepthStencil         = &pPresentationParameters->EnableAutoDepthStencil;     
    localParameters.AutoDepthStencilFormat         = (WINED3DFORMAT *)&pPresentationParameters->AutoDepthStencilFormat;     
    localParameters.Flags                          = &pPresentationParameters->Flags;                      
    localParameters.FullScreen_RefreshRateInHz     = &pPresentationParameters->FullScreen_RefreshRateInHz; 
    localParameters.PresentationInterval           = &pPresentationParameters->FullScreen_PresentationInterval;    /* Renamed in dx9 */
    IWineD3D_CreateDevice(This->WineD3D, Adapter, DeviceType, hFocusWindow, BehaviourFlags, &localParameters, &object->WineD3DDevice, (IUnknown *)object, D3D8CB_CreateAdditionalSwapChain);

    /** use StateBlock Factory here, for creating the startup stateBlock */
    object->StateBlock = NULL;
    IDirect3DDeviceImpl_CreateStateBlock(object, D3DSBT_ALL, NULL);
    object->UpdateStateBlock = object->StateBlock;

    /* Save the creation parameters */
    object->CreateParms.AdapterOrdinal = Adapter;
    object->CreateParms.DeviceType = DeviceType;
    object->CreateParms.hFocusWindow = hFocusWindow;
    object->CreateParms.BehaviorFlags = BehaviourFlags;

    *ppReturnedDeviceInterface = (LPDIRECT3DDEVICE8) object;

    /* Initialize settings */
    object->PresentParms.BackBufferCount = 1; /* Opengl only supports one? */
    object->adapterNo = Adapter;
    object->devType = DeviceType;

    /* Initialize openGl - Note the visual is chosen as the window is created and the glcontext cannot
         use different properties after that point in time. FIXME: How to handle when requested format 
         doesn't match actual visual? Cannot choose one here - code removed as it ONLY works if the one
         it chooses is identical to the one already being used!                                        */
    /* FIXME: Handle stencil appropriately via EnableAutoDepthStencil / AutoDepthStencilFormat */

    /* Which hwnd are we using? */
    whichHWND = pPresentationParameters->hDeviceWindow;
    if (!whichHWND) {
        whichHWND = hFocusWindow;
    }
    whichHWND = GetAncestor(whichHWND, GA_ROOT);
    if ( !( object->win = (Window)GetPropA(whichHWND, "__wine_x11_whole_window") ) ) {
        ERR("Can't get drawable (window), HWND:%p doesn't have the property __wine_x11_whole_window\n", whichHWND);
        return D3DERR_NOTAVAILABLE;
    }
    object->win_handle = whichHWND;

    hDc = GetDC(whichHWND);
    object->display = get_display(hDc);

    TRACE("(%p)->(DepthStencil:(%u,%s), BackBufferFormat:(%u,%s))\n", This, 
          pPresentationParameters->AutoDepthStencilFormat, debug_d3dformat(pPresentationParameters->AutoDepthStencilFormat),
          pPresentationParameters->BackBufferFormat, debug_d3dformat(pPresentationParameters->BackBufferFormat));

    ENTER_GL();

    /* Create a context based off the properties of the existing visual */
    template.visualid = (VisualID)GetPropA(GetDesktopWindow(), "__wine_x11_visual_id");
    object->visInfo = XGetVisualInfo(object->display, VisualIDMask, &template, &num);
    if (NULL == object->visInfo) {
        ERR("cannot really get XVisual\n"); 
        LEAVE_GL();
        return D3DERR_NOTAVAILABLE;
     }
    object->glCtx = glXCreateContext(object->display, object->visInfo, NULL, GL_TRUE);
    if (NULL == object->glCtx) {
      ERR("cannot create glxContext\n"); 
      LEAVE_GL();
      return D3DERR_NOTAVAILABLE;
     }
    LEAVE_GL();

    ReleaseDC(whichHWND, hDc);
    
    if (object->glCtx == NULL) {
        ERR("Error in context creation !\n");
        return D3DERR_INVALIDCALL;
    } else {
        TRACE("Context created (HWND=%p, glContext=%p, Window=%ld, VisInfo=%p)\n",
	      whichHWND, object->glCtx, object->win, object->visInfo);
    }

    /* If not windowed, need to go fullscreen, and resize the HWND to the appropriate  */
    /*        dimensions                                                               */
    if (!pPresentationParameters->Windowed) {
#if 1
	DEVMODEW devmode;
	HDC hdc;
        int bpp = 0;
	memset(&devmode, 0, sizeof(DEVMODEW));
	devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT; 
	MultiByteToWideChar(CP_ACP, 0, "Gamers CG", -1, devmode.dmDeviceName, CCHDEVICENAME);
        hdc = CreateDCA("DISPLAY", NULL, NULL, NULL);
        bpp = GetDeviceCaps(hdc, BITSPIXEL);
        DeleteDC(hdc);
	devmode.dmBitsPerPel = (bpp >= 24) ? 32 : bpp;/*Stupid XVidMode cannot change bpp D3DFmtGetBpp(object, pPresentationParameters->BackBufferFormat);*/
	devmode.dmPelsWidth  = pPresentationParameters->BackBufferWidth;
	devmode.dmPelsHeight = pPresentationParameters->BackBufferHeight;
	ChangeDisplaySettingsExW(devmode.dmDeviceName, &devmode, object->win_handle, CDS_FULLSCREEN, NULL);
#else
        FIXME("Requested full screen support not implemented, expect windowed operation\n");
#endif

        /* Make popup window */
        SetWindowLongA(whichHWND, GWL_STYLE, WS_POPUP);
        SetWindowPos(object->win_handle, HWND_TOP, 0, 0, 
		     pPresentationParameters->BackBufferWidth,
                     pPresentationParameters->BackBufferHeight, SWP_SHOWWINDOW | SWP_FRAMECHANGED);
    }

    TRACE("Creating back buffer\n");
    /* MSDN: If Windowed is TRUE and either of the BackBufferWidth/Height values is zero,
       then the corresponding dimension of the client area of the hDeviceWindow
       (or the focus window, if hDeviceWindow is NULL) is taken. */
    if (pPresentationParameters->Windowed && ((pPresentationParameters->BackBufferWidth  == 0) ||
                                              (pPresentationParameters->BackBufferHeight == 0))) {
        RECT Rect;

        GetClientRect(whichHWND, &Rect);

        if (pPresentationParameters->BackBufferWidth == 0) {
           pPresentationParameters->BackBufferWidth = Rect.right;
           TRACE("Updating width to %d\n", pPresentationParameters->BackBufferWidth);
        }
        if (pPresentationParameters->BackBufferHeight == 0) {
           pPresentationParameters->BackBufferHeight = Rect.bottom;
           TRACE("Updating height to %d\n", pPresentationParameters->BackBufferHeight);
        }
    }

    /* Save the presentation parms now filled in correctly */
    memcpy(&object->PresentParms, pPresentationParameters, sizeof(D3DPRESENT_PARAMETERS));


    IDirect3DDevice8Impl_CreateRenderTarget((LPDIRECT3DDEVICE8) object,
                                            pPresentationParameters->BackBufferWidth,
                                            pPresentationParameters->BackBufferHeight,
                                            pPresentationParameters->BackBufferFormat,
					    pPresentationParameters->MultiSampleType,
					    TRUE,
                                            (LPDIRECT3DSURFACE8*) &object->frontBuffer);

    IDirect3DDevice8Impl_CreateRenderTarget((LPDIRECT3DDEVICE8) object,
                                            pPresentationParameters->BackBufferWidth,
                                            pPresentationParameters->BackBufferHeight,
                                            pPresentationParameters->BackBufferFormat,
					    pPresentationParameters->MultiSampleType,
					    TRUE,
                                            (LPDIRECT3DSURFACE8*) &object->backBuffer);

    if (pPresentationParameters->EnableAutoDepthStencil) {
       IDirect3DDevice8Impl_CreateDepthStencilSurface((LPDIRECT3DDEVICE8) object,
	  					      pPresentationParameters->BackBufferWidth,
						      pPresentationParameters->BackBufferHeight,
						      pPresentationParameters->AutoDepthStencilFormat,
						      D3DMULTISAMPLE_NONE,
						      (LPDIRECT3DSURFACE8*) &object->depthStencilBuffer);
    } else {
      object->depthStencilBuffer = NULL;
    }
    TRACE("FrontBuf @ %p, BackBuf @ %p, DepthStencil @ %p\n",object->frontBuffer, object->backBuffer, object->depthStencilBuffer);

    /* init the default renderTarget management */
    object->drawable = object->win;
    object->render_ctx = object->glCtx;
    object->renderTarget = object->backBuffer;
    IDirect3DSurface8Impl_AddRef((LPDIRECT3DSURFACE8) object->renderTarget);
    object->stencilBufferTarget = object->depthStencilBuffer;
    if (NULL != object->stencilBufferTarget) {
      IDirect3DSurface8Impl_AddRef((LPDIRECT3DSURFACE8) object->stencilBufferTarget);
    }

    ENTER_GL();

    if (glXMakeCurrent(object->display, object->win, object->glCtx) == False) {
      ERR("Error in setting current context (context %p drawable %ld)!\n", object->glCtx, object->win);
    }
    checkGLcall("glXMakeCurrent");

    /* Clear the screen */
    glClearColor(1.0, 0.0, 0.0, 0.0);
    checkGLcall("glClearColor");
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

    /* Setup all the devices defaults */
    IDirect3DDeviceImpl_InitStartupStateBlock(object);

    LEAVE_GL();

    { /* Set a default viewport */
       D3DVIEWPORT8 vp;
       vp.X      = 0;
       vp.Y      = 0;
       vp.Width  = pPresentationParameters->BackBufferWidth;
       vp.Height = pPresentationParameters->BackBufferHeight;
       vp.MinZ   = 0.0f;
       vp.MaxZ   = 1.0f;
       IDirect3DDevice8Impl_SetViewport((LPDIRECT3DDEVICE8) object, &vp);
    }

    /* Initialize the current view state */
    object->modelview_valid = 1;
    object->proj_valid = 0;
    object->view_ident = 1;
    object->last_was_rhw = 0;
    glGetIntegerv(GL_MAX_LIGHTS, &object->maxConcurrentLights);
    TRACE("(%p,%d) All defaults now set up, leaving CreateDevice with %p\n", This, Adapter, object);

    /* Clear the screen */
    IDirect3DDevice8Impl_Clear((LPDIRECT3DDEVICE8) object, 0, NULL, D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER|D3DCLEAR_TARGET, 0x00, 1.0, 0);

    return D3D_OK;
}

const IDirect3D8Vtbl Direct3D8_Vtbl =
{
    IDirect3D8Impl_QueryInterface,
    IDirect3D8Impl_AddRef,
    IDirect3D8Impl_Release,
    IDirect3D8Impl_RegisterSoftwareDevice,
    IDirect3D8Impl_GetAdapterCount,
    IDirect3D8Impl_GetAdapterIdentifier,
    IDirect3D8Impl_GetAdapterModeCount,
    IDirect3D8Impl_EnumAdapterModes,
    IDirect3D8Impl_GetAdapterDisplayMode,
    IDirect3D8Impl_CheckDeviceType,
    IDirect3D8Impl_CheckDeviceFormat,
    IDirect3D8Impl_CheckDeviceMultiSampleType,
    IDirect3D8Impl_CheckDepthStencilMatch,
    IDirect3D8Impl_GetDeviceCaps,
    IDirect3D8Impl_GetAdapterMonitor,
    IDirect3D8Impl_CreateDevice
};
