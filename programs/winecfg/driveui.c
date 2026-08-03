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

#define BOX_MODE_DEVICE 1
#define BOX_MODE_NORMAL 2

static BOOL advanced = FALSE;
static BOOL updating_ui = FALSE;
static struct drive* current_drive;

static void update_controls(HWND dialog);

static DWORD driveui_msgbox (HWND parent, UINT messageId, DWORD flags)
{
  WCHAR* caption = load_string (IDS_WINECFG_TITLE);
  WCHAR* text = load_string (messageId);
  DWORD result = MessageBoxW (parent, text, caption, flags);
  free (caption);
  free (text);
  return result;
}

/**** listview helper functions ****/

/* clears the item at index in the listview */
static void lv_clear_curr_select(HWND dialog, int index)
{
    LVITEMW item;

    item.mask = LVIF_STATE;
    item.state = 0;
    item.stateMask = LVIS_SELECTED;
    SendDlgItemMessageW( dialog, IDC_LIST_DRIVES, LVM_SETITEMSTATE, index, (LPARAM)&item );
}

/* selects the item at index in the listview */
static void lv_set_curr_select(HWND dialog, int index)
{
    LVITEMW item;

    /* no more than one item can be selected in our listview */
    lv_clear_curr_select(dialog, -1);
    item.mask = LVIF_STATE;
    item.state = LVIS_SELECTED;
    item.stateMask = LVIS_SELECTED;
    SendDlgItemMessageW( dialog, IDC_LIST_DRIVES, LVM_SETITEMSTATE, index, (LPARAM)&item );
}

/* returns the currently selected item in the listview */
static int lv_get_curr_select(HWND dialog)
{
    return SendDlgItemMessageW(dialog, IDC_LIST_DRIVES, LVM_GETNEXTITEM, -1, LVNI_SELECTED);
}

/* sets the item in the listview at item->iIndex */
static void lv_set_item(HWND dialog, LVITEMW *item)
{
    SendDlgItemMessageW(dialog, IDC_LIST_DRIVES, LVM_SETITEMW, 0, (LPARAM) item);
}

/* sets specified item's text */
static void lv_set_item_text(HWND dialog, int item, int subItem, WCHAR *text)
{
    LVITEMW lvItem;
    if (item < 0 || subItem < 0) return;
    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = item;
    lvItem.iSubItem = subItem;
    lvItem.pszText = text;
    lvItem.cchTextMax = lstrlenW(lvItem.pszText);
    lv_set_item(dialog, &lvItem);
}

/* inserts an item into the listview */
static void lv_insert_item(HWND dialog, LVITEMW *item)
{
    SendDlgItemMessageW(dialog, IDC_LIST_DRIVES, LVM_INSERTITEMW, 0, (LPARAM) item);
}

/* retrieve the item at index item->iIndex */
static void lv_get_item(HWND dialog, LVITEMW *item)
{
    SendDlgItemMessageW(dialog, IDC_LIST_DRIVES, LVM_GETITEMW, 0, (LPARAM) item);
}

