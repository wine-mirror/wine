/*
 *	ole2.h - Declarations for OLE2
 */

#ifndef __WINE_OLE2_H
#define __WINE_OLE2_H

#include "windef.h"
#include "winerror.h"
#include "wine/obj_storage.h"
#include "wine/obj_moniker.h"
#include "wine/obj_base.h"
#include "wine/obj_dragdrop.h"
#include "wine/obj_inplace.h"
#include "wine/obj_oleobj.h"
#ifndef __WINE__
#include "oleidl.h"
#include "oleauto.h"
#endif
struct tagMSG;

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#define OLEIVERB_PRIMARY            (0L)
#define OLEIVERB_SHOW               (-1L)
#define OLEIVERB_OPEN               (-2L)
#define OLEIVERB_HIDE               (-3L)
#define OLEIVERB_UIACTIVATE         (-4L)
#define OLEIVERB_INPLACEACTIVATE    (-5L)
#define OLEIVERB_DISCARDUNDOSTATE   (-6L)
#define OLEIVERB_PROPERTIES         (-7L)

/*
 * API declarations
 */
HRESULT     WINAPI RegisterDragDrop16(HWND16,LPDROPTARGET);
HRESULT     WINAPI RegisterDragDrop(HWND,LPDROPTARGET);
HRESULT     WINAPI RevokeDragDrop16(HWND16);
HRESULT     WINAPI RevokeDragDrop(HWND);
HRESULT     WINAPI DoDragDrop16(LPDATAOBJECT,LPDROPSOURCE,DWORD,DWORD*);
HRESULT     WINAPI DoDragDrop(LPDATAOBJECT,LPDROPSOURCE,DWORD,DWORD*);
HRESULT  WINAPI OleLoadFromStream(IStream *pStm,REFIID iidInterface,void** ppvObj);
HRESULT  WINAPI OleSaveToStream(IPersistStream *pPStm,IStream *pStm);
HOLEMENU WINAPI OleCreateMenuDescriptor(HMENU hmenuCombined,LPOLEMENUGROUPWIDTHS lpMenuWidths);
HRESULT   WINAPI OleDestroyMenuDescriptor(HOLEMENU hmenuDescriptor);
HRESULT  WINAPI OleSetMenuDescriptor(HOLEMENU hmenuDescriptor,HWND hwndFrame,HWND hwndActiveObject,LPOLEINPLACEFRAME lpFrame,LPOLEINPLACEACTIVEOBJECT lpActiveObject);

HRESULT WINAPI ReadClassStg(IStorage *pstg,CLSID *pclsid);
HRESULT WINAPI WriteClassStm(IStream *pStm,REFCLSID rclsid);
HRESULT WINAPI ReadClassStm(IStream *pStm,CLSID *pclsid);


HRESULT     WINAPI OleSave(LPPERSISTSTORAGE pPS, LPSTORAGE pStg, BOOL fSameAsLoad);
HRESULT     WINAPI OleRegGetUserType(REFCLSID clsid, 
				     DWORD dwFormOfType,
				     LPOLESTR* pszUserType);
HRESULT     WINAPI OleRegGetMiscStatus (REFCLSID clsid, DWORD dwAspect, DWORD* pdwStatus);
HRESULT     WINAPI OleRegEnumFormatEtc (REFCLSID clsid, 
					DWORD    dwDirection,
					LPENUMFORMATETC* ppenumFormatetc);
HRESULT     WINAPI CreateStreamOnHGlobal (HGLOBAL hGlobal, BOOL fDeleteOnRelease, LPSTREAM* ppstm);
HRESULT     WINAPI OleRegEnumVerbs (REFCLSID clsid, LPENUMOLEVERB* ppenum);
BOOL        WINAPI OleIsRunning(LPOLEOBJECT pObject);
HRESULT     WINAPI OleCreateLinkFromData(LPDATAOBJECT pSrcDataObj, REFIID riid,
                DWORD renderopt, LPFORMATETC pFormatEtc,
                LPOLECLIENTSITE pClientSite, LPSTORAGE pStg,
                LPVOID* ppvObj);
HRESULT     WINAPI OleSetContainedObject(LPUNKNOWN pUnknown, BOOL fContained);
HRESULT     WINAPI OleQueryLinkFromData(IDataObject* pSrcDataObject);
HRESULT     WINAPI OleQueryCreateFromData(LPDATAOBJECT pSrcDataObject);
HRESULT     WINAPI OleRun(LPUNKNOWN pUnknown);
VOID        WINAPI ReleaseStgMedium(LPSTGMEDIUM);
HRESULT     WINAPI OleGetClipboard(IDataObject** ppDataObj);
HRESULT     WINAPI OleCreateStaticFromData(LPDATAOBJECT pSrcDataObj, REFIID iid,
                DWORD renderopt, LPFORMATETC pFormatEtc, LPOLECLIENTSITE pClientSite, 
                LPSTORAGE pStg, LPVOID* ppvObj);
