/*
 * The parameters of many functions changes between different OS versions
 * (NT uses Unicode strings, 95 uses ASCII strings)
 * 
 * Copyright 1997 Marcus Meissner
 *           1998 Jürgen Schmied
 */
#include <string.h>
#include <stdio.h>
#include "winerror.h"
#include "winreg.h"
#include "debugtools.h"
#include "winnls.h"
#include "heap.h"

#include "shellapi.h"
#include "shlguid.h"
#include "shlobj.h"
#include "shell32_main.h"
#include "wine/undocshell.h"

DEFAULT_DEBUG_CHANNEL(shell);

/*************************************************************************
 * ParseFieldA					[internal]
 *
 * copys a field from a ',' delimited string
 * 
 * first field is nField = 1
 */
DWORD WINAPI ParseFieldA(
	LPCSTR src,
	DWORD nField,
	LPSTR dst,
	DWORD len) 
{
	WARN("('%s',0x%08lx,%p,%ld) semi-stub.\n",src,nField,dst,len);

	if (!src || !src[0] || !dst || !len)
	  return 0;

	/* skip n fields delimited by ',' */
	while (nField > 1)
	{
	  if (*src=='\0') return FALSE;
	  if (*(src++)==',') nField--;
	}

	/* copy part till the next ',' to dst */
	while ( *src!='\0' && *src!=',' && (len--)>0 ) *(dst++)=*(src++);
	
	/* finalize the string */
	*dst=0x0;
	
	return TRUE;
}

/*************************************************************************
 * ParseFieldW			[internal]
 *
 * copys a field from a ',' delimited string
 * 
 * first field is nField = 1
 */
DWORD WINAPI ParseFieldW(LPCWSTR src, DWORD nField, LPWSTR dst, DWORD len) 
{
	FIXME("('%s',0x%08lx,%p,%ld) stub.\n",
	  debugstr_w(src), nField, dst, len);
	return FALSE;
}

/*************************************************************************
 * ParseFieldAW			[SHELL32.58]
 */
DWORD WINAPI ParseFieldAW(LPCVOID src, DWORD nField, LPVOID dst, DWORD len) 
{
	if (SHELL_OsIsUnicode())
	  return ParseFieldW(src, nField, dst, len);
	return ParseFieldA(src, nField, dst, len);
}

/*************************************************************************
 * GetFileNameFromBrowse			[SHELL32.63]
 * 
 */
BOOL WINAPI GetFileNameFromBrowse(
	HWND hwndOwner,
	LPSTR lpstrFile,
	DWORD nMaxFile,
	LPCSTR lpstrInitialDir,
	LPCSTR lpstrDefExt,
	LPCSTR lpstrFilter,
	LPCSTR lpstrTitle)
{
	FIXME("(%04x,%s,%ld,%s,%s,%s,%s):stub.\n",
	  hwndOwner, lpstrFile, nMaxFile, lpstrInitialDir, lpstrDefExt,
	  lpstrFilter, lpstrTitle);

    /* puts up a Open Dialog and requests input into targetbuf */
    /* OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_FILEMUSTEXIST|OFN_unknown */
    strcpy(lpstrFile,"x:\\dummy.exe");
    return 1;
}

/*************************************************************************
 * SHGetSetSettings				[SHELL32.68]
 */
VOID WINAPI SHGetSetSettings(DWORD x, DWORD y, DWORD z)
{
	FIXME("0x%08lx 0x%08lx 0x%08lx\n", x, y, z);
}

/*************************************************************************
 * SHGetSettings				[SHELL32.@]
 * 
 * NOTES
 *  the registry path are for win98 (tested)
 *  and possibly are the same in nt40
 *
 */
