/*
 * Copyright (C) 1999 Paul Quinn
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

#include <rpc.h>
#include <rpcndr.h>
#ifndef COM_NO_WINDOWS_H
#include <windows.h>
#include <ole2.h>
#endif

#ifndef __WINE_DOCOBJ_H
#define __WINE_DOCOBJ_H

#include <winbase.h>
#include <windef.h>
#include <wingdi.h>
#include <winuser.h>
#include <ole2.h>
#define __WINE_INCLUDE_OAIDL
#include <oaidl.h>
#undef __WINE_INCLUDE_OAIDL

/*****************************************************************************
 * Declare the structures
 */
typedef enum
{
	OLECMDF_SUPPORTED = 0x1,
	OLECMDF_ENABLED = 0x2,
	OLECMDF_LATCHED = 0x4,
	OLECMDF_NINCHED = 0x8
} OLECMDF;

typedef struct _tagOLECMD
{
	ULONG cmdID;
	DWORD cmdf;
} OLECMD;

typedef struct _tagOLECMDTEXT
{
	DWORD cmdtextf;
	ULONG cwActual;
	ULONG cwBuf;
	WCHAR rgwz[1];
} OLECMDTEXT;

typedef enum
{
	PRINTFLAG_MAYBOTHERUSER = 1,
	PRINTFLAG_PROMPTUSER  = 2,
	PRINTFLAG_USERMAYCHANGEPRINTER  = 4,
	PRINTFLAG_RECOMPOSETODEVICE = 8,
	PRINTFLAG_DONTACTUALLYPRINT = 16,
	PRINTFLAG_FORCEPROPERTIES = 32,
	PRINTFLAG_PRINTTOFILE = 64
} PRINTFLAG;

typedef struct tagPAGERANGE
{
	LONG nFromPage;
	LONG nToPage;
} PAGERANGE;

typedef struct  tagPAGESET
{
	ULONG cbStruct;
	BOOL fOddPages;
	BOOL fEvenPages;
	ULONG cPageRange;
	PAGERANGE rgPages[ 1 ];
} PAGESET;

typedef enum
{
	OLECMDTEXTF_NONE  = 0,
	OLECMDTEXTF_NAME  = 1,
	OLECMDTEXTF_STATUS  = 2
} OLECMDTEXTF;

typedef enum
{
	OLECMDEXECOPT_DODEFAULT = 0,
	OLECMDEXECOPT_PROMPTUSER  = 1,
	OLECMDEXECOPT_DONTPROMPTUSER  = 2,
	OLECMDEXECOPT_SHOWHELP  = 3
} OLECMDEXECOPT;

typedef enum OLECMDID
{
	OLECMDID_OPEN = 1,
	OLECMDID_NEW = 2,
	OLECMDID_SAVE = 3,
	OLECMDID_SAVEAS = 4,
	OLECMDID_SAVECOPYAS = 5,
	OLECMDID_PRINT = 6,
	OLECMDID_PRINTPREVIEW = 7,
	OLECMDID_PAGESETUP = 8,
	OLECMDID_SPELL = 9,
	OLECMDID_PROPERTIES = 10,
	OLECMDID_CUT = 11,
	OLECMDID_COPY = 12,
	OLECMDID_PASTE = 13,
	OLECMDID_PASTESPECIAL = 14,
	OLECMDID_UNDO = 15,
	OLECMDID_REDO = 16,
	OLECMDID_SELECTALL = 17,
	OLECMDID_CLEARSELECTION = 18,
	OLECMDID_ZOOM = 19,
	OLECMDID_GETZOOMRANGE = 20,
	OLECMDID_UPDATECOMMANDS = 21,
	OLECMDID_REFRESH = 22,
	OLECMDID_STOP = 23,
	OLECMDID_HIDETOOLBARS = 24,
	OLECMDID_SETPROGRESSMAX = 25,
	OLECMDID_SETPROGRESSPOS = 26,
	OLECMDID_SETPROGRESSTEXT = 27,
	OLECMDID_SETTITLE = 28,
	OLECMDID_SETDOWNLOADSTATE = 29,
	OLECMDID_STOPDOWNLOAD = 30,
	OLECMDID_ONTOOLBARACTIVATED = 31,
	OLECMDID_FIND = 32,
	OLECMDID_DELETE = 33,
	OLECMDID_HTTPEQUIV = 34,
	OLECMDID_HTTPEQUIV_DONE = 35,
	OLECMDID_ENABLE_INTERACTION = 36,
	OLECMDID_ONUNLOAD = 37,
	OLECMDID_PROPERTYBAG2 = 38,
	OLECMDID_PREREFRESH = 39,
	/* iexplorer uses ... 44 */
	OLECMDID_SHOWSCRIPTERROR        = 40,
       OLECMDID_SHOWMESSAGE    = 41,
       OLECMDID_SHOWFIND       = 42,
       OLECMDID_SHOWPAGESETUP  = 43,
       OLECMDID_SHOWPRINT      = 44,
       OLECMDID_CLOSE  = 45,
       OLECMDID_ALLOWUILESSSAVEAS      = 46,
       OLECMDID_DONTDOWNLOADCSS        = 47,
       OLECMDID_UPDATEPAGESTATUS       = 48,
       OLECMDID_PRINT2 = 49,
       OLECMDID_PRINTPREVIEW2  = 50,
       OLECMDID_SETPRINTTEMPLATE       = 51,
       OLECMDID_GETPRINTTEMPLATE       = 52
}	OLECMDID;

