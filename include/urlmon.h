/*
 * Copyright (C) 2000 Ulrich Czekalla
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

#ifndef __WINE_URLMON_H
#define __WINE_URLMON_H

#include <winbase.h>
#include <objbase.h>

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

DEFINE_GUID(IID_IBinding, 0x79EAC9C0, 0xBAF9, 0x11CE,
	0x8C, 0x82, 0x00, 0xAA, 0x00, 0x4B, 0xA9, 0x0B);
typedef struct IBinding IBinding,*LPBINDING;

DEFINE_GUID(IID_IBindStatusCallback, 0x79EAC9C1, 0xBAF9, 0x11CE,
	0x8C, 0x82, 0x00, 0xAA, 0x00, 0x4B, 0xA9, 0x0B);
typedef struct IBindStatusCallback IBindStatusCallback,*LPBINDSTATUSCALLBACK;

DEFINE_GUID(IID_IBindHost, 0XFC4801A1, 0X2BA9, 0X11CF,
    0XA2, 0X29, 0X00, 0XAA, 0X00, 0X3D, 0X73, 0X52);
typedef struct IBindHost IBindHost,*LPBINDHOST;

DEFINE_GUID(IID_IWinInetInfo, 0x79EAC9D6, 0xBAFA, 0x11CE,
    0x8C, 0x82, 0x00, 0xAA, 0x00, 0x4B, 0xA9, 0X0B);
typedef struct IWinInetInfo IWinInetInfo,*LPWININETINFO;

DEFINE_GUID(IID_IWinInetHttpInfo, 0x79EAC9D8, 0xBAFA, 0x11CE,
    0x8C, 0x82, 0x00, 0xAA, 0x00, 0x4B, 0xA9, 0X0B);
typedef struct IWinInetHttpInfo IWinInetHttpInfo,*LPWININETHTTPINFO;

DEFINE_GUID(IID_IPersistMoniker, 0x79eac9c9, 0xbaf9, 0x11ce,
    0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b);
typedef struct IPersistMoniker IPersistMoniker;

typedef enum {
	BINDF_ASYNCHRONOUS = 0x00000001,
	BINDF_ASYNCSTORAGE = 0x00000002,
	BINDF_NOPROGRESSIVERENDERING = 0x00000004,
	BINDF_OFFLINEOPERATION = 0x00000008,
	BINDF_GETNEWESTVERSION = 0x00000010,
	BINDF_NOWRITECACHE = 0x00000020,
	BINDF_NEEDFILE = 0x00000040,
	BINDF_PULLDATA = 0x00000080,
	BINDF_IGNORESECURITYPROBLEM = 0x00000100,
	BINDF_RESYNCHRONIZE = 0x00000200,
	BINDF_HYPERLINK = 0x00000400,
	BINDF_NO_UI = 0x00000800,
	BINDF_SILENTOPERATION = 0x00001000,
	BINDF_PRAGMA_NO_CACHE = 0x00002000,
	BINDF_GETCLASSOBJECT = 0x00004000,
	BINDF_RESERVED_1 = 0x00008000,
	BINDF_FREE_THREADED = 0x00010000,
	BINDF_DIRECT_READ = 0x00020000,
	BINDF_FORMS_SUBMIT = 0x00040000,
	BINDF_GETFROMCACHE_IF_NET_FAIL = 0x00080000,
	BINDF_FROMURLMON = 0x00100000,
	BINDF_FWD_BACK = 0x00200000,
	BINDF_PREFERDEFAULTHANDLER = 0x00400000,
	BINDF_RESERVED_3 = 0x00800000
} BINDF;

typedef struct _tagBINDINFO {
    ULONG cbSize;
    LPWSTR szExtraInfo;
    STGMEDIUM stgmedData;
    DWORD grfBindInfoF;
    DWORD dwBindVerb;
    LPWSTR szCustomVerb;
    DWORD cbStgmedData;
    DWORD dwOptions;
    DWORD dwOptionsFlags;
    DWORD dwCodePage;
    SECURITY_ATTRIBUTES securityAttributes;
    IID iid;
    IUnknown *pUnk;
    DWORD dwReserved;
} BINDINFO;

typedef enum {
    BSCF_FIRSTDATANOTIFICATION = 0x01,
    BSCF_INTERMEDIATEDATANOTIFICATION = 0x02,
    BSCF_LASTDATANOTIFICATION = 0x04,
    BSCF_DATAFULLYAVAILABLE = 0x08,
    BSCF_AVAILABLEDATASIZEUNKNOWN = 0x10
} BSCF;

typedef enum BINDSTATUS {
	BINDSTATUS_FINDINGRESOURCE,
	BINDSTATUS_CONNECTING,
	BINDSTATUS_REDIRECTING,
	BINDSTATUS_BEGINDOWNLOADDATA,
	BINDSTATUS_DOWNLOADINGDATA,
	BINDSTATUS_ENDDOWNLOADDATA,
	BINDSTATUS_BEGINDOWNLOADCOMPONENTS,
	BINDSTATUS_INSTALLINGCOMPONENTS,
	BINDSTATUS_ENDDOWNLOADCOMPONENTS,
	BINDSTATUS_USINGCACHEDCOPY,
	BINDSTATUS_SENDINGREQUEST,
	BINDSTATUS_CLASSIDAVAILABLE,
	BINDSTATUS_MIMETYPEAVAILABLE,
	BINDSTATUS_CACHEFILENAMEAVAILABLE,
	BINDSTATUS_BEGINSYNCOPERATION,
	BINDSTATUS_ENDSYNCOPERATION,
	BINDSTATUS_BEGINUPLOADDATA,
	BINDSTATUS_UPLOADINGDATA,
	BINDSTATUS_ENDUPLOADINGDATA,
	BINDSTATUS_PROTOCOLCLASSID,
	BINDSTATUS_ENCODING,
	BINDSTATUS_VERFIEDMIMETYPEAVAILABLE,
	BINDSTATUS_CLASSINSTALLLOCATION,
	BINDSTATUS_DECODING,
	BINDSTATUS_LOADINGMIMEHANDLER,
	BINDSTATUS_CONTENTDISPOSITIONATTACH,
	BINDSTATUS_FILTERREPORTMIMETYPE,
	BINDSTATUS_CLSIDCANINSTANTIATE,
	BINDSTATUS_IUNKNOWNAVAILABLE,
	BINDSTATUS_DIRECTBIND,
	BINDSTATUS_RAWMIMETYPE,
	BINDSTATUS_PROXYDETECTING,
	BINDSTATUS_ACCEPTRANGES
} BINDSTATUS;

#define MK_S_ASYNCHRONOUS 0x000401E8
#define S_ASYNCHRONOUS    MK_S_ASYNCHRONOUS

#define INET_E_ERROR_FIRST               0x800C0002L
#define INET_E_INVALID_URL               0x800C0002L
#define INET_E_NO_SESSION                0x800C0003L
#define INET_E_CANNOT_CONNECT            0x800C0004L
#define INET_E_RESOURCE_NOT_FOUND        0x800C0005L
#define INET_E_OBJECT_NOT_FOUND          0x800C0006L
#define INET_E_DATA_NOT_AVAILABLE        0x800C0007L
#define INET_E_DOWNLOAD_FAILURE          0x800C0008L
#define INET_E_AUTHENTICATION_REQUIRED   0x800C0009L
#define INET_E_NO_VALID_MEDIA            0x800C000AL
#define INET_E_CONNECTION_TIMEOUT        0x800C000BL
#define INET_E_INVALID_REQUEST           0x800C000CL
#define INET_E_UNKNOWN_PROTOCOL          0x800C000DL
#define INET_E_SECURITY_PROBLEM          0x800C000EL
#define INET_E_CANNOT_LOAD_DATA          0x800C000FL
#define INET_E_CANNOT_INSTANTIATE_OBJECT 0x800C0010L
#define INET_E_QUERYOPTION_UNKNOWN       0x800C0013L
#define INET_E_REDIRECT_FAILED           0x800C0014L
#define INET_E_REDIRECT_TO_DIR           0x800C0015L
#define INET_E_CANNOT_LOCK_REQUEST       0x800C0016L
#define INET_E_ERROR_LAST                INET_E_REDIRECT_TO_DIR


/*****************************************************************************
 * IBinding interface
 */