VOID WINAPI SHGetSettings(LPSHELLFLAGSTATE lpsfs, DWORD dwMask)
{
	HKEY	hKey;
	DWORD	dwData;
	DWORD	dwDataSize = sizeof (DWORD);

	TRACE("(%p 0x%08lx)\n",lpsfs,dwMask);
	
	if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
				 0, 0, 0, KEY_ALL_ACCESS, 0, &hKey, 0))
	  return;
	
	if ( (SSF_SHOWEXTENSIONS & dwMask) && !RegQueryValueExA(hKey, "HideFileExt", 0, 0, (LPBYTE)&dwData, &dwDataSize))
	  lpsfs->fShowExtensions  = ((dwData == 0) ?  0 : 1);

	if ( (SSF_SHOWINFOTIP & dwMask) && !RegQueryValueExA(hKey, "ShowInfoTip", 0, 0, (LPBYTE)&dwData, &dwDataSize))
	  lpsfs->fShowInfoTip  = ((dwData == 0) ?  0 : 1);

	if ( (SSF_DONTPRETTYPATH & dwMask) && !RegQueryValueExA(hKey, "DontPrettyPath", 0, 0, (LPBYTE)&dwData, &dwDataSize))
	  lpsfs->fDontPrettyPath  = ((dwData == 0) ?  0 : 1);

	if ( (SSF_HIDEICONS & dwMask) && !RegQueryValueExA(hKey, "HideIcons", 0, 0, (LPBYTE)&dwData, &dwDataSize))
	  lpsfs->fHideIcons  = ((dwData == 0) ?  0 : 1);

	if ( (SSF_MAPNETDRVBUTTON & dwMask) && !RegQueryValueExA(hKey, "MapNetDrvBtn", 0, 0, (LPBYTE)&dwData, &dwDataSize))
	  lpsfs->fMapNetDrvBtn  = ((dwData == 0) ?  0 : 1);

	if ( (SSF_SHOWATTRIBCOL & dwMask) && !RegQueryValueExA(hKey, "ShowAttribCol", 0, 0, (LPBYTE)&dwData, &dwDataSize))
	  lpsfs->fShowAttribCol  = ((dwData == 0) ?  0 : 1);

	if (((SSF_SHOWALLOBJECTS | SSF_SHOWSYSFILES) & dwMask) && !RegQueryValueExA(hKey, "Hidden", 0, 0, (LPBYTE)&dwData, &dwDataSize))
	{ if (dwData == 0)
	  { if (SSF_SHOWALLOBJECTS & dwMask)	lpsfs->fShowAllObjects  = 0;
	    if (SSF_SHOWSYSFILES & dwMask)	lpsfs->fShowSysFiles  = 0;
	  }
	  else if (dwData == 1)
	  { if (SSF_SHOWALLOBJECTS & dwMask)	lpsfs->fShowAllObjects  = 1;
	    if (SSF_SHOWSYSFILES & dwMask)	lpsfs->fShowSysFiles  = 0;
	  }
	  else if (dwData == 2)
	  { if (SSF_SHOWALLOBJECTS & dwMask)	lpsfs->fShowAllObjects  = 0;
	    if (SSF_SHOWSYSFILES & dwMask)	lpsfs->fShowSysFiles  = 1;
	  }
	}
	RegCloseKey (hKey);

	TRACE("-- 0x%04x\n", *(WORD*)lpsfs);
}

/*************************************************************************
 * SHShellFolderView_Message			[SHELL32.73]
 *
 * PARAMETERS
 *  hwndCabinet defines the explorer cabinet window that contains the 
 *              shellview you need to communicate with
 *  uMsg        identifying the SFVM enum to perform
 *  lParam
 *
 * NOTES
 *  Message SFVM_REARRANGE = 1
 *    This message gets sent when a column gets clicked to instruct the
 *    shell view to re-sort the item list. lParam identifies the column
 *    that was clicked.
 */
int WINAPI SHShellFolderView_Message(
	HWND hwndCabinet, 
	DWORD dwMessage,
	DWORD dwParam)
{
	FIXME("%04x %08lx %08lx stub\n",hwndCabinet, dwMessage, dwParam);
	return 0;
}

/*************************************************************************
 * RegisterShellHook				[SHELL32.181]
 *
 * PARAMS
 *      hwnd [I]  window handle
 *      y    [I]  flag ????
 * 
 * NOTES
 *     exported by ordinal
 */
BOOL WINAPI RegisterShellHook(
	HWND hWnd,
	DWORD dwType)
{
	FIXME("(0x%08x,0x%08lx):stub.\n",hWnd, dwType);
	return TRUE;
}
/*************************************************************************
 * ShellMessageBoxW				[SHELL32.182]
 *
 * Format and output errormessage.
 *
 * idText	resource ID of title or LPSTR
 * idTitle	resource ID of title or LPSTR
 *
 * NOTES
 *     exported by ordinal
 */
