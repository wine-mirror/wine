/*
 * MACDRV Drag and drop code
 *
 * Copyright 2003 Ulrich Czekalla
 * Copyright 2007 Damjan Jovanovic
 * Copyright 2013 Ken Thomases for CodeWeavers Inc.
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

#define NONAMELESSUNION

#include "macdrv.h"

#define COBJMACROS
#include "objidl.h"
#include "shellapi.h"
#include "shlobj.h"

WINE_DEFAULT_DEBUG_CHANNEL(dragdrop);


static IDataObject *active_data_object;
static HWND last_droptarget_hwnd;


typedef struct
{
    IDataObject IDataObject_iface;
    LONG        ref;
    CFTypeRef   pasteboard;
} DragDropDataObject;


static inline DragDropDataObject *impl_from_IDataObject(IDataObject *iface)
{
    return CONTAINING_RECORD(iface, DragDropDataObject, IDataObject_iface);
}


static HRESULT WINAPI dddo_QueryInterface(IDataObject* iface, REFIID riid, LPVOID *ppvObj)
{
    DragDropDataObject *This = impl_from_IDataObject(iface);

    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), ppvObj);

    if (IsEqualIID(riid, &IID_IUnknown) || (IsEqualIID(riid, &IID_IDataObject)))
    {
        *ppvObj = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}


static ULONG WINAPI dddo_AddRef(IDataObject* iface)
{
    DragDropDataObject *This = impl_from_IDataObject(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(count=%u)\n", This, refCount - 1);

    return refCount;
}


static ULONG WINAPI dddo_Release(IDataObject* iface)
{
    DragDropDataObject *This = impl_from_IDataObject(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(count=%u)\n", This, refCount + 1);
    if (refCount)
        return refCount;

    TRACE("-- destroying DragDropDataObject (%p)\n", This);
    CFRelease(This->pasteboard);
    HeapFree(GetProcessHeap(), 0, This);
    return 0;
}


static HRESULT WINAPI dddo_GetData(IDataObject* iface, FORMATETC* formatEtc, STGMEDIUM* medium)
{
    DragDropDataObject *This = impl_from_IDataObject(iface);
    HRESULT hr;

    TRACE("This %p formatEtc %s\n", This, debugstr_format(formatEtc->cfFormat));

    hr = IDataObject_QueryGetData(iface, formatEtc);
    if (SUCCEEDED(hr))
    {
        medium->tymed = TYMED_HGLOBAL;
        medium->u.hGlobal = macdrv_get_pasteboard_data(This->pasteboard, formatEtc->cfFormat);
        medium->pUnkForRelease = NULL;
        hr = medium->u.hGlobal ? S_OK : E_OUTOFMEMORY;
    }

    return hr;
}


static HRESULT WINAPI dddo_GetDataHere(IDataObject* iface, FORMATETC* formatEtc,
                                       STGMEDIUM* medium)
{
    FIXME("iface %p formatEtc %p medium %p; stub\n", iface, formatEtc, medium);
    return DATA_E_FORMATETC;
}


static HRESULT WINAPI dddo_QueryGetData(IDataObject* iface, FORMATETC* formatEtc)
{
    DragDropDataObject *This = impl_from_IDataObject(iface);
    HRESULT hr = DV_E_FORMATETC;

    TRACE("This %p formatEtc %p={.tymed=0x%x, .dwAspect=%d, .cfFormat=%s}\n",
          This, formatEtc, formatEtc->tymed, formatEtc->dwAspect,
          debugstr_format(formatEtc->cfFormat));

    if (formatEtc->tymed && !(formatEtc->tymed & TYMED_HGLOBAL))
    {
        FIXME("only HGLOBAL medium types supported right now\n");
        return DV_E_TYMED;
    }
    if (formatEtc->dwAspect != DVASPECT_CONTENT)
    {
        FIXME("only the content aspect is supported right now\n");
        return E_NOTIMPL;
    }

    if (macdrv_pasteboard_has_format(This->pasteboard, formatEtc->cfFormat))
        hr = S_OK;

    TRACE(" -> 0x%x\n", hr);
    return hr;
}


static HRESULT WINAPI dddo_GetConicalFormatEtc(IDataObject* iface, FORMATETC* formatEtc,
                                               FORMATETC* formatEtcOut)
{
    DragDropDataObject *This = impl_from_IDataObject(iface);

    TRACE("This %p formatEtc %p={.tymed=0x%x, .dwAspect=%d, .cfFormat=%s}\n",
          This, formatEtc, formatEtc->tymed, formatEtc->dwAspect,
          debugstr_format(formatEtc->cfFormat));

    *formatEtcOut = *formatEtc;
    formatEtcOut->ptd = NULL;
    return DATA_S_SAMEFORMATETC;
}


static HRESULT WINAPI dddo_SetData(IDataObject* iface, FORMATETC* formatEtc,
                                   STGMEDIUM* medium, BOOL fRelease)
{
    DragDropDataObject *This = impl_from_IDataObject(iface);

    TRACE("This %p formatEtc %p={.tymed=0x%x, .dwAspect=%d, .cfFormat=%s} medium %p fRelease %d\n",
          This, formatEtc, formatEtc->tymed, formatEtc->dwAspect,
          debugstr_format(formatEtc->cfFormat), medium, fRelease);

    return E_NOTIMPL;
}


static HRESULT WINAPI dddo_EnumFormatEtc(IDataObject* iface, DWORD direction,
                                         IEnumFORMATETC** enumFormatEtc)
{
    DragDropDataObject *This = impl_from_IDataObject(iface);
    CFArrayRef formats;
    HRESULT hr;

    TRACE("This %p direction %u enumFormatEtc %p\n", This, direction, enumFormatEtc);

    if (direction != DATADIR_GET)
    {
        WARN("only the get direction is implemented\n");
        return E_NOTIMPL;
    }

    formats = macdrv_copy_pasteboard_formats(This->pasteboard);
    if (formats)
    {
        INT count = CFArrayGetCount(formats);
        FORMATETC *formatEtcs = HeapAlloc(GetProcessHeap(), 0, count * sizeof(FORMATETC));
        if (formatEtcs)
        {
            INT i;

            for (i = 0; i < count; i++)
            {
                formatEtcs[i].cfFormat = (UINT)CFArrayGetValueAtIndex(formats, i);
                formatEtcs[i].ptd = NULL;
                formatEtcs[i].dwAspect = DVASPECT_CONTENT;
                formatEtcs[i].lindex = -1;
                formatEtcs[i].tymed = TYMED_HGLOBAL;
            }

            hr = SHCreateStdEnumFmtEtc(count, formatEtcs, enumFormatEtc);
            HeapFree(GetProcessHeap(), 0, formatEtcs);
        }
        else
            hr = E_OUTOFMEMORY;

        CFRelease(formats);
    }
    else
        hr = SHCreateStdEnumFmtEtc(0, NULL, enumFormatEtc);

    TRACE(" -> 0x%x\n", hr);
    return hr;
}


static HRESULT WINAPI dddo_DAdvise(IDataObject* iface, FORMATETC* formatEtc, DWORD advf,
                                   IAdviseSink* pAdvSink, DWORD* pdwConnection)
{
    FIXME("(%p, %p, %u, %p, %p): stub\n", iface, formatEtc, advf,
          pAdvSink, pdwConnection);
    return OLE_E_ADVISENOTSUPPORTED;
}


static HRESULT WINAPI dddo_DUnadvise(IDataObject* iface, DWORD dwConnection)
{
    FIXME("(%p, %u): stub\n", iface, dwConnection);
    return OLE_E_ADVISENOTSUPPORTED;
}


static HRESULT WINAPI dddo_EnumDAdvise(IDataObject* iface, IEnumSTATDATA** pEnumAdvise)
{
    FIXME("(%p, %p): stub\n", iface, pEnumAdvise);
    return OLE_E_ADVISENOTSUPPORTED;
}


static const IDataObjectVtbl dovt =
{
    dddo_QueryInterface,
    dddo_AddRef,
    dddo_Release,
    dddo_GetData,
    dddo_GetDataHere,
    dddo_QueryGetData,
    dddo_GetConicalFormatEtc,
    dddo_SetData,
    dddo_EnumFormatEtc,
    dddo_DAdvise,
    dddo_DUnadvise,
    dddo_EnumDAdvise
};


static IDataObject *create_data_object_for_pasteboard(CFTypeRef pasteboard)
{
    DragDropDataObject *dddo;

    dddo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*dddo));
    if (!dddo)
        return NULL;

    dddo->ref = 1;
    dddo->IDataObject_iface.lpVtbl = &dovt;
    dddo->pasteboard = CFRetain(pasteboard);

    return &dddo->IDataObject_iface;
}


/**************************************************************************
 *              drag_operations_to_dropeffects
 */
