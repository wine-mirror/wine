/*
 * SHFileOperation
 *
 * Copyright 2000 Juergen Schmied
 * Copyright 2002 Andriy Palamarchuk
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <string.h>
#include <ctype.h>

#include "winreg.h"
#include "shellapi.h"
#include "shlobj.h"
#include "shresdef.h"
#include "shell32_main.h"
#include "undocshell.h"
#include "shlwapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define IsAttribFile(x) (!(x == -1) && !(x & FILE_ATTRIBUTE_DIRECTORY))
#define IsAttribDir(x)  (!(x == -1) && (x & FILE_ATTRIBUTE_DIRECTORY))

#define IsDotDir(x)     ((x[0] == '.') && ((x[1] == 0) || ((x[1] == '.') && (x[2] == 0))))

CHAR aWildcardFile[] = {'*','.','*',0};
WCHAR wWildcardFile[] = {'*','.','*',0};
WCHAR wWildcardChars[] = {'*','?',0};
WCHAR wBackslash[] = {'\\',0};

static BOOL SHNotifyCreateDirectoryA(LPCSTR path, LPSECURITY_ATTRIBUTES sec);
static BOOL SHNotifyCreateDirectoryW(LPCWSTR path, LPSECURITY_ATTRIBUTES sec);
static BOOL SHNotifyRemoveDirectoryA(LPCSTR path);
static BOOL SHNotifyRemoveDirectoryW(LPCWSTR path);
static BOOL SHNotifyDeleteFileA(LPCSTR path);
static BOOL SHNotifyDeleteFileW(LPCWSTR path);
static BOOL SHNotifyMoveFileW(LPCWSTR src, LPCWSTR dest);
static BOOL SHNotifyCopyFileW(LPCWSTR src, LPCWSTR dest, BOOL bRenameIfExists);

typedef struct
{
	UINT caption_resource_id, text_resource_id;
} SHELL_ConfirmIDstruc;

static BOOL SHELL_ConfirmIDs(int nKindOfDialog, SHELL_ConfirmIDstruc *ids)
{
	switch (nKindOfDialog) {
	  case ASK_DELETE_FILE:
	    ids->caption_resource_id  = IDS_DELETEITEM_CAPTION;
	    ids->text_resource_id  = IDS_DELETEITEM_TEXT;
	    return TRUE;
	  case ASK_DELETE_FOLDER:
	    ids->caption_resource_id  = IDS_DELETEFOLDER_CAPTION;
	    ids->text_resource_id  = IDS_DELETEITEM_TEXT;
	    return TRUE;
	  case ASK_DELETE_MULTIPLE_ITEM:
	    ids->caption_resource_id  = IDS_DELETEITEM_CAPTION;
	    ids->text_resource_id  = IDS_DELETEMULTIPLE_TEXT;
	    return TRUE;
	  case ASK_OVERWRITE_FILE:
	    ids->caption_resource_id  = IDS_OVERWRITEFILE_CAPTION;
	    ids->text_resource_id  = IDS_OVERWRITEFILE_TEXT;
	    return TRUE;
	  default:
	    FIXME(" Unhandled nKindOfDialog %d stub\n", nKindOfDialog);
	}
	return FALSE;
}

BOOL SHELL_ConfirmDialog(int nKindOfDialog, LPCSTR szDir)
{
	CHAR szCaption[255], szText[255], szBuffer[MAX_PATH + 256];
	SHELL_ConfirmIDstruc ids;

	if (!SHELL_ConfirmIDs(nKindOfDialog, &ids))
	  return FALSE;

	LoadStringA(shell32_hInstance, ids.caption_resource_id, szCaption, sizeof(szCaption));
	LoadStringA(shell32_hInstance, ids.text_resource_id, szText, sizeof(szText));

	FormatMessageA(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
	               szText, 0, 0, szBuffer, sizeof(szBuffer), (va_list*)&szDir);

	return (IDOK == MessageBoxA(GetActiveWindow(), szBuffer, szCaption, MB_OKCANCEL | MB_ICONEXCLAMATION));
}

BOOL SHELL_ConfirmDialogW(int nKindOfDialog, LPCWSTR szDir)
{
	WCHAR szCaption[255], szText[255], szBuffer[MAX_PATH + 256];
	SHELL_ConfirmIDstruc ids;

	if (!SHELL_ConfirmIDs(nKindOfDialog, &ids))
	  return FALSE;

	LoadStringW(shell32_hInstance, ids.caption_resource_id, szCaption, sizeof(szCaption));
	LoadStringW(shell32_hInstance, ids.text_resource_id, szText, sizeof(szText));

	FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
	               szText, 0, 0, szBuffer, sizeof(szBuffer), (va_list*)&szDir);

	return (IDOK == MessageBoxW(GetActiveWindow(), szBuffer, szCaption, MB_OKCANCEL | MB_ICONEXCLAMATION));
}

/**************************************************************************
 * SHELL_DeleteDirectoryA()  [internal]
 *
 * like rm -r
 */
