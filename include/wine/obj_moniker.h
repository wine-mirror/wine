/*
 * Defines the COM interfaces and APIs related to the moniker functionality.
 *
 * This file depends on 'obj_storage.h' and 'obj_base.h'.
 */


#include "wine/obj_misc.h"

#ifndef __WINE_WINE_OBJ_MONIKER_H
#define __WINE_WINE_OBJ_MONIKER_H

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_OLEGUID(IID_IBindCtx,0xe,0,0);
typedef struct IBindCtx IBindCtx,*LPBINDCTX;
typedef LPBINDCTX LPBC;

DEFINE_OLEGUID(IID_IClassActivator,	0x00000140L, 0, 0);
typedef struct IClassActivator IClassActivator,*LPCLASSACTIVATOR;

DEFINE_OLEGUID(IID_IEnumMoniker,	0x00000102L, 0, 0);
typedef struct IEnumMoniker IEnumMoniker,*LPENUMMONIKER;

DEFINE_OLEGUID(IID_IMoniker,		0x0000000fL, 0, 0);
typedef struct IMoniker IMoniker,*LPMONIKER;

DEFINE_GUID   (IID_IROTData,		0xf29f6bc0L, 0x5021, 0x11ce, 0xaa, 0x15, 0x00, 0x00, 0x69, 0x01, 0x29, 0x3f);
typedef struct IROTData IROTData,*LPROTDATA;

DEFINE_OLEGUID(IID_IRunnableObject,	0x00000126L, 0, 0);
typedef struct IRunnableObject IRunnableObject,*LPRUNNABLEOBJECT;

DEFINE_OLEGUID(IID_IRunningObjectTable,	0x00000010L, 0, 0);
typedef struct IRunningObjectTable IRunningObjectTable,*LPRUNNINGOBJECTTABLE;

typedef struct  COSERVERINFO COSERVERINFO; // must be defined !


/*********************************************************************************
 *	BIND_OPTS and BIND_OPTS2 structures definition
 *	Thes structures contain parameters used during a moniker-binding operation.
 *********************************************************************************/
typedef struct tagBIND_OPTS{

    DWORD cbStruct;
    DWORD grfFlags;
    DWORD grfMode;
    DWORD dwTickCountDeadline;
} BIND_OPTS, * LPBIND_OPTS;

typedef struct tagBIND_OPTS2{

    DWORD cbStruct;
    DWORD grfFlags;
    DWORD grfMode;
    DWORD dwTickCountDeadline;
    DWORD dwTrackFlags;
    DWORD dwClassContext;
    LCID  locale;
    COSERVERINFO* pServerInfo;
    
} BIND_OPTS2, * LPBIND_OPTS2;

/*****************************************************************************
 * IBindCtx interface
 */
#define ICOM_INTERFACE IBindCtx
ICOM_BEGIN(IBindCtx,IUnknown)
    ICOM_METHOD1 (HRESULT, RegisterObjectBound,IUnknown*,punk);
    ICOM_METHOD1 (HRESULT, RevokeObjectBound,IUnknown*,punk);
    ICOM_METHOD  (HRESULT, ReleaseObjects);
    ICOM_METHOD1 (HRESULT, SetBindOptions,LPBIND_OPTS2,pbindopts);
    ICOM_METHOD1 (HRESULT, GetBindOptions,LPBIND_OPTS2,pbindopts);
    ICOM_METHOD1 (HRESULT, GetRunningObjectTable,IRunningObjectTable**,pprot);
    ICOM_METHOD2 (HRESULT, RegisterObjectParam,LPOLESTR32,pszkey,IUnknown*,punk);
    ICOM_METHOD2 (HRESULT, GetObjectParam,LPOLESTR32,pszkey,IUnknown*,punk);
    ICOM_METHOD1 (HRESULT, EnumObjectParam,IEnumString**,ppenum);
    ICOM_METHOD1 (HRESULT, RevokeObjectParam,LPOLESTR32,pszkey);