int WINAPIV ShellMessageBoxW(
	HINSTANCE hInstance,
	HWND hWnd,
	LPCWSTR lpText,
	LPCWSTR lpCaption,
	UINT uType,
	...)
{
	WCHAR	szText[100],szTitle[100];
	LPCWSTR pszText = szText, pszTitle = szTitle, pszTemp;
	va_list args;
	int	ret;

	va_start(args, uType);
	/* wvsprintfA(buf,fmt, args); */

	TRACE("(%08lx,%08lx,%p,%p,%08x)\n",
	(DWORD)hInstance,(DWORD)hWnd,lpText,lpCaption,uType);

	if (!HIWORD(lpCaption))
	  LoadStringW(hInstance, (DWORD)lpCaption, szTitle, sizeof(szTitle)/sizeof(szTitle[0]));
	else
	  pszTitle = lpCaption;

	if (!HIWORD(lpText))
	  LoadStringW(hInstance, (DWORD)lpText, szText, sizeof(szText)/sizeof(szText[0]));
	else
	  pszText = lpText;

	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING, 
		       pszText, 0, 0, (LPWSTR)&pszTemp, 0, &args);

	va_end(args);

	ret = MessageBoxW(hWnd,pszTemp,pszTitle,uType);
	LocalFree((HLOCAL)pszTemp);
	return ret;
}

/*************************************************************************
 * ShellMessageBoxA				[SHELL32.183]
 */
int WINAPIV ShellMessageBoxA(
	HINSTANCE hInstance,
	HWND hWnd,
	LPCSTR lpText,
	LPCSTR lpCaption,
	UINT uType,
	...)
{
	char	szText[100],szTitle[100];
	LPCSTR  pszText = szText, pszTitle = szTitle, pszTemp;
	va_list args;
	int	ret;

	va_start(args, uType);
	/* wvsprintfA(buf,fmt, args); */

	TRACE("(%08lx,%08lx,%p,%p,%08x)\n",
	(DWORD)hInstance,(DWORD)hWnd,lpText,lpCaption,uType);

	if (!HIWORD(lpCaption))
	  LoadStringA(hInstance, (DWORD)lpCaption, szTitle, sizeof(szTitle));
	else
	  pszTitle = lpCaption;

	if (!HIWORD(lpText))
	  LoadStringA(hInstance, (DWORD)lpText, szText, sizeof(szText));
	else
	  pszText = lpText;

	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING, 
		       pszText, 0, 0, (LPSTR)&pszTemp, 0, &args);

	va_end(args);

	ret = MessageBoxA(hWnd,pszTemp,pszTitle,uType);
	LocalFree((HLOCAL)pszTemp);
	return ret;
}

/*************************************************************************
 * SHFree					[SHELL32.195]
 *
 * NOTES
 *     free_ptr() - frees memory using IMalloc
 *     exported by ordinal
 */
#define MEM_DEBUG 0
void WINAPI SHFree(LPVOID x) 
{
#if MEM_DEBUG
	WORD len = *(LPWORD)((LPBYTE)x-2);

	if ( *(LPWORD)((LPBYTE)x+len) != 0x7384)
	  ERR("MAGIC2!\n");

	if ( (*(LPWORD)((LPBYTE)x-4)) != 0x8271)
	  ERR("MAGIC1!\n");
	else
	  memset((LPBYTE)x-4, 0xde, len+6);

	TRACE("%p len=%u\n",x, len);

	x = (LPBYTE) x - 4;
#else
	TRACE("%p\n",x);
#endif
	HeapFree(GetProcessHeap(), 0, x);
}

/*************************************************************************
 * SHAlloc					[SHELL32.196]
 *
 * NOTES
 *     void *task_alloc(DWORD len), uses SHMalloc allocator
 *     exported by ordinal
 */
LPVOID WINAPI SHAlloc(DWORD len) 
{
	LPBYTE ret;

#if MEM_DEBUG
	ret = (LPVOID) HeapAlloc(GetProcessHeap(),0,len+6);
#else
	ret = (LPVOID) HeapAlloc(GetProcessHeap(),0,len);
#endif

#if MEM_DEBUG
	*(LPWORD)(ret) = 0x8271;
	*(LPWORD)(ret+2) = (WORD)len;
	*(LPWORD)(ret+4+len) = 0x7384;
	ret += 4;
	memset(ret, 0xdf, len);
#endif
	TRACE("%lu bytes at %p\n",len, ret);
	return (LPVOID)ret;
}

/*************************************************************************
 * SHRegisterDragDrop				[SHELL32.86]
 *
 * NOTES
 *     exported by ordinal
 */
HRESULT WINAPI SHRegisterDragDrop(
	HWND hWnd,
	LPDROPTARGET pDropTarget)
{
	FIXME("(0x%08x,%p):stub.\n", hWnd, pDropTarget);
	if (GetShellOle()) return pRegisterDragDrop(hWnd, pDropTarget);
        return 0;
}