BOOL SHELL_DeleteDirectoryA(LPCSTR pszDir, BOOL bShowUI)
{
	BOOL    ret = TRUE;
	HANDLE  hFind;
	WIN32_FIND_DATAA wfd;
	char    szTemp[MAX_PATH];

	/* Make sure the directory exists before eventually prompting the user */
	PathCombineA(szTemp, pszDir, aWildcardFile);
	hFind = FindFirstFileA(szTemp, &wfd);
	if (hFind == INVALID_HANDLE_VALUE)
	  return FALSE;

	if (!bShowUI || SHELL_ConfirmDialog(ASK_DELETE_FOLDER, pszDir))
	{
	  do
	  {
	    LPSTR lp = wfd.cAlternateFileName;
	    if (!lp[0])
	      lp = wfd.cFileName;
	    if (IsDotDir(lp))
	      continue;
	    PathCombineA(szTemp, pszDir, lp);
	    if (FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes)
	      ret = SHELL_DeleteDirectoryA(szTemp, FALSE);
	    else
	      ret = SHNotifyDeleteFileA(szTemp);
	  } while (ret && FindNextFileA(hFind, &wfd));
	}
	FindClose(hFind);
	if (ret)
	  ret = SHNotifyRemoveDirectoryA(pszDir);
	return ret;
}

BOOL SHELL_DeleteDirectoryW(LPCWSTR pszDir, BOOL bShowUI)
{
	BOOL    ret = TRUE;
	HANDLE  hFind;
	WIN32_FIND_DATAW wfd;
	WCHAR   szTemp[MAX_PATH];

	/* Make sure the directory exists before eventually prompting the user */
	PathCombineW(szTemp, pszDir, wWildcardFile);
	hFind = FindFirstFileW(szTemp, &wfd);
	if (hFind == INVALID_HANDLE_VALUE)
	  return FALSE;

	if (!bShowUI || SHELL_ConfirmDialogW(ASK_DELETE_FOLDER, pszDir))
	{
	  do
	  {
	    LPWSTR lp = wfd.cAlternateFileName;
	    if (!lp[0])
	      lp = wfd.cFileName;
	    if (IsDotDir(lp))
	      continue;
	    PathCombineW(szTemp, pszDir, lp);
	    if (FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes)
	      ret = SHELL_DeleteDirectoryW(szTemp, FALSE);
	    else
	      ret = SHNotifyDeleteFileW(szTemp);
	  } while (ret && FindNextFileW(hFind, &wfd));
	}
	FindClose(hFind);
	if (ret)
	  ret = SHNotifyRemoveDirectoryW(pszDir);
	return ret;
}

/**************************************************************************
 *  SHELL_DeleteFileA()      [internal]
 */
BOOL SHELL_DeleteFileA(LPCSTR pszFile, BOOL bShowUI)
{
	if (bShowUI && !SHELL_ConfirmDialog(ASK_DELETE_FILE, pszFile))
	  return FALSE;

	return SHNotifyDeleteFileA(pszFile);
}

