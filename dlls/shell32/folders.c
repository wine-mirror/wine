/*
 *	Copyright 1997	Marcus Meissner
 *	Copyright 1998	Juergen Schmied
 *
 */

#include <stdlib.h>
#include <string.h>

#include "wine/obj_base.h"
#include "wine/obj_extracticon.h"

#include "debug.h"
#include "winerror.h"

#include "pidl.h"
#include "shell32_main.h"


/***********************************************************************
*   IExtractIconA implementation
*/

typedef struct 
{ ICOM_VTABLE(IExtractIconA)* lpvtbl;
  DWORD ref;
  LPITEMIDLIST pidl;
} IExtractIconAImpl;

static struct ICOM_VTABLE(IExtractIconA) eivt;


/**************************************************************************
*  IExtractIconA_Constructor
*/
IExtractIconA* IExtractIconA_Constructor(LPCITEMIDLIST pidl)
{
	IExtractIconAImpl* ei;

	ei=(IExtractIconAImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IExtractIconAImpl));
	ei->ref=1;
	ei->lpvtbl=&eivt;
	ei->pidl=ILClone(pidl);

	pdump(pidl);

	TRACE(shell,"(%p)\n",ei);
	shell32_ObjCount++;
	return (IExtractIconA *)ei;
}
/**************************************************************************
 *  IExtractIconA_QueryInterface
 */
static HRESULT WINAPI IExtractIconA_fnQueryInterface( IExtractIconA * iface, REFIID riid, LPVOID *ppvObj)
{
	ICOM_THIS(IExtractIconAImpl,iface);

	char	xriid[50];
	 WINE_StringFromCLSID((LPCLSID)riid,xriid);
	 TRACE(shell,"(%p)->(\n\tIID:\t%s,%p)\n",This,xriid,ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
	{ *ppvObj = This; 
	}
	else if(IsEqualIID(riid, &IID_IExtractIconA))  /*IExtractIcon*/
	{    *ppvObj = (IExtractIconA*)This;
	}

	if(*ppvObj)
	{ IExtractIconA_AddRef((IExtractIconA*) *ppvObj);  	
	  TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE(shell,"-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

/**************************************************************************
*  IExtractIconA_AddRef
*/
static ULONG WINAPI IExtractIconA_fnAddRef(IExtractIconA * iface)
{
	ICOM_THIS(IExtractIconAImpl,iface);

	TRACE(shell,"(%p)->(count=%lu)\n",This, This->ref );

	shell32_ObjCount++;

	return ++(This->ref);
}
/**************************************************************************
*  IExtractIconA_Release
*/
static ULONG WINAPI IExtractIconA_fnRelease(IExtractIconA * iface)
{
	ICOM_THIS(IExtractIconAImpl,iface);

	TRACE(shell,"(%p)->()\n",This);

	shell32_ObjCount--;

	if (!--(This->ref)) 
	{ TRACE(shell," destroying IExtractIcon(%p)\n",This);
	  SHFree(This->pidl);
	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}
/**************************************************************************
*  IExtractIconA_GetIconLocation
*/
static HRESULT WINAPI IExtractIconA_fnGetIconLocation(IExtractIconA * iface, UINT uFlags, LPSTR szIconFile, UINT cchMax, int * piIndex, UINT * pwFlags)
{
	ICOM_THIS(IExtractIconAImpl,iface);

	WARN (shell,"(%p) (flags=%u file=%s max=%u %p %p) semi-stub\n", This, uFlags, szIconFile, cchMax, piIndex, pwFlags);

	*piIndex = (int) SHMapPIDLToSystemImageListIndex(0, This->pidl,0);
	*pwFlags = GIL_NOTFILENAME;

	WARN (shell,"-- %x\n",*piIndex);

	return NOERROR;
}
/**************************************************************************
*  IExtractIconA_Extract
*/
static HRESULT WINAPI IExtractIconA_fnExtract(IExtractIconA * iface, LPCSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
	ICOM_THIS(IExtractIconAImpl,iface);

	FIXME (shell,"(%p) (file=%s index=%u %p %p size=%u) semi-stub\n", This, pszFile, nIconIndex, phiconLarge, phiconSmall, nIconSize);

	*phiconLarge = pImageList_GetIcon(ShellBigIconList, nIconIndex, ILD_TRANSPARENT);
	*phiconSmall = pImageList_GetIcon(ShellSmallIconList, nIconIndex, ILD_TRANSPARENT);
	return S_OK;
}

static struct ICOM_VTABLE(IExtractIconA) eivt = 
{	IExtractIconA_fnQueryInterface,
	IExtractIconA_fnAddRef,
	IExtractIconA_fnRelease,
	IExtractIconA_fnGetIconLocation,
	IExtractIconA_fnExtract
};
