/*
 * SHFileOperation
 */
#include <string.h>
#include "debugtools.h"
#include "shellapi.h"
#include "shell32_main.h"
#include "winversion.h"

#include "shlobj.h"
#include "shresdef.h"
#include "wine/undocshell.h"

DEFAULT_DEBUG_CHANNEL(shell);

#define ASK_DELETE_FILE 1
#define ASK_DELETE_FOLDER 2
#define ASK_DELETE_MULTIPLE_FILE 3

static BOOL SHELL_WarnFolderDelete (int nKindOfDialog, LPCSTR szDir)
{
	char szCaption[255], szText[255], szBuffer[256];

	LoadStringA(shell32_hInstance, IDS_DELETEFOLDER_TEXT, szText, sizeof(szText));
	LoadStringA(shell32_hInstance, IDS_DELETEFOLDER_CAPTION, szCaption, sizeof(szCaption));
	FormatMessageA(FORMAT_MESSAGE_FROM_STRING, szText, 0,0, szBuffer, sizeof(szBuffer), (DWORD*)&szDir);

	return (IDOK == MessageBoxA(GetActiveWindow(),szText, szCaption, MB_OKCANCEL | MB_ICONEXCLAMATION));
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
	
	if (bShowUI && !SHELL_WarnFolderDelete(ASK_DELETE_FOLDER, pszDir)) return FALSE;
	
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
 * SHFileOperationA				[SHELL32.243]
 *
 * NOTES
 *     exported by name
 */
DWORD WINAPI SHFileOperationA (LPSHFILEOPSTRUCTA lpFileOp)   
{
	FIXME("(%p):stub.\n", lpFileOp);
	return 1;
}

/*************************************************************************
 * SHFileOperationW				[SHELL32.244]
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
 * SHFileOperation				[SHELL32.242]
 *
 */
DWORD WINAPI SHFileOperationAW(LPVOID lpFileOp)
{
	if (VERSION_OsIsUnicode())
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

