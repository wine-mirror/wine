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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "macdrv_dll.h"

#define COBJMACROS
#include "objidl.h"
#include "shellapi.h"
#include "shlobj.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dragdrop);


static IDataObject *active_data_object;
static HWND last_droptarget_hwnd;


typedef struct
{
    IDataObject IDataObject_iface;
    LONG        ref;
    UINT64      pasteboard;
} DragDropDataObject;


/**************************************************************************
 *              debugstr_format
 */
static const char *debugstr_format(UINT id)
{
    WCHAR buffer[256];

    if (GetClipboardFormatNameW(id, buffer, 256))
        return wine_dbg_sprintf("0x%04x %s", id, debugstr_w(buffer));

    switch (id)
    {
#define BUILTIN(id) case id: return #id;
    BUILTIN(CF_TEXT)
    BUILTIN(CF_BITMAP)
    BUILTIN(CF_METAFILEPICT)
    BUILTIN(CF_SYLK)
    BUILTIN(CF_DIF)
    BUILTIN(CF_TIFF)
    BUILTIN(CF_OEMTEXT)
    BUILTIN(CF_DIB)
    BUILTIN(CF_PALETTE)
    BUILTIN(CF_PENDATA)
    BUILTIN(CF_RIFF)
    BUILTIN(CF_WAVE)
    BUILTIN(CF_UNICODETEXT)
    BUILTIN(CF_ENHMETAFILE)
    BUILTIN(CF_HDROP)
    BUILTIN(CF_LOCALE)
    BUILTIN(CF_DIBV5)
    BUILTIN(CF_OWNERDISPLAY)
    BUILTIN(CF_DSPTEXT)
    BUILTIN(CF_DSPBITMAP)
    BUILTIN(CF_DSPMETAFILEPICT)
    BUILTIN(CF_DSPENHMETAFILE)
#undef BUILTIN
    default: return wine_dbg_sprintf("0x%04x", id);
    }
}

static inline DragDropDataObject *impl_from_IDataObject(IDataObject *iface)
{
    return CONTAINING_RECORD(iface, DragDropDataObject, IDataObject_iface);
}


static HANDLE get_pasteboard_data(UINT64 pasteboard, UINT desired_format)
{
    struct dnd_get_data_params params = { .handle = pasteboard, .format = desired_format, .size = 2048 };
    HANDLE handle;
    NTSTATUS status;

    for (;;)
    {
        if (!(handle = GlobalAlloc(GMEM_FIXED, params.size))) return 0;
        params.data = GlobalLock(handle);
        status = MACDRV_CALL(dnd_get_data, &params);
        GlobalUnlock(handle);
        if (!status) return GlobalReAlloc(handle, params.size, GMEM_MOVEABLE);
        GlobalFree(handle);
        if (status != STATUS_BUFFER_OVERFLOW) return 0;
    }
}

static HRESULT WINAPI dddo_QueryInterface(IDataObject* iface, REFIID riid, LPVOID *ppvObj)
{
    DragDropDataObject *This = impl_from_IDataObject(iface);

    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), ppvObj);

    if (IsEqualIID(riid, &IID_IUnknown) || (IsEqualIID(riid, &IID_IDataObject)))
    {
        *ppvObj = iface;
        IDataObject_AddRef(iface);
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}