static void set_advanced(HWND dialog)
{
    int state;
    WCHAR text[256];

    if (advanced)
    {
        state = SW_NORMAL;
        LoadStringW(GetModuleHandleW(NULL), IDS_HIDE_ADVANCED, text, 256);
    }
    else
    {
        state = SW_HIDE;
        LoadStringW(GetModuleHandleW(NULL), IDS_SHOW_ADVANCED, text, 256);
    }

    ShowWindow(GetDlgItem(dialog, IDC_EDIT_DEVICE), state);
    ShowWindow(GetDlgItem(dialog, IDC_STATIC_DEVICE), state);
    ShowWindow(GetDlgItem(dialog, IDC_EDIT_LABEL), state);
    ShowWindow(GetDlgItem(dialog, IDC_STATIC_LABEL), state);
    ShowWindow(GetDlgItem(dialog, IDC_BUTTON_BROWSE_DEVICE), state);
    ShowWindow(GetDlgItem(dialog, IDC_EDIT_SERIAL), state);
    ShowWindow(GetDlgItem(dialog, IDC_STATIC_SERIAL), state);
    ShowWindow(GetDlgItem(dialog, IDC_COMBO_TYPE), state);
    ShowWindow(GetDlgItem(dialog, IDC_STATIC_TYPE), state);

    /* update the button text based on the state */
    SetWindowTextW(GetDlgItem(dialog, IDC_BUTTON_SHOW_HIDE_ADVANCED), text);
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
        case BOX_MODE_DEVICE:
            /* FIXME: enable device editing */
            disable(IDC_EDIT_DEVICE);
            disable(IDC_BUTTON_BROWSE_DEVICE);
            disable(IDC_EDIT_SERIAL);
            disable(IDC_EDIT_LABEL);
            break;

        case BOX_MODE_NORMAL:
            disable(IDC_EDIT_DEVICE);
            disable(IDC_BUTTON_BROWSE_DEVICE);
            enable(IDC_EDIT_SERIAL);
            enable(IDC_EDIT_LABEL);
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
    SendDlgItemMessageW(dialog, IDC_LIST_DRIVES, LVM_DELETEALLITEMS, 0, 0);

    for(i = 0; i < 26; i++)
    {
        LVITEMW item;
        WCHAR *path;
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
        item.pszText = strdupU2W(letter);
        item.cchTextMax = lstrlenW(item.pszText);
        item.lParam = (LPARAM) &drives[i];

        lv_insert_item(dialog, &item);
        free(item.pszText);

        path = strdupU2W(drives[i].unixpath);
        lv_set_item_text(dialog, count, 1, path);
        free(path);

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
        set_reg_key(config_key, L"", L"ShowDotFiles", L"Y");
    else
        set_reg_key(config_key, L"", L"ShowDotFiles", L"N");

    SendMessageW(GetParent(dialog), PSM_CHANGED, 0, 0);
}

static INT_PTR CALLBACK drivechoose_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static int i, sel;
    WCHAR c;
    WCHAR drive[] = L"X:";

    switch(uMsg)
    {
    case WM_INITDIALOG:
        {
        ULONG mask = ~drive_available_mask(0); /* the mask is now which drives aren't available */
        for( c = 'A'; c<= 'Z'; c++){
            drive[0] = c;
            if(!( mask & (1 << (c - 'A'))))
                SendDlgItemMessageW( hwndDlg, IDC_DRIVESA2Z, CB_ADDSTRING, 0, (LPARAM) drive);
        }
        drive[0] = lParam;
        SendDlgItemMessageW( hwndDlg, IDC_DRIVESA2Z, CB_SELECTSTRING, 0, (LPARAM) drive);
        return TRUE;
        }
    case WM_COMMAND:
        if(HIWORD(wParam) != BN_CLICKED) break;
        switch (LOWORD(wParam))
        {
        case IDOK:
            i = SendDlgItemMessageW( hwndDlg, IDC_DRIVESA2Z, CB_GETCURSEL, 0, 0);
            if( i != CB_ERR){
                SendDlgItemMessageW( hwndDlg, IDC_DRIVESA2Z, CB_GETLBTEXT, i, (LPARAM) drive);
                sel = drive[0];
            } else
                sel = -1;
            EndDialog(hwndDlg, sel);
            return TRUE;
        case IDCANCEL:
            EndDialog(hwndDlg, -1);
            return TRUE;
        }
    }
    return FALSE;
}

