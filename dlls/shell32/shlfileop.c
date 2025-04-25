/*
 * SHFileOperation
 *
 * Copyright 2000 Juergen Schmied
 * Copyright 2002 Andriy Palamarchuk
 * Copyright 2004 Dietrich Teickner (from Odin)
 * Copyright 2004 Rolf Kalbermatter
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

#define COBJMACROS

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "shellapi.h"
#include "wingdi.h"
#include "winuser.h"
#include "shlobj.h"
#include "shresdef.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"
#include "shell32_main.h"
#include "shfldr.h"
#include "sherrors.h"
#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define IsAttrib(x, y)  ((INVALID_FILE_ATTRIBUTES != (x)) && ((x) & (y)))
#define IsAttribFile(x) (!((x) & FILE_ATTRIBUTE_DIRECTORY))
#define IsAttribDir(x)  IsAttrib(x, FILE_ATTRIBUTE_DIRECTORY)
#define IsDotDir(x)     ((x[0] == '.') && ((x[1] == 0) || ((x[1] == '.') && (x[2] == 0))))

#define FO_MASK         0xF

#define DE_SAMEFILE        0x71
#define DE_MANYSRC1DEST    0x72
#define DE_DIFFDIR         0x73
#define DE_ROOTDIR         0x74
#define DE_OPCANCELLED     0x75
#define DE_DESTSUBTREE     0x76
#define DE_ACCESSDENIEDSRC 0x78
#define DE_PATHTOODEEP     0x79
#define DE_MANYDEST        0x7A
#define DE_INVALIDFILES    0x7C
#define DE_DESTSAMETREE    0x7D
#define DE_FLDDESTISFILE   0x7E
#define DE_FILEDESTISFLD   0x80

static DWORD SHNotifyCreateDirectoryA(LPCSTR path, LPSECURITY_ATTRIBUTES sec);
static DWORD SHNotifyCreateDirectoryW(LPCWSTR path, LPSECURITY_ATTRIBUTES sec);
static DWORD SHNotifyRemoveDirectoryA(LPCSTR path);
static DWORD SHNotifyRemoveDirectoryW(LPCWSTR path);
static DWORD SHNotifyDeleteFileA(LPCSTR path);
static DWORD SHNotifyDeleteFileW(LPCWSTR path);
static DWORD SHNotifyMoveFileW(LPCWSTR src, LPCWSTR dest);
static DWORD SHNotifyCopyFileW(LPCWSTR src, LPCWSTR dest, BOOL bFailIfExists);
static DWORD SHFindAttrW(LPCWSTR pName, BOOL fileOnly);

typedef struct
{
    SHFILEOPSTRUCTW *req;
    DWORD dwYesToAllMask;
    BOOL bManyItems;
    BOOL bCancelled;
} FILE_OPERATION;

/* Confirm dialogs with an optional "Yes To All" as used in file operations confirmations
 */
struct confirm_msg_info
{
    LPWSTR lpszText;
    LPWSTR lpszCaption;
    HICON hIcon;
    BOOL bYesToAll;
};

/* as some buttons may be hidden and the dialog height may change we may need
 * to move the controls */
static void confirm_msg_move_button(HWND hDlg, INT iId, INT *xPos, INT yOffset, BOOL bShow)
{
    HWND hButton = GetDlgItem(hDlg, iId);
    RECT r;

    if (bShow) {
        int width;

        GetWindowRect(hButton, &r);
        MapWindowPoints( 0, hDlg, (POINT *)&r, 2 );
        width = r.right - r.left;
        SetWindowPos(hButton, 0, *xPos - width, r.top - yOffset, 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW );
        *xPos -= width + 5;
    }
    else
        ShowWindow(hButton, SW_HIDE);
}

/* Note: we paint the text manually and don't use the static control to make
 * sure the text has the same height as the one computed in WM_INITDIALOG
 */
static INT_PTR ConfirmMsgBox_Paint(HWND hDlg)
{
    PAINTSTRUCT ps;
    HFONT hOldFont;
    RECT r;
    HDC hdc;

    BeginPaint(hDlg, &ps);
    hdc = ps.hdc;
    SetBkMode(hdc, TRANSPARENT);

    GetClientRect(GetDlgItem(hDlg, IDD_MESSAGE), &r);
    /* this will remap the rect to dialog coords */
    MapWindowPoints(GetDlgItem(hDlg, IDD_MESSAGE), hDlg, (LPPOINT)&r, 2);
    hOldFont = SelectObject(hdc, (HFONT)SendDlgItemMessageW(hDlg, IDD_MESSAGE, WM_GETFONT, 0, 0));
    DrawTextW(hdc, GetPropW(hDlg, L"WINE_CONFIRM"), -1, &r, DT_NOPREFIX | DT_PATH_ELLIPSIS | DT_WORDBREAK);
    SelectObject(hdc, hOldFont);
    EndPaint(hDlg, &ps);
    return TRUE;
}

static INT_PTR ConfirmMsgBox_Init(HWND hDlg, LPARAM lParam)
{
    struct confirm_msg_info *info = (struct confirm_msg_info *)lParam;
    INT xPos, yOffset;
    int width, height;
    HFONT hOldFont;
    HDC hdc;
    RECT r;

    SetWindowTextW(hDlg, info->lpszCaption);
    ShowWindow(GetDlgItem(hDlg, IDD_MESSAGE), SW_HIDE);
    SetPropW(hDlg, L"WINE_CONFIRM", info->lpszText);
    SendDlgItemMessageW(hDlg, IDD_ICON, STM_SETICON, (WPARAM)info->hIcon, 0);

    /* compute the text height and resize the dialog */
    GetClientRect(GetDlgItem(hDlg, IDD_MESSAGE), &r);
    hdc = GetDC(hDlg);
    yOffset = r.bottom;
    hOldFont = SelectObject(hdc, (HFONT)SendDlgItemMessageW(hDlg, IDD_MESSAGE, WM_GETFONT, 0, 0));
    DrawTextW(hdc, info->lpszText, -1, &r, DT_NOPREFIX | DT_PATH_ELLIPSIS | DT_WORDBREAK | DT_CALCRECT);
    SelectObject(hdc, hOldFont);
    yOffset -= r.bottom;
    yOffset = min(yOffset, 35);  /* don't make the dialog too small */
    ReleaseDC(hDlg, hdc);

    GetClientRect(hDlg, &r);
    xPos = r.right - 7;
    GetWindowRect(hDlg, &r);
    width = r.right - r.left;
    height = r.bottom - r.top - yOffset;
    MoveWindow(hDlg, (GetSystemMetrics(SM_CXSCREEN) - width)/2,
        (GetSystemMetrics(SM_CYSCREEN) - height)/2, width, height, FALSE);

    confirm_msg_move_button(hDlg, IDCANCEL,     &xPos, yOffset, info->bYesToAll);
    confirm_msg_move_button(hDlg, IDNO,         &xPos, yOffset, TRUE);
    confirm_msg_move_button(hDlg, IDD_YESTOALL, &xPos, yOffset, info->bYesToAll);
    confirm_msg_move_button(hDlg, IDYES,        &xPos, yOffset, TRUE);
    return TRUE;
}

static INT_PTR CALLBACK ConfirmMsgBoxProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            return ConfirmMsgBox_Init(hDlg, lParam);
        case WM_PAINT:
            return ConfirmMsgBox_Paint(hDlg);
        case WM_COMMAND:
            EndDialog(hDlg, wParam);
            break;
        case WM_CLOSE:
            EndDialog(hDlg, IDCANCEL);
            break;
    }
    return FALSE;
}

static int SHELL_ConfirmMsgBox(HWND hWnd, LPWSTR lpszText, LPWSTR lpszCaption, HICON hIcon, BOOL bYesToAll)
{
    struct confirm_msg_info info;

    info.lpszText = lpszText;
    info.lpszCaption = lpszCaption;
    info.hIcon = hIcon;
    info.bYesToAll = bYesToAll;
    return DialogBoxParamW(shell32_hInstance, L"SHELL_YESTOALL_MSGBOX", hWnd, ConfirmMsgBoxProc, (LPARAM)&info);
}

/* confirmation dialogs content */
typedef struct
{
        HINSTANCE hIconInstance;
        UINT icon_resource_id;
	UINT caption_resource_id, text_resource_id;
} SHELL_ConfirmIDstruc;

static BOOL SHELL_ConfirmIDs(int nKindOfDialog, SHELL_ConfirmIDstruc *ids)
{
        ids->hIconInstance = shell32_hInstance;
	switch (nKindOfDialog) {
	  case ASK_DELETE_FILE:
            ids->icon_resource_id = IDI_SHELL_CONFIRM_DELETE;
	    ids->caption_resource_id  = IDS_DELETEITEM_CAPTION;
	    ids->text_resource_id  = IDS_DELETEITEM_TEXT;
	    return TRUE;
	  case ASK_DELETE_FOLDER:
            ids->icon_resource_id = IDI_SHELL_CONFIRM_DELETE;
	    ids->caption_resource_id  = IDS_DELETEFOLDER_CAPTION;
	    ids->text_resource_id  = IDS_DELETEITEM_TEXT;
	    return TRUE;
	  case ASK_DELETE_MULTIPLE_ITEM:
            ids->icon_resource_id = IDI_SHELL_CONFIRM_DELETE;
	    ids->caption_resource_id  = IDS_DELETEITEM_CAPTION;
	    ids->text_resource_id  = IDS_DELETEMULTIPLE_TEXT;
	    return TRUE;
          case ASK_TRASH_FILE:
            ids->icon_resource_id = IDI_SHELL_TRASH_FILE;
            ids->caption_resource_id = IDS_DELETEITEM_CAPTION;
            ids->text_resource_id = IDS_TRASHITEM_TEXT;
            return TRUE;
          case ASK_TRASH_FOLDER:
            ids->icon_resource_id = IDI_SHELL_TRASH_FILE;
            ids->caption_resource_id = IDS_DELETEFOLDER_CAPTION;
            ids->text_resource_id = IDS_TRASHFOLDER_TEXT;
            return TRUE;
          case ASK_TRASH_MULTIPLE_ITEM:
            ids->icon_resource_id = IDI_SHELL_TRASH_FILE;
            ids->caption_resource_id = IDS_DELETEITEM_CAPTION;
            ids->text_resource_id = IDS_TRASHMULTIPLE_TEXT;
            return TRUE;
          case ASK_CANT_TRASH_ITEM:
            ids->icon_resource_id = IDI_SHELL_CONFIRM_DELETE;
            ids->caption_resource_id  = IDS_DELETEITEM_CAPTION;
            ids->text_resource_id  = IDS_CANTTRASH_TEXT;
            return TRUE;
	  case ASK_DELETE_SELECTED:
            ids->icon_resource_id = IDI_SHELL_CONFIRM_DELETE;
            ids->caption_resource_id  = IDS_DELETEITEM_CAPTION;
            ids->text_resource_id  = IDS_DELETESELECTED_TEXT;
            return TRUE;
	  case ASK_OVERWRITE_FILE:
            ids->hIconInstance = NULL;
            ids->icon_resource_id = IDI_WARNING;
	    ids->caption_resource_id  = IDS_OVERWRITEFILE_CAPTION;
	    ids->text_resource_id  = IDS_OVERWRITEFILE_TEXT;
            return TRUE;
	  case ASK_OVERWRITE_FOLDER:
            ids->hIconInstance = NULL;
            ids->icon_resource_id = IDI_WARNING;
            ids->caption_resource_id  = IDS_OVERWRITEFILE_CAPTION;
            ids->text_resource_id  = IDS_OVERWRITEFOLDER_TEXT;
            return TRUE;
	  default:
	    FIXME(" Unhandled nKindOfDialog %d stub\n", nKindOfDialog);
	}
	return FALSE;
}

