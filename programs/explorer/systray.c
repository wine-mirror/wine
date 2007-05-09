/*
 * Copyright (C) 2004 Mike Hearn, for CodeWeavers
 * Copyright (C) 2005 Robert Shearman
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

/* There are two types of window involved here. The first is the
 * listener window. This is like the taskbar in Windows. It doesn't
 * ever appear on-screen in our implementation, instead we create
 * individual mini "adaptor" windows which are docked by the native
 * systray host.
 *
 * In future for those who don't have a systray we could make the
 * listener window more clever so it can draw itself like the Windows
 * tray area does (with a clock and stuff).
 */

#include <assert.h>

#define UNICODE
#define _WIN32_IE 0x500
#include <windows.h>
#include <commctrl.h>

#include <wine/debug.h>
#include <wine/list.h>

#include "explorer_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(systray);

#define IS_OPTION_FALSE(ch) \
    ((ch) == 'n' || (ch) == 'N' || (ch) == 'f' || (ch) == 'F' || (ch) == '0')

static const WCHAR adaptor_classname[] = /* Adaptor */ {'A','d','a','p','t','o','r',0};

/* tray state */
struct tray
{
    HWND           window;
    struct list    icons;
};

/* an individual systray icon, unpacked from the NOTIFYICONDATA and always in unicode */
struct icon
{
    struct list    entry;
    HICON          image;    /* the image to render */
    HWND           owner;    /* the HWND passed in to the Shell_NotifyIcon call */
    HWND           window;   /* the adaptor window */
    HWND           tooltip;  /* Icon tooltip */
    UINT           id;       /* the unique id given by the app */
    UINT           callback_message;
};

static struct tray tray;
static BOOL hide_systray;

/* adaptor code */

#define ICON_SIZE GetSystemMetrics(SM_CXSMICON)
/* space around icon (forces icon to center of KDE systray area) */
#define ICON_BORDER  4

static LRESULT WINAPI adaptor_wndproc(HWND window, UINT msg,
                                      WPARAM wparam, LPARAM lparam)
{
    struct icon *icon = NULL;
    BOOL ret;

    WINE_TRACE("hwnd=%p, msg=0x%x\n", window, msg);

    /* set the icon data for the window from the data passed into CreateWindow */
    if (msg == WM_NCCREATE)
        SetWindowLongPtrW(window, GWLP_USERDATA, (LPARAM)((const CREATESTRUCT *)lparam)->lpCreateParams);

    icon = (struct icon *) GetWindowLongPtr(window, GWLP_USERDATA);

    switch (msg)
    {
        case WM_PAINT:
        {
            RECT rc;
            int top;
            PAINTSTRUCT  ps;
            HDC          hdc;

            WINE_TRACE("painting\n");

            hdc = BeginPaint(window, &ps);
            GetClientRect(window, &rc);

            /* calculate top so we can deal with arbitrary sized trays */
            top = ((rc.bottom-rc.top)/2) - ((ICON_SIZE)/2);

            DrawIconEx(hdc, (ICON_BORDER/2), top, icon->image,
                       ICON_SIZE, ICON_SIZE, 0, 0, DI_DEFAULTSIZE|DI_NORMAL);

            EndPaint(window, &ps);
            break;
        }

        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
            /* notify the owner hwnd of the message */
            WINE_TRACE("relaying 0x%x\n", msg);
            ret = PostMessage(icon->owner, icon->callback_message, (WPARAM) icon->id, (LPARAM) msg);
            if (!ret && (GetLastError() == ERROR_INVALID_HANDLE))
            {
                WINE_WARN("application window was destroyed without removing "
                          "notification icon, removing automatically\n");
                DestroyWindow(window);
            }
            break;

        case WM_NCDESTROY:
            SetWindowLongPtr(window, GWLP_USERDATA, 0);

            list_remove(&icon->entry);
            DestroyIcon(icon->image);
            HeapFree(GetProcessHeap(), 0, icon);
            break;

        default:
            return DefWindowProc(window, msg, wparam, lparam);
    }

    return 0;
}


/* listener code */

static struct icon *get_icon(HWND owner, UINT id)
{
    struct icon *this;

    /* search for the icon */
    LIST_FOR_EACH_ENTRY( this, &tray.icons, struct icon, entry )
        if ((this->id == id) && (this->owner == owner)) return this;

    return NULL;
}

