/*
 * Copyright (C) 1998 François Gouget
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

#include "rpc.h"
#include "rpcndr.h"

#ifndef _OBJBASE_H_
#define _OBJBASE_H_

#define __WINE_INCLUDE_OBJIDL
#include "objidl.h"
#undef __WINE_INCLUDE_OBJIDL

#ifndef RC_INVOKED
/* For compatibility only, at least for now */
#include <stdlib.h>
#endif

#ifndef INITGUID
#include "cguid.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NONAMELESSSTRUCT
#define LISet32(li, v)   ((li).HighPart = (v) < 0 ? -1 : 0, (li).LowPart = (v))
#define ULISet32(li, v)  ((li).HighPart = 0, (li).LowPart = (v))
#else
#define LISet32(li, v)   ((li).s.HighPart = (v) < 0 ? -1 : 0, (li).s.LowPart = (v))
#define ULISet32(li, v)  ((li).s.HighPart = 0, (li).s.LowPart = (v))
#endif

/*****************************************************************************
 *	Standard API
 */
DWORD WINAPI CoBuildVersion(void);

typedef enum tagCOINIT
{
    COINIT_APARTMENTTHREADED  = 0x2, /* Apartment model */
    COINIT_MULTITHREADED      = 0x0, /* OLE calls objects on any thread */
    COINIT_DISABLE_OLE1DDE    = 0x4, /* Don't use DDE for Ole1 support */
    COINIT_SPEED_OVER_MEMORY  = 0x8  /* Trade memory for speed */
} COINIT;

HRESULT WINAPI CoInitialize(LPVOID lpReserved);
HRESULT WINAPI CoInitializeEx(LPVOID lpReserved, DWORD dwCoInit);
void WINAPI CoUninitialize(void);
DWORD WINAPI CoGetCurrentProcess(void);

HINSTANCE WINAPI CoLoadLibrary(LPOLESTR lpszLibName, BOOL bAutoFree);
void WINAPI CoFreeAllLibraries(void);
void WINAPI CoFreeLibrary(HINSTANCE hLibrary);
void WINAPI CoFreeUnusedLibraries(void);

HRESULT WINAPI CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID iid, LPVOID *ppv);
HRESULT WINAPI CoCreateInstanceEx(REFCLSID      rclsid,
				  LPUNKNOWN     pUnkOuter,
				  DWORD         dwClsContext,
				  COSERVERINFO* pServerInfo,
				  ULONG         cmq,
				  MULTI_QI*     pResults);

HRESULT WINAPI CoGetInstanceFromFile(COSERVERINFO* pServerInfo, CLSID* pClsid, IUnknown* punkOuter, DWORD dwClsCtx, DWORD grfMode, OLECHAR* pwszName, DWORD dwCount, MULTI_QI* pResults);
HRESULT WINAPI CoGetInstanceFromIStorage(COSERVERINFO* pServerInfo, CLSID* pClsid, IUnknown* punkOuter, DWORD dwClsCtx, IStorage* pstg, DWORD dwCount, MULTI_QI* pResults);

HRESULT WINAPI CoGetMalloc(DWORD dwMemContext, LPMALLOC* lpMalloc);
LPVOID WINAPI CoTaskMemAlloc(ULONG size);
void WINAPI CoTaskMemFree(LPVOID ptr);
LPVOID WINAPI CoTaskMemRealloc(LPVOID ptr, ULONG size);

HRESULT WINAPI CoRegisterMallocSpy(LPMALLOCSPY pMallocSpy);
HRESULT WINAPI CoRevokeMallocSpy(void);

/* class registration flags; passed to CoRegisterClassObject */
typedef enum tagREGCLS
{
    REGCLS_SINGLEUSE = 0,
    REGCLS_MULTIPLEUSE = 1,
    REGCLS_MULTI_SEPARATE = 2,
    REGCLS_SUSPENDED = 4
} REGCLS;

