/*
 * DDEML library
 *
 * Copyright 1997 Alexandre Julliard
 * Copyright 1997 Len White
 * Copyright 1999 Keith Matthews
 * Copyright 2000 Corel
 * Copyright 2001,2002,2009 Eric Pouech
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */


#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wine/windef16.h"
#include "wine/winbase16.h"
#include "wownt32.h"
#include "dde.h"
#include "ddeml.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddeml);


typedef HDDEDATA (CALLBACK *PFNCALLBACK16)(UINT16,UINT16,HCONV,HSZ,HSZ,HDDEDATA,
                                           DWORD,DWORD);

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

static void map1632_conv_context(CONVCONTEXT* cc32, const CONVCONTEXT16* cc16)
{
    cc32->cb = sizeof(*cc32);
    cc32->wFlags = cc16->wFlags;
    cc32->wCountryID = cc16->wCountryID;
    cc32->iCodePage = cc16->iCodePage;
    cc32->dwLangID = cc16->dwLangID;
    cc32->dwSecurity = cc16->dwSecurity;
}

static void map3216_conv_context(CONVCONTEXT16* cc16, const CONVCONTEXT* cc32)
{
    cc16->cb = sizeof(*cc16);
    cc16->wFlags = cc32->wFlags;
    cc16->wCountryID = cc32->wCountryID;
    cc16->iCodePage = cc32->iCodePage;
    cc16->dwLangID = cc32->dwLangID;
    cc16->dwSecurity = cc32->dwSecurity;
}

/******************************************************************
 *		WDML_InvokeCallback16
 *
 *
 */
static HDDEDATA	CALLBACK WDML_InvokeCallback16(DWORD pfn16, UINT uType, UINT uFmt,
                                               HCONV hConv, HSZ hsz1, HSZ hsz2,
                                               HDDEDATA hdata, ULONG_PTR dwData1, ULONG_PTR dwData2)
{
    DWORD               d1 = 0;
    HDDEDATA            ret;
    CONVCONTEXT16       cc16;
    WORD args[16];

    switch (uType)
    {
    case XTYP_CONNECT:
    case XTYP_WILDCONNECT:
        if (dwData1)
        {
            map3216_conv_context(&cc16, (const CONVCONTEXT*)dwData1);
            d1 = MapLS(&cc16);
        }
        break;
    default:
        d1 = dwData1;
        break;
    }
    args[15] = HIWORD(uType);
    args[14] = LOWORD(uType);
    args[13] = HIWORD(uFmt);
    args[12] = LOWORD(uFmt);
    args[11] = HIWORD(hConv);
    args[10] = LOWORD(hConv);
    args[9]  = HIWORD(hsz1);
    args[8]  = LOWORD(hsz1);
    args[7]  = HIWORD(hsz2);
    args[6]  = LOWORD(hsz2);
    args[5]  = HIWORD(hdata);
    args[4]  = LOWORD(hdata);
    args[3]  = HIWORD(d1);
    args[2]  = LOWORD(d1);
    args[1]  = HIWORD(dwData2);
    args[0]  = LOWORD(dwData2);
    WOWCallback16Ex(pfn16, WCB16_PASCAL, sizeof(args), args, (DWORD *)&ret);

    switch (uType)
    {
    case XTYP_CONNECT:
    case XTYP_WILDCONNECT:
        if (d1 != 0) UnMapLS(d1);
        break;
    }
    return ret;
}

#define MAX_THUNKS      32
/* As DDEML doesn't provide a way to get back to an InstanceID when
 * a callback is run, we use thunk in order to implement simply the
 * 32bit->16bit callback mechanism.
 * For each 16bit instance, we create a thunk, which will be passed as
 * a 32bit callback. This thunk also stores (in the code!) the 16bit
 * address of the 16bit callback, and passes it back to
 * WDML_InvokeCallback16.
 * The code below is mainly to create the thunks themselves
 */
#pragma pack(push,1)
static struct ddeml_thunk
{
    BYTE        popl_eax;        /* popl  %eax (return address) */
    BYTE        pushl_func;      /* pushl $pfn16 (16bit callback function) */
    SEGPTR      pfn16;
    BYTE        pushl_eax;       /* pushl %eax */
    BYTE        jmp;             /* ljmp WDML_InvokeCallback16 */
    DWORD       callback;
    DWORD       instId;          /* instance ID */
} *DDEML16_Thunks;
#pragma pack(pop)

