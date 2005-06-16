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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "ole2.h"
#include "docobj.h"

#include "mshtml.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static const WCHAR wszInternetExplorer_Server[] =
    {'I','n','t','e','r','n','e','t',' ','E','x','p','l','o','r','e','r','_','S','e','r','v','e','r',0};
static const WCHAR wszHTML_Document[] =
    {'H','T','M','L',' ','D','o','c','u','m','e','n','t',0};

static ATOM serverwnd_class = 0;

static void paint_disabled(HWND hwnd) {
    HDC hdc;
    PAINTSTRUCT ps;
    HBRUSH brush;
    RECT rect;
    HFONT font;

    font = CreateFontA(25,0,0,0,400,0,0,0,ANSI_CHARSET,0,0,DEFAULT_QUALITY,DEFAULT_PITCH,NULL);
    brush = CreateSolidBrush(RGB(255,255,255));
    GetClientRect(hwnd, &rect);

    hdc = BeginPaint(hwnd, &ps);
    SelectObject(hdc, font);
    SelectObject(hdc, brush);
    Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
    DrawTextA(hdc, "HTML rendering is currently disabled.",-1, &rect,
            DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    EndPaint(hwnd, &ps);

    DeleteObject(font);
    DeleteObject(brush);
}

static LRESULT WINAPI serverwnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if(msg == WM_PAINT)
        paint_disabled(hwnd);
        
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void register_serverwnd_class(void)
{
    static WNDCLASSEXW wndclass = {
        sizeof(WNDCLASSEXW),
        CS_DBLCLKS,
        serverwnd_proc,
        0, 0, NULL, NULL, NULL, NULL, NULL,
        wszInternetExplorer_Server,
        NULL,
    };
    wndclass.hInstance = hInst;
    serverwnd_class = RegisterClassExW(&wndclass);
}


/**********************************************************
 * IOleDocumentView implementation
 */

#define DOCVIEW_THIS \
        HTMLDocument* const This=(HTMLDocument*)((char*)(iface)-offsetof(HTMLDocument,lpOleDocumentViewVtbl));

static HRESULT WINAPI OleDocumentView_QueryInterface(IOleDocumentView *iface, REFIID riid, void **ppvObject)
{
    DOCVIEW_THIS
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppvObject);
}

