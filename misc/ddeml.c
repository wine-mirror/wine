/*
 * DDEML library
 *
 * Copyright 1997 Alexandre Julliard
 * Copyright 1997 Len White
 */

/* Only empty stubs for now */

#include <stdio.h>
#include "ddeml.h"
#include "debug.h"


static LONG     DDE_current_handle;


/*****************************************************************
 *            DdeInitialize16   (DDEML.2)
 */
UINT16 WINAPI DdeInitialize16( LPDWORD pidInst, PFNCALLBACK16 pfnCallback,
                               DWORD afCmd, DWORD ulRes)
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}


/*****************************************************************
 *            DdeInitialize32A   (USER32.106)
 */
UINT32 WINAPI DdeInitialize32A( LPDWORD pidInst, PFNCALLBACK32 pfnCallback,
                                DWORD afCmd, DWORD ulRes )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}


/*****************************************************************
 *            DdeInitialize32W   (USER32.107)
 */
UINT32 WINAPI DdeInitialize32W( LPDWORD pidInst, PFNCALLBACK32 pfnCallback,
                                DWORD afCmd, DWORD ulRes )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}


/*****************************************************************
 *            DdeUninitialize16   (DDEML.3)
 */
BOOL16 WINAPI DdeUninitialize16( DWORD idInst )
{
    return (BOOL16)DdeUninitialize32( idInst );
}


/*****************************************************************
 *            DdeUninitialize32   (USER32.119)
 */
BOOL32 WINAPI DdeUninitialize32( DWORD idInst )
{
    FIXME( ddeml, "empty stub\n" );
    return TRUE;
}

/*****************************************************************
 *            DdeConnectList (DDEML.4)
 */
HCONVLIST WINAPI DdeConnectList( DWORD idInst, HSZ hszService, HSZ hszTopic,
        HCONVLIST hConvList, LPCONVCONTEXT16 pCC )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}

/*****************************************************************
 *            DdeQueryNextServer (DDEML.5)
 */
HCONV WINAPI DdeQueryNextServer( HCONVLIST hConvList, HCONV hConvPrev )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}

/*****************************************************************
 *            DdeDisconnectList (DDEML.6)
 */
BOOL16 WINAPI DdeDisconnectList( HCONVLIST hConvList )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}

/*****************************************************************
 *            DdeConnect16   (DDEML.7)
 */
HCONV WINAPI DdeConnect16( DWORD idInst, HSZ hszService, HSZ hszTopic,
                           LPCONVCONTEXT16 pCC )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}


/*****************************************************************
 *            DdeConnect32   (USER32.92)
 */
HCONV WINAPI DdeConnect32( DWORD idInst, HSZ hszService, HSZ hszTopic,
                           LPCONVCONTEXT32 pCC )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}


/*****************************************************************
 *            DdeDisconnect16   (DDEML.8)
 */
BOOL16 WINAPI DdeDisconnect16( HCONV hConv )
{
    return (BOOL16)DdeDisconnect32( hConv );
}

/*****************************************************************
 *            DdeSetUserHandle (DDEML.10)
 */
BOOL16 WINAPI DdeSetUserHandle( HCONV hConv, DWORD id, DWORD hUser )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}

/*****************************************************************
 *            DdeCreateDataHandle (DDEML.14)
 */
HDDEDATA WINAPI DdeCreateDataHandle( DWORD idInst, LPBYTE pSrc, DWORD cb, DWORD cbOff, HSZ hszItem, UINT16 wFmt, UINT16 afCmd )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}

/*****************************************************************
 *            DdeDisconnect32   (USER32.97)
 */
BOOL32 WINAPI DdeDisconnect32( HCONV hConv )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}


/*****************************************************************
 *            DdeReconnect   (DDEML.37) (USER32.115)
 */
HCONV WINAPI DdeReconnect( HCONV hConv )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}


/*****************************************************************
 *            DdeCreateStringHandle16   (DDEML.21)
 */
HSZ WINAPI DdeCreateStringHandle16( DWORD idInst, LPCSTR str, INT16 codepage )
{
    return DdeCreateStringHandle32A( idInst, str, codepage );
}


/*****************************************************************
 *            DdeCreateStringHandle32A   (USER32.95)
 */
HSZ WINAPI DdeCreateStringHandle32A( DWORD idInst, LPCSTR psz, INT32 codepage )
{
    FIXME( ddeml, "empty stub\n" );
    DDE_current_handle++;
    return DDE_current_handle;
}


/*****************************************************************
 *            DdeCreateStringHandle32W   (USER32.96)
 */
HSZ WINAPI DdeCreateStringHandle32W( DWORD idInst, LPCWSTR psz, INT32 codepage)
{
    FIXME( ddeml, "empty stub\n" );
    DDE_current_handle++;
    return DDE_current_handle;
}


/*****************************************************************
 *            DdeFreeStringHandle16   (DDEML.22)
 */
BOOL16 WINAPI DdeFreeStringHandle16( DWORD idInst, HSZ hsz )
{
    return (BOOL32)DdeFreeStringHandle32( idInst, hsz );
}


/*****************************************************************
 *            DdeFreeStringHandle32   (USER32.101)
 */
