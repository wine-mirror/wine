/*
 * IWineD3D implementation
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

/* Compile time diagnostics: */

/* Uncomment this to force only a single display mode to be exposed: */
/*#define DEBUG_SINGLE_MODE*/


#include "config.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_caps);

UINT     WINAPI  IWineD3DImpl_GetAdapterCount (IWineD3D *iface) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;

    /* FIXME: Set to one for now to imply the display */
    TRACE_(d3d_caps)("(%p): Mostly stub, only returns primary display\n", This);
    return 1;
}

HRESULT  WINAPI  IWineD3DImpl_RegisterSoftwareDevice(IWineD3D *iface, void* pInitializeFunction) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;
    FIXME("(%p)->(%p): stub\n", This, pInitializeFunction);
    return D3D_OK;
}

HMONITOR WINAPI  IWineD3DImpl_GetAdapterMonitor(IWineD3D *iface, UINT Adapter) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;
    FIXME_(d3d_caps)("(%p)->(Adptr:%d)\n", This, Adapter);
    if (Adapter >= IWineD3DImpl_GetAdapterCount(iface)) {
        return NULL;
    }
    return D3D_OK;
}

/* FIXME: GetAdapterModeCount and EnumAdapterModes currently only returns modes 
     of the same bpp but different resolutions                                  */

/* Note: dx9 supplies a format. Calls from d3d8 supply D3DFMT_UNKNOWN */
UINT     WINAPI  IWineD3DImpl_GetAdapterModeCount(IWineD3D *iface, UINT Adapter, D3DFORMAT Format) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;
    TRACE_(d3d_caps)("(%p}->(Adapter: %d, Format: %s)\n", This, Adapter, debug_d3dformat(Format));

    if (Adapter >= IWineD3D_GetAdapterCount(iface)) {
        return 0;
    }

    if (Adapter == 0) { /* Display */
        int i = 0;
        int j = 0;
#if !defined( DEBUG_SINGLE_MODE )
        DEVMODEW DevModeW;

        /* Work out the current screen bpp */
        HDC hdc = CreateDCA("DISPLAY", NULL, NULL, NULL);
        int bpp = GetDeviceCaps(hdc, BITSPIXEL);
        DeleteDC(hdc);

        while (EnumDisplaySettingsExW(NULL, j, &DevModeW, 0)) {
            j++;
            switch (Format)
            {
            case D3DFMT_UNKNOWN: 
                   i++; 
                   break;
            case D3DFMT_X8R8G8B8:   
            case D3DFMT_A8R8G8B8:   
                   if (min(DevModeW.dmBitsPerPel, bpp) == 32) i++; 
                   break;
            case D3DFMT_X1R5G5B5:
            case D3DFMT_A1R5G5B5:
            case D3DFMT_R5G6B5:
                   if (min(DevModeW.dmBitsPerPel, bpp) == 16) i++; 
                   break;
            default:
                   /* Skip other modes as they do not match requested format */
                   break;
            }
        }
#else
        i = 1;
        j = 1;
#endif
        TRACE_(d3d_caps)("(%p}->(Adapter: %d) => %d (out of %d)\n", This, Adapter, i, j);
        return i;
    } else {
        FIXME_(d3d_caps)("Adapter not primary display\n");
    }
    return 0;
}

