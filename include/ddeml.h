/*
 * DDEML library definitions
 *
 * Copyright 1997 Alexandre Julliard
 * Copyright 1997 Len White
 */

#ifndef __WINE__DDEML_H
#define __WINE__DDEML_H

#include "wintypes.h"

/* Codepage Constants
 */
#define CP_WINANSI      1004
#define CP_WINUNICODE   1200

#define MSGF_DDEMGR 0x8001
/***************************************************

      FLAGS Section - copied from Microsoft SDK as must be standard, probably Copyright Microsoft Corporation

***************************************************/

/*
 * Callback filter flags for use with standard apps.
 */

#define     CBF_FAIL_SELFCONNECTIONS     0x00001000
#define     CBF_FAIL_CONNECTIONS         0x00002000
#define     CBF_FAIL_ADVISES             0x00004000
#define     CBF_FAIL_EXECUTES            0x00008000
#define     CBF_FAIL_POKES               0x00010000
#define     CBF_FAIL_REQUESTS            0x00020000
#define     CBF_FAIL_ALLSVRXACTIONS      0x0003f000

#define     CBF_SKIP_CONNECT_CONFIRMS    0x00040000
#define     CBF_SKIP_REGISTRATIONS       0x00080000
#define     CBF_SKIP_UNREGISTRATIONS     0x00100000
#define     CBF_SKIP_DISCONNECTS         0x00200000
#define     CBF_SKIP_ALLNOTIFICATIONS    0x003c0000

/*
 * Application command flags
 */
#define     APPCMD_CLIENTONLY            0x00000010L
#define     APPCMD_FILTERINITS           0x00000020L
#define     APPCMD_MASK                  0x00000FF0L

/*
 * Application classification flags
 */

#define     APPCLASS_STANDARD            0x00000000L
#define     APPCLASS_MONITOR             0x00000001L
#define     APPCLASS_MASK                0x0000000FL

/*
 * Callback filter flags for use with MONITOR apps - 0 implies no monitor
 * callbacks.
 */
#define     MF_HSZ_INFO                  0x01000000
#define     MF_SENDMSGS                  0x02000000
#define     MF_POSTMSGS                  0x04000000
#define     MF_CALLBACKS                 0x08000000
#define     MF_ERRORS                    0x10000000
#define     MF_LINKS                     0x20000000
#define     MF_CONV                      0x40000000

#define     MF_MASK                      0xFF000000

/*
 *	DdeNameService service name flags
 */

#define     DNS_REGISTER		 0x0001
#define     DNS_UNREGISTER		 0x0002
#define     DNS_FILTERON		 0x0004
#define     DNS_FILTEROFF         	 0x0008


/****************************************************

      End of Flags section

****************************************************/

/****************************************************

      Return Codes section again copied from SDK as must be same

*****************************************************/

#define     DMLERR_NO_ERROR                    0       /* must be 0 */

#define     DMLERR_FIRST                       0x4000

#define     DMLERR_ADVACKTIMEOUT               0x4000
#define     DMLERR_BUSY                        0x4001
#define     DMLERR_DATAACKTIMEOUT              0x4002
#define     DMLERR_DLL_NOT_INITIALIZED         0x4003
#define     DMLERR_DLL_USAGE                   0x4004
#define     DMLERR_EXECACKTIMEOUT              0x4005
#define     DMLERR_INVALIDPARAMETER            0x4006
#define     DMLERR_LOW_MEMORY                  0x4007
#define     DMLERR_MEMORY_ERROR                0x4008
#define     DMLERR_NOTPROCESSED                0x4009
#define     DMLERR_NO_CONV_ESTABLISHED         0x400a
#define     DMLERR_POKEACKTIMEOUT              0x400b
#define     DMLERR_POSTMSG_FAILED              0x400c
#define     DMLERR_REENTRANCY                  0x400d
#define     DMLERR_SERVER_DIED                 0x400e
#define     DMLERR_SYS_ERROR                   0x400f
#define     DMLERR_UNADVACKTIMEOUT             0x4010
#define     DMLERR_UNFOUND_QUEUE_ID            0x4011

#define     DMLERR_LAST                        0x4011