static void set_tooltip(struct icon *icon, WCHAR *szTip, BOOL modify)
{
    TTTOOLINFOW ti;

    ti.cbSize = sizeof(TTTOOLINFOW);
    ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
    ti.hwnd = icon->window;
    ti.hinst = 0;
    ti.uId = (UINT_PTR)icon->window;
    ti.lpszText = szTip;
    ti.lParam = 0;
    ti.lpReserved = NULL;

    if (modify)
        SendMessageW(icon->tooltip, TTM_UPDATETIPTEXTW, 0, (LPARAM)&ti);
    else
        SendMessageW(icon->tooltip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
}

static void modify_icon(NOTIFYICONDATAW *nid, BOOL modify_tooltip)
{
    struct icon    *icon;

    WINE_TRACE("id=0x%x, hwnd=%p\n", nid->uID, nid->hWnd);

    /* demarshal the request from the NID */
    icon = get_icon(nid->hWnd, nid->uID);
    if (!icon)
    {
        WINE_WARN("Invalid icon ID (0x%x) for HWND %p\n", nid->uID, nid->hWnd);
        return;
    }

    if (nid->uFlags & NIF_ICON)
    {
        if (icon->image) DestroyIcon(icon->image);
        icon->image = CopyIcon(nid->hIcon);

        RedrawWindow(icon->window, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
    }

    if (nid->uFlags & NIF_MESSAGE)
    {
        icon->callback_message = nid->uCallbackMessage;
    }
    if (nid->uFlags & NIF_TIP)
    {
        set_tooltip(icon, nid->szTip, modify_tooltip);
    }
}

static void add_icon(NOTIFYICONDATAW *nid)
{
    RECT rect;
    struct icon  *icon;
    static const WCHAR adaptor_windowname[] = /* Wine System Tray Adaptor */ {'W','i','n','e',' ','S','y','s','t','e','m',' ','T','r','a','y',' ','A','d','a','p','t','o','r',0};
    static BOOL tooltps_initialized = FALSE;

    WINE_TRACE("id=0x%x, hwnd=%p\n", nid->uID, nid->hWnd);

    if ((icon = get_icon(nid->hWnd, nid->uID)))
    {
        WINE_WARN("duplicate tray icon add, buggy app?\n");
        return;
    }

    if (!(icon = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*icon))))
    {
        WINE_ERR("out of memory\n");
        return;
    }

    icon->id    = nid->uID;
    icon->owner = nid->hWnd;
    icon->image = NULL;

    rect.left = 0;
    rect.top = 0;
    rect.right = GetSystemMetrics(SM_CXSMICON) + ICON_BORDER;
    rect.bottom = GetSystemMetrics(SM_CYSMICON) + ICON_BORDER;
    AdjustWindowRect(&rect, WS_CLIPSIBLINGS | WS_CAPTION, FALSE);

    /* create the adaptor window */
    icon->window = CreateWindowEx(WS_EX_TRAYWINDOW, adaptor_classname,
                                  adaptor_windowname,
                                  WS_CLIPSIBLINGS | WS_CAPTION,
                                  CW_USEDEFAULT, CW_USEDEFAULT,
                                  rect.right - rect.left,
                                  rect.bottom - rect.top,
                                  NULL, NULL, NULL, icon);

    if (!hide_systray)
        ShowWindow(icon->window, SW_SHOWNA);

    /* create icon tooltip */

    /* Register tooltip classes if this is the first icon */
    if (!tooltps_initialized)
    {
        INITCOMMONCONTROLSEX init_tooltip;

        init_tooltip.dwSize = sizeof(INITCOMMONCONTROLSEX);
        init_tooltip.dwICC = ICC_TAB_CLASSES;

        InitCommonControlsEx(&init_tooltip);
        tooltps_initialized = TRUE;
    }

    icon->tooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
                                   WS_POPUP | TTS_ALWAYSTIP,
                                   CW_USEDEFAULT, CW_USEDEFAULT,
                                   CW_USEDEFAULT, CW_USEDEFAULT,
                                   icon->window, NULL, NULL, NULL);

    list_add_tail(&tray.icons, &icon->entry);

    modify_icon(nid, FALSE);
}

static void delete_icon(const NOTIFYICONDATAW *nid)
{
    struct icon *icon = get_icon(nid->hWnd, nid->uID);

    WINE_TRACE("id=0x%x, hwnd=%p\n", nid->uID, nid->hWnd);
   
    if (!icon)
    {
        WINE_ERR("invalid tray icon ID specified: %u\n", nid->uID);
        return;
    }

    DestroyWindow(icon->tooltip);
    DestroyWindow(icon->window);
}

