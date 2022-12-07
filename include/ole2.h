/*
 * Declarations for OLE2
 *
 * Copyright (C) the Wine project
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_OLE2_H
#define __WINE_OLE2_H

#include <winerror.h>
#include <objbase.h>
#include <oleauto.h>
#include <oleidl.h>

struct tagMSG;

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#define E_DRAW                  VIEW_E_DRAW
#define DATA_E_FORMATETC        DV_E_FORMATETC

#define OLEIVERB_PRIMARY            (__MSABI_LONG(0))
#define OLEIVERB_SHOW               (__MSABI_LONG(-1))
#define OLEIVERB_OPEN               (__MSABI_LONG(-2))
#define OLEIVERB_HIDE               (__MSABI_LONG(-3))
#define OLEIVERB_UIACTIVATE         (__MSABI_LONG(-4))
#define OLEIVERB_INPLACEACTIVATE    (__MSABI_LONG(-5))
#define OLEIVERB_DISCARDUNDOSTATE   (__MSABI_LONG(-6))
#define OLEIVERB_PROPERTIES         (__MSABI_LONG(-7))

#define EMBDHLP_INPROC_HANDLER  0x00000000
#define EMBDHLP_INPROC_SERVER   0x00000001
#define EMBDHLP_CREATENOW       0x00000000
#define EMBDHLP_DELAYCREATE     0x00010000

/*
 * API declarations
 */
WINOLE32API HRESULT  WINAPI RegisterDragDrop(HWND,LPDROPTARGET);
WINOLE32API HRESULT  WINAPI RevokeDragDrop(HWND);
WINOLE32API HRESULT  WINAPI DoDragDrop(LPDATAOBJECT,LPDROPSOURCE,DWORD,DWORD*);
WINOLE32API HRESULT  WINAPI OleLoadFromStream(IStream *pStm,REFIID iidInterface,void** ppvObj);
WINOLE32API HRESULT  WINAPI OleSaveToStream(IPersistStream *pPStm,IStream *pStm);
WINOLE32API HOLEMENU WINAPI OleCreateMenuDescriptor(HMENU hmenuCombined,LPOLEMENUGROUPWIDTHS lpMenuWidths);
WINOLE32API HRESULT  WINAPI OleDestroyMenuDescriptor(HOLEMENU hmenuDescriptor);
WINOLE32API HRESULT  WINAPI OleSetMenuDescriptor(HOLEMENU hmenuDescriptor,HWND hwndFrame,HWND hwndActiveObject,LPOLEINPLACEFRAME lpFrame,LPOLEINPLACEACTIVEOBJECT lpActiveObject);

WINOLE32API HRESULT WINAPI WriteClassStg(IStorage *pstg, REFCLSID rclsid);
WINOLE32API HRESULT WINAPI ReadClassStg(IStorage *pstg,CLSID *pclsid);
WINOLE32API HRESULT WINAPI WriteClassStm(IStream *pStm,REFCLSID rclsid);
WINOLE32API HRESULT WINAPI ReadClassStm(IStream *pStm,CLSID *pclsid);


