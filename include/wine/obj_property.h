/*
 * Defines the COM interfaces and APIs from ocidl.h related to property
 *
 * Depends on 'obj_base.h'.
 *
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

#ifndef __WINE_WINE_OBJ_PROPERTY_H
#define __WINE_WINE_OBJ_PROPERTY_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Declare the structures
 */
typedef struct tagPROPPAGEINFO
{
	ULONG cb;
	LPOLESTR pszTitle;
	SIZE size;
	LPOLESTR pszDocString;
	LPOLESTR pszHelpFile;
	DWORD dwHelpContext;
} PROPPAGEINFO, *LPPROPPAGEINFO;

typedef enum tagPROPPAGESTATUS
{
	PROPPAGESTATUS_DIRTY = 0x1,
	PROPPAGESTATUS_VALIDATE = 0x2,
	PROPPAGESTATUS_CLEAN = 0x4
} PROPPAGESTATUS;

typedef struct tagCAUUID
{
	ULONG cElems;
	GUID* pElems;
} CAUUID, *LPCAUUID;

typedef struct tagCALPOLESTR
{
	ULONG cElems;
	LPOLESTR *pElems;
} CALPOLESTR, *LPCALPOLESTR;

typedef struct tagCADWORD
{
	ULONG cElems;
	DWORD *pElems;
} CADWORD, *LPCADWORD;


typedef enum tagPROPBAG2_TYPE
{
	PROPBAG2_TYPE_UNDEFINED = 0,
	PROPBAG2_TYPE_DATA = 1,
	PROPBAG2_TYPE_URL = 2,
	PROPBAG2_TYPE_OBJECT = 3,
	PROPBAG2_TYPE_STREAM = 4,
	PROPBAG2_TYPE_STORAGE = 5,
	PROPBAG2_TYPE_MONIKER = 6
} PROPBAG2_TYPE;

typedef struct tagPROPBAG2
{
	DWORD dwType;
	VARTYPE vt;
	CLIPFORMAT cfType;
	DWORD dwHint;
	LPOLESTR pstrName;
	CLSID clsid;
} PROPBAG2;

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID(IID_IPropertyPage, 0xb196b28dL, 0xbab4, 0x101a, 0xb6, 0x9c, 0x00, 0xaa, 0x00, 0x34, 0x1d, 0x07);
typedef struct IPropertyPage IPropertyPage, *LPPROPERTYPAGE;

DEFINE_GUID(IID_IPropertyPage2, 0x01e44665L, 0x24ac, 0x101b, 0x84, 0xed, 0x08, 0x00, 0x2b, 0x2e, 0xc7, 0x13);
typedef struct IPropertyPage2 IPropertyPage2, *LPPROPERTYPAGE2;

DEFINE_GUID(IID_IPropertyPageSite, 0xb196b28cL, 0xbab4, 0x101a, 0xb6, 0x9c, 0x00, 0xaa, 0x00, 0x34, 0x1d, 0x07);
typedef struct IPropertyPageSite IPropertyPageSite, *LPPROPERTYPAGESITE;

DEFINE_GUID(IID_IPropertyNotifySink, 0x9bfbbc02L, 0xeff1, 0x101a, 0x84, 0xed, 0x00, 0xaa, 0x00, 0x34, 0x1d, 0x07);
typedef struct IPropertyNotifySink IPropertyNotifySink, *LPPROPERTYNOTIFYSINK;

DEFINE_GUID(IID_ISimpleFrameSite, 0x742b0e01L, 0x14e6, 0x101b, 0x91, 0x4e, 0x00, 0xaa, 0x00, 0x30, 0x0c, 0xab);
typedef struct ISimpleFrameSite ISimpleFrameSite, *LPSIMPLEFRAMESITE;

DEFINE_GUID(IID_IPersistStreamInit, 0x7fd52380L, 0x4e07, 0x101b, 0xae, 0x2d, 0x08, 0x00, 0x2b, 0x2e, 0xc7, 0x13);
typedef struct IPersistStreamInit IPersistStreamInit,*LPPERSISTSTREAMINIT;

DEFINE_GUID(IID_IPersistMemory, 0xbd1ae5e0L, 0xa6ae, 0x11ce, 0xbd, 0x37, 0x50, 0x42, 0x00, 0xc1, 0x00, 0x00);
typedef struct IPersistMemory IPersistMemory,*LPPERSISTMEMORY;