static BOOL SHELL_ConfirmDialogW(HWND hWnd, int nKindOfDialog, LPCWSTR szDir, FILE_OPERATION *op)
{
	WCHAR szCaption[255], szText[255], szBuffer[MAX_PATH + 256];
	SHELL_ConfirmIDstruc ids;
	DWORD_PTR args[1];
	HICON hIcon;
	int ret;

        assert(nKindOfDialog >= 0 && nKindOfDialog < 32);
        if (op && (op->dwYesToAllMask & (1 << nKindOfDialog)))
            return TRUE;

        if (!SHELL_ConfirmIDs(nKindOfDialog, &ids)) return FALSE;

        LoadStringW(shell32_hInstance, ids.caption_resource_id, szCaption, ARRAY_SIZE(szCaption));
        LoadStringW(shell32_hInstance, ids.text_resource_id, szText, ARRAY_SIZE(szText));

        args[0] = (DWORD_PTR)szDir;
        FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
            szText, 0, 0, szBuffer, ARRAY_SIZE(szBuffer), (va_list*)args);

        hIcon = LoadIconW(ids.hIconInstance, (LPWSTR)MAKEINTRESOURCE(ids.icon_resource_id));

        ret = SHELL_ConfirmMsgBox(hWnd, szBuffer, szCaption, hIcon, op && op->bManyItems);
        if (op) {
            if (ret == IDD_YESTOALL) {
                op->dwYesToAllMask |= (1 << nKindOfDialog);
                ret = IDYES;
            }
            if (ret == IDCANCEL)
                op->bCancelled = TRUE;
            if (ret != IDYES)
                op->req->fAnyOperationsAborted = TRUE;
        }
        return ret == IDYES;
}

BOOL SHELL_ConfirmYesNoW(HWND hWnd, int nKindOfDialog, LPCWSTR szDir)
{
    return SHELL_ConfirmDialogW(hWnd, nKindOfDialog, szDir, NULL);
}

static DWORD SHELL32_AnsiToUnicodeBuf(LPCSTR aPath, LPWSTR *wPath, DWORD minChars)
{
	DWORD len = MultiByteToWideChar(CP_ACP, 0, aPath, -1, NULL, 0);

	if (len < minChars)
	  len = minChars;

	*wPath = malloc(len * sizeof(WCHAR));
	if (*wPath)
	{
	  MultiByteToWideChar(CP_ACP, 0, aPath, -1, *wPath, len);
	  return NO_ERROR;
	}
	return E_OUTOFMEMORY;
}

HRESULT WINAPI SHIsFileAvailableOffline(LPCWSTR path, LPDWORD status)
{
    FIXME("(%s, %p) stub\n", debugstr_w(path), status);
    return E_FAIL;
}

/**************************************************************************
 * SHELL_DeleteDirectory()  [internal]
 *
 * Asks for confirmation when bShowUI is true and deletes the directory and
 * all its subdirectories and files if necessary.
 */
static DWORD SHELL_DeleteDirectoryW(HWND hwnd, LPCWSTR pszDir, BOOL bShowUI)
{
    DWORD    ret = 0;
    HANDLE  hFind;
    WIN32_FIND_DATAW wfd;
    WCHAR   szTemp[MAX_PATH];

    PathCombineW(szTemp, pszDir, L"*");
    hFind = FindFirstFileW(szTemp, &wfd);

    if (hFind != INVALID_HANDLE_VALUE) {
        if (!bShowUI || SHELL_ConfirmDialogW(hwnd, ASK_DELETE_FOLDER, pszDir, NULL)) {
            do {
                if (IsDotDir(wfd.cFileName))
                    continue;
                PathCombineW(szTemp, pszDir, wfd.cFileName);
                if (FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes)
                    ret = SHELL_DeleteDirectoryW(hwnd, szTemp, FALSE);
                else
                    ret = SHNotifyDeleteFileW(szTemp);
            } while (!ret && FindNextFileW(hFind, &wfd));
        }
        FindClose(hFind);
    }
    if (ret == ERROR_SUCCESS)
        ret = SHNotifyRemoveDirectoryW(pszDir);

    return ret == ERROR_PATH_NOT_FOUND ?
        0x7C: /* DE_INVALIDFILES (legacy Windows error) */
        ret;
}

/**************************************************************************
 * Win32CreateDirectory      [SHELL32.93]
 *
 * Creates a directory. Also triggers a change notify if one exists.
 *
 * PARAMS
 *  path       [I]   path to directory to create
 *
 * RETURNS
 *  TRUE if successful, FALSE otherwise
 *
 * NOTES
 *  Verified on Win98 / IE 5 (SHELL32 4.72, March 1999 build) to be ANSI.
 *  This is Unicode on NT/2000
 */
static DWORD SHNotifyCreateDirectoryA(LPCSTR path, LPSECURITY_ATTRIBUTES sec)
{
	LPWSTR wPath;
	DWORD retCode;

	TRACE("(%s, %p)\n", debugstr_a(path), sec);

	retCode = SHELL32_AnsiToUnicodeBuf(path, &wPath, 0);
	if (!retCode)
	{
	  retCode = SHNotifyCreateDirectoryW(wPath, sec);
	  free(wPath);
	}
	return retCode;
}

/**********************************************************************/

static DWORD SHNotifyCreateDirectoryW(LPCWSTR path, LPSECURITY_ATTRIBUTES sec)
{
	TRACE("(%s, %p)\n", debugstr_w(path), sec);

	if (CreateDirectoryW(path, sec))
	{
	  SHChangeNotify(SHCNE_MKDIR, SHCNF_PATHW, path, NULL);
	  return ERROR_SUCCESS;
	}
	return GetLastError();
}

/**********************************************************************/

BOOL WINAPI Win32CreateDirectoryAW(LPCVOID path, LPSECURITY_ATTRIBUTES sec)
{
	if (SHELL_OsIsUnicode())
	  return (SHNotifyCreateDirectoryW(path, sec) == ERROR_SUCCESS);
	return (SHNotifyCreateDirectoryA(path, sec) == ERROR_SUCCESS);
}

/************************************************************************
 * Win32RemoveDirectory      [SHELL32.94]
 *
 * Deletes a directory. Also triggers a change notify if one exists.
 *
 * PARAMS
 *  path       [I]   path to directory to delete
 *
 * RETURNS
 *  TRUE if successful, FALSE otherwise
 *
 * NOTES
 *  Verified on Win98 / IE 5 (SHELL32 4.72, March 1999 build) to be ANSI.
 *  This is Unicode on NT/2000
 */
static DWORD SHNotifyRemoveDirectoryA(LPCSTR path)
{
	LPWSTR wPath;
	DWORD retCode;

	TRACE("(%s)\n", debugstr_a(path));

	retCode = SHELL32_AnsiToUnicodeBuf(path, &wPath, 0);
	if (!retCode)
	{
	  retCode = SHNotifyRemoveDirectoryW(wPath);
	  free(wPath);
	}
	return retCode;
}

/***********************************************************************/

static DWORD SHNotifyRemoveDirectoryW(LPCWSTR path)
{
	BOOL ret;
	TRACE("(%s)\n", debugstr_w(path));

	ret = RemoveDirectoryW(path);
	if (!ret)
	{
	  /* Directory may be write protected */
	  DWORD dwAttr = GetFileAttributesW(path);
	  if (IsAttrib(dwAttr, FILE_ATTRIBUTE_READONLY))
	    if (SetFileAttributesW(path, dwAttr & ~FILE_ATTRIBUTE_READONLY))
	      ret = RemoveDirectoryW(path);
	}
	if (ret)
	{
	  SHChangeNotify(SHCNE_RMDIR, SHCNF_PATHW, path, NULL);
	  return ERROR_SUCCESS;
	}
	return GetLastError();
}

/***********************************************************************/

BOOL WINAPI Win32RemoveDirectoryAW(LPCVOID path)
{
	if (SHELL_OsIsUnicode())
	  return (SHNotifyRemoveDirectoryW(path) == ERROR_SUCCESS);
	return (SHNotifyRemoveDirectoryA(path) == ERROR_SUCCESS);
}

/************************************************************************
 * Win32DeleteFile           [SHELL32.164]
 *
 * Deletes a file. Also triggers a change notify if one exists.
 *
 * PARAMS
 *  path       [I]   path to file to delete
 *
 * RETURNS
 *  TRUE if successful, FALSE otherwise
 *
 * NOTES
 *  Verified on Win98 / IE 5 (SHELL32 4.72, March 1999 build) to be ANSI.
 *  This is Unicode on NT/2000
 */
static DWORD SHNotifyDeleteFileA(LPCSTR path)
{
	LPWSTR wPath;
	DWORD retCode;

	TRACE("(%s)\n", debugstr_a(path));

	retCode = SHELL32_AnsiToUnicodeBuf(path, &wPath, 0);
	if (!retCode)
	{
	  retCode = SHNotifyDeleteFileW(wPath);
	  free(wPath);
	}
	return retCode;
}

/***********************************************************************/

