/*
 * winemac.drv entry points
 *
 * Copyright 2022 Jacek Caban for CodeWeavers
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

#include <stdarg.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "ntgdi.h"
#include "macdrv_res.h"
#include "shellapi.h"
#include "unixlib.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(macdrv);


static HMODULE macdrv_module = 0;

struct quit_info {
    HWND               *wins;
    UINT                capacity;
    UINT                count;
    UINT                done;
    DWORD               flags;
    BOOL                result;
    BOOL                replied;
};


static BOOL CALLBACK get_process_windows(HWND hwnd, LPARAM lp)
{
    struct quit_info *qi = (struct quit_info*)lp;
    DWORD pid;

    NtUserGetWindowThread(hwnd, &pid);
    if (pid == GetCurrentProcessId())
    {
        if (qi->count >= qi->capacity)
        {
            UINT new_cap = qi->capacity * 2;
            HWND *new_wins = HeapReAlloc(GetProcessHeap(), 0, qi->wins, new_cap * sizeof(*qi->wins));
            if (!new_wins) return FALSE;
            qi->wins = new_wins;
            qi->capacity = new_cap;
        }

        qi->wins[qi->count++] = hwnd;
    }

    return TRUE;
}

#pragma pack(push,1)

typedef struct
{
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD wPlanes;
    WORD wBitCount;
    DWORD dwBytesInRes;
    WORD nID;
} GRPICONDIRENTRY;

typedef struct
{
    WORD idReserved;
    WORD idType;
    WORD idCount;
    GRPICONDIRENTRY idEntries[1];
} GRPICONDIR;

#pragma pack(pop)

static void quit_reply(int reply)
{
    struct quit_result_params params = { .result = reply };
    MACDRV_CALL(quit_result, &params);
}


static void CALLBACK quit_callback(HWND hwnd, UINT msg, ULONG_PTR data, LRESULT result)
{
    struct quit_info *qi = (struct quit_info*)data;

    qi->done++;

    if (msg == WM_QUERYENDSESSION)
    {
        TRACE("got WM_QUERYENDSESSION result %Id from win %p (%u of %u done)\n", result,
              hwnd, qi->done, qi->count);

        if (!result && !IsWindow(hwnd))
        {
            TRACE("win %p no longer exists; ignoring apparent refusal\n", hwnd);
            result = TRUE;
        }

        if (!result && qi->result)
        {
            qi->result = FALSE;

            /* On the first FALSE from WM_QUERYENDSESSION, we already know the
               ultimate reply.  Might as well tell Cocoa now. */
            if (!qi->replied)
            {
                qi->replied = TRUE;
                TRACE("giving quit reply %d\n", qi->result);
                quit_reply(qi->result);
            }
        }

        if (qi->done >= qi->count)
        {
            UINT i;

            qi->done = 0;
            for (i = 0; i < qi->count; i++)
            {
                TRACE("sending WM_ENDSESSION to win %p result %d flags 0x%08lx\n", qi->wins[i],
                      qi->result, qi->flags);
                if (!SendMessageCallbackW(qi->wins[i], WM_ENDSESSION, qi->result, qi->flags,
                                          quit_callback, (ULONG_PTR)qi))
                {
                    WARN("failed to send WM_ENDSESSION to win %p; error 0x%08lx\n",
                         qi->wins[i], RtlGetLastWin32Error());
                    quit_callback(qi->wins[i], WM_ENDSESSION, (ULONG_PTR)qi, 0);
                }
            }
        }
    }
    else /* WM_ENDSESSION */
    {
        TRACE("finished WM_ENDSESSION for win %p (%u of %u done)\n", hwnd, qi->done, qi->count);

        if (qi->done >= qi->count)
        {
            if (!qi->replied)
            {
                TRACE("giving quit reply %d\n", qi->result);
                quit_reply(qi->result);
            }

            TRACE("%sterminating process\n", qi->result ? "" : "not ");
            if (qi->result)
                TerminateProcess(GetCurrentProcess(), 0);

            HeapFree(GetProcessHeap(), 0, qi->wins);
            HeapFree(GetProcessHeap(), 0, qi);
        }
    }
}


