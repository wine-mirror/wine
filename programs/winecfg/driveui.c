/*
 * Drive management UI code
 *
 * Copyright 2003 Mark Westcott
 * Copyright 2004 Chris Morgan
 * Copyright 2003-2004 Mike Hearn
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
 */

#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#define COBJMACROS

#include <windows.h>
#include <shellapi.h>
#include <objbase.h>
#include <shlguid.h>
#include <shlwapi.h>
#include <shlobj.h>

#include <wine/debug.h>

#include "winecfg.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

#define BOX_MODE_CD_ASSIGN 1
#define BOX_MODE_CD_AUTODETECT 2
#define BOX_MODE_NONE 3
#define BOX_MODE_NORMAL 4

static BOOL advanced = FALSE;
static BOOL updating_ui = FALSE;
static struct drive* current_drive;

static void get_etched_rect(HWND dialog, RECT *rect);
static void update_controls(HWND dialog);

static DWORD driveui_msgbox (HWND parent, UINT messageId, DWORD flags)
{
  WCHAR* caption = load_string (IDS_WINECFG_TITLE);
  WCHAR* text = load_string (messageId);
  DWORD result = MessageBoxW (parent, text, caption, flags);
  HeapFree (GetProcessHeap(), 0, caption);
  HeapFree (GetProcessHeap(), 0, text);
  return result;
}

/**** listview helper functions ****/

/* clears the item at index in the listview */
static void lv_clear_curr_select(HWND dialog, int index)
{
    ListView_SetItemState(GetDlgItem(dialog, IDC_LIST_DRIVES), index, 0, LVIS_SELECTED);
}

/* selects the item at index in the listview */
static void lv_set_curr_select(HWND dialog, int index)
{
    /* no more than one item can be selected in our listview */
    lv_clear_curr_select(dialog, -1);
    ListView_SetItemState(GetDlgItem(dialog, IDC_LIST_DRIVES), index, LVIS_SELECTED, LVIS_SELECTED);
}

/* returns the currently selected item in the listview */
static int lv_get_curr_select(HWND dialog)
{
    return SendDlgItemMessage(dialog, IDC_LIST_DRIVES, LVM_GETNEXTITEM, -1, LVNI_SELECTED);
}

/* sets the item in the listview at item->iIndex */
static void lv_set_item(HWND dialog, LVITEM *item)
{
    SendDlgItemMessage(dialog, IDC_LIST_DRIVES, LVM_SETITEM, 0, (LPARAM) item);
}

/* sets specified item's text */
static void lv_set_item_text(HWND dialog, int item, int subItem, char *text)
{
    LVITEM lvItem;
    if (item < 0 || subItem < 0) return;
    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = item;
    lvItem.iSubItem = subItem;
    lvItem.pszText = text;
    lvItem.cchTextMax = lstrlen(lvItem.pszText);
    lv_set_item(dialog, &lvItem);
}

/* inserts an item into the listview */
static void lv_insert_item(HWND dialog, LVITEM *item)
{
    SendDlgItemMessage(dialog, IDC_LIST_DRIVES, LVM_INSERTITEM, 0, (LPARAM) item);
}

/* retrieve the item at index item->iIndex */
static void lv_get_item(HWND dialog, LVITEM *item)
{
    SendDlgItemMessage(dialog, IDC_LIST_DRIVES, LVM_GETITEM, 0, (LPARAM) item);
}

static void set_advanced(HWND dialog)
{
    int state;
    char text[256];
    RECT rect;

    if (advanced)
    {
        state = SW_NORMAL;
        LoadString(GetModuleHandle(NULL), IDS_HIDE_ADVANCED, text, 256);
    }
    else
    {
        state = SW_HIDE;
        LoadString(GetModuleHandle(NULL), IDS_SHOW_ADVANCED, text, 256);
    }

    ShowWindow(GetDlgItem(dialog, IDC_RADIO_AUTODETECT), state);
    ShowWindow(GetDlgItem(dialog, IDC_RADIO_ASSIGN), state);
    ShowWindow(GetDlgItem(dialog, IDC_EDIT_LABEL), state);
    ShowWindow(GetDlgItem(dialog, IDC_EDIT_DEVICE), state);
    ShowWindow(GetDlgItem(dialog, IDC_STATIC_LABEL), state);
    ShowWindow(GetDlgItem(dialog, IDC_BUTTON_BROWSE_DEVICE), state);
    ShowWindow(GetDlgItem(dialog, IDC_EDIT_SERIAL), state);
    ShowWindow(GetDlgItem(dialog, IDC_STATIC_SERIAL), state);
    ShowWindow(GetDlgItem(dialog, IDC_LABELSERIAL_STATIC), state);
    ShowWindow(GetDlgItem(dialog, IDC_COMBO_TYPE), state);
    ShowWindow(GetDlgItem(dialog, IDC_STATIC_TYPE), state);

    /* update the button text based on the state */
    SetWindowText(GetDlgItem(dialog, IDC_BUTTON_SHOW_HIDE_ADVANCED), text);

    /* redraw for the etched line */
    get_etched_rect(dialog, &rect);
    InflateRect(&rect, 5, 5);
    InvalidateRect(dialog, &rect, TRUE);
}

