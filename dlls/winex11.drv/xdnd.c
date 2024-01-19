/*
 * XDND handler code
 *
 * Copyright 2003 Ulrich Czekalla
 * Copyright 2007 Damjan Jovanovic
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
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "x11drv_dll.h"
#include "shellapi.h"
#include "shlobj.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(xdnd);

static struct format_entry *xdnd_formats, *xdnd_formats_end;
static POINT XDNDxy = { 0, 0 };
static IDataObject XDNDDataObject;
static BOOL XDNDAccepted = FALSE;
static DWORD XDNDDropEffect = DROPEFFECT_NONE;
/* the last window the mouse was over */
static HWND XDNDLastTargetWnd;
/* might be an ancestor of XDNDLastTargetWnd */
static HWND XDNDLastDropTargetWnd;

static BOOL X11DRV_XDND_HasHDROP(void);
static HRESULT X11DRV_XDND_SendDropFiles(HWND hwnd);
static void X11DRV_XDND_FreeDragDropOp(void);

static CRITICAL_SECTION xdnd_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &xdnd_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": xdnd_cs") }
};
static CRITICAL_SECTION xdnd_cs = { &critsect_debug, -1, 0, 0, 0, 0 };


static struct format_entry *next_format( struct format_entry *entry )
{
    return (struct format_entry *)&entry->data[(entry->size + 7) & ~7];
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


/* Recursively searches for a window on given coordinates in a drag&drop specific manner.
 *
 * Don't use WindowFromPoint instead, because it omits the STATIC and transparent
 * windows, but they can be a valid drop targets if have WS_EX_ACCEPTFILES set.
 */
static HWND window_from_point_dnd(HWND hwnd, POINT point)
{
    HWND child;
    ScreenToClient(hwnd, &point);
    while ((child = ChildWindowFromPointEx(hwnd, point, CWP_SKIPDISABLED | CWP_SKIPINVISIBLE)) && child != hwnd)
    {
       MapWindowPoints(hwnd, child, &point, 1);
       hwnd = child;
    }

    return hwnd;
}

/* Returns the first window down the hierarchy that has WS_EX_ACCEPTFILES set or
 * returns NULL, if such window does not exists.
 */
static HWND window_accepting_files(HWND hwnd)
{
    while (hwnd && !(GetWindowLongW(hwnd, GWL_EXSTYLE) & WS_EX_ACCEPTFILES))
        /* MUST to be GetParent, not GetAncestor, because the owner window
         * (with WS_EX_ACCEPTFILES) of a window with WS_POPUP is a valid
         * drop target. GetParent works exactly this way!
         */
        hwnd = GetParent(hwnd);
    return hwnd;
}

/**************************************************************************
 *           x11drv_dnd_position_event
 *
 * Handle an XdndPosition event.
 */
NTSTATUS WINAPI x11drv_dnd_position_event( void *arg, ULONG size )
{
    struct dnd_position_event_params *params = arg;
    int accept = 0; /* Assume we're not accepting */
    IDropTarget *dropTarget = NULL;
    DWORD effect = params->effect;
    POINTL pointl = { .x = params->point.x, .y = params->point.y };
    HWND targetWindow;
    HRESULT hr;

    XDNDxy = params->point;
    targetWindow = window_from_point_dnd( UlongToHandle( params->hwnd ), XDNDxy );

    if (!XDNDAccepted || XDNDLastTargetWnd != targetWindow)
    {
        /* Notify OLE of DragEnter. Result determines if we accept */
        HWND dropTargetWindow;

        if (XDNDAccepted && XDNDLastDropTargetWnd)
        {
            dropTarget = get_droptarget_pointer(XDNDLastDropTargetWnd);
            if (dropTarget)
            {
                hr = IDropTarget_DragLeave(dropTarget);
                if (FAILED(hr))
                    WARN("IDropTarget_DragLeave failed, error 0x%08lx\n", hr);
                IDropTarget_Release(dropTarget);
            }
        }
        dropTargetWindow = targetWindow;
        do
        {
            dropTarget = get_droptarget_pointer(dropTargetWindow);
        } while (dropTarget == NULL && (dropTargetWindow = GetParent(dropTargetWindow)) != NULL);
        XDNDLastTargetWnd = targetWindow;
        XDNDLastDropTargetWnd = dropTargetWindow;
        if (dropTarget)
        {
            DWORD effect_ignore = effect;
            hr = IDropTarget_DragEnter(dropTarget, &XDNDDataObject,
                                       MK_LBUTTON, pointl, &effect_ignore);
            if (hr == S_OK)
            {
                XDNDAccepted = TRUE;
                TRACE("the application accepted the drop (effect = %ld)\n", effect_ignore);
            }
            else
            {
                XDNDAccepted = FALSE;
                WARN("IDropTarget_DragEnter failed, error 0x%08lx\n", hr);
            }
            IDropTarget_Release(dropTarget);
        }
    }
    if (XDNDAccepted && XDNDLastTargetWnd == targetWindow)
    {
        /* If drag accepted notify OLE of DragOver */
        dropTarget = get_droptarget_pointer(XDNDLastDropTargetWnd);
        if (dropTarget)
        {
            hr = IDropTarget_DragOver(dropTarget, MK_LBUTTON, pointl, &effect);
            if (hr == S_OK)
                XDNDDropEffect = effect;
            else
                WARN("IDropTarget_DragOver failed, error 0x%08lx\n", hr);
            IDropTarget_Release(dropTarget);
        }
    }

    if (XDNDAccepted && XDNDDropEffect != DROPEFFECT_NONE)
        accept = 1;
    else
    {
        /* fallback search for window able to accept these files. */

        if (window_accepting_files(targetWindow) && X11DRV_XDND_HasHDROP())
        {
            accept = 1;
            effect = DROPEFFECT_COPY;
        }
    }

    if (!accept) effect = DROPEFFECT_NONE;
    return NtCallbackReturn( &effect, sizeof(effect), STATUS_SUCCESS );
}

NTSTATUS WINAPI x11drv_dnd_drop_event( void *args, ULONG size )
{
    HWND hwnd = UlongToHandle( *(ULONG *)args );
    IDropTarget *dropTarget;
    DWORD effect = XDNDDropEffect;
    int accept = 0; /* Assume we're not accepting */
    BOOL drop_file = TRUE;

    /* Notify OLE of Drop */
    if (XDNDAccepted)
    {
        dropTarget = get_droptarget_pointer(XDNDLastDropTargetWnd);
        if (dropTarget && effect!=DROPEFFECT_NONE)
        {
            HRESULT hr;
            POINTL pointl;

            pointl.x = XDNDxy.x;
            pointl.y = XDNDxy.y;
            hr = IDropTarget_Drop(dropTarget, &XDNDDataObject, MK_LBUTTON,
                                  pointl, &effect);
            if (hr == S_OK)
            {
                if (effect != DROPEFFECT_NONE)
                {
                    TRACE("drop succeeded\n");
                    accept = 1;
                    drop_file = FALSE;
                }
                else
                    TRACE("the application refused the drop\n");
            }
            else if (FAILED(hr))
                WARN("drop failed, error 0x%08lx\n", hr);
            else
            {
                WARN("drop returned 0x%08lx\n", hr);
                drop_file = FALSE;
            }
            IDropTarget_Release(dropTarget);
        }
        else if (dropTarget)
        {
            HRESULT hr = IDropTarget_DragLeave(dropTarget);
            if (FAILED(hr))
                WARN("IDropTarget_DragLeave failed, error 0x%08lx\n", hr);
            IDropTarget_Release(dropTarget);
        }
    }

    if (drop_file)
    {
        /* Only send WM_DROPFILES if Drop didn't succeed or DROPEFFECT_NONE was set.
         * Doing both causes winamp to duplicate the dropped files (#29081) */

        HWND hwnd_drop = window_accepting_files(window_from_point_dnd( hwnd, XDNDxy ));

        if (hwnd_drop && X11DRV_XDND_HasHDROP())
        {
            HRESULT hr = X11DRV_XDND_SendDropFiles(hwnd_drop);
            if (SUCCEEDED(hr))
            {
                accept = 1;
                effect = DROPEFFECT_COPY;
            }
        }
    }

    TRACE("effectRequested(0x%lx) accept(%d) performed(0x%lx) at x(%ld),y(%ld)\n",
          XDNDDropEffect, accept, effect, XDNDxy.x, XDNDxy.y);

    if (!accept) effect = DROPEFFECT_NONE;
    return NtCallbackReturn( &effect, sizeof(effect), STATUS_SUCCESS );
}

/**************************************************************************
 *           x11drv_dnd_leave_event
 *
 * Handle an XdndLeave event.
 */
NTSTATUS WINAPI x11drv_dnd_leave_event( void *params, ULONG size )
{
    IDropTarget *dropTarget;

    TRACE("DND Operation canceled\n");

    /* Notify OLE of DragLeave */
    if (XDNDAccepted)
    {
        dropTarget = get_droptarget_pointer(XDNDLastDropTargetWnd);
        if (dropTarget)
        {
            HRESULT hr = IDropTarget_DragLeave(dropTarget);
            if (FAILED(hr))
                WARN("IDropTarget_DragLeave failed, error 0x%08lx\n", hr);
            IDropTarget_Release(dropTarget);
        }
    }

    X11DRV_XDND_FreeDragDropOp();
    return STATUS_SUCCESS;
}


/**************************************************************************
 *           x11drv_dnd_enter_event
 */
NTSTATUS WINAPI x11drv_dnd_enter_event( void *params, ULONG size )
{
    struct format_entry *formats = params;
    XDNDAccepted = FALSE;
    X11DRV_XDND_FreeDragDropOp(); /* Clear previously cached data */

    if ((xdnd_formats = HeapAlloc( GetProcessHeap(), 0, size )))
    {
        memcpy( xdnd_formats, formats, size );
        xdnd_formats_end = (struct format_entry *)((char *)xdnd_formats + size);
    }
    return STATUS_SUCCESS;
}


/**************************************************************************
 * X11DRV_XDND_HasHDROP
 */
static BOOL X11DRV_XDND_HasHDROP(void)
{
    struct format_entry *iter;
    BOOL found = FALSE;

    EnterCriticalSection(&xdnd_cs);

    /* Find CF_HDROP type if any */
    for (iter = xdnd_formats; iter < xdnd_formats_end; iter = next_format( iter ))
    {
        if (iter->format == CF_HDROP)
        {
            found = TRUE;
            break;
        }
    }

    LeaveCriticalSection(&xdnd_cs);

    return found;
}

/**************************************************************************
 * X11DRV_XDND_SendDropFiles
 */
static HRESULT X11DRV_XDND_SendDropFiles(HWND hwnd)
{
    struct format_entry *iter;
    HRESULT hr;
    BOOL found = FALSE;

    EnterCriticalSection(&xdnd_cs);

    for (iter = xdnd_formats; iter < xdnd_formats_end; iter = next_format( iter ))
    {
         if (iter->format == CF_HDROP)
         {
             found = TRUE;
             break;
         }
    }
    if (found)
    {
        HGLOBAL dropHandle = GlobalAlloc(GMEM_FIXED, iter->size);
        if (dropHandle)
        {
            RECT rect;
            DROPFILES *lpDrop = GlobalLock(dropHandle);
            memcpy(lpDrop, iter->data, iter->size);
            lpDrop->pt.x = XDNDxy.x;
            lpDrop->pt.y = XDNDxy.y;
            lpDrop->fNC  = !(ScreenToClient(hwnd, &lpDrop->pt) &&
                             GetClientRect(hwnd, &rect) &&
                             PtInRect(&rect, lpDrop->pt));
            TRACE("Sending WM_DROPFILES: hWnd=0x%p, fNC=%d, x=%ld, y=%ld, files=%p(%s)\n", hwnd,
                    lpDrop->fNC, lpDrop->pt.x, lpDrop->pt.y, ((char*)lpDrop) + lpDrop->pFiles,
                    debugstr_w((WCHAR*)(((char*)lpDrop) + lpDrop->pFiles)));
            GlobalUnlock(dropHandle);
            if (PostMessageW(hwnd, WM_DROPFILES, (WPARAM)dropHandle, 0))
                hr = S_OK;
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                GlobalFree(dropHandle);
            }
        }
        else
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
        hr = E_FAIL;

    LeaveCriticalSection(&xdnd_cs);

    return hr;
}


/**************************************************************************
 * X11DRV_XDND_FreeDragDropOp
 */
static void X11DRV_XDND_FreeDragDropOp(void)
{
    TRACE("\n");

    EnterCriticalSection(&xdnd_cs);

    HeapFree( GetProcessHeap(), 0, xdnd_formats );
    xdnd_formats = xdnd_formats_end = NULL;

    XDNDxy.x = XDNDxy.y = 0;
    XDNDLastTargetWnd = NULL;
    XDNDLastDropTargetWnd = NULL;
    XDNDAccepted = FALSE;

    LeaveCriticalSection(&xdnd_cs);
}


/**************************************************************************
 * X11DRV_XDND_DescribeClipboardFormat
 */
static void X11DRV_XDND_DescribeClipboardFormat(int cfFormat, char *buffer, int size)
{
#define D(x) case x: lstrcpynA(buffer, #x, size); return;
    switch (cfFormat)
    {
        D(CF_TEXT)
        D(CF_BITMAP)
        D(CF_METAFILEPICT)
        D(CF_SYLK)
        D(CF_DIF)
        D(CF_TIFF)
        D(CF_OEMTEXT)
        D(CF_DIB)
        D(CF_PALETTE)
        D(CF_PENDATA)
        D(CF_RIFF)
        D(CF_WAVE)
        D(CF_UNICODETEXT)
        D(CF_ENHMETAFILE)
        D(CF_HDROP)
        D(CF_LOCALE)
        D(CF_DIBV5)
    }
#undef D

    if (CF_PRIVATEFIRST <= cfFormat && cfFormat <= CF_PRIVATELAST)
    {
        lstrcpynA(buffer, "some private object", size);
        return;
    }
    if (CF_GDIOBJFIRST <= cfFormat && cfFormat <= CF_GDIOBJLAST)
    {
        lstrcpynA(buffer, "some GDI object", size);
        return;
    }

    GetClipboardFormatNameA(cfFormat, buffer, size);
}


/* The IDataObject singleton we feed to OLE follows */

static HRESULT WINAPI XDNDDATAOBJECT_QueryInterface(IDataObject *dataObject,
                                                    REFIID riid, void **ppvObject)
{
    TRACE("(%p, %s, %p)\n", dataObject, debugstr_guid(riid), ppvObject);
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDataObject))
    {
        *ppvObject = dataObject;
        IDataObject_AddRef(dataObject);
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI XDNDDATAOBJECT_AddRef(IDataObject *dataObject)
{
    TRACE("(%p)\n", dataObject);
    return 2;
}

static ULONG WINAPI XDNDDATAOBJECT_Release(IDataObject *dataObject)
{
    TRACE("(%p)\n", dataObject);
    return 1;
}

static HRESULT WINAPI XDNDDATAOBJECT_GetData(IDataObject *dataObject,
                                             FORMATETC *formatEtc,
                                             STGMEDIUM *pMedium)
{
    HRESULT hr;
    char formatDesc[1024];

    TRACE("(%p, %p, %p)\n", dataObject, formatEtc, pMedium);
    X11DRV_XDND_DescribeClipboardFormat(formatEtc->cfFormat,
        formatDesc, sizeof(formatDesc));
    TRACE("application is looking for %s\n", formatDesc);

    hr = IDataObject_QueryGetData(dataObject, formatEtc);
    if (SUCCEEDED(hr))
    {
        struct format_entry *iter;

        for (iter = xdnd_formats; iter < xdnd_formats_end; iter = next_format( iter ))
        {
            if (iter->format == formatEtc->cfFormat)
            {
                pMedium->tymed = TYMED_HGLOBAL;
                pMedium->hGlobal = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, iter->size);
                if (pMedium->hGlobal == NULL)
                    return E_OUTOFMEMORY;
                memcpy(GlobalLock(pMedium->hGlobal), iter->data, iter->size);
                GlobalUnlock(pMedium->hGlobal);
                pMedium->pUnkForRelease = 0;
                return S_OK;
            }
        }
    }
    return hr;
}

