/*
 *  ReactOS Task Manager
 *
 *  dbgchnl.c
 *
 *  Copyright (C) 2003 - 2004 Eric Pouech
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
    
#define WIN32_LEAN_AND_MEAN    /* Exclude rarely-used stuff from Windows headers */
#include <windows.h>
#include <ctype.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <stdio.h>
#include <winnt.h>
#include <dbghelp.h>
    
#include "taskmgr.h"
#include "perfdata.h"
#include "column.h"
#include "wine/debug.h"

/* TODO:
 *      - the dialog box could be non modal
 *      - in that case,
 *              + could refresh channels from time to time
 *              + set the name of exec (and perhaps its pid) in dialog title
 *      - get a better UI (replace the 'x' by real tick boxes in list view)
 *      - enhance visual feedback: the list is large, and it's hard to get the
 *        right line when clicking on rightmost column (trace for example)
 *      - get rid of printfs (error reporting) and use real message boxes
 *      - include the column width settings in the full column management scheme
 *      - use more global settings (like having a temporary on/off
 *        setting for a fixme:s or err:s
 */

static DWORD (WINAPI *pSymSetOptions)(DWORD);
static BOOL  (WINAPI *pSymInitialize)(HANDLE, PSTR, BOOL);
static DWORD (WINAPI *pSymLoadModule)(HANDLE, HANDLE, PCSTR, PCSTR, DWORD, DWORD);
static BOOL  (WINAPI *pSymCleanup)(HANDLE);
static BOOL  (WINAPI *pSymFromName)(HANDLE, PCSTR, PSYMBOL_INFO);

BOOL AreDebugChannelsSupported(void)
{
    static HANDLE   hDbgHelp /* = NULL */;

    if (hDbgHelp) return TRUE;

    if (!(hDbgHelp = LoadLibrary("dbghelp.dll"))) return FALSE;
    pSymSetOptions = (void*)GetProcAddress(hDbgHelp, "SymSetOptions");
    pSymInitialize = (void*)GetProcAddress(hDbgHelp, "SymInitialize");
    pSymLoadModule = (void*)GetProcAddress(hDbgHelp, "SymLoadModule");
    pSymFromName   = (void*)GetProcAddress(hDbgHelp, "SymFromName");
    pSymCleanup    = (void*)GetProcAddress(hDbgHelp, "SymCleanup");
    if (!pSymSetOptions || !pSymInitialize || !pSymLoadModule || !pSymCleanup || !pSymFromName)
    {
        FreeLibrary(hDbgHelp);
        hDbgHelp = NULL;
        return FALSE;
    }
    return TRUE;
}

static DWORD    get_selected_pid(void)
{
    LVITEM      lvitem;
    ULONG       Index;
    DWORD       dwProcessId;

    for (Index = 0; Index < (ULONG)ListView_GetItemCount(hProcessPageListCtrl); Index++)
    {
        lvitem.mask = LVIF_STATE;
        lvitem.stateMask = LVIS_SELECTED;
        lvitem.iItem = Index;
        lvitem.iSubItem = 0;

        SendMessage(hProcessPageListCtrl, LVM_GETITEM, 0, (LPARAM) &lvitem);

        if (lvitem.state & LVIS_SELECTED)
            break;
    }

    dwProcessId = PerfDataGetProcessId(Index);

    if ((ListView_GetSelectedCount(hProcessPageListCtrl) != 1) || (dwProcessId == 0))
        return 0;
    return dwProcessId;
}

static int     list_channel_CB(HANDLE hProcess, void* addr, struct __wine_debug_channel* channel, void* user)
{
    int         j;
    char        val[2];
    LVITEMA     lvitem;
    int         index;
    HWND        hChannelLV = (HWND)user;

    lvitem.mask = LVIF_TEXT;
    lvitem.pszText = channel->name;
    lvitem.iItem = 0;
    lvitem.iSubItem = 0;

    index = ListView_InsertItem(hChannelLV, &lvitem);
    if (index == -1) return 0;

    val[1] = '\0';
    for (j = 0; j < 4; j++)
    {
        val[0] = (channel->flags & (1 << j)) ? 'x' : ' ';
        ListView_SetItemText(hChannelLV, index, j + 1, val);
    }
    return 1;
}

struct cce_user
{
    const char* name;           /* channel to look for */
    unsigned    value, mask;    /* how to change channel */
    unsigned    done;           /* number of successful changes */
    unsigned    notdone;        /* number of unsuccessful changes */
};

/******************************************************************
 *		change_channel_CB
 *
 * Callback used for changing a given channel attributes
 */
static int change_channel_CB(HANDLE hProcess, void* addr, struct __wine_debug_channel *channel, void* pmt)
{
    struct cce_user* user = (struct cce_user*)pmt;

    if (!user->name || !strcmp(channel->name, user->name))
    {
        channel->flags = (channel->flags & ~user->mask) | (user->value & user->mask);
        if (WriteProcessMemory(hProcess, addr, channel, sizeof(*channel), NULL))
            user->done++;
        else
            user->notdone++;
    }
    return 1;
}

static void* get_symbol(HANDLE hProcess, const char* name)
{
    char                buffer[sizeof(IMAGEHLP_SYMBOL) + 256];
    SYMBOL_INFO*        si = (SYMBOL_INFO*)buffer;
    void*               ret = NULL;

    /* also ask for wine extensions (loading symbols from ELF files) */
    pSymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_PUBLICS_ONLY | 0x40000000);
    /* FIXME: the TRUE option is due to the fact that dbghelp requires it
     * when loading an ELF module
     */
    if (pSymInitialize(hProcess, NULL, TRUE))
    {
        si->SizeOfStruct = sizeof(*si);
        si->MaxNameLen = sizeof(buffer) - sizeof(IMAGEHLP_SYMBOL);
        if (pSymFromName(hProcess, name, si))
            ret = (void*)(ULONG_PTR)si->Address;
        pSymCleanup(hProcess);
    }
    return ret;
}

