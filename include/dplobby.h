#ifndef __WINE_DPLOBBY_H
#define __WINE_DPLOBBY_H

#include "dplay.h"

#include "pshpack1.h"

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID(CLSID_DirectPlayLobby, 0x2fe8f810, 0xb2a5, 0x11d0, 0xa7, 0x87, 0x0, 0x0, 0xf8, 0x3, 0xab, 0xfc);

DEFINE_GUID(IID_IDirectPlayLobby, 0xaf465c71, 0x9588, 0x11cf, 0xa0, 0x20, 0x0, 0xaa, 0x0, 0x61, 0x57, 0xac);
typedef struct IDirectPlayLobby IDirectPlayLobby,*LPDIRECTPLAYLOBBY;

DEFINE_GUID(IID_IDirectPlayLobbyA, 0x26c66a70, 0xb367, 0x11cf, 0xa0, 0x24, 0x0, 0xaa, 0x0, 0x61, 0x57, 0xac);
typedef struct IDirectPlayLobby IDirectPlayLobbyA,*LPDIRECTPLAYLOBBYA;

DEFINE_GUID(IID_IDirectPlayLobby2, 0x194c220, 0xa303, 0x11d0, 0x9c, 0x4f, 0x0, 0xa0, 0xc9, 0x5, 0x42, 0x5e);
typedef struct IDirectPlayLobby2 IDirectPlayLobby2, *LPDIRECTPLAYLOBBY2;

DEFINE_GUID(IID_IDirectPlayLobby2A, 0x1bb4af80, 0xa303, 0x11d0, 0x9c, 0x4f, 0x0, 0xa0, 0xc9, 0x5, 0x42, 0x5e);
typedef struct IDirectPlayLobby2 IDirectPlayLobby2A, *LPDIRECTPLAYLOBBY2A;


/*****************************************************************************
 * Miscellaneous
 */

typedef struct tagDPLAPPINFO
{
    DWORD       dwSize;            
    GUID        guidApplication;   

    union appName
    {
        LPSTR   lpszAppNameA;      
        LPWSTR  lpszAppName;
    };

} DPLAPPINFO, *LPDPLAPPINFO;
typedef const DPLAPPINFO *LPCDPLAPPINFO;

typedef struct DPCOMPOUNDADDRESSELEMENT
{
    GUID    guidDataType;
    DWORD   dwDataSize;
    LPVOID  lpData;
} DPCOMPOUNDADDRESSELEMENT, *LPDPCOMPOUNDADDRESSELEMENT;
typedef const DPCOMPOUNDADDRESSELEMENT *LPCDPCOMPOUNDADDRESSELEMENT;


extern HRESULT WINAPI DirectPlayLobbyCreateW(LPGUID, LPDIRECTPLAYLOBBY *, IUnknown *, LPVOID, DWORD );
extern HRESULT WINAPI DirectPlayLobbyCreateA(LPGUID, LPDIRECTPLAYLOBBYA *, IUnknown *, LPVOID, DWORD );



typedef BOOL (CALLBACK* LPDPENUMADDRESSCALLBACK)(
    REFGUID         guidDataType,
    DWORD           dwDataSize,
    LPCVOID         lpData,
    LPVOID          lpContext );

typedef BOOL (CALLBACK* LPDPLENUMADDRESSTYPESCALLBACK)(
    REFGUID         guidDataType,
    LPVOID          lpContext,
    DWORD           dwFlags );

typedef BOOL (CALLBACK* LPDPLENUMLOCALAPPLICATIONSCALLBACK)(
    LPCDPLAPPINFO   lpAppInfo,
    LPVOID          lpContext,
    DWORD           dwFlags );

#include "poppack.h"

/*****************************************************************************
 * IDirectPlayLobby interface
 */
#define ICOM_INTERFACE IDirectPlayLobby
#define IDirectPlayLobby_METHODS \
    ICOM_METHOD3(HRESULT,Connect,               DWORD,, LPDIRECTPLAY2*,, IUnknown*,) \
    ICOM_METHOD6(HRESULT,CreateAddress,         REFGUID,, REFGUID,, LPCVOID,, DWORD,, LPVOID,, LPDWORD,) \
    ICOM_METHOD4(HRESULT,EnumAddress,           LPDPENUMADDRESSCALLBACK,, LPCVOID,, DWORD,, LPVOID,) \
    ICOM_METHOD4(HRESULT,EnumAddressTypes,      LPDPLENUMADDRESSTYPESCALLBACK,, REFGUID,, LPVOID,, DWORD,) \
    ICOM_METHOD3(HRESULT,EnumLocalApplications, LPDPLENUMLOCALAPPLICATIONSCALLBACK,, LPVOID,, DWORD,) \
    ICOM_METHOD3(HRESULT,GetConnectionSettings, DWORD,, LPVOID,, LPDWORD,) \
    ICOM_METHOD5(HRESULT,ReceiveLobbyMessage,   DWORD,, DWORD,, LPDWORD,, LPVOID,, LPDWORD,) \
    ICOM_METHOD4(HRESULT,RunApplication,        DWORD,, LPDWORD,, LPDPLCONNECTION,, HANDLE,) \
    ICOM_METHOD4(HRESULT,SendLobbyMessage,      DWORD,, DWORD,, LPVOID,, DWORD,) \
    ICOM_METHOD3(HRESULT,SetConnectionSettings, DWORD,, DWORD,, LPDPLCONNECTION,) \
    ICOM_METHOD3(HRESULT,SetLobbyMessageEvent,  DWORD,, DWORD,, HANDLE,)