static CRITICAL_SECTION ddeml_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &ddeml_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": ddeml_cs") }
};
static CRITICAL_SECTION ddeml_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

static struct ddeml_thunk*      DDEML_AddThunk(DWORD instId, DWORD pfn16)
{
    struct ddeml_thunk* thunk;

    if (!DDEML16_Thunks)
    {
        DDEML16_Thunks = VirtualAlloc(NULL, MAX_THUNKS * sizeof(*DDEML16_Thunks), MEM_COMMIT,
                                      PAGE_EXECUTE_READWRITE);
        if (!DDEML16_Thunks) return NULL;
        for (thunk = DDEML16_Thunks; thunk < &DDEML16_Thunks[MAX_THUNKS]; thunk++)
        {
            thunk->popl_eax     = 0x58;   /* popl  %eax */
            thunk->pushl_func   = 0x68;   /* pushl $pfn16 */
            thunk->pfn16        = 0;
            thunk->pushl_eax    = 0x50;   /* pushl %eax */
            thunk->jmp          = 0xe9;   /* jmp WDML_InvokeCallback16 */
            thunk->callback     = (char *)WDML_InvokeCallback16 - (char *)(&thunk->callback + 1);
            thunk->instId       = 0;
        }
    }
    for (thunk = DDEML16_Thunks; thunk < &DDEML16_Thunks[MAX_THUNKS]; thunk++)
    {
        /* either instId is 0, and we're looking for an empty slot, or
         * instId is an already existing instance, and we should find its thunk
         */
        if (thunk->instId == instId)
        {
            thunk->pfn16 = pfn16;
            return thunk;
        }
    }
    FIXME("Out of ddeml-thunks. Bump MAX_THUNKS\n");
    return NULL;
}

/******************************************************************************
 *            DdeInitialize   (DDEML.2)
 */
UINT16 WINAPI DdeInitialize16(LPDWORD pidInst, PFNCALLBACK16 pfnCallback,
			      DWORD afCmd, DWORD ulRes)
{
    UINT16 ret;
    struct ddeml_thunk* thunk;

    EnterCriticalSection(&ddeml_cs);
    if ((thunk = DDEML_AddThunk(*pidInst, (DWORD)pfnCallback)))
    {
        ret = DdeInitializeA(pidInst, (PFNCALLBACK)thunk, afCmd, ulRes);
        if (ret == DMLERR_NO_ERROR) thunk->instId = *pidInst;
    }
    else ret = DMLERR_SYS_ERROR;
    LeaveCriticalSection(&ddeml_cs);
    return ret;
}

/*****************************************************************
 *            DdeUninitialize   (DDEML.3)
 */
BOOL16 WINAPI DdeUninitialize16(DWORD idInst)
{
    struct ddeml_thunk* thunk;
    BOOL16              ret = FALSE;

    if (!DdeUninitialize(idInst)) return FALSE;
    EnterCriticalSection(&ddeml_cs);
    for (thunk = DDEML16_Thunks; thunk < &DDEML16_Thunks[MAX_THUNKS]; thunk++)
    {
        if (thunk->instId == idInst)
        {
            thunk->instId = 0;
            ret = TRUE;
            break;
        }
    }
    LeaveCriticalSection(&ddeml_cs);
    if (!ret) FIXME("Should never happen\n");
    return ret;
}

/*****************************************************************
 * DdeConnectList [DDEML.4]
 */

HCONVLIST WINAPI DdeConnectList16(DWORD idInst, HSZ hszService, HSZ hszTopic,
				  HCONVLIST hConvList, LPCONVCONTEXT16 pCC16)
{
    CONVCONTEXT	        cc;
    CONVCONTEXT*	pCC = NULL;

    if (pCC16)
        map1632_conv_context(pCC = &cc, pCC16);
    return DdeConnectList(idInst, hszService, hszTopic, hConvList, pCC);
}

/*****************************************************************
 * DdeQueryNextServer [DDEML.5]
 */
HCONV WINAPI DdeQueryNextServer16(HCONVLIST hConvList, HCONV hConvPrev)
{
    return DdeQueryNextServer(hConvList, hConvPrev);
}