static ULONG WINAPI dddo_AddRef(IDataObject* iface)
{
    DragDropDataObject *This = impl_from_IDataObject(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(count=%lu)\n", This, refCount - 1);

    return refCount;
}


static ULONG WINAPI dddo_Release(IDataObject* iface)
{
    DragDropDataObject *This = impl_from_IDataObject(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(count=%lu)\n", This, refCount + 1);
    if (refCount)
        return refCount;

    TRACE("-- destroying DragDropDataObject (%p)\n", This);
    MACDRV_CALL(dnd_release, &This->pasteboard);
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
        medium->hGlobal = get_pasteboard_data(This->pasteboard, formatEtc->cfFormat);
        medium->pUnkForRelease = NULL;
        hr = medium->hGlobal ? S_OK : E_OUTOFMEMORY;
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
    struct dnd_have_format_params params;
    HRESULT hr = DV_E_FORMATETC;

    TRACE("This %p formatEtc %p={.tymed=0x%lx, .dwAspect=%ld, .cfFormat=%s}\n",
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

    params.handle = This->pasteboard;
    params.format = formatEtc->cfFormat;
    if (MACDRV_CALL(dnd_have_format, &params))
        hr = S_OK;

    TRACE(" -> 0x%lx\n", hr);
    return hr;
}


static HRESULT WINAPI dddo_GetConicalFormatEtc(IDataObject* iface, FORMATETC* formatEtc,
                                               FORMATETC* formatEtcOut)
{
    DragDropDataObject *This = impl_from_IDataObject(iface);

    TRACE("This %p formatEtc %p={.tymed=0x%lx, .dwAspect=%ld, .cfFormat=%s}\n",
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

    TRACE("This %p formatEtc %p={.tymed=0x%lx, .dwAspect=%ld, .cfFormat=%s} medium %p fRelease %d\n",
          This, formatEtc, formatEtc->tymed, formatEtc->dwAspect,
          debugstr_format(formatEtc->cfFormat), medium, fRelease);

    return E_NOTIMPL;
}


static HRESULT WINAPI dddo_EnumFormatEtc(IDataObject* iface, DWORD direction,
                                         IEnumFORMATETC** enumFormatEtc)
{
    DragDropDataObject *This = impl_from_IDataObject(iface);
    struct dnd_get_formats_params params;
    UINT count;
    HRESULT hr;

    TRACE("This %p direction %lu enumFormatEtc %p\n", This, direction, enumFormatEtc);

    if (direction != DATADIR_GET)
    {
        WARN("only the get direction is implemented\n");
        return E_NOTIMPL;
    }

    params.handle = This->pasteboard;
    count = MACDRV_CALL(dnd_get_formats, &params);
    if (count)
    {
        FORMATETC *formatEtcs = HeapAlloc(GetProcessHeap(), 0, count * sizeof(FORMATETC));
        if (formatEtcs)
        {
            INT i;

            for (i = 0; i < count; i++)
            {
                formatEtcs[i].cfFormat = params.formats[i];
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
    }
    else
        hr = SHCreateStdEnumFmtEtc(0, NULL, enumFormatEtc);

    TRACE(" -> 0x%lx\n", hr);
    return hr;
}


static HRESULT WINAPI dddo_DAdvise(IDataObject* iface, FORMATETC* formatEtc, DWORD advf,
                                   IAdviseSink* pAdvSink, DWORD* pdwConnection)
{
    FIXME("(%p, %p, %lu, %p, %p): stub\n", iface, formatEtc, advf,
          pAdvSink, pdwConnection);
    return OLE_E_ADVISENOTSUPPORTED;
}


static HRESULT WINAPI dddo_DUnadvise(IDataObject* iface, DWORD dwConnection)
{
    FIXME("(%p, %lu): stub\n", iface, dwConnection);
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


static IDataObject *create_data_object_for_pasteboard(UINT64 pasteboard)
{
    DragDropDataObject *dddo;

    dddo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*dddo));
    if (!dddo)
        return NULL;

    dddo->ref = 1;
    dddo->IDataObject_iface.lpVtbl = &dovt;
    dddo->pasteboard = pasteboard;
    MACDRV_CALL(dnd_retain, &dddo->pasteboard);

    return &dddo->IDataObject_iface;
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
 *              macdrv_dnd_query_drop
 */
NTSTATUS WINAPI macdrv_dnd_query_drop(void *arg, ULONG size)
{
    struct dnd_query_drop_params *params = arg;
    IDropTarget *droptarget;
    BOOL ret = FALSE;
    POINT pt;

    TRACE("win %x x,y %d,%d effect %x pasteboard %s\n", params->hwnd, params->x, params->y,
          params->effect, wine_dbgstr_longlong(params->handle));

    pt.x = params->x;
    pt.y = params->y;

    droptarget = get_droptarget_pointer(last_droptarget_hwnd);
    if (droptarget)
    {
        HRESULT hr;
        POINTL pointl;
        DWORD effect = params->effect;

        if (!active_data_object)
        {
            WARN("shouldn't happen: no active IDataObject\n");
            active_data_object = create_data_object_for_pasteboard(params->handle);
        }

        pointl.x = pt.x;
        pointl.y = pt.y;
        TRACE("Drop hwnd %p droptarget %p pointl (%ld,%ld) effect 0x%08lx\n", last_droptarget_hwnd,
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
            WARN("drop failed, error 0x%08lx\n", hr);
        IDropTarget_Release(droptarget);
    }
    else
    {
        HWND hwnd = WindowFromPoint(pt);
        while (hwnd && !(GetWindowLongW(hwnd, GWL_EXSTYLE) & WS_EX_ACCEPTFILES))
            hwnd = GetParent(hwnd);
        if (hwnd)
        {
            HDROP hdrop = get_pasteboard_data(params->handle, CF_HDROP);
            DROPFILES *dropfiles = GlobalLock(hdrop);
            if (dropfiles)
            {
                RECT rect;
                dropfiles->pt.x = pt.x;
                dropfiles->pt.y = pt.y;
                dropfiles->fNC  = !(ScreenToClient(hwnd, &dropfiles->pt) &&
                                    GetClientRect(hwnd, &rect) &&
                                    PtInRect(&rect, dropfiles->pt));

                TRACE("sending WM_DROPFILES: hwnd %p nc %d pt %s %s\n", hwnd,
                      dropfiles->fNC, wine_dbgstr_point(&dropfiles->pt),
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
    return NtCallbackReturn( &ret, sizeof(ret), STATUS_SUCCESS );
}


/**************************************************************************
 *              macdrv_dnd_query_exited
 */
NTSTATUS WINAPI macdrv_dnd_query_exited(void *arg, ULONG size)
{
    struct dnd_query_exited_params *params = arg;
    HWND hwnd = UlongToHandle(params->hwnd);
    IDropTarget *droptarget;
    BOOL ret = TRUE;

    TRACE("win %p\n", hwnd);

    droptarget = get_droptarget_pointer(last_droptarget_hwnd);
    if (droptarget)
    {
        HRESULT hr;

        TRACE("DragLeave hwnd %p droptarget %p\n", last_droptarget_hwnd, droptarget);
        hr = IDropTarget_DragLeave(droptarget);
        if (FAILED(hr))
            WARN("IDropTarget_DragLeave failed, error 0x%08lx\n", hr);
        IDropTarget_Release(droptarget);
    }

    if (active_data_object) IDataObject_Release(active_data_object);
    active_data_object = NULL;
    last_droptarget_hwnd = NULL;
    return NtCallbackReturn( &ret, sizeof(ret), STATUS_SUCCESS );
}


/**************************************************************************
 *              query_drag_operation
 */
NTSTATUS WINAPI macdrv_dnd_query_drag(void *arg, ULONG size)
{
    struct dnd_query_drag_params *params = arg;
    HWND hwnd = UlongToHandle(params->hwnd);
    BOOL ret = FALSE;
    POINT pt;
    DWORD effect;
    IDropTarget *droptarget;
    HRESULT hr;

    TRACE("win %p x,y %d,%d effect %x pasteboard %s\n", hwnd, params->x, params->y,
          params->effect, wine_dbgstr_longlong(params->handle));

    pt.x = params->x;
    pt.y = params->y;
    effect = params->effect;

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
                    WARN("IDropTarget_DragLeave failed, error 0x%08lx\n", hr);
                IDropTarget_Release(old_droptarget);
            }
        }

        last_droptarget_hwnd = hwnd;

        if (droptarget)
        {
            POINTL pointl = { pt.x, pt.y };

            if (!active_data_object)
                active_data_object = create_data_object_for_pasteboard(params->handle);

            TRACE("DragEnter hwnd %p droptarget %p\n", hwnd, droptarget);
            hr = IDropTarget_DragEnter(droptarget, active_data_object, MK_LBUTTON,
                                       pointl, &effect);
            if (SUCCEEDED(hr))
            {
                TRACE("    effect %ld\n", effect);
                ret = TRUE;
            }
            else
                WARN("IDropTarget_DragEnter failed, error 0x%08lx\n", hr);
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
            TRACE("    effect %ld\n", effect);
            ret = TRUE;
        }
        else
            WARN("IDropTarget_DragOver failed, error 0x%08lx\n", hr);
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
                active_data_object = create_data_object_for_pasteboard(params->handle);

            formatEtc.cfFormat = CF_HDROP;
            formatEtc.ptd = NULL;
            formatEtc.dwAspect = DVASPECT_CONTENT;
            formatEtc.lindex = -1;
            formatEtc.tymed = TYMED_HGLOBAL;
            if (SUCCEEDED(IDataObject_QueryGetData(active_data_object, &formatEtc)))
            {
                TRACE("WS_EX_ACCEPTFILES hwnd %p\n", hwnd);
                effect = DROPEFFECT_COPY | DROPEFFECT_LINK;
                ret = TRUE;
            }
        }
    }

    TRACE(" -> %s\n", ret ? "TRUE" : "FALSE");
    if (!ret) effect = 0;
    return NtCallbackReturn( &effect, sizeof(effect), STATUS_SUCCESS );
}
