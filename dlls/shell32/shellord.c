/*
 * The parameters of many functions changes between different OS versions
 * (NT uses Unicode strings, 95 uses ASCII strings)
 * 
 * Copyright 1997 Marcus Meissner
 *           1998 Jürgen Schmied
 */
#include <string.h>
#include "winerror.h"
#include "winreg.h"
#include "debug.h"
#include "winnls.h"
#include "winversion.h"
#include "heap.h"

#include "shlobj.h"
#include "shell32_main.h"

/*************************************************************************
 * SHChangeNotifyRegister			[SHELL32.2]
 *
 * NOTES
 *   Idlist is an array of structures and Count specifies how many items in the array
 *   (usually just one I think).
 */
DWORD WINAPI
SHChangeNotifyRegister(
    HWND hwnd,
    LONG events1,
    LONG events2,
    DWORD msg,
    int count,
    IDSTRUCT *idlist)
{	FIXME(shell,"(0x%04x,0x%08lx,0x%08lx,0x%08lx,0x%08x,%p):stub.\n",
		hwnd,events1,events2,msg,count,idlist);
	return 0;
}
/*************************************************************************
 * SHChangeNotifyDeregister			[SHELL32.4]
 */
DWORD WINAPI
SHChangeNotifyDeregister(LONG x1)
{	FIXME(shell,"(0x%08lx):stub.\n",x1);
	return 0;
}
/*************************************************************************
 * NTSHChangeNotifyRegister			[SHELL32.640]
 * NOTES
 *   Idlist is an array of structures and Count specifies how many items in the array
 *   (usually just one I think).
 */
DWORD WINAPI NTSHChangeNotifyRegister(
    HWND hwnd,
    LONG events1,
    LONG events2,
    DWORD msg,
    int count,
    IDSTRUCT *idlist)
{	FIXME(shell,"(0x%04x,0x%08lx,0x%08lx,0x%08lx,0x%08x,%p):stub.\n",
		hwnd,events1,events2,msg,count,idlist);
	return 0;
}
/*************************************************************************
 * NTSHChangeNotifyDeregister			[SHELL32.641]
 */
DWORD WINAPI NTSHChangeNotifyDeregister(LONG x1)
{	FIXME(shell,"(0x%08lx):stub.\n",x1);
	return 0;
}

/*************************************************************************
 * ParseField					[SHELL32.58]
 *
 */
DWORD WINAPI ParseFieldA(LPCSTR src, DWORD field, LPSTR dst, DWORD len) 
{	WARN(shell,"('%s',0x%08lx,%p,%ld) semi-stub.\n",src,field,dst,len);

	if (!src || !src[0] || !dst || !len)
	  return 0;

	if (field >1)
	{ field--;	
	  while (field)
	  { if (*src==0x0) return FALSE;
	    if (*src==',') field--;
	    src++;
	  }
	}

	while (*src!=0x00 && *src!=',' && len>0) 
	{ *dst=*src; dst++, src++; len--;
	}
	*dst=0x0;
	
	return TRUE;
}

/*************************************************************************
 * PickIconDlg					[SHELL32.62]
 * 
 */
DWORD WINAPI PickIconDlg(DWORD x,DWORD y,DWORD z,DWORD a) 
{	FIXME(shell,"(%08lx,%08lx,%08lx,%08lx):stub.\n",x,y,z,a);
	return 0xffffffff;
}

/*************************************************************************
 * GetFileNameFromBrowse			[SHELL32.63]
 * 
 */
DWORD WINAPI GetFileNameFromBrowse(HWND howner, LPSTR targetbuf, DWORD len, DWORD x, LPCSTR suffix, LPCSTR y, LPCSTR cmd) 
{	FIXME(shell,"(%04x,%p,%ld,%08lx,%s,%s,%s):stub.\n",
	    howner,targetbuf,len,x,suffix,y,cmd);
    /* puts up a Open Dialog and requests input into targetbuf */
    /* OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_FILEMUSTEXIST|OFN_unknown */
    lstrcpyA(targetbuf,"x:\\dummy.exe");
    return 1;
}

/*************************************************************************
 * SHGetSettings				[SHELL32.68]
 * 
 */