/*************************************************************************
 * SHRevokeDragDrop				[SHELL32.87]
 *
 * NOTES
 *     exported by ordinal
 */
HRESULT WINAPI SHRevokeDragDrop(HWND hWnd)
{
    FIXME("(0x%08x):stub.\n",hWnd);
    return 0;
}

/*************************************************************************
 * SHDoDragDrop					[SHELL32.88]
 *
 * NOTES
 *     exported by ordinal
 */
HRESULT WINAPI SHDoDragDrop(
	HWND hWnd,
	LPDATAOBJECT lpDataObject,
	LPDROPSOURCE lpDropSource,
	DWORD dwOKEffect,
	LPDWORD pdwEffect)
{
    FIXME("(0x%04x %p %p 0x%08lx %p):stub.\n",
    hWnd, lpDataObject, lpDropSource, dwOKEffect, pdwEffect);
    return 0;
}

/*************************************************************************
 * ArrangeWindows				[SHELL32.184]
 * 
 */
WORD WINAPI ArrangeWindows(
	HWND hwndParent,
	DWORD dwReserved,
	LPCRECT lpRect,
	WORD cKids,
	CONST HWND * lpKids)
{
    FIXME("(0x%08x 0x%08lx %p 0x%04x %p):stub.\n",
	   hwndParent, dwReserved, lpRect, cKids, lpKids);
    return 0;
}

/*************************************************************************
 * SignalFileOpen				[SHELL32.103]
 *
 * NOTES
 *     exported by ordinal
 */
DWORD WINAPI
SignalFileOpen (DWORD dwParam1)
{
    FIXME("(0x%08lx):stub.\n", dwParam1);

    return 0;
}

/*************************************************************************
 * SHAddToRecentDocs				[SHELL32.234]
 *
 * PARAMETERS
 *   uFlags  [IN] SHARD_PATH or SHARD_PIDL
 *   pv      [IN] string or pidl, NULL clears the list
 *
 * NOTES
 *     exported by name
 */
DWORD WINAPI SHAddToRecentDocs (UINT uFlags,LPCVOID pv)   
{ if (SHARD_PIDL==uFlags)
  { FIXME("(0x%08x,pidl=%p):stub.\n", uFlags,pv);
	}
	else
	{ FIXME("(0x%08x,%s):stub.\n", uFlags,(char*)pv);
	}
  return 0;
}
/*************************************************************************
 * SHCreateShellFolderViewEx			[SHELL32.174]
 *
 * NOTES
 *  see IShellFolder::CreateViewObject
 */
HRESULT WINAPI SHCreateShellFolderViewEx(
	LPCSHELLFOLDERVIEWINFO psvcbi, /* [in] shelltemplate struct */
	LPSHELLVIEW* ppv)              /* [out] IShellView pointer */
{
	IShellView * psf;
	HRESULT hRes;
	
	TRACE("sf=%p pidl=%p cb=%p mode=0x%08x parm=0x%08lx\n", 
	  psvcbi->pshf, psvcbi->pidlFolder, psvcbi->lpfnCallback,
	  psvcbi->uViewMode, psvcbi->dwUser);

	psf = IShellView_Constructor(psvcbi->pshf);
	
	if (!psf)
	  return E_OUTOFMEMORY;

	IShellView_AddRef(psf);
	hRes = IShellView_QueryInterface(psf, &IID_IShellView, (LPVOID *)ppv);
	IShellView_Release(psf);

	return hRes;
}
/*************************************************************************
 *  SHWinHelp					[SHELL32.127]
 *
 */
HRESULT WINAPI SHWinHelp (DWORD v, DWORD w, DWORD x, DWORD z)
{	FIXME("0x%08lx 0x%08lx 0x%08lx 0x%08lx stub\n",v,w,x,z);
	return 0;
}
/*************************************************************************
 *  SHRunControlPanel [SHELL32.161]
 *
 */
HRESULT WINAPI SHRunControlPanel (DWORD x, DWORD z)
{	FIXME("0x%08lx 0x%08lx stub\n",x,z);
	return 0;
}
/*************************************************************************
 * ShellExecuteEx				[SHELL32.291]
 *
 */
