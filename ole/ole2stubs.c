/*
 * Temporary place for ole2 stubs.
 *
 * Copyright (C) 1999 Corel Corporation
 */

#include "ole2.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(ole)

/******************************************************************************
 *               OleCreateFromData        [OLE32.92]
 */
HRESULT WINAPI OleCreateFromData(LPDATAOBJECT pSrcDataObj, REFIID riid,
                DWORD renderopt, LPFORMATETC pFormatEtc,
                LPOLECLIENTSITE pClientSite, LPSTORAGE pStg,
                LPVOID* ppvObj)
{
    FIXME(ole,"(%p,%p,%li,%p,%p,%p,%p), stub!\n", pSrcDataObj,riid,renderopt,pFormatEtc,pClientSite,pStg,ppvObj);
    return S_OK;
}


/******************************************************************************
 *               OleCreateLinkToFile        [OLE32.96]
 */
HRESULT WINAPI  OleCreateLinkToFile(LPCOLESTR lpszFileName, REFIID riid,
	  		DWORD renderopt, LPFORMATETC lpFormatEtc,
			LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID* ppvObj)
{
    FIXME(ole,"(%p,%p,%li,%p,%p,%p,%p), stub!\n",lpszFileName, riid, renderopt, lpFormatEtc, pClientSite, pStg, ppvObj);
    return S_OK;
}


/******************************************************************************
 *              OleDuplicateData        [OLE32.102]
 */
HRESULT WINAPI OleDuplicateData(HANDLE hSrc, CLIPFORMAT cfFormat,
	                          UINT uiFlags)
{
    FIXME(ole,"(%x,%x,%x), stub!\n", hSrc, cfFormat, uiFlags);
    return S_OK;
}

 
/***********************************************************************
 *               WriteFmtUserTypeStg (OLE32.160)
 */
HRESULT WINAPI WriteFmtUserTypeStg(
	  LPSTORAGE pstg, CLIPFORMAT cf, LPOLESTR lpszUserType)
{
    FIXME(ole,"(%p,%x,%p) stub!\n",pstg,cf,lpszUserType);
    return S_OK;
}

/***********************************************************************
 *             OleTranslateAccelerator [OLE32.130]
 */
HRESULT WINAPI OleTranslateAccelerator (LPOLEINPLACEFRAME lpFrame,
                   LPOLEINPLACEFRAMEINFO lpFrameInfo, LPMSG lpmsg)
{
    FIXME(ole,"(%p,%p,%p),stub!\n", lpFrame, lpFrameInfo, lpmsg);
    return S_OK;
}

/******************************************************************************
 *              CoTreatAsClass        [OLE32.46]
 */
HRESULT WINAPI CoTreatAsClass(REFCLSID clsidOld, REFCLSID clsidNew)
{
  FIXME(ole,"(%p,%p), stub!\n", clsidOld, clsidNew);
  return S_OK;
}


/******************************************************************************
 *              IsAccelerator        [OLE32.75]
 */
BOOL WINAPI IsAccelerator(HACCEL hAccel, int cAccelEntries, LPMSG lpMsg, WORD* lpwCmd)
{
  FIXME(ole,"(%x,%i,%p,%p), stub!\n", hAccel, cAccelEntries, lpMsg, lpwCmd);
  return TRUE;
}

/******************************************************************************
 *              SetConvertStg        [OLE32.142]
 */
HRESULT WINAPI SetConvertStg(LPSTORAGE pStg, BOOL fConvert)
{
  FIXME(ole,"(%p,%x), stub!\n", pStg, fConvert);
  return S_OK;
}

/******************************************************************************
 *              OleFlushClipboard        [OLE32.103]
 */
HRESULT WINAPI OleFlushClipboard()
{
  FIXME(ole,"(), stub!\n");
  return S_OK;
}

/******************************************************************************
 *              OleCreate        [OLE32.80]
 */
HRESULT WINAPI OleCreate(REFCLSID rclsid, REFIID riid, DWORD renderopt, LPFORMATETC pFormatEtc, LPOLECLIENTSITE pClientSite,
                LPSTORAGE pStg, LPVOID* ppvObj)
{
  FIXME(ole,"(not shown), stub!\n");
  return S_OK;
}

/******************************************************************************
 *              OleCreateLink        [OLE32.94]
 */
HRESULT WINAPI OleCreateLink(LPMONIKER pmkLinkSrc, REFIID riid, DWORD renderopt, LPFORMATETC lpFormatEtc,
                LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID* ppvObj)
{
  FIXME(ole,"(not shown), stub!\n");
  return S_OK;
}

/******************************************************************************
 *              OleCreateFromFile        [OLE32.93]
 */
HRESULT WINAPI OleCreateFromFile(REFCLSID rclsid, LPCOLESTR lpszFileName, REFIID riid,
            DWORD renderopt, LPFORMATETC lpFormatEtc, LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID* ppvObj)
{
  FIXME(ole,"(not shown), stub!\n");
  return S_OK;
}

/******************************************************************************
 *              OleLockRunning        [OLE32.114]
 */
HRESULT WINAPI OleLockRunning(LPUNKNOWN pUnknown, BOOL fLock, BOOL fLastUnlockCloses) 
{
  FIXME(ole,"(%p,%x,%x), stub!\n", pUnknown, fLock, fLastUnlockCloses);
  return S_OK;
}

