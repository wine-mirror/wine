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

#define COBJMACROS

#include "dshow.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/strmbase.h"
#include "uuids.h"
#include "vfwmsgs.h"
#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(strmbase);

static LRESULT CALLBACK WndProcW(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BaseWindow* This = (BaseWindow*)GetWindowLongPtrW(hwnd, 0);

    if (!This)
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);

    if (This->pFuncsTable->pfnOnReceiveMessage)
        return This->pFuncsTable->pfnOnReceiveMessage(This, hwnd, uMsg, wParam, lParam);
    else
        return BaseWindowImpl_OnReceiveMessage(This, hwnd, uMsg, wParam, lParam);
}

LRESULT WINAPI BaseWindowImpl_OnReceiveMessage(BaseWindow *This, HWND hwnd, INT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (This->pFuncsTable->pfnPossiblyEatMessage && This->pFuncsTable->pfnPossiblyEatMessage(This, uMsg, wParam, lParam))
        return 0;

    switch (uMsg)
    {
        case WM_SIZE:
            if (This->pFuncsTable->pfnOnSize)
                return This->pFuncsTable->pfnOnSize(This, LOWORD(lParam), HIWORD(lParam));
            else
                return BaseWindowImpl_OnSize(This, LOWORD(lParam), HIWORD(lParam));
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

BOOL WINAPI BaseWindowImpl_OnSize(BaseWindow *This, LONG Width, LONG Height)
{
    This->Width = Width;
    This->Height = Height;
    return TRUE;
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
    WNDCLASSW winclass;
    static const WCHAR windownameW[] = { 'A','c','t','i','v','e','M','o','v','i','e',' ','W','i','n','d','o','w',0 };

    This->pClassName = This->pFuncsTable->pfnGetClassWindowStyles(This, &This->ClassStyles, &This->WindowStyles, &This->WindowStylesEx);

    winclass.style = This->ClassStyles;
    winclass.lpfnWndProc = WndProcW;
    winclass.cbClsExtra = 0;
    winclass.cbWndExtra = sizeof(BaseWindow*);
    winclass.hInstance = This->hInstance;
    winclass.hIcon = NULL;
    winclass.hCursor = NULL;
    winclass.hbrBackground = GetStockObject(BLACK_BRUSH);
    winclass.lpszMenuName = NULL;
    winclass.lpszClassName = This->pClassName;
    if (!RegisterClassW(&winclass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        ERR("Unable to register window class: %u\n", GetLastError());
        return E_FAIL;
    }

    This->hWnd = CreateWindowExW(This->WindowStylesEx,
                                 This->pClassName, windownameW,
                                 This->WindowStyles,
                                 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, NULL, NULL, This->hInstance,
                                 NULL);

    if (!This->hWnd)
    {
        ERR("Unable to create window\n");
        return E_FAIL;
    }

    SetWindowLongPtrW(This->hWnd, 0, (LONG_PTR)This);

    This->hDC = GetDC(This->hWnd);

    return S_OK;
}

HRESULT WINAPI BaseWindowImpl_DoneWithWindow(BaseWindow *This)
{
    if (!This->hWnd)
        return S_OK;

    if (This->hDC)
        ReleaseDC(This->hWnd, This->hDC);
    This->hDC = NULL;

    DestroyWindow(This->hWnd);
    This->hWnd = NULL;

    return S_OK;
}