BOOL WINAPI ShellExecuteExAW (LPVOID sei)
{	if (SHELL_OsIsUnicode())
	  return ShellExecuteExW (sei);
	return ShellExecuteExA (sei);
}
/*************************************************************************
 * ShellExecuteExA				[SHELL32.292]
 *
 * placeholder in the commandline:
 *	%1 file
 *	%2 printer
 *	%3 driver
 *	%4 port
 *	%I adress of a global item ID (explorer switch /idlist)
 *	%L ??? path/url/current file ???
 *	%S ???
 *	%* all following parameters (see batfile)
 */
BOOL WINAPI ShellExecuteExA (LPSHELLEXECUTEINFOA sei)
{ 	CHAR szApplicationName[MAX_PATH],szCommandline[MAX_PATH],szPidl[20];
	LPSTR pos;
	int gap, len;
	STARTUPINFOA  startup;
	PROCESS_INFORMATION info;
			
	WARN("mask=0x%08lx hwnd=0x%04x verb=%s file=%s parm=%s dir=%s show=0x%08x class=%s incomplete\n",
		sei->fMask, sei->hwnd, sei->lpVerb, sei->lpFile,
		sei->lpParameters, sei->lpDirectory, sei->nShow, 
		(sei->fMask & SEE_MASK_CLASSNAME) ? sei->lpClass : "not used");

	ZeroMemory(szApplicationName,MAX_PATH);
	if (sei->lpFile)
	  strcpy(szApplicationName, sei->lpFile);
	
	ZeroMemory(szCommandline,MAX_PATH);
	if (sei->lpParameters)
	  strcpy(szCommandline, sei->lpParameters);
			
	if (sei->fMask & (SEE_MASK_CLASSKEY | SEE_MASK_INVOKEIDLIST | SEE_MASK_ICON | SEE_MASK_HOTKEY |
			  SEE_MASK_CONNECTNETDRV | SEE_MASK_FLAG_DDEWAIT |
			  SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_NO_UI | SEE_MASK_UNICODE | 
			  SEE_MASK_NO_CONSOLE | SEE_MASK_ASYNCOK | SEE_MASK_HMONITOR ))
	{
	  FIXME("flags ignored: 0x%08lx\n", sei->fMask);
	}
	
	/* launch a document by fileclass like 'Wordpad.Document.1' */
	if (sei->fMask & SEE_MASK_CLASSNAME)
	{
	  /* the commandline contains 'c:\Path\wordpad.exe "%1"' */
	  HCR_GetExecuteCommand(sei->lpClass, (sei->lpVerb) ? sei->lpVerb : "open", szCommandline, 256);
	  /* fixme: get the extension of lpFile, check if it fits to the lpClass */
	  TRACE("SEE_MASK_CLASSNAME->'%s'\n", szCommandline);
	}

	/* process the IDList */
	if ( (sei->fMask & SEE_MASK_INVOKEIDLIST) == SEE_MASK_INVOKEIDLIST) /*0x0c*/
	{
	  SHGetPathFromIDListA (sei->lpIDList,szApplicationName);
	  TRACE("-- idlist=%p (%s)\n", sei->lpIDList, szApplicationName);
	}
	else
	{
	  if (sei->fMask & SEE_MASK_IDLIST )
	  {
	    pos = strstr(szCommandline, "%I");
	    if (pos)
	    {
	      LPVOID pv;
	      HGLOBAL hmem = SHAllocShared ( sei->lpIDList, ILGetSize(sei->lpIDList), 0);
	      pv = SHLockShared(hmem,0);
	      sprintf(szPidl,":%p",pv );
	      SHUnlockShared(pv);
	    
	      gap = strlen(szPidl);
	      len = strlen(pos)-2;
	      memmove(pos+gap,pos+2,len);
	      memcpy(pos,szPidl,gap);

	    }
	  }
	}

	TRACE("execute:'%s','%s'\n",szApplicationName, szCommandline);

	strcat(szApplicationName, " ");
	strcat(szApplicationName, szCommandline);

	ZeroMemory(&startup,sizeof(STARTUPINFOA));
	startup.cb = sizeof(STARTUPINFOA);

	if (! CreateProcessA(NULL, szApplicationName,
			 NULL, NULL, FALSE, 0, 
			 NULL, NULL, &startup, &info))
	{
	  sei->hInstApp = GetLastError();
	  return FALSE;
	}

        sei->hInstApp = 33;
	
    	/* Give 30 seconds to the app to come up */
	if ( WaitForInputIdle ( info.hProcess, 30000 ) ==  0xFFFFFFFF )
	  ERR("WaitForInputIdle failed: Error %ld\n", GetLastError() );
 
	if(sei->fMask & SEE_MASK_NOCLOSEPROCESS)
	  sei->hProcess = info.hProcess;	  
        else
          CloseHandle( info.hProcess );
        CloseHandle( info.hThread );
	return TRUE;
}
/*************************************************************************
 * ShellExecuteExW				[SHELL32.293]
 *
 */
