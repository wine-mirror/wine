/*
 * Add/Remove Programs applet
 * Partially based on Wine Uninstaller
 *
 * Copyright 2000 Andreas Mohr
 * Copyright 2004 Hannu Valtonen
 * Copyright 2005 Jonathan Ernst
 * Copyright 2001-2002, 2008 Owen Rudge
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

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <winreg.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <cpl.h>

#include "wine/list.h"
#include "wine/debug.h"
#include "appwiz.h"
#include "res.h"

WINE_DEFAULT_DEBUG_CHANNEL(appwizcpl);

/* define a maximum length for various buffers we use */
#define MAX_STRING_LEN    1024

typedef struct APPINFO
{
    struct list entry;
    int id;

    LPWSTR title;
    LPWSTR path;
    LPWSTR path_modify;

    LPWSTR icon;
    int iconIdx;

    LPWSTR publisher;
    LPWSTR version;
    LPWSTR contact;
    LPWSTR helplink;
    LPWSTR helptelephone;
    LPWSTR readme;
    LPWSTR urlupdateinfo;
    LPWSTR comments;

    HKEY regroot;
    WCHAR regkey[MAX_STRING_LEN];
} APPINFO;

static struct list app_list = LIST_INIT( app_list );
HINSTANCE hInst;

static WCHAR btnRemove[MAX_STRING_LEN];
static WCHAR btnModifyRemove[MAX_STRING_LEN];

