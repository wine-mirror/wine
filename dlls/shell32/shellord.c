/*
 * 				Shell Ordinal Functions
 *
 * These are completely undocumented. The meaning of the functions changes
 * between different OS versions (NT uses Unicode strings, 95 uses ASCII
 * strings, etc. etc.)
 * 
 * They are just here so that explorer.exe and iexplore.exe can be tested.
 *
 * Copyright 1997 Marcus Meissner
 *           1998 Jürgen Schmied
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "windows.h"
#include "winerror.h"
#include "file.h"
#include "shell.h"
#include "heap.h"
#include "module.h"
#include "neexe.h"
#include "resource.h"
#include "dlgs.h"
#include "win.h"
#include "cursoricon.h"
#include "interfaces.h"
#include "shlobj.h"
#include "debug.h"
#include "winreg.h"
#include "winnls.h"
#include "winversion.h"
#include "shell32_main.h"

/*************************************************************************
 * SHChangeNotifyRegister [SHELL32.2]
 * NOTES
 *   Idlist is an array of structures and Count specifies how many items in the array
 *   (usually just one I think).
 */
DWORD WINAPI
SHChangeNotifyRegister(
    HWND32 hwnd,
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
 * SHChangeNotifyDeregister [SHELL32.4]
 */
DWORD WINAPI
SHChangeNotifyDeregister(LONG x1)
{	FIXME(shell,"(0x%08lx):stub.\n",x1);
	return 0;
}

/*************************************************************************
 * ParseField [SHELL32.58]
 *
 */
DWORD WINAPI ParseField32A(LPCSTR src, DWORD field, LPSTR dst, DWORD len) 
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
 * PickIconDlg [SHELL32.62]
 * 
 */
DWORD WINAPI PickIconDlg(DWORD x,DWORD y,DWORD z,DWORD a) 
{	FIXME(shell,"(%08lx,%08lx,%08lx,%08lx):stub.\n",x,y,z,a);
	return 0xffffffff;
}

/*************************************************************************
 * GetFileNameFromBrowse [SHELL32.63]
 * 
 */
DWORD WINAPI GetFileNameFromBrowse(HWND32 howner, LPSTR targetbuf, DWORD len, DWORD x, LPCSTR suffix, LPCSTR y, LPCSTR cmd) 
{	FIXME(shell,"(%04x,%p,%ld,%08lx,%s,%s,%s):stub.\n",
	    howner,targetbuf,len,x,suffix,y,cmd);
    /* puts up a Open Dialog and requests input into targetbuf */
    /* OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_FILEMUSTEXIST|OFN_unknown */
    lstrcpy32A(targetbuf,"x:\\dummy.exe");
    return 1;
}

/*************************************************************************
 * SHGetSettings [SHELL32.68]
 * 
 */
DWORD WINAPI SHGetSettings(DWORD x,DWORD y,DWORD z) 
{	FIXME(shell,"(0x%08lx,0x%08lx,0x%08lx):stub.\n",x,y,z);
	return 0;
}

/*************************************************************************
 * Shell_GetCachedImageIndex [SHELL32.72]
 *
 */
void WINAPI Shell_GetCachedImageIndex(LPVOID x,DWORD y,DWORD z) 
{	if( VERSION_OsIsUnicode())
	{ FIXME(shell,"(L%s,%08lx,%08lx):stub.\n",debugstr_w((LPWSTR)x),y,z);
	}
	else
	{ FIXME(shell,"(%s,%08lx,%08lx):stub.\n",debugstr_a((LPSTR)x),y,z);
	}
}

/*************************************************************************
 * SHShellFolderView_Message [SHELL32.73]
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
int WINAPI SHShellFolderView_Message(HWND32 hwndCabinet,UINT32 uMsg,LPARAM lParam)
{ FIXME(shell,"%04x %08ux %08lx stub\n",hwndCabinet,uMsg,lParam);
  return 0;
}

/*************************************************************************
 * OleStrToStrN  [SHELL32.78]
 * 
 * NOTES
 *     exported by ordinal
 * FIXME
 *  wrong implemented OleStr is NOT wide string !!!! (jsch)
 */
BOOL32 WINAPI
OleStrToStrN (LPSTR lpMulti, INT32 nMulti, LPCWSTR lpWide, INT32 nWide) {
    return WideCharToMultiByte (0, 0, lpWide, nWide,
        lpMulti, nMulti, NULL, NULL);
}

/*************************************************************************
 * StrToOleStrN  [SHELL32.79]
 *
 * NOTES
 *     exported by ordinal
 * FIXME
 *  wrong implemented OleStr is NOT wide string !!!! (jsch)
*/
BOOL32 WINAPI
StrToOleStrN (LPWSTR lpWide, INT32 nWide, LPCSTR lpMulti, INT32 nMulti) {
    return MultiByteToWideChar (0, 0, lpMulti, nMulti, lpWide, nWide);
}

/*************************************************************************
 * SHCloneSpecialIDList [SHELL32.89]
 * 
 * PARAMETERS
 *  hwndOwner 	[in] 
 *  nFolder 	[in]	CSIDL_xxxxx ??
 *
 * RETURNS
 *  pidl ??
 * NOTES
 *     exported by ordinal
 */
LPITEMIDLIST WINAPI SHCloneSpecialIDList(HWND32 hwndOwner,DWORD nFolder,DWORD x3)
{	LPITEMIDLIST ppidl;
	WARN(shell,"(hwnd=0x%x,csidl=0x%lx,0x%lx):semi-stub.\n",
					 hwndOwner,nFolder,x3);

	SHGetSpecialFolderLocation(hwndOwner, nFolder, &ppidl);

	return ppidl;
}


/*************************************************************************
 * SHGetSpecialFolderPath [SHELL32.175]
 * 
 * NOTES
 *     exported by ordinal
 */
void WINAPI SHGetSpecialFolderPath(DWORD x1,DWORD x2,DWORD x3,DWORD x4) {
    FIXME(shell,"(0x%04lx,0x%04lx,csidl=0x%04lx,0x%04lx):stub.\n",
      x1,x2,x3,x4
    );
}

/*************************************************************************
 * RegisterShellHook [SHELL32.181]
 *
 * PARAMS
 *      hwnd [I]  window handle
 *      y    [I]  flag ????
 * 
 * NOTES
 *     exported by ordinal
 */
void WINAPI RegisterShellHook32(HWND32 hwnd, DWORD y) {
    FIXME(shell,"(0x%08x,0x%08lx):stub.\n",hwnd,y);
}

/*************************************************************************
 * ShellMessageBoxA [SHELL32.183]
 *
 * Format and output errormessage.
 *
 * NOTES
 *     exported by ordinal
 */
void __cdecl
ShellMessageBoxA(HMODULE32 hmod,HWND32 hwnd,DWORD id,DWORD x,DWORD type,LPVOID arglist) {
	char	buf[100],buf2[100]/*,*buf3*/;
/*	LPVOID	args = &arglist;*/

	if (!LoadString32A(hmod,x,buf,100))
		strcpy(buf,"Desktop");
/*	LoadString32A(hmod,id,buf2,100); */
	/* FIXME: the varargs handling doesn't. */
/*	FormatMessage32A(0x500,buf2,0,0,(LPSTR)&buf3,256,(LPDWORD)&args); */

	FIXME(shell,"(%08lx,%08lx,%08lx(%s),%08lx(%s),%08lx,%p):stub.\n",
		(DWORD)hmod,(DWORD)hwnd,id,buf2,x,buf,type,arglist
	);
	/*MessageBox32A(hwnd,buf3,buf,id|0x10000);*/
}

/*************************************************************************
 * SHRestricted [SHELL32.100]
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
	if (RegOpenKey32A(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Policies",&xhkey))
		return 0;
	/* FIXME: do nothing for now, just return 0 (== "allowed") */
	RegCloseKey(xhkey);
	return 0;
}