BOOL WINAPI ShellExecuteExW (LPSHELLEXECUTEINFOW sei)
{	SHELLEXECUTEINFOA seiA;
	DWORD ret;

	TRACE("%p\n", sei);

	memcpy(&seiA, sei, sizeof(SHELLEXECUTEINFOA));
	
        if (sei->lpVerb)
	  seiA.lpVerb = HEAP_strdupWtoA( GetProcessHeap(), 0, sei->lpVerb);

        if (sei->lpFile)
	  seiA.lpFile = HEAP_strdupWtoA( GetProcessHeap(), 0, sei->lpFile);

        if (sei->lpParameters)
	  seiA.lpParameters = HEAP_strdupWtoA( GetProcessHeap(), 0, sei->lpParameters);

	if (sei->lpDirectory)
	  seiA.lpDirectory = HEAP_strdupWtoA( GetProcessHeap(), 0, sei->lpDirectory);

        if ((sei->fMask & SEE_MASK_CLASSNAME) && sei->lpClass)
	  seiA.lpClass = HEAP_strdupWtoA( GetProcessHeap(), 0, sei->lpClass);
	else
	  seiA.lpClass = NULL;
	  	  
	ret = ShellExecuteExA(&seiA);

        if (seiA.lpVerb)	HeapFree( GetProcessHeap(), 0, (LPSTR) seiA.lpVerb );
	if (seiA.lpFile)	HeapFree( GetProcessHeap(), 0, (LPSTR) seiA.lpFile );
	if (seiA.lpParameters)	HeapFree( GetProcessHeap(), 0, (LPSTR) seiA.lpParameters );
	if (seiA.lpDirectory)	HeapFree( GetProcessHeap(), 0, (LPSTR) seiA.lpDirectory );
	if (seiA.lpClass)	HeapFree( GetProcessHeap(), 0, (LPSTR) seiA.lpClass );

 	return ret;
}

static LPUNKNOWN SHELL32_IExplorerInterface=0;
/*************************************************************************
 * SHSetInstanceExplorer			[SHELL32.176]
 *
 * NOTES
 *  Sets the interface
 */
HRESULT WINAPI SHSetInstanceExplorer (LPUNKNOWN lpUnknown)
{	TRACE("%p\n", lpUnknown);
	SHELL32_IExplorerInterface = lpUnknown;
	return (HRESULT) lpUnknown;
}
/*************************************************************************
 * SHGetInstanceExplorer			[SHELL32.256]
 *
 * NOTES
 *  gets the interface pointer of the explorer and a reference
 */
HRESULT WINAPI SHGetInstanceExplorer (LPUNKNOWN * lpUnknown)
{	TRACE("%p\n", lpUnknown);

	*lpUnknown = SHELL32_IExplorerInterface;

	if (!SHELL32_IExplorerInterface)
	  return E_FAIL;

	IUnknown_AddRef(SHELL32_IExplorerInterface);
	return NOERROR;
}
/*************************************************************************
 * SHFreeUnusedLibraries			[SHELL32.123]
 *
 * NOTES
 *  exported by name
 */
void WINAPI SHFreeUnusedLibraries (void)
{
	FIXME("stub\n");
}
/*************************************************************************
 * DAD_SetDragImage				[SHELL32.136]
 *
 * NOTES
 *  exported by name
 */
BOOL WINAPI DAD_SetDragImage(
	HIMAGELIST himlTrack,
	LPPOINT lppt)
{
	FIXME("%p %p stub\n",himlTrack, lppt);
  return 0;
}
/*************************************************************************
 * DAD_ShowDragImage				[SHELL32.137]
 *
 * NOTES
 *  exported by name
 */
BOOL WINAPI DAD_ShowDragImage(BOOL bShow)
{
	FIXME("0x%08x stub\n",bShow);
	return 0;
}
/*************************************************************************
 * ReadCabinetState				[NT 4.0:SHELL32.651]
 *
 */
HRESULT WINAPI ReadCabinetState(DWORD u, DWORD v)
{	FIXME("0x%04lx 0x%04lx stub\n",u,v);
	return 0;
}
/*************************************************************************
 * WriteCabinetState				[NT 4.0:SHELL32.652]
 *
 */