static DWORD drag_operations_to_dropeffects(uint32_t ops)
{
    DWORD effects = DROPEFFECT_NONE;
    if (ops & (DRAG_OP_COPY | DRAG_OP_GENERIC))
        effects |= DROPEFFECT_COPY;
    if (ops & DRAG_OP_MOVE)
        effects |= DROPEFFECT_MOVE;
    if (ops & (DRAG_OP_LINK | DRAG_OP_GENERIC))
        effects |= DROPEFFECT_LINK;
    return effects;
}


/**************************************************************************
 *              dropeffect_to_drag_operation
 */
static uint32_t dropeffect_to_drag_operation(DWORD effect, uint32_t ops)
{
    if (effect & DROPEFFECT_LINK && ops & DRAG_OP_LINK) return DRAG_OP_LINK;
    if (effect & DROPEFFECT_COPY && ops & DRAG_OP_COPY) return DRAG_OP_COPY;
    if (effect & DROPEFFECT_MOVE && ops & DRAG_OP_MOVE) return DRAG_OP_MOVE;
    if (effect & DROPEFFECT_LINK && ops & DRAG_OP_GENERIC) return DRAG_OP_GENERIC;
    if (effect & DROPEFFECT_COPY && ops & DRAG_OP_GENERIC) return DRAG_OP_GENERIC;

    return DRAG_OP_NONE;
}