#define OLECMDERR_E_FIRST            (OLE_E_LAST+1)
#define OLECMDERR_E_NOTSUPPORTED     (OLECMDERR_E_FIRST)
#define OLECMDERR_E_DISABLED         (OLECMDERR_E_FIRST+1)
#define OLECMDERR_E_NOHELP           (OLECMDERR_E_FIRST+2)
#define OLECMDERR_E_CANCELED         (OLECMDERR_E_FIRST+3)
#define OLECMDERR_E_UNKNOWNGROUP     (OLECMDERR_E_FIRST+4)

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID(IID_IOleDocument, 0xb722bcc5,0x4e68,0x101b,0xa2,0xbc,0x00,0xaa,0x00,0x40,0x47,0x70);
typedef struct IOleDocument IOleDocument, *LPOLEDOCUMENT;

DEFINE_GUID(IID_IOleDocumentSite, 0xb722bcc7,0x4e68,0x101b,0xa2,0xbc,0x00,0xaa,0x00,0x40,0x47,0x70);
typedef struct IOleDocumentSite IOleDocumentSite, *LPOLEDOCUMENTSITE;

DEFINE_GUID(IID_IOleDocumentView, 0xb722bcc6,0x4e68,0x101b,0xa2,0xbc,0x00,0xaa,0x00,0x40,0x47,0x70);
typedef struct IOleDocumentView IOleDocumentView, *LPOLEDOCUMENTVIEW;

DEFINE_GUID(IID_IEnumOleDocumentViews, 0xb722bcc6,0x4e68,0x101b,0xa2,0xbc,0x00,0xaa,0x00,0x40,0x47,0x70);
typedef struct IEnumOleDocumentViews IEnumOleDocumentViews, *LPENUMOLEDOCUMENTVIEWS;

DEFINE_GUID(IID_IOleCommandTarget, 0xb722bccb,0x4e68,0x101b,0xa2,0xbc,0x00,0xaa,0x00,0x40,0x47,0x70);
typedef struct IOleCommandTarget IOleCommandTarget, *LPOLECOMMANDTARGET;

DEFINE_GUID(IID_IContinueCallback, 0xb722bcca,0x4e68,0x101b,0xa2,0xbc,0x00,0xaa,0x00,0x40,0x47,0x70);
typedef struct IContinueCallback IContinueCallback, *LPCONTINUECALLBACK;

DEFINE_GUID(IID_IPrint, 0xb722bcc9,0x4e68,0x101b,0xa2,0xbc,0x00,0xaa,0x00,0x40,0x47,0x70);
typedef struct IPrint IPrint, *LPPRINT;


/*****************************************************************************
 * IOleDocument interface
 */
#define INTERFACE IOleDocument
#define IOleDocument_METHODS \
	IUnknown_METHODS \
	STDMETHOD(CreateView)(THIS_ IOleInPlaceSite *pIPSite, IStream *pstm, DWORD dwReserved, IOleDocumentView **ppView) PURE; \
	STDMETHOD(GetDocMiscStatus)(THIS_ DWORD *pdwStatus) PURE; \
	STDMETHOD(EnumViews)(THIS_ IEnumOleDocumentViews **ppEnum, IOleDocumentView **ppView) PURE;