HRESULT     WINAPI ReadFmtUserTypeStg(LPSTORAGE pstg, CLIPFORMAT* pcf, LPOLESTR* lplpszUserType);
HRESULT     WINAPI OleLoad(LPSTORAGE pStg, REFIID riid, LPOLECLIENTSITE pClientSite, LPVOID* ppvObj);
HRESULT     WINAPI GetHGlobalFromILockBytes(LPLOCKBYTES plkbyt, HGLOBAL* phglobal);
HRESULT     WINAPI CreateILockBytesOnHGlobal(HGLOBAL hGlobal, BOOL fDeleteOnRelease, LPLOCKBYTES* pplkbyt);
HRESULT     WINAPI CreateDataAdviseHolder(LPDATAADVISEHOLDER* ppDAHolder);
HGLOBAL     WINAPI OleGetIconOfClass(REFCLSID rclsid, LPOLESTR lpszLabel, BOOL fUseTypeAsLabel);
HRESULT     WINAPI OleLockRunning(LPUNKNOWN pUnknown, BOOL fLock, BOOL fLastUnlockCloses);
HRESULT     WINAPI OleCreateFromFile(REFCLSID rclsid, LPCOLESTR lpszFileName, REFIID riid,
                DWORD renderopt, LPFORMATETC lpFormatEtc, LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID* ppvObj);
HRESULT     WINAPI OleCreateLink(LPMONIKER pmkLinkSrc, REFIID riid, DWORD renderopt, LPFORMATETC lpFormatEtc,
                LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID* ppvObj);
HRESULT     WINAPI OleCreate(REFCLSID rclsid, REFIID riid, DWORD renderopt, LPFORMATETC pFormatEtc, LPOLECLIENTSITE pClientSite,
                LPSTORAGE pStg, LPVOID* ppvObj);
HRESULT     WINAPI OleFlushClipboard(void);
HRESULT     WINAPI SetConvertStg(LPSTORAGE pStg, BOOL fConvert);
BOOL        WINAPI IsAccelerator(HACCEL hAccel, int cAccelEntries, struct tagMSG* lpMsg, WORD* lpwCmd);
HRESULT     WINAPI OleCreateLinkToFile(LPCOLESTR lpszFileName, REFIID riid, DWORD renderopt, LPFORMATETC lpFormatEtc,
                LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID* ppvObj);
HRESULT     WINAPI OleDuplicateData(HANDLE hSrc, CLIPFORMAT cfFormat, UINT uiFlags);
HRESULT     WINAPI WriteFmtUserTypeStg(LPSTORAGE pstg, CLIPFORMAT cf, LPOLESTR lpszUserType);
HRESULT     WINAPI OleTranslateAccelerator (LPOLEINPLACEFRAME lpFrame, LPOLEINPLACEFRAMEINFO lpFrameInfo, struct tagMSG* lpmsg);
HRESULT     WINAPI OleCreateFromData(LPDATAOBJECT pSrcDataObj, REFIID riid, DWORD renderopt, LPFORMATETC pFormatEtc,
                LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID* ppvObj);
HRESULT     WINAPI OleCreateDefaultHandler(REFCLSID  clsid,
					   LPUNKNOWN pUnkOuter,
					   REFIID    riid,
					   LPVOID*   ppvObj);
HRESULT     WINAPI CreateOleAdviseHolder (LPOLEADVISEHOLDER *ppOAHolder);
HRESULT     WINAPI OleInitialize(LPVOID pvReserved);
void        WINAPI OleUninitialize(void);

/*
 *  OLE version conversion declarations
 */


typedef struct _OLESTREAM* LPOLESTREAM;
typedef struct _OLESTREAMVTBL {
	DWORD	(CALLBACK *Get)(LPOLESTREAM,LPSTR,DWORD);
	DWORD	(CALLBACK *Put)(LPOLESTREAM,LPSTR,DWORD);
} OLESTREAMVTBL;
typedef OLESTREAMVTBL*	LPOLESTREAMVTBL;
typedef struct _OLESTREAM {
	LPOLESTREAMVTBL	lpstbl;
} OLESTREAM;

HRESULT     WINAPI OleConvertOLESTREAMToIStorage( LPOLESTREAM lpolestream, LPSTORAGE pstg, const DVTARGETDEVICE* ptd);
HRESULT     WINAPI OleConvertIStorageToOLESTREAM( LPSTORAGE pstg, LPOLESTREAM lpolestream);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif  /* __WINE_OLE2_H */

