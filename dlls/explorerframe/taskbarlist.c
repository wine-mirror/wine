/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
 * Copyright 2011 André Hentschel
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

#include "explorerframe_main.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(explorerframe);

#define WM_WINE_DELETE_TAB      (WM_USER + 1)
#define WM_WINE_ADD_TAB         (WM_USER + 2)

struct taskbar_list
{
    ITaskbarList4 ITaskbarList4_iface;
    LONG refcount;
};

static inline struct taskbar_list *impl_from_ITaskbarList4(ITaskbarList4 *iface)
{
    return CONTAINING_RECORD(iface, struct taskbar_list, ITaskbarList4_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE taskbar_list_QueryInterface(ITaskbarList4 *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ITaskbarList) ||
        IsEqualGUID(riid, &IID_ITaskbarList2) ||
        IsEqualGUID(riid, &IID_ITaskbarList3) ||
        IsEqualGUID(riid, &IID_ITaskbarList4) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        ITaskbarList4_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE taskbar_list_AddRef(ITaskbarList4 *iface)
{
    struct taskbar_list *This = impl_from_ITaskbarList4(iface);
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %lu\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE taskbar_list_Release(ITaskbarList4 *iface)
{
    struct taskbar_list *This = impl_from_ITaskbarList4(iface);
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %lu\n", This, refcount);

    if (!refcount)
    {
        free(This);
        EFRAME_UnlockModule();
    }

    return refcount;
}

/* ITaskbarList methods */

static HRESULT STDMETHODCALLTYPE taskbar_list_HrInit(ITaskbarList4 *iface)
{
    TRACE("iface %p\n", iface);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE taskbar_list_AddTab(ITaskbarList4 *iface, HWND hwnd)
{
    TRACE("iface %p, hwnd %p\n", iface, hwnd);

    SendMessageW(GetDesktopWindow(), WM_WINE_ADD_TAB, (WPARAM)hwnd, 0);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE taskbar_list_DeleteTab(ITaskbarList4 *iface, HWND hwnd)
{
    TRACE("iface %p, hwnd %p\n", iface, hwnd);

    SendMessageW(GetDesktopWindow(), WM_WINE_DELETE_TAB, (WPARAM)hwnd, 0);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE taskbar_list_ActivateTab(ITaskbarList4 *iface, HWND hwnd)
{
    FIXME("iface %p, hwnd %p stub!\n", iface, hwnd);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE taskbar_list_SetActiveAlt(ITaskbarList4 *iface, HWND hwnd)
{
    FIXME("iface %p, hwnd %p stub!\n", iface, hwnd);

    return E_NOTIMPL;
}

/* ITaskbarList2 method */

static HRESULT STDMETHODCALLTYPE taskbar_list_MarkFullscreenWindow(ITaskbarList4 *iface,
                                                                   HWND hwnd,
                                                                   BOOL fullscreen)
{
    FIXME("iface %p, hwnd %p, fullscreen %s stub!\n", iface, hwnd, (fullscreen)?"true":"false");

    return E_NOTIMPL;
}

/* ITaskbarList3 methods */

static HRESULT STDMETHODCALLTYPE taskbar_list_SetProgressValue(ITaskbarList4 *iface,
                                                               HWND hwnd,
                                                               ULONGLONG ullCompleted,
                                                               ULONGLONG ullTotal)
{
    static BOOL fixme_once;
    if(!fixme_once++) FIXME("iface %p, hwnd %p, ullCompleted %s, ullTotal %s stub!\n", iface, hwnd,
                            wine_dbgstr_longlong(ullCompleted), wine_dbgstr_longlong(ullTotal));

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE taskbar_list_SetProgressState(ITaskbarList4 *iface,
                                                               HWND hwnd,
                                                               TBPFLAG tbpFlags)
{
    static BOOL fixme_once;
    if(!fixme_once++) FIXME("iface %p, hwnd %p, flags %x stub!\n", iface, hwnd, tbpFlags);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE taskbar_list_RegisterTab(ITaskbarList4 *iface, HWND hwndTab, HWND hwndMDI)
{
    FIXME("iface %p, hwndTab %p, hwndMDI %p stub!\n", iface, hwndTab, hwndMDI);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE taskbar_list_UnregisterTab(ITaskbarList4 *iface, HWND hwndTab)
{
    FIXME("iface %p, hwndTab %p stub!\n", iface, hwndTab);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE taskbar_list_SetTabOrder(ITaskbarList4 *iface,
                                                          HWND hwndTab,
                                                          HWND hwndInsertBefore)
{
    FIXME("iface %p, hwndTab %p, hwndInsertBefore %p stub!\n", iface, hwndTab, hwndInsertBefore);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE taskbar_list_SetTabActive(ITaskbarList4 *iface,
                                                          HWND hwndTab,
                                                          HWND hwndMDI,
                                                          DWORD dwReserved)
{
    FIXME("iface %p, hwndTab %p, hwndMDI %p, dwReserved %lx stub!\n", iface, hwndTab, hwndMDI, dwReserved);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE taskbar_list_ThumbBarAddButtons(ITaskbarList4 *iface,
                                                                 HWND hwnd,
                                                                 UINT cButtons,
                                                                 LPTHUMBBUTTON pButton)
{
    FIXME("iface %p, hwnd %p, cButtons %u, pButton %p stub, faking success!\n", iface, hwnd, cButtons, pButton);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE taskbar_list_ThumbBarUpdateButtons(ITaskbarList4 *iface,
                                                                   HWND hwnd,
                                                                   UINT cButtons,
                                                                   LPTHUMBBUTTON pButton)
{
    FIXME("iface %p, hwnd %p, cButtons %u, pButton %p stub, faking success!\n", iface, hwnd, cButtons, pButton);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE taskbar_list_ThumbBarSetImageList(ITaskbarList4 *iface,
                                                                   HWND hwnd,
                                                                   HIMAGELIST himl)
{
    FIXME("iface %p, hwnd %p, himl %p stub!\n", iface, hwnd, himl);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE taskbar_list_SetOverlayIcon(ITaskbarList4 *iface,
                                                             HWND hwnd,
                                                             HICON hIcon,
                                                             LPCWSTR pszDescription)
{
    FIXME("iface %p, hwnd %p, hIcon %p, pszDescription %s stub!\n", iface, hwnd, hIcon,
          debugstr_w(pszDescription));

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE taskbar_list_SetThumbnailTooltip(ITaskbarList4 *iface,
                                                                  HWND hwnd,
                                                                  LPCWSTR pszTip)
{
    FIXME("iface %p, hwnd %p, pszTip %s stub, faking success!\n", iface, hwnd, debugstr_w(pszTip));

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE taskbar_list_SetThumbnailClip(ITaskbarList4 *iface,
                                                               HWND hwnd,
                                                               RECT *prcClip)
{
    FIXME("iface %p, hwnd %p, prcClip %s stub!\n", iface, hwnd, wine_dbgstr_rect(prcClip));

    return E_NOTIMPL;
}

/* ITaskbarList4 method */

static HRESULT STDMETHODCALLTYPE taskbar_list_SetTabProperties(ITaskbarList4 *iface,
                                                               HWND hwndTab,
                                                               STPFLAG stpFlags)
{
    FIXME("iface %p, hwndTab %p, stpFlags %u stub!\n", iface, hwndTab, stpFlags);

    return E_NOTIMPL;
}

static const struct ITaskbarList4Vtbl taskbar_list_vtbl =
{
    /* IUnknown methods */
    taskbar_list_QueryInterface,
    taskbar_list_AddRef,
    taskbar_list_Release,
    /* ITaskbarList methods */
    taskbar_list_HrInit,
    taskbar_list_AddTab,
    taskbar_list_DeleteTab,
    taskbar_list_ActivateTab,
    taskbar_list_SetActiveAlt,
    /* ITaskbarList2 method */
    taskbar_list_MarkFullscreenWindow,
    /* ITaskbarList3 methods */
    taskbar_list_SetProgressValue,
    taskbar_list_SetProgressState,
    taskbar_list_RegisterTab,
    taskbar_list_UnregisterTab,
    taskbar_list_SetTabOrder,
    taskbar_list_SetTabActive,
    taskbar_list_ThumbBarAddButtons,
    taskbar_list_ThumbBarUpdateButtons,
    taskbar_list_ThumbBarSetImageList,
    taskbar_list_SetOverlayIcon,
    taskbar_list_SetThumbnailTooltip,
    taskbar_list_SetThumbnailClip,
    /* ITaskbarList4 method */
    taskbar_list_SetTabProperties,
};

HRESULT TaskbarList_Constructor(IUnknown *outer, REFIID riid, void **taskbar_list)
{
    struct taskbar_list *object;
    HRESULT hres;

    TRACE("outer %p, riid %s, taskbar_list %p\n", outer, debugstr_guid(riid), taskbar_list);

    if (outer)
    {
        WARN("Aggregation not supported\n");
        *taskbar_list = NULL;
        return CLASS_E_NOAGGREGATION;
    }

    object = malloc(sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate taskbar list object memory\n");
        *taskbar_list = NULL;
        return E_OUTOFMEMORY;
    }

    object->ITaskbarList4_iface.lpVtbl = &taskbar_list_vtbl;
    object->refcount = 1;
    EFRAME_LockModule();

    TRACE("Created ITaskbarList4 %p\n", object);

    hres = ITaskbarList4_QueryInterface(&object->ITaskbarList4_iface, riid, taskbar_list);
    ITaskbarList4_Release(&object->ITaskbarList4_iface);
    return hres;
}
