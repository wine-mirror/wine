/*
 *	Shell Folder stuff (...and all the OLE-Objects of SHELL32.DLL)
 *
 *	Copyright 1997	Marcus Meissner
 *	Copyright 1998	Juergen Schmied
 *
 *  !!! currently work in progress on all classes !!!
 *  <contact juergen.schmied@metronet.de, 980801>
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "ole.h"
#include "ole2.h"
#include "debug.h"
#include "compobj.h"
#include "interfaces.h"
#include "shlobj.h"
#include "shell.h"
#include "winerror.h"
#include "winnls.h"
#include "winproc.h"
#include "commctrl.h"
#include "pidl.h"

#include "shell32_main.h"


/******************************************************************************
* foreward declaration
*/

/* IExtractIcon implementation*/
static HRESULT WINAPI IExtractIcon_QueryInterface(LPEXTRACTICON, REFIID, LPVOID *);
static ULONG WINAPI IExtractIcon_AddRef(LPEXTRACTICON);
static ULONG WINAPI IExtractIcon_AddRef(LPEXTRACTICON);
static ULONG WINAPI IExtractIcon_Release(LPEXTRACTICON);
static HRESULT WINAPI IExtractIcon_GetIconLocation(LPEXTRACTICON, UINT32, LPSTR, UINT32, int *, UINT32 *);
static HRESULT WINAPI IExtractIcon_Extract(LPEXTRACTICON, LPCSTR, UINT32, HICON32 *, HICON32 *, UINT32);

/* IShellLink Implementation */
static HRESULT WINAPI IShellLink_QueryInterface(LPSHELLLINK,REFIID,LPVOID*);
static ULONG WINAPI IShellLink_AddRef(LPSHELLLINK);
static ULONG WINAPI IShellLink_Release(LPSHELLLINK);
static HRESULT WINAPI IShellLink_GetPath(LPSHELLLINK, LPSTR,INT32, WIN32_FIND_DATA32A *, DWORD);
static HRESULT WINAPI IShellLink_GetIDList(LPSHELLLINK, LPITEMIDLIST *);
static HRESULT WINAPI IShellLink_SetIDList(LPSHELLLINK, LPCITEMIDLIST);
static HRESULT WINAPI IShellLink_GetDescription(LPSHELLLINK, LPSTR,INT32);
static HRESULT WINAPI IShellLink_SetDescription(LPSHELLLINK, LPCSTR);
static HRESULT WINAPI IShellLink_GetWorkingDirectory(LPSHELLLINK, LPSTR,INT32);
static HRESULT WINAPI IShellLink_SetWorkingDirectory(LPSHELLLINK, LPCSTR);
static HRESULT WINAPI IShellLink_GetArguments(LPSHELLLINK, LPSTR,INT32);
static HRESULT WINAPI IShellLink_SetArguments(LPSHELLLINK, LPCSTR);
static HRESULT WINAPI IShellLink_GetHotkey(LPSHELLLINK, WORD *);
static HRESULT WINAPI IShellLink_SetHotkey(LPSHELLLINK, WORD);
static HRESULT WINAPI IShellLink_GetShowCmd(LPSHELLLINK, INT32 *);
static HRESULT WINAPI IShellLink_SetShowCmd(LPSHELLLINK, INT32);
static HRESULT WINAPI IShellLink_GetIconLocation(LPSHELLLINK, LPSTR,INT32,INT32 *);
static HRESULT WINAPI IShellLink_SetIconLocation(LPSHELLLINK, LPCSTR,INT32);
static HRESULT WINAPI IShellLink_SetRelativePath(LPSHELLLINK, LPCSTR, DWORD);
static HRESULT WINAPI IShellLink_Resolve(LPSHELLLINK, HWND32, DWORD);
static HRESULT WINAPI IShellLink_SetPath(LPSHELLLINK, LPCSTR);


/***********************************************************************
*   IExtractIcon implementation
*/
static struct IExtractIcon_VTable eivt = 
{ IExtractIcon_QueryInterface,
  IExtractIcon_AddRef,
  IExtractIcon_Release,
  IExtractIcon_GetIconLocation,
  IExtractIcon_Extract
};
/**************************************************************************
*  IExtractIcon_Constructor
*/
LPEXTRACTICON IExtractIcon_Constructor(LPCITEMIDLIST pidl)
{ LPEXTRACTICON ei;
  ei=(LPEXTRACTICON)HeapAlloc(GetProcessHeap(),0,sizeof(IExtractIcon));
  ei->ref=1;
  ei->lpvtbl=&eivt;
  ei->pidl=ILClone(pidl);
  
  TRACE(shell,"(%p)\n",ei);
  return ei;
}
/**************************************************************************
 *  IExtractIcon_QueryInterface
 */