ICOM_DEFINE(IOleDocument,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IOleDocument_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IOleDocument_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IOleDocument_Release(p)                 (p)->lpVtbl->Release(p)
/*** IOleDocument methods ***/
#define IOleDocument_CreateView(p,a,b,c,d)      (p)->lpVtbl->CreateView(p,a,b,c,d)
#define IOleDocument_GetDocMiscStatus(p,a)      (p)->lpVtbl->GetDocMiscStatus(p,a)
#define IOleDocument_EnumViews(p,a,b)           (p)->lpVtbl->EnumViews(p,a,b)
#endif


/*****************************************************************************
 * IOleDocumentSite interface
 */
#define INTERFACE IOleDocumentSite
#define IOleDocumentSite_METHODS \
	IUnknown_METHODS \
	STDMETHOD(ActivateMe)(THIS_ IOleDocumentView *pViewToActivate) PURE;
ICOM_DEFINE(IOleDocumentSite,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IOleDocumentSite_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IOleDocumentSite_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IOleDocumentSite_Release(p)                 (p)->lpVtbl->Release(p)
/*** IOleDocumentSite methods ***/
#define IOleDocumentSite_ActivateMe(p,a)            (p)->lpVtbl->ActivateMe(p,a)
#endif


/*****************************************************************************
 * IOleDocumentSite interface
 */
#define INTERFACE IOleDocumentView
#define IOleDocumentView_METHODS \
	IUnknown_METHODS \
	STDMETHOD(SetInPlaceSite)(THIS_ IOleInPlaceSite *pIPSite) PURE; \
	STDMETHOD(GetInPlaceSite)(THIS_ IOleInPlaceSite **ppIPSite) PURE; \
	STDMETHOD(GetDocument)(THIS_ IUnknown **ppunk) PURE; \
	STDMETHOD(SetRect)(THIS_ LPRECT prcView) PURE; \
	STDMETHOD(GetRect)(THIS_ LPRECT prcView) PURE; \
	STDMETHOD(SetRectComplex)(THIS_ LPRECT prcView, LPRECT prcHScroll, LPRECT prcVScroll, LPRECT prcSizeBox) PURE; \
	STDMETHOD(Show)(THIS_ BOOL fShow) PURE; \
	STDMETHOD(UIActivate)(THIS_ BOOL fUIActivate) PURE; \
	STDMETHOD(Open)(THIS) PURE; \
	STDMETHOD(CloseView)(THIS_ DWORD dwReserved) PURE; \
	STDMETHOD(SaveViewState)(THIS_ LPSTREAM pstm) PURE; \
	STDMETHOD(ApplyViewState)(THIS_ LPSTREAM pstm) PURE; \
	STDMETHOD(Clone)(THIS_ IOleInPlaceSite *pIPSiteNew, IOleDocumentView **ppViewNew) PURE;
ICOM_DEFINE(IOleDocumentView,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IOleDocumentView_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IOleDocumentView_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IOleDocumentView_Release(p)                 (p)->lpVtbl->Release(p)
/*** IOleDocumentView methods ***/
#define IOleDocumentView_SetInPlaceSite(p,a)        (p)->lpVtbl->SetInPlaceSite(p,a)
#define IOleDocumentView_GetInPlaceSite(p,a)        (p)->lpVtbl->GetInPlaceSite(p,a)
#define IOleDocumentView_GetDocument(p,a)           (p)->lpVtbl->GetDocument(p,a)
#define IOleDocumentView_SetRect(p,a)               (p)->lpVtbl->SetRect(p,a)
#define IOleDocumentView_GetRect(p,a)               (p)->lpVtbl->GetRect(p,a)
#define IOleDocumentView_GetRectComplex(p,a,b,c,d)  (p)->lpVtbl->GetRectComplex(p,a,b,c,d)
#define IOleDocumentView_Show(p,a)                  (p)->lpVtbl->Show(p,a)
#define IOleDocumentView_UIActivate(p,a)            (p)->lpVtbl->UIActivate(p,a)
#define IOleDocumentView_Open(p)                    (p)->lpVtbl->Open(p)
#define IOleDocumentView_CloseView(p,a)             (p)->lpVtbl->CloseView(p,a)
#define IOleDocumentView_SaveViewState(p,a)         (p)->lpVtbl->SaveViewState(p,a)
#define IOleDocumentView_ApplyViewState(p,a)        (p)->lpVtbl->ApplyViewState(p,a)
#define IOleDocumentView_Clone(p,a,b)               (p)->lpVtbl->Clone(p,a,b)
#endif


/*****************************************************************************
 * IOleDocumentSite interface
 */
#define INTERFACE IEnumOleDocumentViews
#define IEnumOleDocumentViews_METHODS \
	IUnknown_METHODS \
	STDMETHOD(Next)(THIS_ ULONG cViews, IOleDocumentView **rgpView, ULONG *pcFetched) PURE; \
	STDMETHOD(Skip)(THIS_ ULONG cViews) PURE; \
	STDMETHOD(Reset)(THIS) PURE; \
	STDMETHOD(Clone)(THIS_ IEnumOleDocumentViews **ppEnum) PURE;
ICOM_DEFINE(IEnumOleDocumentViews,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IEnumOleDocumentViews_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IEnumOleDocumentViews_AddRef(p)               (p)->lpVtbl->AddRef(p)
#define IEnumOleDocumentViews_Release(p)              (p)->lpVtbl->Release(p)
/*** IEnumOleDocumentViews methods ***/
#define IEnumOleDocumentViews_Next(p,a,b,c)           (p)->lpVtbl->Next(p,a,b,c)
#define IEnumOleDocumentViews_Skip(p,a)               (p)->lpVtbl->Skip(p,a)
#define IEnumOleDocumentViews_Reset(p)                (p)->lpVtbl->Reset(p)
#define IEnumOleDocumentViews_Clone(p,a)              (p)->lpVtbl->Clone(p,a)
#endif


/*****************************************************************************
 * IOleCommandTarget interface
 */
#define INTERFACE IOleCommandTarget
#define IOleCommandTarget_METHODS \
	IUnknown_METHODS \
	STDMETHOD(QueryStatus)(THIS_ const GUID *pguidCmdGroup, ULONG cCmds, OLECMD *prgCmds, OLECMDTEXT *pCmdText) PURE; \
	STDMETHOD(Exec)(THIS_ const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut) PURE;
ICOM_DEFINE(IOleCommandTarget,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IOleCommandTarget_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IOleCommandTarget_AddRef(p)               (p)->lpVtbl->AddRef(p)
#define IOleCommandTarget_Release(p)              (p)->lpVtbl->Release(p)
/*** IOleCommandTarget methods ***/
#define IOleCommandTarget_QueryStatus(p,a,b,c,d)  (p)->lpVtbl->QueryStatus(p,a,b,c,d)
#define IOleCommandTarget_Exec(p,a,b,c,d,e)       (p)->lpVtbl->Exec(p,a,b,c,d,e)
#endif


/*****************************************************************************
 * IContinueCallback interface
 */
#define INTERFACE IContinueCallback
#define IContinueCallback_METHODS \
	IUnknown_METHODS \
	STDMETHOD(FContinue)(THIS) PURE; \
	STDMETHOD(FContinuePrinting)(THIS_ LONG nCntPrinted, LONG nCurPage, WCHAR *pwszPrintStatus) PURE;
ICOM_DEFINE(IContinueCallback,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IContinueCallback_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IContinueCallback_AddRef(p)               (p)->lpVtbl->AddRef(p)
#define IContinueCallback_Release(p)              (p)->lpVtbl->Release(p)
/*** IContinueCallback methods ***/
#define IContinueCallback_FContinue(p)               (p)->lpVtbl->FContinue(p)
#define IContinueCallback_FContinuePrinting(p,a,b,c) (p)->lpVtbl->FContinuePrinting(p,a,b,c)
#endif


/*****************************************************************************
 * IPrint interface
 */
#define INTERFACE IPrint
#define IPrint_METHODS \
	IUnknown_METHODS \
	STDMETHOD(SetInitialPageNum)(THIS_ LONG nFirstPage) PURE; \
	STDMETHOD(GetPageInfo)(THIS_ LONG *pnFirstPage, LONG *pcPages) PURE; \
	STDMETHOD(Print)(THIS_ DWORD grfFlags, DVTARGETDEVICE **pptd, PAGESET **ppPageSet, STGMEDIUM *pstgmOptions, IContinueCallback *pcallback, LONG nFirstPage, LONG *pcPagesPrinted, LONG *pnLastPage) PURE;
ICOM_DEFINE(IPrint,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IPrint_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IPrint_AddRef(p)               (p)->lpVtbl->AddRef(p)
#define IPrint_Release(p)              (p)->lpVtbl->Release(p)
/*** Iprint methods ***/
#define IPrint_SetInitialPageNum(p,a)  (p)->lpVtbl->SetInitialPageNum(p,a)
#define IPrint_GetPageInfo(p,a,b)      (p)->lpVtbl->GetPageInfo(p,a,b)
#define IPrint_Print(p,a,b,c,d,e,f,g)  (p)->lpVtbl->Print(p,a,b,c,d,e,f,g)
#endif


#define LPMSODOCUMENT                  LPOLEDOCUMENT
#define LPMSODOCUMENTSITE              LPOLEDOCUMENTSITE
#define LPMSOVIEW                      LPOLEDOCUMENTVIEW
#define LPENUMMSOVIEW                  LPENUMOLEDOCUMENTVIEWS
#define LPMSOCOMMANDTARGET             LPOLECOMMANDTARGET
#define IID_IMsoDocument               IID_IOleDocument
#define IID_IMsoDocumentSite           IID_IOleDocumentSite
#define IID_IMsoView                   IID_IOleDocumentView
#define IID_IEnumMsoView               IID_IEnumOleDocumentViews
#define IID_IMsoCommandTarget          IID_IOleCommandTarget


#endif /* __WINE_DOCOBJ_H */