static DWORD SHNotifyDeleteFileW(LPCWSTR path)
{
	BOOL ret;

	TRACE("(%s)\n", debugstr_w(path));

	ret = DeleteFileW(path);
	if (!ret)
	{
	  /* File may be write protected or a system file */
	  DWORD dwAttr = GetFileAttributesW(path);
	  if (IsAttrib(dwAttr, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM))
	    if (SetFileAttributesW(path, dwAttr & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
	      ret = DeleteFileW(path);
	}
	if (ret)
	{
	  SHChangeNotify(SHCNE_DELETE, SHCNF_PATHW, path, NULL);
	  return ERROR_SUCCESS;
	}
	return GetLastError();
}

/***********************************************************************/

DWORD WINAPI Win32DeleteFileAW(LPCVOID path)
{
	if (SHELL_OsIsUnicode())
	  return (SHNotifyDeleteFileW(path) == ERROR_SUCCESS);
	return (SHNotifyDeleteFileA(path) == ERROR_SUCCESS);
}

/************************************************************************
 * SHNotifyMoveFile          [internal]
 *
 * Moves a file. Also triggers a change notify if one exists.
 *
 * PARAMS
 *  src        [I]   path to source file to move
 *  dest       [I]   path to target file to move to
 *
 * RETURNS
 *  ERROR_SUCCESS if successful
 */
static DWORD SHNotifyMoveFileW(LPCWSTR src, LPCWSTR dest)
{
	BOOL ret;

	TRACE("(%s %s)\n", debugstr_w(src), debugstr_w(dest));

        ret = MoveFileExW(src, dest, MOVEFILE_REPLACE_EXISTING);

        /* MOVEFILE_REPLACE_EXISTING fails with dirs, so try MoveFile */
        if (!ret)
            ret = MoveFileW(src, dest);

	if (!ret)
	{
	  DWORD dwAttr;

	  dwAttr = SHFindAttrW(dest, FALSE);
	  if (INVALID_FILE_ATTRIBUTES == dwAttr)
	  {
	    /* Source file may be write protected or a system file */
	    dwAttr = GetFileAttributesW(src);
	    if (IsAttrib(dwAttr, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM))
	      if (SetFileAttributesW(src, dwAttr & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
	        ret = MoveFileW(src, dest);
	  }
	}
	if (ret)
	{
	  SHChangeNotify(SHCNE_RENAMEITEM, SHCNF_PATHW, src, dest);
	  return ERROR_SUCCESS;
	}
	return GetLastError();
}

/************************************************************************
 * SHNotifyCopyFile          [internal]
 *
 * Copies a file. Also triggers a change notify if one exists.
 *
 * PARAMS
 *  src           [I]   path to source file to move
 *  dest          [I]   path to target file to move to
 *  bFailIfExists [I]   if TRUE, the target file will not be overwritten if
 *                      a file with this name already exists
 *
 * RETURNS
 *  ERROR_SUCCESS if successful
 */
static DWORD SHNotifyCopyFileW(LPCWSTR src, LPCWSTR dest, BOOL bFailIfExists)
{
	BOOL ret;
	DWORD attribs;

	TRACE("(%s %s %s)\n", debugstr_w(src), debugstr_w(dest), bFailIfExists ? "failIfExists" : "");

        /* Destination file may already exist with read only attribute */
        attribs = GetFileAttributesW(dest);
        if (IsAttrib(attribs, FILE_ATTRIBUTE_READONLY))
          SetFileAttributesW(dest, attribs & ~FILE_ATTRIBUTE_READONLY);

	ret = CopyFileW(src, dest, bFailIfExists);
	if (ret)
	{
	  SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW, dest, NULL);
	  return ERROR_SUCCESS;
	}

	return GetLastError();
}

/*************************************************************************
 * SHCreateDirectory         [SHELL32.165]
 *
 * This function creates a file system folder whose fully qualified path is
 * given by path. If one or more of the intermediate folders do not exist,
 * they will be created as well.
 *
 * PARAMS
 *  hWnd       [I]
 *  path       [I]   path of directory to create
 *
 * RETURNS
 *  ERROR_SUCCESS or one of the following values:
 *  ERROR_BAD_PATHNAME if the path is relative
 *  ERROR_FILE_EXISTS when a file with that name exists
 *  ERROR_PATH_NOT_FOUND can't find the path, probably invalid
 *  ERROR_INVALID_NAME if the path contains invalid chars
 *  ERROR_ALREADY_EXISTS when the directory already exists
 *  ERROR_FILENAME_EXCED_RANGE if the filename was too long to process
 *
 * NOTES
 *  exported by ordinal
 *  Win9x exports ANSI
 *  WinNT/2000 exports Unicode
 */
DWORD WINAPI SHCreateDirectory(HWND hWnd, LPCVOID path)
{
	if (SHELL_OsIsUnicode())
	  return SHCreateDirectoryExW(hWnd, path, NULL);
	return SHCreateDirectoryExA(hWnd, path, NULL);
}

/*************************************************************************
 * SHCreateDirectoryExA      [SHELL32.@]
 *
 * This function creates a file system folder whose fully qualified path is
 * given by path. If one or more of the intermediate folders do not exist,
 * they will be created as well.
 *
 * PARAMS
 *  hWnd       [I]
 *  path       [I]   path of directory to create
 *  sec        [I]   security attributes to use or NULL
 *
 * RETURNS
 *  ERROR_SUCCESS or one of the following values:
 *  ERROR_BAD_PATHNAME or ERROR_PATH_NOT_FOUND if the path is relative
 *  ERROR_INVALID_NAME if the path contains invalid chars
 *  ERROR_FILE_EXISTS when a file with that name exists
 *  ERROR_ALREADY_EXISTS when the directory already exists
 *  ERROR_FILENAME_EXCED_RANGE if the filename was too long to process
 *
 *  FIXME: Not implemented yet;
 *  SHCreateDirectoryEx also verifies that the files in the directory will be visible
 *  if the path is a network path to deal with network drivers which might have a limited
 *  but unknown maximum path length. If not:
 *
 *  If hWnd is set to a valid window handle, a message box is displayed warning
 *  the user that the files may not be accessible. If the user chooses not to
 *  proceed, the function returns ERROR_CANCELLED.
 *
 *  If hWnd is set to NULL, no user interface is displayed and the function
 *  returns ERROR_CANCELLED.
 */
int WINAPI SHCreateDirectoryExA(HWND hWnd, LPCSTR path, LPSECURITY_ATTRIBUTES sec)
{
	LPWSTR wPath;
	DWORD retCode;

	TRACE("(%s, %p)\n", debugstr_a(path), sec);

	retCode = SHELL32_AnsiToUnicodeBuf(path, &wPath, 0);
	if (!retCode)
	{
	  retCode = SHCreateDirectoryExW(hWnd, wPath, sec);
	  free(wPath);
	}
	return retCode;
}

/*************************************************************************
 * SHCreateDirectoryExW      [SHELL32.@]
 *
 * See SHCreateDirectoryExA.
 */
int WINAPI SHCreateDirectoryExW(HWND hWnd, LPCWSTR path, LPSECURITY_ATTRIBUTES sec)
{
	int ret = ERROR_BAD_PATHNAME;
	TRACE("(%p, %s, %p)\n", hWnd, debugstr_w(path), sec);

	if (PathIsRelativeW(path))
	{
	  SetLastError(ret);
	}
	else
	{
	  ret = SHNotifyCreateDirectoryW(path, sec);
	  /* Refuse to work on certain error codes before trying to create directories recursively */
	  if (ret != ERROR_SUCCESS &&
	      ret != ERROR_FILE_EXISTS &&
	      ret != ERROR_ALREADY_EXISTS &&
	      ret != ERROR_FILENAME_EXCED_RANGE)
	  {
	    WCHAR *pEnd, *pSlash, szTemp[MAX_PATH + 1];  /* extra for PathAddBackslash() */

	    lstrcpynW(szTemp, path, MAX_PATH);
	    pEnd = PathAddBackslashW(szTemp);
	    pSlash = szTemp + 3;

	    while (*pSlash)
	    {
              while (*pSlash && *pSlash != '\\') pSlash++;
	      if (*pSlash)
	      {
	        *pSlash = 0;    /* terminate path at separator */

	        ret = SHNotifyCreateDirectoryW(szTemp, pSlash + 1 == pEnd ? sec : NULL);
	      }
	      *pSlash++ = '\\'; /* put the separator back */
	    }
	  }

	  if (ret && hWnd &&
	      ret != ERROR_CANCELLED &&
	      ret != ERROR_ALREADY_EXISTS)
	  {
	    /* We failed and should show a dialog box */
	    FIXME("Show system error message, creating path %s, failed with error %d\n", debugstr_w(path), ret);
	    ret = ERROR_CANCELLED; /* Error has been already presented to user (not really yet!) */
	  }
	}
	return ret;
}

/*************************************************************************
 * SHFindAttrW      [internal]
 *
 * Get the Attributes for a file or directory. The difference to GetAttributes()
 * is that this function will also work for paths containing wildcard characters
 * in its filename.

 * PARAMS
 *  path       [I]   path of directory or file to check
 *  fileOnly   [I]   TRUE if only files should be found
 *
 * RETURNS
 *  INVALID_FILE_ATTRIBUTES if the path does not exist, the actual attributes of
 *  the first file or directory found otherwise
 */
static DWORD SHFindAttrW(LPCWSTR pName, BOOL fileOnly)
{
	WIN32_FIND_DATAW wfd;
	BOOL b_FileMask = fileOnly && (NULL != StrPBrkW(pName, L"*?"));
	DWORD dwAttr = INVALID_FILE_ATTRIBUTES;
	HANDLE hFind = FindFirstFileW(pName, &wfd);

	TRACE("%s %d\n", debugstr_w(pName), fileOnly);
	if (INVALID_HANDLE_VALUE != hFind)
	{
	  do
	  {
	    if (b_FileMask && IsAttribDir(wfd.dwFileAttributes))
	       continue;
	    dwAttr = wfd.dwFileAttributes;
	    break;
	  }
	  while (FindNextFileW(hFind, &wfd));
	  FindClose(hFind);
	}
	return dwAttr;
}

/*************************************************************************
 *
 * SHNameTranslate HelperFunction for SHFileOperationA
 *
 * Translates a list of 0 terminated ANSI strings into Unicode. If *wString
 * is NULL, only the necessary size of the string is determined and returned,
 * otherwise the ANSI strings are copied into it and the buffer is increased
 * to point to the location after the final 0 termination char.
 */
static DWORD SHNameTranslate(LPWSTR* wString, LPCWSTR* pWToFrom, BOOL more)
{
	DWORD size = 0, aSize = 0;
	LPCSTR aString = (LPCSTR)*pWToFrom;

	if (aString)
	{
	  do
	  {
	    size = lstrlenA(aString) + 1;
	    aSize += size;
	    aString += size;
	  } while ((size != 1) && more);
	  /* The two sizes might be different in the case of multibyte chars */
	  size = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)*pWToFrom, aSize, *wString, 0);
	  if (*wString) /* only in the second loop */
	  {
	    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)*pWToFrom, aSize, *wString, size);
	    *pWToFrom = *wString;
	    *wString += size;
	  }
	}
	return size;
}
/*************************************************************************
 * SHFileOperationA          [SHELL32.@]
 *
 * Function to copy, move, delete and create one or more files with optional
 * user prompts.
 *
 * PARAMS
 *  lpFileOp   [I/O] pointer to a structure containing all the necessary information
 *
 * RETURNS
 *  Success: ERROR_SUCCESS.
 *  Failure: ERROR_CANCELLED.
 *
 * NOTES
 *  exported by name
 */
int WINAPI SHFileOperationA(LPSHFILEOPSTRUCTA lpFileOp)
{
	SHFILEOPSTRUCTW nFileOp = *((LPSHFILEOPSTRUCTW)lpFileOp);
	int retCode = 0;
	DWORD size;
	LPWSTR ForFree = NULL, /* we change wString in SHNameTranslate and can't use it for freeing */
	       wString = NULL; /* we change this in SHNameTranslate */

	TRACE("\n");
	if (FO_DELETE == (nFileOp.wFunc & FO_MASK))
	  nFileOp.pTo = NULL; /* we need a NULL or a valid pointer for translation */
	if (!(nFileOp.fFlags & FOF_SIMPLEPROGRESS))
	  nFileOp.lpszProgressTitle = NULL; /* we need a NULL or a valid pointer for translation */
	while (1) /* every loop calculate size, second translate also, if we have storage for this */
	{
	  size = SHNameTranslate(&wString, &nFileOp.lpszProgressTitle, FALSE); /* no loop */
	  size += SHNameTranslate(&wString, &nFileOp.pFrom, TRUE); /* internal loop */
	  size += SHNameTranslate(&wString, &nFileOp.pTo, TRUE); /* internal loop */

	  if (ForFree)
	  {
	    retCode = SHFileOperationW(&nFileOp);
	    /* Windows 95/98 returns S_OK for this case. */
	    if (retCode == ERROR_ACCESS_DENIED && (GetVersion() & 0x80000000))
	      retCode = S_OK;

	    free(ForFree); /* we cannot use wString, it was changed */
	    break;
	  }
	  else
	  {
	    wString = ForFree = malloc(size * sizeof(WCHAR));
	    if (ForFree) continue;
	    retCode = ERROR_OUTOFMEMORY;
	    nFileOp.fAnyOperationsAborted = TRUE;
	    return retCode;
	  }
	}

	lpFileOp->hNameMappings = nFileOp.hNameMappings;
	lpFileOp->fAnyOperationsAborted = nFileOp.fAnyOperationsAborted;
	return retCode;
}