ICOM_END(IBindCtx)

#undef ICOM_INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IBindCtx_QueryInterface(p,a,b) ICOM_ICALL2(IUnknown,QueryInterface,p,a,b)
#define IBindCtxr_AddRef(p)            ICOM_ICALL (IUnknown,AddRef,p)
#define IBindCtx_Release(p)            ICOM_ICALL (IUnknown,Release,p)
/* IBindCtx methods*/
#define IBindCtx_RegisterObjectBound(p,a)           ICOM_CALL1 (RegisterObjectBound,p,a);
#define IBindCtx_RevokeObjectBound(p,a)             ICOM_CALL1 (RevokeObjectBound,p,a);
#define IBindCtx_ReleaseObjects(p)                  ICOM_CALL  (ReleaseObjects,p);
#define IBindCtx_SetBindOptions(p,a)                ICOM_CALL1 (SetBindOptions,p,a);
#define IBindCtx_GetBindOptions(p,a)                ICOM_CALL1 (GetBindOptions,p,a);
#define IBindCtx_GetRunningObjectTable(p,a)         ICOM_CALL1 (GetRunningObjectTable,p,a);
#define IBindCtx_RegisterObjectParam(p,a,b)         ICOM_CALL2 (RegisterObjectParam,p,a,b);
#define IBindCtx_GetObjectParam(p,a,b)              ICOM_CALL2 (GetObjectParam,p,a,b);
#define IBindCtx_EnumObjectParam(p,a)               ICOM_CALL1 (EnumObjectParam,p,a);
#define IBindCtx_RevokeObjectParam(p,a)             ICOM_CALL1 (RevokeObjectParam,p,a);

#endif

#define CreateBindCtx WINELIB_NAME(CreateBindCtx)
/*****************************************************************************
 * IClassActivator interface
 */
/* FIXME: not implemented */

/*****************************************************************************
 * IEnumMoniker interface
 */
/* FIXME: not implemented */

/*****************************************************************************
 * IMoniker interface
 */
#define ICOM_INTERFACE IMoniker
ICOM_BEGIN(IMoniker,IPersistStream)
    ICOM_METHOD4 (HRESULT,BindToObject,IBindCtx*,pbc,IMoniker*,pmkToLeft,REFIID,riid,VOID**,ppvResult);
    ICOM_METHOD4 (HRESULT,BindToStorage,IBindCtx*,pbc,IMoniker*,pmkToLeft,REFIID,riid,VOID**,ppvResult);
    ICOM_METHOD4 (HRESULT,Reduce,IBindCtx*,pbc,DWORD,dwReduceHowFar,IMoniker**,ppmkToLeft,IMoniker**,ppmkReduced);
    ICOM_METHOD3 (HRESULT,ComposeWith,IMoniker*,pmkRight,BOOL32,fOnlyIfNotGeneric,IMoniker**,ppmkComposite);
    ICOM_METHOD2 (HRESULT,Enum,BOOL32,fForward,IEnumMoniker**,ppenumMoniker);
    ICOM_METHOD1 (HRESULT,IsEqual,IMoniker*, pmkOtherMoniker);
    ICOM_METHOD1 (HRESULT,Hash,DWORD*,pdwHash);
    ICOM_METHOD3 (HRESULT,IsRunning,IBindCtx*,pbc,IMoniker*,pmkToLeft,IMoniker*,pmkNewlyRunning);
    ICOM_METHOD3 (HRESULT,GetTimeOfLastChange,IBindCtx*,pbc,IMoniker*,pmkToLeft,FILETIME*,pFileTime);
    ICOM_METHOD1 (HRESULT,Inverse,IMoniker**,ppmk);
    ICOM_METHOD2 (HRESULT,CommonPrefixWith,IMoniker*,pmkOther,IMoniker**,ppmkPrefix);
    ICOM_METHOD2 (HRESULT,RelativePathTo,IMoniker*,pmOther,IMoniker**,ppmkRelPath);
    ICOM_METHOD3 (HRESULT,GetDisplayName,IBindCtx*,pbc,IMoniker*,pmkToLeft,LPOLESTR32,*ppszDisplayName);
    ICOM_METHOD5 (HRESULT,ParseDisplayName,IBindCtx*,pbc,IMoniker*,pmkToLeft,LPOLESTR32,pszDisplayName,ULONG*,pchEaten,IMoniker**,ppmkOut);
    ICOM_METHOD1 (HRESULT,IsSystemMoniker,DWORD*,pwdMksys);
