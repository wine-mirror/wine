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
HRESULT WINAPI CoCreateGuid(GUID* pguid);

HINSTANCE WINAPI CoLoadLibrary(LPOLESTR lpszLibName, BOOL bAutoFree);

void WINAPI CoFreeAllLibraries(void);

void WINAPI CoFreeLibrary(HINSTANCE hLibrary);

void WINAPI CoFreeUnusedLibraries(void);

HRESULT WINAPI CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID iid, LPVOID *ppv);

HRESULT WINAPI CoGetClassObject(REFCLSID rclsid, DWORD dwClsContext, COSERVERINFO *pServerInfo, REFIID iid, LPVOID *ppv);

HRESULT WINAPI CoInitialize(LPVOID lpReserved);
HRESULT WINAPI CoInitializeEx(LPVOID lpReserved, DWORD dwCoInit);

void WINAPI CoUninitialize(void);

typedef enum tagCOINIT
{
    COINIT_APARTMENTTHREADED  = 0x2, /* Apartment model */
    COINIT_MULTITHREADED      = 0x0, /* OLE calls objects on any thread */
    COINIT_DISABLE_OLE1DDE    = 0x4, /* Don't use DDE for Ole1 support */
    COINIT_SPEED_OVER_MEMORY  = 0x8  /* Trade memory for speed */
} COINIT;


/* FIXME: not implemented */
BOOL WINAPI CoIsOle1Class(REFCLSID rclsid);

HRESULT WINAPI CoLockObjectExternal(LPUNKNOWN pUnk, BOOL fLock, BOOL fLastUnlockReleases);

/* class registration flags; passed to CoRegisterClassObject */
typedef enum tagREGCLS
{
    REGCLS_SINGLEUSE = 0,
    REGCLS_MULTIPLEUSE = 1,
    REGCLS_MULTI_SEPARATE = 2,
    REGCLS_SUSPENDED = 4
} REGCLS;

HRESULT WINAPI CoRegisterClassObject(REFCLSID rclsid,LPUNKNOWN pUnk,DWORD dwClsContext,DWORD flags,LPDWORD lpdwRegister);

HRESULT WINAPI CoRevokeClassObject(DWORD dwRegister);

HRESULT WINAPI CoGetPSClsid(REFIID riid,CLSID *pclsid);

/*****************************************************************************
 * GUID API
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
 *	Moniker API
 */
HRESULT WINAPI GetClassFile(LPCOLESTR filePathName,CLSID *pclsid);

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