#define ERROR_SHELL_INTERNAL_FILE_NOT_FOUND 1026

typedef struct
{
    DWORD attributes;
    LPWSTR szDirectory;
    LPWSTR szFilename;
    LPWSTR szFullPath;
    BOOL bFromWildcard;
    BOOL bFromRelative;
    BOOL bExists;
} FILE_ENTRY;

typedef struct
{
    FILE_ENTRY *feFiles;
    DWORD num_alloc;
    DWORD dwNumFiles;
    BOOL bAnyFromWildcard;
    BOOL bAnyDirectories;
    BOOL bAnyDontExist;
} FILE_LIST;


static inline void grow_list(FILE_LIST *list)
{
    FILE_ENTRY *new = _recalloc(list->feFiles, list->num_alloc * 2, sizeof(*new));
    list->feFiles = new;
    list->num_alloc *= 2;
}

static void file_entry_init(FILE_ENTRY *file_entry,
        const WCHAR *file_name, DWORD attributes, BOOL from_relative, BOOL from_wildcard)
{
    size_t file_name_len = wcslen(file_name) + 1;
    const WCHAR *ptr;

    file_entry->szFullPath = malloc(file_name_len * sizeof(WCHAR));
    wcscpy(file_entry->szFullPath, file_name);

    ptr = wcsrchr(file_name, '\\');
    if (ptr)
    {
        file_name_len = ptr - file_name + 1;
        file_entry->szDirectory = malloc(file_name_len * sizeof(WCHAR));
        lstrcpynW(file_entry->szDirectory, file_name, file_name_len);

        file_name_len = wcslen(file_entry->szFullPath) - file_name_len + 1;
        file_entry->szFilename = malloc(file_name_len * sizeof(WCHAR));
        lstrcpyW(file_entry->szFilename, ptr + 1); /* Skip over backslash. */
    }

    file_entry->attributes = attributes;
    file_entry->bFromRelative = from_relative;
    file_entry->bFromWildcard = from_wildcard;
    file_entry->bExists = (attributes != INVALID_FILE_ATTRIBUTES);
}

static void file_entry_destroy(FILE_ENTRY *file_entry)
{
    if (!file_entry)
        return;
    if (file_entry->szDirectory)
        free(file_entry->szDirectory);
    if(file_entry->szFilename)
        free(file_entry->szFilename);
    if (file_entry->szFullPath)
        free(file_entry->szFullPath);
}

static void file_list_add_entry(FILE_LIST *file_list, DWORD index,
        const WCHAR *file_name, DWORD attributes, BOOL from_relative, BOOL from_wildcard)
{
    FILE_ENTRY *file_entry;

    if (index >= file_list->num_alloc)
        grow_list(file_list);
    file_entry = &file_list->feFiles[index];

    file_entry_init(file_entry, file_name, attributes, from_relative, from_wildcard);

    if (IsAttribDir(attributes))
        file_list->bAnyDirectories = TRUE;
    if (from_wildcard)
        file_list->bAnyFromWildcard = TRUE;
    if (!file_entry->bExists)
        file_list->bAnyDontExist = TRUE;
}

static void file_list_destroy(FILE_LIST *flList)
{
    DWORD i;

    if (!flList || !flList->feFiles)
        return;

    for (i = 0; i < flList->dwNumFiles; i++)
        file_entry_destroy(&flList->feFiles[i]);
    free(flList->feFiles);
}

static LPWSTR wildcard_to_file(LPCWSTR szWildCard, LPCWSTR szFileName)
{
    LPCWSTR ptr;
    LPWSTR szFullPath;
    DWORD dwDirLen, dwFullLen;

    ptr = StrRChrW(szWildCard, NULL, '\\');
    dwDirLen = ptr - szWildCard + 1;

    dwFullLen = dwDirLen + lstrlenW(szFileName) + 1;
    szFullPath = malloc(dwFullLen * sizeof(WCHAR));

    lstrcpynW(szFullPath, szWildCard, dwDirLen + 1);
    lstrcatW(szFullPath, szFileName);

    return szFullPath;
}

static void parse_wildcard_files(FILE_LIST *flList, LPCWSTR szFile, LPDWORD pdwListIndex, BOOL from_relative)
{
    WIN32_FIND_DATAW wfd;
    HANDLE hFile = FindFirstFileW(szFile, &wfd);
    LPWSTR szFullPath;
    BOOL res;

    if (hFile == INVALID_HANDLE_VALUE) return;

    for (res = TRUE; res; res = FindNextFileW(hFile, &wfd))
    {
        if (IsDotDir(wfd.cFileName)) continue;
        szFullPath = wildcard_to_file(szFile, wfd.cFileName);
        file_list_add_entry(flList, (*pdwListIndex)++, szFullPath, wfd.dwFileAttributes, from_relative, TRUE);
        free(szFullPath);
    }

    FindClose(hFile);
}

/* takes the null-separated file list and fills out the FILE_LIST */
static HRESULT parse_file_list(FILE_LIST *flList, LPCWSTR szFiles, BOOL parse_wildcard)
{
    LPCWSTR ptr = szFiles;
    WCHAR szCurFile[MAX_PATH];
    WCHAR *p;
    DWORD i = 0;

    if (!szFiles)
        return ERROR_INVALID_PARAMETER;

    flList->bAnyFromWildcard = FALSE;
    flList->bAnyDirectories = FALSE;
    flList->bAnyDontExist = FALSE;
    flList->num_alloc = 32;
    flList->dwNumFiles = 0;

    /* empty list */
    if (!szFiles[0])
        return ERROR_ACCESS_DENIED;

    flList->feFiles = calloc(flList->num_alloc, sizeof(FILE_ENTRY));

    while (*ptr)
    {
        BOOL from_relative = PathIsRelativeW(ptr), from_wildcard = !!wcspbrk(ptr, L"*?");

        /* change relative to absolute path */
        if (from_relative)
        {
            GetCurrentDirectoryW(MAX_PATH, szCurFile);
            PathCombineW(szCurFile, szCurFile, ptr);
        }
        else
        {
            lstrcpyW(szCurFile, ptr);
        }

        for (p = szCurFile; *p; p++) if (*p == '/') *p = '\\';

        /* parse wildcard files if they are in the filename */
        if (from_wildcard && parse_wildcard)
        {
            parse_wildcard_files(flList, szCurFile, &i, from_relative);
            i--;
        }
        else
        {
            file_list_add_entry(flList, i, szCurFile, GetFileAttributesW(szCurFile), from_relative, from_wildcard);
        }

        /* advance to the next string */
        ptr += lstrlenW(ptr) + 1;
        i++;
    }
    flList->dwNumFiles = i;

    return S_OK;
}

static void copy_dir_to_dir(FILE_OPERATION *op, const FILE_ENTRY *feFrom, LPCWSTR szDestPath)
{
    WCHAR szFrom[MAX_PATH], szTo[MAX_PATH];
    SHFILEOPSTRUCTW fileOp;

    if (IsDotDir(feFrom->szFilename))
        return;

    if (PathFileExistsW(szDestPath))
        PathCombineW(szTo, szDestPath, feFrom->szFilename);
    else
        lstrcpyW(szTo, szDestPath);

    if (!(op->req->fFlags & FOF_NOCONFIRMATION) && PathFileExistsW(szTo)) {
        if (!SHELL_ConfirmDialogW(op->req->hwnd, ASK_OVERWRITE_FOLDER, feFrom->szFilename, op))
        {
            /* Vista returns an ERROR_CANCELLED even if user pressed "No" */
            if (!op->bManyItems)
                op->bCancelled = TRUE;
            return;
        }
    }

    szTo[lstrlenW(szTo) + 1] = '\0';
    SHNotifyCreateDirectoryW(szTo, NULL);

    PathCombineW(szFrom, feFrom->szFullPath, L"*.*");
    szFrom[lstrlenW(szFrom) + 1] = '\0';

    fileOp = *op->req;
    fileOp.pFrom = szFrom;
    fileOp.pTo = szTo;
    fileOp.fFlags &= ~FOF_MULTIDESTFILES; /* we know we're copying to one dir */

    /* Don't ask the user about overwriting files when he accepted to overwrite the
       folder. FIXME: this is not exactly what Windows does - e.g. there would be
       an additional confirmation for a nested folder */
    fileOp.fFlags |= FOF_NOCONFIRMATION;  

    SHFileOperationW(&fileOp);
}

static BOOL copy_file_to_file(FILE_OPERATION *op, const WCHAR *szFrom, const WCHAR *szTo)
{
    if (!(op->req->fFlags & FOF_NOCONFIRMATION) && PathFileExistsW(szTo))
    {
        if (!SHELL_ConfirmDialogW(op->req->hwnd, ASK_OVERWRITE_FILE, PathFindFileNameW(szTo), op))
            return FALSE;
    }

    return SHNotifyCopyFileW(szFrom, szTo, FALSE) == 0;
}

/* copy a file or directory to another directory */
static void copy_to_dir(FILE_OPERATION *op, const FILE_ENTRY *feFrom, const FILE_ENTRY *feTo)
{
    if (!PathFileExistsW(feTo->szFullPath))
        SHNotifyCreateDirectoryW(feTo->szFullPath, NULL);

    if (IsAttribFile(feFrom->attributes))
    {
        WCHAR szDestPath[MAX_PATH];

        PathCombineW(szDestPath, feTo->szFullPath, feFrom->szFilename);
        copy_file_to_file(op, feFrom->szFullPath, szDestPath);
    }
    else if (!(op->req->fFlags & FOF_FILESONLY && feFrom->bFromWildcard))
        copy_dir_to_dir(op, feFrom, feTo->szFullPath);
}

static void create_dest_dirs(LPCWSTR szDestDir)
{
    WCHAR dir[MAX_PATH];
    LPCWSTR ptr = StrChrW(szDestDir, '\\');

    /* make sure all directories up to last one are created */
    while (ptr && (ptr = StrChrW(ptr + 1, '\\')))
    {
        lstrcpynW(dir, szDestDir, ptr - szDestDir + 1);

        if (!PathFileExistsW(dir))
            SHNotifyCreateDirectoryW(dir, NULL);
    }

    /* create last directory */
    if (!PathFileExistsW(szDestDir))
        SHNotifyCreateDirectoryW(szDestDir, NULL);
}