static void on_add_click(HWND dialog)
{
    /* we should allocate a drive letter automatically. We also need
       some way to let the user choose the mapping point, for now we
       will just force them to enter a path automatically, with / being
       the default. In future we should be able to temporarily map /
       then invoke the directory chooser dialog. */

    char new = 'C'; /* we skip A and B, they are historically floppy drives */
    ULONG mask = ~drive_available_mask(0); /* the mask is now which drives aren't available */
    int i, c;
    INT_PTR ret;

    while (mask & (1 << (new - 'A')))
    {
        new++;
        if (new > 'Z')
        {
            driveui_msgbox (dialog, IDS_DRIVE_LETTERS_EXCEEDED, MB_OK | MB_ICONEXCLAMATION);
            return;
        }
    }


    ret = DialogBoxParamW(0, MAKEINTRESOURCEW(IDD_DRIVECHOOSE), dialog, drivechoose_dlgproc, new);

    if( ret == -1) return;
    new = ret;

    WINE_TRACE("selected drive letter %c\n", new);

    if (new == 'C')
    {
        WCHAR label[64];
        LoadStringW(GetModuleHandleW(NULL), IDS_SYSTEM_DRIVE_LABEL, label, ARRAY_SIZE(label));
        add_drive(new, "../drive_c", NULL, label, 0, DRIVE_FIXED);
    }
    else add_drive(new, "/", NULL, NULL, 0, DRIVE_UNKNOWN);

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
    SendMessageW(GetParent(dialog), PSM_CHANGED, (WPARAM) dialog, 0);
}

static void on_remove_click(HWND dialog)
{
    int itemIndex;
    struct drive *drive;
    LVITEMW item;

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
    SendMessageW(GetParent(dialog), PSM_CHANGED, (WPARAM) dialog, 0);
}

static void update_controls(HWND dialog)
{
    static const WCHAR emptyW[1];
    WCHAR *path;
    unsigned int type;
    char serial[16];
    int i, selection = -1;
    LVITEMW item;

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
    WINE_TRACE("set path control text to '%s'\n", current_drive->unixpath);
    path = strdupU2W(current_drive->unixpath);
    set_textW(dialog, IDC_EDIT_PATH, path);
    free(path);

    /* drive type */
    type = current_drive->type;
    SendDlgItemMessageW(dialog, IDC_COMBO_TYPE, CB_RESETCONTENT, 0, 0);

    for (i = 0; i < ARRAY_SIZE(type_pairs); i++)
    {
        WCHAR driveDesc[64];
        LoadStringW(GetModuleHandleW(NULL), type_pairs[i].idDesc, driveDesc, ARRAY_SIZE(driveDesc));
        SendDlgItemMessageW (dialog, IDC_COMBO_TYPE, CB_ADDSTRING, 0, (LPARAM)driveDesc);

        if (type_pairs[i].sCode ==  type)
        {
            selection = i;
        }
    }

    if (selection == -1) selection = DRIVE_TYPE_DEFAULT;
    SendDlgItemMessageW(dialog, IDC_COMBO_TYPE, CB_SETCURSEL, selection, 0);

    EnableWindow( GetDlgItem( dialog, IDC_BUTTON_REMOVE ), (current_drive->letter != 'C') );
    EnableWindow( GetDlgItem( dialog, IDC_EDIT_PATH ), (current_drive->letter != 'C') );
    EnableWindow( GetDlgItem( dialog, IDC_BUTTON_BROWSE_PATH ), (current_drive->letter != 'C') );
    EnableWindow( GetDlgItem( dialog, IDC_COMBO_TYPE ), (current_drive->letter != 'C') );

    /* removable media properties */
    set_textW(dialog, IDC_EDIT_LABEL, current_drive->label ? current_drive->label : emptyW);

    /* set serial edit text */
    sprintf( serial, "%lX", current_drive->serial );
    set_text(dialog, IDC_EDIT_SERIAL, serial);

    set_text(dialog, IDC_EDIT_DEVICE, current_drive->device);

    if ((type == DRIVE_CDROM) || (type == DRIVE_REMOVABLE))
        enable_labelserial_box(dialog, BOX_MODE_DEVICE);
    else
        enable_labelserial_box(dialog, BOX_MODE_NORMAL);

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
            WCHAR *label = get_text(dialog, id);
            free(current_drive->label);
            current_drive->label = label;
            current_drive->modified = TRUE;

            WINE_TRACE("set label to %s\n", wine_dbgstr_w(current_drive->label));

            /* enable the apply button  */
            SendMessageW(GetParent(dialog), PSM_CHANGED, (WPARAM) dialog, 0);
            break;
        }

        case IDC_EDIT_PATH:
        {
            WCHAR *wpath;
            char *path;
            int lenW;

            wpath = get_text(dialog, id);
            if( (lenW = WideCharToMultiByte(CP_UNIXCP, 0, wpath, -1, NULL, 0, NULL, NULL)) )
            {
                path = malloc(lenW);
                WideCharToMultiByte(CP_UNIXCP, 0, wpath, -1, path, lenW, NULL, NULL);
            }
            else
            {
                path = NULL;
                wpath = strdupU2W("drive_c");
            }

            free(current_drive->unixpath);
            current_drive->unixpath = path ? path : strdup("drive_c");
            current_drive->modified = TRUE;

            WINE_TRACE("set path to %s\n", current_drive->unixpath);

            lv_set_item_text(dialog, lv_get_curr_select(dialog), 1,
                             wpath);
            free(wpath);

            /* enable the apply button  */
            SendMessageW(GetParent(dialog), PSM_CHANGED, (WPARAM) dialog, 0);
            break;
        }

        case IDC_EDIT_SERIAL:
        {
            WCHAR *serial;

            serial = get_text(dialog, id);
            current_drive->serial = serial ? wcstoul( serial, NULL, 16 ) : 0;
            free(serial);
            current_drive->modified = TRUE;

            WINE_TRACE("set serial to %08lX\n", current_drive->serial);

            /* enable the apply button  */
            SendMessageW(GetParent(dialog), PSM_CHANGED, (WPARAM) dialog, 0);
            break;
        }

        case IDC_EDIT_DEVICE:
        {
            WCHAR *device = get_text(dialog, id);
            /* TODO: handle device if/when it makes sense to do so.... */
            free(device);
            break;
        }
    }
}