static const WCHAR PathUninstallW[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall";

/******************************************************************************
 * Name       : DllMain
 * Description: Entry point for DLL file
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason,
                    LPVOID lpvReserved)
{
    TRACE("(%p, %ld, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            hInst = hinstDLL;
            break;
    }
    return TRUE;
}

/******************************************************************************
 * Name       : FreeAppInfo
 * Description: Frees memory used by an AppInfo structure, and any children.
 */
static void FreeAppInfo(APPINFO *info)
{
    free(info->title);
    free(info->path);
    free(info->path_modify);
    free(info->icon);
    free(info->publisher);
    free(info->version);
    free(info->contact);
    free(info->helplink);
    free(info->helptelephone);
    free(info->readme);
    free(info->urlupdateinfo);
    free(info->comments);
    free(info);
}

static WCHAR *get_reg_str(HKEY hkey, const WCHAR *value)
{
    DWORD len, type;
    WCHAR *ret = NULL;
    if (!RegQueryValueExW(hkey, value, NULL, &type, NULL, &len) && type == REG_SZ)
    {
        if (!(ret = malloc(len))) return NULL;
        RegQueryValueExW(hkey, value, 0, 0, (BYTE *)ret, &len);
    }
    return ret;
}

/******************************************************************************
 * Name       : ReadApplicationsFromRegistry
 * Description: Creates a linked list of uninstallable applications from the
 *              registry.
 * Parameters : root    - Which registry root to read from
 * Returns    : TRUE if successful, FALSE otherwise
 */
static BOOL ReadApplicationsFromRegistry(HKEY root)
{
    HKEY hkeyApp;
    int i;
    static int id = 0;
    DWORD sizeOfSubKeyName, displen, uninstlen;
    DWORD dwNoModify, dwType, value, size;
    WCHAR subKeyName[256];
    WCHAR *command;
    APPINFO *info = NULL;
    LPWSTR iconPtr;

    sizeOfSubKeyName = ARRAY_SIZE(subKeyName);

    for (i = 0; RegEnumKeyExW(root, i, subKeyName, &sizeOfSubKeyName, NULL,
        NULL, NULL, NULL) != ERROR_NO_MORE_ITEMS; ++i)
    {
        RegOpenKeyExW(root, subKeyName, 0, KEY_READ, &hkeyApp);
        size = sizeof(value);
        if (!RegQueryValueExW(hkeyApp, L"SystemComponent", NULL, &dwType, (BYTE *)&value, &size)
            && dwType == REG_DWORD && value == 1)
        {
            RegCloseKey(hkeyApp);
            sizeOfSubKeyName = ARRAY_SIZE(subKeyName);
            continue;
        }
        displen = 0;
        uninstlen = 0;
        if (!RegQueryValueExW(hkeyApp, L"DisplayName", 0, 0, NULL, &displen))
        {
            size = sizeof(value);
            if (!RegQueryValueExW(hkeyApp, L"WindowsInstaller", NULL, &dwType, (BYTE *)&value, &size)
                && dwType == REG_DWORD && value == 1)
            {
                int len = lstrlenW(L"msiexec /x%s") + lstrlenW(subKeyName);

                if (!(command = malloc(len * sizeof(WCHAR)))) goto err;
                wsprintfW(command, L"msiexec /x%s", subKeyName);
            }
            else if (!RegQueryValueExW(hkeyApp, L"UninstallString", 0, 0, NULL, &uninstlen))
            {
                if (!(command = malloc(uninstlen))) goto err;
                RegQueryValueExW(hkeyApp, L"UninstallString", 0, 0, (BYTE *)command, &uninstlen);
            }
            else
            {
                RegCloseKey(hkeyApp);
                sizeOfSubKeyName = ARRAY_SIZE(subKeyName);
                continue;
            }

            info = calloc(1, sizeof(*info));
            if (!info) goto err;

            info->title = malloc(displen);

            if (!info->title)
                goto err;

            RegQueryValueExW(hkeyApp, L"DisplayName", 0, 0, (BYTE *)info->title, &displen);

            /* now get DisplayIcon */
            displen = 0;
            RegQueryValueExW(hkeyApp, L"DisplayIcon", 0, 0, NULL, &displen);

            if (displen == 0)
                info->icon = 0;
            else
            {
                info->icon = malloc(displen);

                if (!info->icon)
                    goto err;

                RegQueryValueExW(hkeyApp, L"DisplayIcon", 0, 0, (BYTE *)info->icon, &displen);

                /* separate the index from the icon name, if supplied */
                iconPtr = wcschr(info->icon, ',');

                if (iconPtr)
                {
                    *iconPtr++ = 0;
                    info->iconIdx = wcstol(iconPtr, NULL, 10);
                }
            }

            info->publisher = get_reg_str(hkeyApp, L"Publisher");
            info->version = get_reg_str(hkeyApp, L"DisplayVersion");
            info->contact = get_reg_str(hkeyApp, L"Contact");
            info->helplink = get_reg_str(hkeyApp, L"HelpLink");
            info->helptelephone = get_reg_str(hkeyApp, L"HelpTelephone");
            info->readme = get_reg_str(hkeyApp, L"Readme");
            info->urlupdateinfo = get_reg_str(hkeyApp, L"URLUpdateInfo");
            info->comments = get_reg_str(hkeyApp, L"Comments");

            /* Check if NoModify is set */
            dwType = REG_DWORD;
            dwNoModify = 0;
            displen = sizeof(DWORD);

            if (RegQueryValueExW(hkeyApp, L"NoModify", NULL, &dwType, (BYTE *)&dwNoModify, &displen)
                != ERROR_SUCCESS)
            {
                dwNoModify = 0;
            }

            /* Some installers incorrectly create a REG_SZ instead of a REG_DWORD */
            if (dwType == REG_SZ)
                dwNoModify = (*(BYTE *)&dwNoModify == '1');

            /* Fetch the modify path */
            if (!dwNoModify)
            {
                size = sizeof(value);
                if (!RegQueryValueExW(hkeyApp, L"WindowsInstaller", NULL, &dwType, (BYTE *)&value, &size)
                    && dwType == REG_DWORD && value == 1)
                {
                    int len = lstrlenW(L"msiexec /i%s") + lstrlenW(subKeyName);

                    if (!(info->path_modify = malloc(len * sizeof(WCHAR)))) goto err;
                    wsprintfW(info->path_modify, L"msiexec /i%s", subKeyName);
                }
                else if (!RegQueryValueExW(hkeyApp, L"ModifyPath", 0, 0, NULL, &displen))
                {
                    if (!(info->path_modify = malloc(displen))) goto err;
                    RegQueryValueExW(hkeyApp, L"ModifyPath", 0, 0, (BYTE *)info->path_modify, &displen);
                }
            }

            /* registry key */
            RegOpenKeyExW(root, NULL, 0, KEY_READ, &info->regroot);
            lstrcpyW(info->regkey, subKeyName);
            info->path = command;

            info->id = id++;
            list_add_tail( &app_list, &info->entry );
        }

        RegCloseKey(hkeyApp);
        sizeOfSubKeyName = ARRAY_SIZE(subKeyName);
    }

    return TRUE;
err:
    RegCloseKey(hkeyApp);
    if (info) FreeAppInfo(info);
    free(command);
    return FALSE;
}


/******************************************************************************
 * Name       : AddApplicationsToList
 * Description: Populates the list box with applications.
 * Parameters : hWnd    - Handle of the dialog box
 */
static void AddApplicationsToList(HWND hWnd, HIMAGELIST hList)
{
    APPINFO *iter;
    LVITEMW lvItem;
    HICON hIcon;
    int index;

    LIST_FOR_EACH_ENTRY( iter, &app_list, APPINFO, entry )
    {
        if (!iter->title[0]) continue;

        /* get the icon */
        index = 0;

        if (iter->icon)
        {
            if (ExtractIconExW(iter->icon, iter->iconIdx, NULL, &hIcon, 1) == 1)
            {
                index = ImageList_AddIcon(hList, hIcon);
                DestroyIcon(hIcon);
            }
        }

        lvItem.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM;
        lvItem.iItem = iter->id;
        lvItem.iSubItem = 0;
        lvItem.pszText = iter->title;
        lvItem.iImage = index;
        lvItem.lParam = iter->id;

        index = ListView_InsertItemW(hWnd, &lvItem);

        /* now add the subitems (columns) */
        ListView_SetItemTextW(hWnd, index, 1, iter->publisher);
        ListView_SetItemTextW(hWnd, index, 2, iter->version);
    }
}

/******************************************************************************
 * Name       : RemoveItemsFromList
 * Description: Clears the application list box.
 * Parameters : hWnd    - Handle of the dialog box
 */
static void RemoveItemsFromList(HWND hWnd)
{
    SendDlgItemMessageW(hWnd, IDL_PROGRAMS, LVM_DELETEALLITEMS, 0, 0);
}

/******************************************************************************
 * Name       : EmptyList
 * Description: Frees memory used by the application linked list.
 */
static inline void EmptyList(void)
{
    APPINFO *info, *next;
    LIST_FOR_EACH_ENTRY_SAFE( info, next, &app_list, APPINFO, entry )
    {
        list_remove( &info->entry );
        FreeAppInfo( info );
    }
}

/******************************************************************************
 * Name       : UpdateButtons
 * Description: Enables/disables the Add/Remove button depending on current
 *              selection in list box.
 * Parameters : hWnd    - Handle of the dialog box
 */
static void UpdateButtons(HWND hWnd)
{
    APPINFO *iter;
    LVITEMW lvItem;
    LRESULT selitem = SendDlgItemMessageW(hWnd, IDL_PROGRAMS, LVM_GETNEXTITEM, -1,
       LVNI_FOCUSED | LVNI_SELECTED);
    BOOL enable_modify = FALSE;

    if (selitem != -1)
    {
        lvItem.iItem = selitem;
        lvItem.mask = LVIF_PARAM;

        if (SendDlgItemMessageW(hWnd, IDL_PROGRAMS, LVM_GETITEMW, 0, (LPARAM) &lvItem))
        {
            LIST_FOR_EACH_ENTRY( iter, &app_list, APPINFO, entry )
            {
                if (iter->id == lvItem.lParam)
                {
                    /* Decide whether to display Modify/Remove as one button or two */
                    enable_modify = (iter->path_modify != NULL);

                    /* Update title as appropriate */
                    if (iter->path_modify == NULL)
                        SetWindowTextW(GetDlgItem(hWnd, IDC_ADDREMOVE), btnModifyRemove);
                    else
                        SetWindowTextW(GetDlgItem(hWnd, IDC_ADDREMOVE), btnRemove);

                    break;
                }
            }
        }
    }

    /* Enable/disable other buttons if necessary */
    EnableWindow(GetDlgItem(hWnd, IDC_ADDREMOVE), (selitem != -1));
    EnableWindow(GetDlgItem(hWnd, IDC_SUPPORT_INFO), (selitem != -1));
    EnableWindow(GetDlgItem(hWnd, IDC_MODIFY), enable_modify);
}

/******************************************************************************
 * Name       : InstallProgram
 * Description: Search for potential Installer and execute it.
 * Parameters : hWnd    - Handle of the dialog box
 */
static void InstallProgram(HWND hWnd)
{
    OPENFILENAMEW ofn;
    WCHAR titleW[MAX_STRING_LEN];
    WCHAR filter_installs[MAX_STRING_LEN];
    WCHAR filter_programs[MAX_STRING_LEN];
    WCHAR filter_all[MAX_STRING_LEN];
    WCHAR FilterBufferW[MAX_PATH];
    WCHAR FileNameBufferW[MAX_PATH];

    LoadStringW(hInst, IDS_CPL_TITLE, titleW, ARRAY_SIZE(titleW));
    LoadStringW(hInst, IDS_FILTER_INSTALLS, filter_installs, ARRAY_SIZE(filter_installs));
    LoadStringW(hInst, IDS_FILTER_PROGRAMS, filter_programs, ARRAY_SIZE(filter_programs));
    LoadStringW(hInst, IDS_FILTER_ALL, filter_all, ARRAY_SIZE(filter_all));

    swprintf( FilterBufferW, MAX_PATH, L"%s%c*instal*.exe;*setup*.exe;*.msi%c%s%c*.exe%c%s%c*.*%c",
              filter_installs, 0, 0, filter_programs, 0, 0, filter_all, 0, 0 );
    memset(&ofn, 0, sizeof(OPENFILENAMEW));
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = hWnd;
    ofn.hInstance = hInst;
    ofn.lpstrFilter = FilterBufferW;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = FileNameBufferW;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrTitle = titleW;
    ofn.Flags = OFN_HIDEREADONLY | OFN_ENABLESIZING;
    FileNameBufferW[0] = 0;

    if (GetOpenFileNameW(&ofn))
    {
        SHELLEXECUTEINFOW sei;
        memset(&sei, 0, sizeof(sei));
        sei.cbSize = sizeof(sei);
        sei.lpVerb = L"open";
        sei.nShow = SW_SHOWDEFAULT;
        sei.fMask = 0;
        sei.lpFile = ofn.lpstrFile;

        ShellExecuteExW(&sei);
    }
}

static HANDLE run_uninstaller(int id, DWORD button)
{
    APPINFO *iter;
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    WCHAR errormsg[MAX_STRING_LEN];
    WCHAR sUninstallFailed[MAX_STRING_LEN];
    BOOL res;

    LoadStringW(hInst, IDS_UNINSTALL_FAILED, sUninstallFailed,
        ARRAY_SIZE(sUninstallFailed));

    LIST_FOR_EACH_ENTRY( iter, &app_list, APPINFO, entry )
    {
        if (iter->id == id)
        {
            TRACE("Uninstalling %s (%s)\n", wine_dbgstr_w(iter->title),
                wine_dbgstr_w(iter->path));

            memset(&si, 0, sizeof(STARTUPINFOW));
            si.cb = sizeof(STARTUPINFOW);
            si.wShowWindow = SW_NORMAL;

            res = CreateProcessW(NULL, (button == IDC_MODIFY) ? iter->path_modify : iter->path,
                NULL, NULL, FALSE, 0, NULL, NULL, &si, &info);

            if (res)
            {
                CloseHandle(info.hThread);
                return info.hProcess;
            }
            else
            {
                wsprintfW(errormsg, sUninstallFailed, iter->path);

                if (MessageBoxW(0, errormsg, iter->title, MB_YESNO |
                    MB_ICONQUESTION) == IDYES)
                {
                    /* delete the application's uninstall entry */
                    RegDeleteKeyW(iter->regroot, iter->regkey);
                    RegCloseKey(iter->regroot);
                }
            }

            break;
        }
    }

    return NULL;
}

/**********************************************************************************
 * Name       : SetInfoDialogText
 * Description: Sets the text of a label in a window, based upon a registry entry
 *              or string passed to the function.
 * Parameters : hKey         - registry entry to read from, NULL if not reading
 *                             from registry
 *              lpKeyName    - key to read from, or string to check if hKey is NULL
 *              lpAltMessage - alternative message if entry not found
 *              hWnd         - handle of dialog box
 *              iDlgItem     - ID of label in dialog box
 */
static void SetInfoDialogText(HKEY hKey, LPCWSTR lpKeyName, LPCWSTR lpAltMessage,
  HWND hWnd, int iDlgItem)
{
    WCHAR buf[MAX_STRING_LEN];
    DWORD buflen;
    HWND hWndDlgItem;

    hWndDlgItem = GetDlgItem(hWnd, iDlgItem);

    /* if hKey is null, lpKeyName contains the string we want to check */
    if (hKey == NULL)
    {
        if (lpKeyName && lpKeyName[0])
            SetWindowTextW(hWndDlgItem, lpKeyName);
        else
            SetWindowTextW(hWndDlgItem, lpAltMessage);
    }
    else
    {
        buflen = sizeof(buf);

        if ((RegQueryValueExW(hKey, lpKeyName, 0, 0, (LPBYTE) buf, &buflen) ==
           ERROR_SUCCESS) && buf[0])
            SetWindowTextW(hWndDlgItem, buf);
        else
            SetWindowTextW(hWndDlgItem, lpAltMessage);
    }
}

/******************************************************************************
 * Name       : SupportInfoDlgProc
 * Description: Callback procedure for support info dialog
 * Parameters : hWnd    - hWnd of the window
 *              msg     - reason for calling function
 *              wParam  - additional parameter
 *              lParam  - additional parameter
 * Returns    : Depends on the message
 */
static INT_PTR CALLBACK SupportInfoDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    APPINFO *iter;
    HKEY hkey;
    WCHAR oldtitle[MAX_STRING_LEN];
    WCHAR buf[MAX_STRING_LEN];
    WCHAR key[MAX_STRING_LEN];
    WCHAR notfound[MAX_STRING_LEN];

    switch(msg)
    {
        case WM_INITDIALOG:
            LIST_FOR_EACH_ENTRY( iter, &app_list, APPINFO, entry )
            {
                if (iter->id == (int) lParam)
                {
                    lstrcpyW(key, PathUninstallW);
                    lstrcatW(key, L"\\");
                    lstrcatW(key, iter->regkey);

                    /* check the application's registry entries */
                    RegOpenKeyExW(iter->regroot, key, 0, KEY_READ, &hkey);

                    /* Load our "not specified" string */
                    LoadStringW(hInst, IDS_NOT_SPECIFIED, notfound, ARRAY_SIZE(notfound));

                    SetInfoDialogText(NULL, iter->publisher, notfound, hWnd, IDC_INFO_PUBLISHER);
                    SetInfoDialogText(NULL, iter->version, notfound, hWnd, IDC_INFO_VERSION);
                    SetInfoDialogText(hkey, iter->contact, notfound, hWnd, IDC_INFO_CONTACT);
                    SetInfoDialogText(hkey, iter->helplink, notfound, hWnd, IDC_INFO_SUPPORT);
                    SetInfoDialogText(hkey, iter->helptelephone, notfound, hWnd, IDC_INFO_PHONE);
                    SetInfoDialogText(hkey, iter->readme, notfound, hWnd, IDC_INFO_README);
                    SetInfoDialogText(hkey, iter->urlupdateinfo, notfound, hWnd, IDC_INFO_UPDATES);
                    SetInfoDialogText(hkey, iter->comments, notfound, hWnd, IDC_INFO_COMMENTS);

                    /* Update the main label with the app name */
                    if (GetWindowTextW(GetDlgItem(hWnd, IDC_INFO_LABEL), oldtitle,
                        MAX_STRING_LEN) != 0)
                    {
                        wsprintfW(buf, oldtitle, iter->title);
                        SetWindowTextW(GetDlgItem(hWnd, IDC_INFO_LABEL), buf);
                    }

                    RegCloseKey(hkey);

                    break;
                }
            }

            return TRUE;

        case WM_DESTROY:
            return 0;

        case WM_CLOSE:
            EndDialog(hWnd, TRUE);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDCANCEL:
                case IDOK:
                    EndDialog(hWnd, TRUE);
                    break;

            }

            return TRUE;
    }

    return FALSE;
}

