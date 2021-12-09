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

#include "config.h"

#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>

#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#define COBJMACROS
#include "x11drv.h"
#include "shellapi.h"
#include "shlobj.h"  /* DROPFILES */
#include "oleidl.h"
#include "objidl.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(xdnd);

typedef struct tagXDNDDATA
{
    int cf_win;
    Atom cf_xdnd;
    HANDLE contents;
    struct list entry;
} XDNDDATA, *LPXDNDDATA;

static struct list xdndData = LIST_INIT(xdndData);
static POINT XDNDxy = { 0, 0 };
static IDataObject XDNDDataObject;
static BOOL XDNDAccepted = FALSE;
static DWORD XDNDDropEffect = DROPEFFECT_NONE;
/* the last window the mouse was over */
static HWND XDNDLastTargetWnd;
/* might be an ancestor of XDNDLastTargetWnd */
static HWND XDNDLastDropTargetWnd;

static void X11DRV_XDND_InsertXDNDData( Atom property, UINT format, HANDLE contents );
static void X11DRV_XDND_ResolveProperty(Display *display, Window xwin, Time tm,
    Atom *types, unsigned long count);
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
 * X11DRV_XDND_XdndActionToDROPEFFECT
 */
static DWORD X11DRV_XDND_XdndActionToDROPEFFECT(long action)
{
    /* In Windows, nothing but the given effects is allowed.
     * In X the given action is just a hint, and you can always
     * XdndActionCopy and XdndActionPrivate, so be more permissive. */
    if (action == x11drv_atom(XdndActionCopy))
        return DROPEFFECT_COPY;
    else if (action == x11drv_atom(XdndActionMove))
        return DROPEFFECT_MOVE | DROPEFFECT_COPY;
    else if (action == x11drv_atom(XdndActionLink))
        return DROPEFFECT_LINK | DROPEFFECT_COPY;
    else if (action == x11drv_atom(XdndActionAsk))
        /* FIXME: should we somehow ask the user what to do here? */
        return DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK;
    FIXME("unknown action %ld, assuming DROPEFFECT_COPY\n", action);
    return DROPEFFECT_COPY;
}

/**************************************************************************
 * X11DRV_XDND_DROPEFFECTToXdndAction
 */
static long X11DRV_XDND_DROPEFFECTToXdndAction(DWORD effect)
{
    if (effect == DROPEFFECT_COPY)
        return x11drv_atom(XdndActionCopy);
    else if (effect == DROPEFFECT_MOVE)
        return x11drv_atom(XdndActionMove);
    else if (effect == DROPEFFECT_LINK)
        return x11drv_atom(XdndActionLink);
    FIXME("unknown drop effect %u, assuming XdndActionCopy\n", effect);
    return x11drv_atom(XdndActionCopy);
}

/**************************************************************************
 * X11DRV_XDND_EnterEvent
 *
 * Handle an XdndEnter event.
 */