HRESULT WINAPI CoGetClassObject(REFCLSID rclsid, DWORD dwClsContext, COSERVERINFO *pServerInfo, REFIID iid, LPVOID *ppv);
HRESULT WINAPI CoRegisterClassObject(REFCLSID rclsid,LPUNKNOWN pUnk,DWORD dwClsContext,DWORD flags,LPDWORD lpdwRegister);
HRESULT WINAPI CoRevokeClassObject(DWORD dwRegister);
HRESULT WINAPI CoGetPSClsid(REFIID riid,CLSID *pclsid);
HRESULT WINAPI CoRegisterPSClsid(REFIID riid, REFCLSID rclsid);
HRESULT WINAPI CoSuspendClassObjects(void);
HRESULT WINAPI CoResumeClassObjects(void);
ULONG WINAPI CoAddRefServerProcess(void);
HRESULT WINAPI CoReleaseServerProcess(void);

/* marshalling */
HRESULT WINAPI CoCreateFreeThreadedMarshaler(LPUNKNOWN punkOuter, LPUNKNOWN* ppunkMarshal);
HRESULT WINAPI CoGetInterfaceAndReleaseStream(LPSTREAM pStm, REFIID iid, LPVOID* ppv);
HRESULT WINAPI CoGetMarshalSizeMax(ULONG* pulSize, REFIID riid, LPUNKNOWN pUnk, DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags);
HRESULT WINAPI CoGetStandardMarshal(REFIID riid, LPUNKNOWN pUnk, DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags, LPMARSHAL* ppMarshal);
HRESULT WINAPI CoMarshalHresult(LPSTREAM pstm, HRESULT hresult);
HRESULT WINAPI CoMarshalInterface(LPSTREAM pStm, REFIID riid, LPUNKNOWN pUnk, DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags);
HRESULT WINAPI CoMarshalInterThreadInterfaceInStream(REFIID riid, LPUNKNOWN pUnk, LPSTREAM* ppStm);
HRESULT WINAPI CoReleaseMarshalData(LPSTREAM pStm);
HRESULT WINAPI CoUnmarshalHresult(LPSTREAM pstm, HRESULT* phresult);
HRESULT WINAPI CoUnmarshalInterface(LPSTREAM pStm, REFIID riid, LPVOID* ppv);
HRESULT WINAPI CoLockObjectExternal(LPUNKNOWN pUnk, BOOL fLock, BOOL fLastUnlockReleases);
BOOL WINAPI CoIsHandlerConnected(LPUNKNOWN pUnk);

/* security */
HRESULT WINAPI CoInitializeSecurity(PSECURITY_DESCRIPTOR pSecDesc, LONG cAuthSvc, SOLE_AUTHENTICATION_SERVICE* asAuthSvc, void* pReserved1, DWORD dwAuthnLevel, DWORD dwImpLevel, void* pReserved2, DWORD dwCapabilities, void* pReserved3);
HRESULT WINAPI CoGetCallContext(REFIID riid, void** ppInterface);
HRESULT WINAPI CoQueryAuthenticationServices(DWORD* pcAuthSvc, SOLE_AUTHENTICATION_SERVICE** asAuthSvc);

HRESULT WINAPI CoQueryProxyBlanket(IUnknown* pProxy, DWORD* pwAuthnSvc, DWORD* pAuthzSvc, OLECHAR** pServerPrincName, DWORD* pAuthnLevel, DWORD* pImpLevel, RPC_AUTH_IDENTITY_HANDLE* pAuthInfo, DWORD* pCapabilites);
HRESULT WINAPI CoSetProxyBlanket(IUnknown* pProxy, DWORD dwAuthnSvc, DWORD dwAuthzSvc, OLECHAR* pServerPrincName, DWORD dwAuthnLevel, DWORD dwImpLevel, RPC_AUTH_IDENTITY_HANDLE pAuthInfo, DWORD dwCapabilities);
HRESULT WINAPI CoCopyProxy(IUnknown* pProxy, IUnknown** ppCopy);

HRESULT WINAPI CoImpersonateClient(void);
HRESULT WINAPI CoQueryClientBlanket(DWORD* pAuthnSvc, DWORD* pAuthzSvc, OLECHAR16** pServerPrincName, DWORD* pAuthnLevel, DWORD* pImpLevel, RPC_AUTHZ_HANDLE* pPrivs, DWORD* pCapabilities);
HRESULT WINAPI CoRevertToSelf(void);

/* misc */
HRESULT WINAPI CoGetTreatAsClass(REFCLSID clsidOld, LPCLSID pClsidNew);
HRESULT WINAPI CoTreatAsClass(REFCLSID clsidOld, REFCLSID clsidNew);

