/*
 *	Copyright 1997	Marcus Meissner
 *	Copyright 1998	Juergen Schmied
 *
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

  pdump(pidl);

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
*/
static HRESULT WINAPI IExtractIcon_GetIconLocation(LPEXTRACTICON this, UINT32 uFlags, LPSTR szIconFile, UINT32 cchMax, int * piIndex, UINT32 * pwFlags)
{	WARN (shell,"(%p) (flags=%u file=%s max=%u %p %p) semi-stub\n", this, uFlags, szIconFile, cchMax, piIndex, pwFlags);

	*piIndex = (int) SHMapPIDLToSystemImageListIndex(0, this->pidl,0);
	*pwFlags = GIL_NOTFILENAME;

	WARN (shell,"-- %x\n",*piIndex);

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