/*************************************************************************
 * SHCreateDirectory [SHELL32.165]
 *
 * NOTES
 *  exported by ordinal
 *  not sure about LPSECURITY_ATTRIBUTES
 */
DWORD WINAPI SHCreateDirectory(LPSECURITY_ATTRIBUTES sec,LPCSTR path) {
	TRACE(shell,"(%p,%s):stub.\n",sec,path);
	if (CreateDirectory32A(path,sec))
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
 * SHFree [SHELL32.195]
 *
 * NOTES
 *     free_ptr() - frees memory using IMalloc
 *     exported by ordinal
 */
DWORD WINAPI SHFree(LPVOID x) {
  TRACE(shell,"%p\n",x);
  /*return LocalFree32((HANDLE32)x);*/ /* crashes */
  return HeapFree(GetProcessHeap(),0,x);
}

/*************************************************************************
 * SHAlloc [SHELL32.196]
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
 * OpenRegStream [SHELL32.85]
 *
 * NOTES
 *     exported by ordinal
 */
DWORD WINAPI OpenRegStream(DWORD x1,DWORD x2,DWORD x3,DWORD x4) {
    FIXME(shell,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx):stub.\n",
    	x1,x2,x3,x4
    );
    return 0;
}

/*************************************************************************
 * SHRegisterDragDrop [SHELL32.86]
 *
 * NOTES
 *     exported by ordinal
 */