DEFINE_GUID(IID_IPersistPropertyBag, 0x37d84f60, 0x42cb, 0x11ce, 0x81, 0x35, 0x00, 0xaa, 0x00, 0x4b, 0xb8, 0x51);
typedef struct IPersistPropertyBag IPersistPropertyBag,*LPPERSISTPROPERTYBAG;

DEFINE_GUID(IID_IPersistPropertyBag2, 0x22f55881, 0x280b, 0x11d0, 0xa8, 0xa9, 0x00, 0xa0, 0xc9, 0x0c, 0x20, 0x04);
typedef struct IPersistPropertyBag2 IPersistPropertyBag2,*LPPERSISTPROPERTYBAG2;

DEFINE_GUID(IID_IErrorLog, 0x3127ca40L, 0x446e, 0x11ce, 0x81, 0x35, 0x00, 0xaa, 0x00, 0x4b, 0xb8, 0x51);
typedef struct IErrorLog IErrorLog,*LPERRORLOG;

DEFINE_GUID(IID_IPropertyBag, 0x55272a00L, 0x42cb, 0x11ce, 0x81, 0x35, 0x00, 0xaa, 0x00, 0x4b, 0xb8, 0x51);
typedef struct IPropertyBag IPropertyBag,*LPPROPERTYBAG;

DEFINE_GUID(IID_IPropertyBag2, 0x22f55882, 0x280b, 0x11d0, 0xa8, 0xa9, 0x00, 0xa0, 0xc9, 0x0c, 0x20, 0x04);
typedef struct IPropertyBag2 IPropertyBag2,*LPPROPERTYBAG2;

DEFINE_GUID(IID_ISpecifyPropertyPages, 0xb196b28b, 0xbab4, 0x101a, 0xb6, 0x9c, 0x00, 0xaa, 0x00, 0x34, 0x1d, 0x07);
typedef struct ISpecifyPropertyPages ISpecifyPropertyPages,*LPSPECIFYPROPERTYPAGES;

DEFINE_GUID(IID_IPerPropertyBrowsing, 0xb196b28b, 0xbab4, 0x101a, 0xb6, 0x9c, 0x00, 0xaa, 0x00, 0x34, 0x1d, 0x07);
typedef struct IPerPropertyBrowsing IPerPropertyBrowsing,*LPPERPROPERTYBROWSING;


/*****************************************************************************
 * IPropertPage interface
 */
#define INTERFACE IPropertyPage
#define IPropertyPage_METHODS \
	STDMETHOD(SetPageSite)(THIS_ IPropertyPageSite *pPageSite) PURE; \
	STDMETHOD(Activate)(THIS_ HWND hWndParent, LPCRECT pRect, BOOL bModal) PURE; \
	STDMETHOD(Deactivate)(THIS) PURE; \
	STDMETHOD(GetPageInfo)(THIS_ PROPPAGEINFO *pPageInfo) PURE; \
	STDMETHOD(SetObjects)(THIS_ ULONG cObjects, IUnknown **ppUnk) PURE; \
	STDMETHOD(Show)(THIS_ UINT nCmdShow) PURE; \
	STDMETHOD(Move)(THIS_ LPCRECT pRect) PURE; \
	STDMETHOD(IsPageDirty)(THIS) PURE; \
	STDMETHOD(Apply)(THIS) PURE; \
	STDMETHOD(Help)(THIS_ LPCOLESTR pszHelpDir) PURE; \
	STDMETHOD(TranslateAccelerator)(THIS_ MSG *pMsg) PURE;
#define IPropertyPage_IMETHODS \
	IUnknown_IMETHODS \
	IPropertyPage_METHODS
ICOM_DEFINE(IPropertyPage,IUnknown)
#undef INTERFACE

