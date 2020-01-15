/*
 * Generic Implementation of strmbase window classes
 *
 * Copyright 2012 Aric Stewart, CodeWeavers
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

#include "strmbase_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(strmbase);

static inline BaseControlWindow *impl_from_IVideoWindow( IVideoWindow *iface)
{
    return CONTAINING_RECORD(iface, BaseControlWindow, IVideoWindow_iface);
}

static inline BaseControlWindow *impl_from_BaseWindow(BaseWindow *iface)
{
    return CONTAINING_RECORD(iface, BaseControlWindow, baseWindow);
}

static LRESULT CALLBACK WndProcW(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    BaseWindow* This = (BaseWindow*)GetWindowLongPtrW(hwnd, 0);
    BaseControlWindow *window = impl_from_BaseWindow(This);

    if (!This)
        return DefWindowProcW(hwnd, message, wparam, lparam);

    switch (message)
    {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEACTIVATE:
    case WM_MOUSEMOVE:
    case WM_NCLBUTTONDBLCLK:
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONUP:
    case WM_NCMBUTTONDBLCLK:
    case WM_NCMBUTTONDOWN:
    case WM_NCMBUTTONUP:
    case WM_NCMOUSEMOVE:
    case WM_NCRBUTTONDBLCLK:
    case WM_NCRBUTTONDOWN:
    case WM_NCRBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        if (window->hwndDrain)
        {
            PostMessageW(window->hwndDrain, message, wparam, lparam);
            return 0;
        }
        break;
    case WM_SIZE:
        if (This->pFuncsTable->pfnOnSize)
            return This->pFuncsTable->pfnOnSize(This, LOWORD(lparam), HIWORD(lparam));

        This->Width = LOWORD(lparam);
        This->Height = HIWORD(lparam);
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

HRESULT WINAPI BaseWindow_Init(BaseWindow *pBaseWindow, const BaseWindowFuncTable* pFuncsTable)
{
    if (!pFuncsTable)
        return E_INVALIDARG;

    ZeroMemory(pBaseWindow,sizeof(BaseWindow));
    pBaseWindow->pFuncsTable = pFuncsTable;

    return S_OK;
}

HRESULT WINAPI BaseWindow_Destroy(BaseWindow *This)
{
    if (This->hWnd)
        BaseWindowImpl_DoneWithWindow(This);

    HeapFree(GetProcessHeap(), 0, This);
    return S_OK;
}

HRESULT WINAPI BaseWindowImpl_PrepareWindow(BaseWindow *This)
{
    static const WCHAR class_nameW[] = {'w','i','n','e','_','s','t','r','m','b','a','s','e','_','w','i','n','d','o','w',0};
    static const WCHAR windownameW[] = { 'A','c','t','i','v','e','M','o','v','i','e',' ','W','i','n','d','o','w',0 };
    WNDCLASSW winclass = {0};

    winclass.lpfnWndProc = WndProcW;
    winclass.cbWndExtra = sizeof(BaseWindow*);
    winclass.hbrBackground = GetStockObject(BLACK_BRUSH);
    winclass.lpszClassName = class_nameW;
    if (!RegisterClassW(&winclass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        ERR("Unable to register window class: %u\n", GetLastError());
        return E_FAIL;
    }

    This->hWnd = CreateWindowExW(0, class_nameW, windownameW,
            WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            NULL, NULL, NULL, NULL);

    if (!This->hWnd)
    {
        ERR("Unable to create window\n");
        return E_FAIL;
    }

    SetWindowLongPtrW(This->hWnd, 0, (LONG_PTR)This);

    return S_OK;
}

HRESULT WINAPI BaseWindowImpl_DoneWithWindow(BaseWindow *This)
{
    BaseControlWindow *window = impl_from_BaseWindow(This);

    if (!This->hWnd)
        return S_OK;

    /* Media Player Classic deadlocks if WM_PARENTNOTIFY is sent, so unparent
     * the window first. */
    IVideoWindow_put_Owner(&window->IVideoWindow_iface, 0);

    SendMessageW(This->hWnd, WM_CLOSE, 0, 0);
    This->hWnd = NULL;

    return S_OK;
}

