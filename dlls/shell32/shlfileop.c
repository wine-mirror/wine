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

#include "winreg.h"
#include "shellapi.h"
#include "shlobj.h"
#include "shresdef.h"
#include "shell32_main.h"
#include "undocshell.h"
#include "shlwapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

BOOL SHELL_ConfirmDialog (int nKindOfDialog, LPCSTR szDir)
{
	char szCaption[255], szText[255], szBuffer[MAX_PATH + 256];
	UINT caption_resource_id, text_resource_id;

	switch(nKindOfDialog) {

	case ASK_DELETE_FILE:
	  caption_resource_id	= IDS_DELETEITEM_CAPTION;
	  text_resource_id	= IDS_DELETEITEM_TEXT;
	  break;
	case ASK_DELETE_FOLDER:
	  caption_resource_id	= IDS_DELETEFOLDER_CAPTION;
	  text_resource_id	= IDS_DELETEITEM_TEXT;
	  break;
	case ASK_DELETE_MULTIPLE_ITEM:
	  caption_resource_id	= IDS_DELETEITEM_CAPTION;
	  text_resource_id	= IDS_DELETEMULTIPLE_TEXT;
	  break;
	case ASK_OVERWRITE_FILE:
	  caption_resource_id	= IDS_OVERWRITEFILE_CAPTION;
	  text_resource_id	= IDS_OVERWRITEFILE_TEXT;
	  break;
	default:
	  FIXME(" Unhandled nKindOfDialog %d stub\n", nKindOfDialog);
	  return FALSE;
	}

	LoadStringA(shell32_hInstance, caption_resource_id, szCaption, sizeof(szCaption));
	LoadStringA(shell32_hInstance, text_resource_id, szText, sizeof(szText));

	FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
	               szText, 0, 0, szBuffer, sizeof(szBuffer), (va_list*)&szDir);

	return (IDOK == MessageBoxA(GetActiveWindow(), szBuffer, szCaption, MB_OKCANCEL | MB_ICONEXCLAMATION));
}

/**************************************************************************
 *	SHELL_DeleteDirectoryA()
 *
 * like rm -r
 */

BOOL SHELL_DeleteDirectoryA(LPCSTR pszDir, BOOL bShowUI)
{
	BOOL		ret = FALSE;
	HANDLE		hFind;
	WIN32_FIND_DATAA wfd;
	char		szTemp[MAX_PATH];

	strcpy(szTemp, pszDir);
	PathAddBackslashA(szTemp);
	strcat(szTemp, "*.*");

	if (bShowUI && !SHELL_ConfirmDialog(ASK_DELETE_FOLDER, pszDir))
	  return FALSE;

	if(INVALID_HANDLE_VALUE != (hFind = FindFirstFileA(szTemp, &wfd)))
	{
	  do
	  {
	    if(strcasecmp(wfd.cFileName, ".") && strcasecmp(wfd.cFileName, ".."))
	    {
	      strcpy(szTemp, pszDir);
	      PathAddBackslashA(szTemp);
	      strcat(szTemp, wfd.cFileName);

	      if(FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes)
	        SHELL_DeleteDirectoryA(szTemp, FALSE);
	      else
	        DeleteFileA(szTemp);
	    }
	  } while(FindNextFileA(hFind, &wfd));

	  FindClose(hFind);
	  ret = RemoveDirectoryA(pszDir);
	}

	return ret;
}

/**************************************************************************
 *	SHELL_DeleteFileA()
 */

BOOL SHELL_DeleteFileA(LPCSTR pszFile, BOOL bShowUI)
{
	if (bShowUI && !SHELL_ConfirmDialog(ASK_DELETE_FILE, pszFile))
		return FALSE;

        return DeleteFileA(pszFile);
}

/*************************************************************************
 * SHCreateDirectory                       [SHELL32.165]
 *
 * NOTES
 *  exported by ordinal
 *  WinNT/2000 exports Unicode
 */
DWORD WINAPI SHCreateDirectory(HWND hWnd, LPCVOID path)
{
	if (SHELL_OsIsUnicode())
	  return SHCreateDirectoryExW(hWnd, path, NULL);
	return SHCreateDirectoryExA(hWnd, path, NULL);
}

/*************************************************************************
 * SHCreateDirectoryExA                     [SHELL32.@]
 */
DWORD WINAPI SHCreateDirectoryExA(HWND hWnd, LPCSTR path, LPSECURITY_ATTRIBUTES sec)
{
	DWORD ret;
	TRACE("(%p, %s, %p)\n",hWnd, path, sec);
	if ((ret = CreateDirectoryA(path, sec)))
	{
	  SHChangeNotify(SHCNE_MKDIR, SHCNF_PATHA, path, NULL);
	}
	else if (hWnd)
	  FIXME("Semi-stub, non zero hWnd should be used as parent for error dialog!");
	return ret;
}

/*************************************************************************
 * SHCreateDirectoryExW                     [SHELL32.@]
 */
DWORD WINAPI SHCreateDirectoryExW(HWND hWnd, LPCWSTR path, LPSECURITY_ATTRIBUTES sec)
{
	DWORD ret;
	TRACE("(%p, %s, %p)\n",hWnd, debugstr_w(path), sec);
	if ((ret = CreateDirectoryW(path, sec)))
	{
	  SHChangeNotify(SHCNE_MKDIR, SHCNF_PATHW, path, NULL);
	}
	else if (hWnd)
	  FIXME("Semi-stub, non zero hWnd should be used as parent for error dialog!");
	return ret;
}

/************************************************************************
 * Win32DeleteFile                         [SHELL32.164]
 *
 * Deletes a file. Also triggers a change notify if one exists.
 *
 * NOTES
 *  Verified on Win98 / IE 5 (SHELL32 4.72, March 1999 build) to be ANSI.
 *  This is Unicode on NT/2000
 */
static BOOL Win32DeleteFileA(LPCSTR fName)
{
	TRACE("%p(%s)\n", fName, fName);

	DeleteFileA(fName);
	SHChangeNotify(SHCNE_DELETE, SHCNF_PATHA, fName, NULL);
	return TRUE;
}

static BOOL Win32DeleteFileW(LPCWSTR fName)
{
	TRACE("%p(%s)\n", fName, debugstr_w(fName));

	DeleteFileW(fName);
	SHChangeNotify(SHCNE_DELETE, SHCNF_PATHW, fName, NULL);
	return TRUE;
}

DWORD WINAPI Win32DeleteFileAW(LPCVOID fName)
{
	if (SHELL_OsIsUnicode())
	  return Win32DeleteFileW(fName);
	return Win32DeleteFileA(fName);
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
