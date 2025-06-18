/*
 * SHAppBarMessage implementation
 *
 * Copyright 2008 Vincent Povirk for CodeWeavers
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
 * TODO: freedesktop _NET_WM_STRUT integration
 *
 * TODO: find when a fullscreen app is in the foreground and send FULLSCREENAPP
 *  notifications
 *
 * TODO: detect changes in the screen size and send ABN_POSCHANGED ?
 *
 * TODO: multiple monitor support
 */

#include "windows.h"
#include "shellapi.h"
#include "wine/debug.h"
#include "explorer_private.h"

#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(appbar);

struct appbar_data_msg  /* platform-independent data */
{
    LONG      hWnd;
    UINT      uCallbackMessage;
    UINT      uEdge;
    RECT      rc;
    ULONGLONG lParam;
};

struct appbar_cmd
{
    ULONG  return_map;
    DWORD  return_process;
    struct appbar_data_msg abd;
};

struct appbar_response
{
    ULONGLONG result;
    struct appbar_data_msg abd;
};

static HWND appbarmsg_window = NULL;

struct appbar_data
{
    struct list entry;
    HWND hwnd;
    UINT callback_msg;
    UINT edge;
    RECT rc;
    BOOL space_reserved;
    /* BOOL autohide; */
};

static struct list appbars = LIST_INIT(appbars);

static struct appbar_data* get_appbar(HWND hwnd)
{
    struct appbar_data* data;

    LIST_FOR_EACH_ENTRY(data, &appbars, struct appbar_data, entry)
    {
        if (data->hwnd == hwnd)
            return data;
    }

    return NULL;
}

/* send_poschanged: send ABN_POSCHANGED to every appbar except one */
static void send_poschanged(HWND hwnd)
{
    struct appbar_data* data;
    LIST_FOR_EACH_ENTRY(data, &appbars, struct appbar_data, entry)
    {
        if (data->hwnd != hwnd)
        {
            PostMessageW(data->hwnd, data->callback_msg, ABN_POSCHANGED, 0);
        }
    }
}

/* appbar_cliprect: cut out parts of the rectangle that interfere with existing appbars */
static void appbar_cliprect( HWND hwnd, RECT *rect )
{
    struct appbar_data* data;
    LIST_FOR_EACH_ENTRY(data, &appbars, struct appbar_data, entry)
    {
        if (data->hwnd == hwnd)
        {
            /* we only care about appbars that were added before this one */
            return;
        }
        if (data->space_reserved)
        {
            /* move in the side that corresponds to the other appbar's edge */
            switch (data->edge)
            {
            case ABE_BOTTOM:
                rect->bottom = min(rect->bottom, data->rc.top);
                break;
            case ABE_LEFT:
                rect->left = max(rect->left, data->rc.right);
                break;
            case ABE_RIGHT:
                rect->right = min(rect->right, data->rc.left);
                break;
            case ABE_TOP:
                rect->top = max(rect->top, data->rc.bottom);
                break;
            }
        }
    }
}

