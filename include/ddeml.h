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

/* DDE synchronisation constants 
 */

#define MSGF_DDEMGR 0x8001

#define QID_SYNC16	-1L
#define QID_SYNC	0xFFFFFFFF

/*   Type variation for MS  deliberate departures from ANSI standards
 */

#define EXPENTRY CALLBACK

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

#define     CBR_BLOCK			 0xFFFFFFFFL

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

	Message Types Section

****************************************************/

#define XTYPF_NOBLOCK		0x0002		/* CBR_NOBLOCK will not work */
#define XTYPF_NODATA		0x0004		/* DDE_FDEFERUPD  */
#define XTYPF_ACKREQ		0x0008		/* DDE_FACKREQ */

#define XCLASS_MASK		0xFC00
#define XCLASS_BOOL		0x1000
#define XCLASS_DATA		0x2000
#define XCLASS_FLAGS		0x4000
#define	XCLASS_NOTIFICATION	0x8000

#define XTYP_ADVDATA		(0x0010 | XCLASS_FLAGS)
#define XTYP_XACT_COMPLETE	(0x0080 | XCLASS_NOTIFICATION )
#define XTYP_REGISTER		(0x00A0 | XCLASS_NOTIFICATION | XTYPF_NOBLOCK )
#define XTYP_REQUEST		(0x00B0 | XCLASS_DATA )
#define XTYP_DISCONNECT		(0x00C0 | XCLASS_NOTIFICATION | XTYPF_NOBLOCK )
#define XTYP_UNREGISTER		(0x00D0 | XCLASS_NOTIFICATION | XTYPF_NOBLOCK )

/**************************************************

	End of Message Types Section

****************************************************/

/*****************************************************

	DDE Codes for wStatus field

*****************************************************/

#define DDE_FACK		0x8000
#define DDE_FBUSY		0x4000
#define DDE_FDEFERUPD		0x4000
#define DDE_FACKREQ		0x8000
#define DDE_FRELEASE		0x2000
#define DDE_FREQUESTED		0x1000
#define DDE_FAPPSTATUS		0x00FF
#define DDE_FNOTPROCESSED	0x0000


/*****************************************************

	End of wStatus codes

*****************************************************/

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


/*******************************************************

	API Entry Points

*******************************************************/

typedef HDDEDATA (CALLBACK *PFNCALLBACK16)(UINT16,UINT16,HCONV,HSZ,HSZ,
                                           HDDEDATA,DWORD,DWORD);
typedef HDDEDATA (CALLBACK *PFNCALLBACK)(UINT,UINT,HCONV,HSZ,HSZ,
                                           HDDEDATA,DWORD,DWORD);

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
    UINT  cb;
    UINT  wFlags;
    UINT  wCountryID;
    INT   iCodePage;
    DWORD   dwLangID;
    DWORD   dwSecurity;
} CONVCONTEXT, *LPCONVCONTEXT;

typedef struct
{
    DWORD		cb;
    DWORD 		hUser;
    HCONV		hConvPartner;
    HSZ			hszSvcPartner;
    HSZ			hszServiceReq;
    HSZ			hszTopic;
    HSZ			hszItem;
    UINT16		wFmt;
    UINT16		wType;
    UINT16		wStatus;
    UINT16		wConvst;
    UINT16		wLastError;
    HCONVLIST		hConvList;
    CONVCONTEXT16	ConvCtxt;
} CONVINFO16, *LPCONVINFO16;

typedef struct
{
    DWORD		cb;
    DWORD 		hUser;
    HCONV		hConvPartner;
    HSZ			hszSvcPartner;
    HSZ			hszServiceReq;
    HSZ			hszTopic;
    HSZ			hszItem;
    UINT		wFmt;
    UINT		wType;
    UINT		wStatus;
    UINT		wConvst;
    UINT		wLastError;
    HCONVLIST		hConvList;
    CONVCONTEXT		ConvCtxt;
    HWND		hwnd;
    HWND		hwndPartner;
} CONVINFO, *LPCONVINFO;
//  Internal data structures

      /*  entry for handle table     */
typedef struct DDE_HANDLE_ENTRY {
    BOOL16              Monitor;        // have these two as full Booleans cos they'll be tested frequently
    BOOL16              Client_only;    // bit wasteful of space but it will be faster
    BOOL16		Unicode;	/* Flag to indicate Win32 API used to initialise */
    BOOL16		Win16;		/* flag to indicate Win16 API used to initialize */
    DWORD            	Instance_id;  // needed to track monitor usage
    struct DDE_HANDLE_ENTRY    *Next_Entry;
    PFNCALLBACK	CallBack;
    DWORD               CBF_Flags;
    DWORD               Monitor_flags;
    UINT              Txn_count;      // count transactions open to simplify closure
} DDE_HANDLE_ENTRY;

//            Interface Definitions