/*** IUnknown methods ***/
#define IPropertyPage_QueryInterface(p,a,b)     ICOM_CALL2(QueryInterface,p,a,b)
#define IPropertyPage_AddRef(p)                 ICOM_CALL (AddRef,p)
#define IPropertyPage_Release(p)                ICOM_CALL (Release,p)
/*** IPropertyPage methods ***/
#define IPropertyPage_SetPageSite(p,a)          ICOM_CALL1(SetPageSite,p,a)
#define IPropertyPage_Activate(p,a,b,c)         ICOM_CALL3(Activate,p,a,b,c)
#define IPropertyPage_Deactivate(p)             ICOM_CALL (Deactivate,p)
#define IPropertyPage_GetPageInfo(p,a)          ICOM_CALL1(GetPageInfo,p,a)
#define IPropertyPage_SetObjects(p,a,b)         ICOM_CALL2(SetObjects,p,a,b)
#define IPropertyPage_Show(p,a)                 ICOM_CALL1(Show,p,a)
#define IPropertyPage_Move(p,a)                 ICOM_CALL1(Move,p,a)
#define IPropertyPage_IsPageDirty(p)            ICOM_CALL (IsPageDirty,p)
#define IPropertyPage_Apply(p)                  ICOM_CALL (Apply,p)
#define IPropertyPage_Help(p,a)                 ICOM_CALL1(Help,p,a)
#define IPropertyPage_TranslateAccelerator(p,a) ICOM_CALL1(TranslateAccelerator,p,a)


/*****************************************************************************
 * IPropertPage2 interface
 */
#define INTERFACE IPropertyPage2
#define IPropertyPage2_METHODS \
	STDMETHOD(EditProperty)(THIS_ DISPID dispID) PURE;
#define IPropertyPage2_IMETHODS \
	IPropertyPage_IMETHODS \
	IPropertyPage2_METHODS
ICOM_DEFINE(IPropertyPage2,IPropertyPage)
#undef INTERFACE

/*** IUnknown methods ***/
#define IPropertyPage2_QueryInterface(p,a,b)     ICOM_CALL2(QueryInterface,p,a,b)
#define IPropertyPage2_AddRef(p)                 ICOM_CALL (AddRef,p)
#define IPropertyPage2_Release(p)                ICOM_CALL (Release,p)
/*** IPropertyPage methods ***/
#define IPropertyPage2_SetPageSite(p,a)          ICOM_CALL1(SetPageSite,p,a)
#define IPropertyPage2_Activate(p,a,b,c)         ICOM_CALL3(Activate,p,a,b,c)
#define IPropertyPage2_Deactivate(p)             ICOM_CALL (Deactivate,p)
#define IPropertyPage2_GetPageInfo(p,a)          ICOM_CALL1(GetPageInfo,p,a)
#define IPropertyPage2_SetObjects(p,a,b)         ICOM_CALL2(SetObjects,p,a,b)
#define IPropertyPage2_Show(p,a)                 ICOM_CALL1(Show,p,a)
#define IPropertyPage2_Move(p,a)                 ICOM_CALL1(Move,p,a)
#define IPropertyPage2_IsPageDirty(p)            ICOM_CALL (IsPageDirty,p)
#define IPropertyPage2_Apply(p)                  ICOM_CALL (Apply,p)
#define IPropertyPage2_Help(p,a)                 ICOM_CALL1(Help,p,a)
#define IPropertyPage2_TranslateAccelerator(p,a) ICOM_CALL1(TranslateAccelerator,p,a)
/*** IPropertyPage2 methods ***/
#define IPropertyPage2_EditProperty(p,a)         ICOM_CALL1(EditProperty,p,a)


/*****************************************************************************
 * IPropertPageSite interface
 */
#define INTERFACE IPropertyPageSite
#define IPropertyPageSite_METHODS \
	STDMETHOD(OnStatusChange)(THIS_ DWORD dwFlags) PURE; \
	STDMETHOD(GetLocaleID)(THIS_ LCID *pLocaleID) PURE; \
	STDMETHOD(GetPageContainer)(THIS_ IUnknown **ppUnk) PURE; \
	STDMETHOD(TranslateAccelerator)(THIS_ MSG *pMsg) PURE;
#define IPropertyPageSite_IMETHODS \
	IUnknown_IMETHODS \
	IPropertyPageSite_METHODS
ICOM_DEFINE(IPropertyPageSite,IUnknown)
#undef INTERFACE

