/*
 * DDEML library
 *
 * Copyright 1997 Alexandre Julliard
 */

/* Only empty stubs for now */

#include <stdio.h>
#include "ddeml.h"
#include "stddebug.h"
#include "debug.h"


/*****************************************************************
 *            DdeInitialize16   (DDEML.2)
 */
UINT16 WINAPI DdeInitialize16( LPDWORD pidInst, PFNCALLBACK16 pfnCallback,
                               DWORD afCmd, DWORD ulRes)
{
    fprintf( stdnimp, "DdeInitialize16: empty stub\n" );
    return 0;
}


/*****************************************************************
 *            DdeInitialize32A   (USER32.106)
 */
UINT32 WINAPI DdeInitialize32A( LPDWORD pidInst, PFNCALLBACK32 pfnCallback,
                                DWORD afCmd, DWORD ulRes )
{
    fprintf( stdnimp, "DdeInitialize32A: empty stub\n" );
    return 0;
}


/*****************************************************************
 *            DdeInitialize32W   (USER32.107)
 */
UINT32 WINAPI DdeInitialize32W( LPDWORD pidInst, PFNCALLBACK32 pfnCallback,
                                DWORD afCmd, DWORD ulRes )
{
    fprintf( stdnimp, "DdeInitialize32W: empty stub\n" );
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
    fprintf( stdnimp, "DdeUninitialize: empty stub\n" );
    return TRUE;
}


/*****************************************************************
 *            DdeConnect16   (DDEML.7)
 */
HCONV WINAPI DdeConnect16( DWORD idInst, HSZ hszService, HSZ hszTopic,
                           LPCONVCONTEXT16 pCC )
{
    fprintf( stdnimp, "DdeConnect16: empty stub\n" );
    return 0;
}


/*****************************************************************
 *            DdeConnect32   (USER32.92)
 */
HCONV WINAPI DdeConnect32( DWORD idInst, HSZ hszService, HSZ hszTopic,
                           LPCONVCONTEXT32 pCC )
{
    fprintf( stdnimp, "DdeConnect32: empty stub\n" );
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
 *            DdeDisconnect32   (USER32.97)
 */
BOOL32 WINAPI DdeDisconnect32( HCONV hConv )
{
    fprintf( stdnimp, "DdeDisconnect: empty stub\n" );
    return 0;
}


/*****************************************************************
 *            DdeReconnect   (DDEML.37) (USER32.115)
 */
HCONV WINAPI DdeReconnect( HCONV hConv )
{
    fprintf( stdnimp, "DdeReconnect: empty stub\n" );
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
    fprintf( stdnimp, "DdeCreateStringHandle32A: empty stub\n" );
    return 0;
}


/*****************************************************************
 *            DdeCreateStringHandle32W   (USER32.96)
 */
HSZ WINAPI DdeCreateStringHandle32W( DWORD idInst, LPCWSTR psz, INT32 codepage)
{
    fprintf( stdnimp, "DdeCreateStringHandle32W: empty stub\n" );
    return 0;
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
    fprintf( stdnimp, "DdeFreeStringHandle: empty stub\n" );
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
    fprintf( stdnimp, "DdeFreeDataHandle: empty stub\n" );
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
    fprintf( stdnimp, "DdeKeepStringHandle: empty stub\n" );
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
    fprintf( stdnimp, "DdeClientTransaction: empty stub\n" );
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
    fprintf( stdnimp, "DdeNameService: empty stub\n" );
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
    fprintf( stdnimp, "DdeGetLastError: empty stub\n" );
    return 0;
}