static ULONG WINAPI OleDocumentView_AddRef(IOleDocumentView *iface)
{
    DOCVIEW_THIS
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI OleDocumentView_Release(IOleDocumentView *iface)
{
    DOCVIEW_THIS
    return IHTMLDocument2_Release(HTMLDOC(This));
}

static HRESULT WINAPI OleDocumentView_SetInPlaceSite(IOleDocumentView *iface, IOleInPlaceSite *pIPSite)
{
    DOCVIEW_THIS
    TRACE("(%p)->(%p)\n", This, pIPSite);

    if(!pIPSite)
        return E_INVALIDARG;

    if(pIPSite)
        IOleInPlaceSite_AddRef(pIPSite);

    if(This->ipsite)
        IOleInPlaceSite_Release(This->ipsite);

    This->ipsite = pIPSite;
    return S_OK;
}

static HRESULT WINAPI OleDocumentView_GetInPlaceSite(IOleDocumentView *iface, IOleInPlaceSite **ppIPSite)
{
    DOCVIEW_THIS
    TRACE("(%p)->(%p)\n", This, ppIPSite);

    if(!ppIPSite)
        return E_INVALIDARG;

    if(This->ipsite)
        IOleInPlaceSite_AddRef(This->ipsite);

    *ppIPSite = This->ipsite;
    return S_OK;
}

static HRESULT WINAPI OleDocumentView_GetDocument(IOleDocumentView *iface, IUnknown **ppunk)
{
    DOCVIEW_THIS
    TRACE("(%p)->(%p)\n", This, ppunk);

    if(!ppunk)
        return E_INVALIDARG;

    IHTMLDocument2_AddRef(HTMLDOC(This));
    *ppunk = (IUnknown*)HTMLDOC(This);
    return S_OK;
}

static HRESULT WINAPI OleDocumentView_SetRect(IOleDocumentView *iface, LPRECT prcView)
{
    DOCVIEW_THIS
    RECT rect;

    TRACE("(%p)->(%p)\n", This, prcView);

    if(!prcView)
        return E_INVALIDARG;

    if(This->hwnd) {
        GetClientRect(This->hwnd, &rect);
        if(memcmp(prcView, &rect, sizeof(RECT))) {
            InvalidateRect(This->hwnd,NULL,TRUE);
            SetWindowPos(This->hwnd, NULL, prcView->left, prcView->top, prcView->right,
                    prcView->bottom, SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
    
    return S_OK;
}

static HRESULT WINAPI OleDocumentView_GetRect(IOleDocumentView *iface, LPRECT prcView)
{
    DOCVIEW_THIS

    TRACE("(%p)->(%p)\n", This, prcView);

    if(!prcView)
        return E_INVALIDARG;

    GetClientRect(This->hwnd, prcView);
    return S_OK;
}

static HRESULT WINAPI OleDocumentView_SetRectComplex(IOleDocumentView *iface, LPRECT prcView,
                        LPRECT prcHScroll, LPRECT prcVScroll, LPRECT prcSizeBox)
{
    DOCVIEW_THIS
    FIXME("(%p)->(%p %p %p %p)\n", This, prcView, prcHScroll, prcVScroll, prcSizeBox);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleDocumentView_Show(IOleDocumentView *iface, BOOL fShow)
{
    DOCVIEW_THIS
    TRACE("(%p)->(%x)\n", This, fShow);

    if(This->hwnd)
        ShowWindow(This->hwnd, fShow);

    return S_OK;
}

static HRESULT WINAPI OleDocumentView_UIActivate(IOleDocumentView *iface, BOOL fUIActivate)
{
    DOCVIEW_THIS
    HRESULT hres;
    IOleInPlaceUIWindow *pIPWnd;
    IOleInPlaceFrame *pIPFrame;
    RECT posrect, cliprect;
    OLEINPLACEFRAMEINFO frameinfo;
    HWND parent_hwnd, hwnd;

    TRACE("(%p)->(%x)\n", This, fUIActivate);

    if(!This->ipsite) {
        FIXME("This->ipsite = NULL\n");
        return E_FAIL;
    }

    if(fUIActivate) {
        if(This->hwnd)
            return S_OK;
        if(!serverwnd_class)
            register_serverwnd_class();

        hres = IOleInPlaceSite_CanInPlaceActivate(This->ipsite);
        if(hres != S_OK) {
            WARN("CanInPlaceActivate returned: %08lx\n", hres);
            return FAILED(hres) ? hres : E_FAIL;
        }

        hres = IOleInPlaceSite_GetWindowContext(This->ipsite, &pIPFrame, &pIPWnd, &posrect, &cliprect, &frameinfo);
        if(FAILED(hres)) {
            WARN("GetWindowContext failed: %08lx\n", hres);
            return hres;
        }
        if(pIPWnd)
            IOleInPlaceUIWindow_Release(pIPWnd);
        TRACE("got window context: %p %p {%ld %ld %ld %ld} {%ld %ld %ld %ld} {%d %x %p %p %d}\n",
                pIPFrame, pIPWnd, posrect.left, posrect.top, posrect.right, posrect.bottom,
                cliprect.left, cliprect.top, cliprect.right, cliprect.bottom,
                frameinfo.cb, frameinfo.fMDIApp, frameinfo.hwndFrame, frameinfo.haccel, frameinfo.cAccelEntries);

        hres = IOleInPlaceSite_GetWindow(This->ipsite, &parent_hwnd);
        if(FAILED(hres)) {
            WARN("GetWindow failed: %08lx\n", hres);
            return hres;
        }

        hwnd = CreateWindowExW(0, wszInternetExplorer_Server, NULL,
                WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                posrect.left, posrect.top, posrect.right-posrect.left, posrect.bottom-posrect.top,
                parent_hwnd, NULL, hInst, This);

        hres = IOleInPlaceSite_OnInPlaceActivate(This->ipsite);
        if(FAILED(hres)) {
            WARN("OnInPlaceActivate failed: %08lx\n", hres);
            return hres;
        }

        SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_NOERASE | RDW_ALLCHILDREN);
        SetFocus(hwnd);

        /* NOTE:
         * Windows implementation calls:
         * RegisterWindowMessage("MSWHEEL_ROLLMSG");
         * SetTimer(This->hwnd, TIMER_ID, 100, NULL);
         */

        hres = IOleInPlaceSite_OnUIActivate(This->ipsite);
        if(SUCCEEDED(hres)) {
            IOleInPlaceFrame_SetActiveObject(pIPFrame, ACTOBJ(This), wszHTML_Document);
        }else {
            FIXME("OnUIActivate failed: %08lx\n", hres);
            DestroyWindow(hwnd);
            return hres;
        }
        if(This->frame)
            IOleInPlaceFrame_Release(This->frame);
        This->frame = pIPFrame;
        This->hwnd = hwnd;
    }else {
        static const WCHAR wszEmpty[] = {0};
    
        if(This->frame)
            IOleInPlaceFrame_SetActiveObject(This->frame, NULL, wszEmpty);
        if(This->ipsite)
            IOleInPlaceSite_OnUIDeactivate(This->ipsite, FALSE);
    }
    return S_OK;
}

static HRESULT WINAPI OleDocumentView_Open(IOleDocumentView *iface)
{
    DOCVIEW_THIS
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleDocumentView_CloseView(IOleDocumentView *iface, DWORD dwReserved)
{
    DOCVIEW_THIS
    TRACE("(%p)->(%lx)\n", This, dwReserved);

    if(dwReserved)
        WARN("dwReserved = %ld\n", dwReserved);

    /* NOTE:
     * Windows implementation calls QueryInterface(IID_IOleCommandTarget),
     * QueryInterface(IID_IOleControlSite) and KillTimer
     */

    IOleDocumentView_Show(iface, FALSE);

    return S_OK;
}

static HRESULT WINAPI OleDocumentView_SaveViewState(IOleDocumentView *iface, LPSTREAM pstm)
{
    DOCVIEW_THIS
    FIXME("(%p)->(%p)\n", This, pstm);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleDocumentView_ApplyViewState(IOleDocumentView *iface, LPSTREAM pstm)
{
    DOCVIEW_THIS
    FIXME("(%p)->(%p)\n", This, pstm);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleDocumentView_Clone(IOleDocumentView *iface, IOleInPlaceSite *pIPSiteNew,
                                        IOleDocumentView **ppViewNew)
{
    DOCVIEW_THIS
    FIXME("(%p)->(%p %p)\n", This, pIPSiteNew, ppViewNew);
    return E_NOTIMPL;
}

static const IOleDocumentViewVtbl OleDocumentViewVtbl = {
    OleDocumentView_QueryInterface,
    OleDocumentView_AddRef,
    OleDocumentView_Release,
    OleDocumentView_SetInPlaceSite,
    OleDocumentView_GetInPlaceSite,
    OleDocumentView_GetDocument,
    OleDocumentView_SetRect,
    OleDocumentView_GetRect,
    OleDocumentView_SetRectComplex,
    OleDocumentView_Show,
    OleDocumentView_UIActivate,
    OleDocumentView_Open,
    OleDocumentView_CloseView,
    OleDocumentView_SaveViewState,
    OleDocumentView_ApplyViewState,
    OleDocumentView_Clone
};

void HTMLDocument_View_Init(HTMLDocument *This)
{
    This->lpOleDocumentViewVtbl = &OleDocumentViewVtbl;

    This->ipsite = NULL;
    This->frame = NULL;
    This->hwnd = NULL;
}