/*** IUnknown methods ***/
#define IPropertyPageSite_QueryInterface(p,a,b)     ICOM_CALL2(QueryInterface,p,a,b)
#define IPropertyPageSite_AddRef(p)                 ICOM_CALL (AddRef,p)
#define IPropertyPageSite_Release(p)                ICOM_CALL (Release,p)
/*** IPropertyPageSite methods ***/
#define IPropertyPageSite_OnStatusChange(p,a)       ICOM_CALL1(OnStatusChange,p,a)
#define IPropertyPageSite_GetLocaleID(p,a)          ICOM_CALL1(GetLocaleID,p,a)
#define IPropertyPageSite_GetPageContainer(p,a)     ICOM_CALL1(GetPageContainer,p,a)
#define IPropertyPageSite_TranslateAccelerator(p,a) ICOM_CALL1(TranslateAccelerator,p,a)


/*****************************************************************************
 * IPropertyNotifySink interface
 */
#define INTERFACE IPropertyNotifySink
#define IPropertyNotifySink_METHODS \
	STDMETHOD(OnChanged)(THIS_ DISPID dispID) PURE; \
	STDMETHOD(OnRequestEdit)(THIS_ DISPID dispID) PURE;
#define IPropertyNotifySink_IMETHODS \
	IUnknown_IMETHODS \
	IPropertyNotifySink_METHODS
ICOM_DEFINE(IPropertyNotifySink,IUnknown)
#undef INTERFACE

/*** IUnknown methods ***/
#define IPropertyNotifySink_QueryInterface(p,a,b)     ICOM_CALL2(QueryInterface,p,a,b)
#define IPropertyNotifySink_AddRef(p)                 ICOM_CALL (AddRef,p)
#define IPropertyNotifySink_Release(p)                ICOM_CALL (Release,p)
/*** IPropertyNotifySink methods ***/
#define IPropertyNotifySink_OnChanged(p,a)            ICOM_CALL1(OnChanged,p,a)
#define IPropertyNotifySink_OnRequestEdit(p,a)        ICOM_CALL1(OnRequestEdit,p,a)


/*****************************************************************************
 * IPropertyNotifySink interface
 */
#define INTERFACE ISimpleFrameSite
#define ISimpleFrameSite_METHODS \
	STDMETHOD(PreMessageFilter)(THIS_ HWND hWnd, UINT msg, WPARAM wp, LPARAM lp, LRESULT *plResult, DWORD *pwdCookie) PURE; \
	STDMETHOD(PostMessageFilter)(THIS_ HWND hWnd, UINT msg, WPARAM wp, LPARAM lp, LRESULT *plResult, DWORD pwdCookie) PURE;
#define ISimpleFrameSite_IMETHODS \
	IUnknown_IMETHODS \
	ISimpleFrameSite_METHODS
ICOM_DEFINE(ISimpleFrameSite,IUnknown)
#undef INTERFACE

/*** IUnknown methods ***/
#define ISimpleFrameSite_QueryInterface(p,a,b)     ICOM_CALL2(QueryInterface,p,a,b)
#define ISimpleFrameSite_AddRef(p)                 ICOM_CALL (AddRef,p)
#define ISimpleFrameSite_Release(p)                ICOM_CALL (Release,p)
/*** IPropertyNotifySink methods ***/
#define ISimpleFrameSite_PreMessageFilter(p,a,b,c,d,e,f) ICOM_CALL1(PreMessageFilter,p,a,b,c,d,e,f)
#define ISimpleFrameSite_PostMessageFilter(p,a,b,c,d,e,f) ICOM_CALL1(PostMessageFilter,p,a,b,c,d,e,f)


/*****************************************************************************
 * IPersistStreamInit interface
 */
#define INTERFACE IPersistStreamInit
#define IPersistStreamInit_METHODS \
	STDMETHOD(IsDirty)(THIS) PURE; \
	STDMETHOD(Load)(THIS_ LPSTREAM pStm) PURE; \
	STDMETHOD(Save)(THIS_ LPSTREAM pStm, BOOL fClearDirty) PURE; \
	STDMETHOD(GetSizeMax)(THIS_ ULARGE_INTEGER *pcbSize) PURE; \
	STDMETHOD(InitNew)(THIS) PURE;
#define IPersistStreamInit_IMETHODS \
	IPersist_IMETHODS \
	IPersistStreamInit_METHODS
