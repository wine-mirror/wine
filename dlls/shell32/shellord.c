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
#include "debugtools.h"
#include "winnls.h"
#include "winversion.h"
#include "heap.h"

#include "shellapi.h"
#include "shlobj.h"
#include "shell32_main.h"
#include "wine/undocshell.h"
#include "shpolicy.h"

DEFAULT_DEBUG_CHANNEL(shell)

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
{	FIXME("(0x%04x,0x%08lx,0x%08lx,0x%08lx,0x%08x,%p):stub.\n",
		hwnd,events1,events2,msg,count,idlist);
	return 0;
}
/*************************************************************************
 * SHChangeNotifyDeregister			[SHELL32.4]
 */
DWORD WINAPI
SHChangeNotifyDeregister(LONG x1)
{	FIXME("(0x%08lx):stub.\n",x1);
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
{	FIXME("(0x%04x,0x%08lx,0x%08lx,0x%08lx,0x%08x,%p):stub.\n",
		hwnd,events1,events2,msg,count,idlist);
	return 0;
}
/*************************************************************************
 * NTSHChangeNotifyDeregister			[SHELL32.641]
 */
DWORD WINAPI NTSHChangeNotifyDeregister(LONG x1)
{	FIXME("(0x%08lx):stub.\n",x1);
	return 0;
}

/*************************************************************************
 * ParseField					[SHELL32.58]
 *
 */
DWORD WINAPI ParseFieldA(LPCSTR src, DWORD field, LPSTR dst, DWORD len) 
{	WARN("('%s',0x%08lx,%p,%ld) semi-stub.\n",src,field,dst,len);

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
{	FIXME("(%08lx,%08lx,%08lx,%08lx):stub.\n",x,y,z,a);
	return 0xffffffff;
}

/*************************************************************************
 * GetFileNameFromBrowse			[SHELL32.63]
 * 
 */
DWORD WINAPI GetFileNameFromBrowse(HWND howner, LPSTR targetbuf, DWORD len, DWORD x, LPCSTR suffix, LPCSTR y, LPCSTR cmd) 
{	FIXME("(%04x,%p,%ld,%08lx,%s,%s,%s):stub.\n",
	    howner,targetbuf,len,x,suffix,y,cmd);
    /* puts up a Open Dialog and requests input into targetbuf */
    /* OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_FILEMUSTEXIST|OFN_unknown */
    lstrcpyA(targetbuf,"x:\\dummy.exe");
    return 1;
}

/*************************************************************************
 * SHGetSettings				[SHELL32.68]
 * 
 * NOTES
 *  the registry path are for win98 (tested)
 *  and possibly are the same in nt40
 */
void WINAPI SHGetSettings(LPSHELLFLAGSTATE lpsfs, DWORD dwMask, DWORD dwx)
{
	HKEY	hKey;
	DWORD	dwData;
	DWORD	dwDataSize = sizeof (DWORD);

	TRACE("(%p 0x%08lx 0x%08lx)\n",lpsfs,dwMask, dwx);
	
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
int WINAPI SHShellFolderView_Message(HWND hwndCabinet,UINT uMsg,LPARAM lParam)
{ FIXME("%04x %08ux %08lx stub\n",hwndCabinet,uMsg,lParam);
  return 0;
}

/*************************************************************************
 * OleStrToStrN					[SHELL32.78]
 */
BOOL WINAPI OleStrToStrNA (LPSTR lpStr, INT nStr, LPCWSTR lpOle, INT nOle) 
{
	TRACE("%p %x %s %x\n", lpStr, nStr, debugstr_w(lpOle), nOle);
	return WideCharToMultiByte (0, 0, lpOle, nOle, lpStr, nStr, NULL, NULL);
}

BOOL WINAPI OleStrToStrNW (LPWSTR lpwStr, INT nwStr, LPCWSTR lpOle, INT nOle) 
{
	TRACE("%p %x %s %x\n", lpwStr, nwStr, debugstr_w(lpOle), nOle);

	if (lstrcpynW ( lpwStr, lpOle, nwStr))
	{ return lstrlenW (lpwStr);
	}
	return 0;
}

BOOL WINAPI OleStrToStrNAW (LPVOID lpOut, INT nOut, LPCVOID lpIn, INT nIn) 
{
	if (VERSION_OsIsUnicode())
	  return OleStrToStrNW (lpOut, nOut, lpIn, nIn);
	return OleStrToStrNA (lpOut, nOut, lpIn, nIn);
}

/*************************************************************************
 * StrToOleStrN					[SHELL32.79]
 *  lpMulti, nMulti, nWide [IN]
 *  lpWide [OUT]
 */
BOOL WINAPI StrToOleStrNA (LPWSTR lpWide, INT nWide, LPCSTR lpStrA, INT nStr) 
{
	TRACE("%p %x %s %x\n", lpWide, nWide, lpStrA, nStr);
	return MultiByteToWideChar (0, 0, lpStrA, nStr, lpWide, nWide);
}
BOOL WINAPI StrToOleStrNW (LPWSTR lpWide, INT nWide, LPCWSTR lpStrW, INT nStr) 
{
	TRACE("%p %x %s %x\n", lpWide, nWide, debugstr_w(lpStrW), nStr);

	if (lstrcpynW (lpWide, lpStrW, nWide))
	{ return lstrlenW (lpWide);
	}
	return 0;
}

BOOL WINAPI StrToOleStrNAW (LPWSTR lpWide, INT nWide, LPCVOID lpStr, INT nStr) 
{
	if (VERSION_OsIsUnicode())
	  return StrToOleStrNW (lpWide, nWide, lpStr, nStr);
	return StrToOleStrNA (lpWide, nWide, lpStr, nStr);
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
    FIXME("(0x%08x,0x%08lx):stub.\n",hwnd,y);
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

	TRACE("(%08lx,%08lx,%08lx,%08lx,%08lx,%p)\n",(DWORD)hmod,(DWORD)hwnd,idText,idTitle,uType,arglist);

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

	TRACE("(%08lx,%08lx,%08lx,%08lx,%08lx,%p)\n", (DWORD)hmod,(DWORD)hwnd,idText,idTitle,uType,arglist);

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
 * queried (DWORD) value, and caches it between called to SHInitRestricted
 * to prevent unnecessary registry access.
 *
 * NOTES
 *     exported by ordinal
 *
 * REFERENCES: 
 *     MS System Policy Editor
 *     98Lite 2.0 (which uses many of these policy keys) http://www.98lite.net/
 *     "The Windows 95 Registry", by John Woram, 1996 MIS: Press
 */
DWORD WINAPI SHRestricted (DWORD pol) {
        char regstr[256];
	HKEY	xhkey;
	DWORD   retval, polidx, i, datsize = 4;

	TRACE("(%08lx)\n",pol);

	polidx = -1;

	/* scan to see if we know this policy ID */
	for (i = 0; i < SHELL_MAX_POLICIES; i++)
	{
	     if (pol == sh32_policy_table[i].polflags)
	     {
	         polidx = i;
		 break;
	     }
	}

	if (polidx == -1)
	{
	    /* we don't know this policy, return 0 */
	    TRACE("unknown policy: (%08lx)\n", pol);
		return 0;
	}

	/* we have a known policy */
      	lstrcpyA(regstr, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\");
	lstrcatA(regstr, sh32_policy_table[polidx].appstr);

	/* first check if this policy has been cached, return it if so */
	if (sh32_policy_table[polidx].cache != SHELL_NO_POLICY)
	{
	    return sh32_policy_table[polidx].cache;
	}

	/* return 0 and don't set the cache if any registry errors occur */
	retval = 0;
	if (RegOpenKeyA(HKEY_CURRENT_USER, regstr, &xhkey) == ERROR_SUCCESS)
	{
	    if (RegQueryValueExA(xhkey, sh32_policy_table[polidx].keystr, NULL, NULL, (LPBYTE)&retval, &datsize) == ERROR_SUCCESS)
	    {
	        sh32_policy_table[polidx].cache = retval;
	    }

	RegCloseKey(xhkey);
}

	return retval;
}

/*************************************************************************
 *      SHInitRestricted                         [SHELL32.244]
 *
 * Win98+ by-ordinal only routine called by Explorer and MSIE 4 and 5.
 * Inits the policy cache used by SHRestricted to avoid excess
 * registry access.
 *
 * INPUTS
 * Two inputs: one is a string or NULL.  If non-NULL the pointer
 * should point to a string containing the following exact text:
 * "Software\Microsoft\Windows\CurrentVersion\Policies".
 * The other input is unused.
 *
 * NOTES
 * If the input is non-NULL and does not point to a string containing
 * that exact text the routine will do nothing.
 *
 * If the text does match or the pointer is NULL, then the routine
 * will init SHRestricted()'s policy cache to all 0xffffffff and
 * returns 0xffffffff as well.
 *
 * I haven't yet run into anything calling this with inputs other than
 * (NULL, NULL), so I may have the inputs reversed.
 */

BOOL WINAPI SHInitRestricted(LPSTR inpRegKey, LPSTR parm2)
{
     int i;

     TRACE("(%p, %p)\n", inpRegKey, parm2);

     /* first check - if input is non-NULL and points to the secret
        key string, then pass.  Otherwise return 0.
     */

     if (inpRegKey != (LPSTR)NULL)
     {
         if (lstrcmpiA(inpRegKey, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies"))
	 {
	     /* doesn't match, fail */
	     return 0;
	 }
     }                               

     /* check passed, init all policy cache entries with SHELL_NO_POLICY */
     for (i = 0; i < SHELL_MAX_POLICIES; i++)
     {
          sh32_policy_table[i].cache = SHELL_NO_POLICY;
     }

     return SHELL_NO_POLICY;
}

/*************************************************************************
 * SHCreateDirectory				[SHELL32.165]
 *
 * NOTES
 *  exported by ordinal
 *  not sure about LPSECURITY_ATTRIBUTES
 */
DWORD WINAPI SHCreateDirectory(LPSECURITY_ATTRIBUTES sec,LPCSTR path) {
	TRACE("(%p,%s):stub.\n",sec,path);
	if (CreateDirectoryA(path,sec))
		return TRUE;
	/* SHChangeNotify(8,1,path,0); */
	return FALSE;
#if 0
	if (SHELL32_79(path,(LPVOID)x))
		return 0;
	FIXME("(%08lx,%s):stub.\n",x,path);
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
/*#define MEM_DEBUG 1*/
DWORD WINAPI SHFree(LPVOID x) 
{
#ifdef MEM_DEBUG
	WORD len = *(LPWORD)(x-2);

	if ( *(LPWORD)(x+len) != 0x7384)
	  ERR("MAGIC2!\n");

	if ( (*(LPWORD)(x-4)) != 0x8271)
	  ERR("MAGIC1!\n");
	else
	  memset(x-4, 0xde, len+6);

	TRACE("%p len=%u\n",x, len);

	x -= 4;
#else
	TRACE("%p\n",x);
#endif
	return HeapFree(GetProcessHeap(), 0, x);
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

#ifdef MEM_DEBUG
	ret = (LPVOID) HeapAlloc(GetProcessHeap(),0,len+6);
#else
	ret = (LPVOID) HeapAlloc(GetProcessHeap(),0,len);
#endif

#ifdef MEM_DEBUG
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
DWORD WINAPI SHRegisterDragDrop(HWND hWnd,IDropTarget * pDropTarget) 
{
	FIXME("(0x%08x,%p):stub.\n", hWnd, pDropTarget);
	return     RegisterDragDrop(hWnd, pDropTarget);
}

/*************************************************************************
 * SHRevokeDragDrop				[SHELL32.87]
 *
 * NOTES
 *     exported by ordinal
 */
DWORD WINAPI SHRevokeDragDrop(DWORD x) {
    FIXME("(0x%08lx):stub.\n",x);
    return 0;
}

/*************************************************************************
 * SHDoDragDrop					[SHELL32.88]
 *
 * NOTES
 *     exported by ordinal
 */
DWORD WINAPI SHDoDragDrop(DWORD u, DWORD v, DWORD w, DWORD x, DWORD y, DWORD z) {
    FIXME("(0x%08lx 0x%08lx 0x%08lx 0x%08lx 0x%08lx 0x%08lx):stub.\n",u,v,w,x,y,z);
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
    FIXME("(0x%08x 0x%lx 0x%lx \"%s\" \"%s\" 0x%x):stub.\n",
	   hwndOwner, dwParam1, dwParam2, lpszTitle, lpszPrompt, uFlags);
    return 0;
}

/*************************************************************************
 * ExitWindowsDialog				[SHELL32.60]
 *
 * NOTES
 *     exported by ordinal
 */
void WINAPI ExitWindowsDialog (HWND hWndOwner)
{
	TRACE("(0x%08x)\n", hWndOwner);
	if (MessageBoxA( hWndOwner, "Do you want to exit WINE?", "Shutdown", MB_YESNO|MB_ICONQUESTION) == IDOK)
	{ SendMessageA ( hWndOwner, WM_QUIT, 0, 0);
	}
}

/*************************************************************************
 * ArrangeWindows				[SHELL32.184]
 * 
 */
DWORD WINAPI
ArrangeWindows (DWORD dwParam1, DWORD dwParam2, DWORD dwParam3,
		DWORD dwParam4, DWORD dwParam5)
{
    FIXME("(0x%lx 0x%lx 0x%lx 0x%lx 0x%lx):stub.\n",
	   dwParam1, dwParam2, dwParam3, dwParam4, dwParam5);
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
 * SHFileOperation				[SHELL32.242]
 *
 */
DWORD WINAPI SHFileOperationAW(DWORD x)
{	FIXME("0x%08lx stub\n",x);
	return 0;

}

/*************************************************************************
 * SHFileOperationA				[SHELL32.243]
 *
 * NOTES
 *     exported by name
 */
DWORD WINAPI SHFileOperationA (LPSHFILEOPSTRUCTA lpFileOp)   
{ FIXME("(%p):stub.\n", lpFileOp);
  return 1;
}
/*************************************************************************
 * SHFileOperationW				[SHELL32.244]
 *
 * NOTES
 *     exported by name
 */
DWORD WINAPI SHFileOperationW (LPSHFILEOPSTRUCTW lpFileOp)   
{ FIXME("(%p):stub.\n", lpFileOp);
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
{ FIXME("(0x%08x,0x%08ux,%p,%p):stub.\n", wEventId,uFlags,dwItem1,dwItem2);
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
{
	IShellView * psf;
	HRESULT hRes;
	
	TRACE("sf=%p pidl=%p cb=%p mode=0x%08lx parm=0x%08lx\n", 
	  psvcbi->pShellFolder, psvcbi->pidl, psvcbi->pCallBack, psvcbi->viewmode, psvcbi->dwUserParam);

	psf = IShellView_Constructor(psvcbi->pShellFolder);
	
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
			  SEE_MASK_NOCLOSEPROCESS | SEE_MASK_CONNECTNETDRV | SEE_MASK_FLAG_DDEWAIT |
			  SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_NO_UI | SEE_MASK_UNICODE | 
			  SEE_MASK_NO_CONSOLE | SEE_MASK_ASYNCOK | SEE_MASK_HMONITOR ))
	{ FIXME("flags ignored: 0x%08lx\n", sei->fMask);
	}

	if (sei->fMask & SEE_MASK_CLASSNAME)
	{ HCR_GetExecuteCommand(sei->lpClass, (sei->lpVerb) ? sei->lpVerb : "open", szCommandline, 256);	    
	}

	/* process the IDList */
	if ( (sei->fMask & SEE_MASK_INVOKEIDLIST) == SEE_MASK_INVOKEIDLIST) /*0x0c*/
	{ SHGetPathFromIDListA (sei->lpIDList,szApplicationName);
	  TRACE("-- idlist=%p (%s)\n", sei->lpIDList, szApplicationName);
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

	TRACE("execute: %s %s\n",szApplicationName, szCommandline);

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
HRESULT WINAPI SHFreeUnusedLibraries (void)
{	FIXME("stub\n");
	return TRUE;
}
/*************************************************************************
 * DAD_SetDragImage				[SHELL32.136]
 *
 * NOTES
 *  exported by name
 */
HRESULT WINAPI DAD_SetDragImage (DWORD u, DWORD v)
{ FIXME("0x%08lx 0x%08lx stub\n",u, v);
  return 0;
}
/*************************************************************************
 * DAD_ShowDragImage				[SHELL32.137]
 *
 * NOTES
 *  exported by name
 */
HRESULT WINAPI DAD_ShowDragImage (DWORD u)
{ FIXME("0x%08lx stub\n",u);
  return 0;
}
/*************************************************************************
 * SHRegCloseKey			[NT4.0:SHELL32.505]
 *
 */
HRESULT WINAPI SHRegCloseKey (HKEY hkey)
{	TRACE("0x%04x\n",hkey);
	return RegCloseKey( hkey );
}
/*************************************************************************
 * SHRegOpenKeyA				[SHELL32.506]
 *
 */
HRESULT WINAPI SHRegOpenKeyA(HKEY hKey, LPSTR lpSubKey, LPHKEY phkResult)
{
	TRACE("(0x%08x, %s, %p)\n", hKey, debugstr_a(lpSubKey), phkResult);
	return RegOpenKeyA(hKey, lpSubKey, phkResult);
}

/*************************************************************************
 * SHRegOpenKeyW				[NT4.0:SHELL32.507]
 *
 */
HRESULT WINAPI SHRegOpenKeyW (HKEY hkey, LPCWSTR lpszSubKey, LPHKEY retkey)
{	WARN("0x%04x %s %p\n",hkey,debugstr_w(lpszSubKey),retkey);
	return RegOpenKeyW( hkey, lpszSubKey, retkey );
}
/*************************************************************************
 * SHRegQueryValueExA				[SHELL32.509]
 *
 */
HRESULT WINAPI SHRegQueryValueExA(
	HKEY hkey,
	LPSTR lpValueName,
	LPDWORD lpReserved,
	LPDWORD lpType,
	LPBYTE lpData,
	LPDWORD lpcbData)
{
	TRACE("0x%04x %s %p %p %p %p\n", hkey, lpValueName, lpReserved, lpType, lpData, lpcbData);
	return RegQueryValueExA (hkey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}
/*************************************************************************
 * SHRegQueryValueW				[NT4.0:SHELL32.510]
 *
 */
HRESULT WINAPI SHRegQueryValueW (HKEY hkey, LPWSTR lpszSubKey,
				 LPWSTR lpszData, LPDWORD lpcbData )
{	WARN("0x%04x %s %p %p semi-stub\n",
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
	WARN("0x%04x %s %p %p %p %p semi-stub\n",
		hkey, debugstr_w(pszValue), pdwReserved, pdwType, pvData, pcbData);
	ret = RegQueryValueExW ( hkey, pszValue, pdwReserved, pdwType, pvData, pcbData);
	return ret;
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
 * StrRetToStrN					[SHELL32.96]
 * 
 * converts a STRRET to a normal string
 *
 * NOTES
 *  the pidl is for STRRET OFFSET
 */
HRESULT WINAPI StrRetToBufA (LPSTRRET src, LPITEMIDLIST pidl, LPSTR dest, DWORD len)
{
	return StrRetToStrNA(dest, len, src, pidl);
}

HRESULT WINAPI StrRetToStrNA (LPVOID dest, DWORD len, LPSTRRET src, LPITEMIDLIST pidl)
{
	TRACE("dest=0x%p len=0x%lx strret=0x%p pidl=%p stub\n",dest,len,src,pidl);

	switch (src->uType)
	{
	  case STRRET_WSTR:
	    WideCharToMultiByte(CP_ACP, 0, src->u.pOleStr, -1, (LPSTR)dest, len, NULL, NULL);
	    SHFree(src->u.pOleStr);
	    break;

	  case STRRET_CSTRA:
	    lstrcpynA((LPSTR)dest, src->u.cStr, len);
	    break;

	  case STRRET_OFFSETA:
	    lstrcpynA((LPSTR)dest, ((LPCSTR)&pidl->mkid)+src->u.uOffset, len);
	    break;

	  default:
	    FIXME("unknown type!\n");
	    if (len)
	    {
	      *(LPSTR)dest = '\0';
	    }
	    return(FALSE);
	}
	return S_OK;
}

HRESULT WINAPI StrRetToBufW (LPSTRRET src, LPITEMIDLIST pidl, LPWSTR dest, DWORD len)
{
	return StrRetToStrNW(dest, len, src, pidl);
}

HRESULT WINAPI StrRetToStrNW (LPVOID dest, DWORD len, LPSTRRET src, LPITEMIDLIST pidl)
{
	TRACE("dest=0x%p len=0x%lx strret=0x%p pidl=%p stub\n",dest,len,src,pidl);

	switch (src->uType)
	{
	  case STRRET_WSTR:
	    lstrcpynW((LPWSTR)dest, src->u.pOleStr, len);
	    SHFree(src->u.pOleStr);
	    break;

	  case STRRET_CSTRA:
	    lstrcpynAtoW((LPWSTR)dest, src->u.cStr, len);
	    break;

	  case STRRET_OFFSETA:
	    if (pidl)
	    {
	      lstrcpynAtoW((LPWSTR)dest, ((LPCSTR)&pidl->mkid)+src->u.uOffset, len);
	    }
	    break;

	  default:
	    FIXME("unknown type!\n");
	    if (len)
	    { *(LPSTR)dest = '\0';
	    }
	    return(FALSE);
	}
	return S_OK;
}
HRESULT WINAPI StrRetToStrNAW (LPVOID dest, DWORD len, LPSTRRET src, LPITEMIDLIST pidl)
{
	if(VERSION_OsIsUnicode())
	  return StrRetToStrNW (dest, len, src, pidl);
	return StrRetToStrNA (dest, len, src, pidl);
}

/*************************************************************************
 * StrChrW					[NT 4.0:SHELL32.651]
 *
 */
LPWSTR WINAPI StrChrW (LPWSTR str, WCHAR x )
{	LPWSTR ptr=str;
	
	TRACE("%s 0x%04x\n",debugstr_w(str),x);
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
{	FIXME("%s %s %i stub\n", debugstr_w(wstr1),debugstr_w(wstr2),len);
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
BOOL WINAPI SHUnlockShared(HANDLE pmem)
{	TRACE("handle=0x%04x\n",pmem);
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
{	TRACE("handle=0x%04x 0x%04lx\n",hmem,procID);
	return GlobalFree(hmem);
}

/*************************************************************************
 * SetAppStartingCursor				[SHELL32.99]
 *
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
HRESULT WINAPI SHOutOfMemoryMessageBox(DWORD u, DWORD v, DWORD w)
{	FIXME("0x%04lx 0x%04lx 0x%04lx stub\n",u,v,w);
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
 * StrRChrA					[SHELL32.346]
 *
 */
LPSTR WINAPI StrRChrA(LPCSTR lpStart, LPCSTR lpEnd, DWORD wMatch)
{
    	if (!lpStart)
	    return NULL;

	/* if the end not given, search*/
	if (!lpEnd)
	{ lpEnd=lpStart;
	  while (*lpEnd) 
	    lpEnd++;
	}

	for (--lpEnd;lpStart <= lpEnd; lpEnd--)
	    if (*lpEnd==(char)wMatch)
		return (LPSTR)lpEnd;

	return NULL;
}
/*************************************************************************
 * StrRChrW					[SHELL32.320]
 *
 */
LPWSTR WINAPI StrRChrW(LPWSTR lpStart, LPWSTR lpEnd, DWORD wMatch)
{	LPWSTR wptr=NULL;
	TRACE("%s %s 0x%04x\n",debugstr_w(lpStart),debugstr_w(lpEnd), (WCHAR)wMatch );

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
	TRACE("%lx %p %i\n", dw, pszBuf, cchBuf);
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
	lstrcpynA (pszBuf, buf, cchBuf);
	return pszBuf;	
}
LPWSTR WINAPI StrFormatByteSizeW ( DWORD dw, LPWSTR pszBuf, UINT cchBuf )
{	char buf[64];
	TRACE("%lx %p %i\n", dw, pszBuf, cchBuf);
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
{	FIXME("0x%04lx 0x%04lx 0x%04lx stub\n",u,v,w);
	return 0;
}
/*************************************************************************
 * Control_FillCache_RunDLL			[SHELL32.8]
 *
 */
HRESULT WINAPI Control_FillCache_RunDLL(HWND hWnd, HANDLE hModule, DWORD w, DWORD x)
{	FIXME("0x%04x 0x%04x 0x%04lx 0x%04lx stub\n",hWnd, hModule,w,x);
	return 0;
}
/*************************************************************************
 * RunDLL_CallEntry16				[SHELL32.122]
 * the name is propably wrong
 */
HRESULT WINAPI RunDLL_CallEntry16(DWORD v, DWORD w, DWORD x, DWORD y, DWORD z)
{	FIXME("0x%04lx 0x%04lx 0x%04lx 0x%04lx 0x%04lx stub\n",v,w,x,y,z);
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
 *	StrToOleStr			[SHELL32.163]
 *
 */
int WINAPI StrToOleStrA (LPWSTR lpWideCharStr, LPCSTR lpMultiByteString)
{
	TRACE("%p %p(%s)\n",
	lpWideCharStr, lpMultiByteString, lpMultiByteString);

	return MultiByteToWideChar(0, 0, lpMultiByteString, -1, lpWideCharStr, MAX_PATH);

}
int WINAPI StrToOleStrW (LPWSTR lpWideCharStr, LPCWSTR lpWString)
{
	TRACE("%p %p(%s)\n",
	lpWideCharStr, lpWString, debugstr_w(lpWString));

	if (lstrcpyW (lpWideCharStr, lpWString ))
	{ return lstrlenW (lpWideCharStr);
	}
	return 0;
}

BOOL WINAPI StrToOleStrAW (LPWSTR lpWideCharStr, LPCVOID lpString)
{
	if (VERSION_OsIsUnicode())
	  return StrToOleStrW (lpWideCharStr, lpString);
	return StrToOleStrA (lpWideCharStr, lpString);
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
 *	DoEnvironmentSubstW			[SHELL32.53]
 *
 */
HRESULT WINAPI DoEnvironmentSubstA(LPSTR x, LPSTR y)
{
	FIXME("%p(%s) %p(%s) stub\n", x, x, y, y);
	return 0;
}

HRESULT WINAPI DoEnvironmentSubstW(LPWSTR x, LPWSTR y)
{
	FIXME("%p(%s) %p(%s) stub\n", x, debugstr_w(x), y, debugstr_w(y));
	return 0;
}

HRESULT WINAPI DoEnvironmentSubstAW(LPVOID x, LPVOID y)
{
	if (VERSION_OsIsUnicode())
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