struct drive_typemap {
    unsigned int sCode;
    UINT idDesc;
};

static const struct drive_typemap type_pairs[] = {
  { DRIVE_UNKNOWN,    IDS_DRIVE_UNKNOWN   },
  { DRIVE_FIXED,      IDS_DRIVE_FIXED     },
  { DRIVE_REMOTE,     IDS_DRIVE_REMOTE    },
  { DRIVE_REMOVABLE,  IDS_DRIVE_REMOVABLE },
  { DRIVE_CDROM,      IDS_DRIVE_CDROM     }
};

#define DRIVE_TYPE_DEFAULT 0

static void enable_labelserial_box(HWND dialog, int mode)
{
    WINE_TRACE("mode=%d\n", mode);

    switch (mode)
    {
        case BOX_MODE_CD_ASSIGN:
            enable(IDC_RADIO_ASSIGN);
            disable(IDC_EDIT_DEVICE);
            disable(IDC_BUTTON_BROWSE_DEVICE);
            enable(IDC_EDIT_SERIAL);
            enable(IDC_EDIT_LABEL);
            enable(IDC_STATIC_SERIAL);
            enable(IDC_STATIC_LABEL);
            break;

        case BOX_MODE_CD_AUTODETECT:
            enable(IDC_RADIO_ASSIGN);
            enable(IDC_EDIT_DEVICE);
            enable(IDC_BUTTON_BROWSE_DEVICE);
            disable(IDC_EDIT_SERIAL);
            disable(IDC_EDIT_LABEL);
            disable(IDC_STATIC_SERIAL);
            disable(IDC_STATIC_LABEL);
            break;

        case BOX_MODE_NONE:
            disable(IDC_RADIO_ASSIGN);
            disable(IDC_EDIT_DEVICE);
            disable(IDC_BUTTON_BROWSE_DEVICE);
            disable(IDC_EDIT_SERIAL);
            disable(IDC_EDIT_LABEL);
            disable(IDC_STATIC_SERIAL);
            disable(IDC_STATIC_LABEL);
            break;

        case BOX_MODE_NORMAL:
            enable(IDC_RADIO_ASSIGN);
            disable(IDC_EDIT_DEVICE);
            disable(IDC_BUTTON_BROWSE_DEVICE);
            enable(IDC_EDIT_SERIAL);
            enable(IDC_EDIT_LABEL);
            enable(IDC_STATIC_SERIAL);
            enable(IDC_STATIC_LABEL);
            break;
    }
}

static int fill_drives_list(HWND dialog)
{
    int count = 0;
    BOOL drivec_present = FALSE;
    int i;
    int prevsel = -1;

    WINE_TRACE("\n");

    updating_ui = TRUE;

    prevsel = lv_get_curr_select(dialog); 

    /* Clear the listbox */
    SendDlgItemMessage(dialog, IDC_LIST_DRIVES, LVM_DELETEALLITEMS, 0, 0);

    for(i = 0; i < 26; i++)
    {
        LVITEM item;
        char letter[4];

        /* skip over any unused drives */
        if (!drives[i].in_use)
            continue;

        if (drives[i].letter == 'C')
            drivec_present = TRUE;

        letter[0] = 'A' + i;
        letter[1] = ':';
        letter[2] = 0;

        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = count;
        item.iSubItem = 0;
        item.pszText = letter;
        item.cchTextMax = lstrlen(item.pszText);
        item.lParam = (LPARAM) &drives[i];

        lv_insert_item(dialog, &item);
        lv_set_item_text(dialog, count, 1, drives[i].unixpath);

        count++;
    }

    WINE_TRACE("loaded %d drives\n", count);

    /* show the warning if there is no Drive C */
    if (!drivec_present)
        ShowWindow(GetDlgItem(dialog, IDS_DRIVE_NO_C), SW_NORMAL);
    else
        ShowWindow(GetDlgItem(dialog, IDS_DRIVE_NO_C), SW_HIDE);

    lv_set_curr_select(dialog, prevsel == -1 ? 0 : prevsel);

    updating_ui = FALSE;
    return count;
}