ICOM_DEFINE(IPersistStreamInit,IPersist)
#undef INTERFACE

/*** IUnknown methods ***/
#define IPersistStreamInit_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IPersistStreamInit_AddRef(p)             ICOM_CALL (AddRef,p)
#define IPersistStreamInit_Release(p)            ICOM_CALL (Release,p)
/*** IPersist methods ***/
#define IPersistStreamInit_GetClassID(p,a) ICOM_CALL1(GetClassID,p,a)
/*** IPersistStreamInit methods ***/
#define IPersistStreamInit_IsDirty(p)      ICOM_CALL (IsDirty,p)
#define IPersistStreamInit_Load(p,a)       ICOM_CALL1(Load,p,a)
#define IPersistStreamInit_Save(p,a,b)     ICOM_CALL2(Save,p,a,b)
#define IPersistStreamInit_GetSizeMax(p,a) ICOM_CALL1(GetSizeMax,p,a)
#define IPersistStreamInit_InitNew(p)      ICOM_CALL (InitNew,p)


/*****************************************************************************
 * IPersistMemory interface
 */
#define INTERFACE IPersistMemory
#define IPersistMemory_METHODS \
	STDMETHOD(IsDirty)(THIS) PURE; \
	STDMETHOD(Load)(THIS_ LPVOID pMem, ULONG cbSize) PURE; \
	STDMETHOD(Save)(THIS_ LPVOID pMem, BOOL fClearDirty, ULONG cbSize) PURE; \
	STDMETHOD(GetSizeMax)(THIS_ ULONG *pCbSize) PURE; \
	STDMETHOD(InitNew)(THIS) PURE;
#define IPersistMemory_IMETHODS \
	IPersist_IMETHODS \
	IPersistMemory_METHODS
ICOM_DEFINE(IPersistMemory,IPersist)
#undef INTERFACE

/*** IUnknown methods ***/
#define IPersistMemory_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IPersistMemory_AddRef(p)             ICOM_CALL (AddRef,p)
#define IPersistMemory_Release(p)            ICOM_CALL (Release,p)
/*** IPersist methods ***/
#define IPersistMemory_GetClassID(p,a)       ICOM_CALL1(GetClassID,p,a)
/*** IPersistMemory methods ***/
#define IPersistMemory_IsDirty(p)            ICOM_CALL (IsDirty,p)
#define IPersistMemory_Load(p,a,b)           ICOM_CALL2(Load,p,a,b)
#define IPersistMemory_Save(p,a,b,c)         ICOM_CALL3(Save,p,a,b,c)
#define IPersistMemory_GetSizeMax(p,a)       ICOM_CALL1(GetSizeMax,p,a)
#define IPersistMemory_InitNew(p)            ICOM_CALL (InitNew,p)


/*****************************************************************************
 * IPersistPropertyBag interface
 */
#define INTERFACE IPersistPropertyBag
#define IPersistPropertyBag_METHODS \
	STDMETHOD(InitNew)(THIS) PURE; \
	STDMETHOD(Load)(THIS_ IPropertyBag *pPropBag, IErrorLog *pErrorLog) PURE; \
	STDMETHOD(Save)(THIS_ IPropertyBag *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties) PURE;
#define IPersistPropertyBag_IMETHODS \
	IPersist_IMETHODS \
	IPersistPropertyBag_METHODS
ICOM_DEFINE(IPersistPropertyBag,IPersist)
#undef INTERFACE

/*** IUnknown methods ***/
#define IPersistPropertyBag_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IPersistPropertyBag_AddRef(p)             ICOM_CALL (AddRef,p)
#define IPersistPropertyBag_Release(p)            ICOM_CALL (Release,p)
/*** IPersist methods ***/
#define IPersistPropertyBag_GetClassID(p,a)       ICOM_CALL1(GetClassID,p,a)
/*** IPersistPropertyBag methods ***/
#define IPersistPropertyBag_InitNew(p)            ICOM_CALL (InitNew,p)
#define IPersistPropertyBag_Load(p,a,b)           ICOM_CALL2(Load,p,a,b)
#define IPersistPropertyBag_Save(p,a,b,c)         ICOM_CALL3(Save,p,a,b,c)