UINT16    WINAPI DdeInitialize16(LPDWORD,PFNCALLBACK16,DWORD,DWORD);
UINT    WINAPI DdeInitializeA(LPDWORD,PFNCALLBACK,DWORD,DWORD);
UINT    WINAPI DdeInitializeW(LPDWORD,PFNCALLBACK,DWORD,DWORD);
#define   DdeInitialize WINELIB_NAME_AW(DdeInitialize)
BOOL16    WINAPI DdeUninitialize16(DWORD);
BOOL    WINAPI DdeUninitialize(DWORD);
HCONVLIST WINAPI DdeConnectList16(DWORD,HSZ,HSZ,HCONVLIST,LPCONVCONTEXT16);
HCONVLIST WINAPI DdeConnectList(DWORD,HSZ,HSZ,HCONVLIST,LPCONVCONTEXT);
HCONV     WINAPI DdeQueryNextServer16(HCONVLIST, HCONV);
HCONV     WINAPI DdeQueryNextServer(HCONVLIST, HCONV);
DWORD     WINAPI DdeQueryStringA(DWORD, HSZ, LPSTR, DWORD, INT);
DWORD     WINAPI DdeQueryStringW(DWORD, HSZ, LPWSTR, DWORD, INT);
#define   DdeQueryString WINELIB_NAME_AW(DdeQueryString)
BOOL16    WINAPI DdeDisconnectList16(HCONVLIST);
BOOL    WINAPI DdeDisconnectList(HCONVLIST);
HCONV     WINAPI DdeConnect16(DWORD,HSZ,HSZ,LPCONVCONTEXT16);
HCONV     WINAPI DdeConnect(DWORD,HSZ,HSZ,LPCONVCONTEXT);
BOOL16    WINAPI DdeDisconnect16(HCONV);
BOOL    WINAPI DdeDisconnect(HCONV);
BOOL16    WINAPI DdeSetUserHandle16(HCONV,DWORD,DWORD);
HDDEDATA  WINAPI DdeCreateDataHandle16(DWORD,LPBYTE,DWORD,DWORD,HSZ,UINT16,UINT16);
HDDEDATA  WINAPI DdeCreateDataHandle(DWORD,LPBYTE,DWORD,DWORD,HSZ,UINT,UINT);
HCONV     WINAPI DdeReconnect(HCONV);
HSZ       WINAPI DdeCreateStringHandle16(DWORD,LPCSTR,INT16);
HSZ       WINAPI DdeCreateStringHandleA(DWORD,LPCSTR,INT);
HSZ       WINAPI DdeCreateStringHandleW(DWORD,LPCWSTR,INT);
#define   DdeCreateStringHandle WINELIB_NAME_AW(DdeCreateStringHandle)
BOOL16    WINAPI DdeFreeStringHandle16(DWORD,HSZ);
BOOL    WINAPI DdeFreeStringHandle(DWORD,HSZ);
BOOL16    WINAPI DdeFreeDataHandle16(HDDEDATA);
BOOL    WINAPI DdeFreeDataHandle(HDDEDATA);
BOOL16    WINAPI DdeKeepStringHandle16(DWORD,HSZ);
BOOL    WINAPI DdeKeepStringHandle(DWORD,HSZ);
HDDEDATA  WINAPI DdeClientTransaction16(LPVOID,DWORD,HCONV,HSZ,UINT16,
                                        UINT16,DWORD,LPDWORD);
HDDEDATA  WINAPI DdeClientTransaction(LPBYTE,DWORD,HCONV,HSZ,UINT,
                                        UINT,DWORD,LPDWORD);
BOOL16    WINAPI DdeAbandonTransaction16(DWORD,HCONV,DWORD);
BOOL16    WINAPI DdePostAdvise16(DWORD,HSZ,HSZ);
BOOL    WINAPI DdePostAdvise(DWORD,HSZ,HSZ);
HDDEDATA  WINAPI DdeAddData16(HDDEDATA,LPBYTE,DWORD,DWORD);
DWORD     WINAPI DdeGetData(HDDEDATA,LPBYTE,DWORD,DWORD);
LPBYTE    WINAPI DdeAccessData16(HDDEDATA,LPDWORD);
LPBYTE    WINAPI DdeAccessData(HDDEDATA,LPDWORD);
BOOL16    WINAPI DdeUnaccessData16(HDDEDATA);
BOOL    WINAPI DdeUnaccessData(HDDEDATA);
BOOL16    WINAPI DdeEnableCallback16(DWORD,HCONV,UINT16);
BOOL    WINAPI DdeEnableCallback(DWORD,HCONV,UINT);
int       WINAPI DdeCmpStringHandles16(HSZ,HSZ);
int       WINAPI DdeCmpStringHandles(HSZ,HSZ);

HDDEDATA  WINAPI DdeNameService16(DWORD,HSZ,HSZ,UINT16);
HDDEDATA  WINAPI DdeNameService(DWORD,HSZ,HSZ,UINT);
UINT16    WINAPI DdeGetLastError16(DWORD);
UINT    WINAPI DdeGetLastError(DWORD);

#endif  /* __WINE__DDEML_H */
