/*
 * urlmon.h
 */

#ifndef __WINE_URLMON_H
#define __WINE_URLMON_H

#include "winbase.h"
#include "wine/obj_base.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

DEFINE_GUID(IID_IBinding, 0x79EAC9C0, 0xBAF9, 0x11CE,
	0x8C, 0x82, 0x00, 0xAA, 0x00, 0x4B, 0xA9, 0x0B);
typedef struct IBinding IBinding,*LPBINDING;

DEFINE_GUID(IID_IBindStatusCallback, 0x79EAC9C1, 0xBAF9, 0x11CE,
	0x8C, 0x82, 0x00, 0xAA, 0x00, 0x4B, 0xA9, 0x0B);

typedef struct IBindStatusCallback IBindStatusCallback,*LPBINDSTATUSCALLBACK;

typedef struct _tagBINDINFO {
    ULONG cbSize;
    LPWSTR szExtraInfo;
    STGMEDIUM stgmedData;
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

#define MK_S_ASYNCHRONOUS 0x000401E8
#define S_ASYNCHRONOUS    MK_S_ASYNCHRONOUS

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

HRESULT WINAPI CreateURLMoniker(IMoniker *pmkContext, LPWSTR szURL, IMoniker **ppmk);
HRESULT WINAPI RegisterBindStatusCallback(IBindCtx *pbc, IBindStatusCallback *pbsc, IBindStatusCallback **ppbsc, DWORD dwReserved);

#ifdef __cplusplus
}      /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_URLMON_H */

