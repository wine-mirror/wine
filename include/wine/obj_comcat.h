/*
 * Defines the COM interfaces and APIs related to Component Category Manager
 *
 * Depends on 'obj_enumguid.h'.
 *
 * Copyright (C) 2002 John K. Hohm
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

#ifndef __WINE_WINE_OBJ_COMCAT_H
#define __WINE_WINE_OBJ_COMCAT_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Types
 */
typedef GUID CATID;
typedef REFGUID REFCATID;
#define CATID_NULL GUID_NULL
#define IsEqualCATID(a, b) IsEqualGUID(a, b)

typedef struct tagCATEGORYINFO {
    CATID   catid;		/* category identifier for component */
    LCID    lcid;		/* locale identifier */
    OLECHAR szDescription[128];	/* description of the category */
} CATEGORYINFO, *LPCATEGORYINFO;

/*****************************************************************************
 * Category IDs
 */
DEFINE_GUID(CATID_Insertable,			  0x40FC6ED3, 0x2438, 0x11CF, 0xA3, 0xDB, 0x08, 0x00, 0x36, 0xF1, 0x25, 0x02);
DEFINE_GUID(CATID_Control,			  0x40FC6ED4, 0x2438, 0x11CF, 0xA3, 0xDB, 0x08, 0x00, 0x36, 0xF1, 0x25, 0x02);
DEFINE_GUID(CATID_Programmable,			  0x40FC6ED5, 0x2438, 0x11CF, 0xA3, 0xDB, 0x08, 0x00, 0x36, 0xF1, 0x25, 0x02);
DEFINE_GUID(CATID_IsShortcut,			  0x40FC6ED6, 0x2438, 0x11CF, 0xA3, 0xDB, 0x08, 0x00, 0x36, 0xF1, 0x25, 0x02);
DEFINE_GUID(CATID_NeverShowExt,			  0x40FC6ED7, 0x2438, 0x11CF, 0xA3, 0xDB, 0x08, 0x00, 0x36, 0xF1, 0x25, 0x02);
DEFINE_GUID(CATID_DocObject,			  0x40FC6ED8, 0x2438, 0x11CF, 0xA3, 0xDB, 0x08, 0x00, 0x36, 0xF1, 0x25, 0x02);
DEFINE_GUID(CATID_Printable,			  0x40FC6ED9, 0x2438, 0x11CF, 0xA3, 0xDB, 0x08, 0x00, 0x36, 0xF1, 0x25, 0x02);
DEFINE_GUID(CATID_RequiresDataPathHost,		  0x0DE86A50, 0x2BAA, 0x11CF, 0xA2, 0x29, 0x00, 0xAA, 0x00, 0x3D, 0x73, 0x52);
DEFINE_GUID(CATID_PersistsToMoniker,		  0x0DE86A51, 0x2BAA, 0x11CF, 0xA2, 0x29, 0x00, 0xAA, 0x00, 0x3D, 0x73, 0x52);
DEFINE_GUID(CATID_PersistsToStorage,		  0x0DE86A52, 0x2BAA, 0x11CF, 0xA2, 0x29, 0x00, 0xAA, 0x00, 0x3D, 0x73, 0x52);
DEFINE_GUID(CATID_PersistsToStreamInit,		  0x0DE86A53, 0x2BAA, 0x11CF, 0xA2, 0x29, 0x00, 0xAA, 0x00, 0x3D, 0x73, 0x52);
DEFINE_GUID(CATID_PersistsToStream,		  0x0DE86A54, 0x2BAA, 0x11CF, 0xA2, 0x29, 0x00, 0xAA, 0x00, 0x3D, 0x73, 0x52);
DEFINE_GUID(CATID_PersistsToMemory,		  0x0DE86A55, 0x2BAA, 0x11CF, 0xA2, 0x29, 0x00, 0xAA, 0x00, 0x3D, 0x73, 0x52);
DEFINE_GUID(CATID_PersistsToFile,		  0x0DE86A56, 0x2BAA, 0x11CF, 0xA2, 0x29, 0x00, 0xAA, 0x00, 0x3D, 0x73, 0x52);
DEFINE_GUID(CATID_PersistsToPropertyBag,	  0x0DE86A57, 0x2BAA, 0x11CF, 0xA2, 0x29, 0x00, 0xAA, 0x00, 0x3D, 0x73, 0x52);
DEFINE_GUID(CATID_InternetAware,		  0x0DE86A58, 0x2BAA, 0x11CF, 0xA2, 0x29, 0x00, 0xAA, 0x00, 0x3D, 0x73, 0x52);
DEFINE_GUID(CATID_DesignTimeUIActivatableControl, 0xF2BB56D1, 0xDB07, 0x11D1, 0xAA, 0x6B, 0x00, 0x60, 0x97, 0xDB, 0x95, 0x39);

