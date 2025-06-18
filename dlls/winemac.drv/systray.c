/*
 * Mac driver system tray management
 *
 * Copyright (C) 2004 Mike Hearn, for CodeWeavers
 * Copyright (C) 2005 Robert Shearman
 * Copyright (C) 2008 Alexandre Julliard
 * Copyright (C) 2012, 2013 Ken Thomases for CodeWeavers Inc.
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include "macdrv.h"

#include "windef.h"
#include "winuser.h"
#include "shellapi.h"

#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(systray);


/* an individual systray icon */
struct tray_icon
{
    struct list         entry;
    HWND                owner;              /* the HWND passed in to the Shell_NotifyIcon call */
    UINT                id;                 /* the unique id given by the app */
    UINT                callback_message;
    HICON               image;              /* the image to render */
    WCHAR               tiptext[128];       /* tooltip text */
    DWORD               state;              /* state flags */
    macdrv_status_item  status_item;
    UINT                version;
};

static struct list icon_list = LIST_INIT(icon_list);


static BOOL delete_icon(struct tray_icon *icon);



/***********************************************************************
 *              CleanupIcons   (MACDRV.@)
 */
void macdrv_CleanupIcons(HWND hwnd)
{
    struct tray_icon *icon, *next;

    LIST_FOR_EACH_ENTRY_SAFE(icon, next, &icon_list, struct tray_icon, entry)
        if (icon->owner == hwnd) delete_icon(icon);
}


/***********************************************************************
 *              get_icon
 *
 * Retrieves an icon record by owner window and ID.
 */
static struct tray_icon *get_icon(HWND owner, UINT id)
{
    struct tray_icon *this;

    LIST_FOR_EACH_ENTRY(this, &icon_list, struct tray_icon, entry)
        if ((this->id == id) && (this->owner == owner)) return this;
    return NULL;
}


/***********************************************************************
 *              modify_icon
 *
 * Modifies an existing tray icon and updates its status item as needed.
 */
static BOOL modify_icon(struct tray_icon *icon, NOTIFYICONDATAW *nid)
{
    BOOL update_image = FALSE, update_tooltip = FALSE;

    TRACE("hwnd %p id 0x%x flags %x\n", nid->hWnd, nid->uID, nid->uFlags);

    if (nid->uFlags & NIF_STATE)
    {
        DWORD changed = (icon->state ^ nid->dwState) & nid->dwStateMask;
        icon->state = (icon->state & ~nid->dwStateMask) | (nid->dwState & nid->dwStateMask);
        if (changed & NIS_HIDDEN)
        {
            if (icon->state & NIS_HIDDEN)
            {
                if (icon->status_item)
                {
                    TRACE("destroying status item %p\n", icon->status_item);
                    macdrv_destroy_status_item(icon->status_item);
                    icon->status_item = NULL;
                }
            }
            else
            {
                if (!icon->status_item)
                {
                    struct macdrv_thread_data *thread_data = macdrv_init_thread_data();

                    icon->status_item = macdrv_create_status_item(thread_data->queue);
                    if (icon->status_item)
                    {
                        TRACE("created status item %p\n", icon->status_item);

                        if (icon->image)
                            update_image = TRUE;
                        if (*icon->tiptext)
                            update_tooltip = TRUE;
                    }
                    else
                        WARN("failed to create status item\n");
                }
            }
        }
    }

    if (nid->uFlags & NIF_ICON)
    {
        if (icon->image) NtUserDestroyCursor(icon->image, 0);
        icon->image = CopyImage(nid->hIcon, IMAGE_ICON, 0, 0, 0);
        if (icon->status_item)
            update_image = TRUE;
    }

    if (nid->uFlags & NIF_MESSAGE)
    {
        icon->callback_message = nid->uCallbackMessage;
    }
    if (nid->uFlags & NIF_TIP)
    {
        lstrcpynW(icon->tiptext, nid->szTip, ARRAY_SIZE(icon->tiptext));
        if (icon->status_item)
            update_tooltip = TRUE;
    }

    if (update_image)
    {
        CGImageRef cgimage = NULL;
        if (icon->image)
            cgimage = create_cgimage_from_icon(icon->image, 0, 0);
        macdrv_set_status_item_image(icon->status_item, cgimage);
        CGImageRelease(cgimage);
    }

    if (update_tooltip)
    {
        CFStringRef s;

        TRACE("setting tooltip text for status item %p to %s\n", icon->status_item,
              debugstr_w(icon->tiptext));
        s = CFStringCreateWithCharacters(NULL, (UniChar*)icon->tiptext,
                                         lstrlenW(icon->tiptext));
        macdrv_set_status_item_tooltip(icon->status_item, s);
        CFRelease(s);
    }

    return TRUE;
}


/***********************************************************************
 *              add_icon
 *
 * Creates a new tray icon structure and adds it to the list.
 */
static BOOL add_icon(NOTIFYICONDATAW *nid)
{
    NOTIFYICONDATAW new_nid;
    struct tray_icon *icon;

    TRACE("hwnd %p id 0x%x\n", nid->hWnd, nid->uID);

    if ((icon = get_icon(nid->hWnd, nid->uID)))
    {
        WARN("duplicate tray icon add, buggy app?\n");
        return FALSE;
    }

    if (!(icon = calloc(1, sizeof(*icon))))
    {
        ERR("out of memory\n");
        return FALSE;
    }

    icon->id     = nid->uID;
    icon->owner  = nid->hWnd;
    icon->state  = NIS_HIDDEN;

    list_add_tail(&icon_list, &icon->entry);

    if (!(nid->uFlags & NIF_STATE) || !(nid->dwStateMask & NIS_HIDDEN))
    {
        new_nid = *nid;
        new_nid.uFlags |= NIF_STATE;
        new_nid.dwState     &= ~NIS_HIDDEN;
        new_nid.dwStateMask |= NIS_HIDDEN;
        nid = &new_nid;
    }
    return modify_icon(icon, nid);
}