HRESULT WINAPI strmbase_window_init(BaseControlWindow *pControlWindow,
        const IVideoWindowVtbl *lpVtbl, struct strmbase_filter *owner,
        struct strmbase_pin *pPin, const BaseWindowFuncTable *pFuncsTable)
{
    HRESULT hr;

    hr = BaseWindow_Init(&pControlWindow->baseWindow, pFuncsTable);
    if (SUCCEEDED(hr))
    {
        pControlWindow->IVideoWindow_iface.lpVtbl = lpVtbl;
        pControlWindow->AutoShow = OATRUE;
        pControlWindow->hwndDrain = NULL;
        pControlWindow->hwndOwner = NULL;
        pControlWindow->pFilter = owner;
        pControlWindow->pPin = pPin;
    }
    return hr;
}

HRESULT WINAPI BaseControlWindow_Destroy(BaseControlWindow *pControlWindow)
{
    BaseWindowImpl_DoneWithWindow(&pControlWindow->baseWindow);
    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_QueryInterface(IVideoWindow *iface, REFIID iid, void **out)
{
    BaseControlWindow *window = impl_from_IVideoWindow(iface);
    return IUnknown_QueryInterface(window->pFilter->outer_unk, iid, out);
}

ULONG WINAPI BaseControlWindowImpl_AddRef(IVideoWindow *iface)
{
    BaseControlWindow *window = impl_from_IVideoWindow(iface);
    return IUnknown_AddRef(window->pFilter->outer_unk);
}

ULONG WINAPI BaseControlWindowImpl_Release(IVideoWindow *iface)
{
    BaseControlWindow *window = impl_from_IVideoWindow(iface);
    return IUnknown_Release(window->pFilter->outer_unk);
}

HRESULT WINAPI BaseControlWindowImpl_GetTypeInfoCount(IVideoWindow *iface, UINT *count)
{
    TRACE("iface %p, count %p.\n", iface, count);
    *count = 1;
    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_GetTypeInfo(IVideoWindow *iface, UINT index,
        LCID lcid, ITypeInfo **typeinfo)
{
    TRACE("iface %p, index %u, lcid %#x, typeinfo %p.\n", iface, index, lcid, typeinfo);
    return strmbase_get_typeinfo(IVideoWindow_tid, typeinfo);
}

HRESULT WINAPI BaseControlWindowImpl_GetIDsOfNames(IVideoWindow *iface, REFIID iid,
        LPOLESTR *names, UINT count, LCID lcid, DISPID *ids)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, iid %s, names %p, count %u, lcid %#x, ids %p.\n",
            iface, debugstr_guid(iid), names, count, lcid, ids);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IVideoWindow_tid, &typeinfo)))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, ids);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

HRESULT WINAPI BaseControlWindowImpl_Invoke(IVideoWindow *iface, DISPID id, REFIID iid, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *error_arg)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, id %d, iid %s, lcid %#x, flags %#x, params %p, result %p, excepinfo %p, error_arg %p.\n",
            iface, id, debugstr_guid(iid), lcid, flags, params, result, excepinfo, error_arg);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IVideoWindow_tid, &typeinfo)))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, id, flags, params, result, excepinfo, error_arg);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

HRESULT WINAPI BaseControlWindowImpl_put_Caption(IVideoWindow *iface, BSTR strCaption)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%s (%p))\n", This, iface, debugstr_w(strCaption), strCaption);

    if (!SetWindowTextW(This->baseWindow.hWnd, strCaption))
        return E_FAIL;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_Caption(IVideoWindow *iface, BSTR *caption)
{
    BaseControlWindow *window = impl_from_IVideoWindow(iface);
    WCHAR *str;
    int len;

    TRACE("window %p, caption %p.\n", window, caption);

    *caption = NULL;

    len = GetWindowTextLengthW(window->baseWindow.hWnd) + 1;
    if (!(str = heap_alloc(len * sizeof(WCHAR))))
        return E_OUTOFMEMORY;

    GetWindowTextW(window->baseWindow.hWnd, str, len);
    *caption = SysAllocString(str);
    heap_free(str);
    return *caption ? S_OK : E_OUTOFMEMORY;
}