HRESULT WINAPI CoCreateGuid(GUID* pguid);
BOOL WINAPI CoIsOle1Class(REFCLSID rclsid);

BOOL WINAPI CoDosDateTimeToFileTime(WORD nDosDate, WORD nDosTime, FILETIME* lpFileTime);
BOOL WINAPI CoFileTimeToDosDateTime(FILETIME* lpFileTime, WORD* lpDosDate, WORD* lpDosTime);
HRESULT WINAPI CoFileTimeNow(FILETIME* lpFileTime);

/*****************************************************************************
 *	GUID API
 */
HRESULT WINAPI StringFromCLSID16(REFCLSID id, LPOLESTR16*);
HRESULT WINAPI StringFromCLSID(REFCLSID id, LPOLESTR*);

HRESULT WINAPI CLSIDFromString16(LPCOLESTR16, CLSID *);
HRESULT WINAPI CLSIDFromString(LPCOLESTR, CLSID *);

HRESULT WINAPI CLSIDFromProgID16(LPCOLESTR16 progid, LPCLSID riid);
HRESULT WINAPI CLSIDFromProgID(LPCOLESTR progid, LPCLSID riid);

HRESULT WINAPI ProgIDFromCLSID(REFCLSID clsid, LPOLESTR *lplpszProgID);

INT WINAPI StringFromGUID2(REFGUID id, LPOLESTR str, INT cmax);

/*****************************************************************************
 *	COM Server dll - exports
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID * ppv);
HRESULT WINAPI DllCanUnloadNow(void);

/*****************************************************************************
 *	Data Object
 */
HRESULT WINAPI CreateDataAdviseHolder(LPDATAADVISEHOLDER* ppDAHolder);
HRESULT WINAPI CreateDataCache(LPUNKNOWN pUnkOuter, REFCLSID rclsid, REFIID iid, LPVOID* ppv);

/*****************************************************************************
 *	Moniker API
 */
HRESULT WINAPI GetClassFile(LPCOLESTR filePathName,CLSID *pclsid);

HRESULT WINAPI CreateBindCtx16(DWORD reserved, LPBC* ppbc);
HRESULT WINAPI CreateBindCtx(DWORD reserved, LPBC* ppbc);

HRESULT WINAPI CreateFileMoniker16(LPCOLESTR16 lpszPathName, LPMONIKER* ppmk);
HRESULT WINAPI CreateFileMoniker(LPCOLESTR lpszPathName, LPMONIKER* ppmk);

HRESULT WINAPI CreateItemMoniker16(LPCOLESTR16 lpszDelim, LPCOLESTR  lpszItem, LPMONIKER* ppmk);
HRESULT WINAPI CreateItemMoniker(LPCOLESTR lpszDelim, LPCOLESTR  lpszItem, LPMONIKER* ppmk);

HRESULT WINAPI CreateAntiMoniker(LPMONIKER * ppmk);

HRESULT WINAPI CreateGenericComposite(LPMONIKER pmkFirst, LPMONIKER pmkRest, LPMONIKER* ppmkComposite);

HRESULT WINAPI BindMoniker(LPMONIKER pmk, DWORD grfOpt, REFIID iidResult, LPVOID* ppvResult);

HRESULT WINAPI CreateClassMoniker(REFCLSID rclsid, LPMONIKER* ppmk);

HRESULT WINAPI CreatePointerMoniker(LPUNKNOWN punk, LPMONIKER* ppmk);

HRESULT WINAPI MonikerCommonPrefixWith(IMoniker* pmkThis,IMoniker* pmkOther,IMoniker** ppmkCommon);

HRESULT WINAPI GetRunningObjectTable(DWORD reserved, LPRUNNINGOBJECTTABLE *pprot);
HRESULT WINAPI GetRunningObjectTable16(DWORD reserved, LPRUNNINGOBJECTTABLE *pprot);

/*****************************************************************************
 *	Storage API
 */
