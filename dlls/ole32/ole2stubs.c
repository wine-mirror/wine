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
 *           OLE_FreeClipDataArray   [internal]
 *
 * NOTES:
 *  frees the data associated with an array of CLIPDATAs
 */
static void OLE_FreeClipDataArray(ULONG count, CLIPDATA * pClipDataArray)
{
    ULONG i;
    for (i = 0; i < count; i++)
    {
        if (pClipDataArray[i].pClipData)
        {
            CoTaskMemFree(pClipDataArray[i].pClipData);
        }
    }
}

HRESULT WINAPI FreePropVariantArray(ULONG,PROPVARIANT*);

/***********************************************************************
 *           PropVariantClear			    [OLE32.166]
 */
HRESULT WINAPI PropVariantClear(PROPVARIANT * pvar) /* [in/out] */
{
	TRACE("(%p)\n", pvar);

	if (!pvar)
	    return S_OK;

	switch(pvar->vt)
	{
	    case VT_STREAM:
	    case VT_STREAMED_OBJECT:
	    case VT_STORAGE:
	    case VT_STORED_OBJECT:
	        IUnknown_Release((LPUNKNOWN)pvar->u.pStream);
		break;
	    case VT_CLSID:
	    case VT_LPSTR:
	    case VT_LPWSTR:
	        CoTaskMemFree(pvar->u.puuid); /* pick an arbitary typed pointer - we don't care about the type as we are just freeing it */
		break;
	    case VT_BLOB:
	    case VT_BLOB_OBJECT:
		CoTaskMemFree(pvar->u.blob.pBlobData);
		break;
	    case VT_BSTR:
		FIXME("Need to load OLEAUT32 for SysFreeString\n");
		/* SysFreeString(pvar->u.bstrVal); */
		break;
	    case VT_CF:
		if (pvar->u.pclipdata)
		{
	        	OLE_FreeClipDataArray(1, pvar->u.pclipdata);
			CoTaskMemFree(pvar->u.pclipdata);
		}
		break;
	    default:
		if (pvar->vt & VT_ARRAY)
		{
		    FIXME("Need to call SafeArrayDestroy\n");
/*		    SafeArrayDestroy(pvar->u.caub); */
		}
		switch (pvar->vt & VT_VECTOR)
		{
		case VT_VARIANT:
			FreePropVariantArray(pvar->u.capropvar.cElems, pvar->u.capropvar.pElems);
			break;
		case VT_CF:
			OLE_FreeClipDataArray(pvar->u.caclipdata.cElems, pvar->u.caclipdata.pElems);
			break;
		case VT_BSTR:
		case VT_LPSTR:
		case VT_LPWSTR:
			FIXME("Freeing of vector sub-type not supported yet\n");
		}
	        if (pvar->vt & VT_VECTOR)
		{
			CoTaskMemFree(pvar->u.capropvar.pElems); /* pick an arbitary VT_VECTOR structure - they all have the same memory layout */
		}
	}

	ZeroMemory(pvar, sizeof(*pvar));

	return S_OK;
}

/***********************************************************************
 *           PropVariantCopy			    [OLE32.246]
 */