/******************************************************************************
 * Name       : SupportInfo
 * Description: Displays the Support Information dialog
 * Parameters : hWnd    - Handle of the main dialog
 *              id      - ID of the application to display information for
 */
static void SupportInfo(HWND hWnd, int id)
{
    DialogBoxParamW(hInst, MAKEINTRESOURCEW(IDD_INFO), hWnd, SupportInfoDlgProc, id);
}

/* Definition of column headers for AddListViewColumns function */
typedef struct AppWizColumn {
   int width;
   int fmt;
   int title;
} AppWizColumn;

static const AppWizColumn columns[] = {
    {200, LVCFMT_LEFT, IDS_COLUMN_NAME},
    {150, LVCFMT_LEFT, IDS_COLUMN_PUBLISHER},
    {100, LVCFMT_LEFT, IDS_COLUMN_VERSION},
};

/******************************************************************************
 * Name       : AddListViewColumns
 * Description: Adds column headers to the list view control.
 * Parameters : hWnd    - Handle of the list view control.
 * Returns    : TRUE if completed successfully, FALSE otherwise.
 */
static BOOL AddListViewColumns(HWND hWnd)
{
    WCHAR buf[MAX_STRING_LEN];
    LVCOLUMNW lvc;
    UINT i;

    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;

    /* Add the columns */
    for (i = 0; i < ARRAY_SIZE(columns); i++)
    {
        lvc.iSubItem = i;
        lvc.pszText = buf;

        /* set width and format */
        lvc.cx = columns[i].width;
        lvc.fmt = columns[i].fmt;

        LoadStringW(hInst, columns[i].title, buf, ARRAY_SIZE(buf));

        if (ListView_InsertColumnW(hWnd, i, &lvc) == -1)
            return FALSE;
    }

    return TRUE;
}