DWORD WINAPI SHRegisterDragDrop(HWND32 hwnd,DWORD x2) {
    FIXME (shell, "(0x%08x,0x%08lx):stub.\n", hwnd, x2);
    return 0;
}

/*************************************************************************
 * SHRevokeDragDrop [SHELL32.87]
 *
 * NOTES
 *     exported by ordinal
 */
DWORD WINAPI SHRevokeDragDrop(DWORD x) {
    FIXME(shell,"(0x%08lx):stub.\n",x);
    return 0;
}

/*************************************************************************
 * RunFileDlg [SHELL32.61]
 *
 * NOTES
 *     Original name: RunFileDlg (exported by ordinal)
 */
DWORD WINAPI
RunFileDlg (HWND32 hwndOwner, DWORD dwParam1, DWORD dwParam2,
	    LPSTR lpszTitle, LPSTR lpszPrompt, UINT32 uFlags)
{
    FIXME (shell,"(0x%08x 0x%lx 0x%lx \"%s\" \"%s\" 0x%x):stub.\n",
	   hwndOwner, dwParam1, dwParam2, lpszTitle, lpszPrompt, uFlags);
    return 0;
}

/*************************************************************************
 * ExitWindowsDialog [SHELL32.60]
 *
 * NOTES
 *     exported by ordinal
 */
DWORD WINAPI
ExitWindowsDialog (HWND32 hwndOwner)
{
    FIXME (shell,"(0x%08x):stub.\n", hwndOwner);
    return 0;
}

/*************************************************************************
 * ArrangeWindows [SHELL32.184]
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
 * SHCLSIDFromString [SHELL32.147]
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
 * SignalFileOpen [SHELL32.103]
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
 * SHAddToRecentDocs [SHELL32.234]
 *
 * PARAMETERS
 *   uFlags  [IN] SHARD_PATH or SHARD_PIDL
 *   pv      [IN] string or pidl, NULL clears the list
 *
 * NOTES
 *     exported by name
 */
DWORD WINAPI SHAddToRecentDocs32 (UINT32 uFlags,LPCVOID pv)   
{ if (SHARD_PIDL==uFlags)
  { FIXME (shell,"(0x%08x,pidl=%p):stub.\n", uFlags,pv);
	}
	else
	{ FIXME (shell,"(0x%08x,%s):stub.\n", uFlags,(char*)pv);
	}
  return 0;
}
/*************************************************************************
 * SHFileOperation32 [SHELL32.242]
 *
 */
DWORD WINAPI SHFileOperation32(DWORD x)
{	FIXME(shell,"0x%08lx stub\n",x);
	return 0;

}

/*************************************************************************
 * SHFileOperation32A [SHELL32.243]
 *
 * NOTES
 *     exported by name
 */
DWORD WINAPI SHFileOperation32A (LPSHFILEOPSTRUCT32A lpFileOp)   
{ FIXME (shell,"(%p):stub.\n", lpFileOp);
  return 1;
}
/*************************************************************************
 * SHFileOperation32W [SHELL32.244]
 *
 * NOTES
 *     exported by name
 */
