/*
 * DDEML library definitions
 *
 * Copyright 1997 Alexandre Julliard
 * Copyright 1997 Len White
 */

#ifndef __WINE__DDEML_H
#define __WINE__DDEML_H

#include "wintypes.h"

typedef DWORD HCONVLIST;
typedef DWORD HCONV;
typedef DWORD HSZ;
typedef DWORD HDDEDATA;

typedef HDDEDATA (CALLBACK *PFNCALLBACK16)(UINT16,UINT16,HCONV,HSZ,HSZ,
                                           HDDEDATA,DWORD,DWORD);
typedef HDDEDATA (CALLBACK *PFNCALLBACK32)(UINT32,UINT32,HCONV,HSZ,HSZ,
                                           HDDEDATA,DWORD,DWORD);
DECL_WINELIB_TYPE(PFNCALLBACK)

typedef struct
{
    UINT16  cb;
    UINT16  wFlags;
    UINT16  wCountryID;
    INT16   iCodePage;
    DWORD   dwLangID;
    DWORD   dwSecurity;
} CONVCONTEXT16, *LPCONVCONTEXT16;

typedef struct
{
    UINT32  cb;
    UINT32  wFlags;
    UINT32  wCountryID;
    INT32   iCodePage;
    DWORD   dwLangID;
    DWORD   dwSecurity;
} CONVCONTEXT32, *LPCONVCONTEXT32;

DECL_WINELIB_TYPE(CONVCONTEXT)
DECL_WINELIB_TYPE(LPCONVCONTEXT)

UINT16    WINAPI DdeInitialize16(LPDWORD,PFNCALLBACK16,DWORD,DWORD);
UINT32    WINAPI DdeInitialize32A(LPDWORD,PFNCALLBACK32,DWORD,DWORD);
UINT32    WINAPI DdeInitialize32W(LPDWORD,PFNCALLBACK32,DWORD,DWORD);
#define   DdeInitialize WINELIB_NAME_AW(DdeInitialize)
BOOL16    WINAPI DdeUninitialize16(DWORD);
BOOL32    WINAPI DdeUninitialize32(DWORD);
#define   DdeUninitialize WINELIB_NAME(DdeUninitialize)
HCONVLIST WINAPI DdeConnectList16(DWORD,HSZ,HSZ,HCONVLIST,LPCONVCONTEXT16);
HCONVLIST WINAPI DdeConnectList32(DWORD,HSZ,HSZ,HCONVLIST,LPCONVCONTEXT32);
#define   DdeConnectList WINELIB_NAME(DdeConnectList)
HCONV     WINAPI DdeQueryNextServer16(HCONVLIST, HCONV);
HCONV     WINAPI DdeQueryNextServer32(HCONVLIST, HCONV);
#define   DdeQueryNextServer WINELIB_NAME(DdeQueryNextServer)
BOOL16    WINAPI DdeDisconnectList16(HCONVLIST);
BOOL32    WINAPI DdeDisconnectList32(HCONVLIST);
#define   DdeDisConnectList WINELIB_NAME(DdeDisconnectList)
HCONV     WINAPI DdeConnect16(DWORD,HSZ,HSZ,LPCONVCONTEXT16);
HCONV     WINAPI DdeConnect32(DWORD,HSZ,HSZ,LPCONVCONTEXT32);
#define   DdeConnect WINELIB_NAME(DdeConnect)
BOOL16    WINAPI DdeDisconnect16(HCONV);
BOOL32    WINAPI DdeDisconnect32(HCONV);
#define   DdeDisconnect WINELIB_NAME(DdeDisconnect)
BOOL16    WINAPI DdeSetUserHandle(HCONV,DWORD,DWORD);
HDDEDATA  WINAPI DdeCreateHandleData(DWORD,LPBYTE,DWORD,DWORD,HSZ,UINT16,UINT16);
HCONV     WINAPI DdeReconnect(HCONV);
HSZ       WINAPI DdeCreateStringHandle16(DWORD,LPCSTR,INT16);
HSZ       WINAPI DdeCreateStringHandle32A(DWORD,LPCSTR,INT32);
HSZ       WINAPI DdeCreateStringHandle32W(DWORD,LPCWSTR,INT32);
#define   DdeCreateStringHandle WINELIB_NAME_AW(DdeCreateStringHandle)
BOOL16    WINAPI DdeFreeStringHandle16(DWORD,HSZ);
BOOL32    WINAPI DdeFreeStringHandle32(DWORD,HSZ);
#define   DdeFreeStringHandle WINELIB_NAME(DdeFreeStringHandle)
BOOL16    WINAPI DdeFreeDataHandle16(HDDEDATA);
BOOL32    WINAPI DdeFreeDataHandle32(HDDEDATA);
#define   DdeFreeDataHandle WINELIB_NAME(DdeFreeDataHandle)
BOOL16    WINAPI DdeKeepStringHandle16(DWORD,HSZ);
BOOL32    WINAPI DdeKeepStringHandle32(DWORD,HSZ);
#define   DdeKeepStringHandle WINELIB_NAME(DdeKeepStringHandle)
HDDEDATA  WINAPI DdeClientTransaction16(LPVOID,DWORD,HCONV,HSZ,UINT16,
                                        UINT16,DWORD,LPDWORD);
HDDEDATA  WINAPI DdeClientTransaction32(LPBYTE,DWORD,HCONV,HSZ,UINT32,
                                        UINT32,DWORD,LPDWORD);
#define   DdeClientTransaction WINELIB_NAME(DdeClientTransaction)
BOOL16    WINAPI DdeAbandonTransaction(DWORD,HCONV,DWORD);
BOOL16    WINAPI DdePostAdvise16(DWORD,HSZ,HSZ);
BOOL32    WINAPI DdePostAdvise32(DWORD,HSZ,HSZ);
#define   DdePostAdvise WINELIB_NAME(DdePostAdvise)
HDDEDATA  WINAPI DdeAddData(HDDEDATA,LPBYTE,DWORD,DWORD);
DWORD     WINAPI DdeGetData(HDDEDATA,LPBYTE,DWORD,DWORD);
LPBYTE    WINAPI DdeAccessData(HDDEDATA,LPDWORD);
BOOL16    WINAPI DdeUnaccessData(HDDEDATA);
BOOL16    WINAPI DdeEnableCallback(DWORD,HCONV,UINT16);
int       WINAPI DdeCmpStringHandles(HSZ,HSZ);


HDDEDATA  WINAPI DdeNameService16(DWORD,HSZ,HSZ,UINT16);
HDDEDATA  WINAPI DdeNameService32(DWORD,HSZ,HSZ,UINT32);
#define   DdeNameService WINELIB_NAME(DdeNameService)
UINT16    WINAPI DdeGetLastError16(DWORD);
UINT32    WINAPI DdeGetLastError32(DWORD);
#define   DdeGetLastError WINELIB_NAME(DdeGetLastError)

#endif  /* __WINE__DDEML_H */