HRESULT WINAPI WriteCabinetState(DWORD u)
{	FIXME("0x%04lx stub\n",u);
	return 0;
}
/*************************************************************************
 * FileIconInit 				[SHELL32.660]
 *
 */
BOOL WINAPI FileIconInit(BOOL bFullInit)
{	FIXME("(%s)\n", bFullInit ? "true" : "false");
	return 0;
}
/*************************************************************************
 * IsUserAdmin					[NT 4.0:SHELL32.680]
 *
 */
HRESULT WINAPI IsUserAdmin(void)
{	FIXME("stub\n");
	return TRUE;
}

/*************************************************************************
 * SHAllocShared				[SHELL32.520]
 *
 * NOTES
 *  parameter1 is return value from HeapAlloc
 *  parameter2 is equal to the size allocated with HeapAlloc
 *  parameter3 is return value from GetCurrentProcessId
 *
 *  the return value is posted as lParam with 0x402 (WM_USER+2) to somewhere
 *  WM_USER+2 could be the undocumented CWM_SETPATH
 *  the allocated memory contains a pidl
 */
HGLOBAL WINAPI SHAllocShared(LPVOID psrc, DWORD size, DWORD procID)
{	HGLOBAL hmem;
	LPVOID pmem;
	
	TRACE("ptr=%p size=0x%04lx procID=0x%04lx\n",psrc,size,procID);
	hmem = GlobalAlloc(GMEM_FIXED, size);
	if (!hmem)
	  return 0;
	
	pmem =  GlobalLock (hmem);

	if (! pmem)
	  return 0;
	  
	memcpy (pmem, psrc, size);
	GlobalUnlock(hmem); 
	return hmem;
}
/*************************************************************************
 * SHLockShared					[SHELL32.521]
 *
 * NOTES
 *  parameter1 is return value from SHAllocShared
 *  parameter2 is return value from GetCurrentProcessId
 *  the receiver of (WM_USER+2) trys to lock the HANDLE (?) 
 *  the returnvalue seems to be a memoryadress
 */
LPVOID WINAPI SHLockShared(HANDLE hmem, DWORD procID)
{	TRACE("handle=0x%04x procID=0x%04lx\n",hmem,procID);
	return GlobalLock(hmem);
}
/*************************************************************************
 * SHUnlockShared				[SHELL32.522]
 *
 * NOTES
 *  parameter1 is return value from SHLockShared
 */
BOOL WINAPI SHUnlockShared(LPVOID pv)
{
	TRACE("%p\n",pv);
	return GlobalUnlock((HANDLE)pv); 
}
/*************************************************************************
 * SHFreeShared					[SHELL32.523]
 *
 * NOTES
 *  parameter1 is return value from SHAllocShared
 *  parameter2 is return value from GetCurrentProcessId
 */
BOOL WINAPI SHFreeShared(
	HANDLE hMem,
	DWORD pid)
{
	TRACE("handle=0x%04x 0x%04lx\n",hMem,pid);
	return GlobalFree(hMem);
}

/*************************************************************************
 * SetAppStartingCursor				[SHELL32.99]
 */
HRESULT WINAPI SetAppStartingCursor(HWND u, DWORD v)
{	FIXME("hwnd=0x%04x 0x%04lx stub\n",u,v );
	return 0;
}
/*************************************************************************
 * SHLoadOLE					[SHELL32.151]
 *
 */
HRESULT WINAPI SHLoadOLE(DWORD u)
{	FIXME("0x%04lx stub\n",u);
	return S_OK;
}
/*************************************************************************
 * DriveType					[SHELL32.64]
 *
 */
HRESULT WINAPI DriveType(DWORD u)
{	FIXME("0x%04lx stub\n",u);
	return 0;
}
/*************************************************************************
 * SHAbortInvokeCommand				[SHELL32.198]
 *
 */
HRESULT WINAPI SHAbortInvokeCommand(void)
{	FIXME("stub\n");
	return 1;
}
/*************************************************************************
 * SHOutOfMemoryMessageBox			[SHELL32.126]
 *
 */
int WINAPI SHOutOfMemoryMessageBox(
	HWND hwndOwner,
	LPCSTR lpCaption,
	UINT uType)
{
	FIXME("0x%04x %s 0x%08x stub\n",hwndOwner, lpCaption, uType);
	return 0;
}
/*************************************************************************
 * SHFlushClipboard				[SHELL32.121]
 *
 */
