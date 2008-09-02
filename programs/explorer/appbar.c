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

#include "wine/unicode.h"

#include <windows.h>
#include <wine/debug.h>
#include "explorer_private.h"

#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(appbar);

struct appbar_cmd
{
    HANDLE return_map;
    DWORD return_process;
    APPBARDATA abd;
};

struct appbar_response
{
    UINT_PTR result;
    RECT rc;
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
static void appbar_cliprect(PAPPBARDATA abd)
{
    struct appbar_data* data;
    LIST_FOR_EACH_ENTRY(data, &appbars, struct appbar_data, entry)
    {
        if (data->hwnd == abd->hWnd)
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
                abd->rc.bottom = min(abd->rc.bottom, data->rc.top);
                break;
            case ABE_LEFT:
                abd->rc.left = max(abd->rc.left, data->rc.right);
                break;
            case ABE_RIGHT:
                abd->rc.right = min(abd->rc.right, data->rc.left);
                break;
            case ABE_TOP:
                abd->rc.top = max(abd->rc.top, data->rc.bottom);
                break;
            }
        }
    }
}

static UINT_PTR handle_appbarmessage(DWORD msg, PAPPBARDATA abd)
{
    struct appbar_data* data;

    switch (msg)
    {
    case ABM_NEW:
        if (get_appbar(abd->hWnd))
        {
            /* fail when adding an hwnd the second time */
            return FALSE;
        }

        data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct appbar_data));
        if (!data)
        {
            WINE_ERR("out of memory\n");
            return FALSE;
        }
        data->hwnd = abd->hWnd;
        data->callback_msg = abd->uCallbackMessage;

        list_add_tail(&appbars, &data->entry);

        return TRUE;
    case ABM_REMOVE:
        if ((data = get_appbar(abd->hWnd)))
        {
            list_remove(&data->entry);

            send_poschanged(abd->hWnd);

            HeapFree(GetProcessHeap(), 0, data);
        }
        else
            WINE_WARN("removing hwnd %p not on the list\n", abd->hWnd);
        return TRUE;
    case ABM_QUERYPOS:
        if (abd->uEdge > ABE_BOTTOM)
            WINE_WARN("invalid edge %i for %p\n", abd->uEdge, abd->hWnd);
        appbar_cliprect(abd);
        return TRUE;
    case ABM_SETPOS:
        if (abd->uEdge > ABE_BOTTOM)
        {
            WINE_WARN("invalid edge %i for %p\n", abd->uEdge, abd->hWnd);
            return TRUE;
        }
        if ((data = get_appbar(abd->hWnd)))
        {
            /* calculate acceptable space */
            appbar_cliprect(abd);

            if (!EqualRect(&abd->rc, &data->rc))
                send_poschanged(abd->hWnd);

            /* reserve that space for this appbar */
            data->edge = abd->uEdge;
            data->rc = abd->rc;
            data->space_reserved = TRUE;
        }
        else
        {
            WINE_WARN("app sent ABM_SETPOS message for %p without ABM_ADD\n", abd->hWnd);
        }
        return TRUE;
    case ABM_GETSTATE:
        WINE_FIXME("SHAppBarMessage(ABM_GETSTATE): stub\n");
        return ABS_ALWAYSONTOP | ABS_AUTOHIDE;
    case ABM_GETTASKBARPOS:
        WINE_FIXME("SHAppBarMessage(ABM_GETTASKBARPOS, hwnd=%p): stub\n", abd->hWnd);
        return FALSE;
    case ABM_ACTIVATE:
        WINE_FIXME("SHAppBarMessage(ABM_ACTIVATE, hwnd=%p, lparam=%lx): stub\n", abd->hWnd, abd->lParam);
        return TRUE;
    case ABM_GETAUTOHIDEBAR:
        WINE_FIXME("SHAppBarMessage(ABM_GETAUTOHIDEBAR, hwnd=%p, edge=%x): stub\n", abd->hWnd, abd->uEdge);
        return 0;
    case ABM_SETAUTOHIDEBAR:
        WINE_FIXME("SHAppBarMessage(ABM_SETAUTOHIDEBAR, hwnd=%p, edge=%x, lparam=%lx): stub\n", abd->hWnd, abd->uEdge, abd->lParam);
        return TRUE;
    case ABM_WINDOWPOSCHANGED:
        WINE_FIXME("SHAppBarMessage(ABM_WINDOWPOSCHANGED, hwnd=%p): stub\n", abd->hWnd);
        return TRUE;
    default:
        WINE_FIXME("SHAppBarMessage(%x) unimplemented\n", msg);
        return FALSE;
    }
}

LRESULT CALLBACK appbar_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
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
                WINE_ERR("couldn't open calling process\n");
                return TRUE;
            }

            if (!DuplicateHandle(return_hproc, cmd.return_map, GetCurrentProcess(), &return_map, 0, FALSE, DUPLICATE_SAME_ACCESS))
            {
                WINE_ERR("couldn't duplicate handle\n");
                CloseHandle(return_hproc);
                return TRUE;
            }
            CloseHandle(return_hproc);

            return_view = MapViewOfFile(return_map, FILE_MAP_WRITE, 0, 0, sizeof(struct appbar_response));

            if (return_view)
            {
                response = (struct appbar_response*)return_view;
                response->result = result;
                response->rc = cmd.abd.rc;

                UnmapViewOfFile(return_view);
            }
            else
                WINE_ERR("couldn't map view of file\n");

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
    static const WCHAR classname[] = {'W','i','n','e','A','p','p','B','a','r',0};

    /* register the appbar window class */
    ZeroMemory(&class, sizeof(class));
    class.cbSize = sizeof(class);
    class.lpfnWndProc = appbar_wndproc;
    class.hInstance = NULL;
    class.lpszClassName = classname;

    if (!RegisterClassExW(&class))
    {
        WINE_ERR("Could not register appbar message window class\n");
        return;
    }

    appbarmsg_window = CreateWindowW(classname, classname, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
    if (!appbarmsg_window)
    {
        WINE_ERR("Could not create appbar message window\n");
        return;
    }
}
