/*
 * Temporary place for ole2 stubs.
 *
 * Copyright (C) 1999 Corel Corporation
 * Move these functions to dlls/ole32/ole2impl.c when you implement them.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "objidl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/******************************************************************************
 *               OleCreateLinkToFile        [OLE32.96]
 */
HRESULT WINAPI  OleCreateLinkToFile(LPCOLESTR lpszFileName, REFIID riid,
	  		DWORD renderopt, LPFORMATETC lpFormatEtc,
			LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID* ppvObj)
{
    FIXME("(%p,%p,%li,%p,%p,%p,%p), stub!\n",lpszFileName, riid, renderopt, lpFormatEtc, pClientSite, pStg, ppvObj);
    return E_NOTIMPL;
}


/******************************************************************************
 *              OleDuplicateData        [OLE32.102]
 */
HRESULT WINAPI OleDuplicateData(HANDLE hSrc, CLIPFORMAT cfFormat,
	                          UINT uiFlags)
{
    FIXME("(%p,%x,%x), stub!\n", hSrc, cfFormat, uiFlags);
    return E_NOTIMPL;
}


/***********************************************************************
 *               WriteFmtUserTypeStg (OLE32.160)
 */
HRESULT WINAPI WriteFmtUserTypeStg(
	  LPSTORAGE pstg, CLIPFORMAT cf, LPOLESTR lpszUserType)
{
    FIXME("(%p,%x,%s) stub!\n",pstg,cf,debugstr_w(lpszUserType));
    return E_NOTIMPL;
}

/***********************************************************************
 *             OleTranslateAccelerator [OLE32.130]
 */
HRESULT WINAPI OleTranslateAccelerator (LPOLEINPLACEFRAME lpFrame,
                   LPOLEINPLACEFRAMEINFO lpFrameInfo, LPMSG lpmsg)
{
    FIXME("(%p,%p,%p),stub!\n", lpFrame, lpFrameInfo, lpmsg);
    return S_OK;
}

/******************************************************************************
 *              SetConvertStg        [OLE32.142]
 */
HRESULT WINAPI SetConvertStg(LPSTORAGE pStg, BOOL fConvert)
{
  FIXME("(%p,%x), stub!\n", pStg, fConvert);
  return E_NOTIMPL;
}

/******************************************************************************
 *              OleCreate        [OLE32.89]
 *
 */
HRESULT WINAPI OleCreate(
	REFCLSID rclsid,
	REFIID riid,
	DWORD renderopt,
	LPFORMATETC pFormatEtc,
	LPOLECLIENTSITE pClientSite,
	LPSTORAGE pStg,
	LPVOID* ppvObj)
{
  HRESULT hres, hres1;
  IUnknown * pUnk = NULL;

  FIXME("\n\t%s\n\t%s stub!\n", debugstr_guid(rclsid), debugstr_guid(riid));

  if (SUCCEEDED((hres = CoCreateInstance(rclsid, 0, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER|CLSCTX_LOCAL_SERVER , riid, (LPVOID*)&pUnk))))
  {
    if (pClientSite)
    {
      IOleObject * pOE;
      IPersistStorage * pPS;
      if (SUCCEEDED((hres = IUnknown_QueryInterface( pUnk, &IID_IOleObject, (LPVOID*)&pOE))))
      {
        TRACE("trying to set clientsite %p\n", pClientSite);
        hres1 = IOleObject_SetClientSite(pOE, pClientSite);
        TRACE("-- result 0x%08lx\n", hres1);
	IOleObject_Release(pOE);
      }
      if (SUCCEEDED((hres = IUnknown_QueryInterface( pUnk, &IID_IPersistStorage, (LPVOID*)&pPS))))
      {
        TRACE("trying to set stg %p\n", pStg);
	hres1 = IPersistStorage_InitNew(pPS, pStg);
        TRACE("-- result 0x%08lx\n", hres1);
	IPersistStorage_Release(pPS);
      }
    }
  }

  *ppvObj = pUnk;

  TRACE("-- %p \n", pUnk);
  return hres;
}

/******************************************************************************
 *              OleCreateLink        [OLE32.94]
 */
HRESULT WINAPI OleCreateLink(LPMONIKER pmkLinkSrc, REFIID riid, DWORD renderopt, LPFORMATETC lpFormatEtc,
                LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID* ppvObj)
{
  FIXME("(not shown), stub!\n");
  return E_NOTIMPL;
}

/******************************************************************************
 *              OleCreateFromFile        [OLE32.93]
 */
HRESULT WINAPI OleCreateFromFile(REFCLSID rclsid, LPCOLESTR lpszFileName, REFIID riid,
            DWORD renderopt, LPFORMATETC lpFormatEtc, LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID* ppvObj)
{
  FIXME("(not shown), stub!\n");
  return E_NOTIMPL;
}


/******************************************************************************
 *              OleGetIconOfClass        [OLE32.106]
 */
HGLOBAL WINAPI OleGetIconOfClass(REFCLSID rclsid, LPOLESTR lpszLabel, BOOL fUseTypeAsLabel)
{
  FIXME("(%p,%p,%x), stub!\n", rclsid, lpszLabel, fUseTypeAsLabel);
  return NULL;
}

/******************************************************************************
 *              ReadFmtUserTypeStg        [OLE32.136]
 */