HRESULT WINAPI SHFlushClipboard(void)
{	FIXME("stub\n");
	return 1;
}

/*************************************************************************
 * SHWaitForFileToOpen				[SHELL32.97]
 *
 */
BOOL WINAPI SHWaitForFileToOpen(
	LPCITEMIDLIST pidl, 
	DWORD dwFlags,
	DWORD dwTimeout)
{
	FIXME("%p 0x%08lx 0x%08lx stub\n", pidl, dwFlags, dwTimeout);
	return 0;
}

/************************************************************************
 *	shell32_654				[SHELL32.654]
 *
 * NOTES: first parameter seems to be a pointer (same as passed to WriteCabinetState)
 * second one could be a size (0x0c). The size is the same as the structure saved to
 * HCU\Software\Microsoft\Windows\CurrentVersion\Explorer\CabinetState
 * I'm (js) guessing: this one is just ReadCabinetState ;-)
 */
HRESULT WINAPI shell32_654 (DWORD x, DWORD y)
{	FIXME("0x%08lx 0x%08lx stub\n",x,y);
	return 0;
}

/************************************************************************
 *	RLBuildListOfPaths			[SHELL32.146]
 *
 * NOTES
 *   builds a DPA
 */
DWORD WINAPI RLBuildListOfPaths (void)
{	FIXME("stub\n");
	return 0;
}
/************************************************************************
 *	SHValidateUNC				[SHELL32.173]
 *
 */
HRESULT WINAPI SHValidateUNC (DWORD x, DWORD y, DWORD z)
{
	FIXME("0x%08lx 0x%08lx 0x%08lx stub\n",x,y,z);
	return 0;
}

/************************************************************************
 *	DoEnvironmentSubstA			[SHELL32.1222]
 *
 */
HRESULT WINAPI DoEnvironmentSubstA(LPSTR x, LPSTR y)
{
	FIXME("(%s, %s) stub\n", debugstr_a(x), debugstr_a(y));
	return 0;
}

/************************************************************************
 *	DoEnvironmentSubstW			[SHELL32.1223]
 *
 */
HRESULT WINAPI DoEnvironmentSubstW(LPWSTR x, LPWSTR y)
{
	FIXME("(%s, %s): stub\n", debugstr_w(x), debugstr_w(y));
	return 0;
}

/************************************************************************
 *	DoEnvironmentSubst			[SHELL32.53]
 *
 */
HRESULT WINAPI DoEnvironmentSubstAW(LPVOID x, LPVOID y)
{
	if (SHELL_OsIsUnicode())
	  return DoEnvironmentSubstW(x, y);
	return DoEnvironmentSubstA(x, y);
}

/*************************************************************************
 *      shell32_243                             [SHELL32.243]
 * 
 * Win98+ by-ordinal routine.  In Win98 this routine returns zero and
 * does nothing else.  Possibly this does something in NT or SHELL32 5.0?
 *
 */

BOOL WINAPI shell32_243(DWORD a, DWORD b) 
{ 
  return FALSE; 
}

/*************************************************************************
 *      SHELL32_714	[SHELL32]
 */
DWORD WINAPI SHELL32_714(LPVOID x)
{
 	FIXME("(%s)stub\n", debugstr_w(x));
	return 0;
}

/*************************************************************************
 *      SHAddFromPropSheetExtArray	[SHELL32]
 */
DWORD WINAPI SHAddFromPropSheetExtArray(DWORD a, DWORD b, DWORD c)
{
 	FIXME("(%08lx,%08lx,%08lx)stub\n", a, b, c);
	return 0;
}

/*************************************************************************
 *      SHCreatePropSheetExtArray	[SHELL32]
 */
DWORD WINAPI SHCreatePropSheetExtArray(DWORD a, LPCSTR b, DWORD c)
{
 	FIXME("(%08lx,%s,%08lx)stub\n", a, debugstr_a(b), c);
	return 0;
}

/*************************************************************************
 *      SHReplaceFromPropSheetExtArray	[SHELL]
 */
DWORD WINAPI SHReplaceFromPropSheetExtArray(DWORD a, DWORD b, DWORD c, DWORD d)
{
 	FIXME("(%08lx,%08lx,%08lx,%08lx)stub\n", a, b, c, d);
	return 0;
}

/*************************************************************************
 *      SHDestroyPropSheetExtArray	[SHELL32]
 */
DWORD WINAPI SHDestroyPropSheetExtArray(DWORD a)
{
 	FIXME("(%08lx)stub\n", a);
	return 0;
}