#define STGM_DIRECT		0x00000000
#define STGM_TRANSACTED		0x00010000
#define STGM_SIMPLE		0x08000000
#define STGM_READ		0x00000000
#define STGM_WRITE		0x00000001
#define STGM_READWRITE		0x00000002
#define STGM_SHARE_DENY_NONE	0x00000040
#define STGM_SHARE_DENY_READ	0x00000030
#define STGM_SHARE_DENY_WRITE	0x00000020
#define STGM_SHARE_EXCLUSIVE	0x00000010
#define STGM_PRIORITY		0x00040000
#define STGM_DELETEONRELEASE	0x04000000
#define STGM_CREATE		0x00001000
#define STGM_CONVERT		0x00020000
#define STGM_FAILIFTHERE	0x00000000
#define STGM_NOSCRATCH		0x00100000
#define STGM_NOSNAPSHOT		0x00200000

HRESULT WINAPI StgCreateDocFile16(LPCOLESTR16 pwcsName,DWORD grfMode,DWORD reserved,IStorage16 **ppstgOpen);
HRESULT WINAPI StgCreateDocfile(LPCOLESTR pwcsName,DWORD grfMode,DWORD reserved,IStorage **ppstgOpen);

HRESULT WINAPI StgIsStorageFile16(LPCOLESTR16 fn);
HRESULT WINAPI StgIsStorageFile(LPCOLESTR fn);
HRESULT WINAPI StgIsStorageILockBytes(ILockBytes *plkbyt);

HRESULT WINAPI StgOpenStorage16(const OLECHAR16* pwcsName,IStorage16* pstgPriority,DWORD grfMode,SNB16 snbExclude,DWORD reserved,IStorage16**ppstgOpen);
HRESULT WINAPI StgOpenStorage(const OLECHAR* pwcsName,IStorage* pstgPriority,DWORD grfMode,SNB snbExclude,DWORD reserved,IStorage**ppstgOpen);

HRESULT WINAPI WriteClassStg(IStorage* pStg, REFCLSID rclsid);
HRESULT WINAPI ReadClassStg(IStorage *pstg,CLSID *pclsid);

HRESULT WINAPI StgCreateDocfileOnILockBytes(ILockBytes *plkbyt,DWORD grfMode, DWORD reserved, IStorage** ppstgOpen);
HRESULT WINAPI StgOpenStorageOnILockBytes(ILockBytes *plkbyt, IStorage *pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, IStorage **ppstgOpen);

#ifdef __cplusplus
}
#endif

#ifndef __WINE__
/* These macros are msdev's way of defining COM objects.
 * They are provided here for use by Winelib developpers.
 */
#define FARSTRUCT
#define HUGEP

#define WINOLEAPI        STDAPI
#define WINOLEAPI_(type) STDAPI_(type)

#if defined(__cplusplus) && !defined(CINTERFACE)
#define interface struct
#define STDMETHOD(method)       virtual HRESULT STDMETHODCALLTYPE method
#define STDMETHOD_(type,method) virtual type STDMETHODCALLTYPE method
#define PURE                    = 0
#define THIS_
#define THIS                    void
#define DECLARE_INTERFACE(iface)    interface iface
#define DECLARE_INTERFACE_(iface, baseiface)    interface iface : public baseiface

#define BEGIN_INTERFACE
#define END_INTERFACE

#else

#define interface               struct
#define STDMETHOD(method)       HRESULT STDMETHODCALLTYPE (*method)
#define STDMETHOD_(type,method) type STDMETHODCALLTYPE (*method)
#define PURE
#define THIS_                   INTERFACE FAR* This,
#define THIS                    INTERFACE FAR* This

#ifdef CONST_VTABLE
#undef CONST_VTBL
#define CONST_VTBL const
#define DECLARE_INTERFACE(iface) \
         typedef interface iface { const struct iface##Vtbl FAR* lpVtbl; } iface; \
         typedef const struct iface##Vtbl iface##Vtbl; \
         const struct iface##Vtbl
#else
#undef CONST_VTBL
#define CONST_VTBL
#define DECLARE_INTERFACE(iface) \
         typedef interface iface { struct iface##Vtbl FAR* lpVtbl; } iface; \
         typedef struct iface##Vtbl iface##Vtbl; \
         struct iface##Vtbl
#endif
#define DECLARE_INTERFACE_(iface, baseiface)    DECLARE_INTERFACE(iface)

#define BEGIN_INTERFACE
#define END_INTERFACE

#endif /* __cplusplus && !CINTERFACE */

#endif /* __WINE__ */

#endif /* _OBJBASE_H_ */