/*****************************************************************
 *            DdeDisconnectList (DDEML.6)
 */
BOOL16 WINAPI DdeDisconnectList16(HCONVLIST hConvList)
{
    return (BOOL16)DdeDisconnectList(hConvList);
}


/*****************************************************************
 *		DdeQueryString (DDEML.23)
 */
DWORD WINAPI DdeQueryString16(DWORD idInst, HSZ hsz, LPSTR lpsz, DWORD cchMax,
                              INT16 codepage)
{
    return DdeQueryStringA(idInst, hsz, lpsz, cchMax, codepage);
}

/*****************************************************************
 *            DdeConnect   (DDEML.7)
 */
HCONV WINAPI DdeConnect16(DWORD idInst, HSZ hszService, HSZ hszTopic,
                          LPCONVCONTEXT16 pCC16)
{
    CONVCONTEXT	        cc;
    CONVCONTEXT*	pCC = NULL;

    if (pCC16)
        map1632_conv_context(pCC = &cc, pCC16);
    return DdeConnect(idInst, hszService, hszTopic, pCC);
}

/*****************************************************************
 *            DdeDisconnect   (DDEML.8)
 */
BOOL16 WINAPI DdeDisconnect16(HCONV hConv)
{
    return (BOOL16)DdeDisconnect(hConv);
}

/*****************************************************************
 *            DdeSetUserHandle (DDEML.10)
 */
BOOL16 WINAPI DdeSetUserHandle16(HCONV hConv, DWORD id, DWORD hUser)
{
    return DdeSetUserHandle(hConv, id, hUser);
}

/*****************************************************************
 *            DdeCreateDataHandle (DDEML.14)
 */
HDDEDATA WINAPI DdeCreateDataHandle16(DWORD idInst, LPBYTE pSrc, DWORD cb,
                                      DWORD cbOff, HSZ hszItem, UINT16 wFmt,
				      UINT16 afCmd)
{
    return DdeCreateDataHandle(idInst, pSrc, cb, cbOff, hszItem, wFmt, afCmd);
}

/*****************************************************************
 *            DdeCreateStringHandle   (DDEML.21)
 */
HSZ WINAPI DdeCreateStringHandle16(DWORD idInst, LPCSTR str, INT16 codepage)
{
    if  (codepage)
    {
        return DdeCreateStringHandleA(idInst, str, codepage);
    }
    else
    {
        TRACE("Default codepage supplied\n");
        return DdeCreateStringHandleA(idInst, str, CP_WINANSI);
    }
}

/*****************************************************************
 *            DdeFreeStringHandle   (DDEML.22)
 */
BOOL16 WINAPI DdeFreeStringHandle16(DWORD idInst, HSZ hsz)
{
    return (BOOL16)DdeFreeStringHandle(idInst, hsz);
}

/*****************************************************************
 *            DdeFreeDataHandle   (DDEML.19)
 */
BOOL16 WINAPI DdeFreeDataHandle16(HDDEDATA hData)
{
    return (BOOL16)DdeFreeDataHandle(hData);
}

/*****************************************************************
 *            DdeKeepStringHandle   (DDEML.24)
 */
BOOL16 WINAPI DdeKeepStringHandle16(DWORD idInst, HSZ hsz)
{
    return DdeKeepStringHandle(idInst, hsz);
}

/*****************************************************************
 *            DdeClientTransaction  (DDEML.11)
 */
HDDEDATA WINAPI DdeClientTransaction16(LPVOID pData, DWORD cbData, HCONV hConv,
                                       HSZ hszItem, UINT16 wFmt, UINT16 wType,
                                       DWORD dwTimeout, LPDWORD pdwResult)
{
    if (cbData != (DWORD)-1)
    {
        /* pData is not a pointer if cbData is -1, so we linearize the address
         * here rather than in the calling code. */
        pData = MapSL((SEGPTR)pData);
    }
    return DdeClientTransaction(pData, cbData, hConv, hszItem,
                                wFmt, wType, dwTimeout, pdwResult);
}

/*****************************************************************
 *
 *            DdeAbandonTransaction (DDEML.12)
 *
 */