/* Based on functions in dlls/ole32/ole2.c */
static HANDLE get_droptarget_local_handle(HWND hwnd)
{
    static const WCHAR prop_marshalleddroptarget[] =
        {'W','i','n','e','M','a','r','s','h','a','l','l','e','d','D','r','o','p','T','a','r','g','e','t',0};
    HANDLE handle;
    HANDLE local_handle = 0;

    handle = GetPropW(hwnd, prop_marshalleddroptarget);
    if (handle)
    {
        DWORD pid;
        HANDLE process;

        GetWindowThreadProcessId(hwnd, &pid);
        process = OpenProcess(PROCESS_DUP_HANDLE, FALSE, pid);
        if (process)
        {
            DuplicateHandle(process, handle, GetCurrentProcess(), &local_handle, 0, FALSE, DUPLICATE_SAME_ACCESS);
            CloseHandle(process);
        }
    }
    return local_handle;
}


static HRESULT create_stream_from_map(HANDLE map, IStream **stream)
{
    HRESULT hr = E_OUTOFMEMORY;
    HGLOBAL hmem;
    void *data;
    MEMORY_BASIC_INFORMATION info;

    data = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
    if(!data) return hr;

    VirtualQuery(data, &info, sizeof(info));
    TRACE("size %d\n", (int)info.RegionSize);

    hmem = GlobalAlloc(GMEM_MOVEABLE, info.RegionSize);
    if(hmem)
    {
        memcpy(GlobalLock(hmem), data, info.RegionSize);
        GlobalUnlock(hmem);
        hr = CreateStreamOnHGlobal(hmem, TRUE, stream);
    }
    UnmapViewOfFile(data);
    return hr;
}


static IDropTarget* get_droptarget_pointer(HWND hwnd)
{
    IDropTarget *droptarget = NULL;
    HANDLE map;
    IStream *stream;

    map = get_droptarget_local_handle(hwnd);
    if(!map) return NULL;

    if(SUCCEEDED(create_stream_from_map(map, &stream)))
    {
        CoUnmarshalInterface(stream, &IID_IDropTarget, (void**)&droptarget);
        IStream_Release(stream);
    }
    CloseHandle(map);
    return droptarget;
}


/**************************************************************************
 *              query_drag_drop
 */