/*****************************************************************************
 * Aliases for EnumGUID
 */
#define IEnumCATID IEnumGUID
#define LPENUMCATID LPENUMGUID
#define IID_IEnumCATID IID_IEnumGUID

#define IEnumCLSID IEnumGUID
#define LPENUMCLSID LPENUMGUID
#define IID_IEnumCLSID IID_IEnumGUID

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_OLEGUID(IID_ICatInformation, 0x0002E013L, 0, 0);
typedef struct ICatInformation ICatInformation, *LPCATINFORMATION;

DEFINE_OLEGUID(IID_ICatRegister, 0x0002E012L, 0, 0);
typedef struct ICatRegister ICatRegister, *LPCATREGISTER;

DEFINE_OLEGUID(IID_IEnumCATEGORYINFO, 0x0002E011L, 0, 0);
typedef struct IEnumCATEGORYINFO IEnumCATEGORYINFO, *LPENUMCATEGORYINFO;

/* The Component Category Manager */
DEFINE_OLEGUID(CLSID_StdComponentCategoriesMgr, 0x0002E005L, 0, 0);

/*****************************************************************************
 * ICatInformation
 */
#define INTERFACE ICatInformation
#define ICatInformation_METHODS \
    STDMETHOD(EnumCategories)(THIS_ LCID  lcid, IEnumCATEGORYINFO ** ppenumCatInfo) PURE; \
    STDMETHOD(GetCategoryDesc)(THIS_ REFCATID  rcatid, LCID  lcid, PWCHAR * ppszDesc) PURE; \
    STDMETHOD(EnumClassesOfCategories)(THIS_ ULONG  cImplemented, CATID * rgcatidImpl, ULONG  cRequired, CATID * rgcatidReq, IEnumCLSID ** ppenumCLSID) PURE; \
    STDMETHOD(IsClassOfCategories)(THIS_ REFCLSID  rclsid, ULONG  cImplemented, CATID * rgcatidImpl, ULONG  cRequired, CATID * rgcatidReq) PURE; \
    STDMETHOD(EnumImplCategoriesOfClass)(THIS_ REFCLSID  rclsid, IEnumCATID ** ppenumCATID) PURE; \
    STDMETHOD(EnumReqCategoriesOfClass)(THIS_ REFCLSID  rclsid, IEnumCATID ** ppenumCATID) PURE;
#define ICatInformation_IMETHODS \
    IUnknown_IMETHODS \
    ICatInformation_METHODS