/******************************************************************************
 * Name       : AddListViewImageList
 * Description: Creates an ImageList for the list view control.
 * Parameters : hWnd    - Handle of the list view control.
 * Returns    : Handle of the image list.
 */
static HIMAGELIST AddListViewImageList(HWND hWnd)
{
    HIMAGELIST hSmall;
    HICON hDefaultIcon;

    hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                              ILC_COLOR32 | ILC_MASK, 1, 1);

    /* Add default icon to image list */
    hDefaultIcon = LoadIconW(hInst, MAKEINTRESOURCEW(ICO_MAIN));
    ImageList_AddIcon(hSmall, hDefaultIcon);
    DestroyIcon(hDefaultIcon);

    SendMessageW(hWnd, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)hSmall);

    return hSmall;
}

/******************************************************************************
 * Name       : ResetApplicationList
 * Description: Empties the app list, if need be, and recreates it.
 * Parameters : bFirstRun  - TRUE if this is the first time this is run, FALSE otherwise
 *              hWnd       - handle of the dialog box
 *              hImageList - handle of the image list
 * Returns    : New handle of the image list.
 */
static HIMAGELIST ResetApplicationList(BOOL bFirstRun, HWND hWnd, HIMAGELIST hImageList)
{
    static const BOOL is_64bit = sizeof(void *) > sizeof(int);
    HWND hWndListView;
    HKEY hkey;

    hWndListView = GetDlgItem(hWnd, IDL_PROGRAMS);

    /* if first run, create the image list and add the listview columns */
    if (bFirstRun)
    {
        if (!AddListViewColumns(hWndListView))
            return NULL;
    }
    else /* we need to remove the existing things first */
    {
        RemoveItemsFromList(hWnd);
        ImageList_Destroy(hImageList);

        /* reset the list, since it's probably changed if the uninstallation was
           successful */
        EmptyList();
    }

    /* now create the image list and add the applications to the listview */
    hImageList = AddListViewImageList(hWndListView);

    if (!RegOpenKeyExW(HKEY_LOCAL_MACHINE, PathUninstallW, 0, KEY_READ, &hkey))
    {
        ReadApplicationsFromRegistry(hkey);
        RegCloseKey(hkey);
    }
    if (is_64bit &&
        !RegOpenKeyExW(HKEY_LOCAL_MACHINE, PathUninstallW, 0, KEY_READ|KEY_WOW64_32KEY, &hkey))
    {
        ReadApplicationsFromRegistry(hkey);
        RegCloseKey(hkey);
    }
    if (!RegOpenKeyExW(HKEY_CURRENT_USER, PathUninstallW, 0, KEY_READ, &hkey))
    {
        ReadApplicationsFromRegistry(hkey);
        RegCloseKey(hkey);
    }

    AddApplicationsToList(hWndListView, hImageList);
    UpdateButtons(hWnd);

    return(hImageList);
}