void X11DRV_XDND_EnterEvent( HWND hWnd, XClientMessageEvent *event )
{
    int version;
    Atom *xdndtypes;
    unsigned long count = 0;

    version = (event->data.l[1] & 0xFF000000) >> 24;
    TRACE("ver(%d) check-XdndTypeList(%ld) data=%ld,%ld,%ld,%ld,%ld\n",
          version, (event->data.l[1] & 1),
          event->data.l[0], event->data.l[1], event->data.l[2],
          event->data.l[3], event->data.l[4]);

    if (version > WINE_XDND_VERSION)
    {
        ERR("ignoring unsupported XDND version %d\n", version);
        return;
    }

    XDNDAccepted = FALSE;

    /* If the source supports more than 3 data types we retrieve
     * the entire list. */
    if (event->data.l[1] & 1)
    {
        Atom acttype;
        int actfmt;
        unsigned long bytesret;

        /* Request supported formats from source window */
        XGetWindowProperty(event->display, event->data.l[0], x11drv_atom(XdndTypeList),
                           0, 65535, FALSE, AnyPropertyType, &acttype, &actfmt, &count,
                           &bytesret, (unsigned char**)&xdndtypes);
    }
    else
    {
        count = 3;
        xdndtypes = (Atom*) &event->data.l[2];
    }

    if (TRACE_ON(xdnd))
    {
        unsigned int i;

        for (i = 0; i < count; i++)
        {
            if (xdndtypes[i] != 0)
            {
                char * pn = XGetAtomName(event->display, xdndtypes[i]);
                TRACE("XDNDEnterAtom %ld: %s\n", xdndtypes[i], pn);
                XFree(pn);
            }
        }
    }

    /* Do a one-time data read and cache results */
    X11DRV_XDND_ResolveProperty(event->display, event->window,
                                event->data.l[1], xdndtypes, count);

    if (event->data.l[1] & 1)
        XFree(xdndtypes);
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
 * X11DRV_XDND_PositionEvent
 *
 * Handle an XdndPosition event.
 */
void X11DRV_XDND_PositionEvent( HWND hWnd, XClientMessageEvent *event )
{
    XClientMessageEvent e;
    int accept = 0; /* Assume we're not accepting */
    IDropTarget *dropTarget = NULL;
    DWORD effect;
    POINTL pointl;
    HWND targetWindow;
    HRESULT hr;

    XDNDxy = root_to_virtual_screen( event->data.l[2] >> 16, event->data.l[2] & 0xFFFF );
    targetWindow = window_from_point_dnd(hWnd, XDNDxy);

    pointl.x = XDNDxy.x;
    pointl.y = XDNDxy.y;
    effect = X11DRV_XDND_XdndActionToDROPEFFECT(event->data.l[4]);

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
                    WARN("IDropTarget_DragLeave failed, error 0x%08X\n", hr);
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
                TRACE("the application accepted the drop (effect = %d)\n", effect_ignore);
            }
            else
            {
                XDNDAccepted = FALSE;
                WARN("IDropTarget_DragEnter failed, error 0x%08X\n", hr);
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
                WARN("IDropTarget_DragOver failed, error 0x%08X\n", hr);
            IDropTarget_Release(dropTarget);
        }
    }

    if (XDNDAccepted)
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

    TRACE("actionRequested(%ld) accept(%d) chosen(0x%x) at x(%d),y(%d)\n",
          event->data.l[4], accept, effect, XDNDxy.x, XDNDxy.y);

    /*
     * Let source know if we're accepting the drop by
     * sending a status message.
     */
    e.type = ClientMessage;
    e.display = event->display;
    e.window = event->data.l[0];
    e.message_type = x11drv_atom(XdndStatus);
    e.format = 32;
    e.data.l[0] = event->window;
    e.data.l[1] = accept;
    e.data.l[2] = 0; /* Empty Rect */
    e.data.l[3] = 0; /* Empty Rect */
    if (accept)
        e.data.l[4] = X11DRV_XDND_DROPEFFECTToXdndAction(effect);
    else
        e.data.l[4] = None;
    XSendEvent(event->display, event->data.l[0], False, NoEventMask, (XEvent*)&e);
}

/**************************************************************************
 * X11DRV_XDND_DropEvent
 *
 * Handle an XdndDrop event.
 */
