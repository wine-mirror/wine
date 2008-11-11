/*
 * Copyright 2005 Jacek Caban
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

#include "wine/debug.h"
#include "shdocvw.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

#define VIEWOBJ_THIS(iface) DEFINE_THIS(WebBrowser, ViewObject, iface)

static HRESULT WINAPI ViewObject_QueryInterface(IViewObjectEx *iface, REFIID riid, void **ppv)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    return IWebBrowser2_QueryInterface(WEBBROWSER(This), riid, ppv);
}

static ULONG WINAPI ViewObject_AddRef(IViewObjectEx *iface)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    return IWebBrowser2_AddRef(WEBBROWSER(This));
}

static ULONG WINAPI ViewObject_Release(IViewObjectEx *iface)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    return IWebBrowser2_Release(WEBBROWSER(This));
}

static HRESULT WINAPI ViewObject_Draw(IViewObjectEx *iface, DWORD dwDrawAspect,
        LONG lindex, void *pvAspect, DVTARGETDEVICE *ptd, HDC hdcTargetDev,
        HDC hdcDraw, LPCRECTL lprcBounds, LPCRECTL lprcWBounds,
        BOOL (STDMETHODCALLTYPE *pfnContinue)(ULONG_PTR),
        ULONG_PTR dwContinue)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%d %d %p %p %p %p %p %p %p %08lx)\n", This, dwDrawAspect, lindex,
            pvAspect, ptd, hdcTargetDev, hdcDraw, lprcBounds, lprcWBounds, pfnContinue,
            dwContinue);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_GetColorSet(IViewObjectEx *iface, DWORD dwAspect,
        LONG lindex, void *pvAspect, DVTARGETDEVICE *ptd, HDC hicTargetDev,
        LOGPALETTE **ppColorSet)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%d %d %p %p %p %p)\n", This, dwAspect, lindex, pvAspect, ptd,
            hicTargetDev, ppColorSet);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_Freeze(IViewObjectEx *iface, DWORD dwDrawAspect, LONG lindex,
                                        void *pvAspect, DWORD *pdwFreeze)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%d %d %p %p)\n", This, dwDrawAspect, lindex, pvAspect, pdwFreeze);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_Unfreeze(IViewObjectEx *iface, DWORD dwFreeze)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%d)\n", This, dwFreeze);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_SetAdvise(IViewObjectEx *iface, DWORD aspects, DWORD advf,
        IAdviseSink *pAdvSink)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%d %08x %p)\n", This, aspects, advf, pAdvSink);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_GetAdvise(IViewObjectEx *iface, DWORD *pAspects,
        DWORD *pAdvf, IAdviseSink **ppAdvSink)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%p %p %p)\n", This, pAspects, pAdvf, ppAdvSink);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_GetExtent(IViewObjectEx *iface, DWORD dwAspect, LONG lindex,
        DVTARGETDEVICE *ptd, LPSIZEL lpsizel)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%d %d %p %p)\n", This, dwAspect, lindex, ptd, lpsizel);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_GetRect(IViewObjectEx *iface, DWORD dwAspect, LPRECTL pRect)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%d %p)\n", This, dwAspect, pRect);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_GetViewStatus(IViewObjectEx *iface, DWORD *pdwStatus)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pdwStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_QueryHitPoint(IViewObjectEx *iface, DWORD dwAspect, LPCRECT pRectBounds,
        POINT ptlLoc, LONG lCloseHint, DWORD *pHitResult)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%d %p %p %d %p)\n", This, dwAspect, pRectBounds, pRectBounds, lCloseHint, pHitResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_QueryHitRect(IViewObjectEx * iface, DWORD dwAspect, LPCRECT pRectBounds,
        LPCRECT pRectLoc, LONG lCloseHint, DWORD *pHitResult)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%d %p %p %p %d %p)\n", This, dwAspect, pRectBounds, pRectLoc, pRectBounds, lCloseHint, pHitResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_GetNaturalExtent(IViewObjectEx *iface, DWORD dwAspect, LONG lindex,
        DVTARGETDEVICE *ptd, HDC hicTargetDev, DVEXTENTINFO *pExtentInfo, LPSIZEL pSizel)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%d %d %p %p %p)\n", This, dwAspect, lindex, ptd, pExtentInfo, pSizel);
    return E_NOTIMPL;
}

static const IViewObjectExVtbl ViewObjectVtbl = {
    ViewObject_QueryInterface,
    ViewObject_AddRef,
    ViewObject_Release,
    ViewObject_Draw,
    ViewObject_GetColorSet,
    ViewObject_Freeze,
    ViewObject_Unfreeze,
    ViewObject_SetAdvise,
    ViewObject_GetAdvise,
    ViewObject_GetExtent,
    ViewObject_GetRect,
    ViewObject_GetViewStatus,
    ViewObject_QueryHitPoint,
    ViewObject_QueryHitRect,
    ViewObject_GetNaturalExtent
};

#undef VIEWOBJ_THIS

void WebBrowser_ViewObject_Init(WebBrowser *This)
{
    This->lpViewObjectVtbl = &ViewObjectVtbl;
}