BOOL32 WINAPI DdeFreeStringHandle32( DWORD idInst, HSZ hsz )
{
    FIXME( ddeml, "empty stub\n" );
    return TRUE;
}


/*****************************************************************
 *            DdeFreeDataHandle16   (DDEML.19)
 */
BOOL16 WINAPI DdeFreeDataHandle16( HDDEDATA hData )
{
    return (BOOL32)DdeFreeDataHandle32( hData );
}


/*****************************************************************
 *            DdeFreeDataHandle32   (USER32.100)
 */
BOOL32 WINAPI DdeFreeDataHandle32( HDDEDATA hData )
{
    FIXME( ddeml, "empty stub\n" );
    return TRUE;
}


/*****************************************************************
 *            DdeKeepStringHandle16   (DDEML.24)
 */
BOOL16 WINAPI DdeKeepStringHandle16( DWORD idInst, HSZ hsz )
{
    return (BOOL32)DdeKeepStringHandle32( idInst, hsz );
}


/*****************************************************************
 *            DdeKeepStringHandle32  (USER32.108)
 */
BOOL32 WINAPI DdeKeepStringHandle32( DWORD idInst, HSZ hsz )
{
    FIXME( ddeml, "empty stub\n" );
    return TRUE;
}


/*****************************************************************
 *            DdeClientTransaction16  (DDEML.11)
 */
HDDEDATA WINAPI DdeClientTransaction16( LPVOID pData, DWORD cbData,
                                        HCONV hConv, HSZ hszItem, UINT16 wFmt,
                                        UINT16 wType, DWORD dwTimeout,
                                        LPDWORD pdwResult )
{
    return DdeClientTransaction32( (LPBYTE)pData, cbData, hConv, hszItem,
                                   wFmt, wType, dwTimeout, pdwResult );
}


/*****************************************************************
 *            DdeClientTransaction32  (USER32.90)
 */
HDDEDATA WINAPI DdeClientTransaction32( LPBYTE pData, DWORD cbData,
                                        HCONV hConv, HSZ hszItem, UINT32 wFmt,
                                        UINT32 wType, DWORD dwTimeout,
                                        LPDWORD pdwResult )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}

/*****************************************************************
 *            DdeAbandonTransaction (DDEML.12)
 */
BOOL16 WINAPI DdeAbandonTransaction( DWORD idInst, HCONV hConv, 
                                     DWORD idTransaction )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}

/*****************************************************************
 *            DdePostAdvise (DDEML.13)
 */
BOOL16 WINAPI DdePostAdvise( DWORD idInst, HSZ hszTopic, HSZ hszItem )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}

/*****************************************************************
 *            DdeAddData (DDEML.15)
 */
HDDEDATA WINAPI DdeAddData( HDDEDATA hData, LPBYTE pSrc, DWORD cb,
                            DWORD cbOff )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}

/*****************************************************************
 *            DdeGetData (DDEML.16)
 */
DWORD WINAPI DdeGetData( HDDEDATA hData, LPBYTE pDst, DWORD cbMax, 
                         DWORD cbOff )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}

/*****************************************************************
 *            DdeAccessData (DDEML.17)
 */
LPBYTE WINAPI DdeAccessData( HDDEDATA hData, LPDWORD pcbDataSize )
{
     FIXME( ddeml, "empty stub\n" );
     return 0;
}

/*****************************************************************
 *            DdeUnaccessData (DDEML.18)
 */
BOOL16 WINAPI DdeUnaccessData( HDDEDATA hData )
{
     FIXME( ddeml, "empty stub\n" );
     return 0;
}

/*****************************************************************
 *            DdeEnableCallback (DDEML.26)
 */
BOOL16 WINAPI DdeEnableCallback( DWORD idInst, HCONV hConv, UINT16 wCmd )
{
     FIXME( ddeml, "empty stub\n" );
     return 0;
}

/*****************************************************************
 *            DdeNameService16  (DDEML.27)
 */
HDDEDATA WINAPI DdeNameService16( DWORD idInst, HSZ hsz1, HSZ hsz2,
                                  UINT16 afCmd )
{
    return DdeNameService32( idInst, hsz1, hsz2, afCmd );
}


/*****************************************************************
 *            DdeNameService32  (USER32.109)
 */
HDDEDATA WINAPI DdeNameService32( DWORD idInst, HSZ hsz1, HSZ hsz2,
                                  UINT32 afCmd )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}


/*****************************************************************
 *            DdeGetLastError16  (DDEML.20)
 */
UINT16 WINAPI DdeGetLastError16( DWORD idInst )
{
    return (UINT16)DdeGetLastError32( idInst );
}


/*****************************************************************
 *            DdeGetLastError32  (USER32.103)
 */
UINT32 WINAPI DdeGetLastError32( DWORD idInst )
{
    FIXME( ddeml, "empty stub\n" );
    return 0;
}

/*****************************************************************
 *            DdeCmpStringHandles (DDEML.36)
 */
int WINAPI DdeCmpStringHandles( HSZ hsz1, HSZ hsz2 )
{
     FIXME( ddeml, "empty stub\n" );
     return 0;
}