void X11DRV_XDND_DropEvent( HWND hWnd, XClientMessageEvent *event )
{
    XClientMessageEvent e;
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
                WARN("drop failed, error 0x%08X\n", hr);
            else
            {
                WARN("drop returned 0x%08X\n", hr);
                drop_file = FALSE;
            }
            IDropTarget_Release(dropTarget);
        }
        else if (dropTarget)
        {
            HRESULT hr = IDropTarget_DragLeave(dropTarget);
            if (FAILED(hr))
                WARN("IDropTarget_DragLeave failed, error 0x%08X\n", hr);
            IDropTarget_Release(dropTarget);
        }
    }

    if (drop_file)
    {
        /* Only send WM_DROPFILES if Drop didn't succeed or DROPEFFECT_NONE was set.
         * Doing both causes winamp to duplicate the dropped files (#29081) */

        HWND hwnd_drop = window_accepting_files(window_from_point_dnd(hWnd, XDNDxy));

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

    TRACE("effectRequested(0x%x) accept(%d) performed(0x%x) at x(%d),y(%d)\n",
          XDNDDropEffect, accept, effect, XDNDxy.x, XDNDxy.y);

    /* Tell the target we are finished. */
    memset(&e, 0, sizeof(e));
    e.type = ClientMessage;
    e.display = event->display;
    e.window = event->data.l[0];
    e.message_type = x11drv_atom(XdndFinished);
    e.format = 32;
    e.data.l[0] = event->window;
    e.data.l[1] = accept;
    if (accept)
        e.data.l[2] = X11DRV_XDND_DROPEFFECTToXdndAction(effect);
    else
        e.data.l[2] = None;
    XSendEvent(event->display, event->data.l[0], False, NoEventMask, (XEvent*)&e);
}

/**************************************************************************
 * X11DRV_XDND_LeaveEvent
 *
 * Handle an XdndLeave event.
 */
void X11DRV_XDND_LeaveEvent( HWND hWnd, XClientMessageEvent *event )
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
                WARN("IDropTarget_DragLeave failed, error 0x%08X\n", hr);
            IDropTarget_Release(dropTarget);
        }
    }

    X11DRV_XDND_FreeDragDropOp();
}


/**************************************************************************
 * X11DRV_XDND_ResolveProperty
 *
 * Resolve all MIME types to windows clipboard formats. All data is cached.
 */
static void X11DRV_XDND_ResolveProperty(Display *display, Window xwin, Time tm,
                                        Atom *types, unsigned long count)
{
    XDNDDATA *current, *next;
    BOOL haveHDROP = FALSE;

    TRACE("count(%ld)\n", count);

    X11DRV_XDND_FreeDragDropOp(); /* Clear previously cached data */

    X11DRV_CLIPBOARD_ImportSelection( display, xwin, x11drv_atom(XdndSelection),
                                      types, count, X11DRV_XDND_InsertXDNDData );

    /* On Windows when there is a CF_HDROP, there are no other CF_ formats.
     * foobar2000 relies on this (spaces -> %20's without it).
     */
    LIST_FOR_EACH_ENTRY(current, &xdndData, XDNDDATA, entry)
    {
        if (current->cf_win == CF_HDROP)
        {
            haveHDROP = TRUE;
            break;
        }
    }
    if (haveHDROP)
    {
        LIST_FOR_EACH_ENTRY_SAFE(current, next, &xdndData, XDNDDATA, entry)
        {
            if (current->cf_win != CF_HDROP && current->cf_win < CF_MAX)
            {
                list_remove(&current->entry);
                GlobalFree(current->contents);
                HeapFree(GetProcessHeap(), 0, current);
            }
        }
    }
}


/**************************************************************************
 * X11DRV_XDND_InsertXDNDData
 *
 * Cache available XDND property
 */
static void X11DRV_XDND_InsertXDNDData( Atom property, UINT format, HANDLE contents )
{
    LPXDNDDATA current = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(XDNDDATA));

    if (current)
    {
        EnterCriticalSection(&xdnd_cs);
        current->cf_xdnd = property;
        current->cf_win = format;
        current->contents = contents;
        list_add_tail(&xdndData, &current->entry);
        LeaveCriticalSection(&xdnd_cs);
    }
}


/**************************************************************************
 * X11DRV_XDND_HasHDROP
 */
