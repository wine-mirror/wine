/*
 * DDEML 16-bit library definitions
 *
 * Copyright 1997 Alexandre Julliard
 * Copyright 1997 Len White
 */

#ifndef __WINE_WINE_DDEML16_H
#define __WINE_WINE_DDEML16_H

#include "windef.h"
#include "wine/windef16.h"

#define QID_SYNC16  -1L

typedef HDDEDATA CALLBACK (*PFNCALLBACK16)(UINT16,UINT16,HCONV,HSZ,HSZ,HDDEDATA,DWORD,DWORD);

/***************************************************

	Externally visible data structures

***************************************************/

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
    DWORD          cb;
    DWORD          hUser;
    HCONV          hConvPartner;
    HSZ            hszSvcPartner;
    HSZ            hszServiceReq;
    HSZ            hszTopic;
    HSZ            hszItem;
    UINT16         wFmt;
    UINT16         wType;
    UINT16         wStatus;
    UINT16         wConvst;
    UINT16         wLastError;
    HCONVLIST      hConvList;
    CONVCONTEXT16  ConvCtxt;
} CONVINFO16, *LPCONVINFO16;

/*            Interface Definitions		*/

UINT16    WINAPI DdeInitialize16(LPDWORD,PFNCALLBACK16,DWORD,DWORD);
BOOL16    WINAPI DdeUninitialize16(DWORD);
HCONVLIST WINAPI DdeConnectList16(DWORD,HSZ,HSZ,HCONVLIST,LPCONVCONTEXT16);
HCONV     WINAPI DdeQueryNextServer16(HCONVLIST, HCONV);
BOOL16    WINAPI DdeDisconnectList16(HCONVLIST);
HCONV     WINAPI DdeConnect16(DWORD,HSZ,HSZ,LPCONVCONTEXT16);
BOOL16    WINAPI DdeDisconnect16(HCONV);
BOOL16    WINAPI DdeSetUserHandle16(HCONV,DWORD,DWORD);
HDDEDATA  WINAPI DdeCreateDataHandle16(DWORD,LPBYTE,DWORD,DWORD,HSZ,UINT16,UINT16);
HSZ       WINAPI DdeCreateStringHandle16(DWORD,LPCSTR,INT16);
BOOL16    WINAPI DdeFreeStringHandle16(DWORD,HSZ);
BOOL16    WINAPI DdeFreeDataHandle16(HDDEDATA);
BOOL16    WINAPI DdeKeepStringHandle16(DWORD,HSZ);
HDDEDATA  WINAPI DdeClientTransaction16(LPVOID,DWORD,HCONV,HSZ,UINT16,UINT16,DWORD,LPDWORD);
BOOL16    WINAPI DdeAbandonTransaction16(DWORD,HCONV,DWORD);
BOOL16    WINAPI DdePostAdvise16(DWORD,HSZ,HSZ);
HDDEDATA  WINAPI DdeAddData16(HDDEDATA,LPBYTE,DWORD,DWORD);
LPBYTE    WINAPI DdeAccessData16(HDDEDATA,LPDWORD);
BOOL16    WINAPI DdeUnaccessData16(HDDEDATA);
BOOL16    WINAPI DdeEnableCallback16(DWORD,HCONV,UINT16);
INT16     WINAPI DdeCmpStringHandles16(HSZ,HSZ);
HDDEDATA  WINAPI DdeNameService16(DWORD,HSZ,HSZ,UINT16);
UINT16    WINAPI DdeGetLastError16(DWORD);
UINT16    WINAPI DdeQueryConvInfo16(HCONV,DWORD,LPCONVINFO16);

#endif  /* __WINE_WINE_DDEML16_H */