DWORD WINAPI SHFileOperation32W (LPSHFILEOPSTRUCT32W lpFileOp)   
{ FIXME (shell,"(%p):stub.\n", lpFileOp);
  return 1;
}

/*************************************************************************
 * SHChangeNotify [SHELL32.239]
 *
 * NOTES
 *     exported by name
 */
DWORD WINAPI SHChangeNotify32 (
    INT32   wEventId,  /* [IN] flags that specifies the event*/
    UINT32  uFlags,   /* [IN] the meaning of dwItem[1|2]*/
		LPCVOID dwItem1,
		LPCVOID dwItem2)
{ FIXME (shell,"(0x%08x,0x%08ux,%p,%p):stub.\n", wEventId,uFlags,dwItem1,dwItem2);
  return 0;
}
/*************************************************************************
 * SHCreateShellFolderViewEx [SHELL32.174]
 *
 * NOTES
 *  see IShellFolder::CreateViewObject
 */
HRESULT WINAPI SHCreateShellFolderViewEx32(
  LPSHELLVIEWDATA psvcbi, /*[in ] shelltemplate struct*/
  LPVOID* ppv)            /*[out] IShellView pointer*/
{ FIXME (shell,"(%p,%p):stub.\n", psvcbi,ppv);
  return 0;
}
/*************************************************************************
 * SHFind_InitMenuPopup [SHELL32.149]
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
HRESULT WINAPI SHFind_InitMenuPopup (HMENU32 hMenu, HWND32 hWndParent, DWORD w, DWORD x)
{	FIXME(shell,"hmenu=0x%08x hwnd=0x%08x 0x%08lx 0x%08lx stub\n",
		hMenu,hWndParent,w,x);
	return 0;
}
/*************************************************************************
 * FileMenu_InitMenuPopup [SHELL32.109]
 *
 */
HRESULT WINAPI FileMenu_InitMenuPopup (DWORD hmenu)
{	FIXME(shell,"hmenu=0x%lx stub\n",hmenu);
	return 0;
}
/*************************************************************************
 * FileMenu_Create [SHELL32.114]
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
 * FileMenu_TrackPopupMenuEx [SHELL32.116]
 *
 * PARAMETERS
 *  uFlags		[in]	according to TrackPopupMenuEx
 *  posX		[in]
 *  posY		[in]
 *  hWndParent		[in]
 *  z	could be rect (trace) or TPMPARAMS (TrackPopupMenuEx)
 */
HRESULT WINAPI FileMenu_TrackPopupMenuEx (DWORD t, DWORD uFlags, DWORD posX, DWORD posY, HWND32 hWndParent, DWORD z)
{	FIXME(shell,"0x%lx flags=0x%lx posx=0x%lx posy=0x%lx hwndp=0x%x 0x%lx stub\n",
		t,uFlags,posX,posY, hWndParent,z);
	return 0;
}
/*************************************************************************
 *  SHWinHelp [SHELL32.127]
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
 * ShellExecuteEx [SHELL32.291]
 *
 */
BOOL32 WINAPI ShellExecuteEx32 (LPVOID sei)
{	if (VERSION_OsIsUnicode())
	  return ShellExecuteEx32W (sei);
	return ShellExecuteEx32A (sei);
}
/*************************************************************************
 * ShellExecuteEx32A [SHELL32.292]
 *
 */