/* the FO_COPY operation */
static int copy_files(FILE_OPERATION *op, const FILE_LIST *flFrom, FILE_LIST *flTo)
{
    DWORD i;
    const FILE_ENTRY *entryToCopy;
    const FILE_ENTRY *fileDest = &flTo->feFiles[0];

    if (flFrom->bAnyDontExist)
        return ERROR_SHELL_INTERNAL_FILE_NOT_FOUND;

    if (flTo->dwNumFiles == 0)
    {
        /* If the destination is empty, SHFileOperation should use the current directory */
        WCHAR curdir[MAX_PATH+1];

        GetCurrentDirectoryW(MAX_PATH, curdir);
        curdir[lstrlenW(curdir)+1] = 0;

        file_list_destroy(flTo);
        ZeroMemory(flTo, sizeof(FILE_LIST));
        parse_file_list(flTo, curdir, FALSE);
        fileDest = &flTo->feFiles[0];
    }

    if (op->req->fFlags & FOF_MULTIDESTFILES && flTo->dwNumFiles > 1)
    {
        if (flFrom->bAnyFromWildcard)
            return ERROR_CANCELLED;

        if (flFrom->dwNumFiles != flTo->dwNumFiles)
        {
            if (flFrom->dwNumFiles != 1 && !IsAttribDir(fileDest->attributes))
                return ERROR_CANCELLED;

            /* Free all but the first entry. */
            for (i = 1; i < flTo->dwNumFiles; i++)
            {
                free(flTo->feFiles[i].szDirectory);
                free(flTo->feFiles[i].szFilename);
                free(flTo->feFiles[i].szFullPath);
            }

            flTo->dwNumFiles = 1;
        }
        else if (IsAttribDir(fileDest->attributes))
        {
            for (i = 1; i < flTo->dwNumFiles; i++)
                if (!IsAttribDir(flTo->feFiles[i].attributes) ||
                    !IsAttribDir(flFrom->feFiles[i].attributes))
                {
                    return ERROR_CANCELLED;
                }
        }
    }
    else if (flFrom->dwNumFiles != 1)
    {
        if (flTo->dwNumFiles != 1 && !IsAttribDir(fileDest->attributes))
            return ERROR_CANCELLED;

        if (PathFileExistsW(fileDest->szFullPath) &&
            IsAttribFile(fileDest->attributes))
        {
            return ERROR_CANCELLED;
        }

        if (flTo->dwNumFiles == 1 && fileDest->bFromRelative &&
            !PathFileExistsW(fileDest->szFullPath))
        {
            return ERROR_CANCELLED;
        }
    }

    for (i = 0; i < flFrom->dwNumFiles; i++)
    {
        entryToCopy = &flFrom->feFiles[i];

        if ((op->req->fFlags & FOF_MULTIDESTFILES) &&
            flTo->dwNumFiles > 1)
        {
            fileDest = &flTo->feFiles[i];
        }

        if (IsAttribDir(entryToCopy->attributes) &&
            !lstrcmpiW(entryToCopy->szFullPath, fileDest->szDirectory))
        {
            return ERROR_SUCCESS;
        }

        create_dest_dirs(fileDest->szDirectory);

        if (!lstrcmpiW(entryToCopy->szFullPath, fileDest->szFullPath))
        {
            if (IsAttribFile(entryToCopy->attributes))
                return ERROR_NO_MORE_SEARCH_HANDLES;
            else
                return ERROR_SUCCESS;
        }

        if ((flFrom->dwNumFiles > 1 && flTo->dwNumFiles == 1) ||
            IsAttribDir(fileDest->attributes))
        {
            copy_to_dir(op, entryToCopy, fileDest);
        }
        else if (IsAttribDir(entryToCopy->attributes))
        {
            copy_dir_to_dir(op, entryToCopy, fileDest->szFullPath);
        }
        else
        {
            if (!copy_file_to_file(op, entryToCopy->szFullPath, fileDest->szFullPath))
            {
                op->req->fAnyOperationsAborted = TRUE;
                return ERROR_CANCELLED;
            }
        }

        /* Vista return code. XP would return e.g. ERROR_FILE_NOT_FOUND, ERROR_ALREADY_EXISTS */
        if (op->bCancelled)
            return ERROR_CANCELLED;
    }

    /* Vista return code. On XP if the used pressed "No" for the last item,
     * ERROR_ARENA_TRASHED would be returned */
    return ERROR_SUCCESS;
}

static BOOL confirm_delete_list(HWND hWnd, DWORD fFlags, BOOL fTrash, const FILE_LIST *flFrom)
{
    if (flFrom->dwNumFiles > 1)
    {
        WCHAR tmp[12];

        swprintf(tmp, ARRAY_SIZE(tmp), L"%d", flFrom->dwNumFiles);
        return SHELL_ConfirmDialogW(hWnd, (fTrash?ASK_TRASH_MULTIPLE_ITEM:ASK_DELETE_MULTIPLE_ITEM), tmp, NULL);
    }
    else
    {
        const FILE_ENTRY *fileEntry = &flFrom->feFiles[0];

        if (IsAttribFile(fileEntry->attributes))
            return SHELL_ConfirmDialogW(hWnd, (fTrash?ASK_TRASH_FILE:ASK_DELETE_FILE), fileEntry->szFullPath, NULL);
        else if (!(fFlags & FOF_FILESONLY && fileEntry->bFromWildcard))
            return SHELL_ConfirmDialogW(hWnd, (fTrash?ASK_TRASH_FOLDER:ASK_DELETE_FOLDER), fileEntry->szFullPath, NULL);
    }
    return TRUE;
}

/* the FO_DELETE operation */
static int delete_files(LPSHFILEOPSTRUCTW lpFileOp, const FILE_LIST *flFrom)
{
    const FILE_ENTRY *fileEntry;
    DWORD i;
    int ret;
    BOOL bTrash;

    if (!flFrom->dwNumFiles)
        return ERROR_SUCCESS;

    /* Windows also checks only the first item */
    bTrash = (lpFileOp->fFlags & FOF_ALLOWUNDO) && is_trash_available();

    if (!(lpFileOp->fFlags & FOF_NOCONFIRMATION) || (!bTrash && lpFileOp->fFlags & FOF_WANTNUKEWARNING))
        if (!confirm_delete_list(lpFileOp->hwnd, lpFileOp->fFlags, bTrash, flFrom))
        {
            lpFileOp->fAnyOperationsAborted = TRUE;
            return 0;
        }

    for (i = 0; i < flFrom->dwNumFiles; i++)
    {
        fileEntry = &flFrom->feFiles[i];

        if (!IsAttribFile(fileEntry->attributes) &&
            (lpFileOp->fFlags & FOF_FILESONLY && fileEntry->bFromWildcard))
            continue;

        if (bTrash)
        {
            BOOL bDelete;
            if (trash_file(fileEntry->szFullPath))
                continue;

            /* Note: Windows silently deletes the file in such a situation, we show a dialog */
            if (!(lpFileOp->fFlags & FOF_NOCONFIRMATION) || (lpFileOp->fFlags & FOF_WANTNUKEWARNING))
                bDelete = SHELL_ConfirmDialogW(lpFileOp->hwnd, ASK_CANT_TRASH_ITEM, fileEntry->szFullPath, NULL);
            else
                bDelete = TRUE;

            if (!bDelete)
            {
                lpFileOp->fAnyOperationsAborted = TRUE;
                break;
            }
        }
        
        /* delete the file or directory */
        if (IsAttribFile(fileEntry->attributes))
            ret = DeleteFileW(fileEntry->szFullPath) ?
                    ERROR_SUCCESS : GetLastError();
        else
            ret = SHELL_DeleteDirectoryW(lpFileOp->hwnd, fileEntry->szFullPath, FALSE);

        if (ret)
            return ret;
    }

    return ERROR_SUCCESS;
}

/* move a directory to another directory */
static void move_dir_to_dir(LPSHFILEOPSTRUCTW lpFileOp, const FILE_ENTRY *feFrom, const FILE_ENTRY *feTo)
{
    WCHAR from[MAX_PATH], to[MAX_PATH];

    /* Windows doesn't combine path when FOF_MULTIDESTFILES is set */
    if (lpFileOp->fFlags & FOF_MULTIDESTFILES)
        lstrcpyW(to, feTo->szFullPath);
    else
        PathCombineW(to, feTo->szFullPath, feFrom->szFilename);

    to[lstrlenW(to) + 1] = '\0';

    /* If destination directory already exists, append source directory
       with wildcard and restart SHFileOperationW */
    if (PathFileExistsW(to))
    {
        SHFILEOPSTRUCTW fileOp;

        PathCombineW(from, feFrom->szFullPath, L"*.*");
        from[lstrlenW(from) + 1] = '\0';

        fileOp = *lpFileOp;
        fileOp.pFrom = from;
        fileOp.pTo = to;
        fileOp.fFlags &= ~FOF_MULTIDESTFILES; /* we know we're moving to one dir */

        /* Don't ask the user about overwriting files when he accepted to overwrite the
           folder. FIXME: this is not exactly what Windows does - e.g. there would be
           an additional confirmation for a nested folder */
        fileOp.fFlags |= FOF_NOCONFIRMATION;

        if (!SHFileOperationW(&fileOp))
            RemoveDirectoryW(feFrom->szFullPath);
        return;
    }
    else
    {
        SHNotifyMoveFileW(feFrom->szFullPath, to);
    }
}

/* move a file to another directory */
static void move_file_to_dir(LPSHFILEOPSTRUCTW lpFileOp, const FILE_ENTRY *feFrom, const FILE_ENTRY *feTo)
{
    WCHAR to[MAX_PATH];

    PathCombineW(to, feTo->szFullPath, feFrom->szFilename);
    to[lstrlenW(to) + 1] = '\0';
    SHNotifyMoveFileW(feFrom->szFullPath, to);
}

