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

static POINT XDNDxy = { 0, 0 };
static IDataObject *xdnd_data_object;
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

static const char *debugstr_format( int format )
{
    WCHAR buffer[256];
    switch (format)
    {
#define X(x) case x: return #x;
    X(CF_TEXT)
    X(CF_BITMAP)
    X(CF_METAFILEPICT)
    X(CF_SYLK)
    X(CF_DIF)
    X(CF_TIFF)
    X(CF_OEMTEXT)
    X(CF_DIB)
    X(CF_PALETTE)
    X(CF_PENDATA)
    X(CF_RIFF)
    X(CF_WAVE)
    X(CF_UNICODETEXT)
    X(CF_ENHMETAFILE)
    X(CF_HDROP)
    X(CF_LOCALE)
    X(CF_DIBV5)
#undef X
    }

    if (CF_PRIVATEFIRST <= format && format <= CF_PRIVATELAST) return "some private object";
    if (CF_GDIOBJFIRST <= format && format <= CF_GDIOBJLAST) return "some GDI object";
    GetClipboardFormatNameW( format, buffer, sizeof(buffer) );
    return debugstr_w( buffer );
}

struct data_object
{
    IDataObject IDataObject_iface;
    LONG refcount;

    struct format_entry *entries_end;
    struct format_entry entries[];
};

static struct data_object *data_object_from_IDataObject( IDataObject *iface )
{
    return CONTAINING_RECORD( iface, struct data_object, IDataObject_iface );
}

static HRESULT WINAPI data_object_QueryInterface( IDataObject *iface, REFIID iid, void **obj )
{
    struct data_object *object = data_object_from_IDataObject( iface );

    TRACE( "object %p, iid %s, obj %p\n", object, debugstr_guid(iid), obj );

    if (IsEqualIID( iid, &IID_IUnknown ) || IsEqualIID( iid, &IID_IDataObject ))
    {
        IDataObject_AddRef( &object->IDataObject_iface );
        *obj = &object->IDataObject_iface;
        return S_OK;
    }

    *obj = NULL;
    WARN( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid) );
    return E_NOINTERFACE;
}

static ULONG WINAPI data_object_AddRef( IDataObject *iface )
{
    struct data_object *object = data_object_from_IDataObject( iface );
    ULONG ref = InterlockedIncrement( &object->refcount );
    TRACE( "object %p increasing refcount to %lu.\n", object, ref );
    return ref;
}

static ULONG WINAPI data_object_Release( IDataObject *iface )
{
    struct data_object *object = data_object_from_IDataObject( iface );
    ULONG ref = InterlockedDecrement( &object->refcount );
    TRACE( "object %p decreasing refcount to %lu.\n", object, ref );
    if (!ref) free( object );
    return ref;
}

static HRESULT WINAPI data_object_GetData( IDataObject *iface, FORMATETC *format, STGMEDIUM *medium )
{
    struct data_object *object = data_object_from_IDataObject( iface );
    struct format_entry *iter;
    HRESULT hr;

    TRACE( "object %p, format %p (%s), medium %p\n", object, format, debugstr_format(format->cfFormat), medium );

    if (FAILED(hr = IDataObject_QueryGetData( iface, format ))) return hr;

    for (iter = object->entries; iter < object->entries_end; iter = next_format( iter ))
    {
        if (iter->format == format->cfFormat)
        {
            medium->tymed = TYMED_HGLOBAL;
            medium->hGlobal = GlobalAlloc( GMEM_FIXED | GMEM_ZEROINIT, iter->size );
            if (medium->hGlobal == NULL) return E_OUTOFMEMORY;
            memcpy( GlobalLock( medium->hGlobal ), iter->data, iter->size );
            GlobalUnlock( medium->hGlobal );
            medium->pUnkForRelease = 0;
            return S_OK;
        }
    }

    return DATA_E_FORMATETC;
}

static HRESULT WINAPI data_object_GetDataHere( IDataObject *iface, FORMATETC *format, STGMEDIUM *medium )
{
    struct data_object *object = data_object_from_IDataObject( iface );
    FIXME( "object %p, format %p, medium %p stub!\n", object, format, medium );
    return DATA_E_FORMATETC;
}