/******************************************************************************
 *              OleGetIconOfClass        [OLE32.106]
 */
HGLOBAL WINAPI OleGetIconOfClass(REFCLSID rclsid, LPOLESTR lpszLabel, BOOL fUseTypeAsLabel)
{
  FIXME(ole,"(%p,%p,%x), stub!\n", rclsid, lpszLabel, fUseTypeAsLabel);
  return S_OK;
}

/******************************************************************************
 *              OleQueryCreateFromData        [OLE32.117]
 */
HRESULT WINAPI OleQueryCreateFromData(LPDATAOBJECT pSrcDataObject)
{
  FIXME(ole,"(%p), stub!\n", pSrcDataObject);
  return S_OK;
}

/******************************************************************************
 *              CreateDataAdviseHolder        [OLE32.53]
 */
HRESULT WINAPI CreateDataAdviseHolder(LPDATAADVISEHOLDER* ppDAHolder)
{
  FIXME(ole,"(%p), stub!\n", ppDAHolder);
  return S_OK;
}

/******************************************************************************
 *              CreateILockBytesOnHGlobal        [OLE32.67]
 */
HRESULT WINAPI CreateILockBytesOnHGlobal(HGLOBAL hGlobal, BOOL fDeleteOnRelease, LPLOCKBYTES* pplkbyt)
{
  FIXME(ole,"(%x,%x,%p), stub!\n", hGlobal, fDeleteOnRelease, pplkbyt);
  return S_OK;
}

/******************************************************************************
 *              GetHGlobalFromILockBytes        [OLE32.70]
 */
HRESULT WINAPI GetHGlobalFromILockBytes (LPLOCKBYTES plkbyt, HGLOBAL* phglobal)
{
  FIXME(ole,"(%p,%p), stub!\n", plkbyt, phglobal);
  return S_OK;
}

/******************************************************************************
 *              OleLoad        [OLE32.112]
 */
HRESULT WINAPI OleLoad(LPSTORAGE pStg, REFIID riid, LPOLECLIENTSITE pClientSite, LPVOID* ppvObj)
{
  FIXME(ole,"(%p,%p,%p,%p), stub!\n", pStg, riid, pClientSite, ppvObj);
  return S_OK;
}

/******************************************************************************
 *              ReadFmtUserTypeStg        [OLE32.136]
 */
HRESULT WINAPI ReadFmtUserTypeStg (LPSTORAGE pstg, CLIPFORMAT* pcf, LPOLESTR* lplpszUserType)
{
  FIXME(ole,"(%p,%p,%p), stub!\n", pstg, pcf, lplpszUserType);
  return S_OK;
}

/******************************************************************************
 *              OleCreateStaticFromData        [OLE32.98]
 */
HRESULT     WINAPI OleCreateStaticFromData(LPDATAOBJECT pSrcDataObj, REFIID iid,
                DWORD renderopt, LPFORMATETC pFormatEtc, LPOLECLIENTSITE pClientSite,
                LPSTORAGE pStg, LPVOID* ppvObj)
{
  FIXME(ole,"(not shown), stub!\n");
  return S_OK;
}

/******************************************************************************
 *              OleRun        [OLE32.123]
 */
HRESULT WINAPI OleRun(LPUNKNOWN pUnknown)
{
  FIXME(ole,"(%p), stub!\n", pUnknown);
  return S_OK;
}

/******************************************************************************
 *              OleSetContainedObject        [OLE32.128]
 */
HRESULT WINAPI OleSetContainedObject(LPUNKNOWN pUnknown, BOOL fContained)
{
  FIXME(ole,"(%p,%x), stub!\n", pUnknown, fContained);
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
  FIXME(ole,"(not shown), stub!\n");
  return S_OK;
}

/******************************************************************************
 *              CreateDataCache        [OLE32.54]
 */
HRESULT WINAPI CreateDataCache(LPUNKNOWN pUnkOuter, REFCLSID rclsid, REFIID iid, LPVOID* ppv)
{
  FIXME(ole,"(%p,%p,%p,%p), stub!\n", pUnkOuter, rclsid, iid, ppv);
  return S_OK;
}

/******************************************************************************
 *              OleIsRunning        [OLE32.111]
 */
BOOL WINAPI OleIsRunning(LPOLEOBJECT pObject)
{
  FIXME(ole,"(%p), stub!\n", pObject);
  return TRUE;
}

/***********************************************************************
 *           OleRegEnumVerbs    [OLE32.120]
 */
HRESULT WINAPI OleRegEnumVerbs (REFCLSID clsid, LPENUMOLEVERB* ppenum)
{
    FIXME(ole,"(%p,%p), stub!\n", clsid, ppenum);
    return S_OK;
}

/***********************************************************************
 *           OleSave     [OLE32.124]
 */
HRESULT WINAPI OleSave(
    LPPERSISTSTORAGE pPS,
    LPSTORAGE pStg,
    BOOL fSameAsLoad)
{
    FIXME(ole,"(%p,%p,%x), stub!\n", pPS, pStg, fSameAsLoad);
    return S_OK;
}