/* the FO_MOVE operation */
static int move_files(LPSHFILEOPSTRUCTW lpFileOp, const FILE_LIST *flFrom, const FILE_LIST *flTo)
{
    DWORD i;
    INT mismatched = 0;
    const FILE_ENTRY *entryToMove;
    const FILE_ENTRY *fileDest;
    int ret;

    if (!flFrom->dwNumFiles)
        return ERROR_SUCCESS;

    if (!flTo->dwNumFiles)
        return ERROR_FILE_NOT_FOUND;

    if (!(lpFileOp->fFlags & FOF_MULTIDESTFILES) &&
        flTo->dwNumFiles > 1 && flFrom->dwNumFiles > 1)
    {
        return ERROR_CANCELLED;
    }

    if (!(lpFileOp->fFlags & FOF_MULTIDESTFILES) &&
        !flFrom->bAnyDirectories &&
        flFrom->dwNumFiles > flTo->dwNumFiles)
    {
        return ERROR_CANCELLED;
    }

    ret = SHCreateDirectoryExW(NULL, flTo->feFiles[0].szDirectory, NULL);
    if (ret && ret != ERROR_ALREADY_EXISTS)
        return ret;

    if (lpFileOp->fFlags & FOF_MULTIDESTFILES)
        mismatched = flFrom->dwNumFiles - flTo->dwNumFiles;

    fileDest = &flTo->feFiles[0];
    for (i = 0; i < flFrom->dwNumFiles; i++)
    {
        entryToMove = &flFrom->feFiles[i];

        if (!PathFileExistsW(fileDest->szDirectory))
            return ERROR_CANCELLED;

        if (lpFileOp->fFlags & FOF_MULTIDESTFILES)
        {
            if (i >= flTo->dwNumFiles)
                break;
            fileDest = &flTo->feFiles[i];
            if (mismatched && !fileDest->bExists)
            {
                create_dest_dirs(flTo->feFiles[i].szFullPath);
                flTo->feFiles[i].bExists = TRUE;
                flTo->feFiles[i].attributes = FILE_ATTRIBUTE_DIRECTORY;
            }
        }

        if (fileDest->bExists && IsAttribDir(fileDest->attributes))
        {
            if (IsAttribDir(entryToMove->attributes))
                move_dir_to_dir(lpFileOp, entryToMove, fileDest);
            else
                move_file_to_dir(lpFileOp, entryToMove, fileDest);
        }
        else
            SHNotifyMoveFileW(entryToMove->szFullPath, fileDest->szFullPath);
    }

    if (mismatched > 0)
    {
        if (flFrom->bAnyDirectories)
            return DE_DESTSAMETREE;
        else
            return DE_SAMEFILE;
    }

    return ERROR_SUCCESS;
}

/* The FO_RENAME operation of SHFileOperation. */
static DWORD rename_files(SHFILEOPSTRUCTW *op, const FILE_LIST *from, const FILE_LIST *to)
{
    const FILE_ENTRY *entry_from, *entry_to;

    if (from->dwNumFiles != 1)
        return DE_MANYSRC1DEST;

    entry_from = &from->feFiles[0];
    if (entry_from->bFromWildcard)
        return DE_MANYSRC1DEST;
    if (!entry_from->bExists)
        return ERROR_FILE_NOT_FOUND;

    if (!to->dwNumFiles)
        return DE_DIFFDIR;
    entry_to = &to->feFiles[0];

    if (wcscmp(entry_from->szDirectory, entry_to->szDirectory) != 0)
        return DE_DIFFDIR;

    if (entry_to->bExists && IsAttribDir(entry_from->attributes) != IsAttribDir(entry_to->attributes))
        return IsAttribDir(entry_to->attributes) ? DE_FILEDESTISFLD : DE_FLDDESTISFILE;

    return SHNotifyMoveFileW(entry_from->szFullPath, entry_to->szFullPath);
}

/* alert the user if an unsupported flag is used */
static void check_flags(FILEOP_FLAGS fFlags)
{
    WORD wUnsupportedFlags = FOF_NO_CONNECTED_ELEMENTS |
        FOF_NOCOPYSECURITYATTRIBS | FOF_NORECURSEREPARSE |
        FOF_RENAMEONCOLLISION | FOF_WANTMAPPINGHANDLE;

    if (fFlags & wUnsupportedFlags)
        FIXME("Unsupported flags: %04x\n", fFlags);
}

/*************************************************************************
 * SHFileOperationW          [SHELL32.@]
 *
 * See SHFileOperationA
 */
int WINAPI SHFileOperationW(LPSHFILEOPSTRUCTW lpFileOp)
{
    FILE_OPERATION op;
    FILE_LIST flFrom, flTo;
    int ret = 0;

    if (!lpFileOp)
        return ERROR_INVALID_PARAMETER;

    check_flags(lpFileOp->fFlags);

    ZeroMemory(&flFrom, sizeof(FILE_LIST));
    ZeroMemory(&flTo, sizeof(FILE_LIST));

    if ((ret = parse_file_list(&flFrom, lpFileOp->pFrom, lpFileOp->wFunc != FO_RENAME)))
        return ret;

    if (lpFileOp->wFunc != FO_DELETE)
        parse_file_list(&flTo, lpFileOp->pTo, FALSE);

    ZeroMemory(&op, sizeof(op));
    op.req = lpFileOp;
    op.bManyItems = (flFrom.dwNumFiles > 1);
    lpFileOp->fAnyOperationsAborted = FALSE;

    if (flTo.bAnyFromWildcard)
        return ERROR_INVALID_NAME;

    switch (lpFileOp->wFunc)
    {
        case FO_COPY:
            ret = copy_files(&op, &flFrom, &flTo);
            break;
        case FO_DELETE:
            ret = delete_files(lpFileOp, &flFrom);
            break;
        case FO_MOVE:
            ret = move_files(lpFileOp, &flFrom, &flTo);
            break;
        case FO_RENAME:
            ret = rename_files(lpFileOp, &flFrom, &flTo);
            break;
        default:
            ret = ERROR_INVALID_PARAMETER;
            break;
    }

    file_list_destroy(&flFrom);

    if (lpFileOp->wFunc != FO_DELETE)
        file_list_destroy(&flTo);

    if (ret == ERROR_CANCELLED)
        lpFileOp->fAnyOperationsAborted = TRUE;

    SetLastError(ERROR_SUCCESS);
    return ret;
}

#define SHDSA_GetItemCount(hdsa) (*(int*)(hdsa))

/*************************************************************************
 * SHFreeNameMappings      [shell32.246]
 *
 * Free the mapping handle returned by SHFileOperation if FOF_WANTSMAPPINGHANDLE
 * was specified.
 *
 * PARAMS
 *  hNameMapping [I] handle to the name mappings used during renaming of files
 *
 * RETURNS
 *  Nothing
 */
void WINAPI SHFreeNameMappings(HANDLE hNameMapping)
{
	if (hNameMapping)
	{
	  int i = SHDSA_GetItemCount((HDSA)hNameMapping) - 1;

	  for (; i>= 0; i--)
	  {
            LPSHNAMEMAPPINGW lp = DSA_GetItemPtr(hNameMapping, i);

	    SHFree(lp->pszOldPath);
	    SHFree(lp->pszNewPath);
	  }
          DSA_Destroy(hNameMapping);
	}
}

/*************************************************************************
 * SheGetDirA [SHELL32.@]
 *
 * drive = 0: returns the current directory path
 * drive > 0: returns the current directory path of the specified drive
 *            drive=1 -> A:  drive=2 -> B:  ...
 * returns 0 if successful
*/
DWORD WINAPI SheGetDirA(DWORD drive, LPSTR buffer)
{
    WCHAR org_path[MAX_PATH];
    DWORD ret;
    char drv_path[3];

    /* change current directory to the specified drive */
    if (drive) {
        strcpy(drv_path, "A:");
        drv_path[0] += (char)drive-1;

        GetCurrentDirectoryW(MAX_PATH, org_path);

        SetCurrentDirectoryA(drv_path);
    }

    /* query current directory path of the specified drive */
    ret = GetCurrentDirectoryA(MAX_PATH, buffer);

    /* back to the original drive */
    if (drive)
        SetCurrentDirectoryW(org_path);

    if (!ret)
        return GetLastError();

    return 0;
}

/*************************************************************************
 * SheGetDirW [SHELL32.@]
 *
 * drive = 0: returns the current directory path
 * drive > 0: returns the current directory path of the specified drive
 *            drive=1 -> A:  drive=2 -> B:  ...
 * returns 0 if successful
 */
DWORD WINAPI SheGetDirW(DWORD drive, LPWSTR buffer)
{
    WCHAR org_path[MAX_PATH];
    DWORD ret;
    char drv_path[3];

    /* change current directory to the specified drive */
    if (drive) {
        strcpy(drv_path, "A:");
        drv_path[0] += (char)drive-1;

        GetCurrentDirectoryW(MAX_PATH, org_path);

        SetCurrentDirectoryA(drv_path);
    }

    /* query current directory path of the specified drive */
    ret = GetCurrentDirectoryW(MAX_PATH, buffer);

    /* back to the original drive */
    if (drive)
        SetCurrentDirectoryW(org_path);

    if (!ret)
        return GetLastError();

    return 0;
}

/*************************************************************************
 * SheChangeDirA [SHELL32.@]
 *
 * changes the current directory to the specified path
 * and returns 0 if successful
 */
DWORD WINAPI SheChangeDirA(LPSTR path)
{
    if (SetCurrentDirectoryA(path))
        return 0;
    else
        return GetLastError();
}

/*************************************************************************
 * SheChangeDirW [SHELL32.@]
 *
 * changes the current directory to the specified path
 * and returns 0 if successful
 */
DWORD WINAPI SheChangeDirW(LPWSTR path)
{
    if (SetCurrentDirectoryW(path))
        return 0;
    else
        return GetLastError();
}

/*************************************************************************
 * IsNetDrive			[SHELL32.66]
 */
int WINAPI IsNetDrive(int drive)
{
	char root[4];
	strcpy(root, "A:\\");
	root[0] += (char)drive;
	return (GetDriveTypeA(root) == DRIVE_REMOTE);
}


/*************************************************************************
 * RealDriveType                [SHELL32.524]
 */
int WINAPI RealDriveType(int drive, BOOL bQueryNet)
{
    char root[] = "A:\\";
    root[0] += (char)drive;
    return GetDriveTypeA(root);
}

/***********************************************************************
 *              SHPathPrepareForWriteA (SHELL32.@)
 */
HRESULT WINAPI SHPathPrepareForWriteA(HWND hwnd, IUnknown *modless, LPCSTR path, DWORD flags)
{
    WCHAR wpath[MAX_PATH];
    MultiByteToWideChar( CP_ACP, 0, path, -1, wpath, MAX_PATH);
    return SHPathPrepareForWriteW(hwnd, modless, wpath, flags);
}

/***********************************************************************
 *              SHPathPrepareForWriteW (SHELL32.@)
 */
HRESULT WINAPI SHPathPrepareForWriteW(HWND hwnd, IUnknown *modless, LPCWSTR path, DWORD flags)
{
    DWORD res;
    DWORD err;
    LPCWSTR realpath;
    int len;
    WCHAR* last_slash;
    WCHAR* temppath=NULL;

    TRACE("%p %p %s 0x%08lx\n", hwnd, modless, debugstr_w(path), flags);

    if (flags & ~(SHPPFW_DIRCREATE|SHPPFW_ASKDIRCREATE|SHPPFW_IGNOREFILENAME))
        FIXME("unimplemented flags 0x%08lx\n", flags);

    /* cut off filename if necessary */
    if (flags & SHPPFW_IGNOREFILENAME)
    {
        last_slash = StrRChrW(path, NULL, '\\');
        if (last_slash == NULL)
            len = 1;
        else
            len = last_slash - path + 1;
        temppath = malloc(len * sizeof(WCHAR));
        if (!temppath)
            return E_OUTOFMEMORY;
        StrCpyNW(temppath, path, len);
        realpath = temppath;
    }
    else
    {
        realpath = path;
    }

    /* try to create the directory if asked to */
    if (flags & (SHPPFW_DIRCREATE|SHPPFW_ASKDIRCREATE))
    {
        if (flags & SHPPFW_ASKDIRCREATE)
            FIXME("treating SHPPFW_ASKDIRCREATE as SHPPFW_DIRCREATE\n");

        SHCreateDirectoryExW(0, realpath, NULL);
    }

    /* check if we can access the directory */
    res = GetFileAttributesW(realpath);

    free(temppath);

    if (res == INVALID_FILE_ATTRIBUTES)
    {
        err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND)
            return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
        return HRESULT_FROM_WIN32(err);
    }
    else if (res & FILE_ATTRIBUTE_DIRECTORY)
        return S_OK;
    else
        return HRESULT_FROM_WIN32(ERROR_DIRECTORY);
}