/******************************************************************************
 * Name       : MainDlgProc
 * Description: Callback procedure for main tab
 * Parameters : hWnd    - hWnd of the window
 *              msg     - reason for calling function
 *              wParam  - additional parameter
 *              lParam  - additional parameter
 * Returns    : Depends on the message
 */
static INT_PTR CALLBACK MainDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int selitem;
    static HANDLE uninstaller = NULL;
    static HIMAGELIST hImageList;
    LPNMHDR nmh;
    LVITEMW lvItem;

    switch(msg)
    {
        case WM_INITDIALOG:
            SendDlgItemMessageW(hWnd, IDL_PROGRAMS, LVM_SETEXTENDEDLISTVIEWSTYLE,
                LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

            hImageList = ResetApplicationList(TRUE, hWnd, hImageList);

            if (!hImageList)
                return FALSE;

            return TRUE;

        case WM_DESTROY:
            RemoveItemsFromList(hWnd);
            ImageList_Destroy(hImageList);

            EmptyList();

            return 0;

        case WM_NOTIFY:
            nmh = (LPNMHDR) lParam;

            switch (nmh->idFrom)
            {
                case IDL_PROGRAMS:
                    switch (nmh->code)
                    {
                        case LVN_ITEMCHANGED:
                            UpdateButtons(hWnd);
                            break;
                    }
                    break;
            }

            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_INSTALL:
                    InstallProgram(hWnd);
                    break;

                case IDC_ADDREMOVE:
                case IDC_MODIFY:
                    if (uninstaller)
                    {
                        WCHAR titleW[MAX_STRING_LEN], wait_tip[MAX_STRING_LEN];

                        LoadStringW(hInst, IDS_CPL_TITLE, titleW, ARRAY_SIZE(titleW));
                        LoadStringW(hInst, IDS_WAIT_COMPLETE, wait_tip, ARRAY_SIZE(wait_tip));
                        MessageBoxW(hWnd, wait_tip, titleW, MB_OK | MB_ICONWARNING);
                        break;
                    }

                    selitem = SendDlgItemMessageW(hWnd, IDL_PROGRAMS,
                        LVM_GETNEXTITEM, -1, LVNI_FOCUSED|LVNI_SELECTED);

                    if (selitem != -1)
                    {
                        lvItem.iItem = selitem;
                        lvItem.mask = LVIF_PARAM;

                        if (SendDlgItemMessageW(hWnd, IDL_PROGRAMS, LVM_GETITEMW, 0, (LPARAM)&lvItem))
                        {
                            uninstaller = run_uninstaller(lvItem.lParam, LOWORD(wParam));
                            while (MsgWaitForMultipleObjects(1, &uninstaller, FALSE, INFINITE, QS_ALLINPUT) == 1)
                            {
                                MSG message;

                                while (PeekMessageW(&message, 0, 0, 0, PM_REMOVE))
                                {
                                    TranslateMessage(&message);
                                    DispatchMessageW(&message);
                                }
                            }
                            CloseHandle(uninstaller);
                            uninstaller = NULL;
                        }
                    }

                    hImageList = ResetApplicationList(FALSE, hWnd, hImageList);

                    break;

                case IDC_SUPPORT_INFO:
                    selitem = SendDlgItemMessageW(hWnd, IDL_PROGRAMS,
                        LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED);

                    if (selitem != -1)
                    {
                        lvItem.iItem = selitem;
                        lvItem.mask = LVIF_PARAM;

                        if (SendDlgItemMessageW(hWnd, IDL_PROGRAMS, LVM_GETITEMW,
                          0, (LPARAM) &lvItem))
                            SupportInfo(hWnd, lvItem.lParam);
                    }

                    break;
            }

            return TRUE;
    }

    return FALSE;
}