BOOL query_drag_drop(macdrv_query* query)
{
    BOOL ret = FALSE;
    HWND hwnd = macdrv_get_window_hwnd(query->window);
    struct macdrv_win_data *data = get_win_data(hwnd);
    POINT pt;
    IDropTarget *droptarget;

    TRACE("win %p/%p x,y %d,%d op 0x%08x pasteboard %p\n", hwnd, query->window,
          query->drag_drop.x, query->drag_drop.y, query->drag_drop.op, query->drag_drop.pasteboard);

    if (!data)
    {
        WARN("no win_data for win %p/%p\n", hwnd, query->window);
        return FALSE;
    }

    pt.x = query->drag_drop.x + data->whole_rect.left;
    pt.y = query->drag_drop.y + data->whole_rect.top;
    release_win_data(data);

    droptarget = get_droptarget_pointer(last_droptarget_hwnd);
    if (droptarget)
    {
        HRESULT hr;
        POINTL pointl;
        DWORD effect = drag_operations_to_dropeffects(query->drag_drop.op);

        if (!active_data_object)
        {
            WARN("shouldn't happen: no active IDataObject\n");
            active_data_object = create_data_object_for_pasteboard(query->drag_drop.pasteboard);
        }

        pointl.x = pt.x;
        pointl.y = pt.y;
        TRACE("Drop hwnd %p droptarget %p pointl (%d,%d) effect 0x%08x\n", last_droptarget_hwnd,
              droptarget, pointl.x, pointl.y, effect);
        hr = IDropTarget_Drop(droptarget, active_data_object, MK_LBUTTON, pointl, &effect);
        if (SUCCEEDED(hr))
        {
            if (effect != DROPEFFECT_NONE)
            {
                TRACE("drop succeeded\n");
                ret = TRUE;
            }
            else
                TRACE("the application refused the drop\n");
        }
        else
            WARN("drop failed, error 0x%08X\n", hr);
        IDropTarget_Release(droptarget);
    }
    else
    {
        hwnd = WindowFromPoint(pt);
        while (hwnd && !(GetWindowLongW(hwnd, GWL_EXSTYLE) & WS_EX_ACCEPTFILES))
            hwnd = GetParent(hwnd);
        if (hwnd)
        {
            HDROP hdrop = macdrv_get_pasteboard_data(query->drag_drop.pasteboard, CF_HDROP);
            DROPFILES *dropfiles = GlobalLock(hdrop);
            if (dropfiles)
            {
                dropfiles->pt.x = pt.x;
                dropfiles->pt.y = pt.y;
                dropfiles->fNC  = TRUE;

                TRACE("sending WM_DROPFILES: hwnd %p pt %s %s\n", hwnd, wine_dbgstr_point(&pt),
                      debugstr_w((WCHAR*)((char*)dropfiles + dropfiles->pFiles)));

                GlobalUnlock(hdrop);

                ret = PostMessageW(hwnd, WM_DROPFILES, (WPARAM)hdrop, 0L);
                /* hdrop is owned by the message and freed when the recipient calls DragFinish(). */
            }

            if (!ret) GlobalFree(hdrop);
        }
    }

    if (active_data_object) IDataObject_Release(active_data_object);
    active_data_object = NULL;
    last_droptarget_hwnd = NULL;
    return ret;
}


/**************************************************************************
 *              query_drag_exited
 */
BOOL query_drag_exited(macdrv_query* query)
{
    HWND hwnd = macdrv_get_window_hwnd(query->window);
    IDropTarget *droptarget;

    TRACE("win %p/%p\n", hwnd, query->window);

    droptarget = get_droptarget_pointer(last_droptarget_hwnd);
    if (droptarget)
    {
        HRESULT hr;

        TRACE("DragLeave hwnd %p droptarget %p\n", last_droptarget_hwnd, droptarget);
        hr = IDropTarget_DragLeave(droptarget);
        if (FAILED(hr))
            WARN("IDropTarget_DragLeave failed, error 0x%08X\n", hr);
        IDropTarget_Release(droptarget);
    }

    if (active_data_object) IDataObject_Release(active_data_object);
    active_data_object = NULL;
    last_droptarget_hwnd = NULL;

    return TRUE;
}


/**************************************************************************
 *              query_drag_operation
 */