static void on_options_click(HWND dialog)
{
    if (IsDlgButtonChecked(dialog, IDC_SHOW_DOT_FILES) == BST_CHECKED)
        set_reg_key(config_key, "", "ShowDotFiles", "Y");
    else
        set_reg_key(config_key, "", "ShowDotFiles", "N");

    SendMessage(GetParent(dialog), PSM_CHANGED, 0, 0);
}

static void on_add_click(HWND dialog)
{
    /* we should allocate a drive letter automatically. We also need
       some way to let the user choose the mapping point, for now we
       will just force them to enter a path automatically, with / being
       the default. In future we should be able to temporarily map /
       then invoke the directory chooser dialog. */

    char new = 'C'; /* we skip A and B, they are historically floppy drives */
    long mask = ~drive_available_mask(0); /* the mask is now which drives aren't available */
    int i, c;

    while (mask & (1 << (new - 'A')))
    {
        new++;
        if (new > 'Z')
        {
            driveui_msgbox (dialog, IDS_DRIVE_LETTERS_EXCEEDED, MB_OK | MB_ICONEXCLAMATION);
            return;
        }
    }

    WINE_TRACE("allocating drive letter %c\n", new);

    if (new == 'C')
    {
        char label[64];
        LoadStringA (GetModuleHandle (NULL), IDS_SYSTEM_DRIVE_LABEL, label,
            sizeof(label)/sizeof(label[0])); 
        add_drive(new, "../drive_c", label, "", DRIVE_FIXED);
    }
    else add_drive(new, "/", "", "", DRIVE_UNKNOWN);

    fill_drives_list(dialog);

    /* select the newly created drive */
    mask = ~drive_available_mask(0);
    c = 0;
    for (i = 0; i < 26; i++)
    {
        if ('A' + i == new) break;
        if ((1 << i) & mask) c++;
    }
    lv_set_curr_select(dialog, c);

    SetFocus(GetDlgItem(dialog, IDC_LIST_DRIVES));

    update_controls(dialog);
}

static void on_remove_click(HWND dialog)
{
    int itemIndex;
    struct drive *drive;
    LVITEM item;

    itemIndex = lv_get_curr_select(dialog);
    if (itemIndex == -1) return; /* no selection */

    item.mask = LVIF_PARAM;
    item.iItem = itemIndex;
    item.iSubItem = 0;

    lv_get_item(dialog, &item);

    drive = (struct drive *) item.lParam;

    WINE_TRACE("unixpath: %s\n", drive->unixpath);

    if (drive->letter == 'C')
    {
        DWORD result = driveui_msgbox (dialog, IDS_CONFIRM_DELETE_C, MB_YESNO | MB_ICONEXCLAMATION);
        if (result == IDNO) return;
    }

    delete_drive(drive);

    fill_drives_list(dialog);

    itemIndex = itemIndex - 1;
    if (itemIndex < 0) itemIndex = 0;
    lv_set_curr_select(dialog, itemIndex);   /* previous item */

    SetFocus(GetDlgItem(dialog, IDC_LIST_DRIVES));

    update_controls(dialog);
}