static int CALLBACK propsheet_callback( HWND hwnd, UINT msg, LPARAM lparam )
{
    switch (msg)
    {
    case PSCB_INITIALIZED:
        SendMessageW( hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIconW( hInst, MAKEINTRESOURCEW(ICO_MAIN) ));
        break;
    }
    return 0;
}

/******************************************************************************
 * Name       : StartApplet
 * Description: Main routine for applet
 * Parameters : hWnd    - hWnd of the Control Panel
 */
static void StartApplet(HWND hWnd)
{
    PROPSHEETPAGEW psp;
    PROPSHEETHEADERW psh;
    WCHAR tab_title[MAX_STRING_LEN], app_title[MAX_STRING_LEN];

    /* Load the strings we will use */
    LoadStringW(hInst, IDS_TAB1_TITLE, tab_title, ARRAY_SIZE(tab_title));
    LoadStringW(hInst, IDS_CPL_TITLE, app_title, ARRAY_SIZE(app_title));
    LoadStringW(hInst, IDS_REMOVE, btnRemove, ARRAY_SIZE(btnRemove));
    LoadStringW(hInst, IDS_MODIFY_REMOVE, btnModifyRemove, ARRAY_SIZE(btnModifyRemove));

    /* Fill out the PROPSHEETPAGE */
    psp.dwSize = sizeof (PROPSHEETPAGEW);
    psp.dwFlags = PSP_USETITLE;
    psp.hInstance = hInst;
    psp.pszTemplate = MAKEINTRESOURCEW (IDD_MAIN);
    psp.pszIcon = NULL;
    psp.pfnDlgProc = MainDlgProc;
    psp.pszTitle = tab_title;
    psp.lParam = 0;

    /* Fill out the PROPSHEETHEADER */
    psh.dwSize = sizeof (PROPSHEETHEADERW);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_USECALLBACK;
    psh.hwndParent = hWnd;
    psh.hInstance = hInst;
    psh.pszIcon = MAKEINTRESOURCEW(ICO_MAIN);
    psh.pszCaption = app_title;
    psh.nPages = 1;
    psh.ppsp = &psp;
    psh.pfnCallback = propsheet_callback;
    psh.nStartPage = 0;

    /* Display the property sheet */
    PropertySheetW (&psh);
}