/* Note: dx9 supplies a format. Calls from d3d8 supply D3DFMT_UNKNOWN */
HRESULT WINAPI IWineD3DImpl_EnumAdapterModes(IWineD3D *iface, UINT Adapter, D3DFORMAT Format, UINT Mode, D3DDISPLAYMODE* pMode) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;
    TRACE_(d3d_caps)("(%p}->(Adapter:%d, mode:%d, pMode:%p, format:%s)\n", This, Adapter, Mode, pMode, debug_d3dformat(Format));

    /* Validate the parameters as much as possible */
    if (NULL == pMode || 
        Adapter >= IWineD3DImpl_GetAdapterCount(iface) ||
        Mode    >= IWineD3DImpl_GetAdapterModeCount(iface, Adapter, Format)) {
        return D3DERR_INVALIDCALL;
    }

    if (Adapter == 0) { /* Display */
#if !defined( DEBUG_SINGLE_MODE )
        DEVMODEW DevModeW;
        int ModeIdx = 0;
            
        /* Work out the current screen bpp */
        HDC hdc = CreateDCA("DISPLAY", NULL, NULL, NULL);
        int bpp = GetDeviceCaps(hdc, BITSPIXEL);
        DeleteDC(hdc);

        /* If we are filtering to a specific format, then need to skip all unrelated
           modes, but if mode is irrelevant, then we can use the index directly      */
        if (Format == D3DFMT_UNKNOWN) 
        {
            ModeIdx = Mode;
        } else {
            int i = 0;
            int j = 0;
            DEVMODEW DevModeWtmp;
            
            
            while ((Mode+1) < i && EnumDisplaySettingsExW(NULL, j, &DevModeWtmp, 0)) {
                j++;
                switch (Format)
                {
                case D3DFMT_UNKNOWN: 
                       i++; 
                       break;
                case D3DFMT_X8R8G8B8:   
                case D3DFMT_A8R8G8B8:   
                       if (min(DevModeWtmp.dmBitsPerPel, bpp) == 32) i++; 
                       break;
                case D3DFMT_X1R5G5B5:
                case D3DFMT_A1R5G5B5:
                case D3DFMT_R5G6B5:
                       if (min(DevModeWtmp.dmBitsPerPel, bpp) == 16) i++; 
                       break;
                default:
                       /* Skip other modes as they do not match requested format */
                       break;
                }
            }
            ModeIdx = j;
        }

        /* Now get the display mode via the calculated index */
        if (EnumDisplaySettingsExW(NULL, ModeIdx, &DevModeW, 0)) 
        {
            pMode->Width        = DevModeW.dmPelsWidth;
            pMode->Height       = DevModeW.dmPelsHeight;
            bpp                 = min(DevModeW.dmBitsPerPel, bpp);
            pMode->RefreshRate  = D3DADAPTER_DEFAULT;
            if (DevModeW.dmFields&DM_DISPLAYFREQUENCY)
            {
                pMode->RefreshRate = DevModeW.dmDisplayFrequency;
            }

            if (Format == D3DFMT_UNKNOWN)
            {
                switch (bpp) {
                case  8: pMode->Format = D3DFMT_R3G3B2;   break;
                case 16: pMode->Format = D3DFMT_R5G6B5;   break;
                case 24: /* pMode->Format = D3DFMT_R5G6B5;   break;*/ /* Make 24bit appear as 32 bit */
                case 32: pMode->Format = D3DFMT_A8R8G8B8; break;
                default: pMode->Format = D3DFMT_UNKNOWN;
                }
            } else {
                pMode->Format = Format;
            }
        }
        else
        {
            TRACE_(d3d_caps)("Requested mode out of range %d\n", Mode);
            return D3DERR_INVALIDCALL;
        }

#else
        /* Return one setting of the format requested */
        if (Mode > 0) return D3DERR_INVALIDCALL;
        pMode->Width        = 800;
        pMode->Height       = 600;
        pMode->RefreshRate  = D3DADAPTER_DEFAULT;
        pMode->Format       = (Format==D3DFMT_UNKNOWN)?D3DFMT_A8R8G8B8:Format;
        bpp = 32;
#endif
        TRACE_(d3d_caps)("W %d H %d rr %d fmt (%x - %s) bpp %u\n", pMode->Width, pMode->Height, 
                 pMode->RefreshRate, pMode->Format, debug_d3dformat(pMode->Format), bpp);

    } else {
        FIXME_(d3d_caps)("Adapter not primary display\n");
    }

    return D3D_OK;
}

HRESULT WINAPI IWineD3DImpl_GetAdapterDisplayMode(IWineD3D *iface, UINT Adapter, D3DDISPLAYMODE* pMode) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;
    TRACE_(d3d_caps)("(%p}->(Adapter: %d, pMode: %p)\n", This, Adapter, pMode);

    if (NULL == pMode || 
        Adapter >= IWineD3D_GetAdapterCount(iface)) {
        return D3DERR_INVALIDCALL;
    }

    if (Adapter == 0) { /* Display */
        int bpp = 0;
        DEVMODEW DevModeW;

        EnumDisplaySettingsExW(NULL, (DWORD)-1, &DevModeW, 0);
        pMode->Width        = DevModeW.dmPelsWidth;
        pMode->Height       = DevModeW.dmPelsHeight;
        bpp                 = DevModeW.dmBitsPerPel;
        pMode->RefreshRate  = D3DADAPTER_DEFAULT;
        if (DevModeW.dmFields&DM_DISPLAYFREQUENCY)
        {
            pMode->RefreshRate = DevModeW.dmDisplayFrequency;
        }

        switch (bpp) {
        case  8: pMode->Format       = D3DFMT_R3G3B2;   break;
        case 16: pMode->Format       = D3DFMT_R5G6B5;   break;
        case 24: /*pMode->Format       = D3DFMT_R5G6B5;   break;*/ /* Make 24bit appear as 32 bit */
        case 32: pMode->Format       = D3DFMT_A8R8G8B8; break;
        default: pMode->Format       = D3DFMT_UNKNOWN;
        }

    } else {
        FIXME_(d3d_caps)("Adapter not primary display\n");
    }

    TRACE_(d3d_caps)("returning w:%d, h:%d, ref:%d, fmt:%s\n", pMode->Width,
          pMode->Height, pMode->RefreshRate, debug_d3dformat(pMode->Format));
    return D3D_OK;
}

/* IUnknown parts follow: */
HRESULT WINAPI IWineD3DImpl_QueryInterface(IWineD3D *iface,REFIID riid,LPVOID *ppobj)
{
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DImpl_AddRef(IWineD3D *iface) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;
    FIXME("(%p) : AddRef increasing from %ld\n", This, This->ref);
    return InterlockedIncrement(&This->ref);
}

ULONG WINAPI IWineD3DImpl_Release(IWineD3D *iface) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %ld\n", This, This->ref);
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) HeapFree(GetProcessHeap(), 0, This);
    return ref;
}

/* VTbl definition */
IWineD3DVtbl IWineD3D_Vtbl =
{
    IWineD3DImpl_QueryInterface,
    IWineD3DImpl_AddRef,
    IWineD3DImpl_Release,
    IWineD3DImpl_GetAdapterCount,
    IWineD3DImpl_RegisterSoftwareDevice,
    IWineD3DImpl_GetAdapterMonitor,
    IWineD3DImpl_GetAdapterModeCount,
    IWineD3DImpl_EnumAdapterModes,
    IWineD3DImpl_GetAdapterDisplayMode
};