HRESULT WINAPI ReadFmtUserTypeStg (LPSTORAGE pstg, CLIPFORMAT* pcf, LPOLESTR* lplpszUserType)
{
  FIXME("(%p,%p,%p), stub!\n", pstg, pcf, lplpszUserType);
  return E_NOTIMPL;
}

/******************************************************************************
 *              OleCreateStaticFromData        [OLE32.98]
 */
HRESULT     WINAPI OleCreateStaticFromData(LPDATAOBJECT pSrcDataObj, REFIID iid,
                DWORD renderopt, LPFORMATETC pFormatEtc, LPOLECLIENTSITE pClientSite,
                LPSTORAGE pStg, LPVOID* ppvObj)
{
  FIXME("(not shown), stub!\n");
  return E_NOTIMPL;
}

/******************************************************************************
 *              OleCreateLinkFromData        [OLE32.95]
 */

HRESULT WINAPI  OleCreateLinkFromData(LPDATAOBJECT pSrcDataObj, REFIID riid,
                DWORD renderopt, LPFORMATETC pFormatEtc,
                LPOLECLIENTSITE pClientSite, LPSTORAGE pStg,
                LPVOID* ppvObj)
{
  FIXME("(not shown), stub!\n");
  return E_NOTIMPL;
}

/******************************************************************************
 *              OleIsRunning        [OLE32.111]
 */
BOOL WINAPI OleIsRunning(LPOLEOBJECT pObject)
{
  FIXME("(%p), stub!\n", pObject);
  return TRUE;
}

/***********************************************************************
 *           OleRegEnumVerbs    [OLE32.120]
 */
HRESULT WINAPI OleRegEnumVerbs (REFCLSID clsid, LPENUMOLEVERB* ppenum)
{
    FIXME("(%p,%p), stub!\n", clsid, ppenum);
    return OLEOBJ_E_NOVERBS;
}

/***********************************************************************
 *           OleRegEnumFormatEtc    [OLE32.119]
 */
HRESULT     WINAPI OleRegEnumFormatEtc (
  REFCLSID clsid,
  DWORD    dwDirection,
  LPENUMFORMATETC* ppenumFormatetc)
{
    FIXME("(%p, %ld, %p), stub!\n", clsid, dwDirection, ppenumFormatetc);

    return E_NOTIMPL;
}

/***********************************************************************
 *           PropVariantClear			    [OLE32.166]
 */
HRESULT WINAPI PropVariantClear(PROPVARIANT * pvar) /* [in/out] FIXME: PROPVARIANT * */
{
	TRACE("(%p)\n", pvar);

	if (!pvar)
	    return S_OK;

	switch(pvar->vt)
	{
	    case VT_BSTR:
	        CoTaskMemFree(pvar->u.bstrVal);
		break;
	    case VT_STREAM:
	    case VT_STREAMED_OBJECT:
	    case VT_STORAGE:
	    case VT_STORED_OBJECT:
	        IUnknown_Release((LPUNKNOWN)pvar->u.pStream);
		break;
	    case VT_CLSID:
	    case VT_CF:
	    case VT_LPSTR:
	    case VT_LPWSTR:
	    case VT_BLOB:
	    case VT_BLOB_OBJECT:
	        FIXME("Don't know what to do for variant type %d\n", pvar->vt);
	    default:
	        if (pvar->vt && VT_VECTOR)
		{
		    FIXME("Need to recursively destroy elements in vector\n");
/*		    SafeArrayDestroy(pvar->u.caub); */
		}
	}

	ZeroMemory(pvar, sizeof(PROPVARIANT));

	return S_OK;
}

/***********************************************************************
 *           PropVariantCopy			    [OLE32.246]
 */
HRESULT WINAPI PropVariantCopy(void *pvarDest,      /* [out] FIXME: PROPVARIANT * */
			       const void *pvarSrc) /* [in] FIXME: const PROPVARIANT * */
{
	FIXME("(%p, %p): stub:\n", pvarDest, pvarSrc);

	return E_NOTIMPL;
}

/***********************************************************************
 *           FreePropVariantArray			    [OLE32.195]
 */
HRESULT WINAPI FreePropVariantArray(ULONG cVariants, /* [in] */
				    void *rgvars)    /* [in/out] FIXME: PROPVARIANT * */
{
	FIXME("(%lu, %p): stub:\n", cVariants, rgvars);

	return E_NOTIMPL;
}

/***********************************************************************
 *           CoIsOle1Class                              [OLE32.29]
 */
BOOL WINAPI CoIsOle1Class(REFCLSID clsid)
{
  FIXME("%s\n", debugstr_guid(clsid));
  return FALSE;
}

/***********************************************************************
 *           DllGetClassObject                          [OLE2.4]
 */
HRESULT WINAPI DllGetClassObject16(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
  FIXME("(%s, %s, %p): stub\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);
  return E_NOTIMPL;
}

/***********************************************************************
 *           OleSetClipboard                            [OLE2.49]
 */
HRESULT WINAPI OleSetClipboard16(IDataObject* pDataObj)
{
  FIXME("(%p): stub\n", pDataObj);
  return S_OK;
}

/***********************************************************************
 *           OleGetClipboard                            [OLE2.50]
 */
HRESULT WINAPI OleGetClipboard16(IDataObject** ppDataObj)
{
  FIXME("(%p): stub\n", ppDataObj);
  return E_NOTIMPL;
}