HRESULT WINAPI BaseControlWindowImpl_put_WindowStyle(IVideoWindow *iface, LONG WindowStyle)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);
    LONG old;

    old = GetWindowLongW(This->baseWindow.hWnd, GWL_STYLE);

    TRACE("(%p/%p)->(%x -> %x)\n", This, iface, old, WindowStyle);

    if (WindowStyle & (WS_DISABLED|WS_HSCROLL|WS_MAXIMIZE|WS_MINIMIZE|WS_VSCROLL))
        return E_INVALIDARG;

    SetWindowLongW(This->baseWindow.hWnd, GWL_STYLE, WindowStyle);
    SetWindowPos(This->baseWindow.hWnd, 0, 0, 0, 0, 0,
            SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_WindowStyle(IVideoWindow *iface, LONG *style)
{
    BaseControlWindow *window = impl_from_IVideoWindow(iface);

    TRACE("window %p, style %p.\n", window, style);

    *style = GetWindowLongW(window->baseWindow.hWnd, GWL_STYLE);

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_WindowStyleEx(IVideoWindow *iface, LONG WindowStyleEx)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, WindowStyleEx);

    if (!SetWindowLongW(This->baseWindow.hWnd, GWL_EXSTYLE, WindowStyleEx))
        return E_FAIL;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_WindowStyleEx(IVideoWindow *iface, LONG *WindowStyleEx)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, WindowStyleEx);

    *WindowStyleEx = GetWindowLongW(This->baseWindow.hWnd, GWL_EXSTYLE);

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_AutoShow(IVideoWindow *iface, LONG AutoShow)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, AutoShow);

    This->AutoShow = AutoShow;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_AutoShow(IVideoWindow *iface, LONG *AutoShow)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, AutoShow);

    *AutoShow = This->AutoShow;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_WindowState(IVideoWindow *iface, LONG WindowState)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, WindowState);
    ShowWindow(This->baseWindow.hWnd, WindowState);
    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_WindowState(IVideoWindow *iface, LONG *state)
{
    BaseControlWindow *window = impl_from_IVideoWindow(iface);
    DWORD style;

    TRACE("window %p, state %p.\n", window, state);

    style = GetWindowLongPtrW(window->baseWindow.hWnd, GWL_STYLE);
    if (!(style & WS_VISIBLE))
        *state = SW_HIDE;
    else if (style & WS_MINIMIZE)
        *state = SW_MINIMIZE;
    else if (style & WS_MAXIMIZE)
        *state = SW_MAXIMIZE;
    else
        *state = SW_SHOW;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_BackgroundPalette(IVideoWindow *iface, LONG BackgroundPalette)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    FIXME("(%p/%p)->(%d): stub !!!\n", This, iface, BackgroundPalette);

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_BackgroundPalette(IVideoWindow *iface, LONG *pBackgroundPalette)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, pBackgroundPalette);

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_Visible(IVideoWindow *iface, LONG Visible)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, Visible);

    ShowWindow(This->baseWindow.hWnd, Visible ? SW_SHOW : SW_HIDE);

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_Visible(IVideoWindow *iface, LONG *pVisible)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pVisible);

    *pVisible = IsWindowVisible(This->baseWindow.hWnd) ? OATRUE : OAFALSE;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_Left(IVideoWindow *iface, LONG Left)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);
    RECT WindowPos;

    TRACE("(%p/%p)->(%d)\n", This, iface, Left);

    GetWindowRect(This->baseWindow.hWnd, &WindowPos);
    if (!SetWindowPos(This->baseWindow.hWnd, NULL, Left, WindowPos.top, 0, 0,
            SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE))
        return E_FAIL;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_Left(IVideoWindow *iface, LONG *pLeft)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);
    RECT WindowPos;

    TRACE("(%p/%p)->(%p)\n", This, iface, pLeft);
    GetWindowRect(This->baseWindow.hWnd, &WindowPos);

    *pLeft = WindowPos.left;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_Width(IVideoWindow *iface, LONG Width)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, Width);

    if (!SetWindowPos(This->baseWindow.hWnd, NULL, 0, 0, Width, This->baseWindow.Height,
            SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE))
        return E_FAIL;

    This->baseWindow.Width = Width;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_Width(IVideoWindow *iface, LONG *pWidth)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pWidth);

    *pWidth = This->baseWindow.Width;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_Top(IVideoWindow *iface, LONG Top)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);
    RECT WindowPos;

    TRACE("(%p/%p)->(%d)\n", This, iface, Top);
    GetWindowRect(This->baseWindow.hWnd, &WindowPos);

    if (!SetWindowPos(This->baseWindow.hWnd, NULL, WindowPos.left, Top, 0, 0,
            SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE))
        return E_FAIL;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_Top(IVideoWindow *iface, LONG *pTop)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);
    RECT WindowPos;

    TRACE("(%p/%p)->(%p)\n", This, iface, pTop);
    GetWindowRect(This->baseWindow.hWnd, &WindowPos);

    *pTop = WindowPos.top;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_Height(IVideoWindow *iface, LONG Height)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, Height);

    if (!SetWindowPos(This->baseWindow.hWnd, NULL, 0, 0, This->baseWindow.Width,
            Height, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE))
        return E_FAIL;

    This->baseWindow.Height = Height;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_Height(IVideoWindow *iface, LONG *pHeight)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pHeight);

    *pHeight = This->baseWindow.Height;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_Owner(IVideoWindow *iface, OAHWND owner)
{
    BaseControlWindow *window = impl_from_IVideoWindow(iface);
    HWND hwnd = window->baseWindow.hWnd;

    TRACE("window %p, owner %#lx.\n", window, owner);

    /* Make sure we are marked as WS_CHILD before reparenting ourselves, so that
     * we do not steal focus. LEGO Island depends on this. */

    window->hwndOwner = (HWND)owner;
    if (owner)
        SetWindowLongPtrW(hwnd, GWL_STYLE, GetWindowLongPtrW(hwnd, GWL_STYLE) | WS_CHILD);
    else
        SetWindowLongPtrW(hwnd, GWL_STYLE, GetWindowLongPtrW(hwnd, GWL_STYLE) & ~WS_CHILD);
    SetParent(hwnd, (HWND)owner);

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_Owner(IVideoWindow *iface, OAHWND *Owner)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, Owner);

    *(HWND*)Owner = This->hwndOwner;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_MessageDrain(IVideoWindow *iface, OAHWND Drain)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%08x)\n", This, iface, (DWORD) Drain);

    This->hwndDrain = (HWND)Drain;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_MessageDrain(IVideoWindow *iface, OAHWND *Drain)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, Drain);

    *Drain = (OAHWND)This->hwndDrain;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_BorderColor(IVideoWindow *iface, LONG *Color)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, Color);

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_BorderColor(IVideoWindow *iface, LONG Color)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    FIXME("(%p/%p)->(%d): stub !!!\n", This, iface, Color);

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_FullScreenMode(IVideoWindow *iface, LONG *FullScreenMode)
{
    TRACE("(%p)->(%p)\n", iface, FullScreenMode);

    return E_NOTIMPL;
}