BOOL16 WINAPI DdeAbandonTransaction16(DWORD idInst, HCONV hConv, DWORD idTransaction)
{
    return (BOOL16)DdeAbandonTransaction(idInst, hConv, idTransaction);
}

/*****************************************************************
 * DdePostAdvise [DDEML.13]
 */
BOOL16 WINAPI DdePostAdvise16(DWORD idInst, HSZ hszTopic, HSZ hszItem)
{
    return (BOOL16)DdePostAdvise(idInst, hszTopic, hszItem);
}

/*****************************************************************
 *            DdeAddData (DDEML.15)
 */
HDDEDATA WINAPI DdeAddData16(HDDEDATA hData, LPBYTE pSrc, DWORD cb, DWORD cbOff)
{
    return DdeAddData(hData, pSrc, cb, cbOff);
}

/*****************************************************************
 * DdeGetData [DDEML.16]
 */
DWORD WINAPI DdeGetData16(HDDEDATA hData, LPBYTE pDst, DWORD cbMax, DWORD cbOff)
{
    return DdeGetData(hData, pDst, cbMax, cbOff);
}

/*****************************************************************
 *            DdeAccessData (DDEML.17)
 */
LPBYTE WINAPI DdeAccessData16(HDDEDATA hData, LPDWORD pcbDataSize)
{
    FIXME("expect trouble\n");
    /* FIXME: there's a memory leak here... */
    return (LPBYTE)MapLS(DdeAccessData(hData, pcbDataSize));
}

/*****************************************************************
 *            DdeUnaccessData (DDEML.18)
 */
BOOL16 WINAPI DdeUnaccessData16(HDDEDATA hData)
{
    return DdeUnaccessData(hData);
}

/*****************************************************************
 *            DdeEnableCallback (DDEML.26)
 */
BOOL16 WINAPI DdeEnableCallback16(DWORD idInst, HCONV hConv, UINT16 wCmd)
{
    return DdeEnableCallback(idInst, hConv, wCmd);
}

/*****************************************************************
 *            DdeNameService  (DDEML.27)
 */
HDDEDATA WINAPI DdeNameService16(DWORD idInst, HSZ hsz1, HSZ hsz2, UINT16 afCmd)
{
    return DdeNameService(idInst, hsz1, hsz2, afCmd);
}

/*****************************************************************
 *            DdeGetLastError  (DDEML.20)
 */
UINT16 WINAPI DdeGetLastError16(DWORD idInst)
{
    return (UINT16)DdeGetLastError(idInst);
}

/*****************************************************************
 *            DdeCmpStringHandles (DDEML.36)
 */
INT16 WINAPI DdeCmpStringHandles16(HSZ hsz1, HSZ hsz2)
{
    return DdeCmpStringHandles(hsz1, hsz2);
}

/******************************************************************
 *		DdeQueryConvInfo (DDEML.9)
 *
 */
UINT16 WINAPI DdeQueryConvInfo16(HCONV hConv, DWORD idTransaction,
                                 LPCONVINFO16 lpConvInfo)
{
    CONVINFO    ci32;
    CONVINFO16  ci16;
    UINT        ret;

    ci32.cb = sizeof(ci32);
    ci32.ConvCtxt.cb = sizeof(ci32.ConvCtxt);

    ret = DdeQueryConvInfo(hConv, idTransaction, &ci32);
    if (ret == 0) return 0;

    ci16.cb = lpConvInfo->cb;
    ci16.hUser = ci32.hUser;
    ci16.hConvPartner = ci32.hConvPartner;
    ci16.hszSvcPartner = ci32.hszSvcPartner;
    ci16.hszServiceReq = ci32.hszServiceReq;
    ci16.hszTopic = ci32.hszTopic;
    ci16.hszItem = ci32.hszItem;
    ci16.wFmt = ci32.wFmt;
    ci16.wType = ci32.wType;
    ci16.wStatus = ci32.wStatus;
    ci16.wConvst = ci32.wConvst;
    ci16.wLastError = ci32.wLastError;
    ci16.hConvList = ci32.hConvList;

    map3216_conv_context(&ci16.ConvCtxt, &ci32.ConvCtxt);

    memcpy(lpConvInfo, &ci16, lpConvInfo->cb);
    return lpConvInfo->cb;
}