ICOM_END(IMoniker)
#undef ICOM_INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IMoniker_QueryInterface(p,a,b) ICOM_ICALL2(IUnknown,QueryInterface,p,a,b)
#define IMoniker_AddRef(p)             ICOM_ICALL (IUnknown,AddRef,p)
#define IMoniker_Release(p)            ICOM_ICALL (IUnknown,Release,p)
/*** IPersist methods ***/
#define IMoniker_GetClassID(p,a)       ICOM_ICALL1(IPersist,GetClassID,p,a)
/*** IPersistStream ***/
#define IMoniker_Stream_IsDirty(p)      ICOM_ICALL(IPersistStream,IsDirty,p)
#define IMoniker_Stream_Load(p,a)       ICOM_ICALL1(IPersistStream,Load,p,a)
#define IMoniker_Stream_Save(p,a,b)     ICOM_ICALL2(IPersistStream,Save,p,a,b)
#define IMoniker_Stream_GetSizeMax(p,a) ICOM_ICALL1(IPersistStream,GetSizeMax,p,a)
/* IMonker methods*/
#define IMoniker_BindToObject(p,a,b,c,d)            ICOM_CALL4(BindToObject,p,a,b,c,d)
#define IMoniker_BindToStorage(p,a,b,c,d)           ICOM_CALL4(BindToStorage,p,a,b,c,d)
#define IMoniker_Reduce(p,a,b,c,d)                  ICOM_CALL4(Reduce,p,a,b,c,d)
#define IMoniker_ComposeWith(p,a,b,c)               ICOM_CALL3(ComposeWith,p,a,b,c)
#define IMoniker_Enum(p,a,b)                        ICOM_CALL2(Enum,p,a,b)
#define IMoniker_IsEqual(p,a)                       ICOM_CALL1(IsEqual,p,a)
#define IMoniker_Hash(p,a)                          ICOM_CALL1(Hash,p,a)
#define IMoniker_IsRunning(p,a,b,c)                 ICOM_CALL3(IsRunning,p,a,b,c)
#define IMoniker_GetTimeOfLastChange(p,a,b,c)       ICOM_CALL3(GetTimeOfLastChange,p,a,b,c)
#define IMoniker_Inverse(p,a)                       ICOM_CALL1(Inverse,p,a)
#define IMoniker_CommonPrefixWith(p,a,b)            ICOM_CALL2(CommonPrefixWith,p,a,b)
#define IMoniker_RelativePathTo(p,a,b)              ICOM_CALL2(RelativePathTo,p,a,b)
#define IMoniker_GetDisplayName(p,a,b,c)            ICOM_CALL3(GetDisplayName,p,a,b,c)
#define IMoniker_ParseDisplayName(p,a,b,c,d,e)      ICOM_CALL5(ParseDisplayName,p,a,b,c,d,e)
#define IMoniker_IsSystemMoniker(p,a)               ICOM_CALL1(IsSystemMonker,p,a)
#endif

#define CreateFileMoniker WINELIB_NAME(CreateFileMoniker)
#define CreateItemMoniker WINELIB_NAME(CreateItemMoniker)


/*****************************************************************************
 * IROTData interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * IRunnableObject interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * IRunningObjectTable interface
 */
/* FIXME: not implemented */


#endif /* __WINE_WINE_OBJ_MONIKER_H */