static BOOL X11DRV_XDND_HasHDROP(void)
{
    LPXDNDDATA current = NULL;
    BOOL found = FALSE;

    EnterCriticalSection(&xdnd_cs);

    /* Find CF_HDROP type if any */
    LIST_FOR_EACH_ENTRY(current, &xdndData, XDNDDATA, entry)
    {
        if (current->cf_win == CF_HDROP)
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
    HRESULT hr;
    LPXDNDDATA current = NULL;
    BOOL found = FALSE;

    EnterCriticalSection(&xdnd_cs);

    LIST_FOR_EACH_ENTRY(current, &xdndData, XDNDDATA, entry)
    {
         if (current->cf_win == CF_HDROP)
         {
             found = TRUE;
             break;
         }
    }
    if (found)
    {
        HGLOBAL dropHandle = GlobalAlloc(GMEM_FIXED, GlobalSize(current->contents));
        if (dropHandle)
        {
            RECT rect;
            DROPFILES *lpDrop = GlobalLock(dropHandle);
            memcpy(lpDrop, GlobalLock(current->contents), GlobalSize(current->contents));
            GlobalUnlock(current->contents);
            lpDrop->pt.x = XDNDxy.x;
            lpDrop->pt.y = XDNDxy.y;
            lpDrop->fNC  = !(ScreenToClient(hwnd, &lpDrop->pt) &&
                             GetClientRect(hwnd, &rect) &&
                             PtInRect(&rect, lpDrop->pt));
            TRACE("Sending WM_DROPFILES: hWnd=0x%p, fNC=%d, x=%d, y=%d, files=%p(%s)\n", hwnd,
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
    LPXDNDDATA next;
    LPXDNDDATA current;

    TRACE("\n");

    EnterCriticalSection(&xdnd_cs);

    /** Free data cache */
    LIST_FOR_EACH_ENTRY_SAFE(current, next, &xdndData, XDNDDATA, entry)
    {
        list_remove(&current->entry);
        GlobalFree(current->contents);
        HeapFree(GetProcessHeap(), 0, current);
    }

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
        XDNDDATA *current;
        LIST_FOR_EACH_ENTRY(current, &xdndData, XDNDDATA, entry)
        {
            if (current->cf_win == formatEtc->cfFormat)
            {
                pMedium->tymed = TYMED_HGLOBAL;
                pMedium->u.hGlobal = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, GlobalSize(current->contents));
                if (pMedium->u.hGlobal == NULL)
                    return E_OUTOFMEMORY;
                memcpy(GlobalLock(pMedium->u.hGlobal), GlobalLock(current->contents), GlobalSize(current->contents));
                GlobalUnlock(pMedium->u.hGlobal);
                GlobalUnlock(current->contents);
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
    char formatDesc[1024];
    XDNDDATA *current;

    TRACE("(%p, %p={.tymed=0x%x, .dwAspect=%d, .cfFormat=%d}\n",
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

    LIST_FOR_EACH_ENTRY(current, &xdndData, XDNDDATA, entry)
    {
        if (current->cf_win == formatEtc->cfFormat)
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
    DWORD count;
    FORMATETC *formats;

    TRACE("(%p, %u, %p)\n", dataObject, dwDirection, ppEnumFormatEtc);

    if (dwDirection != DATADIR_GET)
    {
        FIXME("only the get direction is implemented\n");
        return E_NOTIMPL;
    }

    count = list_count(&xdndData);
    formats = HeapAlloc(GetProcessHeap(), 0, count * sizeof(FORMATETC));
    if (formats)
    {
        XDNDDATA *current;
        DWORD i = 0;
        HRESULT hr;
        LIST_FOR_EACH_ENTRY(current, &xdndData, XDNDDATA, entry)
        {
            formats[i].cfFormat = current->cf_win;
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
    FIXME("(%p, %p, %u, %p, %p): stub\n", dataObject, formatEtc, advf,
        adviseSink, pdwConnection);
    return OLE_E_ADVISENOTSUPPORTED;
}

static HRESULT WINAPI XDNDDATAOBJECT_DUnadvise(IDataObject *dataObject,
                                               DWORD dwConnection)
{
    FIXME("(%p, %u): stub\n", dataObject, dwConnection);
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
