/*
 *	Copyright 1997	Marcus Meissner
 *	Copyright 1998	Juergen Schmied
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "wine/obj_base.h"
#include "wine/obj_extracticon.h"
#include "wine/undocshell.h"
#include "shlguid.h"

#include "debugtools.h"
#include "winerror.h"

#include "pidl.h"
#include "shell32_main.h"

DEFAULT_DEBUG_CHANNEL(shell)


/***********************************************************************
*   IExtractIconA implementation
*/

typedef struct 
{	ICOM_VFIELD(IExtractIconA);
	DWORD	ref;
	ICOM_VTABLE(IPersistFile)*	lpvtblPersistFile;
	LPITEMIDLIST	pidl;
} IExtractIconAImpl;

static struct ICOM_VTABLE(IExtractIconA) eivt;
static struct ICOM_VTABLE(IPersistFile) pfvt;

#define _IPersistFile_Offset ((int)(&(((IExtractIconAImpl*)0)->lpvtblPersistFile)))
#define _ICOM_THIS_From_IPersistFile(class, name) class* This = (class*)(((char*)name)-_IPersistFile_Offset);

/**************************************************************************
*  IExtractIconA_Constructor
*/
IExtractIconA* IExtractIconA_Constructor(LPCITEMIDLIST pidl)
{
	IExtractIconAImpl* ei;

	ei=(IExtractIconAImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IExtractIconAImpl));
	ei->ref=1;
	ICOM_VTBL(ei) = &eivt;
	ei->lpvtblPersistFile = &pfvt;
	ei->pidl=ILClone(pidl);

	pdump(pidl);

	TRACE("(%p)\n",ei);
	shell32_ObjCount++;
	return (IExtractIconA *)ei;
}
/**************************************************************************
 *  IExtractIconA_QueryInterface
 */