/***********************************************************************
 *              macdrv_app_quit_request
 */
NTSTATUS WINAPI macdrv_app_quit_request(void *arg, ULONG size)
{
    struct app_quit_request_params *params = arg;
    struct quit_info *qi;
    UINT i;

    qi = HeapAlloc(GetProcessHeap(), 0, sizeof(*qi));
    if (!qi)
        goto fail;

    qi->capacity = 32;
    qi->wins = HeapAlloc(GetProcessHeap(), 0, qi->capacity * sizeof(*qi->wins));
    qi->count = qi->done = 0;

    if (!qi->wins || !EnumWindows(get_process_windows, (LPARAM)qi))
        goto fail;

    qi->flags = params->flags;
    qi->result = TRUE;
    qi->replied = FALSE;

    for (i = 0; i < qi->count; i++)
    {
        TRACE("sending WM_QUERYENDSESSION to win %p\n", qi->wins[i]);
        if (!SendMessageCallbackW(qi->wins[i], WM_QUERYENDSESSION, 0, qi->flags,
                                  quit_callback, (ULONG_PTR)qi))
        {
            DWORD error = RtlGetLastWin32Error();
            BOOL invalid = (error == ERROR_INVALID_WINDOW_HANDLE);
            if (invalid)
                TRACE("failed to send WM_QUERYENDSESSION to win %p because it's invalid; assuming success\n",
                     qi->wins[i]);
            else
                WARN("failed to send WM_QUERYENDSESSION to win %p; error 0x%08lx; assuming refusal\n",
                     qi->wins[i], error);
            quit_callback(qi->wins[i], WM_QUERYENDSESSION, (ULONG_PTR)qi, invalid);
        }
    }

    /* quit_callback() will clean up qi */
    return STATUS_SUCCESS;

fail:
    WARN("failed to allocate window list\n");
    if (qi)
    {
        HeapFree(GetProcessHeap(), 0, qi->wins);
        HeapFree(GetProcessHeap(), 0, qi);
    }
    quit_reply(FALSE);
    return STATUS_SUCCESS;
}

/***********************************************************************
 *              get_first_resource
 *
 * Helper for create_app_icon_images().  Enum proc for EnumResourceNamesW()
 * which just gets the handle for the first resource and stops further
 * enumeration.
 */
static BOOL CALLBACK get_first_resource(HMODULE module, LPCWSTR type, LPWSTR name, LONG_PTR lparam)
{
    HRSRC *res_info = (HRSRC*)lparam;

    *res_info = FindResourceW(module, name, (LPCWSTR)RT_GROUP_ICON);
    return FALSE;
}


/***********************************************************************
 *              macdrv_app_icon
 */