/*****************************************************************************
 * IPersistPropertyBag2 interface
 */
#define INTERFACE IPersistPropertyBag2
#define IPersistPropertyBag2_METHODS \
	STDMETHOD(InitNew)(THIS) PURE; \
	STDMETHOD(Load)(THIS_ IPropertyBag2 *pPropBag, IErrorLog *pErrorLog) PURE; \
	STDMETHOD(Save)(THIS_ IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties) PURE; \
    STDMETHOD(IsDirty)(THIS) PURE;
#define IPersistPropertyBag2_IMETHODS \
	IPersist_IMETHODS \
	IPersistPropertyBag2_METHODS
ICOM_DEFINE(IPersistPropertyBag2,IPersist)
#undef INTERFACE

/*** IUnknown methods ***/
#define IPersistPropertyBag2_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IPersistPropertyBag2_AddRef(p)             ICOM_CALL (AddRef,p)
#define IPersistPropertyBag2_Release(p)            ICOM_CALL (Release,p)
/*** IPersist methods ***/
#define IPersistPropertyBag2_GetClassID(p,a)       ICOM_CALL1(GetClassID,p,a)
/*** IPersistPropertyBag methods ***/
#define IPersistPropertyBag2_InitNew(p)            ICOM_CALL (InitNew,p)
#define IPersistPropertyBag2_Load(p,a,b)           ICOM_CALL2(Load,p,a,b)
#define IPersistPropertyBag2_Save(p,a,b,c)         ICOM_CALL3(Save,p,a,b,c)
#define IPersistPropertyBag2_IsDirty(p)            ICON_CALL (IsDirty,p)


/*****************************************************************************
 * IErrorLog interface
 */
#define INTERFACE IErrorLog
#define IErrorLog_METHODS \
	STDMETHOD(AddError)(THIS_ LPCOLESTR pszPropName, EXCEPINFO *pExcepInfo) PURE;
#define IErrorLog_IMETHODS \
	IUnknown_IMETHODS \
	IErrorLog_METHODS
ICOM_DEFINE(IErrorLog,IUnknown)
#undef INTERFACE

/*** IUnknown methods ***/
#define IErrorLog_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IErrorLog_AddRef(p)             ICOM_CALL (AddRef,p)
#define IErrorLog_Release(p)            ICOM_CALL (Release,p)
/*** IErrorLog methods ***/
#define IErrorLog_AddError(p,a,b)       ICOM_CALL2(GetClassID,p,a,b)


/*****************************************************************************
 * IPropertyBag interface
 */
#define INTERFACE IPropertyBag
#define IPropertyBag_METHODS \
	STDMETHOD(Read)(THIS_ LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog) PURE; \
	STDMETHOD(Write)(THIS_ LPCOLESTR pszPropName, VARIANT *pVar) PURE;
#define IPropertyBag_IMETHODS \
	IUnknown_IMETHODS \
	IPropertyBag_METHODS
ICOM_DEFINE(IPropertyBag,IUnknown)
#undef INTERFACE

/*** IUnknown methods ***/
#define IPropertyBag_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IPropertyBag_AddRef(p)             ICOM_CALL (AddRef,p)
#define IPropertyBag_Release(p)            ICOM_CALL (Release,p)
/*** IPropertyBag methods ***/
#define IPropertyBag_Read(p,a,b,c)         ICOM_CALL3(Read,p,a,b,c)
#define IPropertyBag_Write(p,a,b)          ICOM_CALL2(Write,p,a,b)


/*****************************************************************************
 * IPropertyBag2 interface
 */
#define INTERFACE IPropertyBag2
#define IPropertyBag2_METHODS \
    STDMETHOD(Read)(THIS_ ULONG cProperties, PROPBAG2 *pPropBag, IErrorLog *pErrLog, VARIANT *pvarValue, HRESULT *phrError) PURE; \
    STDMETHOD(Write)(THIS_ ULONG cProperties, PROPBAG2 *pPropBag, VARIANT *pvarValue) PURE; \
    STDMETHOD(CountProperties)(THIS_ ULONG *pcProperties) PURE; \
    STDMETHOD(GetPropertyInfo)(THIS_ ULONG iProperty, ULONG cProperties, PROPBAG2 *pPropBag, ULONG *pcProperties) PURE; \
    STDMETHOD(LoadObject)(THIS_ LPCOLESTR pstrName, DWORD dwHint, IUnknown *pUnkObject, IErrorLog *pErrLog) PURE;