BOOL browse_for_unix_folder(HWND dialog, WCHAR *pszPath)
{
    static WCHAR wszUnixRootDisplayName[] = L"::{CC702EB2-7DC5-11D9-C687-0004238A01CD}";
    WCHAR pszChoosePath[FILENAME_MAX];
    BROWSEINFOW bi = {
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

    LoadStringW(GetModuleHandleW(NULL), IDS_CHOOSE_PATH, pszChoosePath, FILENAME_MAX);

    hr = SHGetDesktopFolder(&pDesktop);
    if (FAILED(hr)) return FALSE;

    hr = IShellFolder_ParseDisplayName(pDesktop, NULL, NULL, wszUnixRootDisplayName, NULL, 
                                       &pidlUnixRoot, NULL);
    if (FAILED(hr)) {
        IShellFolder_Release(pDesktop);
        return FALSE;
    }

    bi.pidlRoot = pidlUnixRoot;
    pidlSelectedPath = SHBrowseForFolderW(&bi);
    SHFree(pidlUnixRoot);
    
    if (pidlSelectedPath) {
        STRRET strSelectedPath;
        WCHAR *pszSelectedPath;
        HRESULT hr;
        
        hr = IShellFolder_GetDisplayNameOf(pDesktop, pidlSelectedPath, SHGDN_FORPARSING, 
                                           &strSelectedPath);
        IShellFolder_Release(pDesktop);
        if (FAILED(hr)) {
            SHFree(pidlSelectedPath);
            return FALSE;
        }

        hr = StrRetToStrW(&strSelectedPath, pidlSelectedPath, &pszSelectedPath);
        SHFree(pidlSelectedPath);
        if (FAILED(hr)) return FALSE;

        lstrcpyW(pszPath, pszSelectedPath);
        
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

    LoadStringW(GetModuleHandleW(NULL), IDS_COL_DRIVELETTER, column, ARRAY_SIZE(column));
    listColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    listColumn.pszText = column;
    listColumn.cchTextMax = lstrlenW (listColumn.pszText);
    listColumn.cx = width;

    SendDlgItemMessageW (dialog, IDC_LIST_DRIVES, LVM_INSERTCOLUMNW, 0, (LPARAM) &listColumn);

    LoadStringW(GetModuleHandleW(NULL), IDS_COL_DRIVEMAPPING, column, ARRAY_SIZE(column));
    listColumn.cx = viewRect.right - viewRect.left - width;
    listColumn.pszText = column;
    listColumn.cchTextMax = lstrlenW (listColumn.pszText);

    SendDlgItemMessageW (dialog, IDC_LIST_DRIVES, LVM_INSERTCOLUMNW, 1, (LPARAM) &listColumn);
}

static void load_drive_options(HWND dialog)
{
    if (!wcscmp(get_reg_key(config_key, L"", L"ShowDotFiles", L"N"), L"Y"))
        CheckDlgButton(dialog, IDC_SHOW_DOT_FILES, BST_CHECKED);
}

INT_PTR CALLBACK
DriveDlgProc (HWND dialog, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int item;

    switch (msg)
    {
        case WM_INITDIALOG:
            init_listview_columns(dialog);
            if (!load_drives())
            {
                ShowWindow( GetDlgItem( dialog, IDC_STATIC_MOUNTMGR_ERROR ), SW_SHOW );
                ShowWindow( GetDlgItem( dialog, IDC_LIST_DRIVES ), SW_HIDE );
                ShowWindow( GetDlgItem( dialog, IDC_BUTTON_ADD ), SW_HIDE );
                ShowWindow( GetDlgItem( dialog, IDC_BUTTON_REMOVE ), SW_HIDE );
                ShowWindow( GetDlgItem( dialog, IDC_STATIC_PATH ), SW_HIDE );
                ShowWindow( GetDlgItem( dialog, IDC_EDIT_PATH ), SW_HIDE );
                ShowWindow( GetDlgItem( dialog, IDC_BUTTON_BROWSE_PATH ), SW_HIDE );
                ShowWindow( GetDlgItem( dialog, IDC_COMBO_TYPE ), SW_HIDE );
                ShowWindow( GetDlgItem( dialog, IDC_BUTTON_SHOW_HIDE_ADVANCED ), SW_HIDE );
                set_advanced(dialog);
                break;
            }
            ShowWindow( GetDlgItem( dialog, IDC_STATIC_MOUNTMGR_ERROR ), SW_HIDE );
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
                    SendMessageW(GetParent(dialog), PSM_CHANGED, 0, 0);
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
                    item = SendMessageW(GetDlgItem(dialog, IDC_LIST_DRIVES),  LB_GETCURSEL, 0, 0);
                    SendMessageW(GetDlgItem(dialog, IDC_LIST_DRIVES), LB_GETITEMDATA, item, 0);
                    break;

                case IDC_BUTTON_SHOW_HIDE_ADVANCED:
                    advanced = !advanced;
                    set_advanced(dialog);
                    break;

                case IDC_BUTTON_BROWSE_PATH:
                {
                    WCHAR szTargetPath[FILENAME_MAX];
                    if (browse_for_unix_folder(dialog, szTargetPath)) 
                        set_textW(dialog, IDC_EDIT_PATH, szTargetPath);
                    break;
                }

                case IDC_COMBO_TYPE:
                {
                    int mode = BOX_MODE_NORMAL;
                    int selection;

                    if (HIWORD(wParam) != CBN_SELCHANGE) break;

                    selection = SendDlgItemMessageW(dialog, IDC_COMBO_TYPE, CB_GETCURSEL, 0, 0);

                    if (selection >= 0 &&
                        (type_pairs[selection].sCode == DRIVE_CDROM ||
                         type_pairs[selection].sCode == DRIVE_REMOVABLE))
                        mode = BOX_MODE_DEVICE;
                    else
                        mode = BOX_MODE_NORMAL;

                    enable_labelserial_box(dialog, mode);

                    current_drive->type = type_pairs[selection].sCode;
                    current_drive->modified = TRUE;
                    break;
                }

            }
            break;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case PSN_KILLACTIVE:
                    WINE_TRACE("PSN_KILLACTIVE\n");
                    SetWindowLongPtrW(dialog, DWLP_MSGRESULT, FALSE);
                    break;
                case PSN_APPLY:
                    apply_drive_changes();
                    SetWindowLongPtrW(dialog, DWLP_MSGRESULT, PSNRET_NOERROR);
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