static NTSTATUS WINAPI macdrv_app_icon(void *arg, ULONG size)
{
    struct app_icon_entry entries[64];
    HRSRC res_info;
    HGLOBAL res_data;
    GRPICONDIR *icon_dir;
    unsigned count;
    int i;

    TRACE("()\n");

    count = 0;

    res_info = NULL;
    EnumResourceNamesW(NULL, (LPCWSTR)RT_GROUP_ICON, get_first_resource, (LONG_PTR)&res_info);
    if (!res_info)
    {
        WARN("found no RT_GROUP_ICON resource\n");
        return STATUS_SUCCESS;
    }

    if (!(res_data = LoadResource(NULL, res_info)))
    {
        WARN("failed to load RT_GROUP_ICON resource\n");
        return STATUS_SUCCESS;
    }

    if (!(icon_dir = LockResource(res_data)))
    {
        WARN("failed to lock RT_GROUP_ICON resource\n");
        goto cleanup;
    }

    for (i = 0; i < icon_dir->idCount && count < ARRAYSIZE(entries); i++)
    {
        struct app_icon_entry *entry = &entries[count];
        int width = icon_dir->idEntries[i].bWidth;
        int height = icon_dir->idEntries[i].bHeight;
        BOOL found_better_bpp = FALSE;
        int j;
        LPCWSTR name;
        HGLOBAL icon_res_data;
        BYTE *icon_bits;

        if (!width) width = 256;
        if (!height) height = 256;

        /* If there's another icon at the same size but with better
           color depth, skip this one.  We end up making CGImages that
           are all 32 bits per pixel, so Cocoa doesn't get the original
           color depth info to pick the best representation itself. */
        for (j = 0; j < icon_dir->idCount; j++)
        {
            int jwidth = icon_dir->idEntries[j].bWidth;
            int jheight = icon_dir->idEntries[j].bHeight;

            if (!jwidth) jwidth = 256;
            if (!jheight) jheight = 256;

            if (j != i && jwidth == width && jheight == height &&
                icon_dir->idEntries[j].wBitCount > icon_dir->idEntries[i].wBitCount)
            {
                found_better_bpp = TRUE;
                break;
            }
        }

        if (found_better_bpp) continue;

        name = MAKEINTRESOURCEW(icon_dir->idEntries[i].nID);
        res_info = FindResourceW(NULL, name, (LPCWSTR)RT_ICON);
        if (!res_info)
        {
            WARN("failed to find RT_ICON resource %d with ID %hd\n", i, icon_dir->idEntries[i].nID);
            continue;
        }

        icon_res_data = LoadResource(NULL, res_info);
        if (!icon_res_data)
        {
            WARN("failed to load icon %d with ID %hd\n", i, icon_dir->idEntries[i].nID);
            continue;
        }

        icon_bits = LockResource(icon_res_data);
        if (icon_bits)
        {
            static const BYTE png_magic[] = { 0x89, 0x50, 0x4e, 0x47 };

            entry->width = width;
            entry->height = height;
            entry->size = icon_dir->idEntries[i].dwBytesInRes;

            if (!memcmp(icon_bits, png_magic, sizeof(png_magic)))
            {
                entry->png = (UINT_PTR)icon_bits;
                entry->icon = 0;
                count++;
            }
            else
            {
                HICON icon = CreateIconFromResourceEx(icon_bits, icon_dir->idEntries[i].dwBytesInRes,
                                                      TRUE, 0x00030000, width, height, 0);
                if (icon)
                {
                    entry->icon = HandleToUlong(icon);
                    entry->png = 0;
                    count++;
                }
                else
                    WARN("failed to create icon %d from resource with ID %hd\n", i, icon_dir->idEntries[i].nID);
            }
        }
        else
            WARN("failed to lock RT_ICON resource %d with ID %hd\n", i, icon_dir->idEntries[i].nID);

        FreeResource(icon_res_data);
    }

cleanup:
    FreeResource(res_data);

    return NtCallbackReturn(entries, count * sizeof(entries[0]), 0);
}


static BOOL process_attach(void)
{
    struct init_params params;

    struct localized_string *str;
    struct localized_string strings[] = {
        { .id = STRING_MENU_WINE },
        { .id = STRING_MENU_ITEM_HIDE_APPNAME },
        { .id = STRING_MENU_ITEM_HIDE },
        { .id = STRING_MENU_ITEM_HIDE_OTHERS },
        { .id = STRING_MENU_ITEM_SHOW_ALL },
        { .id = STRING_MENU_ITEM_QUIT_APPNAME },
        { .id = STRING_MENU_ITEM_QUIT },

        { .id = STRING_MENU_WINDOW },
        { .id = STRING_MENU_ITEM_MINIMIZE },
        { .id = STRING_MENU_ITEM_ZOOM },
        { .id = STRING_MENU_ITEM_ENTER_FULL_SCREEN },
        { .id = STRING_MENU_ITEM_BRING_ALL_TO_FRONT },

        { .id = 0 }
    };

    if (__wine_init_unix_call()) return FALSE;

    for (str = strings; str->id; str++)
        str->len = LoadStringW(macdrv_module, str->id, (WCHAR *)&str->str, 0);
    params.strings = strings;
    params.app_icon_callback = (UINT_PTR)macdrv_app_icon;
    params.app_quit_request_callback = (UINT_PTR)macdrv_app_quit_request;

    if (MACDRV_CALL(init, &params)) return FALSE;

    return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    if (reason != DLL_PROCESS_ATTACH) return TRUE;

    DisableThreadLibraryCalls(instance);
    macdrv_module = instance;
    return process_attach();
}