#define IPropertyBag2_IMETHODS \
	IUnknown_IMETHODS \
	IPropertyBag2_METHODS
ICOM_DEFINE(IPropertyBag2,IUnknown)
#undef INTERFACE

/*** IUnknown methods ***/
#define IPropertyBag2_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IPropertyBag2_AddRef(p)             ICOM_CALL (AddRef,p)
#define IPropertyBag2_Release(p)            ICOM_CALL (Release,p)
/*** IPropertyBag methods ***/
#define IPropertyBag2_Read(p,a,b,c,d,e)     ICOM_CALL5(Read,p,a,b,c,d,e)
#define IPropertyBag2_Write(p,a,b,c)        ICOM_CALL3(Write,p,a,b,c)
#define IPropertyBag2_CountProperties(p,a)  ICOM_CALL1(CountProperties,p,a)
#define IPropertyBag2_GetPropertyInfo(p,a,b,c,d) ICOM_CALL4(GetPropertyInfo,p,a,b,c,d)
#define IPropertyBag2_LoadObject(p,a,b,c,d) ICOM_CALL4(LoadObject,p,a,b,c,d)


/*****************************************************************************
 * ISpecifyPropertyPages interface
 */
#define INTERFACE ISpecifyPropertyPages
#define ISpecifyPropertyPages_METHODS \
	STDMETHOD(GetPages)(THIS_ CAUUID *pPages) PURE;
#define ISpecifyPropertyPages_IMETHODS \
	IUnknown_IMETHODS \
	ISpecifyPropertyPages_METHODS
ICOM_DEFINE(ISpecifyPropertyPages,IUnknown)
#undef INTERFACE

/*** IUnknown methods ***/
#define ISpecifyPropertyPages_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define ISpecifyPropertyPages_AddRef(p)             ICOM_CALL (AddRef,p)
#define ISpecifyPropertyPages_Release(p)            ICOM_CALL (Release,p)
/*** ISpecifyPropertyPages methods ***/
#define ISpecifyPropertyPages_GetPages(p,a)         ICOM_CALL1(GetPages,p,a)


/*****************************************************************************
 * IPerPropertyBrowsing interface
 */
#define INTERFACE IPerPropertyBrowsing
#define IPerPropertyBrowsing_METHODS \
	STDMETHOD(GetDisplayString)(THIS_ DISPID dispID, BSTR *pBstr) PURE; \
	STDMETHOD(MapPropertyToPage)(THIS_ DISPID dispID, CLSID *pClsid) PURE; \
	STDMETHOD(GetPredefinedStrings)(THIS_ DISPID dispID, CALPOLESTR *pCaStringsOut, CADWORD *pCaCookiesOut) PURE; \
	STDMETHOD(GetPredefinedValue)(THIS_ DISPID dispID, DWORD dwCookie, VARIANT *pVarOut) PURE;
#define IPerPropertyBrowsing_IMETHODS \
	IUnknown_IMETHODS \
	IPerPropertyBrowsing_METHODS
ICOM_DEFINE(IPerPropertyBrowsing,IUnknown)
#undef INTERFACE

/*** IUnknown methods ***/
#define IPerPropertyBrowsing_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IPerPropertyBrowsing_AddRef(p)             ICOM_CALL (AddRef,p)
#define IPerPropertyBrowsing_Release(p)            ICOM_CALL (Release,p)
/*** IPerPropertyBrowsing methods ***/
#define IPerPropertyBrowsing_GetDisplayString(p,a,b)       ICOM_CALL2(GetDisplayString,p,a,b)
#define IPerPropertyBrowsing_MapPropertyToPage(p,a,b)      ICOM_CALL2(MapPropertyToPage,p,a,b)
#define IPerPropertyBrowsing_GetPredefinedStrings(p,a,b,c) ICOM_CALL3(GetPredefinedStrings,p,a,b,c)
#define IPerPropertyBrowsing_GetPredefinedValue(p,a,b,c)   ICOM_CALL3(GetPredefinedValue,p,a,b,c)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_PROPERTY_H */