WINOLE32API HRESULT     WINAPI OleSave(LPPERSISTSTORAGE pPS, LPSTORAGE pStg, BOOL fSameAsLoad);
WINOLE32API HRESULT     WINAPI OleRegGetUserType(REFCLSID clsid, DWORD dwFormOfType, LPOLESTR* pszUserType);
WINOLE32API HRESULT     WINAPI OleRegGetMiscStatus (REFCLSID clsid, DWORD dwAspect, DWORD* pdwStatus);
WINOLE32API HRESULT     WINAPI OleRegEnumFormatEtc (REFCLSID clsid, DWORD dwDirection, LPENUMFORMATETC* ppenumFormatetc);
WINOLE32API HRESULT     WINAPI CreateStreamOnHGlobal (HGLOBAL hGlobal, BOOL fDeleteOnRelease, LPSTREAM* ppstm);
WINOLE32API HRESULT     WINAPI GetHGlobalFromStream(LPSTREAM pstm, HGLOBAL* phglobal);
WINOLE32API HRESULT     WINAPI OleRegEnumVerbs (REFCLSID clsid, LPENUMOLEVERB* ppenum);
WINOLE32API BOOL        WINAPI OleIsRunning(LPOLEOBJECT pObject);
WINOLE32API HRESULT     WINAPI OleCreateLinkFromData(LPDATAOBJECT pSrcDataObj, REFIID riid, DWORD renderopt, LPFORMATETC pFormatEtc, LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID* ppvObj);
WINOLE32API HRESULT     WINAPI OleSetContainedObject(LPUNKNOWN pUnknown, BOOL fContained);
WINOLE32API HRESULT     WINAPI OleNoteObjectVisible(LPUNKNOWN pUnknown, BOOL fVisible);
WINOLE32API HRESULT     WINAPI OleQueryLinkFromData(IDataObject* pSrcDataObject);
WINOLE32API HRESULT     WINAPI OleQueryCreateFromData(LPDATAOBJECT pSrcDataObject);
WINOLE32API HRESULT     WINAPI OleRun(LPUNKNOWN pUnknown);
WINOLE32API HRESULT     WINAPI OleDraw(LPUNKNOWN pUnknown, DWORD dwAspect, HDC hdcDraw, LPCRECT lprcBounds);
WINOLE32API VOID        WINAPI ReleaseStgMedium(LPSTGMEDIUM);
WINOLE32API HRESULT     WINAPI OleGetClipboard(IDataObject** ppDataObj);
WINOLE32API HRESULT     WINAPI OleIsCurrentClipboard(LPDATAOBJECT);
WINOLE32API HRESULT     WINAPI OleSetClipboard(LPDATAOBJECT);
WINOLE32API HRESULT     WINAPI OleCreateStaticFromData(LPDATAOBJECT pSrcDataObj, REFIID iid, DWORD renderopt, LPFORMATETC pFormatEtc, LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID* ppvObj);
WINOLE32API HRESULT     WINAPI ReadFmtUserTypeStg(LPSTORAGE pstg, CLIPFORMAT* pcf, LPOLESTR* lplpszUserType);
WINOLE32API HRESULT     WINAPI OleLoad(LPSTORAGE pStg, REFIID riid, LPOLECLIENTSITE pClientSite, LPVOID* ppvObj);
WINOLE32API HRESULT     WINAPI GetHGlobalFromILockBytes(LPLOCKBYTES plkbyt, HGLOBAL* phglobal);
WINOLE32API HRESULT     WINAPI CreateILockBytesOnHGlobal(HGLOBAL hGlobal, BOOL fDeleteOnRelease, LPLOCKBYTES* pplkbyt);
WINOLE32API HRESULT     WINAPI CreateDataAdviseHolder(LPDATAADVISEHOLDER* ppDAHolder);
WINOLE32API HGLOBAL     WINAPI OleGetIconOfClass(REFCLSID rclsid, LPOLESTR lpszLabel, BOOL fUseTypeAsLabel);
WINOLE32API HGLOBAL     WINAPI OleGetIconOfFile(LPOLESTR lpszPath, BOOL fUseFileAsLabel);
WINOLE32API HGLOBAL     WINAPI OleMetafilePictFromIconAndLabel(HICON hIcon, LPOLESTR lpszLabel, LPOLESTR lpszSourceFile, UINT iIconIndex);
WINOLE32API HRESULT     WINAPI OleLockRunning(LPUNKNOWN pUnknown, BOOL fLock, BOOL fLastUnlockCloses);
WINOLE32API HRESULT     WINAPI OleCreateFromFile(REFCLSID rclsid, LPCOLESTR lpszFileName, REFIID riid,
                                                 DWORD renderopt, LPFORMATETC lpFormatEtc, LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID* ppvObj);
WINOLE32API HRESULT     WINAPI OleCreateFromFileEx(REFCLSID clsid, LPCOLESTR filename, REFIID iid, DWORD flags,
                                                   DWORD renderopt, ULONG num_fmts, DWORD *adv_flags, LPFORMATETC fmts, IAdviseSink *sink,
                                                   DWORD *conns, LPOLECLIENTSITE client_site, LPSTORAGE storage, LPVOID* obj);