#define IDirectPlayLobby_IMETHODS \
    IUnknown_IMETHODS \
    IDirectPlayLobby_METHODS
ICOM_DEFINE(IDirectPlayLobby,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
/*** IUnknown methods ***/
#define IDirectPlayLobby_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectPlayLobby_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirectPlayLobby_Release(p)            ICOM_CALL (Release,p)
/*** IDirectPlayLobby methods ***/
#define IDirectPlayLobby_Connect(p,a,b,c)                 ICOM_CALL3(Connect,p,a,b,c)
#define IDirectPlayLobby_CreateAddress(p,a,b,c,d,e,f)     ICOM_CALL6(CreateAddress,p,a,b,c,d,e,f)
#define IDirectPlayLobby_EnumAddress(p,a,b,c,d)           ICOM_CALL4(EnumAddress,p,a,b,c,d)
#define IDirectPlayLobby_EnumAddressTypes(p,a,b,c,d)      ICOM_CALL4(EnumAddressTypes,p,a,b,c,d)
#define IDirectPlayLobby_EnumLocalApplications(p,a,b,c)   ICOM_CALL3(EnumLocalApplications,p,a,b,c)
#define IDirectPlayLobby_GetConnectionSettings(p,a,b,c)   ICOM_CALL3(GetConnectionSettings,p,a,b,c)
#define IDirectPlayLobby_ReceiveLobbyMessage(p,a,b,c,d,e) ICOM_CALL5(ReceiveLobbyMessage,p,a,b,c,d,e)
#define IDirectPlayLobby_RunApplication(p,a,b,c,d)        ICOM_CALL4(RunApplication,p,a,b,c,d)
#define IDirectPlayLobby_SendLobbyMessage(p,a,b,c,d)      ICOM_CALL4(SendLobbyMessage,p,a,b,c,d)
#define IDirectPlayLobby_SetConnectionSettings(p,a,b,c)   ICOM_CALL3(SetConnectionSettings,p,a,b,c)
#define IDirectPlayLobby_SetLobbyMessageEvent(p,a,b,c)    ICOM_CALL3(SetLobbyMessageEvent,p,a,b,c)
#endif


/*****************************************************************************
 * IDirectPlayLobby2 interface
 */
#define ICOM_INTERFACE IDirectPlayLobby2
#define IDirectPlayLobby2_METHODS \
    ICOM_METHOD4(HRESULT,CreateCompoundAddress, LPCDPCOMPOUNDADDRESSELEMENT,, DWORD,, LPVOID,, LPDWORD,)
#define IDirectPlayLobby2_IMETHODS \
    IDirectPlayLobby_IMETHODS \
    IDirectPlayLobby2_METHODS
ICOM_DEFINE(IDirectPlayLobby2,IDirectPlayLobby)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
/*** IUnknown methods ***/
#define IDirectPlayLobby2_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectPlayLobby2_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirectPlayLobby2_Release(p)            ICOM_CALL (Release,p)
/*** IDirectPlayLobby methods ***/
#define IDirectPlayLobby2_Connect(p,a,b,c)                 ICOM_CALL3(Connect,p,a,b,c)
#define IDirectPlayLobby2_CreateAddress(p,a,b,c,d,e,f)     ICOM_CALL6(CreateAddress,p,a,b,c,d,e,f)
#define IDirectPlayLobby2_EnumAddress(p,a,b,c,d)           ICOM_CALL4(EnumAddress,p,a,b,c,d)
#define IDirectPlayLobby2_EnumAddressTypes(p,a,b,c,d)      ICOM_CALL4(EnumAddressTypes,p,a,b,c,d)
#define IDirectPlayLobby2_EnumLocalApplications(p,a,b,c)   ICOM_CALL3(EnumLocalApplications,p,a,b,c)
#define IDirectPlayLobby2_GetConnectionSettings(p,a,b,c)   ICOM_CALL3(GetConnectionSettings,p,a,b,c)
#define IDirectPlayLobby2_ReceiveLobbyMessage(p,a,b,c,d,e) ICOM_CALL5(ReceiveLobbyMessage,p,a,b,c,d,e)
#define IDirectPlayLobby2_RunApplication(p,a,b,c,d)        ICOM_CALL4(RunApplication,p,a,b,c,d)
#define IDirectPlayLobby2_SendLobbyMessage(p,a,b,c,d)      ICOM_CALL4(SendLobbyMessage,p,a,b,c,d)
#define IDirectPlayLobby2_SetConnectionSettings(p,a,b,c)   ICOM_CALL3(SetConnectionSettings,p,a,b,c)
#define IDirectPlayLobby2_SetLobbyMessageEvent(p,a,b,c)    ICOM_CALL3(SetLobbyMessageEvent,p,a,b,c)
/*** IDirectPlayLobby2 methods ***/
#define IDirectPlayLobby2_CreateCompoundAddress(p,a,b,c,d) ICOM_CALL4(CreateCompoundAddress,p,a,b,c,d)
#endif

#endif /* __WINE_DPLOBBY_H */