static UINT_PTR handle_appbarmessage(DWORD msg, struct appbar_data_msg *abd)
{
    struct appbar_data* data;
    HWND hwnd = LongToHandle( abd->hWnd );

    switch (msg)
    {
    case ABM_NEW:
        if (get_appbar(hwnd))
        {
            /* fail when adding an hwnd the second time */
            return FALSE;
        }

        data = calloc( 1, sizeof(struct appbar_data) );
        if (!data)
        {
            ERR( "out of memory\n" );
            return FALSE;
        }
        data->hwnd = hwnd;
        data->callback_msg = abd->uCallbackMessage;

        list_add_tail(&appbars, &data->entry);

        return TRUE;
    case ABM_REMOVE:
        if ((data = get_appbar(hwnd)))
        {
            list_remove(&data->entry);

            send_poschanged(hwnd);

            free( data );
        }
        else WARN( "removing hwnd %p not on the list\n", hwnd );
        return TRUE;
    case ABM_QUERYPOS:
        if (abd->uEdge > ABE_BOTTOM) WARN( "invalid edge %i for %p\n", abd->uEdge, hwnd );
        appbar_cliprect( hwnd, &abd->rc );
        return TRUE;
    case ABM_SETPOS:
        if (abd->uEdge > ABE_BOTTOM)
        {
            WARN( "invalid edge %i for %p\n", abd->uEdge, hwnd );
            return TRUE;
        }
        if ((data = get_appbar(hwnd)))
        {
            /* calculate acceptable space */
            appbar_cliprect( hwnd, &abd->rc );

            if (!EqualRect(&abd->rc, &data->rc))
                send_poschanged(hwnd);

            /* reserve that space for this appbar */
            data->edge = abd->uEdge;
            data->rc = abd->rc;
            data->space_reserved = TRUE;
        }
        else
        {
            WARN( "app sent ABM_SETPOS message for %p without ABM_ADD\n", hwnd );
        }
        return TRUE;
    case ABM_GETSTATE:
        FIXME( "SHAppBarMessage(ABM_GETSTATE): stub\n" );
        return ABS_ALWAYSONTOP | ABS_AUTOHIDE;
    case ABM_GETTASKBARPOS:
        FIXME( "SHAppBarMessage(ABM_GETTASKBARPOS, hwnd=%p): stub\n", hwnd );
        /* Report the taskbar is at the bottom of the screen. */
        abd->rc.left = 0;
        abd->rc.right = GetSystemMetrics(SM_CXSCREEN);
        abd->rc.bottom = GetSystemMetrics(SM_CYSCREEN);
        abd->rc.top = abd->rc.bottom-1;
        abd->uEdge = ABE_BOTTOM;
        return TRUE;
    case ABM_ACTIVATE:
        return TRUE;
    case ABM_GETAUTOHIDEBAR:
        FIXME( "SHAppBarMessage(ABM_GETAUTOHIDEBAR, hwnd=%p, edge=%x): stub\n", hwnd, abd->uEdge );
        return 0;
    case ABM_SETAUTOHIDEBAR:
        FIXME( "SHAppBarMessage(ABM_SETAUTOHIDEBAR, hwnd=%p, edge=%x, lparam=%s): stub\n", hwnd,
               abd->uEdge, wine_dbgstr_longlong( abd->lParam ) );
        return TRUE;
    case ABM_WINDOWPOSCHANGED:
        return TRUE;
    default:
        FIXME( "SHAppBarMessage(%lx) unimplemented\n", msg );
        return FALSE;
    }
}

static LRESULT CALLBACK appbar_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_COPYDATA:
        {
            COPYDATASTRUCT* cds;
            struct appbar_cmd cmd;
            UINT_PTR result;
            HANDLE return_hproc;
            HANDLE return_map;
            LPVOID return_view;
            struct appbar_response* response;

            cds = (COPYDATASTRUCT*)lparam;
            if (cds->cbData != sizeof(struct appbar_cmd))
                return TRUE;
            CopyMemory(&cmd, cds->lpData, cds->cbData);

            result = handle_appbarmessage(cds->dwData, &cmd.abd);

            return_hproc = OpenProcess(PROCESS_DUP_HANDLE, FALSE, cmd.return_process);
            if (return_hproc == NULL)
            {
                ERR( "couldn't open calling process\n" );
                return TRUE;
            }

            if (!DuplicateHandle(return_hproc, UlongToHandle(cmd.return_map),
                                 GetCurrentProcess(), &return_map, 0, FALSE, DUPLICATE_SAME_ACCESS))
            {
                ERR( "couldn't duplicate handle\n" );
                CloseHandle(return_hproc);
                return TRUE;
            }
            CloseHandle(return_hproc);

            return_view = MapViewOfFile(return_map, FILE_MAP_WRITE, 0, 0, sizeof(struct appbar_response));

            if (return_view)
            {
                response = (struct appbar_response*)return_view;
                response->result = result;
                response->abd = cmd.abd;

                UnmapViewOfFile(return_view);
            }
            else ERR( "couldn't map view of file\n" );

            CloseHandle(return_map);
            return TRUE;
        }
    default:
        break;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

void initialize_appbar(void)
{
    WNDCLASSEXW class;

    /* register the appbar window class */
    ZeroMemory(&class, sizeof(class));
    class.cbSize = sizeof(class);
    class.lpfnWndProc = appbar_wndproc;
    class.hInstance = NULL;
    class.lpszClassName = L"WineAppBar";

    if (!RegisterClassExW(&class))
    {
        ERR( "Could not register appbar message window class\n" );
        return;
    }

    appbarmsg_window = CreateWindowW(class.lpszClassName, class.lpszClassName, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
    if (!appbarmsg_window)
    {
        ERR( "Could not create appbar message window\n" );
        return;
    }
}
