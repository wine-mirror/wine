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

#ifndef __WINE__
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif
#endif

#ifndef __WINE_URLMON_H
#define __WINE_URLMON_H

#include "winbase.h"
#include "objbase.h"

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
#define ICOM_INTERFACE IBinding
#define IBinding_METHODS \
    ICOM_METHOD  (HRESULT,Abort) \
    ICOM_METHOD  (HRESULT,Suspend) \
    ICOM_METHOD  (HRESULT,Resume) \
    ICOM_METHOD1 (HRESULT,SetPriority,       LONG,nPriority) \
    ICOM_METHOD1 (HRESULT,GetPriority,       LONG*,pnPriority) \
    ICOM_METHOD4 (HRESULT,GetBindResult,     CLSID*,pclsidProtocol, DWORD*,pdwResult, LPOLESTR*,pszResult, DWORD*,pdwReserved)
#define IBinding_IMETHODS \
    IUnknown_IMETHODS \
    IBinding_METHODS
ICOM_DEFINE(IBinding,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IBinding_QueryInterface(p,a,b)      ICOM_CALL2(QueryInterface,p,a,b)
#define IBinding_AddRef(p)                  ICOM_CALL (AddRef,p)
#define IBinding_Release(p)                 ICOM_CALL (Release,p)
/*** IBinding methods ***/
#define IBinding_Abort(p)                   ICOM_CALL (Abort,p)
#define IBinding_Suspend(p)                 ICOM_CALL (Suspend,p)
#define IBinding_Resume(p)                  ICOM_CALL (Resume,p)
#define IBinding_SetPriority(p,a)           ICOM_CALL2(SetPriority,p,a)
#define IBinding_GetPriority(p,a)           ICOM_CALL2(GetPriority,p,a)
#define IBinding_GetBindResult(p,a,b,c,d)   ICOM_CALL4(GetBindResult,p,a,b,c,d)

/*****************************************************************************
 * IBindStatusCallback interface
 */
#define ICOM_INTERFACE IBindStatusCallback
#define IBindStatusCallback_METHODS \
    ICOM_METHOD2 (HRESULT,OnStartBinding,    DWORD,dwReserved, IBinding*,pib) \
    ICOM_METHOD1 (HRESULT,GetPriority,       LONG*,pnPriority) \
    ICOM_METHOD  (HRESULT,OnLowResource) \
    ICOM_METHOD4 (HRESULT,OnProgress,        ULONG,ulProgress, ULONG,ulProgressMax, ULONG,ulStatusCode, LPCWSTR,szStatusText) \
    ICOM_METHOD2 (HRESULT,OnStopBinding,     HRESULT,hresult, LPCWSTR,szError) \
    ICOM_METHOD2 (HRESULT,GetBindInfo,       DWORD*,grfBINDF, BINDINFO*,pbindinfo) \
    ICOM_METHOD4 (HRESULT,OnDataAvailable,   DWORD,grfBSCF, DWORD,dwSize, FORMATETC*,pformatetc, STGMEDIUM*,pstgmed) \
    ICOM_METHOD2 (HRESULT,OnObjectAvailable, REFIID,iid, IUnknown*,punk)
#define IBindStatusCallback_IMETHODS \
    IUnknown_IMETHODS \
    IBindStatusCallback_METHODS
ICOM_DEFINE(IBindStatusCallback,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IBindStatusCallback_QueryInterface(p,a,b)      ICOM_CALL2(QueryInterface,p,a,b)
#define IBindStatusCallback_AddRef(p)                  ICOM_CALL (AddRef,p)
#define IBindStatusCallback_Release(p)                 ICOM_CALL (Release,p)
/*** IBindStatusCallback methods ***/
#define IBindStatusCallback_OnStartBinding(p,a,b)      ICOM_CALL2(OnStartBinding,p,a,b)
#define IBindStatusCallback_GetPriority(p,a)           ICOM_CALL1(GetPriority,p,a)
#define IBindStatusCallback_OnLowResource(p)           ICOM_CALL (OnLowResource,p)
#define IBindStatusCallback_OnProgress(p,a,b,c,d)      ICOM_CALL4(OnProgress,p,a,b,c,d)
#define IBindStatusCallback_OnStopBinding(p,a,b)       ICOM_CALL2(OnStopBinding,p,a,b)
#define IBindStatusCallback_GetBindInfo(p,a,b)         ICOM_CALL2(GetBindInfo,p,a,b)
#define IBindStatusCallback_OnDataAvailable(p,a,b,c,d) ICOM_CALL4(OnDataAvailable,p,a,b,c,d)
#define IBindStatusCallback_OnObjectAvailable(p,a,b)   ICOM_CALL2(OnObjectAvailable,p,a,b)

/*****************************************************************************
 * IBindHost interface
 */
#define ICOM_INTERFACE IBindHost
#define IBindHost_METHODS \
    ICOM_METHOD4 (HRESULT,CreateMoniker,              LPOLESTR,szName, IBindCtx*,pBC, IMoniker**,ppmk, DWORD,dwReserved) \
    ICOM_METHOD5 (HRESULT,MonikerBindToStorage,       IMoniker*,pMk, IBindCtx*,pBC, IBindStatusCallback*,pBSC, REFIID,riid, LPVOID*,ppvObj) \
    ICOM_METHOD5 (HRESULT,MonikerBindToObject,        IMoniker*,pMk, IBindCtx*,pBC, IBindStatusCallback*,pBSC, REFIID,riid, LPVOID*,ppvObj)
#define IBindHost_IMETHODS \
    IUnknown_IMETHODS \
    IBindHost_METHODS
ICOM_DEFINE(IBindHost,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IBindHost_QueryInterface(p,a,b)                   ICOM_CALL2(QueryInterface,p,a,b)
#define IBindHost_AddRef(p)                               ICOM_CALL (AddRef,p)
#define IBindHost_Release(p)                              ICOM_CALL (Release,p)
/*** IBindHost methods ***/
#define IBindHost_CreateMoniker(p,a,b,c,d)                ICOM_CALL4(CreateMoniker,p,a,b,c,d)
#define IBindHost_MonikerBindToStorage(p,a,b,c,d,e)       ICOM_CALL5(MonikerBindToStorage,p,a,b,c,d,e)
#define IBindHost_MonikerBindToObject(p,a,b,c,d,e)        ICOM_CALL5(MonikerBindToObject,p,a,b,c,d,e)

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
#define ICOM_INTERFACE IWinInetInfo
#define IWinInetInfo_METHODS \
    ICOM_METHOD3 (HRESULT,QueryOption,       DWORD,dwOption, LPVOID,pBuffer, DWORD*,pcbBuf)
#define IWinInetInfo_IMETHODS \
    IUnknown_IMETHODS \
    IWinInetInfo_METHODS
ICOM_DEFINE(IWinInetInfo,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IWinInetInfo_QueryInterface(p,a,b)      ICOM_CALL2(QueryInterface,p,a,b)
#define IWinInetInfo_AddRef(p)                  ICOM_CALL (AddRef,p)
#define IWinInetInfo_Release(p)                 ICOM_CALL (Release,p)
/*** IWinInetInfo methods ***/
#define IWinInetInfo_QueryOption(p,a,b,c)       ICOM_CALL3(QueryOption,p,a,b,c)

/*****************************************************************************
 * IWinInetHttpInfo interface
 */
#define ICOM_INTERFACE IWinInetHttpInfo
#define IWinInetHttpInfo_METHODS \
    ICOM_METHOD5 (HRESULT,QueryInfo, DWORD,dwOption, LPVOID,pBuffer, DWORD*,pcbBuf, DWORD*,pdwFlags, DWORD*,pdwReserved)
#define IWinInetHttpInfo_IMETHODS \
    IWinInetInfo_IMETHODS \
    IWinInetHttpInfo_METHODS
ICOM_DEFINE(IWinInetHttpInfo,IWinInetInfo)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IWinInetHttpInfo_QueryInterface(p,a,b)  ICOM_CALL2(QueryInterface,p,a,b)
#define IWinInetHttpInfo_AddRef(p)              ICOM_CALL (AddRef,p)
#define IWinInetHttpInfo_Release(p)             ICOM_CALL (Release,p)
/*** IWinInetHttpInfo methods ***/
#define IWinInetHttpInfo_QueryInfo(p,a,b,c,d,e) ICOM_CALL5(QueryInfo,p,a,b,c,d,e)

HRESULT WINAPI CreateURLMoniker(IMoniker *pmkContext, LPCWSTR szURL, IMoniker **ppmk);
HRESULT WINAPI RegisterBindStatusCallback(IBindCtx *pbc, IBindStatusCallback *pbsc, IBindStatusCallback **ppbsc, DWORD dwReserved);

#ifdef __cplusplus
}      /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_URLMON_H */