HRESULT WINAPI BaseControlWindowImpl_put_FullScreenMode(IVideoWindow *iface, LONG FullScreenMode)
{
    TRACE("(%p)->(%d)\n", iface, FullScreenMode);
    return E_NOTIMPL;
}

HRESULT WINAPI BaseControlWindowImpl_SetWindowForeground(IVideoWindow *iface, LONG focus)
{
    BaseControlWindow *window = impl_from_IVideoWindow(iface);
    UINT flags = SWP_NOMOVE | SWP_NOSIZE;

    TRACE("window %p, focus %d.\n", window, focus);

    if (focus != OAFALSE && focus != OATRUE)
        return E_INVALIDARG;

    if (!window->pPin->peer)
        return VFW_E_NOT_CONNECTED;

    if (!focus)
        flags |= SWP_NOACTIVATE;
    SetWindowPos(window->baseWindow.hWnd, HWND_TOP, 0, 0, 0, 0, flags);

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_SetWindowPosition(IVideoWindow *iface, LONG Left, LONG Top, LONG Width, LONG Height)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%d, %d, %d, %d)\n", This, iface, Left, Top, Width, Height);

    if (!SetWindowPos(This->baseWindow.hWnd, NULL, Left, Top, Width, Height, SWP_NOACTIVATE | SWP_NOZORDER))
        return E_FAIL;

    This->baseWindow.Width = Width;
    This->baseWindow.Height = Height;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_GetWindowPosition(IVideoWindow *iface, LONG *pLeft, LONG *pTop, LONG *pWidth, LONG *pHeight)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);
    RECT WindowPos;

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);
    GetWindowRect(This->baseWindow.hWnd, &WindowPos);

    *pLeft = WindowPos.left;
    *pTop = WindowPos.top;
    *pWidth = This->baseWindow.Width;
    *pHeight = This->baseWindow.Height;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_NotifyOwnerMessage(IVideoWindow *iface,
        OAHWND hwnd, LONG message, LONG_PTR wparam, LONG_PTR lparam)
{
    BaseControlWindow *window = impl_from_IVideoWindow(iface);

    TRACE("window %p, hwnd %#lx, message %#x, wparam %#lx, lparam %#lx.\n",
            window, hwnd, message, wparam, lparam);

    /* That these messages are forwarded, and no others, is stated by the
     * DirectX documentation, and supported by manual testing. */
    switch (message)
    {
    case WM_ACTIVATEAPP:
    case WM_DEVMODECHANGE:
    case WM_DISPLAYCHANGE:
    case WM_PALETTECHANGED:
    case WM_PALETTEISCHANGING:
    case WM_QUERYNEWPALETTE:
    case WM_SYSCOLORCHANGE:
        SendMessageW(window->baseWindow.hWnd, message, wparam, lparam);
        break;
    }

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_GetMinIdealImageSize(IVideoWindow *iface, LONG *pWidth, LONG *pHeight)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);
    RECT defaultRect;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pWidth, pHeight);
    defaultRect = This->baseWindow.pFuncsTable->pfnGetDefaultRect(&This->baseWindow);

    *pWidth = defaultRect.right - defaultRect.left;
    *pHeight = defaultRect.bottom - defaultRect.top;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_GetMaxIdealImageSize(IVideoWindow *iface, LONG *pWidth, LONG *pHeight)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);
    RECT defaultRect;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pWidth, pHeight);
    defaultRect = This->baseWindow.pFuncsTable->pfnGetDefaultRect(&This->baseWindow);

    *pWidth = defaultRect.right - defaultRect.left;
    *pHeight = defaultRect.bottom - defaultRect.top;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_GetRestorePosition(IVideoWindow *iface, LONG *pLeft, LONG *pTop, LONG *pWidth, LONG *pHeight)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    FIXME("(%p/%p)->(%p, %p, %p, %p): stub !!!\n", This, iface, pLeft, pTop, pWidth, pHeight);

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_HideCursor(IVideoWindow *iface, LONG HideCursor)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    FIXME("(%p/%p)->(%d): stub !!!\n", This, iface, HideCursor);

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_IsCursorHidden(IVideoWindow *iface, LONG *CursorHidden)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, CursorHidden);

    return S_OK;
}