static LONG start_params(const WCHAR *params)
{
    if(!params)
        return FALSE;

    if(!wcscmp(params, L"install_gecko")) {
        install_addon(ADDON_GECKO);
        return TRUE;
    }

    if(!wcscmp(params, L"install_mono")) {
        install_addon(ADDON_MONO);
        return TRUE;
    }

    WARN("unknown param %s\n", debugstr_w(params));
    return FALSE;
}

/******************************************************************************
 * Name       : CPlApplet
 * Description: Entry point for Control Panel applets
 * Parameters : hwndCPL - hWnd of the Control Panel
 *              message - reason for calling function
 *              lParam1 - additional parameter
 *              lParam2 - additional parameter
 * Returns    : Depends on the message
 */
LONG CALLBACK CPlApplet(HWND hwndCPL, UINT message, LPARAM lParam1, LPARAM lParam2)
{
    INITCOMMONCONTROLSEX iccEx;

    switch (message)
    {
        case CPL_INIT:
            iccEx.dwSize = sizeof(iccEx);
            iccEx.dwICC = ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES | ICC_LINK_CLASS;

            InitCommonControlsEx(&iccEx);

            return TRUE;

        case CPL_GETCOUNT:
            return 1;

        case CPL_STARTWPARMSW:
            return start_params((const WCHAR *)lParam2);

        case CPL_INQUIRE:
        {
            CPLINFO *appletInfo = (CPLINFO *) lParam2;

            appletInfo->idIcon = ICO_MAIN;
            appletInfo->idName = IDS_CPL_TITLE;
            appletInfo->idInfo = IDS_CPL_DESC;
            appletInfo->lData = 0;

            break;
        }

        case CPL_DBLCLK:
            StartApplet(hwndCPL);
            break;
    }

    return FALSE;
}
