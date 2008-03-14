/*
 * Copyright (C) 2008 Tony Wasserka
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
 *
 */

#include "config.h"
#include "wine/port.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "d3dx9_36_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

static HRESULT WINAPI ID3DXFontImpl_QueryInterface(LPD3DXFONT iface, REFIID riid, LPVOID *object)
{
    ID3DXFontImpl *This=(ID3DXFontImpl*)iface;

    TRACE("(%p): QueryInterface from %s\n", This, debugstr_guid(riid));
    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_ID3DXFont)) {
        IUnknown_AddRef(iface);
        *object=This;
        return S_OK;
    }
    WARN("(%p)->(%s, %p): not found\n", iface, debugstr_guid(riid), *object);
    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXFontImpl_AddRef(LPD3DXFONT iface)
{
    ID3DXFontImpl *This=(ID3DXFontImpl*)iface;
    ULONG ref=InterlockedIncrement(&This->ref);
    TRACE("(%p): AddRef from %d\n", This, ref-1);
    return ref;
}

static ULONG WINAPI ID3DXFontImpl_Release(LPD3DXFONT iface)
{
    ID3DXFontImpl *This=(ID3DXFontImpl*)iface;
    ULONG ref=InterlockedDecrement(&This->ref);
    TRACE("(%p): ReleaseRef to %d\n", This, ref);

    if(ref==0) {
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static HRESULT WINAPI ID3DXFontImpl_GetDevice(LPD3DXFONT iface, LPDIRECT3DDEVICE9 *device)
{
    ID3DXFontImpl *This=(ID3DXFontImpl*)iface;
    FIXME("(%p): stub\n", This);
    return D3D_OK;
}

static HRESULT WINAPI ID3DXFontImpl_GetDescA(LPD3DXFONT iface, D3DXFONT_DESCA *desc)
{
    ID3DXFontImpl *This=(ID3DXFontImpl*)iface;
    FIXME("(%p): stub\n", This);
    return D3D_OK;
}

static HRESULT WINAPI ID3DXFontImpl_GetDescW(LPD3DXFONT iface, D3DXFONT_DESCW *desc)
{
    ID3DXFontImpl *This=(ID3DXFontImpl*)iface;
    FIXME("(%p): stub\n", This);
    return D3D_OK;
}

static BOOL WINAPI ID3DXFontImpl_GetTextMetricsA(LPD3DXFONT iface, TEXTMETRICA *metrics)
{
    ID3DXFontImpl *This=(ID3DXFontImpl*)iface;
    FIXME("(%p): stub\n", This);
    return FALSE;
}

static BOOL WINAPI ID3DXFontImpl_GetTextMetricsW(LPD3DXFONT iface, TEXTMETRICW *metrics)
{
    ID3DXFontImpl *This=(ID3DXFontImpl*)iface;
    FIXME("(%p): stub\n", This);
    return FALSE;
}

static HDC WINAPI ID3DXFontImpl_GetDC(LPD3DXFONT iface)
{
    ID3DXFontImpl *This=(ID3DXFontImpl*)iface;
    FIXME("(%p): stub\n", This);
    return NULL;
}

static HRESULT WINAPI ID3DXFontImpl_GetGlyphData(LPD3DXFONT iface, UINT glyph, LPDIRECT3DTEXTURE9 *texture, RECT *blackbox, POINT *cellinc)
{
    ID3DXFontImpl *This=(ID3DXFontImpl*)iface;
    FIXME("(%p): stub\n", This);
    return D3D_OK;
}

static HRESULT WINAPI ID3DXFontImpl_PreloadCharacters(LPD3DXFONT iface, UINT first, UINT last)
{
    ID3DXFontImpl *This=(ID3DXFontImpl*)iface;
    FIXME("(%p): stub\n", This);
    return D3D_OK;
}

static HRESULT WINAPI ID3DXFontImpl_PreloadGlyphs(LPD3DXFONT iface, UINT first, UINT last)
{
    ID3DXFontImpl *This=(ID3DXFontImpl*)iface;
    FIXME("(%p): stub\n", This);
    return D3D_OK;
}

static HRESULT WINAPI ID3DXFontImpl_PreloadTextA(LPD3DXFONT iface, LPCSTR string, INT count)
{
    ID3DXFontImpl *This=(ID3DXFontImpl*)iface;
    FIXME("(%p): stub\n", This);
    return D3D_OK;
}

static HRESULT WINAPI ID3DXFontImpl_PreloadTextW(LPD3DXFONT iface, LPCWSTR string, INT count)
{
    ID3DXFontImpl *This=(ID3DXFontImpl*)iface;
    FIXME("(%p): stub\n", This);
    return D3D_OK;
}

static INT WINAPI ID3DXFontImpl_DrawTextA(LPD3DXFONT iface, LPD3DXSPRITE sprite, LPCSTR string, INT count, LPRECT rect, DWORD format, D3DCOLOR color)
{
    ID3DXFontImpl *This=(ID3DXFontImpl*)iface;
    FIXME("(%p): stub\n", This);
    return 1;
}

static INT WINAPI ID3DXFontImpl_DrawTextW(LPD3DXFONT iface, LPD3DXSPRITE sprite, LPCWSTR string, INT count, LPRECT rect, DWORD format, D3DCOLOR color)
{
    ID3DXFontImpl *This=(ID3DXFontImpl*)iface;
    FIXME("(%p): stub\n", This);
    return 1;
}

static HRESULT WINAPI ID3DXFontImpl_OnLostDevice(LPD3DXFONT iface)
{
    ID3DXFontImpl *This=(ID3DXFontImpl*)iface;
    FIXME("(%p): stub\n", This);
    return D3D_OK;
}

static HRESULT WINAPI ID3DXFontImpl_OnResetDevice(LPD3DXFONT iface)
{
    ID3DXFontImpl *This=(ID3DXFontImpl*)iface;
    FIXME("(%p): stub\n", This);
    return D3D_OK;
}

static const ID3DXFontVtbl D3DXFont_Vtbl =
{
    /*** IUnknown methods ***/
    ID3DXFontImpl_QueryInterface,
    ID3DXFontImpl_AddRef,
    ID3DXFontImpl_Release,
    /*** ID3DXFont methods ***/
    ID3DXFontImpl_GetDevice,
    ID3DXFontImpl_GetDescA,
    ID3DXFontImpl_GetDescW,
    ID3DXFontImpl_GetTextMetricsA,
    ID3DXFontImpl_GetTextMetricsW,
    ID3DXFontImpl_GetDC,
    ID3DXFontImpl_GetGlyphData,
    ID3DXFontImpl_PreloadCharacters,
    ID3DXFontImpl_PreloadGlyphs,
    ID3DXFontImpl_PreloadTextA,
    ID3DXFontImpl_PreloadTextW,
    ID3DXFontImpl_DrawTextA,
    ID3DXFontImpl_DrawTextW,
    ID3DXFontImpl_OnLostDevice,
    ID3DXFontImpl_OnResetDevice
};

HRESULT WINAPI D3DXCreateFontA(LPDIRECT3DDEVICE9 device, INT height, UINT width, UINT weight, UINT miplevels, BOOL italic, DWORD charset,
                               DWORD precision, DWORD quality, DWORD pitchandfamily, LPCSTR facename, LPD3DXFONT *font)
{
    D3DXFONT_DESCA desc;

    if(!facename) return D3DXERR_INVALIDDATA;

    desc.Height=height;
    desc.Width=width;
    desc.Weight=weight;
    desc.MipLevels=miplevels;
    desc.Italic=italic;
    desc.CharSet=charset;
    desc.OutputPrecision=precision;
    desc.Quality=quality;
    desc.PitchAndFamily=pitchandfamily;
    lstrcpyA(desc.FaceName, facename);

    return D3DXCreateFontIndirectA(device, &desc, font);
}

HRESULT WINAPI D3DXCreateFontW(LPDIRECT3DDEVICE9 device, INT height, UINT width, UINT weight, UINT miplevels, BOOL italic, DWORD charset,
                               DWORD precision, DWORD quality, DWORD pitchandfamily, LPCWSTR facename, LPD3DXFONT *font)
{
    D3DXFONT_DESCW desc;

    if(!facename) return D3DXERR_INVALIDDATA;

    desc.Height=height;
    desc.Width=width;
    desc.Weight=weight;
    desc.MipLevels=miplevels;
    desc.Italic=italic;
    desc.CharSet=charset;
    desc.OutputPrecision=precision;
    desc.Quality=quality;
    desc.PitchAndFamily=pitchandfamily;
    strcpyW(desc.FaceName, facename);

    return D3DXCreateFontIndirectW(device, &desc, font);
}

HRESULT WINAPI D3DXCreateFontIndirectA(LPDIRECT3DDEVICE9 device, CONST D3DXFONT_DESCA *desc, LPD3DXFONT *font)
{
    D3DXFONT_DESCW widedesc;

    if(!desc) return D3DERR_INVALIDCALL;
    if(!desc->FaceName) return D3DERR_INVALIDCALL;

    /* Copy everything but the last structure member. This requires the
       two D3DXFONT_DESC structures to be equal until the FaceName member */
    memcpy(&widedesc, desc, FIELD_OFFSET(D3DXFONT_DESCA, FaceName));
    MultiByteToWideChar(CP_ACP, 0, desc->FaceName, -1,
                        widedesc.FaceName, sizeof(widedesc.FaceName)/sizeof(WCHAR));
    return D3DXCreateFontIndirectW(device, &widedesc, font);
}

HRESULT WINAPI D3DXCreateFontIndirectW(LPDIRECT3DDEVICE9 device, CONST D3DXFONT_DESCW *desc, LPD3DXFONT *font)
{
    ID3DXFontImpl *object;

    FIXME("stub\n");

    if(!desc) return D3DERR_INVALIDCALL;

    object=HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ID3DXFontImpl));
    if(object==NULL) {
        *font=NULL;
        return E_OUTOFMEMORY;
    }
    object->lpVtbl=&D3DXFont_Vtbl;
    object->ref=1;

    *font=(LPD3DXFONT)object;

    return D3D_OK;
}