HRESULT WINAPI PropVariantCopy(PROPVARIANT *pvarDest,      /* [out] FIXME: PROPVARIANT * */
			       const PROPVARIANT *pvarSrc) /* [in] FIXME: const PROPVARIANT * */
{
	ULONG len;
	TRACE("(%p, %p): stub:\n", pvarDest, pvarSrc);

	/* this will deal with most cases */
	CopyMemory(pvarDest, pvarSrc, sizeof(*pvarDest));

	switch(pvarSrc->vt)
	{
	case VT_STREAM:
	case VT_STREAMED_OBJECT:
	case VT_STORAGE:
	case VT_STORED_OBJECT:
		IUnknown_AddRef((LPUNKNOWN)pvarDest->u.pStream);
		break;
	case VT_CLSID:
		pvarDest->u.puuid = CoTaskMemAlloc(sizeof(CLSID));
		CopyMemory(pvarDest->u.puuid, pvarSrc->u.puuid, sizeof(CLSID));
		break;
	case VT_LPSTR:
		len = strlen(pvarSrc->u.pszVal);
		pvarDest->u.pszVal = CoTaskMemAlloc(len);
		CopyMemory(pvarDest->u.pszVal, pvarSrc->u.pszVal, len);
		break;
	case VT_LPWSTR:
		len = lstrlenW(pvarSrc->u.pwszVal);
		pvarDest->u.pwszVal = CoTaskMemAlloc(len);
		CopyMemory(pvarDest->u.pwszVal, pvarSrc->u.pwszVal, len);
		break;
	case VT_BLOB:
	case VT_BLOB_OBJECT:
		if (pvarSrc->u.blob.pBlobData)
		{
			len = pvarSrc->u.blob.cbSize;
			pvarDest->u.blob.pBlobData = CoTaskMemAlloc(len);
			CopyMemory(pvarDest->u.blob.pBlobData, pvarSrc->u.blob.pBlobData, len);
		}
		break;
	case VT_BSTR:
		FIXME("Need to copy BSTR\n");
		break;
	case VT_CF:
		if (pvarSrc->u.pclipdata)
		{
			len = pvarSrc->u.pclipdata->cbSize - sizeof(pvarSrc->u.pclipdata->ulClipFmt);
			CoTaskMemAlloc(len);
	        	CopyMemory(pvarDest->u.pclipdata->pClipData, pvarSrc->u.pclipdata->pClipData, len);
		}
		break;
	default:
		if (pvarSrc->vt & VT_ARRAY)
		{
		    FIXME("Need to call SafeArrayCopy\n");
/*		    SafeArrayCopy(...); */
		}
	        if (pvarSrc->vt & VT_VECTOR)
		{
			int elemSize;
			switch(pvarSrc->vt & VT_VECTOR)
			{
			case VT_I1:
				elemSize = sizeof(pvarSrc->u.cVal);
				break;
			case VT_UI1:
				elemSize = sizeof(pvarSrc->u.bVal);
				break;
			case VT_I2:
				elemSize = sizeof(pvarSrc->u.iVal);
				break;
			case VT_UI2:
				elemSize = sizeof(pvarSrc->u.uiVal);
				break;
			case VT_BOOL:
				elemSize = sizeof(pvarSrc->u.boolVal);
				break;
			case VT_I4:
				elemSize = sizeof(pvarSrc->u.lVal);
				break;
			case VT_UI4:
				elemSize = sizeof(pvarSrc->u.ulVal);
				break;
			case VT_R4:
				elemSize = sizeof(pvarSrc->u.fltVal);
				break;
			case VT_R8:
				elemSize = sizeof(pvarSrc->u.dblVal);
				break;
			case VT_ERROR:
				elemSize = sizeof(pvarSrc->u.scode);
				break;
			case VT_I8:
				elemSize = sizeof(pvarSrc->u.hVal);
				break;
			case VT_UI8:
				elemSize = sizeof(pvarSrc->u.uhVal);
				break;
			case VT_CY:
				elemSize = sizeof(pvarSrc->u.cyVal);
				break;
			case VT_DATE:
				elemSize = sizeof(pvarSrc->u.date);
				break;
			case VT_FILETIME:
				elemSize = sizeof(pvarSrc->u.filetime);
				break;
			case VT_CLSID:
				elemSize = sizeof(*pvarSrc->u.puuid);
				break;
			case VT_CF:
				elemSize = sizeof(*pvarSrc->u.pclipdata);
				break;
			case VT_BSTR:
			case VT_LPSTR:
			case VT_LPWSTR:
			case VT_VARIANT:
			default:
				FIXME("Invalid element type: %ul\n", pvarSrc->vt & VT_VECTOR);
				return E_INVALIDARG;
			}
			len = pvarSrc->u.capropvar.cElems;
			pvarDest->u.capropvar.pElems = CoTaskMemAlloc(len * elemSize);
			if (pvarSrc->vt == (VT_VECTOR | VT_VARIANT))
			{
				ULONG i;
				for (i = 0; i < len; i++)
					PropVariantCopy(&pvarDest->u.capropvar.pElems[i], &pvarSrc->u.capropvar.pElems[i]);
			}
			else if (pvarSrc->vt == (VT_VECTOR | VT_CF))
			{
				FIXME("Copy clipformats\n");
			}
			else if (pvarSrc->vt == (VT_VECTOR | VT_BSTR))
			{
				FIXME("Copy BSTRs\n");
			}
			else if (pvarSrc->vt == (VT_VECTOR | VT_LPSTR))
			{
				FIXME("Copy LPSTRs\n");
			}
			else if (pvarSrc->vt == (VT_VECTOR | VT_LPSTR))
			{
				FIXME("Copy LPWSTRs\n");
			}
			else
				CopyMemory(pvarDest->u.capropvar.pElems, pvarSrc->u.capropvar.pElems, len * elemSize);
		}
	}

	return S_OK;
}

/***********************************************************************
 *           FreePropVariantArray			    [OLE32.195]
 */
HRESULT WINAPI FreePropVariantArray(ULONG cVariants, /* [in] */
				    PROPVARIANT *rgvars)    /* [in/out] */
{
	ULONG i;

	TRACE("(%lu, %p)\n", cVariants, rgvars);

	for(i = 0; i < cVariants; i++)
		PropVariantClear(&rgvars[i]);

	return S_OK;
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