BOOL32 WINAPI ShellExecuteEx32A (LPSHELLEXECUTEINFO32A sei)
{ 	CHAR szApplicationName[MAX_PATH],szCommandline[MAX_PATH],szPidl[20];
	LPSTR pos;
	int gap, len;
	STARTUPINFO32A  startupinfo;
	PROCESS_INFORMATION processinformation;
			
  	FIXME(shell,"mask=0x%08lx hwnd=0x%04x verb=%s file=%s parm=%s dir=%s show=0x%08x class=%sstub\n",
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
	{ SHGetPathFromIDList32A (sei->lpIDList,szApplicationName);
	  FIXME(shell,"-- idlist=%p (%s)\n", sei->lpIDList, szApplicationName);
	}
	else
	{ if (sei->fMask & SEE_MASK_IDLIST )
	  { /* %I is the adress of a global item ID*/
	    pos = strstr(szCommandline, "%I");
	    if (pos)
	    { HGLOBAL32 hmem = SHAllocShared ( sei->lpIDList, ILGetSize(sei->lpIDList), 0);
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

	FIXME(shell,"-- %s\n",szCommandline);

	ZeroMemory(&startupinfo,sizeof(STARTUPINFO32A));
	startupinfo.cb = sizeof(STARTUPINFO32A);

	CreateProcess32A(szApplicationName[0] ? szApplicationName:NULL,
			 szCommandline[0] ? szCommandline : NULL,
			 NULL, NULL, FALSE, 0, 
			 NULL, NULL, &startupinfo, &processinformation);
	  
	
	return 0;
}
/*************************************************************************
 * ShellExecuteEx [SHELL32.293]
 *
 */
BOOL32 WINAPI ShellExecuteEx32W (LPSHELLEXECUTEINFO32W sei)
{ 	WCHAR szTemp[MAX_PATH];

  	FIXME(shell,"(%p): stub\n",sei);

	if (sei->fMask & SEE_MASK_IDLIST)
	{ SHGetPathFromIDList32W (sei->lpIDList,szTemp);
	  TRACE (shell,"-- idlist=%p (%s)\n", sei->lpIDList, debugstr_w(szTemp));
	}

	if (sei->fMask & SEE_MASK_CLASSNAME)
	{ TRACE (shell,"-- classname= %s\n", debugstr_w(sei->lpClass));
	}
    
	if (sei->lpVerb)
	{ TRACE (shell,"-- action=%s\n", debugstr_w(sei->lpVerb));
	}
	
	return 0;
}
static LPUNKNOWN SHELL32_IExplorerInterface=0;
/*************************************************************************
 * SHSetInstanceExplorer [SHELL32.176]
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
 * SHGetInstanceExplorer [SHELL32.256]
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
 * SHFreeUnusedLibraries [SHELL32.123]
 *
 * NOTES
 *  exported by name
 */
HRESULT WINAPI SHFreeUnusedLibraries (void)
{	FIXME(shell,"stub\n");
	return TRUE;
}
/*************************************************************************
 * DAD_ShowDragImage [SHELL32.137]
 *
 * NOTES
 *  exported by name
 */
HRESULT WINAPI DAD_ShowDragImage (DWORD u)
{ FIXME(shell,"0x%08lx stub\n",u);
  return 0;
}
/*************************************************************************
 * FileMenu_Destroy [SHELL32.118]
 *
 * NOTES
 *  exported by name
 */
HRESULT WINAPI FileMenu_Destroy (DWORD u)
{ FIXME(shell,"0x%08lx stub\n",u);
  return 0;
}
/*************************************************************************
 * SHGetDataFromIDListA [SHELL32.247]
 *
 */
HRESULT WINAPI SHGetDataFromIDListA(DWORD u, DWORD v, DWORD w, DWORD x, DWORD y)
{	FIXME(shell,"0x%04lx 0x%04lx 0x%04lx 0x%04lx 0x%04lx stub\n",u,v,w,x,y);
	return 0;
}
/*************************************************************************
 * SHRegCloseKey32 [NT4.0:SHELL32.505]
 *
 */
HRESULT WINAPI SHRegCloseKey32 (HKEY hkey)
{	TRACE(shell,"0x%04x\n",hkey);
	return RegCloseKey( hkey );
}
/*************************************************************************
 * SHRegOpenKey32A [SHELL32.506]
 *
 */
HRESULT WINAPI SHRegOpenKey32A(HKEY hKey, LPSTR lpSubKey, LPHKEY phkResult)
{	FIXME(shell,"(0x%08x, %s, %p)\n", hKey, debugstr_a(lpSubKey),
	      phkResult);
	return RegOpenKey32A(hKey, lpSubKey, phkResult);
}

/*************************************************************************
 * SHRegOpenKey32W [NT4.0:SHELL32.507]
 *
 */
HRESULT WINAPI SHRegOpenKey32W (HKEY hkey, LPCWSTR lpszSubKey, LPHKEY retkey)
{	WARN(shell,"0x%04x %s %p\n",hkey,debugstr_w(lpszSubKey),retkey);
	return RegOpenKey32W( hkey, lpszSubKey, retkey );
}
/*************************************************************************
 * SHRegQueryValueExA [SHELL32.509]
 *
 */
HRESULT WINAPI SHRegQueryValueEx32A(DWORD u, LPSTR v, DWORD w, DWORD x,
				  DWORD y, DWORD z)
{	FIXME(shell,"0x%04lx %s 0x%04lx 0x%04lx 0x%04lx  0x%04lx stub\n",
		u,debugstr_a(v),w,x,y,z);
	return 0;
}
/*************************************************************************
 * SHRegQueryValue32W [NT4.0:SHELL32.510]
 *
 */
HRESULT WINAPI SHRegQueryValue32W (HKEY hkey, LPWSTR lpszSubKey,
				 LPWSTR lpszData, LPDWORD lpcbData )
{	WARN(shell,"0x%04x %s %p %p semi-stub\n",
		hkey, debugstr_w(lpszSubKey), lpszData, lpcbData);
	return RegQueryValue32W( hkey, lpszSubKey, lpszData, lpcbData );
}

/*************************************************************************
 * SHRegQueryValueEx32W [NT4.0:SHELL32.511]
 *
 * FIXME 
 *  if the datatype REG_EXPAND_SZ then expand the string and change
 *  *pdwType to REG_SZ. 
 */
HRESULT WINAPI SHRegQueryValueEx32W (HKEY hkey, LPWSTR pszValue, LPDWORD pdwReserved,
		 LPDWORD pdwType, LPVOID pvData, LPDWORD pcbData)
{	DWORD ret;
	WARN(shell,"0x%04x %s %p %p %p %p semi-stub\n",
		hkey, debugstr_w(pszValue), pdwReserved, pdwType, pvData, pcbData);
	ret = RegQueryValueEx32W ( hkey, pszValue, pdwReserved, pdwType, pvData, pcbData);
	return ret;
}

/*************************************************************************
 * ReadCabinetState [NT 4.0:SHELL32.651]
 *
 */
HRESULT WINAPI ReadCabinetState(DWORD u, DWORD v)
{	FIXME(shell,"0x%04lx 0x%04lx stub\n",u,v);
	return 0;
}
/*************************************************************************
 * WriteCabinetState [NT 4.0:SHELL32.652]
 *
 */
HRESULT WINAPI WriteCabinetState(DWORD u)
{	FIXME(shell,"0x%04lx stub\n",u);
	return 0;
}
/*************************************************************************
 * FileIconInit [SHELL32.660]
 *
 */
BOOL32 WINAPI FileIconInit(BOOL32 bFullInit)
{	FIXME(shell,"(%s)\n", bFullInit ? "true" : "false");
	return 0;
}
/*************************************************************************
 * IsUserAdmin [NT 4.0:SHELL32.680]
 *
 */
HRESULT WINAPI IsUserAdmin(void)
{	FIXME(shell,"stub\n");
	return TRUE;
}
/*************************************************************************
 * StrRetToStrN [SHELL32.96]
 * 
 * converts a STRRET to a normal string
 *
 * NOTES
 *  FIXME the string handling is to simple (different STRRET choices)
 *  at the moment only CSTR
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
 * StrChrW [NT 4.0:SHELL32.651]
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
 *	StrCmpNIW		[NT 4.0:SHELL32.*]
 *
 */
INT32 WINAPI StrCmpNIW ( LPWSTR wstr1, LPWSTR wstr2, INT32 len)
{	FIXME( shell,"%s %s %i stub\n", debugstr_w(wstr1),debugstr_w(wstr2),len);
	return 0;
}

/*************************************************************************
 * SHAllocShared [SHELL32.520]
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
HGLOBAL32 WINAPI SHAllocShared(LPVOID psrc, DWORD size, DWORD procID)
{	HGLOBAL32 hmem;
	LPVOID pmem;
	
	TRACE(shell,"ptr=%p size=0x%04lx procID=0x%04lx\n",psrc,size,procID);
	hmem = GlobalAlloc32(GMEM_FIXED, size);
	if (!hmem)
	  return 0;
	
	pmem =  GlobalLock32 (hmem);

	if (! pmem)
	  return 0;
	  
	memcpy (pmem, psrc, size);
	GlobalUnlock32(hmem); 
	return hmem;
}
/*************************************************************************
 * SHLockShared [SHELL32.521]
 *
 * NOTES
 *  parameter1 is return value from SHAllocShared
 *  parameter2 is return value from GetCurrentProcessId
 *  the receiver of (WM_USER+2) trys to lock the HANDLE (?) 
 *  the returnvalue seems to be a memoryadress
 */
LPVOID WINAPI SHLockShared(HANDLE32 hmem, DWORD procID)
{	TRACE(shell,"handle=0x%04x procID=0x%04lx\n",hmem,procID);
	return GlobalLock32(hmem);
}
/*************************************************************************
 * SHUnlockShared [SHELL32.522]
 *
 * NOTES
 *  parameter1 is return value from SHLockShared
 */
BOOL32 WINAPI SHUnlockShared(HANDLE32 pmem)
{	TRACE(shell,"handle=0x%04x\n",pmem);
	return GlobalUnlock32(pmem); 
}
/*************************************************************************
 * SHFreeShared [SHELL32.523]
 *
 * NOTES
 *  parameter1 is return value from SHAllocShared
 *  parameter2 is return value from GetCurrentProcessId
 */
HANDLE32 WINAPI SHFreeShared(HANDLE32 hmem, DWORD procID)
{	TRACE(shell,"handle=0x%04x 0x%04lx\n",hmem,procID);
	return GlobalFree32(hmem);
}

/*************************************************************************
 * SetAppStartingCursor32 [SHELL32.99]
 *
 */
HRESULT WINAPI SetAppStartingCursor32(DWORD u, DWORD v)
{	FIXME(shell,"0x%04lx 0x%04lx stub\n",u,v );
	return 0;
}
/*************************************************************************
 * SHLoadOLE32 [SHELL32.151]
 *
 */
HRESULT WINAPI SHLoadOLE32(DWORD u)
{	FIXME(shell,"0x%04lx stub\n",u);
	return S_OK;
}
/*************************************************************************
 * Shell_MergeMenus32 [SHELL32.67]
 *
 */
BOOL32 _SHIsMenuSeparator(HMENU32 hm, int i)
{
	MENUITEMINFO32A mii;

	mii.cbSize = sizeof(MENUITEMINFO32A);
	mii.fMask = MIIM_TYPE;
	mii.cch = 0;    /* WARNING: We MUST initialize it to 0*/
	if (!GetMenuItemInfo32A(hm, i, TRUE, &mii))
	{ return(FALSE);
	}

	if (mii.fType & MFT_SEPARATOR)
	{ return(TRUE);
	}

        return(FALSE);
}
#define MM_ADDSEPARATOR         0x00000001L
#define MM_SUBMENUSHAVEIDS      0x00000002L
HRESULT WINAPI Shell_MergeMenus32 (HMENU32 hmDst, HMENU32 hmSrc, UINT32 uInsert, UINT32 uIDAdjust, UINT32 uIDAdjustMax, ULONG uFlags)
{	int		nItem;
	HMENU32		hmSubMenu;
	BOOL32		bAlreadySeparated;
	MENUITEMINFO32A miiSrc;
	char		szName[256];
	UINT32		uTemp, uIDMax = uIDAdjust;

	FIXME(shell,"hmenu1=0x%04x hmenu2=0x%04x 0x%04x 0x%04x 0x%04x  0x%04lx stub\n",
		 hmDst, hmSrc, uInsert, uIDAdjust, uIDAdjustMax, uFlags);

	if (!hmDst || !hmSrc)
	{ return uIDMax;
	}

	nItem = GetMenuItemCount32(hmDst);
	if (uInsert >= (UINT32)nItem)
	{ uInsert = (UINT32)nItem;
	  bAlreadySeparated = TRUE;
	}
	else
	{ bAlreadySeparated = _SHIsMenuSeparator(hmDst, uInsert);;
	}
	if ((uFlags & MM_ADDSEPARATOR) && !bAlreadySeparated)
	{ /* Add a separator between the menus */
	  InsertMenu32A(hmDst, uInsert, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
	  bAlreadySeparated = TRUE;
        }


        /* Go through the menu items and clone them*/
        for (nItem = GetMenuItemCount32(hmSrc) - 1; nItem >= 0; nItem--)
        { miiSrc.cbSize = sizeof(MENUITEMINFO32A);
	  miiSrc.fMask = MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_CHECKMARKS | MIIM_TYPE | MIIM_DATA;
	  /* We need to reset this every time through the loop in case
	  menus DON'T have IDs*/
	  miiSrc.fType = MFT_STRING;
	  miiSrc.dwTypeData = szName;
	  miiSrc.dwItemData = 0;
	  miiSrc.cch = sizeof(szName);

	  if (!GetMenuItemInfo32A(hmSrc, nItem, TRUE, &miiSrc))
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
	    miiSrc.hSubMenu = CreatePopupMenu32();
	    if (!miiSrc.hSubMenu)
	    { return(uIDMax);
	    }
	    uTemp = Shell_MergeMenus32(miiSrc.hSubMenu, hmSubMenu, 0, uIDAdjust, uIDAdjustMax, uFlags&MM_SUBMENUSHAVEIDS);
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
	  if (!InsertMenuItem32A(hmDst, uInsert, TRUE, &miiSrc))
	  { return(uIDMax);
	  }
	}

	/* Ensure the correct number of separators at the beginning of the
        inserted menu items*/
        if (uInsert == 0)
        { if (bAlreadySeparated)
	  { DeleteMenu32(hmDst, uInsert, MF_BYPOSITION);
	  }
	}
	else
	{ if (_SHIsMenuSeparator(hmDst, uInsert-1))
	  { if (bAlreadySeparated)
	    { DeleteMenu32(hmDst, uInsert, MF_BYPOSITION);
	    }
	  }
	  else
	  { if ((uFlags & MM_ADDSEPARATOR) && !bAlreadySeparated)
	    { /* Add a separator between the menus*/
	      InsertMenu32A(hmDst, uInsert, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
	    }
	  }
	}
        return(uIDMax);

}
/*************************************************************************
 * DriveType32 [SHELL32.64]
 *
 */
HRESULT WINAPI DriveType32(DWORD u)
{	FIXME(shell,"0x%04lx stub\n",u);
	return 0;
}
/*************************************************************************
 * SHAbortInvokeCommand [SHELL32.198]
 *
 */
HRESULT WINAPI SHAbortInvokeCommand(void)
{	FIXME(shell,"stub\n");
	return 1;
}
/*************************************************************************
 * SHOutOfMemoryMessageBox [SHELL32.126]
 *
 */
HRESULT WINAPI SHOutOfMemoryMessageBox(DWORD u, DWORD v, DWORD w)
{	FIXME(shell,"0x%04lx 0x%04lx 0x%04lx stub\n",u,v,w);
	return 0;
}
/*************************************************************************
 * SHFlushClipboard [SHELL32.121]
 *
 */
HRESULT WINAPI SHFlushClipboard(void)
{	FIXME(shell,"stub\n");
	return 1;
}
/*************************************************************************
 * StrRChrW [SHELL32.320]
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
*	StrFormatByteSize	[SHLWAPI]
*/
LPSTR WINAPI StrFormatByteSize32A ( DWORD dw, LPSTR pszBuf, UINT32 cchBuf )
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
LPWSTR WINAPI StrFormatByteSize32W ( DWORD dw, LPWSTR pszBuf, UINT32 cchBuf )
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
 * SHWaitForFileToOpen [SHELL32.97]
 *
 */
HRESULT WINAPI SHWaitForFileToOpen(DWORD u, DWORD v, DWORD w)
{	FIXME(shell,"0x%04lx 0x%04lx 0x%04lx stub\n",u,v,w);
	return 0;
}
/*************************************************************************
 * Control_FillCache_RunDLL [SHELL32.8]
 *
 */
HRESULT WINAPI Control_FillCache_RunDLL(DWORD u, DWORD v, DWORD w, DWORD x)
{	FIXME(shell,"0x%04lx 0x%04lx 0x%04lx 0x%04lx stub\n",u,v,w,x);
	return 0;
}