static HRESULT WINAPI data_object_QueryGetData( IDataObject *iface, FORMATETC *format )
{
    struct data_object *object = data_object_from_IDataObject( iface );
    struct format_entry *iter;

    TRACE( "object %p, format %p (%s)\n", object, format, debugstr_format(format->cfFormat) );

    if (format->tymed && !(format->tymed & TYMED_HGLOBAL))
    {
        FIXME("only HGLOBAL medium types supported right now\n");
        return DV_E_TYMED;
    }
    /* Windows Explorer ignores .dwAspect and .lindex for CF_HDROP,
     * and we have no way to implement them on XDnD anyway, so ignore them too.
     */

    for (iter = object->entries; iter < object->entries_end; iter = next_format( iter ))
    {
        if (iter->format == format->cfFormat)
        {
            TRACE("application found %s\n", debugstr_format(format->cfFormat));
            return S_OK;
        }
    }
    TRACE("application didn't find %s\n", debugstr_format(format->cfFormat));
    return DV_E_FORMATETC;
}

static HRESULT WINAPI data_object_GetCanonicalFormatEtc( IDataObject *iface, FORMATETC *format, FORMATETC *out )
{
    struct data_object *object = data_object_from_IDataObject( iface );
    FIXME( "object %p, format %p, out %p stub!\n", object, format, out );
    out->ptd = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI data_object_SetData( IDataObject *iface, FORMATETC *format, STGMEDIUM *medium, BOOL release )
{
    struct data_object *object = data_object_from_IDataObject( iface );
    FIXME( "object %p, format %p, medium %p, release %u stub!\n", object, format, medium, release );
    return E_NOTIMPL;
}

static HRESULT WINAPI data_object_EnumFormatEtc( IDataObject *iface, DWORD direction, IEnumFORMATETC **out )
{
    struct data_object *object = data_object_from_IDataObject( iface );
    struct format_entry *iter;
    DWORD i = 0, count = 0;
    FORMATETC *formats;
    HRESULT hr;

    TRACE( "object %p, direction %lu, out %p\n", object, direction, out );

    if (direction != DATADIR_GET)
    {
        FIXME("only the get direction is implemented\n");
        return E_NOTIMPL;
    }

    for (iter = object->entries; iter < object->entries_end; iter = next_format( iter )) count++;

    if (!(formats = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*formats) ))) return E_OUTOFMEMORY;

    for (iter = object->entries; iter < object->entries_end; iter = next_format( iter ), i++)
    {
        formats[i].cfFormat = iter->format;
        formats[i].ptd = NULL;
        formats[i].dwAspect = DVASPECT_CONTENT;
        formats[i].lindex = -1;
        formats[i].tymed = TYMED_HGLOBAL;
    }

    hr = SHCreateStdEnumFmtEtc( count, formats, out );
    HeapFree( GetProcessHeap(), 0, formats );
    return hr;
}

static HRESULT WINAPI data_object_DAdvise( IDataObject *iface, FORMATETC *format, DWORD flags,
                                           IAdviseSink *sink, DWORD *connection )
{
    struct data_object *object = data_object_from_IDataObject( iface );
    FIXME( "object %p, format %p, flags %#lx, sink %p, connection %p stub!\n",
           object, format, flags, sink, connection );
    return OLE_E_ADVISENOTSUPPORTED;
}

static HRESULT WINAPI data_object_DUnadvise( IDataObject *iface, DWORD connection )
{
    struct data_object *object = data_object_from_IDataObject( iface );
    FIXME( "object %p, connection %lu stub!\n", object, connection );
    return OLE_E_ADVISENOTSUPPORTED;
}

static HRESULT WINAPI data_object_EnumDAdvise( IDataObject *iface, IEnumSTATDATA **advise )
{
    struct data_object *object = data_object_from_IDataObject( iface );
    FIXME( "object %p, advise %p stub!\n", object, advise );
    return OLE_E_ADVISENOTSUPPORTED;
}

static IDataObjectVtbl data_object_vtbl =
{
    data_object_QueryInterface,
    data_object_AddRef,
    data_object_Release,
    data_object_GetData,
    data_object_GetDataHere,
    data_object_QueryGetData,
    data_object_GetCanonicalFormatEtc,
    data_object_SetData,
    data_object_EnumFormatEtc,
    data_object_DAdvise,
    data_object_DUnadvise,
    data_object_EnumDAdvise,
};