BOOL SHELL_DeleteFileW(LPCWSTR pszFile, BOOL bShowUI)
{
	if (bShowUI && !SHELL_ConfirmDialogW(ASK_DELETE_FILE, pszFile))
	  return FALSE;

	return SHNotifyDeleteFileW(pszFile);
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
 * NOTES:
 *  Verified on Win98 / IE 5 (SHELL32 4.72, March 1999 build) to be ANSI.
 *  This is Unicode on NT/2000
 */

static BOOL SHNotifyCreateDirectoryA(LPCSTR path, LPSECURITY_ATTRIBUTES sec)
{
	BOOL ret;
	TRACE("(%s, %p)\n", debugstr_a(path), sec);

	ret = CreateDirectoryA(path, sec);
	if (ret)
	{
	  SHChangeNotify(SHCNE_MKDIR, SHCNF_PATHA, path, NULL);
	}
	return ret;
}

static BOOL SHNotifyCreateDirectoryW(LPCWSTR path, LPSECURITY_ATTRIBUTES sec)
{
	BOOL ret;
	TRACE("(%s, %p)\n", debugstr_w(path), sec);

	ret = CreateDirectoryW(path, sec);
	if (ret)
	{
	  SHChangeNotify(SHCNE_MKDIR, SHCNF_PATHW, path, NULL);
	}
	return ret;
}

BOOL WINAPI Win32CreateDirectoryAW(LPCVOID path, LPSECURITY_ATTRIBUTES sec)
{
	if (SHELL_OsIsUnicode())
	  return SHNotifyCreateDirectoryW(path, sec);
	return SHNotifyCreateDirectoryA(path, sec);
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
 * NOTES:
 *  Verified on Win98 / IE 5 (SHELL32 4.72, March 1999 build) to be ANSI.
 *  This is Unicode on NT/2000
 */
static BOOL SHNotifyRemoveDirectoryA(LPCSTR path)
{
	BOOL ret;
	TRACE("(%s)\n", debugstr_a(path));

	ret = RemoveDirectoryA(path);
	if (!ret)
	{
	  /* Directory may be write protected */
	  DWORD dwAttr = GetFileAttributesA(path);
	  if (dwAttr != -1 && dwAttr & FILE_ATTRIBUTE_READONLY)
	    if (SetFileAttributesA(path, dwAttr & ~FILE_ATTRIBUTE_READONLY))
	      ret = RemoveDirectoryA(path);
	}
	if (ret)
	  SHChangeNotify(SHCNE_RMDIR, SHCNF_PATHA, path, NULL);
	return ret;
}

static BOOL SHNotifyRemoveDirectoryW(LPCWSTR path)
{
	BOOL ret;
	TRACE("(%s)\n", debugstr_w(path));

	ret = RemoveDirectoryW(path);
	if (!ret)
	{
	  /* Directory may be write protected */
	  DWORD dwAttr = GetFileAttributesW(path);
	  if (dwAttr != -1 && dwAttr & FILE_ATTRIBUTE_READONLY)
	    if (SetFileAttributesW(path, dwAttr & ~FILE_ATTRIBUTE_READONLY))
	      ret = RemoveDirectoryW(path);
	}
	if (ret)
	  SHChangeNotify(SHCNE_RMDIR, SHCNF_PATHW, path, NULL);
	return ret;
}

BOOL WINAPI Win32RemoveDirectoryAW(LPCVOID path)
{
	if (SHELL_OsIsUnicode())
	  return SHNotifyRemoveDirectoryW(path);
	return SHNotifyRemoveDirectoryA(path);
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
 * NOTES:
 *  Verified on Win98 / IE 5 (SHELL32 4.72, March 1999 build) to be ANSI.
 *  This is Unicode on NT/2000
 */
static BOOL SHNotifyDeleteFileA(LPCSTR path)
{
	BOOL ret;

	TRACE("(%s)\n", debugstr_a(path));

	ret = DeleteFileA(path);
	if (!ret)
	{
	  /* File may be write protected or a system file */
	  DWORD dwAttr = GetFileAttributesA(path);
	  if ((dwAttr != -1) && (dwAttr & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
	    if (SetFileAttributesA(path, dwAttr & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
	      ret = DeleteFileA(path);
	}
	if (ret)
	  SHChangeNotify(SHCNE_DELETE, SHCNF_PATHA, path, NULL);
	return ret;
}

static BOOL SHNotifyDeleteFileW(LPCWSTR path)
{
	BOOL ret;

	TRACE("(%s)\n", debugstr_w(path));

	ret = DeleteFileW(path);
	if (!ret)
	{
	  /* File may be write protected or a system file */
	  DWORD dwAttr = GetFileAttributesW(path);
	  if ((dwAttr != -1) && (dwAttr & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
	    if (SetFileAttributesW(path, dwAttr & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
	      ret = DeleteFileW(path);
	}
	if (ret)
	  SHChangeNotify(SHCNE_DELETE, SHCNF_PATHW, path, NULL);
	return ret;
}

DWORD WINAPI Win32DeleteFileAW(LPCVOID path)
{
	if (SHELL_OsIsUnicode())
	  return SHNotifyDeleteFileW(path);
	return SHNotifyDeleteFileA(path);
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
 *  TRUE if successful, FALSE otherwise
 */
static BOOL SHNotifyMoveFileW(LPCWSTR src, LPCWSTR dest)
{
	BOOL ret;

	TRACE("(%s %s)\n", debugstr_w(src), debugstr_w(dest));

	ret = MoveFileW(src, dest);
	if (!ret)
	{
	  /* Source file may be write protected or a system file */
	  DWORD dwAttr = GetFileAttributesW(src);
	  if ((dwAttr != -1) && (dwAttr & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
	    if (SetFileAttributesW(src, dwAttr & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
	      ret = MoveFileW(src, dest);
	}
	if (ret)
	  SHChangeNotify(SHCNE_RENAMEITEM, SHCNF_PATHW, src, dest);
	return ret;
}

/************************************************************************
 * SHNotifyCopyFile          [internal]
 *
 * Copies a file. Also triggers a change notify if one exists.
 *
 * PARAMS
 *  src        [I]   path to source file to move
 *  dest       [I]   path to target file to move to
 *  bRename    [I]   if TRUE, the target file will be renamed if a
 *                   file with this name already exists
 *
 * RETURNS
 *  TRUE if successful, FALSE otherwise
 */
static BOOL SHNotifyCopyFileW(LPCWSTR src, LPCWSTR dest, BOOL bRename)
{
	BOOL ret;

	TRACE("(%s %s %s)\n", debugstr_w(src), debugstr_w(dest), bRename ? "renameIfExists" : "");

	ret = CopyFileW(src, dest, TRUE);
	if (!ret && bRename)
	{
	  /* Destination file probably exists */
	  DWORD dwAttr = GetFileAttributesW(dest);
	  if (dwAttr != -1)
	  {
	    FIXME("Rename on copy to existing file not implemented!");
	  }
	}
	if (ret)
	  SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW, dest, NULL);
	return ret;
}

/*************************************************************************
 * SHCreateDirectory         [SHELL32.165]
 *
 * Create a directory at the specified location
 *
 * PARAMS
 *  hWnd       [I]   
 *  path       [I]   path of directory to create 
 *
 * RETURNS
 *  ERRROR_SUCCESS or one of the following values:
 *  ERROR_BAD_PATHNAME if the path is relative
 *  ERROR_FILE_EXISTS when a file with that name exists
 *  ERROR_ALREADY_EXISTS when the directory already exists
 *  ERROR_FILENAME_EXCED_RANGE if the filename was to long to process
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
 * Create a directory at the specified location
 *
 * PARAMS
 *  hWnd       [I]   
 *  path       [I]   path of directory to create 
 *  sec        [I]   security attributes to use or NULL
 *
 * RETURNS
 *  ERRROR_SUCCESS or one of the following values:
 *  ERROR_BAD_PATHNAME if the path is relative
 *  ERROR_FILE_EXISTS when a file with that name exists
 *  ERROR_ALREADY_EXISTS when the directory already exists
 *  ERROR_FILENAME_EXCED_RANGE if the filename was to long to process
 */
DWORD WINAPI SHCreateDirectoryExA(HWND hWnd, LPCSTR path, LPSECURITY_ATTRIBUTES sec)
{
	WCHAR wPath[MAX_PATH];
	TRACE("(%p, %s, %p)\n",hWnd, debugstr_a(path), sec);

	MultiByteToWideChar(CP_ACP, 0, path, -1, wPath, MAX_PATH);
	return SHCreateDirectoryExW(hWnd, wPath, sec);
}

/*************************************************************************
 * SHCreateDirectoryExW      [SHELL32.@]
 */
DWORD WINAPI SHCreateDirectoryExW(HWND hWnd, LPCWSTR path, LPSECURITY_ATTRIBUTES sec)
{
	DWORD ret = ERROR_SUCCESS;
	TRACE("(%p, %s, %p)\n",hWnd, debugstr_w(path), sec);

	if (PathIsRelativeW(path))
	{
	  ret = ERROR_BAD_PATHNAME;
	  SetLastError(ERROR_BAD_PATHNAME);
	}
	else
	{
	  if (!SHNotifyCreateDirectoryW(path, sec))
	  {
	    ret = GetLastError();
	    if (ret != ERROR_FILE_EXISTS &&
	        ret != ERROR_ALREADY_EXISTS &&
	        ret != ERROR_FILENAME_EXCED_RANGE)
	    {
	    /* handling network file names?
	      lstrcpynW(pathName, path, MAX_PATH);
	      lpStr = PathAddBackslashW(pathName);*/
	      FIXME("Semi-stub, non zero hWnd should be used somehow?");
	    }
	  }
	}
	return ret;
}

/**************************************************************************
 *	SHELL_FileNamesMatch()
 *
 * Accepts two \0 delimited lists of the file names. Checks whether number of
 * files in the both lists is the same.
 */
BOOL SHELL_FileNamesMatch(LPCSTR pszFiles1, LPCSTR pszFiles2)
{
    while ((pszFiles1[strlen(pszFiles1) + 1] != '\0') &&
           (pszFiles2[strlen(pszFiles2) + 1] != '\0'))
    {
        pszFiles1 += strlen(pszFiles1) + 1;
        pszFiles2 += strlen(pszFiles2) + 1;
    }

    return
        ((pszFiles1[strlen(pszFiles1) + 1] == '\0') &&
         (pszFiles2[strlen(pszFiles2) + 1] == '\0')) ||
        ((pszFiles1[strlen(pszFiles1) + 1] != '\0') &&
         (pszFiles2[strlen(pszFiles2) + 1] != '\0'));
}

/*************************************************************************
 * SHFileOperationA				[SHELL32.@]
 *
 * NOTES
 *     exported by name
 */
DWORD WINAPI SHFileOperationA (LPSHFILEOPSTRUCTA lpFileOp)
{
	LPSTR pFrom = (LPSTR)lpFileOp->pFrom;
	LPSTR pTo = (LPSTR)lpFileOp->pTo;
	LPSTR pTempTo;
        TRACE("flags (0x%04x) : %s%s%s%s%s%s%s%s%s%s%s%s \n", lpFileOp->fFlags,
                lpFileOp->fFlags & FOF_MULTIDESTFILES ? "FOF_MULTIDESTFILES " : "",
                lpFileOp->fFlags & FOF_CONFIRMMOUSE ? "FOF_CONFIRMMOUSE " : "",
                lpFileOp->fFlags & FOF_SILENT ? "FOF_SILENT " : "",
                lpFileOp->fFlags & FOF_RENAMEONCOLLISION ? "FOF_RENAMEONCOLLISION " : "",
                lpFileOp->fFlags & FOF_NOCONFIRMATION ? "FOF_NOCONFIRMATION " : "",
                lpFileOp->fFlags & FOF_WANTMAPPINGHANDLE ? "FOF_WANTMAPPINGHANDLE " : "",
                lpFileOp->fFlags & FOF_ALLOWUNDO ? "FOF_ALLOWUNDO " : "",
                lpFileOp->fFlags & FOF_FILESONLY ? "FOF_FILESONLY " : "",
                lpFileOp->fFlags & FOF_SIMPLEPROGRESS ? "FOF_SIMPLEPROGRESS " : "",
                lpFileOp->fFlags & FOF_NOCONFIRMMKDIR ? "FOF_NOCONFIRMMKDIR " : "",
                lpFileOp->fFlags & FOF_NOERRORUI ? "FOF_NOERRORUI " : "",
                lpFileOp->fFlags & 0xf800 ? "MORE-UNKNOWN-Flags" : "");
	switch(lpFileOp->wFunc) {
	case FO_COPY:
        case FO_MOVE:
        {
                /* establish when pTo is interpreted as the name of the destination file
                 * or the directory where the Fromfile should be copied to.
                 * This depends on:
                 * (1) pTo points to the name of an existing directory;
                 * (2) the flag FOF_MULTIDESTFILES is present;
                 * (3) whether pFrom point to multiple filenames.
                 *
                 * Some experiments:
                 *
                 * destisdir               1 1 1 1 0 0 0 0
                 * FOF_MULTIDESTFILES      1 1 0 0 1 1 0 0
                 * multiple from filenames 1 0 1 0 1 0 1 0
                 *                         ---------------
                 * copy files to dir       1 0 1 1 0 0 1 0
                 * create dir              0 0 0 0 0 0 1 0
                 */
                int multifrom = pFrom[strlen(pFrom) + 1] != '\0';
                int destisdir = PathIsDirectoryA( pTo );
                int todir = 0;

                if (lpFileOp->wFunc == FO_COPY)
                    TRACE("File Copy:\n");
                else
                    TRACE("File Move:\n");

                if( destisdir ) {
                    if ( !((lpFileOp->fFlags & FOF_MULTIDESTFILES) && !multifrom))
                        todir = 1;
                } else {
                    if ( !(lpFileOp->fFlags & FOF_MULTIDESTFILES) && multifrom)
                        todir = 1;
                }

                if ((pTo[strlen(pTo) + 1] != '\0') &&
                    !(lpFileOp->fFlags & FOF_MULTIDESTFILES))
                {
                    WARN("Attempt to use multiple file names as a destination "
                         "without specifying FOF_MULTIDESTFILES\n");
                    return 1;
                }

                if ((lpFileOp->fFlags & FOF_MULTIDESTFILES) &&
                    !SHELL_FileNamesMatch(pTo, pFrom))
                {
                    WARN("Attempt to use multiple file names as a destination "
                         "with mismatching number of files in the source and "
                         "destination lists\n");
                    return 1;
                }

                if ( todir ) {
                    char szTempFrom[MAX_PATH];
                    char *fromfile;
                    int lenPTo;
                    if ( ! destisdir) {
                      TRACE("   creating directory %s\n",pTo);
                      SHCreateDirectoryExA(NULL, pTo, NULL);
                    }
                    lenPTo = strlen(pTo);
                    while(1) {
		        HANDLE hFind;
		        WIN32_FIND_DATAA wfd;

                        if(!pFrom[0]) break;
                        TRACE("   From Pattern='%s'\n", pFrom);
			if(INVALID_HANDLE_VALUE != (hFind = FindFirstFileA(pFrom, &wfd)))
			{
			  do
			  {
			    if(strcasecmp(wfd.cFileName, ".") && strcasecmp(wfd.cFileName, ".."))
			    {
			      strcpy(szTempFrom, pFrom);

                              pTempTo = HeapAlloc(GetProcessHeap(), 0,
                                                  lenPTo + strlen(wfd.cFileName) + 5);
                              if (pTempTo) {
                                  strcpy(pTempTo,pTo);
                                  PathAddBackslashA(pTempTo);
                                  strcat(pTempTo,wfd.cFileName);

                                  fromfile = PathFindFileNameA(szTempFrom);
                                  fromfile[0] = '\0';
                                  PathAddBackslashA(szTempFrom);
                                  strcat(szTempFrom, wfd.cFileName);
                                  TRACE("   From='%s' To='%s'\n", szTempFrom, pTempTo);
                                  if(lpFileOp->wFunc == FO_COPY)
                                  {
                                      if(FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes)
                                      {
                                          /* copy recursively */
                                          if(!(lpFileOp->fFlags & FOF_FILESONLY))
                                          {
                                              SHFILEOPSTRUCTA shfo;

                                              SHCreateDirectoryExA(NULL, pTempTo, NULL);
                                              PathAddBackslashA(szTempFrom);
                                              strcat(szTempFrom, "*.*");
                                              szTempFrom[strlen(szTempFrom) + 1] = '\0';
                                              pTempTo[strlen(pTempTo) + 1] = '\0';
                                              memcpy(&shfo, lpFileOp, sizeof(shfo));
                                              shfo.pFrom = szTempFrom;
                                              shfo.pTo = pTempTo;
                                              SHFileOperationA(&shfo);

                                              szTempFrom[strlen(szTempFrom) - 4] = '\0';
                                          }
                                      }
                                      else
                                          CopyFileA(szTempFrom, pTempTo, FALSE);
                                  }
                                  else
                                  {
                                      /* move file/directory */
                                      MoveFileA(szTempFrom, pTempTo);
                                  }
                                  HeapFree(GetProcessHeap(), 0, pTempTo);
                              }
			    }
			  } while(FindNextFileA(hFind, &wfd));
			  FindClose(hFind);
                        }
                        else
                        {
                            /* can't find file with specified name */
                            break;
                        }
                        pFrom += strlen(pFrom) + 1;
                    }
                } else {
                    while (1) {
                            if(!pFrom[0]) break;
                            if(!pTo[0]) break;
                            TRACE("   From='%s' To='%s'\n", pFrom, pTo);

                            pTempTo = HeapAlloc(GetProcessHeap(), 0, strlen(pTo)+1);
                            if (pTempTo)
                            {
                                strcpy( pTempTo, pTo );
                                PathRemoveFileSpecA(pTempTo);
                                TRACE("   Creating Directory '%s'\n", pTempTo);
                                SHCreateDirectoryExA(NULL, pTempTo, NULL);
                                HeapFree(GetProcessHeap(), 0, pTempTo);
                            }
                            if (lpFileOp->wFunc == FO_COPY)
                                CopyFileA(pFrom, pTo, FALSE);
                            else
                                MoveFileA(pFrom, pTo);

                            pFrom += strlen(pFrom) + 1;
                            pTo += strlen(pTo) + 1;
                    }
                }
		TRACE("Setting AnyOpsAborted=FALSE\n");
		lpFileOp->fAnyOperationsAborted=FALSE;
		return 0;
        }

	case FO_DELETE:
	{
		HANDLE		hFind;
		WIN32_FIND_DATAA wfd;
		char		szTemp[MAX_PATH];
		char		*file_name;

		TRACE("File Delete:\n");
		while(1) {
			if(!pFrom[0]) break;
			TRACE("   Pattern='%s'\n", pFrom);
			if(INVALID_HANDLE_VALUE != (hFind = FindFirstFileA(pFrom, &wfd)))
			{
			  do
			  {
			    if(strcasecmp(wfd.cFileName, ".") && strcasecmp(wfd.cFileName, ".."))
			    {
			      strcpy(szTemp, pFrom);
			      file_name = PathFindFileNameA(szTemp);
			      file_name[0] = '\0';
			      PathAddBackslashA(szTemp);
			      strcat(szTemp, wfd.cFileName);

			      TRACE("   File='%s'\n", szTemp);
			      if(FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes)
			      {
			        if(!(lpFileOp->fFlags & FOF_FILESONLY))
			            SHELL_DeleteDirectoryA(szTemp, FALSE);
			      }
			      else
			        DeleteFileA(szTemp);
			    }
			  } while(FindNextFileA(hFind, &wfd));

			  FindClose(hFind);
			}
			pFrom += strlen(pFrom) + 1;
		}
		TRACE("Setting AnyOpsAborted=FALSE\n");
		lpFileOp->fAnyOperationsAborted=FALSE;
		return 0;
	}

        case FO_RENAME:
            TRACE("File Rename:\n");
            if (pFrom[strlen(pFrom) + 1] != '\0')
            {
                WARN("Attempt to rename more than one file\n");
                return 1;
            }
            lpFileOp->fAnyOperationsAborted = FALSE;
            TRACE("From %s, To %s\n", pFrom, pTo);
            return !MoveFileA(pFrom, pTo);

	default:
		FIXME("Unhandled shell file operation %d\n", lpFileOp->wFunc);
	}

	return 1;
}

/*************************************************************************
 * SHFileOperationW				[SHELL32.@]
 *
 * NOTES
 *     exported by name
 */
DWORD WINAPI SHFileOperationW (LPSHFILEOPSTRUCTW lpFileOp)
{
	FIXME("(%p):stub.\n", lpFileOp);
	return 1;
}

/*************************************************************************
 * SHFileOperation				[SHELL32.@]
 *
 */
DWORD WINAPI SHFileOperationAW(LPVOID lpFileOp)
{
	if (SHELL_OsIsUnicode())
	  return SHFileOperationW(lpFileOp);
	return SHFileOperationA(lpFileOp);
}

/*************************************************************************
 * SheGetDirW [SHELL32.281]
 *
 */
HRESULT WINAPI SheGetDirW(LPWSTR u, LPWSTR v)
{	FIXME("%p %p stub\n",u,v);
	return 0;
}

/*************************************************************************
 * SheChangeDirW [SHELL32.274]
 *
 */
HRESULT WINAPI SheChangeDirW(LPWSTR u)
{	FIXME("(%s),stub\n",debugstr_w(u));
	return 0;
}

/*************************************************************************
 * IsNetDrive			[SHELL32.66]
 */
BOOL WINAPI IsNetDrive(DWORD drive)
{
	char root[4];
	strcpy(root, "A:\\");
	root[0] += (char)drive;
	return (GetDriveTypeA(root) == DRIVE_REMOTE);
}