/*************************************************************************
 * SHMultiFileProperties [SHELL32.@]
 */

HRESULT WINAPI SHMultiFileProperties(IDataObject *pdtobj, DWORD flags)
{
    FIXME("stub: %p %lu\n", pdtobj, flags);
    return E_NOTIMPL;
}

enum copy_engine_opcode
{
    COPY_ENGINE_MOVE,
    COPY_ENGINE_REMOVE_DIRECTORY_SILENT,
};

#define TSF_UNKNOWN_MEGRE_FLAG 0x1000

struct copy_engine_operation
{
    struct list entry;
    IFileOperationProgressSink *sink;
    enum copy_engine_opcode opcode;
    IShellItem *folder;
    PIDLIST_ABSOLUTE item_pidl;
    WCHAR *name;
    DWORD tsf;
};

struct file_operation_sink
{
    struct list entry;
    IFileOperationProgressSink *sink;
    DWORD cookie;
};

struct file_operation
{
    IFileOperation IFileOperation_iface;
    LONG ref;
    struct list sinks;
    DWORD next_cookie;
    struct list ops;
    DWORD flags;
    BOOL aborted;
    unsigned int progress_total, progress_sofar;
};

static void free_file_operation_ops(struct file_operation *operation)
{
    struct copy_engine_operation *op, *next;

    LIST_FOR_EACH_ENTRY_SAFE(op, next, &operation->ops, struct copy_engine_operation, entry)
    {
        if (op->sink)
            IFileOperationProgressSink_Release(op->sink);
        ILFree(op->item_pidl);
        if (op->folder)
            IShellItem_Release(op->folder);
        CoTaskMemFree(op->name);
        list_remove(&op->entry);
        free(op);
    }
}

static HRESULT add_operation(struct file_operation *operation, enum copy_engine_opcode opcode, IShellItem *item,
        IShellItem *folder, const WCHAR *name, IFileOperationProgressSink *sink, DWORD tsf, struct list *add_after)
{
    struct copy_engine_operation *op;
    HRESULT hr;

    if (!name)
        name = L"";

    if (!(op = calloc(1, sizeof(*op))))
        return E_OUTOFMEMORY;

    op->opcode = opcode;
    if (item && FAILED((hr = SHGetIDListFromObject((IUnknown *)item, &op->item_pidl))))
    {
        hr = E_INVALIDARG;
        goto error;
    }

    op->tsf = tsf;
    if (folder)
    {
        IShellItem_AddRef(folder);
        op->folder = folder;
    }
    if (!(op->name = wcsdup(name)))
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    if (sink)
    {
        IFileOperationProgressSink_AddRef(sink);
        op->sink = sink;
    }
    if (add_after)
        list_add_after(add_after, &op->entry);
    else
        list_add_tail(&operation->ops, &op->entry);
    return S_OK;

error:
    ILFree(op->item_pidl);
    if (op->folder)
        IShellItem_Release(op->folder);
    if (op->sink)
        IFileOperationProgressSink_Release(sink);
    CoTaskMemFree(op->name);
    free(op);
    return hr;
}

static HRESULT file_operation_notify(struct file_operation *operation, struct copy_engine_operation *op, BOOL post_notif,
        void *context,
        HRESULT (*callback)(IFileOperationProgressSink *, struct file_operation *, struct copy_engine_operation *op, void *))
{
    struct file_operation_sink *op_sink;
    HRESULT hr = S_OK;

    if (op && op->sink && post_notif)
    {
        if (FAILED(hr = callback(op->sink, operation, op, context)))
            goto done;
    }
    LIST_FOR_EACH_ENTRY(op_sink, &operation->sinks, struct file_operation_sink, entry)
    {
        if (FAILED(hr = callback(op_sink->sink, operation, op, context)))
            goto done;
    }
    if (op && op->sink && !post_notif)
        hr = callback(op->sink, operation, op, context);

done:
    if (FAILED(hr))
        WARN("sink returned %#lx.\n", hr);
    return hr;
}

static HRESULT notify_start_operations(IFileOperationProgressSink *sink, struct file_operation *operations,
        struct copy_engine_operation *op, void *context)
{
    return IFileOperationProgressSink_StartOperations(sink);
}

static HRESULT notify_reset_timer(IFileOperationProgressSink *sink, struct file_operation *operations,
        struct copy_engine_operation *op, void *context)
{
    return IFileOperationProgressSink_ResetTimer(sink);
}

static HRESULT notify_finish_operations(IFileOperationProgressSink *sink, struct file_operation *operations,
        struct copy_engine_operation *op, void *context)
{
    return IFileOperationProgressSink_FinishOperations(sink, S_OK);
}

static HRESULT notify_update_progress(IFileOperationProgressSink *sink, struct file_operation *operations,
        struct copy_engine_operation *op, void *context)
{
    return IFileOperationProgressSink_UpdateProgress(sink, operations->progress_total, operations->progress_sofar);
}

struct notify_move_item_param
{
    IShellItem *item;
    IShellItem *new_item;
    HRESULT result;
};

static HRESULT notify_pre_move_item(IFileOperationProgressSink *sink, struct file_operation *operations,
        struct copy_engine_operation *op, void *context)
{
    struct notify_move_item_param *p = context;

    return IFileOperationProgressSink_PreMoveItem(sink, op->tsf & ~TSF_UNKNOWN_MEGRE_FLAG, p->item, op->folder, op->name);
}

static HRESULT notify_post_move_item(IFileOperationProgressSink *sink, struct file_operation *operations,
        struct copy_engine_operation *op, void *context)
{
    struct notify_move_item_param *p = context;

    return IFileOperationProgressSink_PostMoveItem(sink, op->tsf, p->item, op->folder, op->name, p->result, p->new_item);
}

static void set_file_operation_progress(struct file_operation *operation, unsigned int total, unsigned int sofar)
{
    operation->progress_total = total;
    operation->progress_sofar = sofar;
    file_operation_notify(operation, NULL, FALSE, NULL, notify_update_progress);
}

static HRESULT copy_engine_merge_dir(struct file_operation *operation, struct copy_engine_operation *op,
        const WCHAR *src_dir_path, IShellItem *dest_folder, struct list **add_after)
{
    struct list *add_files_after, *curr_add_after;
    WIN32_FIND_DATAW wfd;
    WCHAR path[MAX_PATH];
    IShellItem *item;
    HRESULT hr = S_OK;
    HANDLE fh;

    if (!PathCombineW(path, src_dir_path, L"*.*"))
        return HRESULT_FROM_WIN32(GetLastError());

    fh = FindFirstFileW(path, &wfd);
    if (fh == INVALID_HANDLE_VALUE) return S_OK;
    add_files_after = *add_after;
    do
    {
        if (!wcscmp(wfd.cFileName, L".") || !wcscmp(wfd.cFileName, L".."))
            continue;
        if (!PathCombineW(path, src_dir_path, wfd.cFileName))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }
        if (FAILED(hr = SHCreateItemFromParsingName(path, NULL, &IID_IShellItem, (void **)&item)))
            break;

        /* Queue files before directories. */
        curr_add_after = IsAttribFile(wfd.dwFileAttributes) ? add_files_after : *add_after;
        hr = add_operation(operation, COPY_ENGINE_MOVE, item, dest_folder, NULL, op->sink,
                (op->tsf & ~TSF_COPY_LOCALIZED_NAME) | TSF_UNKNOWN_MEGRE_FLAG, curr_add_after);
        IShellItem_Release(item);
        if (FAILED(hr))
            break;
        if (curr_add_after == *add_after)
            *add_after = (*add_after)->next;
        if (IsAttribFile(wfd.dwFileAttributes))
            add_files_after = add_files_after->next;
    } while (FindNextFileW(fh, &wfd));
    FindClose(fh);
    return hr;
}