static void update_controls(HWND dialog)
{
    char *path;
    unsigned int type;
    char *label;
    char *serial;
    const char *device;
    int i, selection = -1;
    LVITEM item;

    updating_ui = TRUE;

    i = lv_get_curr_select(dialog);
    if (i == -1)
    {
        /* no selection? let's select something for the user. this will re-enter */
        lv_set_curr_select(dialog, i);
        return;
    }

    item.mask = LVIF_PARAM;
    item.iItem = i;
    item.iSubItem = 0;

    lv_get_item(dialog, &item);
    current_drive = (struct drive *) item.lParam;

    WINE_TRACE("Updating sheet for drive %c\n", current_drive->letter);

    /* path */
    path = current_drive->unixpath;
    WINE_TRACE("set path control text to '%s'\n", path);
    set_text(dialog, IDC_EDIT_PATH, path);

    /* drive type */
    type = current_drive->type;
    SendDlgItemMessage(dialog, IDC_COMBO_TYPE, CB_RESETCONTENT, 0, 0);

    for (i = 0; i < sizeof(type_pairs) / sizeof(struct drive_typemap); i++)
    {
        WCHAR driveDesc[64];
        LoadStringW (GetModuleHandle (NULL), type_pairs[i].idDesc, driveDesc,
            sizeof(driveDesc)/sizeof(driveDesc[0]));
        SendDlgItemMessageW (dialog, IDC_COMBO_TYPE, CB_ADDSTRING, 0, (LPARAM)driveDesc);

        if (type_pairs[i].sCode ==  type)
        {
            selection = i;
        }
    }

    if (selection == -1) selection = DRIVE_TYPE_DEFAULT;
    SendDlgItemMessage(dialog, IDC_COMBO_TYPE, CB_SETCURSEL, selection, 0);

    /* removeable media properties */
    label = current_drive->label;
    set_text(dialog, IDC_EDIT_LABEL, label);

    /* set serial edit text */
    serial = current_drive->serial;
    set_text(dialog, IDC_EDIT_SERIAL, serial);

    /* TODO: get the device here to put into the edit box */
    device = "Not implemented yet";
    set_text(dialog, IDC_EDIT_DEVICE, device);
    device = NULL;

    selection = IDC_RADIO_ASSIGN;
    if ((type == DRIVE_CDROM) || (type == DRIVE_REMOVABLE))
    {
        if (device)
        {
            selection = IDC_RADIO_AUTODETECT;
            enable_labelserial_box(dialog, BOX_MODE_CD_AUTODETECT);
        }
        else
        {
            selection = IDC_RADIO_ASSIGN;
            enable_labelserial_box(dialog, BOX_MODE_CD_ASSIGN);
        }
    }
    else
    {
        enable_labelserial_box(dialog, BOX_MODE_NORMAL);
        selection = IDC_RADIO_ASSIGN;
    }

    CheckRadioButton(dialog, IDC_RADIO_AUTODETECT, IDC_RADIO_ASSIGN, selection);

    updating_ui = FALSE;

    return;
}

static void on_edit_changed(HWND dialog, WORD id)
{
    if (updating_ui) return;

    WINE_TRACE("edit id %d changed\n", id);

    switch (id)
    {
        case IDC_EDIT_LABEL:
        {
            char *label;

            label = get_text(dialog, id);
            HeapFree(GetProcessHeap(), 0, current_drive->label);
            current_drive->label = label ? label :  strdupA("");

            WINE_TRACE("set label to %s\n", current_drive->label);

            /* enable the apply button  */
            SendMessage(GetParent(dialog), PSM_CHANGED, (WPARAM) dialog, 0);
            break;
        }

        case IDC_EDIT_PATH:
        {
            char *path;

            path = get_text(dialog, id);
            HeapFree(GetProcessHeap(), 0, current_drive->unixpath);
            current_drive->unixpath = path ? path : strdupA("drive_c");

            WINE_TRACE("set path to %s\n", current_drive->unixpath);

            lv_set_item_text(dialog, lv_get_curr_select(dialog), 1,
                             current_drive->unixpath);

            /* enable the apply button  */
            SendMessage(GetParent(dialog), PSM_CHANGED, (WPARAM) dialog, 0);
            break;
        }

        case IDC_EDIT_SERIAL:
        {
            char *serial;

            serial = get_text(dialog, id);
            HeapFree(GetProcessHeap(), 0, current_drive->serial);
            current_drive->serial = serial ? serial : strdupA("");

            WINE_TRACE("set serial to %s\n", current_drive->serial);

            /* enable the apply button  */
            SendMessage(GetParent(dialog), PSM_CHANGED, (WPARAM) dialog, 0);
            break;
        }

        case IDC_EDIT_DEVICE:
        {
            char *device = get_text(dialog, id);
            /* TODO: handle device if/when it makes sense to do so.... */
            HeapFree(GetProcessHeap(), 0, device);
            break;
        }
    }
}

static void get_etched_rect(HWND dialog, RECT *rect)
{
    GetClientRect(dialog, rect);

    /* these dimensions from the labelserial static in En.rc  */
    rect->top = 265;
    rect->bottom = 265;
    rect->left += 25;
    rect->right -= 25;
}

/* this just draws a nice line to separate the advanced gui from the n00b gui :) */
static void paint(HWND dialog)
{
    PAINTSTRUCT ps;

    BeginPaint(dialog, &ps);

    if (advanced)
    {
        RECT rect;

        get_etched_rect(dialog, &rect);

        DrawEdge(ps.hdc, &rect, EDGE_ETCHED, BF_TOP);
    }

    EndPaint(dialog, &ps);
}