DWORD WINAPI SHGetSettings(DWORD x,DWORD y,DWORD z) 
{	FIXME(shell,"(0x%08lx,0x%08lx,0x%08lx):stub.\n",x,y,z);
	return 0;
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
int WINAPI SHShellFolderView_Message(HWND hwndCabinet,UINT uMsg,LPARAM lParam)
{ FIXME(shell,"%04x %08ux %08lx stub\n",hwndCabinet,uMsg,lParam);
  return 0;
}

/*************************************************************************
 * OleStrToStrN					[SHELL32.78]
 */
BOOL WINAPI OleStrToStrN (LPSTR lpMulti, INT nMulti, LPCWSTR lpWide, INT nWide) 
{
	TRACE(shell,"%s %x %s %x\n", lpMulti, nMulti, debugstr_w(lpWide), nWide);
	return WideCharToMultiByte (0, 0, lpWide, nWide, lpMulti, nMulti, NULL, NULL);
}

/*************************************************************************
 * StrToOleStrN					[SHELL32.79]
 */
BOOL WINAPI StrToOleStrN (LPWSTR lpWide, INT nWide, LPCSTR lpMulti, INT nMulti) 
{
	TRACE(shell,"%s %x %s %x\n", debugstr_w(lpWide), nWide, lpMulti, nMulti);
	return MultiByteToWideChar (0, 0, lpMulti, nMulti, lpWide, nWide);
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
void WINAPI RegisterShellHook(HWND hwnd, DWORD y) {
    FIXME(shell,"(0x%08x,0x%08lx):stub.\n",hwnd,y);
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
INT __cdecl
ShellMessageBoxW(HMODULE hmod,HWND hwnd,DWORD idText,DWORD idTitle,DWORD uType,LPCVOID arglist) 
{	WCHAR	szText[100],szTitle[100],szTemp[256];
	LPWSTR   pszText = &szText[0], pszTitle = &szTitle[0];
	LPVOID	args = &arglist;

	TRACE(shell,"(%08lx,%08lx,%08lx,%08lx,%08lx,%p)\n",(DWORD)hmod,(DWORD)hwnd,idText,idTitle,uType,arglist);

	if (!HIWORD (idTitle))
	  LoadStringW(hmod,idTitle,pszTitle,100);
	else
	  pszTitle = (LPWSTR)idTitle;

	if (! HIWORD (idText))
	  LoadStringW(hmod,idText,pszText,100);
	else
	  pszText = (LPWSTR)idText;

	FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY ,szText,0,0,szTemp,256,args);
	return MessageBoxW(hwnd,szTemp,szTitle,uType);
}

/*************************************************************************
 * ShellMessageBoxA				[SHELL32.183]
 */
INT __cdecl
ShellMessageBoxA(HMODULE hmod,HWND hwnd,DWORD idText,DWORD idTitle,DWORD uType,LPCVOID arglist) 
{	char	szText[100],szTitle[100],szTemp[256];
	LPSTR   pszText = &szText[0], pszTitle = &szTitle[0];
	LPVOID	args = &arglist;

	TRACE(shell,"(%08lx,%08lx,%08lx,%08lx,%08lx,%p)\n", (DWORD)hmod,(DWORD)hwnd,idText,idTitle,uType,arglist);

	if (!HIWORD (idTitle))
	  LoadStringA(hmod,idTitle,pszTitle,100);
	else
	  pszTitle = (LPSTR)idTitle;

	if (! HIWORD (idText))
	  LoadStringA(hmod,idText,pszText,100);
	else
	  pszText = (LPSTR)idText;

	FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY ,pszText,0,0,szTemp,256,args);
	return MessageBoxA(hwnd,szTemp,pszTitle,uType);
}

/*************************************************************************
 * SHRestricted				[SHELL32.100]
 *
 * walks through policy table, queries <app> key, <type> value, returns 
 * queried (DWORD) value.
 * {0x00001,Explorer,NoRun}
 * {0x00002,Explorer,NoClose}
 * {0x00004,Explorer,NoSaveSettings}
 * {0x00008,Explorer,NoFileMenu}
 * {0x00010,Explorer,NoSetFolders}
 * {0x00020,Explorer,NoSetTaskbar}
 * {0x00040,Explorer,NoDesktop}
 * {0x00080,Explorer,NoFind}
 * {0x00100,Explorer,NoDrives}
 * {0x00200,Explorer,NoDriveAutoRun}
 * {0x00400,Explorer,NoDriveTypeAutoRun}
 * {0x00800,Explorer,NoNetHood}
 * {0x01000,Explorer,NoStartBanner}
 * {0x02000,Explorer,RestrictRun}
 * {0x04000,Explorer,NoPrinterTabs}
 * {0x08000,Explorer,NoDeletePrinter}
 * {0x10000,Explorer,NoAddPrinter}
 * {0x20000,Explorer,NoStartMenuSubFolders}
 * {0x40000,Explorer,MyDocsOnNet}
 * {0x80000,WinOldApp,NoRealMode}
 *
 * NOTES
 *     exported by ordinal
 */
DWORD WINAPI SHRestricted (DWORD pol) {
	HKEY	xhkey;

	FIXME(shell,"(%08lx):stub.\n",pol);
	if (RegOpenKeyA(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Policies",&xhkey))
		return 0;
	/* FIXME: do nothing for now, just return 0 (== "allowed") */
	RegCloseKey(xhkey);
	return 0;
}

/*************************************************************************
 * SHCreateDirectory				[SHELL32.165]
 *
 * NOTES
 *  exported by ordinal
 *  not sure about LPSECURITY_ATTRIBUTES
 */
DWORD WINAPI SHCreateDirectory(LPSECURITY_ATTRIBUTES sec,LPCSTR path) {
	TRACE(shell,"(%p,%s):stub.\n",sec,path);
	if (CreateDirectoryA(path,sec))
		return TRUE;
	/* SHChangeNotify(8,1,path,0); */
	return FALSE;
#if 0
	if (SHELL32_79(path,(LPVOID)x))
		return 0;
	FIXME(shell,"(%08lx,%s):stub.\n",x,path);
	return 0;
#endif
}

/*************************************************************************
 * SHFree					[SHELL32.195]
 *
 * NOTES
 *     free_ptr() - frees memory using IMalloc
 *     exported by ordinal
 */
DWORD WINAPI SHFree(LPVOID x) {
  TRACE(shell,"%p\n",x);
  if (!HIWORD(x))
  { *(LPDWORD)0xdeaf0000 = 0;
  }
  return HeapFree(GetProcessHeap(),0,x);
}

/*************************************************************************
 * SHAlloc					[SHELL32.196]
 *
 * NOTES
 *     void *task_alloc(DWORD len), uses SHMalloc allocator
 *     exported by ordinal
 */
LPVOID WINAPI SHAlloc(DWORD len) {
 /* void * ret = (LPVOID)LocalAlloc32(len,LMEM_ZEROINIT);*/ /* chrashes */
 void * ret = (LPVOID) HeapAlloc(GetProcessHeap(),0,len);
  TRACE(shell,"%lu bytes at %p\n",len, ret);
  return ret;
}

/*************************************************************************
 * SHRegisterDragDrop				[SHELL32.86]
 *
 * NOTES
 *     exported by ordinal
 */
DWORD WINAPI SHRegisterDragDrop(HWND hwnd,DWORD x2) {
    FIXME (shell, "(0x%08x,0x%08lx):stub.\n", hwnd, x2);
    return 0;
}

/*************************************************************************
 * SHRevokeDragDrop				[SHELL32.87]
 *
 * NOTES
 *     exported by ordinal
 */
DWORD WINAPI SHRevokeDragDrop(DWORD x) {
    FIXME(shell,"(0x%08lx):stub.\n",x);
    return 0;
}

/*************************************************************************
 * RunFileDlg					[SHELL32.61]
 *
 * NOTES
 *     Original name: RunFileDlg (exported by ordinal)
 */
DWORD WINAPI
RunFileDlg (HWND hwndOwner, DWORD dwParam1, DWORD dwParam2,
	    LPSTR lpszTitle, LPSTR lpszPrompt, UINT uFlags)
{
    FIXME (shell,"(0x%08x 0x%lx 0x%lx \"%s\" \"%s\" 0x%x):stub.\n",
	   hwndOwner, dwParam1, dwParam2, lpszTitle, lpszPrompt, uFlags);
    return 0;
}

/*************************************************************************
 * ExitWindowsDialog				[SHELL32.60]
 *
 * NOTES
 *     exported by ordinal
 */
DWORD WINAPI
ExitWindowsDialog (HWND hwndOwner)
{
    FIXME (shell,"(0x%08x):stub.\n", hwndOwner);
    return 0;
}

/*************************************************************************
 * ArrangeWindows				[SHELL32.184]
 * 
 */
DWORD WINAPI
ArrangeWindows (DWORD dwParam1, DWORD dwParam2, DWORD dwParam3,
		DWORD dwParam4, DWORD dwParam5)
{
    FIXME (shell,"(0x%lx 0x%lx 0x%lx 0x%lx 0x%lx):stub.\n",
	   dwParam1, dwParam2, dwParam3, dwParam4, dwParam5);
    return 0;
}

/*************************************************************************
 * SHCLSIDFromString				[SHELL32.147]
 *
 * NOTES
 *     exported by ordinal
 */
DWORD WINAPI
SHCLSIDFromString (DWORD dwParam1, DWORD dwParam2)
{
    FIXME (shell,"(0x%lx 0x%lx):stub.\n", dwParam1, dwParam2);
    FIXME (shell,"(\"%s\" \"%s\"):stub.\n", (LPSTR)dwParam1, (LPSTR)dwParam2);

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
    FIXME (shell,"(0x%08lx):stub.\n", dwParam1);

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
  { FIXME (shell,"(0x%08x,pidl=%p):stub.\n", uFlags,pv);
	}
	else
	{ FIXME (shell,"(0x%08x,%s):stub.\n", uFlags,(char*)pv);
	}
  return 0;
}
/*************************************************************************
 * SHFileOperation				[SHELL32.242]
 *
 */
DWORD WINAPI SHFileOperationAW(DWORD x)
{	FIXME(shell,"0x%08lx stub\n",x);
	return 0;

}

/*************************************************************************
 * SHFileOperationA				[SHELL32.243]
 *
 * NOTES
 *     exported by name
 */
DWORD WINAPI SHFileOperationA (LPSHFILEOPSTRUCTA lpFileOp)   
{ FIXME (shell,"(%p):stub.\n", lpFileOp);
  return 1;
}
/*************************************************************************
 * SHFileOperationW				[SHELL32.244]
 *
 * NOTES
 *     exported by name
 */
DWORD WINAPI SHFileOperationW (LPSHFILEOPSTRUCTW lpFileOp)   
{ FIXME (shell,"(%p):stub.\n", lpFileOp);
  return 1;
}

/*************************************************************************
 * SHChangeNotify				[SHELL32.239]
 *
 * NOTES
 *     exported by name
 */
DWORD WINAPI SHChangeNotify (
    INT   wEventId,  /* [IN] flags that specifies the event*/
    UINT  uFlags,   /* [IN] the meaning of dwItem[1|2]*/
		LPCVOID dwItem1,
		LPCVOID dwItem2)
{ FIXME (shell,"(0x%08x,0x%08ux,%p,%p):stub.\n", wEventId,uFlags,dwItem1,dwItem2);
  return 0;
}
/*************************************************************************
 * SHCreateShellFolderViewEx			[SHELL32.174]
 *
 * NOTES
 *  see IShellFolder::CreateViewObject
 */
HRESULT WINAPI SHCreateShellFolderViewEx(
  LPSHELLVIEWDATA psvcbi, /*[in ] shelltemplate struct*/
  LPVOID* ppv)            /*[out] IShellView pointer*/
{ FIXME (shell,"(%p,%p):stub.\n", psvcbi,ppv);
  return 0;
}
/*************************************************************************
 * SHFind_InitMenuPopup				[SHELL32.149]
 *
 * NOTES
 *  Registers the menu behind the "Start" button
 *
 * PARAMETERS
 *  hMenu		[in] handel of menu previously created
 *  hWndParent	[in] parent window
 *  w			[in] no pointer
 *  x			[in] no pointer
 */
HRESULT WINAPI SHFind_InitMenuPopup (HMENU hMenu, HWND hWndParent, DWORD w, DWORD x)
{	FIXME(shell,"hmenu=0x%08x hwnd=0x%08x 0x%08lx 0x%08lx stub\n",
		hMenu,hWndParent,w,x);
	return 0;
}
/*************************************************************************
 * FileMenu_InitMenuPopup			[SHELL32.109]
 *
 */
HRESULT WINAPI FileMenu_InitMenuPopup (DWORD hmenu)
{	FIXME(shell,"hmenu=0x%lx stub\n",hmenu);
	return 0;
}
/*************************************************************************
 * FileMenu_Create				[SHELL32.114]
 *
 * w retval from LoadBitmapA
 *
 *
 */
HRESULT WINAPI FileMenu_Create (DWORD u, DWORD v, DWORD w, DWORD x, DWORD z)
{ FIXME(shell,"0x%08lx 0x%08lx hbmp=0x%lx 0x%08lx 0x%08lx stub\n",u,v,w,x,z);
  return 0;
}
/*************************************************************************
 * FileMenu_TrackPopupMenuEx			[SHELL32.116]
 *
 * PARAMETERS
 *  uFlags		[in]	according to TrackPopupMenuEx
 *  posX		[in]
 *  posY		[in]
 *  hWndParent		[in]
 *  z	could be rect (trace) or TPMPARAMS (TrackPopupMenuEx)
 */
HRESULT WINAPI FileMenu_TrackPopupMenuEx (DWORD t, DWORD uFlags, DWORD posX, DWORD posY, HWND hWndParent, DWORD z)
{	FIXME(shell,"0x%lx flags=0x%lx posx=0x%lx posy=0x%lx hwndp=0x%x 0x%lx stub\n",
		t,uFlags,posX,posY, hWndParent,z);
	return 0;
}
/*************************************************************************
 *  SHWinHelp					[SHELL32.127]
 *
 */
HRESULT WINAPI SHWinHelp (DWORD v, DWORD w, DWORD x, DWORD z)
{	FIXME(shell,"0x%08lx 0x%08lx 0x%08lx 0x%08lx stub\n",v,w,x,z);
	return 0;
}
/*************************************************************************
 *  SHRunControlPanel [SHELL32.161]
 *
 */
HRESULT WINAPI SHRunControlPanel (DWORD x, DWORD z)
{	FIXME(shell,"0x%08lx 0x%08lx stub\n",x,z);
	return 0;
}
/*************************************************************************
 * ShellExecuteEx				[SHELL32.291]
 *
 */
BOOL WINAPI ShellExecuteExAW (LPVOID sei)
{	if (VERSION_OsIsUnicode())
	  return ShellExecuteExW (sei);
	return ShellExecuteExA (sei);
}
/*************************************************************************
 * ShellExecuteExA				[SHELL32.292]
 *
 */
BOOL WINAPI ShellExecuteExA (LPSHELLEXECUTEINFOA sei)
{ 	CHAR szApplicationName[MAX_PATH],szCommandline[MAX_PATH],szPidl[20];
	LPSTR pos;
	int gap, len;
	STARTUPINFOA  startupinfo;
	PROCESS_INFORMATION processinformation;
			
  	WARN(shell,"mask=0x%08lx hwnd=0x%04x verb=%s file=%s parm=%s dir=%s show=0x%08x class=%s incomplete\n",
		sei->fMask, sei->hwnd, sei->lpVerb, sei->lpFile,
		sei->lpParameters, sei->lpDirectory, sei->nShow, sei->lpClass);

	ZeroMemory(szApplicationName,MAX_PATH);
	if (sei->lpFile)
	  strcpy(szApplicationName, sei->lpFile);
	
	ZeroMemory(szCommandline,MAX_PATH);
	if (sei->lpParameters)
	  strcpy(szCommandline, sei->lpParameters);
			
	if (sei->fMask & (SEE_MASK_CLASSKEY | SEE_MASK_INVOKEIDLIST | SEE_MASK_ICON | SEE_MASK_HOTKEY |
			  SEE_MASK_NOCLOSEPROCESS | SEE_MASK_CONNECTNETDRV | SEE_MASK_FLAG_DDEWAIT |
			  SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_NO_UI | SEE_MASK_UNICODE | 
			  SEE_MASK_NO_CONSOLE | SEE_MASK_ASYNCOK | SEE_MASK_HMONITOR ))
	{ FIXME (shell,"flags ignored: 0x%08lx\n", sei->fMask);
	}

	if (sei->fMask & SEE_MASK_CLASSNAME)
	{ HCR_GetExecuteCommand(sei->lpClass, (sei->lpVerb) ? sei->lpVerb : "open", szCommandline, 256);	    
	}

	/* process the IDList */
	if ( (sei->fMask & SEE_MASK_INVOKEIDLIST) == SEE_MASK_INVOKEIDLIST) /*0x0c*/
	{ SHGetPathFromIDListA (sei->lpIDList,szApplicationName);
	  TRACE(shell,"-- idlist=%p (%s)\n", sei->lpIDList, szApplicationName);
	}
	else
	{ if (sei->fMask & SEE_MASK_IDLIST )
	  { /* %I is the adress of a global item ID*/
	    pos = strstr(szCommandline, "%I");
	    if (pos)
	    { HGLOBAL hmem = SHAllocShared ( sei->lpIDList, ILGetSize(sei->lpIDList), 0);
	      sprintf(szPidl,":%li",(DWORD)SHLockShared(hmem,0) );
	      SHUnlockShared(hmem);
	    
	      gap = strlen(szPidl);
	      len = strlen(pos)-2;
	      memmove(pos+gap,pos+2,len);
	      memcpy(pos,szPidl,gap);

	    }
	  }
	}

	pos = strstr(szCommandline, ",%L");	/* dunno what it means: kill it*/
	if (pos)
	{ len = strlen(pos)-2;
	  *pos=0x0;
	  memmove(pos,pos+3,len);
	}

	TRACE(shell,"execute: %s %s\n",szApplicationName, szCommandline);

	ZeroMemory(&startupinfo,sizeof(STARTUPINFOA));
	startupinfo.cb = sizeof(STARTUPINFOA);

	return CreateProcessA(szApplicationName[0] ? szApplicationName:NULL,
			 szCommandline[0] ? szCommandline : NULL,
			 NULL, NULL, FALSE, 0, 
			 NULL, NULL, &startupinfo, &processinformation);
	  
	
}
/*************************************************************************
 * ShellExecuteExW				[SHELL32.293]
 *
 */
BOOL WINAPI ShellExecuteExW (LPSHELLEXECUTEINFOW sei)
{	SHELLEXECUTEINFOA seiA;
	DWORD ret;

	TRACE (shell,"%p\n", sei);

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
{	TRACE (shell,"%p\n", lpUnknown);
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
{	TRACE(shell,"%p\n", lpUnknown);

	*lpUnknown = SHELL32_IExplorerInterface;

	if (!SHELL32_IExplorerInterface)
	  return E_FAIL;

	SHELL32_IExplorerInterface->lpvtbl->fnAddRef(SHELL32_IExplorerInterface);
	return NOERROR;
}
/*************************************************************************
 * SHFreeUnusedLibraries			[SHELL32.123]
 *
 * NOTES
 *  exported by name
 */
HRESULT WINAPI SHFreeUnusedLibraries (void)
{	FIXME(shell,"stub\n");
	return TRUE;
}
/*************************************************************************
 * DAD_ShowDragImage				[SHELL32.137]
 *
 * NOTES
 *  exported by name
 */
HRESULT WINAPI DAD_ShowDragImage (DWORD u)
{ FIXME(shell,"0x%08lx stub\n",u);
  return 0;
}
/*************************************************************************
 * FileMenu_Destroy				[SHELL32.118]
 *
 * NOTES
 *  exported by name
 */
HRESULT WINAPI FileMenu_Destroy (DWORD u)
{ FIXME(shell,"0x%08lx stub\n",u);
  return 0;
}
/*************************************************************************
 * SHRegCloseKey			[NT4.0:SHELL32.505]
 *
 */
HRESULT WINAPI SHRegCloseKey (HKEY hkey)
{	TRACE(shell,"0x%04x\n",hkey);
	return RegCloseKey( hkey );
}
/*************************************************************************
 * SHRegOpenKeyA				[SHELL32.506]
 *
 */
HRESULT WINAPI SHRegOpenKeyA(HKEY hKey, LPSTR lpSubKey, LPHKEY phkResult)
{	FIXME(shell,"(0x%08x, %s, %p)\n", hKey, debugstr_a(lpSubKey),
	      phkResult);
	return RegOpenKeyA(hKey, lpSubKey, phkResult);
}

/*************************************************************************
 * SHRegOpenKeyW				[NT4.0:SHELL32.507]
 *
 */
HRESULT WINAPI SHRegOpenKeyW (HKEY hkey, LPCWSTR lpszSubKey, LPHKEY retkey)
{	WARN(shell,"0x%04x %s %p\n",hkey,debugstr_w(lpszSubKey),retkey);
	return RegOpenKeyW( hkey, lpszSubKey, retkey );
}
/*************************************************************************
 * SHRegQueryValueExA				[SHELL32.509]
 *
 */
HRESULT WINAPI SHRegQueryValueExA(DWORD u, LPSTR v, DWORD w, DWORD x,
				  DWORD y, DWORD z)
{	FIXME(shell,"0x%04lx %s 0x%04lx 0x%04lx 0x%04lx  0x%04lx stub\n",
		u,debugstr_a(v),w,x,y,z);
	return 0;
}
/*************************************************************************
 * SHRegQueryValueW				[NT4.0:SHELL32.510]
 *
 */
HRESULT WINAPI SHRegQueryValueW (HKEY hkey, LPWSTR lpszSubKey,
				 LPWSTR lpszData, LPDWORD lpcbData )
{	WARN(shell,"0x%04x %s %p %p semi-stub\n",
		hkey, debugstr_w(lpszSubKey), lpszData, lpcbData);
	return RegQueryValueW( hkey, lpszSubKey, lpszData, lpcbData );
}

/*************************************************************************
 * SHRegQueryValueExW				[NT4.0:SHELL32.511]
 *
 * FIXME 
 *  if the datatype REG_EXPAND_SZ then expand the string and change
 *  *pdwType to REG_SZ. 
 */
HRESULT WINAPI SHRegQueryValueExW (HKEY hkey, LPWSTR pszValue, LPDWORD pdwReserved,
		 LPDWORD pdwType, LPVOID pvData, LPDWORD pcbData)
{	DWORD ret;
	WARN(shell,"0x%04x %s %p %p %p %p semi-stub\n",
		hkey, debugstr_w(pszValue), pdwReserved, pdwType, pvData, pcbData);
	ret = RegQueryValueExW ( hkey, pszValue, pdwReserved, pdwType, pvData, pcbData);
	return ret;
}

/*************************************************************************
 * ReadCabinetState				[NT 4.0:SHELL32.651]
 *
 */
HRESULT WINAPI ReadCabinetState(DWORD u, DWORD v)
{	FIXME(shell,"0x%04lx 0x%04lx stub\n",u,v);
	return 0;
}
/*************************************************************************
 * WriteCabinetState				[NT 4.0:SHELL32.652]
 *
 */
HRESULT WINAPI WriteCabinetState(DWORD u)
{	FIXME(shell,"0x%04lx stub\n",u);
	return 0;
}
/*************************************************************************
 * FileIconInit 				[SHELL32.660]
 *
 */
BOOL WINAPI FileIconInit(BOOL bFullInit)
{	FIXME(shell,"(%s)\n", bFullInit ? "true" : "false");
	return 0;
}
/*************************************************************************
 * IsUserAdmin					[NT 4.0:SHELL32.680]
 *
 */
HRESULT WINAPI IsUserAdmin(void)
{	FIXME(shell,"stub\n");
	return TRUE;
}
/*************************************************************************
 * StrRetToStrN					[SHELL32.96]
 * 
 * converts a STRRET to a normal string
 *
 * NOTES
 *  the pidl is for STRRET OFFSET
 */
HRESULT WINAPI StrRetToStrN (LPVOID dest, DWORD len, LPSTRRET src, LPITEMIDLIST pidl)
{	TRACE(shell,"dest=0x%p len=0x%lx strret=0x%p pidl=%p stub\n",dest,len,src,pidl);

	switch (src->uType)
	{ case STRRET_WSTR:
	    WideCharToMultiByte(CP_ACP, 0, src->u.pOleStr, -1, (LPSTR)dest, len, NULL, NULL);
	    SHFree(src->u.pOleStr);
	    break;

	  case STRRET_CSTRA:
	    if (VERSION_OsIsUnicode())
	      lstrcpynAtoW((LPWSTR)dest, src->u.cStr, len);
	    else
	      strncpy((LPSTR)dest, src->u.cStr, len);
	    break;

	  case STRRET_OFFSETA:
	    if (pidl)
	    { if(VERSION_OsIsUnicode())
	        lstrcpynAtoW((LPWSTR)dest, ((LPCSTR)&pidl->mkid)+src->u.uOffset, len);
	      else
	        strncpy((LPSTR)dest, ((LPCSTR)&pidl->mkid)+src->u.uOffset, len);
	      break;
	    }

	  default:
	    FIXME(shell,"unknown type!\n");
	    if (len)
	    { *(LPSTR)dest = '\0';
	    }
	    return(FALSE);
	}
	return(TRUE);
}

/*************************************************************************
 * StrChrW					[NT 4.0:SHELL32.651]
 *
 */
LPWSTR WINAPI StrChrW (LPWSTR str, WCHAR x )
{	LPWSTR ptr=str;
	
	TRACE(shell,"%s 0x%04x\n",debugstr_w(str),x);
	do 
	{  if (*ptr==x)
	   { return ptr;
	   }
	   ptr++;
	} while (*ptr);
	return NULL;
}

/*************************************************************************
 * StrCmpNIW					[NT 4.0:SHELL32.*]
 *
 */
INT WINAPI StrCmpNIW ( LPWSTR wstr1, LPWSTR wstr2, INT len)
{	FIXME( shell,"%s %s %i stub\n", debugstr_w(wstr1),debugstr_w(wstr2),len);
	return 0;
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
	
	TRACE(shell,"ptr=%p size=0x%04lx procID=0x%04lx\n",psrc,size,procID);
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
{	TRACE(shell,"handle=0x%04x procID=0x%04lx\n",hmem,procID);
	return GlobalLock(hmem);
}
/*************************************************************************
 * SHUnlockShared				[SHELL32.522]
 *
 * NOTES
 *  parameter1 is return value from SHLockShared
 */
BOOL WINAPI SHUnlockShared(HANDLE pmem)
{	TRACE(shell,"handle=0x%04x\n",pmem);
	return GlobalUnlock(pmem); 
}
/*************************************************************************
 * SHFreeShared					[SHELL32.523]
 *
 * NOTES
 *  parameter1 is return value from SHAllocShared
 *  parameter2 is return value from GetCurrentProcessId
 */
HANDLE WINAPI SHFreeShared(HANDLE hmem, DWORD procID)
{	TRACE(shell,"handle=0x%04x 0x%04lx\n",hmem,procID);
	return GlobalFree(hmem);
}

/*************************************************************************
 * SetAppStartingCursor				[SHELL32.99]
 *
 */
HRESULT WINAPI SetAppStartingCursor(HWND u, DWORD v)
{	FIXME(shell,"hwnd=0x%04x 0x%04lx stub\n",u,v );
	return 0;
}
/*************************************************************************
 * SHLoadOLE					[SHELL32.151]
 *
 */
HRESULT WINAPI SHLoadOLE(DWORD u)
{	FIXME(shell,"0x%04lx stub\n",u);
	return S_OK;
}
/*************************************************************************
 * Shell_MergeMenus				[SHELL32.67]
 *
 */
BOOL _SHIsMenuSeparator(HMENU hm, int i)
{
	MENUITEMINFOA mii;

	mii.cbSize = sizeof(MENUITEMINFOA);
	mii.fMask = MIIM_TYPE;
	mii.cch = 0;    /* WARNING: We MUST initialize it to 0*/
	if (!GetMenuItemInfoA(hm, i, TRUE, &mii))
	{ return(FALSE);
	}

	if (mii.fType & MFT_SEPARATOR)
	{ return(TRUE);
	}

        return(FALSE);
}
#define MM_ADDSEPARATOR         0x00000001L
#define MM_SUBMENUSHAVEIDS      0x00000002L
HRESULT WINAPI Shell_MergeMenus (HMENU hmDst, HMENU hmSrc, UINT uInsert, UINT uIDAdjust, UINT uIDAdjustMax, ULONG uFlags)
{	int		nItem;
	HMENU		hmSubMenu;
	BOOL		bAlreadySeparated;
	MENUITEMINFOA miiSrc;
	char		szName[256];
	UINT		uTemp, uIDMax = uIDAdjust;

	FIXME(shell,"hmenu1=0x%04x hmenu2=0x%04x 0x%04x 0x%04x 0x%04x  0x%04lx stub\n",
		 hmDst, hmSrc, uInsert, uIDAdjust, uIDAdjustMax, uFlags);

	if (!hmDst || !hmSrc)
	{ return uIDMax;
	}

	nItem = GetMenuItemCount(hmDst);
	if (uInsert >= (UINT)nItem)
	{ uInsert = (UINT)nItem;
	  bAlreadySeparated = TRUE;
	}
	else
	{ bAlreadySeparated = _SHIsMenuSeparator(hmDst, uInsert);;
	}
	if ((uFlags & MM_ADDSEPARATOR) && !bAlreadySeparated)
	{ /* Add a separator between the menus */
	  InsertMenuA(hmDst, uInsert, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
	  bAlreadySeparated = TRUE;
        }


        /* Go through the menu items and clone them*/
        for (nItem = GetMenuItemCount(hmSrc) - 1; nItem >= 0; nItem--)
        { miiSrc.cbSize = sizeof(MENUITEMINFOA);
	  miiSrc.fMask = MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_CHECKMARKS | MIIM_TYPE | MIIM_DATA;
	  /* We need to reset this every time through the loop in case
	  menus DON'T have IDs*/
	  miiSrc.fType = MFT_STRING;
	  miiSrc.dwTypeData = szName;
	  miiSrc.dwItemData = 0;
	  miiSrc.cch = sizeof(szName);

	  if (!GetMenuItemInfoA(hmSrc, nItem, TRUE, &miiSrc))
	  { continue;
	  }
	  if (miiSrc.fType & MFT_SEPARATOR)
	  { /* This is a separator; don't put two of them in a row*/
	    if (bAlreadySeparated)
	    { continue;
	    }
	    bAlreadySeparated = TRUE;
	  }
	  else if (miiSrc.hSubMenu)
	  { if (uFlags & MM_SUBMENUSHAVEIDS)
	    { /* Adjust the ID and check it*/
	      miiSrc.wID += uIDAdjust;
	      if (miiSrc.wID > uIDAdjustMax)
	      { continue;
	      }
	      if (uIDMax <= miiSrc.wID)
	      { uIDMax = miiSrc.wID + 1;
	      }
	    }
	    else
	    { /* Don't set IDs for submenus that didn't have them already */
	      miiSrc.fMask &= ~MIIM_ID;
	    }
	    hmSubMenu = miiSrc.hSubMenu;
	    miiSrc.hSubMenu = CreatePopupMenu();
	    if (!miiSrc.hSubMenu)
	    { return(uIDMax);
	    }
	    uTemp = Shell_MergeMenus(miiSrc.hSubMenu, hmSubMenu, 0, uIDAdjust, uIDAdjustMax, uFlags&MM_SUBMENUSHAVEIDS);
	    if (uIDMax <= uTemp)
	    { uIDMax = uTemp;
	    }
	    bAlreadySeparated = FALSE;
	  }
	  else
	  { /* Adjust the ID and check it*/
	    miiSrc.wID += uIDAdjust;
	    if (miiSrc.wID > uIDAdjustMax)
	    { continue;
	    }
	    if (uIDMax <= miiSrc.wID)
	    { uIDMax = miiSrc.wID + 1;
	    }
	    bAlreadySeparated = FALSE;
	  }
	  if (!InsertMenuItemA(hmDst, uInsert, TRUE, &miiSrc))
	  { return(uIDMax);
	  }
	}

	/* Ensure the correct number of separators at the beginning of the
        inserted menu items*/
        if (uInsert == 0)
        { if (bAlreadySeparated)
	  { DeleteMenu(hmDst, uInsert, MF_BYPOSITION);
	  }
	}
	else
	{ if (_SHIsMenuSeparator(hmDst, uInsert-1))
	  { if (bAlreadySeparated)
	    { DeleteMenu(hmDst, uInsert, MF_BYPOSITION);
	    }
	  }
	  else
	  { if ((uFlags & MM_ADDSEPARATOR) && !bAlreadySeparated)
	    { /* Add a separator between the menus*/
	      InsertMenuA(hmDst, uInsert, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
	    }
	  }
	}
	return(uIDMax);
}
/*************************************************************************
 * DriveType					[SHELL32.64]
 *
 */
HRESULT WINAPI DriveType(DWORD u)
{	FIXME(shell,"0x%04lx stub\n",u);
	return 0;
}
/*************************************************************************
 * SHAbortInvokeCommand				[SHELL32.198]
 *
 */
HRESULT WINAPI SHAbortInvokeCommand(void)
{	FIXME(shell,"stub\n");
	return 1;
}
/*************************************************************************
 * SHOutOfMemoryMessageBox			[SHELL32.126]
 *
 */
HRESULT WINAPI SHOutOfMemoryMessageBox(DWORD u, DWORD v, DWORD w)
{	FIXME(shell,"0x%04lx 0x%04lx 0x%04lx stub\n",u,v,w);
	return 0;
}
/*************************************************************************
 * SHFlushClipboard				[SHELL32.121]
 *
 */
HRESULT WINAPI SHFlushClipboard(void)
{	FIXME(shell,"stub\n");
	return 1;
}
/*************************************************************************
 * StrRChrW					[SHELL32.320]
 *
 */
LPWSTR WINAPI StrRChrW(LPWSTR lpStart, LPWSTR lpEnd, DWORD wMatch)
{	LPWSTR wptr=NULL;
	TRACE(shell,"%s %s 0x%04x\n",debugstr_w(lpStart),debugstr_w(lpEnd), (WCHAR)wMatch );

	/* if the end not given, search*/
	if (!lpEnd)
	{ lpEnd=lpStart;
	  while (*lpEnd) 
	    lpEnd++;
	}

	do 
	{ if (*lpStart==(WCHAR)wMatch)
	    wptr = lpStart;
	  lpStart++;  
	} while ( lpStart<=lpEnd ); 
	return wptr;
}
/*************************************************************************
* StrFormatByteSize				[SHLWAPI]
*/
LPSTR WINAPI StrFormatByteSizeA ( DWORD dw, LPSTR pszBuf, UINT cchBuf )
{	char buf[64];
	TRACE(shell,"%lx %p %i\n", dw, pszBuf, cchBuf);
	if ( dw<1024L )
	{ sprintf (buf,"%3.1f bytes", (FLOAT)dw);
	}
	else if ( dw<1048576L)
	{ sprintf (buf,"%3.1f KB", (FLOAT)dw/1024);
	}
	else if ( dw < 1073741824L)
	{ sprintf (buf,"%3.1f MB", (FLOAT)dw/1048576L);
	}
	else
	{ sprintf (buf,"%3.1f GB", (FLOAT)dw/1073741824L);
	}
	strncpy (pszBuf, buf, cchBuf);
	return pszBuf;	
}
LPWSTR WINAPI StrFormatByteSizeW ( DWORD dw, LPWSTR pszBuf, UINT cchBuf )
{	char buf[64];
	TRACE(shell,"%lx %p %i\n", dw, pszBuf, cchBuf);
	if ( dw<1024L )
	{ sprintf (buf,"%3.1f bytes", (FLOAT)dw);
	}
	else if ( dw<1048576L)
	{ sprintf (buf,"%3.1f KB", (FLOAT)dw/1024);
	}
	else if ( dw < 1073741824L)
	{ sprintf (buf,"%3.1f MB", (FLOAT)dw/1048576L);
	}
	else
	{ sprintf (buf,"%3.1f GB", (FLOAT)dw/1073741824L);
	}
	lstrcpynAtoW (pszBuf, buf, cchBuf);
	return pszBuf;	
}
/*************************************************************************
 * SHWaitForFileToOpen				[SHELL32.97]
 *
 */
HRESULT WINAPI SHWaitForFileToOpen(DWORD u, DWORD v, DWORD w)
{	FIXME(shell,"0x%04lx 0x%04lx 0x%04lx stub\n",u,v,w);
	return 0;
}
/*************************************************************************
 * Control_FillCache_RunDLL			[SHELL32.8]
 *
 */
HRESULT WINAPI Control_FillCache_RunDLL(HWND hWnd, HANDLE hModule, DWORD w, DWORD x)
{	FIXME(shell,"0x%04x 0x%04x 0x%04lx 0x%04lx stub\n",hWnd, hModule,w,x);
	return 0;
}
/*************************************************************************
 * RunDLL_CallEntry16				[SHELL32.122]
 * the name is propably wrong
 */
HRESULT WINAPI RunDLL_CallEntry16(DWORD v, DWORD w, DWORD x, DWORD y, DWORD z)
{	FIXME(shell,"0x%04lx 0x%04lx 0x%04lx 0x%04lx 0x%04lx stub\n",v,w,x,y,z);
	return 0;
}

HRESULT shell32_654 (DWORD x, DWORD y)
{	FIXME(shell,"0x%08lx 0x%08lx stub\n",x,y);
	return 0;
}