static HRESULT WINAPI IExtractIconA_fnQueryInterface( IExtractIconA * iface, REFIID riid, LPVOID *ppvObj)
{
	ICOM_THIS(IExtractIconAImpl,iface);

	 TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))		/*IUnknown*/
	{ *ppvObj = This; 
	}
	else if(IsEqualIID(riid, &IID_IPersistFile))	/*IExtractIcon*/
	{    *ppvObj = (IPersistFile*)&(This->lpvtblPersistFile);
	}
	else if(IsEqualIID(riid, &IID_IExtractIconA))	/*IExtractIcon*/
	{    *ppvObj = (IExtractIconA*)This;
	}

	if(*ppvObj)
	{ IExtractIconA_AddRef((IExtractIconA*) *ppvObj);  	
	  TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

/**************************************************************************
*  IExtractIconA_AddRef
*/
static ULONG WINAPI IExtractIconA_fnAddRef(IExtractIconA * iface)
{
	ICOM_THIS(IExtractIconAImpl,iface);

	TRACE("(%p)->(count=%lu)\n",This, This->ref );

	shell32_ObjCount++;

	return ++(This->ref);
}
/**************************************************************************
*  IExtractIconA_Release
*/
static ULONG WINAPI IExtractIconA_fnRelease(IExtractIconA * iface)
{
	ICOM_THIS(IExtractIconAImpl,iface);

	TRACE("(%p)->()\n",This);

	shell32_ObjCount--;

	if (!--(This->ref)) 
	{ TRACE(" destroying IExtractIcon(%p)\n",This);
	  SHFree(This->pidl);
	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}
/**************************************************************************
*  IExtractIconA_GetIconLocation
*
* mapping filetype to icon
*/
static HRESULT WINAPI IExtractIconA_fnGetIconLocation(
	IExtractIconA * iface,
	UINT uFlags,
	LPSTR szIconFile,
	UINT cchMax,
	int * piIndex,
	UINT * pwFlags)
{
	ICOM_THIS(IExtractIconAImpl,iface);

	char	sTemp[MAX_PATH];
	DWORD	dwNr;
	GUID const * riid;
	LPITEMIDLIST	pSimplePidl = ILFindLastID(This->pidl);
			
	TRACE("(%p) (flags=%u %p %u %p %p)\n", This, uFlags, szIconFile, cchMax, piIndex, pwFlags);

	if (pwFlags)
	  *pwFlags = 0;

	if (_ILIsDesktop(pSimplePidl))
	{
	  lstrcpynA(szIconFile, "shell32.dll", cchMax);
	  *piIndex = 34;
	}

	/* my computer and other shell extensions */
	else if ( (riid = _ILGetGUIDPointer(pSimplePidl)) )
	{ 
	  char xriid[50];
          sprintf( xriid, "CLSID\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                   riid->Data1, riid->Data2, riid->Data3,
                   riid->Data4[0], riid->Data4[1], riid->Data4[2], riid->Data4[3],
                   riid->Data4[4], riid->Data4[5], riid->Data4[6], riid->Data4[7] );

	  if (HCR_GetDefaultIcon(xriid, sTemp, MAX_PATH, &dwNr))
	  {
	    lstrcpynA(szIconFile, sTemp, cchMax);
	    *piIndex = dwNr;
	  }
	  else
	  {
	    lstrcpynA(szIconFile, "shell32.dll", cchMax);
	    *piIndex = 15;
	  }
	}

	else if (_ILIsDrive (pSimplePidl))
	{
	  if (HCR_GetDefaultIcon("Drive", sTemp, MAX_PATH, &dwNr))
	  {
	    lstrcpynA(szIconFile, sTemp, cchMax);
	    *piIndex = dwNr;
	  }
	  else
	  {
	    lstrcpynA(szIconFile, "shell32.dll", cchMax);
	    *piIndex = 8;
	  }
	}
	else if (_ILIsFolder (pSimplePidl))
	{
	  if (HCR_GetDefaultIcon("Folder", sTemp, MAX_PATH, &dwNr))
	  {
	    lstrcpynA(szIconFile, sTemp, cchMax);
	    *piIndex = dwNr;
	  }
	  else
	  {
	    lstrcpynA(szIconFile, "shell32.dll", cchMax);
	    *piIndex = (uFlags & GIL_OPENICON)? 4 : 3;
	  }
	}
	else	/* object is file */
	{
	  if (_ILGetExtension (pSimplePidl, sTemp, MAX_PATH)
	      && HCR_MapTypeToValue(sTemp, sTemp, MAX_PATH, TRUE)
	      && HCR_GetDefaultIcon(sTemp, sTemp, MAX_PATH, &dwNr))
	  {
	    if (!strcmp("%1",sTemp))		/* icon is in the file */
	    {
	      SHGetPathFromIDListA(This->pidl, sTemp);
	      dwNr = 0;
	    }
	    lstrcpynA(szIconFile, sTemp, cchMax);
	    *piIndex = dwNr;
	  }
	  else					/* default icon */
	  {
	    lstrcpynA(szIconFile, "shell32.dll", cchMax);
	    *piIndex = 0;
	  }
	}

	TRACE("-- %s %x\n", szIconFile, *piIndex);
	return NOERROR;
}
/**************************************************************************
*  IExtractIconA_Extract
*/
static HRESULT WINAPI IExtractIconA_fnExtract(IExtractIconA * iface, LPCSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
	ICOM_THIS(IExtractIconAImpl,iface);

	FIXME("(%p) (file=%p index=%u %p %p size=%u) semi-stub\n", This, pszFile, nIconIndex, phiconLarge, phiconSmall, nIconSize);

	if (phiconLarge)
	  *phiconLarge = pImageList_GetIcon(ShellBigIconList, nIconIndex, ILD_TRANSPARENT);

	if (phiconSmall)
	  *phiconSmall = pImageList_GetIcon(ShellSmallIconList, nIconIndex, ILD_TRANSPARENT);

	return S_OK;
}

static struct ICOM_VTABLE(IExtractIconA) eivt = 
{	
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IExtractIconA_fnQueryInterface,
	IExtractIconA_fnAddRef,
	IExtractIconA_fnRelease,
	IExtractIconA_fnGetIconLocation,
	IExtractIconA_fnExtract
};

/************************************************************************
 * IEIPersistFile_QueryInterface (IUnknown)
 */
static HRESULT WINAPI IEIPersistFile_fnQueryInterface(
	IPersistFile	*iface,
	REFIID		iid,
	LPVOID		*ppvObj)
{
	_ICOM_THIS_From_IPersistFile(IExtractIconA, iface);

	return IShellFolder_QueryInterface((IExtractIconA*)This, iid, ppvObj);
}

/************************************************************************
 * IEIPersistFile_AddRef (IUnknown)
 */
static ULONG WINAPI IEIPersistFile_fnAddRef(
	IPersistFile	*iface)
{
	_ICOM_THIS_From_IPersistFile(IExtractIconA, iface);

	return IExtractIconA_AddRef((IExtractIconA*)This);
}

/************************************************************************
 * IEIPersistFile_Release (IUnknown)
 */
static ULONG WINAPI IEIPersistFile_fnRelease(
	IPersistFile	*iface)
{
	_ICOM_THIS_From_IPersistFile(IExtractIconA, iface);

	return IExtractIconA_Release((IExtractIconA*)This);
}

/************************************************************************
 * IEIPersistFile_GetClassID (IPersist)
 */
static HRESULT WINAPI IEIPersistFile_fnGetClassID(
	IPersistFile	*iface,
	LPCLSID		lpClassId)
{
	CLSID StdFolderID = { 0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} };

	if (lpClassId==NULL)
	  return E_POINTER;

	memcpy(lpClassId, &StdFolderID, sizeof(StdFolderID));

	return S_OK;
}

/************************************************************************
 * IEIPersistFile_Load (IPersistFile)
 */
static HRESULT WINAPI IEIPersistFile_fnLoad(IPersistFile* iface, LPCOLESTR pszFileName, DWORD dwMode)
{
	_ICOM_THIS_From_IPersistFile(IExtractIconA, iface);
	FIXME("%p\n", This);
	return E_NOTIMPL;

}

static struct ICOM_VTABLE(IPersistFile) pfvt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IEIPersistFile_fnQueryInterface,
	IEIPersistFile_fnAddRef,
	IEIPersistFile_fnRelease,
	IEIPersistFile_fnGetClassID,
	(void *) 0xdeadbeef /* IEIPersistFile_fnIsDirty */,
	IEIPersistFile_fnLoad,
	(void *) 0xdeadbeef /* IEIPersistFile_fnSave */,
	(void *) 0xdeadbeef /* IEIPersistFile_fnSaveCompleted */,
	(void *) 0xdeadbeef /* IEIPersistFile_fnGetCurFile */
};