BOOL browse_for_unix_folder(HWND dialog, char *pszPath)
{
    static WCHAR wszUnixRootDisplayName[] = 
        { ':',':','{','C','C','7','0','2','E','B','2','-','7','D','C','5','-','1','1','D','9','-',
          'C','6','8','7','-','0','0','0','4','2','3','8','A','0','1','C','D','}', 0 };
    char pszChoosePath[256];
    BROWSEINFOA bi = {
        dialog,
        NULL,
        NULL,
        pszChoosePath,
        0,
        NULL,
        0,
        0
    };
    IShellFolder *pDesktop;
    LPITEMIDLIST pidlUnixRoot, pidlSelectedPath;
    HRESULT hr;
   
    LoadString(GetModuleHandle(NULL), IDS_CHOOSE_PATH, pszChoosePath, 256);
    
    hr = SHGetDesktopFolder(&pDesktop);
    if (!SUCCEEDED(hr)) return FALSE;

    hr = IShellFolder_ParseDisplayName(pDesktop, NULL, NULL, wszUnixRootDisplayName, NULL, 
                                       &pidlUnixRoot, NULL);
    if (!SUCCEEDED(hr)) {
        IShellFolder_Release(pDesktop);
        return FALSE;
    }

    bi.pidlRoot = pidlUnixRoot;
    pidlSelectedPath = SHBrowseForFolderA(&bi);
    SHFree(pidlUnixRoot);
    
    if (pidlSelectedPath) {
        STRRET strSelectedPath;
        char *pszSelectedPath;
        HRESULT hr;
        
        hr = IShellFolder_GetDisplayNameOf(pDesktop, pidlSelectedPath, SHGDN_FORPARSING, 
                                           &strSelectedPath);
        IShellFolder_Release(pDesktop);
        if (!SUCCEEDED(hr)) {
            SHFree(pidlSelectedPath);
            return FALSE;
        }

        hr = StrRetToStr(&strSelectedPath, pidlSelectedPath, &pszSelectedPath);
        SHFree(pidlSelectedPath);
        if (!SUCCEEDED(hr)) return FALSE;

        lstrcpy(pszPath, pszSelectedPath);
        
        CoTaskMemFree(pszSelectedPath);
        return TRUE;
    }
    return FALSE;
}

static void init_listview_columns(HWND dialog)
{
    LVCOLUMNW listColumn;
    RECT viewRect;
    int width;
    WCHAR column[64];

    GetClientRect(GetDlgItem(dialog, IDC_LIST_DRIVES), &viewRect);
    width = (viewRect.right - viewRect.left) / 6 - 5;

    LoadStringW (GetModuleHandle (NULL), IDS_COL_DRIVELETTER, column,
        sizeof(column)/sizeof(column[0]));
    listColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    listColumn.pszText = column;
    listColumn.cchTextMax = lstrlenW (listColumn.pszText);
    listColumn.cx = width;

    SendDlgItemMessageW (dialog, IDC_LIST_DRIVES, LVM_INSERTCOLUMNW, 0, (LPARAM) &listColumn);

    LoadStringW (GetModuleHandle (NULL), IDS_COL_DRIVEMAPPING, column,
        sizeof(column)/sizeof(column[0]));
    listColumn.cx = viewRect.right - viewRect.left - width;
    listColumn.pszText = column;
    listColumn.cchTextMax = lstrlenW (listColumn.pszText);

    SendDlgItemMessageW (dialog, IDC_LIST_DRIVES, LVM_INSERTCOLUMNW, 1, (LPARAM) &listColumn);
}

static void load_drive_options(HWND dialog)
{
    if (!strcmp(get_reg_key(config_key, "", "ShowDotFiles", "N"), "Y"))
        CheckDlgButton(dialog, IDC_SHOW_DOT_FILES, BST_CHECKED);
}