typedef int (*EnumChannelCB)(HANDLE, void*, struct __wine_debug_channel*, void*);

/******************************************************************
 *		enum_channel
 *
 * Enumerates all known channels on process hProcess through callback
 * ce.
 */
static int enum_channel(HANDLE hProcess, EnumChannelCB ce, void* user)
{
    struct __wine_debug_channel channel;
    int                         ret = 1;
    void*                       addr;

    if (!(addr = get_symbol(hProcess, "libwine.so.1!debug_options"))) return -1;

    while (ret && addr && ReadProcessMemory(hProcess, addr, &channel, sizeof(channel), NULL))
    {
        if (!channel.name[0]) break;
        ret = ce(hProcess, addr, &channel, user);
        addr = (struct __wine_debug_channel *)addr + 1;
    }
    return 0;
}

static void DebugChannels_FillList(HWND hChannelLV)
{
    HANDLE      hProcess;

    SendMessage(hChannelLV, LVM_DELETEALLITEMS, 0, 0);

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ, FALSE, get_selected_pid());
    if (!hProcess) return; /* FIXME messagebox */
    SendMessage(hChannelLV, WM_SETREDRAW, FALSE, 0);
    enum_channel(hProcess, list_channel_CB, (void*)hChannelLV);
    SendMessage(hChannelLV, WM_SETREDRAW, TRUE, 0);
    CloseHandle(hProcess);
}

static void DebugChannels_OnCreate(HWND hwndDlg)
{
    HWND        hLV = GetDlgItem(hwndDlg, IDC_DEBUG_CHANNELS_LIST);
    LVCOLUMN    lvc;
    static TCHAR debug_channelT[] = {'D','e','b','u','g',' ','C','h','a','n','n','e','l',0},
                 fixmeT[]         = {'F','i','x','m','e',0},
                 errT[]           = {'E','r','r',0},
                 warnT[]          = {'W','a','r','n',0},
                 traceT[]         = {'T','r','a','c','e',0};

    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.pszText = debug_channelT;
    lvc.cx = 100;
    SendMessage(hLV, LVM_INSERTCOLUMN, 0, (LPARAM) &lvc);

    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_CENTER;
    lvc.pszText = fixmeT;
    lvc.cx = 55;
    SendMessage(hLV, LVM_INSERTCOLUMN, 1, (LPARAM) &lvc);

    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_CENTER;
    lvc.pszText = errT;
    lvc.cx = 55;
    SendMessage(hLV, LVM_INSERTCOLUMN, 2, (LPARAM) &lvc);

    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_CENTER;
    lvc.pszText = warnT;
    lvc.cx = 55;
    SendMessage(hLV, LVM_INSERTCOLUMN, 3, (LPARAM) &lvc);

    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_CENTER;
    lvc.pszText = traceT;
    lvc.cx = 55;
    SendMessage(hLV, LVM_INSERTCOLUMN, 4, (LPARAM) &lvc);

    DebugChannels_FillList(hLV);
}

static void DebugChannels_OnNotify(HWND hDlg, LPARAM lParam)
{
    NMHDR*      nmh = (NMHDR*)lParam;

    switch (nmh->code)
    {
    case NM_CLICK:
        if (nmh->idFrom == IDC_DEBUG_CHANNELS_LIST)
        {
            LVHITTESTINFO       lhti;
            HWND                hChannelLV;
            HANDLE              hProcess;
            NMITEMACTIVATE*     nmia = (NMITEMACTIVATE*)lParam;

            hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, get_selected_pid());
            if (!hProcess) return; /* FIXME message box */
            lhti.pt = nmia->ptAction;
            hChannelLV = GetDlgItem(hDlg, IDC_DEBUG_CHANNELS_LIST);
            SendMessage(hChannelLV, LVM_SUBITEMHITTEST, 0, (LPARAM)&lhti);
            if (nmia->iSubItem >= 1 && nmia->iSubItem <= 4)
            {
                TCHAR           val[2];
                TCHAR           name[32];
                unsigned        bitmask = 1 << (lhti.iSubItem - 1);
                struct cce_user user;

                ListView_GetItemText(hChannelLV, lhti.iItem, 0, name, sizeof(name) / sizeof(name[0]));
                ListView_GetItemText(hChannelLV, lhti.iItem, lhti.iSubItem, val, sizeof(val) / sizeof(val[0]));
                user.name = name;
                user.value = (val[0] == 'x') ? 0 : bitmask;
                user.mask = bitmask;
                user.done = user.notdone = 0;
                enum_channel(hProcess, change_channel_CB, &user);
                if (user.done)
                {
                    val[0] ^= ('x' ^ ' ');
                    ListView_SetItemText(hChannelLV, lhti.iItem, lhti.iSubItem, val);
                }
                if (user.notdone)
                    printf("Some channel instances weren't correctly set\n");
            }
            CloseHandle(hProcess);
        }
        break;
    }
}

static INT_PTR CALLBACK DebugChannelsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        DebugChannels_OnCreate(hDlg);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    case WM_NOTIFY:
        DebugChannels_OnNotify(hDlg, lParam);
        break;
    }
    return FALSE;
}

void ProcessPage_OnDebugChannels(void)
{
    DialogBox(hInst, (LPCTSTR)IDD_DEBUG_CHANNELS_DIALOG, hMainWnd, DebugChannelsDlgProc);
}
