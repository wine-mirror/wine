/*
 * Temporary place for ole2 stubs.
 *
 * Copyright (C) 1999 Corel Corporation
 * Move these functions to dlls/ole32/ole2impl.c when you implement them.
 */

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "ole2.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ole);

/******************************************************************************
 *               OleCreateLinkToFile        [OLE32.96]
 */
HRESULT WINAPI  OleCreateLinkToFile(LPCOLESTR lpszFileName, REFIID riid,
	  		DWORD renderopt, LPFORMATETC lpFormatEtc,
			LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID* ppvObj)
{
    FIXME("(%p,%p,%li,%p,%p,%p,%p), stub!\n",lpszFileName, riid, renderopt, lpFormatEtc, pClientSite, pStg, ppvObj);
    return S_OK;
}


/******************************************************************************
 *              OleDuplicateData        [OLE32.102]
 */
HRESULT WINAPI OleDuplicateData(HANDLE hSrc, CLIPFORMAT cfFormat,
	                          UINT uiFlags)
{
    FIXME("(%x,%x,%x), stub!\n", hSrc, cfFormat, uiFlags);
    return S_OK;
}

 
/***********************************************************************
 *               WriteFmtUserTypeStg (OLE32.160)
 */
HRESULT WINAPI WriteFmtUserTypeStg(
	  LPSTORAGE pstg, CLIPFORMAT cf, LPOLESTR lpszUserType)
{
    FIXME("(%p,%x,%s) stub!\n",pstg,cf,debugstr_w(lpszUserType));
    return S_OK;
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
 *              CoTreatAsClass        [OLE32.46]
 */
HRESULT WINAPI CoTreatAsClass(REFCLSID clsidOld, REFCLSID clsidNew)
{
  FIXME("(%p,%p), stub!\n", clsidOld, clsidNew);
  return S_OK;
}

/******************************************************************************
 *              SetConvertStg        [OLE32.142]
 */
HRESULT WINAPI SetConvertStg(LPSTORAGE pStg, BOOL fConvert)
{
  FIXME("(%p,%x), stub!\n", pStg, fConvert);
  return S_OK;
}

/******************************************************************************
 *              OleCreate        [OLE32.80]
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

  if (SUCCEEDED((hres = CoCreateInstance(rclsid, 0, CLSCTX_INPROC_SERVER, riid, (LPVOID*)&pUnk))))
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
  return S_OK;
}

/******************************************************************************
 *              OleCreateFromFile        [OLE32.93]
 */
HRESULT WINAPI OleCreateFromFile(REFCLSID rclsid, LPCOLESTR lpszFileName, REFIID riid,
            DWORD renderopt, LPFORMATETC lpFormatEtc, LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID* ppvObj)
{
  FIXME("(not shown), stub!\n");
  return S_OK;
}


/******************************************************************************
 *              OleGetIconOfClass        [OLE32.106]
 */
HGLOBAL WINAPI OleGetIconOfClass(REFCLSID rclsid, LPOLESTR lpszLabel, BOOL fUseTypeAsLabel)
{
  FIXME("(%p,%p,%x), stub!\n", rclsid, lpszLabel, fUseTypeAsLabel);
  return S_OK;
}

/******************************************************************************
 *              ReadFmtUserTypeStg        [OLE32.136]
 */
HRESULT WINAPI ReadFmtUserTypeStg (LPSTORAGE pstg, CLIPFORMAT* pcf, LPOLESTR* lplpszUserType)
{
  FIXME("(%p,%p,%p), stub!\n", pstg, pcf, lplpszUserType);
  return S_OK;
}

/******************************************************************************
 *              OleCreateStaticFromData        [OLE32.98]
 */
HRESULT     WINAPI OleCreateStaticFromData(LPDATAOBJECT pSrcDataObj, REFIID iid,
                DWORD renderopt, LPFORMATETC pFormatEtc, LPOLECLIENTSITE pClientSite,
                LPSTORAGE pStg, LPVOID* ppvObj)
{
  FIXME("(not shown), stub!\n");
  return S_OK;
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
  return S_OK;
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

    return S_OK;
}

/***********************************************************************
 *           PropVariantClear			    [OLE32.166]
 */
HRESULT WINAPI PropVariantClear(void *pvar) /* [in/out] FIXME: PROPVARIANT * */
{
	FIXME("(%p): stub:\n", pvar);

	*(LPWORD)pvar = 0;
	/* sets at least the vt field to VT_EMPTY */
	return S_OK;
}

/***********************************************************************
 *           PropVariantCopy			    [OLE32.246]
 */
HRESULT WINAPI PropVariantCopy(void *pvarDest,      /* [out] FIXME: PROPVARIANT * */
			       const void *pvarSrc) /* [in] FIXME: const PROPVARIANT * */
{
	FIXME("(%p, %p): stub:\n", pvarDest, pvarSrc);

	return S_OK;
}

/***********************************************************************
 *           FreePropVariantArray			    [OLE32.195]
 */
HRESULT WINAPI FreePropVariantArray(ULONG cVariants, /* [in] */
				    void *rgvars)    /* [in/out] FIXME: PROPVARIANT * */
{
	FIXME("(%lu, %p): stub:\n", cVariants, rgvars);

	return S_OK;
}

/***********************************************************************
 *           CoIsOle1Class                              [OLE32]
 */
BOOL WINAPI CoIsOle1Class(REFCLSID clsid)
{
  FIXME("%s\n", debugstr_guid(clsid));
  return FALSE;
}