INT_PTR CALLBACK
DriveDlgProc (HWND dialog, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int item;
    struct drive *drive;

    switch (msg)
    {
        case WM_INITDIALOG:
            init_listview_columns(dialog);
            load_drives();
            load_drive_options(dialog);

            if (!drives[2].in_use)
                driveui_msgbox (dialog, IDS_NO_DRIVE_C, MB_OK | MB_ICONEXCLAMATION);

            fill_drives_list(dialog);
            update_controls(dialog);
            /* put in non-advanced mode by default  */
            set_advanced(dialog);
            break;

        case WM_SHOWWINDOW:
            set_window_title(dialog);
            break;

        case WM_PAINT:
            paint(dialog);
            break;

        case WM_COMMAND:
            switch (HIWORD(wParam))
            {
                case EN_CHANGE:
                    on_edit_changed(dialog, LOWORD(wParam));
                    break;

                case BN_CLICKED:
                    switch (LOWORD(wParam))
                    {
                        case IDC_SHOW_DOT_FILES:
                            on_options_click(dialog);
                        break;
                    }
                    break;

                case CBN_SELCHANGE:
                    SendMessage(GetParent(dialog), PSM_CHANGED, 0, 0);
                    break;
            }

            switch (LOWORD(wParam))
            {
                case IDC_BUTTON_ADD:
                    if (HIWORD(wParam) != BN_CLICKED) break;
                    on_add_click(dialog);
                    break;

                case IDC_BUTTON_REMOVE:
                    if (HIWORD(wParam) != BN_CLICKED) break;
                    on_remove_click(dialog);
                    break;

                case IDC_BUTTON_EDIT:
                    if (HIWORD(wParam) != BN_CLICKED) break;
                    item = SendMessage(GetDlgItem(dialog, IDC_LIST_DRIVES),  LB_GETCURSEL, 0, 0);
                    drive = (struct drive *) SendMessage(GetDlgItem(dialog, IDC_LIST_DRIVES), LB_GETITEMDATA, item, 0);
                    break;

                case IDC_BUTTON_AUTODETECT:
                    autodetect_drives();
                    fill_drives_list(dialog);
                    SendMessage(GetParent(dialog), PSM_CHANGED, 0, 0);
                    break;

                case IDC_BUTTON_SHOW_HIDE_ADVANCED:
                    advanced = !advanced;
                    set_advanced(dialog);
                    break;

                case IDC_BUTTON_BROWSE_PATH:
                {
                    char szTargetPath[FILENAME_MAX];
                    if (browse_for_unix_folder(dialog, szTargetPath)) 
                        set_text(dialog, IDC_EDIT_PATH, szTargetPath);
                    break;
                }

                case IDC_RADIO_ASSIGN:
                {
                    char *str;

                    str = get_text(dialog, IDC_EDIT_LABEL);
                    HeapFree(GetProcessHeap(), 0, current_drive->label);
                    current_drive->label = str ? str : strdupA("");

                    str = get_text(dialog, IDC_EDIT_SERIAL);
                    HeapFree(GetProcessHeap(), 0, current_drive->serial);
                    current_drive->serial = str ? str : strdupA("");

                    /* TODO: we don't have a device at this point */

                    enable_labelserial_box(dialog, BOX_MODE_CD_ASSIGN);

                    break;
                }


                case IDC_COMBO_TYPE:
                {
                    int mode = BOX_MODE_NORMAL;
                    int selection;

                    if (HIWORD(wParam) != CBN_SELCHANGE) break;

                    selection = SendDlgItemMessage(dialog, IDC_COMBO_TYPE, CB_GETCURSEL, 0, 0);

                    if (selection >= 0 &&
                        (type_pairs[selection].sCode == DRIVE_CDROM ||
                         type_pairs[selection].sCode == DRIVE_REMOVABLE))
                    {
                        if (IsDlgButtonChecked(dialog, IDC_RADIO_AUTODETECT))
                            mode = BOX_MODE_CD_AUTODETECT;
                        else
                            mode = BOX_MODE_CD_ASSIGN;
                    }

                    enable_labelserial_box(dialog, mode);

                    current_drive->type = type_pairs[selection].sCode;
                    break;
                }

            }
            break;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case PSN_KILLACTIVE:
                    WINE_TRACE("PSN_KILLACTIVE\n");
                    SetWindowLongPtr(dialog, DWLP_MSGRESULT, FALSE);
                    break;
                case PSN_APPLY:
                    apply_drive_changes();
                    SetWindowLongPtr(dialog, DWLP_MSGRESULT, PSNRET_NOERROR);
                    break;
                case PSN_SETACTIVE:
                    break;
                case LVN_ITEMCHANGED:
                {
                    LPNMLISTVIEW lpnm = (LPNMLISTVIEW)lParam;
                    if (!(lpnm->uOldState & LVIS_SELECTED) &&
                         (lpnm->uNewState & LVIS_SELECTED))
                    update_controls(dialog);
                    break;
                }
            }
            break;
    }

    return FALSE;
}