static HRESULT data_object_create( UINT entries_size, const struct format_entry *entries, IDataObject **out )
{
    struct data_object *object;

    if (!(object = calloc( 1, sizeof(*object) + entries_size ))) return E_OUTOFMEMORY;
    object->IDataObject_iface.lpVtbl = &data_object_vtbl;
    object->refcount = 1;

    object->entries_end = (struct format_entry *)((char *)object->entries + entries_size);
    memcpy( object->entries, entries, entries_size );
    *out = &object->IDataObject_iface;

    return S_OK;
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
            hr = IDropTarget_DragEnter(dropTarget, xdnd_data_object,
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
    struct dnd_drop_event_params *params = args;
    HWND hwnd = UlongToHandle( params->hwnd );
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
            hr = IDropTarget_Drop(dropTarget, xdnd_data_object, MK_LBUTTON,
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
NTSTATUS WINAPI x11drv_dnd_enter_event( void *args, ULONG size )
{
    UINT formats_size = size - offsetof(struct dnd_enter_event_params, entries);
    struct dnd_enter_event_params *params = args;
    IDataObject *object;

    XDNDAccepted = FALSE;
    X11DRV_XDND_FreeDragDropOp(); /* Clear previously cached data */

    if (FAILED(data_object_create( formats_size, params->entries, &object ))) return STATUS_NO_MEMORY;

    EnterCriticalSection( &xdnd_cs );
    xdnd_data_object = object;
    LeaveCriticalSection( &xdnd_cs );

    return STATUS_SUCCESS;
}


/**************************************************************************
 * X11DRV_XDND_HasHDROP
 */
static BOOL X11DRV_XDND_HasHDROP(void)
{
    FORMATETC format = {.cfFormat = CF_HDROP};
    BOOL found = FALSE;

    EnterCriticalSection(&xdnd_cs);
    found = xdnd_data_object && SUCCEEDED(IDataObject_QueryGetData( xdnd_data_object, &format ));
    LeaveCriticalSection(&xdnd_cs);

    return found;
}

/**************************************************************************
 * X11DRV_XDND_SendDropFiles
 */
static HRESULT X11DRV_XDND_SendDropFiles(HWND hwnd)
{
    FORMATETC format = {.cfFormat = CF_HDROP};
    STGMEDIUM medium;
    HRESULT hr;

    EnterCriticalSection(&xdnd_cs);

    if (!xdnd_data_object) hr = E_FAIL;
    else if (SUCCEEDED(hr = IDataObject_GetData( xdnd_data_object, &format, &medium )))
    {
        DROPFILES *drop = GlobalLock( medium.hGlobal );
        void *files = (char *)drop + drop->pFiles;
        RECT rect;

        drop->pt.x = XDNDxy.x;
        drop->pt.y = XDNDxy.y;
        drop->fNC  = !ScreenToClient( hwnd, &drop->pt ) || !GetClientRect( hwnd, &rect ) || !PtInRect( &rect, drop->pt );

        TRACE( "Sending WM_DROPFILES: hwnd %p, pt %s, fNC %u, files %p (%s)\n", hwnd,
               wine_dbgstr_point( &drop->pt), drop->fNC, files, debugstr_w(files) );
        GlobalUnlock( medium.hGlobal );

        if (PostMessageW( hwnd, WM_DROPFILES, (WPARAM)medium.hGlobal, 0 ))
            hr = S_OK;
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            GlobalFree( medium.hGlobal );
        }
    }

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

    if (xdnd_data_object)
    {
        IDataObject_Release( xdnd_data_object );
        xdnd_data_object = NULL;
    }

    XDNDxy.x = XDNDxy.y = 0;
    XDNDLastTargetWnd = NULL;
    XDNDLastDropTargetWnd = NULL;
    XDNDAccepted = FALSE;

    LeaveCriticalSection(&xdnd_cs);
}

NTSTATUS WINAPI x11drv_dnd_post_drop( void *args, ULONG size )
{
    UINT drop_size = size - offsetof(struct dnd_post_drop_params, drop);
    struct dnd_post_drop_params *params = args;
    HDROP handle;

    if ((handle = GlobalAlloc( GMEM_SHARE, drop_size )))
    {
        DROPFILES *ptr = GlobalLock( handle );
        HWND hwnd;
        memcpy( ptr, &params->drop, drop_size );
        hwnd = UlongToHandle( ptr->fWide );
        ptr->fWide = TRUE;
        GlobalUnlock( handle );
        PostMessageW( hwnd, WM_DROPFILES, (WPARAM)handle, 0 );
    }

    return STATUS_SUCCESS;
}
