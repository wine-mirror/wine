/*
 * SHFileOperation
 *
 * Copyright 2000 Juergen Schmied
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

BOOL SHELL_WarnItemDelete (int nKindOfDialog, LPCSTR szDir)
{
	char szCaption[255], szText[255], szBuffer[MAX_PATH + 256];

        if(nKindOfDialog == ASK_DELETE_FILE)
        {
	  LoadStringA(shell32_hInstance, IDS_DELETEITEM_TEXT, szText,
		sizeof(szText));
	  LoadStringA(shell32_hInstance, IDS_DELETEITEM_CAPTION,
		szCaption, sizeof(szCaption));
	}
        else if(nKindOfDialog == ASK_DELETE_FOLDER)
        {
	  LoadStringA(shell32_hInstance, IDS_DELETEITEM_TEXT, szText,
		sizeof(szText));
	  LoadStringA(shell32_hInstance, IDS_DELETEFOLDER_CAPTION,
		szCaption, sizeof(szCaption));
        }
        else if(nKindOfDialog == ASK_DELETE_MULTIPLE_ITEM)
        {
	  LoadStringA(shell32_hInstance, IDS_DELETEMULTIPLE_TEXT, szText,
		sizeof(szText));
	  LoadStringA(shell32_hInstance, IDS_DELETEITEM_CAPTION,
		szCaption, sizeof(szCaption));
        }
	else {
          FIXME("Called without a valid nKindOfDialog specified!\n");
	  LoadStringA(shell32_hInstance, IDS_DELETEITEM_TEXT, szText,
		sizeof(szText));
	  LoadStringA(shell32_hInstance, IDS_DELETEITEM_CAPTION,
		szCaption, sizeof(szCaption));
	}

	FormatMessageA(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
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

	if (bShowUI && !SHELL_WarnItemDelete(ASK_DELETE_FOLDER, pszDir))
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
	if (bShowUI && !SHELL_WarnItemDelete(ASK_DELETE_FILE, pszFile))
		return FALSE;

        return DeleteFileA(pszFile);
}

/*************************************************************************
 * SHCreateDirectory				[SHELL32.165]
 *
 * NOTES
 *  exported by ordinal
 *  not sure about LPSECURITY_ATTRIBUTES
 */
DWORD WINAPI SHCreateDirectory(LPSECURITY_ATTRIBUTES sec,LPCSTR path)
{
	DWORD ret;
	TRACE("(%p,%s)\n",sec,path);
	if ((ret = CreateDirectoryA(path,sec)))
	{
	  SHChangeNotifyA(SHCNE_MKDIR, SHCNF_PATHA, path, NULL);
	}
	return ret;
}

/************************************************************************
 *      Win32DeleteFile                         [SHELL32.164]
 *
 * Deletes a file.  Also triggers a change notify if one exists.
 *
 * FIXME:
 * Verified on Win98 / IE 5 (SHELL32 4.72, March 1999 build) to be
 * ANSI.  Is this Unicode on NT?
 *
 */

BOOL WINAPI Win32DeleteFile(LPSTR fName)
{
	TRACE("%p(%s)\n", fName, fName);

	DeleteFileA(fName);
	SHChangeNotifyA(SHCNE_DELETE, SHCNF_PATHA, fName, NULL);
	return TRUE;
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
	case FO_COPY: {
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
                int copytodir = 0;
		TRACE("File Copy:\n");
                if( destisdir ) {
                    if ( !((lpFileOp->fFlags & FOF_MULTIDESTFILES) && !multifrom))
                        copytodir = 1;
                } else {
                    if ( !(lpFileOp->fFlags & FOF_MULTIDESTFILES) && multifrom)
                        copytodir = 1;
                }
                if ( copytodir ) {
                    char *fromfile;
                    int lenPTo;
                    if ( ! destisdir) {
                        TRACE("   creating directory %s\n",pTo);
                        SHCreateDirectory(NULL,pTo);
                    }
                    lenPTo = strlen(pTo);
                    while(1) {
                        if(!pFrom[0]) break;
                        fromfile = PathFindFileNameA( pFrom);
                        pTempTo = HeapAlloc(GetProcessHeap(), 0, lenPTo + strlen(fromfile) + 2);
                        if (pTempTo) {
                            strcpy(pTempTo,pTo);
                            if(lenPTo && pTo[lenPTo] != '\\')
                                strcat(pTempTo,"\\");
                            strcat(pTempTo,fromfile);
                            TRACE("   From='%s' To='%s'\n", pFrom, pTempTo);
                            CopyFileA(pFrom, pTempTo, FALSE);
                            HeapFree(GetProcessHeap(), 0, pTempTo);
                        }
                        pFrom += strlen(pFrom) + 1;
                    }
                } else {
                    while(1) {
                            if(!pFrom[0]) break;
                            if(!pTo[0]) break;
                            TRACE("   From='%s' To='%s'\n", pFrom, pTo);

                            pTempTo = HeapAlloc(GetProcessHeap(), 0, strlen(pTo)+1);
                            if (pTempTo)
                            {
                                strcpy( pTempTo, pTo );
                                PathRemoveFileSpecA(pTempTo);
                                TRACE("   Creating Directory '%s'\n", pTempTo);
                                SHCreateDirectory(NULL,pTempTo);
                                HeapFree(GetProcessHeap(), 0, pTempTo);
                            }
                            CopyFileA(pFrom, pTo, FALSE);

                            pFrom += strlen(pFrom) + 1;
                            pTo += strlen(pTo) + 1;
                    }
                }
		TRACE("Setting AnyOpsAborted=FALSE\n");
		lpFileOp->fAnyOperationsAborted=FALSE;
		return 0;
        }

	case FO_DELETE:
		TRACE("File Delete:\n");
		while(1) {
			if(!pFrom[0]) break;
			TRACE("   File='%s'\n", pFrom);
			DeleteFileA(pFrom);
			pFrom += strlen(pFrom) + 1;
		}
		TRACE("Setting AnyOpsAborted=FALSE\n");
		lpFileOp->fAnyOperationsAborted=FALSE;
		return 0;

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
	root[0] += drive;
	return (GetDriveTypeA(root) == DRIVE_REMOTE);
}