/***********************************************************************
 *              delete_icon
 *
 * Destroy tray icon status item and delete structure.
 */
static BOOL delete_icon(struct tray_icon *icon)
{
    TRACE("hwnd %p id 0x%x\n", icon->owner, icon->id);

    if (icon->status_item)
    {
        TRACE("destroying status item %p\n", icon->status_item);
        macdrv_destroy_status_item(icon->status_item);
    }
    list_remove(&icon->entry);
    NtUserDestroyCursor(icon->image, 0);
    free(icon);
    return TRUE;
}


/***********************************************************************
 *              NotifyIcon   (MACDRV.@)
 */
LRESULT macdrv_NotifyIcon(HWND hwnd, UINT msg, NOTIFYICONDATAW *data)
{
    BOOL ret = FALSE;
    struct tray_icon *icon;

    switch (msg)
    {
    case NIM_ADD:
        ret = add_icon(data);
        break;
    case NIM_DELETE:
        if ((icon = get_icon(data->hWnd, data->uID))) ret = delete_icon(icon);
        break;
    case NIM_MODIFY:
        if ((icon = get_icon(data->hWnd, data->uID))) ret = modify_icon(icon, data);
        break;
    case NIM_SETVERSION:
        if ((icon = get_icon(data->hWnd, data->uID)))
        {
            icon->version = data->uVersion;
            ret = TRUE;
        }
        break;
    default:
        ERR("Unexpected NotifyIconProc call\n");
        return -1;
    }
    return ret;
}

static BOOL notify_owner(struct tray_icon *icon, UINT msg, int x, int y)
{
    WPARAM wp = icon->id;
    LPARAM lp = msg;

    if (icon->version >= NOTIFYICON_VERSION_4)
    {
        wp = MAKEWPARAM(x, y);
        lp = MAKELPARAM(msg, icon->id);
    }

    TRACE("posting msg 0x%04x to hwnd %p id 0x%x\n", msg, icon->owner, icon->id);
    if (!NtUserMessageCall(icon->owner, icon->callback_message, wp, lp,
                           0, NtUserSendNotifyMessage, FALSE) &&
        (RtlGetLastWin32Error() == ERROR_INVALID_WINDOW_HANDLE))
    {
        WARN("window %p was destroyed, removing icon 0x%x\n", icon->owner, icon->id);
        delete_icon(icon);
        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 *              macdrv_status_item_mouse_button
 *
 * Handle STATUS_ITEM_MOUSE_BUTTON events.
 */
void macdrv_status_item_mouse_button(const macdrv_event *event)
{
    struct tray_icon *icon;

    TRACE("item %p button %d down %d count %d pos %d,%d\n", event->status_item_mouse_button.item,
          event->status_item_mouse_button.button, event->status_item_mouse_button.down,
          event->status_item_mouse_button.count, event->status_item_mouse_button.x,
          event->status_item_mouse_button.y);

    LIST_FOR_EACH_ENTRY(icon, &icon_list, struct tray_icon, entry)
    {
        if (icon->status_item == event->status_item_mouse_button.item)
        {
            UINT msg;

            switch (event->status_item_mouse_button.button)
            {
                case 0: msg = WM_LBUTTONDOWN; break;
                case 1: msg = WM_RBUTTONDOWN; break;
                case 2: msg = WM_MBUTTONDOWN; break;
                default:
                    TRACE("ignoring button beyond the third\n");
                    return;
            }

            if (!event->status_item_mouse_button.down)
                msg += WM_LBUTTONUP - WM_LBUTTONDOWN;
            else if (event->status_item_mouse_button.count % 2 == 0)
                msg += WM_LBUTTONDBLCLK - WM_LBUTTONDOWN;

            send_message(icon->owner, WM_MACDRV_ACTIVATE_ON_FOLLOWING_FOCUS, 0, 0);

            if (!notify_owner(icon, msg, event->status_item_mouse_button.x, event->status_item_mouse_button.y))
                return;

            if (icon->version)
            {
                if (msg == WM_LBUTTONUP)
                    notify_owner(icon, NIN_SELECT, event->status_item_mouse_button.x, event->status_item_mouse_button.y);
                else if (msg == WM_RBUTTONUP)
                    notify_owner(icon, WM_CONTEXTMENU, event->status_item_mouse_button.x, event->status_item_mouse_button.y);
            }

            break;
        }
    }
}


/***********************************************************************
 *              macdrv_status_item_mouse_move
 *
 * Handle STATUS_ITEM_MOUSE_MOVE events.
 */
void macdrv_status_item_mouse_move(const macdrv_event *event)
{
    struct tray_icon *icon;

    TRACE("item %p pos %d,%d\n", event->status_item_mouse_move.item,
          event->status_item_mouse_move.x, event->status_item_mouse_move.y);

    LIST_FOR_EACH_ENTRY(icon, &icon_list, struct tray_icon, entry)
    {
        if (icon->status_item == event->status_item_mouse_move.item)
        {
            notify_owner(icon, WM_MOUSEMOVE, event->status_item_mouse_move.x, event->status_item_mouse_move.y);
            break;
        }
    }
}