static HRESULT WINAPI XDNDDATAOBJECT_GetDataHere(IDataObject *dataObject,
                                                 FORMATETC *formatEtc,
                                                 STGMEDIUM *pMedium)
{
    FIXME("(%p, %p, %p): stub\n", dataObject, formatEtc, pMedium);
    return DATA_E_FORMATETC;
}

static HRESULT WINAPI XDNDDATAOBJECT_QueryGetData(IDataObject *dataObject,
                                                  FORMATETC *formatEtc)
{
    struct format_entry *iter;
    char formatDesc[1024];

    TRACE("(%p, %p={.tymed=0x%lx, .dwAspect=%ld, .cfFormat=%d}\n",
        dataObject, formatEtc, formatEtc->tymed, formatEtc->dwAspect, formatEtc->cfFormat);
    X11DRV_XDND_DescribeClipboardFormat(formatEtc->cfFormat, formatDesc, sizeof(formatDesc));

    if (formatEtc->tymed && !(formatEtc->tymed & TYMED_HGLOBAL))
    {
        FIXME("only HGLOBAL medium types supported right now\n");
        return DV_E_TYMED;
    }
    /* Windows Explorer ignores .dwAspect and .lindex for CF_HDROP,
     * and we have no way to implement them on XDnD anyway, so ignore them too.
     */

    for (iter = xdnd_formats; iter < xdnd_formats_end; iter = next_format( iter ))
    {
        if (iter->format == formatEtc->cfFormat)
        {
            TRACE("application found %s\n", formatDesc);
            return S_OK;
        }
    }
    TRACE("application didn't find %s\n", formatDesc);
    return DV_E_FORMATETC;
}