#define INTERFACE IBinding
#define IBinding_METHODS \
    IUnknown_METHODS \
    STDMETHOD(Abort)(THIS) PURE; \
    STDMETHOD(Suspend)(THIS) PURE; \
    STDMETHOD(Resume)(THIS) PURE; \
    STDMETHOD(SetPriority)(THIS_ LONG nPriority) PURE; \
    STDMETHOD(GetPriority)(THIS_ LONG *pnPriority) PURE; \
    STDMETHOD(GetBindResult)(THIS_ CLSID *pclsidProtocol, DWORD *pdwResult, LPOLESTR *pszResult, DWORD *pdwReserved) PURE;
ICOM_DEFINE(IBinding,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IBinding_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IBinding_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IBinding_Release(p)                 (p)->lpVtbl->Release(p)
/*** IBinding methods ***/
#define IBinding_Abort(p)                   (p)->lpVtbl->Abort(p)
#define IBinding_Suspend(p)                 (p)->lpVtbl->Suspend(p)
#define IBinding_Resume(p)                  (p)->lpVtbl->Resume(p)
#define IBinding_SetPriority(p,a)           (p)->lpVtbl->SetPriority(p,a)
#define IBinding_GetPriority(p,a)           (p)->lpVtbl->GetPriority(p,a)
#define IBinding_GetBindResult(p,a,b,c,d)   (p)->lpVtbl->GetBindResult(p,a,b,c,d)
#endif

/*****************************************************************************
 * IBindStatusCallback interface
 */
#define INTERFACE IBindStatusCallback
#define IBindStatusCallback_METHODS \
    IUnknown_METHODS \
    STDMETHOD(OnStartBinding)(THIS_ DWORD dwReserved, IBinding *pib) PURE; \
    STDMETHOD(GetPriority)(THIS_ LONG *pnPriority) PURE; \
    STDMETHOD(OnLowResource)(THIS) PURE; \
    STDMETHOD(OnProgress)(THIS_ ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText) PURE; \
    STDMETHOD(OnStopBinding)(THIS_ HRESULT hresult, LPCWSTR szError) PURE; \
    STDMETHOD(GetBindInfo)(THIS_ DWORD *grfBINDF, BINDINFO *pbindinfo) PURE; \
    STDMETHOD(OnDataAvailable)(THIS_ DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed) PURE; \
    STDMETHOD(OnObjectAvailable)(THIS_ REFIID iid, IUnknown *punk) PURE;
ICOM_DEFINE(IBindStatusCallback,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IBindStatusCallback_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IBindStatusCallback_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IBindStatusCallback_Release(p)                 (p)->lpVtbl->Release(p)
/*** IBindStatusCallback methods ***/
#define IBindStatusCallback_OnStartBinding(p,a,b)      (p)->lpVtbl->OnStartBinding(p,a,b)
#define IBindStatusCallback_GetPriority(p,a)           (p)->lpVtbl->GetPriority(p,a)
#define IBindStatusCallback_OnLowResource(p)           (p)->lpVtbl->OnLowResource(p)
#define IBindStatusCallback_OnProgress(p,a,b,c,d)      (p)->lpVtbl->OnProgress(p,a,b,c,d)
#define IBindStatusCallback_OnStopBinding(p,a,b)       (p)->lpVtbl->OnStopBinding(p,a,b)
#define IBindStatusCallback_GetBindInfo(p,a,b)         (p)->lpVtbl->GetBindInfo(p,a,b)
#define IBindStatusCallback_OnDataAvailable(p,a,b,c,d) (p)->lpVtbl->OnDataAvailable(p,a,b,c,d)
#define IBindStatusCallback_OnObjectAvailable(p,a,b)   (p)->lpVtbl->OnObjectAvailable(p,a,b)
#endif

/*****************************************************************************
 * IBindHost interface
 */
#define INTERFACE IBindHost
#define IBindHost_METHODS \
    IUnknown_METHODS \
    STDMETHOD(CreateMoniker)(THIS_ LPOLESTR szName, IBindCtx *pBC, IMoniker **ppmk, DWORD dwReserved) PURE; \
    STDMETHOD(MonikerBindToStorage)(THIS_ IMoniker *pMk, IBindCtx *pBC, IBindStatusCallback *pBSC, REFIID riid, LPVOID *ppvObj) PURE; \
    STDMETHOD(MonikerBindToObject)(THIS_ IMoniker *pMk, IBindCtx *pBC, IBindStatusCallback *pBSC, REFIID riid, LPVOID *ppvObj) PURE;
ICOM_DEFINE(IBindHost,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IBindHost_QueryInterface(p,a,b)                   (p)->lpVtbl->QueryInterface(p,a,b)
#define IBindHost_AddRef(p)                               (p)->lpVtbl->AddRef(p)
#define IBindHost_Release(p)                              (p)->lpVtbl->Release(p)
/*** IBindHost methods ***/
#define IBindHost_CreateMoniker(p,a,b,c,d)                (p)->lpVtbl->CreateMoniker(p,a,b,c,d)
#define IBindHost_MonikerBindToStorage(p,a,b,c,d,e)       (p)->lpVtbl->MonikerBindToStorage(p,a,b,c,d,e)
#define IBindHost_MonikerBindToObject(p,a,b,c,d,e)        (p)->lpVtbl->MonikerBindToObject(p,a,b,c,d,e)
#endif

/*** IUnknown methods ***/
typedef enum _tagQUERYOPTION {
    QUERY_EXPIRATION_DATE = 1,
    QUERY_TIME_OF_LAST_CHANGE,
    QUERY_CONTENT_ENCODING,
    QUERY_CONTENT_TYPE,
    QUERY_REFRESH,
    QUERY_RECOMBINE,
    QUERY_CAN_NAVIGATE,
    QUERY_USES_NETWORK,
    QUERY_IS_CACHED,
    QUERY_IS_INSTALLEDENTRY,
    QUERY_IS_CACHED_OR_MAPPED,
    QUERY_USES_CACHE,
    QUERY_IS_SECURE,
    QUERY_IS_SAFE
} QUERYOPTION;

/*****************************************************************************
 * IWinInetInfo interface
 */
#define INTERFACE IWinInetInfo
#define IWinInetInfo_METHODS \
    IUnknown_METHODS \
    STDMETHOD(QueryOption)(THIS_ DWORD dwOption, LPVOID pBuffer, DWORD *pcbBuf) PURE;
ICOM_DEFINE(IWinInetInfo,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IWinInetInfo_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IWinInetInfo_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IWinInetInfo_Release(p)                 (p)->lpVtbl->Release(p)
/*** IWinInetInfo methods ***/
#define IWinInetInfo_QueryOption(p,a,b,c)       (p)->lpVtbl->QueryOption(p,a,b,c)
#endif

/*****************************************************************************
 * IWinInetHttpInfo interface
 */
#define INTERFACE IWinInetHttpInfo
#define IWinInetHttpInfo_METHODS \
    IWinInetInfo_METHODS \
    STDMETHOD(QueryInfo)(THIS_ DWORD dwOption, LPVOID pBuffer, DWORD *pcbBuf, DWORD *pdwFlags, DWORD *pdwReserved) PURE;
ICOM_DEFINE(IWinInetHttpInfo,IWinInetInfo)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IWinInetHttpInfo_QueryInterface(p,a,b)  (p)->lpVtbl->QueryInterface(p,a,b)
#define IWinInetHttpInfo_AddRef(p)              (p)->lpVtbl->AddRef(p)
#define IWinInetHttpInfo_Release(p)             (p)->lpVtbl->Release(p)
/*** IWinInetHttpInfo methods ***/
#define IWinInetHttpInfo_QueryInfo(p,a,b,c,d,e) (p)->lpVtbl->QueryInfo(p,a,b,c,d,e)
#endif

#define INTERFACE IPersistMoniker
#define IPersistMoniker_METHODS \
    IUnknown_METHODS \
    STDMETHOD(GetClassID)(THIS_ CLSID *pClassID ) PURE; \
    STDMETHOD(IsDirty)(THIS ) PURE; \
    STDMETHOD(Load)(THIS_ BOOL fFullyAvailable, IMoniker *pimkName, LPBC pibc, DWORD grfMode ) PURE; \
    STDMETHOD(Save)(THIS_ IMoniker *pinkName, LPBC pibc, BOOL fRemember ) PURE; \
    STDMETHOD(SaveCompleted)(THIS_ IMoniker *pinkName, LPBC pibc ) PURE; \
    STDMETHOD(GetCurMoniker)(THIS_ IMoniker *pinkName ) PURE;
ICOM_DEFINE(IPersistMoniker,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
#define IPersistMoniker_QueryInterface(p,a,b)  (p)->lpVtbl->QueryInterface(p,a,b)
#define IPersistMoniker_AddRef(p)              (p)->lpVtbl->AddRef(p)
#define IPersistMoniker_Release(p)             (p)->lpVtbl->Release(p)
#define IPersistMoniker_GetClassID(p,a)        (p)->lpVtbl->GetClassID(p,a)
#define IPersistMoniker_IsDirty(p)             (p)->lpVtbl->IsDirty(p)
#define IPersistMoniker_Load(p,a,b,c,d)        (p)->lpVtbl->Load(p,a,b,c,d)
#define IPersistMoniker_Save(p,a,b,c)          (p)->lpVtbl->Save(p,a,b,c)
#define IPersistMoniker_SaveCompleted(p,a,b)   (p)->lpVtbl->SaveCompleted(p,a,b)
#define IPersistMoniker_GetCurMoniker(p,a)     (p)->lpVtbl->GetCurMoniker(p,a)
#endif

HRESULT WINAPI CreateURLMoniker(IMoniker *pmkContext, LPCWSTR szURL, IMoniker **ppmk);
HRESULT WINAPI RegisterBindStatusCallback(IBindCtx *pbc, IBindStatusCallback *pbsc, IBindStatusCallback **ppbsc, DWORD dwReserved);
HRESULT WINAPI CompareSecurityIds(BYTE*,DWORD,BYTE*,DWORD,DWORD);

HRESULT WINAPI URLDownloadToFileA(LPUNKNOWN pCaller, LPCSTR szURL,  LPCSTR  szFileName, DWORD dwReserved, LPBINDSTATUSCALLBACK lpfnCB);
HRESULT WINAPI URLDownloadToFileW(LPUNKNOWN pCaller, LPCWSTR szURL, LPCWSTR szFileName, DWORD dwReserved, LPBINDSTATUSCALLBACK lpfnCB);


#ifdef __cplusplus
}      /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_URLMON_H */