static void handle_incoming(HWND hwndSource, COPYDATASTRUCT *cds)
{
    NOTIFYICONDATAW nid;
    DWORD cbSize;

    if (cds->cbData < NOTIFYICONDATAW_V1_SIZE) return;
    cbSize = ((PNOTIFYICONDATA)cds->lpData)->cbSize;
    if (cbSize < NOTIFYICONDATAW_V1_SIZE) return;

    ZeroMemory(&nid, sizeof(nid));
    memcpy(&nid, cds->lpData, min(sizeof(nid), cbSize));

    /* FIXME: if statement only needed because we don't support interprocess
     * icon handles */
    if ((nid.uFlags & NIF_ICON) && (cds->cbData >= nid.cbSize + 2 * sizeof(BITMAP)))
    {
        LONG cbMaskBits;
        LONG cbColourBits;
        BITMAP bmMask;
        BITMAP bmColour;
        const char *buffer = cds->lpData;

        buffer += nid.cbSize;

        memcpy(&bmMask, buffer, sizeof(bmMask));
        buffer += sizeof(bmMask);
        memcpy(&bmColour, buffer, sizeof(bmColour));
        buffer += sizeof(bmColour);

        cbMaskBits = (bmMask.bmPlanes * bmMask.bmWidth * bmMask.bmHeight * bmMask.bmBitsPixel) / 8;
        cbColourBits = (bmColour.bmPlanes * bmColour.bmWidth * bmColour.bmHeight * bmColour.bmBitsPixel) / 8;

        if (cds->cbData < nid.cbSize + 2 * sizeof(BITMAP) + cbMaskBits + cbColourBits)
        {
            WINE_ERR("buffer underflow\n");
            return;
        }

        /* sanity check */
        if ((bmColour.bmWidth != bmMask.bmWidth) || (bmColour.bmHeight != bmMask.bmHeight))
        {
            WINE_ERR("colour and mask bitmaps aren't consistent\n");
            return;
        }

        nid.hIcon = CreateIcon(NULL, bmColour.bmWidth, bmColour.bmHeight,
                               bmColour.bmPlanes, bmColour.bmBitsPixel,
                               buffer, buffer + cbMaskBits);
    }

    switch (cds->dwData)
    {
    case NIM_ADD:
        add_icon(&nid);
        break;
    case NIM_DELETE:
        delete_icon(&nid);
        break;
    case NIM_MODIFY:
        modify_icon(&nid, TRUE);
        break;
    default:
        WINE_FIXME("unhandled tray message: %ld\n", cds->dwData);
        break;
    }

    /* FIXME: if statement only needed because we don't support interprocess
     * icon handles */
    if (nid.uFlags & NIF_ICON)
        DestroyIcon(nid.hIcon);
}

static LRESULT WINAPI listener_wndproc(HWND window, UINT msg,
                                       WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_COPYDATA)
        handle_incoming((HWND)wparam, (COPYDATASTRUCT *)lparam);

    return DefWindowProc(window, msg, wparam, lparam);
}


static BOOL is_systray_hidden(void)
{
    const WCHAR show_systray_keyname[] = {'S','o','f','t','w','a','r','e','\\','W','i','n','e','\\',
                                          'X','1','1',' ','D','r','i','v','e','r',0};
    const WCHAR show_systray_valuename[] = {'S','h','o','w','S','y','s','t','r','a','y',0};
    HKEY hkey;
    BOOL ret = FALSE;

    /* @@ Wine registry key: HKCU\Software\Wine\X11 Driver */
    if (RegOpenKeyW(HKEY_CURRENT_USER, show_systray_keyname, &hkey) == ERROR_SUCCESS)
    {
        WCHAR value[10];
        DWORD type, size = sizeof(value);
        if (RegQueryValueExW(hkey, show_systray_valuename, 0, &type, (LPBYTE)&value, &size) == ERROR_SUCCESS)
        {
            ret = IS_OPTION_FALSE(value[0]);
        }
        RegCloseKey(hkey);
    }
    return ret;
}

/* this function creates the the listener window */
void initialize_systray(void)
{
    WNDCLASSEX class;
    static const WCHAR classname[] = /* Shell_TrayWnd */ {'S','h','e','l','l','_','T','r','a','y','W','n','d',0};
    static const WCHAR winname[]   = /* Wine Systray Listener */
        {'W','i','n','e',' ','S','y','s','t','r','a','y',' ','L','i','s','t','e','n','e','r',0};

    WINE_TRACE("initiaizing\n");

    hide_systray = is_systray_hidden();

    list_init(&tray.icons);

    /* register the systray listener window class */
    ZeroMemory(&class, sizeof(class));
    class.cbSize        = sizeof(class);
    class.lpfnWndProc   = &listener_wndproc;
    class.hInstance     = NULL;
    class.hIcon         = LoadIcon(0, IDI_WINLOGO);
    class.hCursor       = LoadCursor(0, IDC_ARROW);
    class.hbrBackground = (HBRUSH) COLOR_WINDOW;
    class.lpszClassName = (WCHAR *) &classname;

    if (!RegisterClassEx(&class))
    {
        WINE_ERR("Could not register SysTray window class\n");
        return;
    }

    /* now register the adaptor window class */
    ZeroMemory(&class, sizeof(class));
    class.cbSize        = sizeof(class);
    class.lpfnWndProc   = adaptor_wndproc;
    class.hInstance     = NULL;
    class.hIcon         = LoadIcon(0, IDI_WINLOGO);
    class.hCursor       = LoadCursor(0, IDC_ARROW);
    class.hbrBackground = (HBRUSH) COLOR_WINDOW;
    class.lpszClassName = adaptor_classname;
    class.style         = CS_SAVEBITS | CS_DBLCLKS;

    if (!RegisterClassEx(&class))
    {
        WINE_ERR("Could not register adaptor class\n");
        return;
    }

    tray.window = CreateWindow(classname, winname, WS_OVERLAPPED,
                               CW_USEDEFAULT, CW_USEDEFAULT,
                               0, 0, 0, 0, 0, 0);

    if (!tray.window)
    {
        WINE_ERR("Could not create tray window\n");
        return;
    }
}