/*****************************************************

      End of Return Codes and Microsoft section

******************************************************/



typedef DWORD HCONVLIST;
typedef DWORD HCONV;
typedef DWORD HSZ;
typedef DWORD HDDEDATA;
typedef CHAR *LPTSTR;

typedef HDDEDATA (CALLBACK *PFNCALLBACK16)(UINT16,UINT16,HCONV,HSZ,HSZ,
                                           HDDEDATA,DWORD,DWORD);
typedef HDDEDATA (CALLBACK *PFNCALLBACK32)(UINT32,UINT32,HCONV,HSZ,HSZ,
                                           HDDEDATA,DWORD,DWORD);
DECL_WINELIB_TYPE(PFNCALLBACK)

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
    UINT32  cb;
    UINT32  wFlags;
    UINT32  wCountryID;
    INT32   iCodePage;
    DWORD   dwLangID;
    DWORD   dwSecurity;
} CONVCONTEXT32, *LPCONVCONTEXT32;

//  Internal data structures

      /*  entry for handle table     */
typedef struct DDE_HANDLE_ENTRY {
    BOOL16              Monitor;        // have these two as full Booleans cos they'll be tested frequently
    BOOL16              Client_only;    // bit wasteful of space but it will be faster
    BOOL16		Unicode;	/* Flag to indicate Win32 API used to initialise */
    BOOL16		Win16;		/* flag to indicate Win16 API used to initialize */
    LPDWORD            	Instance_id;  // needed to track monitor usage
    struct DDE_HANDLE_ENTRY    *Next_Entry;
    PFNCALLBACK32	CallBack;
    DWORD               CBF_Flags;
    DWORD               Monitor_flags;
    UINT32              Txn_count;      // count transactions open to simplify closure
} DDE_HANDLE_ENTRY;

//            Interface Definitions

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
DWORD     WINAPI DdeQueryString32A(DWORD, HSZ, LPSTR, DWORD, INT32);
DWORD     WINAPI DdeQueryString32W(DWORD, HSZ, LPWSTR, DWORD, INT32);
#define   DdeQueryString WINELIB_NAME_AW(DdeQueryString)
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
HDDEDATA  WINAPI DdeCreateDataHandle16(DWORD,LPBYTE,DWORD,DWORD,HSZ,UINT16,UINT16);
HDDEDATA  WINAPI DdeCreateDataHandle32(DWORD,LPBYTE,DWORD,DWORD,HSZ,UINT32,UINT32);
#define   DdeCreateDataHandle WINELIB_NAME(DdeCreateDataHandle)
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
LPBYTE    WINAPI DdeAccessData16(HDDEDATA,LPDWORD);
LPBYTE    WINAPI DdeAccessData32(HDDEDATA,LPDWORD);
#define   DdeAccessData WINELIB_NAME(DdeAccessData)
BOOL16    WINAPI DdeUnaccessData16(HDDEDATA);
BOOL32    WINAPI DdeUnaccessData32(HDDEDATA);
#define   DdeUnaccessData WINELIB_NAME(DdeUnaccessData)
BOOL16    WINAPI DdeEnableCallback16(DWORD,HCONV,UINT16);
BOOL32    WINAPI DdeEnableCallback32(DWORD,HCONV,UINT32);
#define   DdeEnableCallback WINELIB_NAME(DdeEnableCallback)
int       WINAPI DdeCmpStringHandles16(HSZ,HSZ);
int       WINAPI DdeCmpStringHandles32(HSZ,HSZ);
#define   DdeCmpStringHandles WINELIB_NAME(DdeCmpStringHandles)

HDDEDATA  WINAPI DdeNameService16(DWORD,HSZ,HSZ,UINT16);
HDDEDATA  WINAPI DdeNameService32(DWORD,HSZ,HSZ,UINT32);
#define   DdeNameService WINELIB_NAME(DdeNameService)
UINT16    WINAPI DdeGetLastError16(DWORD);
UINT32    WINAPI DdeGetLastError32(DWORD);
#define   DdeGetLastError WINELIB_NAME(DdeGetLastError)

#endif  /* __WINE__DDEML_H */