static HRESULT WINAPI XDNDDATAOBJECT_GetCanonicalFormatEtc(IDataObject *dataObject,
                                                           FORMATETC *formatEtc,
                                                           FORMATETC *formatEtcOut)
{
    FIXME("(%p, %p, %p): stub\n", dataObject, formatEtc, formatEtcOut);
    formatEtcOut->ptd = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI XDNDDATAOBJECT_SetData(IDataObject *dataObject,
                                             FORMATETC *formatEtc,
                                             STGMEDIUM *pMedium, BOOL fRelease)
{
    FIXME("(%p, %p, %p, %s): stub\n", dataObject, formatEtc,
        pMedium, fRelease?"TRUE":"FALSE");
    return E_NOTIMPL;
}

static HRESULT WINAPI XDNDDATAOBJECT_EnumFormatEtc(IDataObject *dataObject,
                                                   DWORD dwDirection,
                                                   IEnumFORMATETC **ppEnumFormatEtc)
{
    struct format_entry *iter;
    DWORD count = 0;
    FORMATETC *formats;

    TRACE("(%p, %lu, %p)\n", dataObject, dwDirection, ppEnumFormatEtc);

    if (dwDirection != DATADIR_GET)
    {
        FIXME("only the get direction is implemented\n");
        return E_NOTIMPL;
    }

    for (iter = xdnd_formats; iter < xdnd_formats_end; iter = next_format( iter )) count++;

    formats = HeapAlloc(GetProcessHeap(), 0, count * sizeof(FORMATETC));
    if (formats)
    {
        DWORD i = 0;
        HRESULT hr;
        for (iter = xdnd_formats; iter < xdnd_formats_end; iter = next_format( iter ))
        {
            formats[i].cfFormat = iter->format;
            formats[i].ptd = NULL;
            formats[i].dwAspect = DVASPECT_CONTENT;
            formats[i].lindex = -1;
            formats[i].tymed = TYMED_HGLOBAL;
            i++;
        }
        hr = SHCreateStdEnumFmtEtc(count, formats, ppEnumFormatEtc);
        HeapFree(GetProcessHeap(), 0, formats);
        return hr;
    }
    else
        return E_OUTOFMEMORY;
}

static HRESULT WINAPI XDNDDATAOBJECT_DAdvise(IDataObject *dataObject,
                                             FORMATETC *formatEtc, DWORD advf,
                                             IAdviseSink *adviseSink,
                                             DWORD *pdwConnection)
{
    FIXME("(%p, %p, %lu, %p, %p): stub\n", dataObject, formatEtc, advf,
        adviseSink, pdwConnection);
    return OLE_E_ADVISENOTSUPPORTED;
}

static HRESULT WINAPI XDNDDATAOBJECT_DUnadvise(IDataObject *dataObject,
                                               DWORD dwConnection)
{
    FIXME("(%p, %lu): stub\n", dataObject, dwConnection);
    return OLE_E_ADVISENOTSUPPORTED;
}

static HRESULT WINAPI XDNDDATAOBJECT_EnumDAdvise(IDataObject *dataObject,
                                                 IEnumSTATDATA **pEnumAdvise)
{
    FIXME("(%p, %p): stub\n", dataObject, pEnumAdvise);
    return OLE_E_ADVISENOTSUPPORTED;
}

static IDataObjectVtbl xdndDataObjectVtbl =
{
    XDNDDATAOBJECT_QueryInterface,
    XDNDDATAOBJECT_AddRef,
    XDNDDATAOBJECT_Release,
    XDNDDATAOBJECT_GetData,
    XDNDDATAOBJECT_GetDataHere,
    XDNDDATAOBJECT_QueryGetData,
    XDNDDATAOBJECT_GetCanonicalFormatEtc,
    XDNDDATAOBJECT_SetData,
    XDNDDATAOBJECT_EnumFormatEtc,
    XDNDDATAOBJECT_DAdvise,
    XDNDDATAOBJECT_DUnadvise,
    XDNDDATAOBJECT_EnumDAdvise
};

static IDataObject XDNDDataObject = { &xdndDataObjectVtbl };

NTSTATUS WINAPI x11drv_dnd_post_drop( void *data, ULONG size )
{
    HDROP handle;

    if ((handle = GlobalAlloc( GMEM_SHARE, size )))
    {
        DROPFILES *ptr = GlobalLock( handle );
        HWND hwnd;
        memcpy( ptr, data, size );
        hwnd = UlongToHandle( ptr->fWide );
        ptr->fWide = TRUE;
        GlobalUnlock( handle );
        PostMessageW( hwnd, WM_DROPFILES, (WPARAM)handle, 0 );
    }

    return STATUS_SUCCESS;
}