WINOLE32API HRESULT     WINAPI OleCreateLink(LPMONIKER pmkLinkSrc, REFIID riid, DWORD renderopt, LPFORMATETC lpFormatEtc,LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID* ppvObj);
WINOLE32API HRESULT     WINAPI OleCreate(REFCLSID rclsid, REFIID riid, DWORD renderopt, LPFORMATETC pFormatEtc, LPOLECLIENTSITE pClientSite,LPSTORAGE pStg, LPVOID* ppvObj);
WINOLE32API HRESULT     WINAPI OleFlushClipboard(void);
WINOLE32API HRESULT     WINAPI GetConvertStg(LPSTORAGE pStg);
WINOLE32API HRESULT     WINAPI SetConvertStg(LPSTORAGE pStg, BOOL fConvert);
WINOLE32API BOOL        WINAPI IsAccelerator(HACCEL hAccel, int cAccelEntries, struct tagMSG* lpMsg, WORD* lpwCmd);
WINOLE32API HRESULT     WINAPI OleCreateLinkToFile(LPCOLESTR lpszFileName, REFIID riid, DWORD renderopt, LPFORMATETC lpFormatEtc,
                                                   LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID* ppvObj);
WINOLE32API HANDLE      WINAPI OleDuplicateData(HANDLE hSrc, CLIPFORMAT cfFormat, UINT uiFlags);
WINOLE32API HRESULT     WINAPI WriteFmtUserTypeStg(LPSTORAGE pstg, CLIPFORMAT cf, LPOLESTR lpszUserType);
WINOLE32API HRESULT     WINAPI OleTranslateAccelerator (LPOLEINPLACEFRAME lpFrame, LPOLEINPLACEFRAMEINFO lpFrameInfo, struct tagMSG* lpmsg);
WINOLE32API HRESULT     WINAPI OleCreateFromData(LPDATAOBJECT pSrcDataObj, REFIID riid, DWORD renderopt, LPFORMATETC pFormatEtc,
                                                 LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID* ppvObj);
WINOLE32API HRESULT     WINAPI OleCreateFromDataEx(LPDATAOBJECT pSrcDataObj, REFIID riid, DWORD dwFlags, DWORD renderopt, ULONG num_formats,
                                                   DWORD *adv_flags, LPFORMATETC fmts, IAdviseSink *sink, DWORD *conns,
                                                   LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID* ppvObj);
WINOLE32API HRESULT     WINAPI OleCreateDefaultHandler(REFCLSID clsid, LPUNKNOWN pUnkOuter, REFIID riid, LPVOID* ppvObj);
WINOLE32API HRESULT     WINAPI OleCreateEmbeddingHelper(REFCLSID clsid, LPUNKNOWN pUnkOuter, DWORD flags, IClassFactory *pCF, REFIID riid, LPVOID* ppvObj);
WINOLE32API HRESULT     WINAPI CreateOleAdviseHolder (LPOLEADVISEHOLDER *ppOAHolder);
WINOLE32API HRESULT     WINAPI OleInitialize(LPVOID pvReserved);
WINOLE32API void        WINAPI OleUninitialize(void);
WINOLE32API BOOL        WINAPI IsValidInterface(LPUNKNOWN punk);
WINOLE32API DWORD       WINAPI OleBuildVersion(VOID);

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

WINOLE32API HRESULT     WINAPI OleConvertOLESTREAMToIStorage( LPOLESTREAM lpolestream, LPSTORAGE pstg, const DVTARGETDEVICE* ptd);
WINOLE32API HRESULT     WINAPI OleConvertIStorageToOLESTREAM( LPSTORAGE pstg, LPOLESTREAM lpolestream);
WINOLE32API HRESULT     WINAPI OleConvertIStorageToOLESTREAMEx( LPSTORAGE stg, CLIPFORMAT cf, LONG width, LONG height,
                                                                DWORD size, LPSTGMEDIUM medium, LPOLESTREAM olestream );
WINOLE32API HRESULT     WINAPI OleDoAutoConvert( LPSTORAGE pStg, LPCLSID pClsidNew );
WINOLE32API HRESULT     WINAPI OleGetAutoConvert( REFCLSID clsidOld, LPCLSID pClsidNew );
WINOLE32API HRESULT     WINAPI OleSetAutoConvert( REFCLSID clsidOld, REFCLSID clsidNew );

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif  /* __WINE_OLE2_H */