static HRESULT copy_engine_move(struct file_operation *operation, struct copy_engine_operation *op, IShellItem *src_item,
        IShellItem **dest_folder)
{
    WCHAR path[MAX_PATH], item_path[MAX_PATH];
    DWORD src_attrs, dst_attrs;
    struct list *add_after;
    WCHAR *str, *ptr;
    HRESULT hr;

    *dest_folder = NULL;
    if (FAILED(hr = IShellItem_GetDisplayName(src_item, SIGDN_FILESYSPATH, &str)))
        return hr;
    wcscpy_s(item_path, ARRAY_SIZE(item_path), str);
    if (!*op->name && (ptr = StrRChrW(str, NULL, '\\')))
    {
        free(op->name);
        op->name = wcsdup(ptr + 1);
    }
    CoTaskMemFree(str);

    if (FAILED(hr = IShellItem_GetDisplayName(op->folder, SIGDN_FILESYSPATH, &str)))
        return hr;
    hr = PathCombineW(path, str, op->name) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
    CoTaskMemFree(str);
    if (FAILED(hr))
        return hr;

    if ((src_attrs = GetFileAttributesW(item_path)) == INVALID_FILE_ATTRIBUTES)
        return HRESULT_FROM_WIN32(GetLastError());
    dst_attrs = GetFileAttributesW(path);
    if (IsAttribFile(src_attrs) && IsAttribDir(dst_attrs))
        return COPYENGINE_E_FILE_IS_FLD_DEST;
    if (IsAttribDir(src_attrs) && IsAttribFile(dst_attrs))
        return COPYENGINE_E_FLD_IS_FILE_DEST;
    if (dst_attrs == INVALID_FILE_ATTRIBUTES || (IsAttribFile(src_attrs) && IsAttribFile(dst_attrs)))
    {
        if (MoveFileExW(item_path, path, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING))
        {
            if (FAILED((hr = SHCreateItemFromParsingName(path, NULL, &IID_IShellItem, (void **)dest_folder))))
                return hr;
            return COPYENGINE_S_DONT_PROCESS_CHILDREN;
        }
        IShellItem_Release(*dest_folder);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    /* Merge directory to existing directory. */
    if (FAILED((hr = SHCreateItemFromParsingName(path, NULL, &IID_IShellItem, (void **)dest_folder))))
        return hr;

    add_after = &op->entry;
    if (FAILED((hr = copy_engine_merge_dir(operation, op, item_path, *dest_folder, &add_after))))
    {
        IShellItem_Release(*dest_folder);
        *dest_folder = NULL;
        return hr;
    }
    add_operation(operation, COPY_ENGINE_REMOVE_DIRECTORY_SILENT, src_item, NULL, NULL, NULL, 0, add_after);

    return COPYENGINE_S_NOT_HANDLED;
}

static HRESULT perform_file_operations(struct file_operation *operation)
{
    struct copy_engine_operation *op;
    HRESULT hr;

    file_operation_notify(operation, NULL, FALSE, NULL, notify_start_operations);
    set_file_operation_progress(operation, 0, 0);
    set_file_operation_progress(operation, list_count(&operation->ops), 0);
    file_operation_notify(operation, NULL, FALSE, NULL, notify_reset_timer);

    LIST_FOR_EACH_ENTRY(op, &operation->ops, struct copy_engine_operation, entry)
    {
        hr = E_FAIL;
        switch (op->opcode)
        {
            case COPY_ENGINE_REMOVE_DIRECTORY_SILENT:
            {
                WCHAR *path;
                if (FAILED(SHGetNameFromIDList(op->item_pidl, SIGDN_FILESYSPATH, &path)))
                {
                    ERR("SHGetNameFromIDList failed.\n");
                    break;
                }
                if (!RemoveDirectoryW(path))
                    WARN("Remove directory failed, err %lu.\n", GetLastError());
                CoTaskMemFree(path);
                break;
            }

            case COPY_ENGINE_MOVE:
            {
                struct notify_move_item_param p;

                p.new_item = NULL;
                if (FAILED(hr = SHCreateItemFromIDList(op->item_pidl, &IID_IShellItem, (void**)&p.item)))
                    break;
                if (FAILED((hr = file_operation_notify(operation, op, FALSE, &p, notify_pre_move_item))))
                {
                    IShellItem_Release(p.item);
                    return hr;
                }
                hr = copy_engine_move(operation, op, p.item, &p.new_item);
                p.result = hr;
                if (FAILED(hr = file_operation_notify(operation, op, TRUE, &p, notify_post_move_item)))
                {
                    if (p.new_item)
                        IShellItem_Release(p.new_item);
                    return hr;
                }
                hr = p.result;
                IShellItem_Release(p.item);
                if (p.new_item)
                    IShellItem_Release(p.new_item);
                break;
            }
        }
        set_file_operation_progress(operation, list_count(&operation->ops), operation->progress_sofar + 1);
        TRACE("op %d, hr %#lx.\n", op->opcode, hr);
        operation->aborted = FAILED(hr) || operation->aborted;
    }
    file_operation_notify(operation, NULL, FALSE, NULL, notify_finish_operations);
    return S_OK;
}

static inline struct file_operation *impl_from_IFileOperation(IFileOperation *iface)
{
    return CONTAINING_RECORD(iface, struct file_operation, IFileOperation_iface);
}

static HRESULT WINAPI file_operation_QueryInterface(IFileOperation *iface, REFIID riid, void **out)
{
    struct file_operation *operation = impl_from_IFileOperation(iface);

    TRACE("(%p, %s, %p).\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(&IID_IFileOperation, riid) ||
        IsEqualIID(&IID_IUnknown, riid))
        *out = &operation->IFileOperation_iface;
    else
    {
        FIXME("not implemented for %s.\n", debugstr_guid(riid));
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI file_operation_AddRef(IFileOperation *iface)
{
    struct file_operation *operation = impl_from_IFileOperation(iface);
    ULONG ref = InterlockedIncrement(&operation->ref);

    TRACE("(%p): ref=%lu.\n", iface, ref);

    return ref;
}

static ULONG WINAPI file_operation_Release(IFileOperation *iface)
{
    struct file_operation *operation = impl_from_IFileOperation(iface);
    ULONG ref = InterlockedDecrement(&operation->ref);

    TRACE("(%p): ref=%lu.\n", iface, ref);

    if (!ref)
    {
        struct file_operation_sink *sink, *next_sink;

        LIST_FOR_EACH_ENTRY_SAFE(sink, next_sink, &operation->sinks, struct file_operation_sink, entry)
        {
            IFileOperationProgressSink_Release(sink->sink);
            list_remove(&sink->entry);
            free(sink);
        }
        free_file_operation_ops(operation);
        free(operation);
    }

    return ref;
}

static HRESULT WINAPI file_operation_Advise(IFileOperation *iface, IFileOperationProgressSink *sink, DWORD *cookie)
{
    struct file_operation *operation = impl_from_IFileOperation(iface);
    struct file_operation_sink *op_sink;

    TRACE("(%p, %p, %p).\n", iface, sink, cookie);

    if (!sink)
        return E_INVALIDARG;
    if (!(op_sink = calloc(1, sizeof(*op_sink))))
        return E_OUTOFMEMORY;

    op_sink->cookie = ++operation->next_cookie;
    IFileOperationProgressSink_AddRef(sink);
    op_sink->sink = sink;
    list_add_tail(&operation->sinks, &op_sink->entry);
    return S_OK;
}

static HRESULT WINAPI file_operation_Unadvise(IFileOperation *iface, DWORD cookie)
{
    struct file_operation *operation = impl_from_IFileOperation(iface);
    struct file_operation_sink *sink;

    TRACE("(%p, %lx).\n", iface, cookie);
    LIST_FOR_EACH_ENTRY(sink, &operation->sinks, struct file_operation_sink, entry)
    {
        if (sink->cookie == cookie)
        {
            IFileOperationProgressSink_Release(sink->sink);
            list_remove(&sink->entry);
            free(sink);
            return S_OK;
        }
    }
    return S_OK;
}

static HRESULT WINAPI file_operation_SetOperationFlags(IFileOperation *iface, DWORD flags)
{
    struct file_operation *operation = impl_from_IFileOperation(iface);

    TRACE("(%p, %lx).\n", iface, flags);

    operation->flags = flags;
    return S_OK;
}

static HRESULT WINAPI file_operation_SetProgressMessage(IFileOperation *iface, LPCWSTR message)
{
    FIXME("(%p, %s): stub.\n", iface, debugstr_w(message));

    return E_NOTIMPL;
}

static HRESULT WINAPI file_operation_SetProgressDialog(IFileOperation *iface, IOperationsProgressDialog *dialog)
{
    FIXME("(%p, %p): stub.\n", iface, dialog);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_operation_SetProperties(IFileOperation *iface, IPropertyChangeArray *array)
{
    FIXME("(%p, %p): stub.\n", iface, array);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_operation_SetOwnerWindow(IFileOperation *iface, HWND owner)
{
    FIXME("(%p, %p): stub.\n", iface, owner);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_operation_ApplyPropertiesToItem(IFileOperation *iface, IShellItem *item)
{
    FIXME("(%p, %p): stub.\n", iface, item);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_operation_ApplyPropertiesToItems(IFileOperation *iface, IUnknown *items)
{
    FIXME("(%p, %p): stub.\n", iface, items);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_operation_RenameItem(IFileOperation *iface, IShellItem *item, LPCWSTR name,
        IFileOperationProgressSink *sink)
{
    FIXME("(%p, %p, %s, %p): stub.\n", iface, item, debugstr_w(name), sink);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_operation_RenameItems(IFileOperation *iface, IUnknown *items, LPCWSTR name)
{
    FIXME("(%p, %p, %s): stub.\n", iface, items, debugstr_w(name));

    return E_NOTIMPL;
}

static HRESULT WINAPI file_operation_MoveItem(IFileOperation *iface, IShellItem *item, IShellItem *folder,
        LPCWSTR name, IFileOperationProgressSink *sink)
{
    TRACE("(%p, %p, %p, %s, %p).\n", iface, item, folder, debugstr_w(name), sink);

    if (!folder || !item)
        return E_INVALIDARG;

    return add_operation(impl_from_IFileOperation(iface), COPY_ENGINE_MOVE, item, folder, name, sink,
            TSF_COPY_LOCALIZED_NAME | TSF_COPY_WRITE_TIME | TSF_COPY_CREATION_TIME | TSF_OVERWRITE_EXIST, NULL);
}

static HRESULT WINAPI file_operation_MoveItems(IFileOperation *iface, IUnknown *items, IShellItem *folder)
{
    FIXME("(%p, %p, %p): stub.\n", iface, items, folder);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_operation_CopyItem(IFileOperation *iface, IShellItem *item, IShellItem *folder,
        LPCWSTR name, IFileOperationProgressSink *sink)
{
    FIXME("(%p, %p, %p, %s, %p): stub.\n", iface, item, folder, debugstr_w(name), sink);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_operation_CopyItems(IFileOperation *iface, IUnknown *items, IShellItem *folder)
{
    FIXME("(%p, %p, %p): stub.\n", iface, items, folder);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_operation_DeleteItem(IFileOperation *iface, IShellItem *item,
        IFileOperationProgressSink *sink)
{
    FIXME("(%p, %p, %p): stub.\n", iface, item, sink);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_operation_DeleteItems(IFileOperation *iface, IUnknown *items)
{
    FIXME("(%p, %p): stub.\n", iface, items);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_operation_NewItem(IFileOperation *iface, IShellItem *folder, DWORD attributes,
        LPCWSTR name, LPCWSTR template, IFileOperationProgressSink *sink)
{
    FIXME("(%p, %p, %lx, %s, %s, %p): stub.\n", iface, folder, attributes,
          debugstr_w(name), debugstr_w(template), sink);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_operation_PerformOperations(IFileOperation *iface)
{
    struct file_operation *operation = impl_from_IFileOperation(iface);
    HRESULT hr;

    TRACE("(%p).\n", iface);

    if (list_empty(&operation->ops))
        return E_UNEXPECTED;

    if (operation->flags != FOF_NO_UI)
        FIXME("Unhandled flags %#lx.\n", operation->flags);
    hr = perform_file_operations(operation);
    free_file_operation_ops(operation);
    return hr;
}

static HRESULT WINAPI file_operation_GetAnyOperationsAborted(IFileOperation *iface, BOOL *aborted)
{
    struct file_operation *operation = impl_from_IFileOperation(iface);

    TRACE("(%p, %p).\n", iface, aborted);

    if (!aborted)
        return E_POINTER;
    *aborted = operation->aborted;
    TRACE("-> aborted %d.\n", *aborted);
    return S_OK;
}

static const IFileOperationVtbl file_operation_vtbl =
{
    file_operation_QueryInterface,
    file_operation_AddRef,
    file_operation_Release,
    file_operation_Advise,
    file_operation_Unadvise,
    file_operation_SetOperationFlags,
    file_operation_SetProgressMessage,
    file_operation_SetProgressDialog,
    file_operation_SetProperties,
    file_operation_SetOwnerWindow,
    file_operation_ApplyPropertiesToItem,
    file_operation_ApplyPropertiesToItems,
    file_operation_RenameItem,
    file_operation_RenameItems,
    file_operation_MoveItem,
    file_operation_MoveItems,
    file_operation_CopyItem,
    file_operation_CopyItems,
    file_operation_DeleteItem,
    file_operation_DeleteItems,
    file_operation_NewItem,
    file_operation_PerformOperations,
    file_operation_GetAnyOperationsAborted
};

HRESULT WINAPI IFileOperation_Constructor(IUnknown *outer, REFIID riid, void **out)
{
    struct file_operation *object;
    HRESULT hr;

    object = calloc(1, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IFileOperation_iface.lpVtbl = &file_operation_vtbl;
    list_init(&object->sinks);
    list_init(&object->ops);
    object->ref = 1;

    hr = IFileOperation_QueryInterface(&object->IFileOperation_iface, riid, out);
    IFileOperation_Release(&object->IFileOperation_iface);

    return hr;
}