ICOM_DEFINE(ICatInformation,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define ICatInformation_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define ICatInformation_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define ICatInformation_Release(p)            (p)->lpVtbl->Release(p)
/*** ICatInformation methods ***/
#define ICatInformation_EnumCategories(p,a,b) (p)->lpVtbl->EnumCategories(p,a,b)
#define ICatInformation_GetCategoryDesc(p,a,b,c) (p)->lpVtbl->GetCategoryDesc(p,a,b,c)
#define ICatInformation_EnumClassesOfCategories(p,a,b,c,d,e) (p)->lpVtbl->EnumClassesOfCategories(p,a,b,c,d,e)
#define ICatInformation_IsClassOfCategories(p,a,b,c,d,e) (p)->lpVtbl->IsClassOfCategories(p,a,b,c,d,e)
#define ICatInformation_EnumImplCategoriesOfClass(p,a,b) (p)->lpVtbl->EnumImplCategoriesOfClass(p,a,b)
#define ICatInformation_EnumReqCategoriesOfClass(p,a,b) (p)->lpVtbl->EnumReqCategoriesOfClass(p,a,b)
#endif

/*****************************************************************************
 * ICatRegister
 */
#define INTERFACE ICatRegister
#define ICatRegister_METHODS \
    STDMETHOD(RegisterCategories)(THIS_ ULONG  cCategories, CATEGORYINFO * rgCategoryInfo) PURE; \
    STDMETHOD(UnRegisterCategories)(THIS_ ULONG  cCategories, CATID * rgcatid) PURE; \
    STDMETHOD(RegisterClassImplCategories)(THIS_ REFCLSID  rclsid, ULONG  cCategories, CATID * rgcatid) PURE; \
    STDMETHOD(UnRegisterClassImplCategories)(THIS_ REFCLSID  rclsid, ULONG  cCategories, CATID * rgcatid) PURE; \
    STDMETHOD(RegisterClassReqCategories)(THIS_ REFCLSID  rclsid, ULONG  cCategories, CATID * rgcatid) PURE; \
    STDMETHOD(UnRegisterClassReqCategories)(THIS_ REFCLSID  rclsid, ULONG  cCategories, CATID * rgcatid) PURE;
#define ICatRegister_IMETHODS \
    IUnknown_IMETHODS \
    ICatRegister_METHODS
ICOM_DEFINE(ICatRegister,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define ICatRegister_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define ICatRegister_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define ICatRegister_Release(p)            (p)->lpVtbl->Release(p)
/*** ICatRegister methods ***/
#define ICatRegister_RegisterCategories(p,a,b) (p)->lpVtbl->RegisterCategories(p,a,b)
#define ICatRegister_UnRegisterCategories(p,a,b) (p)->lpVtbl->UnRegisterCategories(p,a,b)
#define ICatRegister_RegisterClassImplCategories(p,a,b,c) (p)->lpVtbl->RegisterClassImplCategories(p,a,b,c)
#define ICatRegister_UnRegisterClassImplCategories(p,a,b,c) (p)->lpVtbl->UnRegisterClassImplCategories(p,a,b,c)
#define ICatRegister_RegisterClassReqCategories(p,a,b,c) (p)->lpVtbl->RegisterClassReqCategories(p,a,b,c)
#define ICatRegister_UnRegisterClassReqCategories(p,a,b,c) (p)->lpVtbl->UnRegisterClassReqCategories(p,a,b,c)
#endif

/*****************************************************************************
 * IEnumCATEGORYINFO
 */
#define INTERFACE IEnumCATEGORYINFO
#define IEnumCATEGORYINFO_METHODS \
    STDMETHOD(Next)(THIS_ ULONG  celt, CATEGORYINFO * rgelt, ULONG * pceltFetched) PURE; \
    STDMETHOD(Skip)(THIS_ ULONG  celt) PURE; \
    STDMETHOD(Reset)(THIS) PURE; \
    STDMETHOD(Clone)(THIS_ IEnumCATEGORYINFO ** ppenum) PURE;
#define IEnumCATEGORYINFO_IMETHODS \
    IUnknown_IMETHODS \
    IEnumCATEGORYINFO_METHODS
ICOM_DEFINE(IEnumCATEGORYINFO,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IEnumCATEGORYINFO_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IEnumCATEGORYINFO_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define IEnumCATEGORYINFO_Release(p)            (p)->lpVtbl->Release(p)
/*** IEnumCATEGORYINFO methods ***/
#define IEnumCATEGORYINFO_Next(p,a,b,c)         (p)->lpVtbl->Next(p,a,b,c)
#define IEnumCATEGORYINFO_Skip(p,a)             (p)->lpVtbl->Skip(p,a)
#define IEnumCATEGORYINFO_Reset(p)              (p)->lpVtbl->Reset(p)
#define IEnumCATEGORYINFO_Clone(p,a)            (p)->lpVtbl->Clone(p,a)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_COMCAT_H */