static HRESULT WINAPI IExtractIcon_QueryInterface( LPEXTRACTICON this, REFIID riid, LPVOID *ppvObj)
{  char	xriid[50];
   WINE_StringFromCLSID((LPCLSID)riid,xriid);
   TRACE(shell,"(%p)->(\n\tIID:\t%s,%p)\n",this,xriid,ppvObj);

  *ppvObj = NULL;

  if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
  { *ppvObj = this; 
  }
  else if(IsEqualIID(riid, &IID_IExtractIcon))  /*IExtractIcon*/
  {    *ppvObj = (IExtractIcon*)this;
  }   

  if(*ppvObj)
  { (*(LPEXTRACTICON*)ppvObj)->lpvtbl->fnAddRef(this);  	
    TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
    return S_OK;
  }
	TRACE(shell,"-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}   

/**************************************************************************
*  IExtractIcon_AddRef
*/
static ULONG WINAPI IExtractIcon_AddRef(LPEXTRACTICON this)
{ TRACE(shell,"(%p)->(count=%lu)\n",this,(this->ref)+1);
  return ++(this->ref);
}
/**************************************************************************
*  IExtractIcon_Release
*/
static ULONG WINAPI IExtractIcon_Release(LPEXTRACTICON this)
{ TRACE(shell,"(%p)->()\n",this);
  if (!--(this->ref)) 
  { TRACE(shell," destroying IExtractIcon(%p)\n",this);
    SHFree(this->pidl);
    HeapFree(GetProcessHeap(),0,this);
    return 0;
  }
  return this->ref;
}
/**************************************************************************
*  IExtractIcon_GetIconLocation
* NOTE
*  FIXME returns allways the icon no. 3 (closed Folder)
*/
static HRESULT WINAPI IExtractIcon_GetIconLocation(LPEXTRACTICON this, UINT32 uFlags, LPSTR szIconFile, UINT32 cchMax, int * piIndex, UINT32 * pwFlags)
{ FIXME (shell,"(%p) (flags=%u file=%s max=%u %p %p) semi-stub\n", this, uFlags, szIconFile, cchMax, piIndex, pwFlags);
	if (!szIconFile)
	{ *piIndex = 20;
	}
	else
	{ *piIndex = 3;
	}
	*pwFlags = GIL_NOTFILENAME;

	return NOERROR;
}
/**************************************************************************
*  IExtractIcon_Extract
*/
static HRESULT WINAPI IExtractIcon_Extract(LPEXTRACTICON this, LPCSTR pszFile, UINT32 nIconIndex, HICON32 *phiconLarge, HICON32 *phiconSmall, UINT32 nIconSize)
{ FIXME (shell,"(%p) (file=%s index=%u %p %p size=%u) semi-stub\n", this, pszFile, nIconIndex, phiconLarge, phiconSmall, nIconSize);
  *phiconLarge = pImageList_GetIcon(ShellBigIconList, nIconIndex, ILD_TRANSPARENT);
  *phiconSmall = pImageList_GetIcon(ShellSmallIconList, nIconIndex, ILD_TRANSPARENT);
  return S_OK;
}

/**************************************************************************
* IShellLink Implementation
*/

static struct IShellLink_VTable slvt = 
{	IShellLink_QueryInterface,
	IShellLink_AddRef,
	IShellLink_Release,
	IShellLink_GetPath,
	IShellLink_GetIDList,
	IShellLink_SetIDList,
	IShellLink_GetDescription,
	IShellLink_SetDescription,
	IShellLink_GetWorkingDirectory,
	IShellLink_SetWorkingDirectory,
	IShellLink_GetArguments,
	IShellLink_SetArguments,
	IShellLink_GetHotkey,
	IShellLink_SetHotkey,
	IShellLink_GetShowCmd,
	IShellLink_SetShowCmd,
	IShellLink_GetIconLocation,
	IShellLink_SetIconLocation,
	IShellLink_SetRelativePath,
	IShellLink_Resolve,
	IShellLink_SetPath
};

/**************************************************************************
 *	  IShellLink_Constructor
 */
LPSHELLLINK IShellLink_Constructor() 
{	LPSHELLLINK sl;

	sl = (LPSHELLLINK)HeapAlloc(GetProcessHeap(),0,sizeof(IShellLink));
	sl->ref = 1;
	sl->lpvtbl = &slvt;
	TRACE(shell,"(%p)->()\n",sl);
	return sl;
}

/**************************************************************************
 *  IShellLink::QueryInterface
 */
static HRESULT WINAPI IShellLink_QueryInterface(
  LPSHELLLINK this, REFIID riid, LPVOID *ppvObj)
{  char    xriid[50];
   WINE_StringFromCLSID((LPCLSID)riid,xriid);
   TRACE(shell,"(%p)->(\n\tIID:\t%s)\n",this,xriid);

  *ppvObj = NULL;

  if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
  { *ppvObj = this; 
  }
  else if(IsEqualIID(riid, &IID_IShellLink))  /*IShellLink*/
  {    *ppvObj = (LPSHELLLINK)this;
  }   

  if(*ppvObj)
  { (*(LPSHELLLINK*)ppvObj)->lpvtbl->fnAddRef(this);      
    TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
    return S_OK;
  }
    TRACE(shell,"-- Interface: E_NOINTERFACE\n");
  return E_NOINTERFACE;
}  
/******************************************************************************
 * IShellLink_AddRef
 */
static ULONG WINAPI IShellLink_AddRef(LPSHELLLINK this)
{ TRACE(shell,"(%p)->(count=%lu)\n",this,this->ref);
    return ++(this->ref);
}
/******************************************************************************
 * IClassFactory_Release
 */
static ULONG WINAPI IShellLink_Release(LPSHELLLINK this)
{ TRACE(shell,"(%p)->(count=%lu)\n",this,this->ref);
  if (!--(this->ref)) 
  { TRACE(shell,"-- destroying IShellLink(%p)\n",this);
    HeapFree(GetProcessHeap(),0,this);
    return 0;
  }
  return this->ref;
}

static HRESULT WINAPI IShellLink_GetPath(LPSHELLLINK this, LPSTR pszFile,INT32 cchMaxPath, WIN32_FIND_DATA32A *pfd, DWORD fFlags)
{	FIXME(shell,"(%p)->(pfile=%p len=%u find_data=%p flags=%lu)\n",this, pszFile, cchMaxPath, pfd, fFlags);
	strncpy(pszFile,"c:\\foo.bar", cchMaxPath);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_GetIDList(LPSHELLLINK this, LPITEMIDLIST * ppidl)
{	FIXME(shell,"(%p)->(ppidl=%p)\n",this, ppidl);
	*ppidl = _ILCreateDesktop();
	return NOERROR;
}
static HRESULT WINAPI IShellLink_SetIDList(LPSHELLLINK this, LPCITEMIDLIST pidl)
{	FIXME(shell,"(%p)->(pidl=%p)\n",this, pidl);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_GetDescription(LPSHELLLINK this, LPSTR pszName,INT32 cchMaxName)
{	FIXME(shell,"(%p)->(%p len=%u)\n",this, pszName, cchMaxName);
	strncpy(pszName,"Description, FIXME",cchMaxName);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_SetDescription(LPSHELLLINK this, LPCSTR pszName)
{	FIXME(shell,"(%p)->(desc=%s)\n",this, pszName);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_GetWorkingDirectory(LPSHELLLINK this, LPSTR pszDir,INT32 cchMaxPath)
{	FIXME(shell,"(%p)->()\n",this);
	strncpy(pszDir,"c:\\", cchMaxPath);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_SetWorkingDirectory(LPSHELLLINK this, LPCSTR pszDir)
{	FIXME(shell,"(%p)->(dir=%s)\n",this, pszDir);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_GetArguments(LPSHELLLINK this, LPSTR pszArgs,INT32 cchMaxPath)
{	FIXME(shell,"(%p)->(%p len=%u)\n",this, pszArgs, cchMaxPath);
	strncpy(pszArgs, "", cchMaxPath);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_SetArguments(LPSHELLLINK this, LPCSTR pszArgs)
{	FIXME(shell,"(%p)->(args=%s)\n",this, pszArgs);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_GetHotkey(LPSHELLLINK this, WORD *pwHotkey)
{	FIXME(shell,"(%p)->(%p)\n",this, pwHotkey);
	*pwHotkey=0x0;
	return NOERROR;
}
static HRESULT WINAPI IShellLink_SetHotkey(LPSHELLLINK this, WORD wHotkey)
{	FIXME(shell,"(%p)->(hotkey=%x)\n",this, wHotkey);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_GetShowCmd(LPSHELLLINK this, INT32 *piShowCmd)
{	FIXME(shell,"(%p)->(%p)\n",this, piShowCmd);
	*piShowCmd=0;
	return NOERROR;
}
static HRESULT WINAPI IShellLink_SetShowCmd(LPSHELLLINK this, INT32 iShowCmd)
{	FIXME(shell,"(%p)->(showcmd=%x)\n",this, iShowCmd);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_GetIconLocation(LPSHELLLINK this, LPSTR pszIconPath,INT32 cchIconPath,INT32 *piIcon)
{	FIXME(shell,"(%p)->(%p len=%u iicon=%p)\n",this, pszIconPath, cchIconPath, piIcon);
	strncpy(pszIconPath,"shell32.dll",cchIconPath);
	*piIcon=1;
	return NOERROR;
}
static HRESULT WINAPI IShellLink_SetIconLocation(LPSHELLLINK this, LPCSTR pszIconPath,INT32 iIcon)
{	FIXME(shell,"(%p)->(path=%s iicon=%u)\n",this, pszIconPath, iIcon);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_SetRelativePath(LPSHELLLINK this, LPCSTR pszPathRel, DWORD dwReserved)
{	FIXME(shell,"(%p)->(path=%s %lx)\n",this, pszPathRel, dwReserved);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_Resolve(LPSHELLLINK this, HWND32 hwnd, DWORD fFlags)
{	FIXME(shell,"(%p)->(hwnd=%x flags=%lx)\n",this, hwnd, fFlags);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_SetPath(LPSHELLLINK this, LPCSTR pszFile)
{	FIXME(shell,"(%p)->(path=%s)\n",this, pszFile);
	return NOERROR;
}