BOOL query_drag_operation(macdrv_query* query)
{
    BOOL ret = FALSE;
    HWND hwnd = macdrv_get_window_hwnd(query->window);
    struct macdrv_win_data *data = get_win_data(hwnd);
    POINT pt;
    DWORD effect;
    IDropTarget *droptarget;
    HRESULT hr;

    TRACE("win %p/%p x,y %d,%d offered_ops 0x%x pasteboard %p\n", hwnd, query->window,
          query->drag_operation.x, query->drag_operation.y, query->drag_operation.offered_ops,
          query->drag_operation.pasteboard);

    if (!data)
    {
        WARN("no win_data for win %p/%p\n", hwnd, query->window);
        return FALSE;
    }

    pt.x = query->drag_operation.x + data->whole_rect.left;
    pt.y = query->drag_operation.y + data->whole_rect.top;
    release_win_data(data);

    effect = drag_operations_to_dropeffects(query->drag_operation.offered_ops);

    /* Instead of the top-level window we got in the query, start with the deepest
       child under the cursor.  Travel up the hierarchy looking for a window that
       has an associated IDropTarget. */
    hwnd = WindowFromPoint(pt);
    do
    {
        droptarget = get_droptarget_pointer(hwnd);
    } while (!droptarget && (hwnd = GetParent(hwnd)));

    if (last_droptarget_hwnd != hwnd)
    {
        if (last_droptarget_hwnd)
        {
            IDropTarget *old_droptarget = get_droptarget_pointer(last_droptarget_hwnd);
            if (old_droptarget)
            {
                TRACE("DragLeave hwnd %p droptarget %p\n", last_droptarget_hwnd, old_droptarget);
                hr = IDropTarget_DragLeave(old_droptarget);
                if (FAILED(hr))
                    WARN("IDropTarget_DragLeave failed, error 0x%08X\n", hr);
                IDropTarget_Release(old_droptarget);
            }
        }

        last_droptarget_hwnd = hwnd;

        if (droptarget)
        {
            POINTL pointl = { pt.x, pt.y };

            if (!active_data_object)
                active_data_object = create_data_object_for_pasteboard(query->drag_operation.pasteboard);

            TRACE("DragEnter hwnd %p droptarget %p\n", hwnd, droptarget);
            hr = IDropTarget_DragEnter(droptarget, active_data_object, MK_LBUTTON,
                                       pointl, &effect);
            if (SUCCEEDED(hr))
            {
                query->drag_operation.accepted_op = dropeffect_to_drag_operation(effect,
                                                        query->drag_operation.offered_ops);
                TRACE("    effect %d accepted op %d\n", effect, query->drag_operation.accepted_op);
                ret = TRUE;
            }
            else
                WARN("IDropTarget_DragEnter failed, error 0x%08X\n", hr);
            IDropTarget_Release(droptarget);
        }
    }
    else if (droptarget)
    {
        POINTL pointl = { pt.x, pt.y };

        TRACE("DragOver hwnd %p droptarget %p\n", hwnd, droptarget);
        hr = IDropTarget_DragOver(droptarget, MK_LBUTTON, pointl, &effect);
        if (SUCCEEDED(hr))
        {
            query->drag_operation.accepted_op = dropeffect_to_drag_operation(effect,
                                                    query->drag_operation.offered_ops);
            TRACE("    effect %d accepted op %d\n", effect, query->drag_operation.accepted_op);
            ret = TRUE;
        }
        else
            WARN("IDropTarget_DragOver failed, error 0x%08X\n", hr);
        IDropTarget_Release(droptarget);
    }

    if (!ret)
    {
        hwnd = WindowFromPoint(pt);
        while (hwnd && !(GetWindowLongW(hwnd, GWL_EXSTYLE) & WS_EX_ACCEPTFILES))
            hwnd = GetParent(hwnd);
        if (hwnd)
        {
            FORMATETC formatEtc;

            if (!active_data_object)
                active_data_object = create_data_object_for_pasteboard(query->drag_operation.pasteboard);

            formatEtc.cfFormat = CF_HDROP;
            formatEtc.ptd = NULL;
            formatEtc.dwAspect = DVASPECT_CONTENT;
            formatEtc.lindex = -1;
            formatEtc.tymed = TYMED_HGLOBAL;
            if (SUCCEEDED(IDataObject_QueryGetData(active_data_object, &formatEtc)))
            {
                TRACE("WS_EX_ACCEPTFILES hwnd %p\n", hwnd);
                query->drag_operation.accepted_op = DRAG_OP_GENERIC;
                ret = TRUE;
            }
        }
    }

    TRACE(" -> %s\n", ret ? "TRUE" : "FALSE");
    return ret;
}
