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

DEFAULT_DEBUG_CHANNEL(shell)


/***********************************************************************
*   IExtractIconA implementation
*/

typedef struct 
{	ICOM_VTABLE(IExtractIconA)*	lpvtbl;
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
	ei->lpvtbl = &eivt;
	ei->lpvtblPersistFile = &pfvt;
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
	DWORD	ret = S_FALSE, dwNr;
	LPITEMIDLIST	pSimplePidl = ILFindLastID(This->pidl);
			
	TRACE (shell,"(%p) (flags=%u %p %u %p %p)\n", This, uFlags, szIconFile, cchMax, piIndex, pwFlags);

	if (pwFlags)
	  *pwFlags = 0;

	if (_ILIsDesktop(pSimplePidl))
	{ strncpy(szIconFile, "shell32.dll", cchMax);
	  *piIndex = 34;
	  ret = NOERROR;
	}
	else if (_ILIsMyComputer(pSimplePidl))
	{ if (HCR_GetDefaultIcon("CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}", sTemp, MAX_PATH, &dwNr))
	  { strncpy(szIconFile, sTemp, cchMax);
	    *piIndex = dwNr;
	  }
	  else
	  { strncpy(szIconFile, "shell32.dll", cchMax);
	    *piIndex = 15;
	  }
	  ret = NOERROR;
	}
	else if (_ILIsDrive (pSimplePidl))
	{ if (HCR_GetDefaultIcon("Drive", sTemp, MAX_PATH, &dwNr))
	  { strncpy(szIconFile, sTemp, cchMax);
	    *piIndex = dwNr;
	  }
	  else
	  { strncpy(szIconFile, "shell32.dll", cchMax);
	    *piIndex = 8;
	  }
	  ret = NOERROR;
	}
	else if (_ILIsFolder (pSimplePidl))
	{ if (HCR_GetDefaultIcon("Folder", sTemp, MAX_PATH, &dwNr))
	  { strncpy(szIconFile, sTemp, cchMax);
	    *piIndex = dwNr;
	  }
	  else
	  { strncpy(szIconFile, "shell32.dll", cchMax);
	    *piIndex = 3;
	  }
	  ret = NOERROR;
	}
	else
	{ if (_ILGetExtension (pSimplePidl, sTemp, MAX_PATH))		/* object is file */
	  { if ( HCR_MapTypeToValue(sTemp, sTemp, MAX_PATH))
	    { if (HCR_GetDefaultIcon(sTemp, sTemp, MAX_PATH, &dwNr))
	      { if (!strcmp("%1",sTemp))					/* icon is in the file */
	        { _ILGetPidlPath(This->pidl, sTemp, MAX_PATH);
	          dwNr = 0;
	        }
	        strncpy(szIconFile, sTemp, cchMax);
	        *piIndex = dwNr;
	        ret = NOERROR;
	      }
	    }
	  }
	}

	TRACE (shell,"-- %s %x\n", (ret==NOERROR)?debugstr_a(szIconFile):"[error]", *piIndex);
	return ret;
}
/**************************************************************************
*  IExtractIconA_Extract
*/
static HRESULT WINAPI IExtractIconA_fnExtract(IExtractIconA * iface, LPCSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
	ICOM_THIS(IExtractIconAImpl,iface);

	FIXME (shell,"(%p) (file=%p index=%u %p %p size=%u) semi-stub\n", This, pszFile, nIconIndex, phiconLarge, phiconSmall, nIconSize);

	if (phiconLarge)
	  *phiconLarge = pImageList_GetIcon(ShellBigIconList, nIconIndex, ILD_TRANSPARENT);

	if (phiconSmall)
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
	const IPersistFile	*iface,
	LPCLSID			lpClassId)
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
	FIXME(shell,"%p\n", This);
	return E_NOTIMPL;

}

static struct ICOM_VTABLE(IPersistFile) pfvt =
{
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

