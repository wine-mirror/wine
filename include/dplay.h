#ifndef __WINE_DPLAY_H
#define __WINE_DPLAY_H

#include "wine/obj_base.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#include "pshpack1.h"

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID(CLSID_DirectPlay,0xd1eb6d20, 0x8923, 0x11d0, 0x9d, 0x97, 0x0, 0xa0, 0xc9, 0xa, 0x43, 0xcb);

DEFINE_GUID(IID_IDirectPlay, 0x5454e9a0, 0xdb65, 0x11ce, 0x92, 0x1c, 0x00, 0xaa, 0x00, 0x6c, 0x49, 0x72);
typedef struct IDirectPlay IDirectPlay,*LPDIRECTPLAY;

DEFINE_GUID(IID_IDirectPlay2, 0x2b74f7c0, 0x9154, 0x11cf, 0xa9, 0xcd, 0x0, 0xaa, 0x0, 0x68, 0x86, 0xe3);
typedef struct IDirectPlay2 IDirectPlay2,*LPDIRECTPLAY2;

DEFINE_GUID(IID_IDirectPlay2A,0x9d460580, 0xa822, 0x11cf, 0x96, 0xc, 0x0, 0x80, 0xc7, 0x53, 0x4e, 0x82);
typedef struct IDirectPlay2 IDirectPlay2A,*LPDIRECTPLAY2A;

DEFINE_GUID(IID_IDirectPlay3, 0x133efe40, 0x32dc, 0x11d0, 0x9c, 0xfb, 0x0, 0xa0, 0xc9, 0xa, 0x43, 0xcb);
typedef struct IDirectPlay3 IDirectPlay3,*LPDIRECTPLAY3;

DEFINE_GUID(IID_IDirectPlay3A,0x133efe41, 0x32dc, 0x11d0, 0x9c, 0xfb, 0x0, 0xa0, 0xc9, 0xa, 0x43, 0xcb);
typedef struct IDirectPlay3 IDirectPlay3A,*LPDIRECTPLAY3A;

/*
 * GUIDS used by Service Providers shipped with DirectPlay
 * Use these to identify Service Provider returned by EnumConnections
 */

/* GUID for IPX service provider {685BC400-9D2C-11cf-A9CD-00AA006886E3} */
DEFINE_GUID(DPSPGUID_IPX, 0x685bc400, 0x9d2c, 0x11cf, 0xa9, 0xcd, 0x0, 0xaa, 0x0, 0x68, 0x86, 0xe3);

/* GUID for TCP/IP service provider 36E95EE0-8577-11cf-960C-0080C7534E82 */
DEFINE_GUID(DPSPGUID_TCPIP, 0x36E95EE0, 0x8577, 0x11cf, 0x96, 0xc, 0x0, 0x80, 0xc7, 0x53, 0x4e, 0x82);

/* GUID for Serial service provider {0F1D6860-88D9-11cf-9C4E-00A0C905425E} */
DEFINE_GUID(DPSPGUID_SERIAL, 0xf1d6860, 0x88d9, 0x11cf, 0x9c, 0x4e, 0x0, 0xa0, 0xc9, 0x5, 0x42, 0x5e);

/* GUID for Modem service provider {44EAA760-CB68-11cf-9C4E-00A0C905425E} */
DEFINE_GUID(DPSPGUID_MODEM, 0x44eaa760, 0xcb68, 0x11cf, 0x9c, 0x4e, 0x0, 0xa0, 0xc9, 0x5, 0x42, 0x5e);


/*****************************************************************************
 * Miscellaneous
 */

/* Return Values for Direct Play */
#define _FACDP  0x877
#define MAKE_DPHRESULT( code )    MAKE_HRESULT( 1, _FACDP, code )

#define DP_OK                           S_OK
#define DPERR_ALREADYINITIALIZED        MAKE_DPHRESULT(   5 )
#define DPERR_ACCESSDENIED              MAKE_DPHRESULT(  10 )
#define DPERR_ACTIVEPLAYERS             MAKE_DPHRESULT(  20 )
#define DPERR_BUFFERTOOSMALL            MAKE_DPHRESULT(  30 )
#define DPERR_CANTADDPLAYER             MAKE_DPHRESULT(  40 )
#define DPERR_CANTCREATEGROUP           MAKE_DPHRESULT(  50 )
#define DPERR_CANTCREATEPLAYER          MAKE_DPHRESULT(  60 )
#define DPERR_CANTCREATESESSION         MAKE_DPHRESULT(  70 )
#define DPERR_CAPSNOTAVAILABLEYET       MAKE_DPHRESULT(  80 )
#define DPERR_EXCEPTION                 MAKE_DPHRESULT(  90 )
#define DPERR_GENERIC                   E_FAIL
#define DPERR_INVALIDFLAGS              MAKE_DPHRESULT( 120 )
#define DPERR_INVALIDOBJECT             MAKE_DPHRESULT( 130 )
#define DPERR_INVALIDPARAM              E_INVALIDARG
#define DPERR_INVALIDPARAMS             DPERR_INVALIDPARAM
#define DPERR_INVALIDPLAYER             MAKE_DPHRESULT( 150 )
#define DPERR_INVALIDGROUP              MAKE_DPHRESULT( 155 )
#define DPERR_NOCAPS                    MAKE_DPHRESULT( 160 )
#define DPERR_NOCONNECTION              MAKE_DPHRESULT( 170 )
#define DPERR_NOMEMORY                  E_OUTOFMEMORY
#define DPERR_OUTOFMEMORY               DPERR_NOMEMORY
#define DPERR_NOMESSAGES                MAKE_DPHRESULT( 190 )
#define DPERR_NONAMESERVERFOUND         MAKE_DPHRESULT( 200 )
#define DPERR_NOPLAYERS                 MAKE_DPHRESULT( 210 )
#define DPERR_NOSESSIONS                MAKE_DPHRESULT( 220 )
#define DPERR_PENDING                   E_PENDING
#define DPERR_SENDTOOBIG                MAKE_DPHRESULT( 230 )
#define DPERR_TIMEOUT                   MAKE_DPHRESULT( 240 )
#define DPERR_UNAVAILABLE               MAKE_DPHRESULT( 250 )
#define DPERR_UNSUPPORTED               E_NOTIMPL
#define DPERR_BUSY                      MAKE_DPHRESULT( 270 )
#define DPERR_USERCANCEL                MAKE_DPHRESULT( 280 )
#define DPERR_NOINTERFACE               E_NOINTERFACE
#define DPERR_CANNOTCREATESERVER        MAKE_DPHRESULT( 290 )
#define DPERR_PLAYERLOST                MAKE_DPHRESULT( 300 )
#define DPERR_SESSIONLOST               MAKE_DPHRESULT( 310 )
#define DPERR_UNINITIALIZED             MAKE_DPHRESULT( 320 )
#define DPERR_NONEWPLAYERS              MAKE_DPHRESULT( 330 )
#define DPERR_INVALIDPASSWORD           MAKE_DPHRESULT( 340 )
#define DPERR_CONNECTING                MAKE_DPHRESULT( 350 )
#define DPERR_BUFFERTOOLARGE            MAKE_DPHRESULT( 1000 )
#define DPERR_CANTCREATEPROCESS         MAKE_DPHRESULT( 1010 )
#define DPERR_APPNOTSTARTED             MAKE_DPHRESULT( 1020 )
#define DPERR_INVALIDINTERFACE          MAKE_DPHRESULT( 1030 )
#define DPERR_NOSERVICEPROVIDER         MAKE_DPHRESULT( 1040 )
#define DPERR_UNKNOWNAPPLICATION        MAKE_DPHRESULT( 1050 )
#define DPERR_NOTLOBBIED                MAKE_DPHRESULT( 1070 )
#define DPERR_SERVICEPROVIDERLOADED     MAKE_DPHRESULT( 1080 )
#define DPERR_ALREADYREGISTERED         MAKE_DPHRESULT( 1090 )
#define DPERR_NOTREGISTERED             MAKE_DPHRESULT( 1100 )
#define DPERR_AUTHENTICATIONFAILED      MAKE_DPHRESULT(  2000 )
#define DPERR_CANTLOADSSPI              MAKE_DPHRESULT(  2010 )
#define DPERR_ENCRYPTIONFAILED          MAKE_DPHRESULT(  2020 )
#define DPERR_SIGNFAILED                MAKE_DPHRESULT(  2030 )
#define DPERR_CANTLOADSECURITYPACKAGE   MAKE_DPHRESULT(  2040 )
#define DPERR_ENCRYPTIONNOTSUPPORTED    MAKE_DPHRESULT(  2050 )
#define DPERR_CANTLOADCAPI              MAKE_DPHRESULT(  2060 )
#define DPERR_NOTLOGGEDIN               MAKE_DPHRESULT(  2070 )
#define DPERR_LOGONDENIED               MAKE_DPHRESULT(  2080 )


/* DPID - DirectPlay player and group ID */
typedef DWORD DPID, *LPDPID;

/* DPID from whence originate messages - just an ID */
#define DPID_SYSMSG             0           /* DPID of system */
#define DPID_ALLPLAYERS         0           /* DPID of all players */
#define DPID_SERVERPLAYER       1           /* DPID of the server player */
#define DPID_UNKNOWN            0xFFFFFFFF  /* Player ID is unknown */

/*  DPCAPS -  Used to obtain the capabilities of a DirectPlay object */
typedef struct tagDPCAPS
{
    DWORD dwSize;               /* Size of structure in bytes */
    DWORD dwFlags;              
    DWORD dwMaxBufferSize;      
    DWORD dwMaxQueueSize;       /* Obsolete. */
    DWORD dwMaxPlayers;         /* Maximum players/groups (local + remote) */
    DWORD dwHundredBaud;        /* Bandwidth in 100 bits per second units;
                                 * i.e. 24 is 2400, 96 is 9600, etc. 
                                 */
    DWORD dwLatency;            /* Estimated latency; 0 = unknown */
    DWORD dwMaxLocalPlayers;    /* Maximum # of locally created players */
    DWORD dwHeaderLength;       /* Maximum header length in bytes */
    DWORD dwTimeout;            /* Service provider's suggested timeout value
                                 * This is how long DirectPlay will wait for
                                 * responses to system messages
                                 */
} DPCAPS, *LPDPCAPS;

typedef struct tagDPNAME
{
    DWORD   dwSize;             
    DWORD   dwFlags;            /* Not used must be 0 */

    union /*playerShortName */      /* Player's Handle? */
    {                           
        LPWSTR  lpszShortName;  
        LPSTR   lpszShortNameA; 
    }psn;

    union /*playerLongName */       /* Player's formal/real name */
    {                         
        LPWSTR  lpszLongName;  
        LPSTR   lpszLongNameA;  
    }pln;

} DPNAME, *LPDPNAME;

#define DPLONGNAMELEN     52
#define DPSHORTNAMELEN    20
#define DPSESSIONNAMELEN  32
#define DPPASSWORDLEN     16
#define DPUSERRESERVED    16

typedef struct tagDPSESSIONDESC
{
    DWORD   dwSize;
    GUID    guidSession;
    DWORD   dwSession;
    DWORD   dwMaxPlayers;
    DWORD   dwCurrentPlayers;
    DWORD   dwFlags;
    char    szSessionName[ DPSESSIONNAMELEN ];
    char    szUserField[ DPUSERRESERVED ];
    DWORD   dwReserved1;
    char    szPassword[ DPPASSWORDLEN ];
    DWORD   dwReserved2;
    DWORD   dwUser1;
    DWORD   dwUser2;
    DWORD   dwUser3;
    DWORD   dwUser4;
} DPSESSIONDESC, *LPDPSESSIONDESC;

typedef struct tagDPSESSIONDESC2
{
    DWORD   dwSize;             
    DWORD   dwFlags;           
    GUID    guidInstance;      
    GUID    guidApplication;   /* GUID of the DP application, GUID_NULL if
                                * all applications! */
                               
    DWORD   dwMaxPlayers;      
    DWORD   dwCurrentPlayers;   /* (read only value) */

    union  /* Session name */
    {                             
        LPWSTR  lpszSessionName;  
        LPSTR   lpszSessionNameA; 
    }sess;

    union  /* Optional password */
    {                           
        LPWSTR  lpszPassword;   
        LPSTR   lpszPasswordA;  
    }pass;

    DWORD   dwReserved1;       
    DWORD   dwReserved2;

    DWORD   dwUser1;        /* For use by the application */  
    DWORD   dwUser2;
    DWORD   dwUser3;
    DWORD   dwUser4;
} DPSESSIONDESC2, *LPDPSESSIONDESC2;
typedef const DPSESSIONDESC2* LPCDPSESSIONDESC2;

#define DPOPEN_JOIN                     0x00000001
#define DPOPEN_CREATE                   0x00000002
#define DPOPEN_RETURNSTATUS             DPENUMSESSIONS_RETURNSTATUS

#define DPSESSION_NEWPLAYERSDISABLED    0x00000001
#define DPSESSION_MIGRATEHOST           0x00000004
#define DPSESSION_NOMESSAGEID           0x00000008
#define DPSESSION_JOINDISABLED          0x00000020
#define DPSESSION_KEEPALIVE             0x00000040
#define DPSESSION_NODATAMESSAGES        0x00000080
#define DPSESSION_SECURESERVER          0x00000100
#define DPSESSION_PRIVATE               0x00000200
#define DPSESSION_PASSWORDREQUIRED      0x00000400
#define DPSESSION_MULTICASTSERVER       0x00000800
#define DPSESSION_CLIENTSERVER          0x00001000

typedef struct tagDPLCONNECTION
{
    DWORD               dwSize;          
    DWORD               dwFlags;          
    LPDPSESSIONDESC2    lpSessionDesc;  /* Ptr to session desc to use for connect */  
    LPDPNAME            lpPlayerName;   /* Ptr to player name structure */
    GUID                guidSP;         /* GUID of Service Provider to use */ 
    LPVOID              lpAddress;      /* Ptr to Address of Service Provider to use */
    DWORD               dwAddressSize;  /* Size of address data */
} DPLCONNECTION, *LPDPLCONNECTION;

/* DPLCONNECTION flags (for dwFlags) */
#define DPLCONNECTION_CREATESESSION DPOPEN_CREATE
#define DPLCONNECTION_JOINSESSION   DPOPEN_JOIN

typedef struct tagDPCHAT
{
    DWORD               dwSize;
    DWORD               dwFlags;
    union
    {                          /* Message string */
        LPWSTR  lpszMessage;   /* Unicode */
        LPSTR   lpszMessageA;  /* ANSI */
    }msgstr;
} DPCHAT, *LPDPCHAT;

typedef struct tagDPSECURITYDESC
{
    DWORD dwSize;                   /* Size of structure */
    DWORD dwFlags;                  /* Not used. Must be zero. */
    union
    {                               /* SSPI provider name */
        LPWSTR  lpszSSPIProvider;   /* Unicode */
        LPSTR   lpszSSPIProviderA;  /* ANSI */
    }sspi;
    union
    {                               /* CAPI provider name */
        LPWSTR lpszCAPIProvider;    /* Unicode */
        LPSTR  lpszCAPIProviderA;   /* ANSI */
    }capi;
    DWORD dwCAPIProviderType;       /* Crypto Service Provider type */
    DWORD dwEncryptionAlgorithm;    /* Encryption Algorithm type */
} DPSECURITYDESC, *LPDPSECURITYDESC;

typedef const DPSECURITYDESC *LPCDPSECURITYDESC;

typedef struct tagDPCREDENTIALS
{
    DWORD dwSize;               /* Size of structure */
    DWORD dwFlags;              /* Not used. Must be zero. */
    union
    {                           /* User name of the account */
        LPWSTR  lpszUsername;   /* Unicode */
        LPSTR   lpszUsernameA;  /* ANSI */
    }name;
    union
    {                           /* Password of the account */
        LPWSTR  lpszPassword;   /* Unicode */
        LPSTR   lpszPasswordA;  /* ANSI */
    }pass;
    union
    {                           /* Domain name of the account */
        LPWSTR  lpszDomain;     /* Unicode */
        LPSTR   lpszDomainA;    /* ANSI */
    }domain;
} DPCREDENTIALS, *LPDPCREDENTIALS;

typedef const DPCREDENTIALS *LPCDPCREDENTIALS;



typedef BOOL (CALLBACK* LPDPENUMDPCALLBACKW)(
    LPGUID      lpguidSP,
    LPWSTR      lpSPName,
    DWORD       dwMajorVersion,
    DWORD       dwMinorVersion,
    LPVOID      lpContext);

typedef BOOL (CALLBACK* LPDPENUMDPCALLBACKA)(
    LPGUID      lpguidSP,
    LPSTR       lpSPName,       /* ptr to str w/ driver description */
    DWORD       dwMajorVersion, /* Major # of driver spec in lpguidSP */
    DWORD       dwMinorVersion, /* Minor # of driver spec in lpguidSP */ 
    LPVOID      lpContext);     /* User given */

typedef const GUID   *LPCGUID;
typedef const DPNAME *LPCDPNAME;

typedef BOOL (CALLBACK* LPDPENUMCONNECTIONSCALLBACK)(
    LPCGUID     lpguidSP,
    LPVOID      lpConnection,
    DWORD       dwConnectionSize,
    LPCDPNAME   lpName,
    DWORD       dwFlags,
    LPVOID      lpContext);

typedef BOOL (CALLBACK* LPDPENUMSESSIONSCALLBACK)(
    LPDPSESSIONDESC lpDPSessionDesc,
    LPVOID      lpContext,
    LPDWORD     lpdwTimeOut,
    DWORD       dwFlags);


extern HRESULT WINAPI DirectPlayEnumerateA( LPDPENUMDPCALLBACKA, LPVOID );
extern HRESULT WINAPI DirectPlayEnumerateW( LPDPENUMDPCALLBACKW, LPVOID );
extern HRESULT WINAPI DirectPlayCreate( LPGUID lpGUID, LPDIRECTPLAY2 *lplpDP, IUnknown *pUnk);

typedef BOOL (CALLBACK* LPDPENUMPLAYERSCALLBACK)(
    DPID   dpId,
    LPSTR  lpFriendlyName,
    LPSTR  lpFormalName,
    DWORD  dwFlags,
    LPVOID          lpContext );

typedef BOOL (CALLBACK* LPDPENUMPLAYERSCALLBACK2)(
    DPID            dpId,
    DWORD           dwPlayerType,
    LPCDPNAME       lpName,
    DWORD           dwFlags,
    LPVOID          lpContext );

typedef BOOL (CALLBACK* LPDPENUMSESSIONSCALLBACK2)(
    LPCDPSESSIONDESC2   lpThisSD,
    LPDWORD             lpdwTimeOut,
    DWORD               dwFlags,
    LPVOID              lpContext );

#include "poppack.h"

/*****************************************************************************
 * IDirectPlay interface
 */
#define ICOM_INTERFACE IDirectPlay
#define IDirectPlay_METHODS \
    ICOM_METHOD2(HRESULT,AddPlayerToGroup,      DPID,idGroup, DPID,idPlayer) \
    ICOM_METHOD (HRESULT,Close) \
    ICOM_METHOD4(HRESULT,CreatePlayer,          LPDPID,lpidPlayer, LPSTR,lpPlayerName, LPSTR,, LPHANDLE,) \
    ICOM_METHOD3(HRESULT,CreateGroup,           LPDPID,lpidGroup, LPSTR,lpGroupName, LPSTR,) \
    ICOM_METHOD2(HRESULT,DeletePlayerFromGroup, DPID,idGroup, DPID,idPlayer) \
    ICOM_METHOD1(HRESULT,DestroyPlayer,         DPID,idPlayer) \
    ICOM_METHOD1(HRESULT,DestroyGroup,          DPID,idGroup) \
    ICOM_METHOD1(HRESULT,EnableNewPlayers,      BOOL,) \
    ICOM_METHOD4(HRESULT,EnumGroupPlayers,      DPID,idGroup, LPDPENUMPLAYERSCALLBACK,lpEnumPlayersCallback, LPVOID,lpContext, DWORD,dwFlags) \
    ICOM_METHOD4(HRESULT,EnumGroups,            DWORD,, LPDPENUMPLAYERSCALLBACK,lpEnumPlayersCallback, LPVOID,lpContext, DWORD,dwFlags) \
    ICOM_METHOD4(HRESULT,EnumPlayers,           DWORD,, LPDPENUMPLAYERSCALLBACK,lpEnumPlayersCallback, LPVOID,lpContext, DWORD,dwFlags) \
    ICOM_METHOD5(HRESULT,EnumSessions,          LPDPSESSIONDESC,lpsd, DWORD,dwTimeout, LPDPENUMSESSIONSCALLBACK,lpEnumSessionsCallback, LPVOID,lpContext, DWORD,dwFlags) \
    ICOM_METHOD1(HRESULT,GetCaps,               LPDPCAPS,lpDPCaps) \
    ICOM_METHOD2(HRESULT,GetMessageCount,       DPID,idPlayer, LPDWORD,lpdwCount) \
    ICOM_METHOD2(HRESULT,GetPlayerCaps,         DPID,idPlayer, LPDPCAPS,lpPlayerCaps) \
    ICOM_METHOD5(HRESULT,GetPlayerName,         DPID,idPlayer, LPSTR,, LPDWORD,, LPSTR,, LPDWORD,) \
    ICOM_METHOD1(HRESULT,Initialize,            LPGUID,lpGUID) \
    ICOM_METHOD1(HRESULT,Open,                  LPDPSESSIONDESC,lpsd) \
    ICOM_METHOD5(HRESULT,Receive,               LPDPID,lpidFrom, LPDPID,lpidTo, DWORD,dwFlags, LPVOID,lpData, LPDWORD,lpdwDataSize) \
    ICOM_METHOD1(HRESULT,SaveSession,           LPSTR,) \
    ICOM_METHOD5(HRESULT,Send,                  DPID,idFrom, DPID,idTo, DWORD,dwFlags, LPVOID,lpData, DWORD,dwDataSize) \
    ICOM_METHOD3(HRESULT,SetPlayerName,         DPID,idPlayer, LPSTR,lpPlayerName, LPSTR,)
#define IDirectPlay_IMETHODS \
    IUnknown_IMETHODS \
    IDirectPlay_METHODS
ICOM_DEFINE(IDirectPlay,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
    /*** IUnknown methods ***/
#define IDirectPlay_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectPlay_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirectPlay_Release(p)            ICOM_CALL (Release,p)
/*** IDirectPlay methods ***/
#define IDirectPlay_AddPlayerToGroup(p,a,b)      ICOM_CALL2(AddPlayerToGroup,p,a,b)
#define IDirectPlay_Close(p)                     ICOM_CALL (Close,p)
#define IDirectPlay_CreatePlayer(p,a,b,c,d)      ICOM_CALL4(CreatePlayer,p,a,b,c,d)
#define IDirectPlay_CreateGroup(p,a,b,c)         ICOM_CALL3(CreateGroup,p,a,b,c)
#define IDirectPlay_DeletePlayerFromGroup(p,a,b) ICOM_CALL2(DeletePlayerFromGroup,p,a,b)
#define IDirectPlay_DestroyPlayer(p,a)           ICOM_CALL1(DestroyPlayer,p,a)
#define IDirectPlay_DestroyGroup(p,a)            ICOM_CALL1(DestroyGroup,p,a)
#define IDirectPlay_EnableNewPlayers(p,a)        ICOM_CALL1(EnableNewPlayers,p,a)
#define IDirectPlay_EnumGroupPlayers(p,a,b,c,d)  ICOM_CALL4(EnumGroupPlayers,p,a,b,c,d)
#define IDirectPlay_EnumGroups(p,a,b,c,d)        ICOM_CALL4(EnumGroups,p,a,b,c,d)
#define IDirectPlay_EnumPlayers(p,a,b,c,d)       ICOM_CALL4(EnumPlayers,p,a,b,c,d)
#define IDirectPlay_EnumSessions(p,a,b,c,d,e)    ICOM_CALL5(EnumSessions,p,a,b,c,d,e)
#define IDirectPlay_GetCaps(p,a)                 ICOM_CALL1(GetCaps,p,a)
#define IDirectPlay_GetMessageCount(p,a,b)       ICOM_CALL2(GetMessageCount,p,a,b)
#define IDirectPlay_GetPlayerCaps(p,a,b)         ICOM_CALL2(GetPlayerCaps,p,a,b)
#define IDirectPlay_GetPlayerName(p,a,b,c,d,e)   ICOM_CALL5(GetPlayerName,p,a,b,c,d,e)
#define IDirectPlay_Initialize(p,a)              ICOM_CALL1(Initialize,p,a)
#define IDirectPlay_Open(p,a)                    ICOM_CALL1(Open,p,a)
#define IDirectPlay_Receive(p,a,b,c,d,e)         ICOM_CALL5(Receive,p,a,b,c,d,e)
#define IDirectPlay_SaveSession(p,a)             ICOM_CALL1(SaveSession,p,a)
#define IDirectPlay_Send(p,a,b,c,d,e)            ICOM_CALL5(Send,p,a,b,c,d,e)
#define IDirectPlay_SetPlayerName(p,a,b,c)       ICOM_CALL3(SetPlayerName,p,a,b,c)
#endif


/*****************************************************************************
 * IDirectPlay2 interface
 */
#define ICOM_INTERFACE IDirectPlay2
#define IDirectPlay2_METHODS \
    ICOM_METHOD2(HRESULT,AddPlayerToGroup,      DPID,idGroup, DPID,idPlayer) \
    ICOM_METHOD (HRESULT,Close) \
    ICOM_METHOD5(HRESULT,CreateGroup,           LPDPID,lpidGroup, LPDPNAME,lpGroupName, LPVOID,lpData, DWORD,dwDataSize, DWORD,dwFlags) \
    ICOM_METHOD6(HRESULT,CreatePlayer,          LPDPID,lpidPlayer, LPDPNAME,lpPlayerName, HANDLE,hEvent, LPVOID,lpData, DWORD,dwDataSize, DWORD,dwFlags) \
    ICOM_METHOD2(HRESULT,DeletePlayerFromGroup, DPID,idGroup, DPID,idPlayer) \
    ICOM_METHOD1(HRESULT,DestroyGroup,          DPID,idGroup) \
    ICOM_METHOD1(HRESULT,DestroyPlayer,         DPID,idPlayer) \
    ICOM_METHOD5(HRESULT,EnumGroupPlayers,      DPID,idGroup, LPGUID,lpguidInstance, LPDPENUMPLAYERSCALLBACK2,lpEnumPlayersCallback2, LPVOID,lpContext, DWORD,dwFlags) \
    ICOM_METHOD4(HRESULT,EnumGroups,            LPGUID,lpguidInstance, LPDPENUMPLAYERSCALLBACK2,lpEnumPlayersCallback2, LPVOID,lpContext, DWORD,dwFlags) \
    ICOM_METHOD4(HRESULT,EnumPlayers,           LPGUID,lpguidInstance, LPDPENUMPLAYERSCALLBACK2,lpEnumPlayersCallback2, LPVOID,lpContext, DWORD,dwFlags) \
    ICOM_METHOD5(HRESULT,EnumSessions,          LPDPSESSIONDESC2,lpsd, DWORD,dwTimeout, LPDPENUMSESSIONSCALLBACK2,lpEnumSessionsCallback2, LPVOID,lpContext, DWORD,dwFlags) \
    ICOM_METHOD2(HRESULT,GetCaps,               LPDPCAPS,lpDPCaps, DWORD,dwFlags) \
    ICOM_METHOD4(HRESULT,GetGroupData,          DPID,idGroup, LPVOID,lpData, LPDWORD,lpdwDataSize, DWORD,dwFlags) \
    ICOM_METHOD3(HRESULT,GetGroupName,          DPID,idGroup, LPVOID,lpData, LPDWORD,lpdwDataSize) \
    ICOM_METHOD2(HRESULT,GetMessageCount,       DPID,idPlayer, LPDWORD,lpdwCount) \
    ICOM_METHOD3(HRESULT,GetPlayerAddress,      DPID,idPlayer, LPVOID,lpData, LPDWORD,lpdwDataSize) \
    ICOM_METHOD3(HRESULT,GetPlayerCaps,         DPID,idPlayer, LPDPCAPS,lpPlayerCaps, DWORD,dwFlags) \
    ICOM_METHOD4(HRESULT,GetPlayerData,         DPID,idPlayer, LPVOID,lpData, LPDWORD,lpdwDataSize, DWORD,dwFlags) \
    ICOM_METHOD3(HRESULT,GetPlayerName,         DPID,idPlayer, LPVOID,lpData, LPDWORD,lpdwDataSize) \
    ICOM_METHOD2(HRESULT,GetSessionDesc,        LPVOID,lpData, LPDWORD,lpdwDataSize) \
    ICOM_METHOD1(HRESULT,Initialize,            LPGUID,lpGUID) \
    ICOM_METHOD2(HRESULT,Open,                  LPDPSESSIONDESC2,lpsd, DWORD,dwFlags) \
    ICOM_METHOD5(HRESULT,Receive,               LPDPID,lpidFrom, LPDPID,lpidTo, DWORD,dwFlags, LPVOID,lpData, LPDWORD,lpdwDataSize) \
    ICOM_METHOD5(HRESULT,Send,                  DPID,idFrom, DPID,idTo, DWORD,dwFlags, LPVOID,lpData, DWORD,dwDataSize) \
    ICOM_METHOD4(HRESULT,SetGroupData,          DPID,idGroup, LPVOID,lpData, DWORD,dwDataSize, DWORD,dwFlags) \
    ICOM_METHOD3(HRESULT,SetGroupName,          DPID,idGroup, LPDPNAME,lpGroupName, DWORD,dwFlags) \
    ICOM_METHOD4(HRESULT,SetPlayerData,         DPID,idPlayer, LPVOID,lpData, DWORD,dwDataSize, DWORD,dwFlags) \
    ICOM_METHOD3(HRESULT,SetPlayerName,         DPID,idPlayer, LPDPNAME,lpPlayerName, DWORD,dwFlags) \
    ICOM_METHOD2(HRESULT,SetSessionDesc,        LPDPSESSIONDESC2,lpSessDesc, DWORD,dwFlags)
#define IDirectPlay2_IMETHODS \
    IUnknown_IMETHODS \
    IDirectPlay2_METHODS
ICOM_DEFINE(IDirectPlay2,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
/*** IUnknown methods ***/
#define IDirectPlay2_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectPlay2_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirectPlay2_Release(p)            ICOM_CALL (Release,p)
    /*** IDirectPlay2 methods ***/
#define IDirectPlay2_AddPlayerToGroup(p,a,b)       ICOM_CALL2(AddPlayerToGroup,p,a,b)
#define IDirectPlay2_Close(p)                      ICOM_CALL (Close,p)
#define IDirectPlay2_CreateGroup(p,a,b,c,d,e)      ICOM_CALL5(CreateGroup,p,a,b,c,d,e)
#define IDirectPlay2_CreatePlayer(p,a,b,c,d,e,f)   ICOM_CALL6(CreatePlayer,p,a,b,c,d,e,f)
#define IDirectPlay2_DeletePlayerFromGroup(p,a,b)  ICOM_CALL2(DeletePlayerFromGroup,p,a,b)
#define IDirectPlay2_DestroyGroup(p,a)             ICOM_CALL1(DestroyGroup,p,a)
#define IDirectPlay2_DestroyPlayer(p,a)            ICOM_CALL1(DestroyPlayer,p,a)
#define IDirectPlay2_EnumGroupPlayers(p,a,b,c,d,e) ICOM_CALL5(EnumGroupPlayers,p,a,b,c,d,e)
#define IDirectPlay2_EnumGroups(p,a,b,c,d)         ICOM_CALL4(EnumGroups,p,a,b,c,d)
#define IDirectPlay2_EnumPlayers(p,a,b,c,d)        ICOM_CALL4(EnumPlayers,p,a,b,c,d)
#define IDirectPlay2_EnumSessions(p,a,b,c,d,e)     ICOM_CALL5(EnumSessions,p,a,b,c,d,e)
#define IDirectPlay2_GetCaps(p,a,b)                ICOM_CALL2(GetCaps,p,a,b)
#define IDirectPlay2_GetGroupData(p,a,b,c,d)       ICOM_CALL4(GetGroupData,p,a,b,c,d)
#define IDirectPlay2_GetGroupName(p,a,b,c)         ICOM_CALL3(GetGroupName,p,a,b,c)
#define IDirectPlay2_GetMessageCount(p,a,b)        ICOM_CALL2(GetMessageCount,p,a,b)
#define IDirectPlay2_GetPlayerAddress(p,a,b,c)     ICOM_CALL3(GetPlayerAddress,p,a,b,c)
#define IDirectPlay2_GetPlayerCaps(p,a,b,c)        ICOM_CALL3(GetPlayerCaps,p,a,b,c)
#define IDirectPlay2_GetPlayerData(p,a,b,c,d)      ICOM_CALL4(GetPlayerData,p,a,b,c,d)
#define IDirectPlay2_GetPlayerName(p,a,b,c)        ICOM_CALL3(GetPlayerName,p,a,b,c)
#define IDirectPlay2_GetSessionDesc(p,a,b)         ICOM_CALL2(GetSessionDesc,p,a,b)
#define IDirectPlay2_Initialize(p,a)               ICOM_CALL1(Initialize,p,a)
#define IDirectPlay2_Open(p,a,b)                   ICOM_CALL2(Open,p,a,b)
#define IDirectPlay2_Receive(p,a,b,c,d,e)          ICOM_CALL5(Receive,p,a,b,c,d,e)
#define IDirectPlay2_Send(p,a,b,c,d,e)             ICOM_CALL5(Send,p,a,b,c,d,e)
#define IDirectPlay2_SetGroupData(p,a,b,c,d)       ICOM_CALL4(SetGroupData,p,a,b,c,d)
#define IDirectPlay2_SetGroupName(p,a,b,c)         ICOM_CALL3(SetGroupName,p,a,b,c)
#define IDirectPlay2_SetPlayerData(p,a,b,c,d)      ICOM_CALL4(SetPlayerData,p,a,b,c,d)
#define IDirectPlay2_SetPlayerName(p,a,b,c)        ICOM_CALL3(SetPlayerName,p,a,b,c)
#define IDirectPlay2_SetSessionDesc(p,a,b)         ICOM_CALL2(SetSessionDesc,p,a,b)
#endif


/*****************************************************************************
 * IDirectPlay3 interface
 */
#define ICOM_INTERFACE IDirectPlay3
#define IDirectPlay3_METHODS \
    ICOM_METHOD2(HRESULT,AddGroupToGroup,            DPID,idParentGroup, DPID,idGroup) \
    ICOM_METHOD6(HRESULT,CreateGroupInGroup,         DPID,idParentGroup, LPDPID,lpidGroup, LPDPNAME,lpGroupName, LPVOID,lpData, DWORD,dwDataSize, DWORD,dwFlags) \
    ICOM_METHOD2(HRESULT,DeleteGroupFromGroup,       DPID,idParentGroup, DPID,idGroup) \
    ICOM_METHOD4(HRESULT,EnumConnections,            LPCGUID,lpguidApplication, LPDPENUMCONNECTIONSCALLBACK,lpEnumCallback, LPVOID,lpContext, DWORD,dwFlags) \
    ICOM_METHOD5(HRESULT,EnumGroupsInGroup,          DPID,idGroup, LPGUID,lpguidInstance, LPDPENUMPLAYERSCALLBACK2,lpEnumCallback, LPVOID,lpContext, DWORD,dwFlags) \
    ICOM_METHOD4(HRESULT,GetGroupConnectionSettings, DWORD,dwFlags, DPID,idGroup, LPVOID,lpData, LPDWORD,lpdwDataSize) \
    ICOM_METHOD2(HRESULT,InitializeConnection,       LPVOID,lpConnection, DWORD,dwFlags) \
    ICOM_METHOD4(HRESULT,SecureOpen,                 LPCDPSESSIONDESC2,lpsd, DWORD,dwFlags, LPCDPSECURITYDESC,lpSecurity, LPCDPCREDENTIALS,lpCredentials) \
    ICOM_METHOD4(HRESULT,SendChatMessage,            DPID,idFrom, DPID,idTo, DWORD,dwFlags, LPDPCHAT,lpChatMessage) \
    ICOM_METHOD3(HRESULT,SetGroupConnectionSettings, DWORD,dwFlags, DPID,idGroup, LPDPLCONNECTION,lpConnection) \
    ICOM_METHOD2(HRESULT,StartSession,               DWORD,dwFlags, DPID,idGroup) \
    ICOM_METHOD2(HRESULT,GetGroupFlags,              DPID,idGroup, LPDWORD,lpdwFlags) \
    ICOM_METHOD2(HRESULT,GetGroupParent,             DPID,idGroup, LPDPID,lpidParent) \
    ICOM_METHOD4(HRESULT,GetPlayerAccount,           DPID,idPlayer, DWORD,dwFlags, LPVOID,lpData, LPDWORD,lpdwDataSize) \
    ICOM_METHOD2(HRESULT,GetPlayerFlags,             DPID,idPlayer, LPDWORD,lpdwFlags)
#define IDirectPlay3_IMETHODS \
    IDirectPlay2_IMETHODS \
    IDirectPlay3_METHODS
ICOM_DEFINE(IDirectPlay3,IDirectPlay2)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
/*** IUnknown methods ***/
#define IDirectPlay3_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectPlay3_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirectPlay3_Release(p)            ICOM_CALL (Release,p)
/*** IDirectPlay2 methods ***/
#define IDirectPlay3_AddPlayerToGroup(p,a,b)       ICOM_CALL2(AddPlayerToGroup,p,a,b)
#define IDirectPlay3_Close(p)                      ICOM_CALL (Close,p)
#define IDirectPlay3_CreateGroup(p,a,b,c,d,e)      ICOM_CALL5(CreateGroup,p,a,b,c,d,e)
#define IDirectPlay3_CreatePlayer(p,a,b,c,d,e,f)   ICOM_CALL6(CreatePlayer,p,a,b,c,d,e,f)
#define IDirectPlay3_DeletePlayerFromGroup(p,a,b)  ICOM_CALL2(DeletePlayerFromGroup,p,a,b)
#define IDirectPlay3_DestroyGroup(p,a)             ICOM_CALL1(DestroyGroup,p,a)
#define IDirectPlay3_DestroyPlayer(p,a)            ICOM_CALL1(DestroyPlayer,p,a)
#define IDirectPlay3_EnumGroupPlayers(p,a,b,c,d,e) ICOM_CALL5(EnumGroupPlayers,p,a,b,c,d,e)
#define IDirectPlay3_EnumGroups(p,a,b,c,d)         ICOM_CALL4(EnumGroups,p,a,b,c,d)
#define IDirectPlay3_EnumPlayers(p,a,b,c,d)        ICOM_CALL4(EnumPlayers,p,a,b,c,d)
#define IDirectPlay3_EnumSessions(p,a,b,c,d,e)     ICOM_CALL5(EnumSessions,p,a,b,c,d,e)
#define IDirectPlay3_GetCaps(p,a,b)                ICOM_CALL2(GetCaps,p,a,b)
#define IDirectPlay3_GetGroupData(p,a,b,c,d)       ICOM_CALL4(GetGroupData,p,a,b,c,d)
#define IDirectPlay3_GetGroupName(p,a,b,c)         ICOM_CALL3(GetGroupName,p,a,b,c)
#define IDirectPlay3_GetMessageCount(p,a,b)        ICOM_CALL2(GetMessageCount,p,a,b)
#define IDirectPlay3_GetPlayerAddress(p,a,b,c)     ICOM_CALL3(GetPlayerAddress,p,a,b,c)
#define IDirectPlay3_GetPlayerCaps(p,a,b,c)        ICOM_CALL3(GetPlayerCaps,p,a,b,c)
#define IDirectPlay3_GetPlayerData(p,a,b,c,d)      ICOM_CALL4(GetPlayerData,p,a,b,c,d)
#define IDirectPlay3_GetPlayerName(p,a,b,c)        ICOM_CALL3(GetPlayerName,p,a,b,c)
#define IDirectPlay3_GetSessionDesc(p,a,b)         ICOM_CALL2(GetSessionDesc,p,a,b)
#define IDirectPlay3_Initialize(p,a)               ICOM_CALL1(Initialize,p,a)
#define IDirectPlay3_Open(p,a,b)                   ICOM_CALL2(Open,p,a,b)
#define IDirectPlay3_Receive(p,a,b,c,d,e)          ICOM_CALL5(Receive,p,a,b,c,d,e)
#define IDirectPlay3_Send(p,a,b,c,d,e)             ICOM_CALL5(Send,p,a,b,c,d,e)
#define IDirectPlay3_SetGroupData(p,a,b,c,d)       ICOM_CALL4(SetGroupData,p,a,b,c,d)
#define IDirectPlay3_SetGroupName(p,a,b,c)         ICOM_CALL3(SetGroupName,p,a,b,c)
#define IDirectPlay3_SetPlayerData(p,a,b,c,d)      ICOM_CALL4(SetPlayerData,p,a,b,c,d)
#define IDirectPlay3_SetPlayerName(p,a,b,c)        ICOM_CALL3(SetPlayerName,p,a,b,c)
#define IDirectPlay3_SetSessionDesc(p,a,b)         ICOM_CALL2(SetSessionDesc,p,a,b)
/*** IDirectPlay3 methods ***/
#define IDirectPlay3_AddGroupToGroup(p,a,b)                ICOM_CALL2(AddGroupToGroup,p,a,b)
#define IDirectPlay3_CreateGroupInGroup(p,a,b,c,d,e,f)     ICOM_CALL6(CreateGroupInGroup,p,a,b,c,d,e,f)
#define IDirectPlay3_DeleteGroupFromGroup(p,a,b)           ICOM_CALL2(DeleteGroupFromGroup,p,a,b)
#define IDirectPlay3_EnumConnections(p,a,b,c,d)            ICOM_CALL4(EnumConnections,p,a,b,c,d)
#define IDirectPlay3_EnumGroupsInGroup(p,a,b,c,d,e)        ICOM_CALL5(EnumGroupsInGroup,p,a,b,c,d,e)
#define IDirectPlay3_GetGroupConnectionSettings(p,a,b,c,d) ICOM_CALL4(GetGroupConnectionSettings,p,a,b,c,d)
#define IDirectPlay3_InitializeConnection(p,a,b)           ICOM_CALL2(InitializeConnection,p,a,b)
#define IDirectPlay3_SecureOpen(p,a,b,c,d)                 ICOM_CALL4(SecureOpen,p,a,b,c,d)
#define IDirectPlay3_SendChatMessage(p,a,b,c,d)            ICOM_CALL4(SendChatMessage,p,a,b,c,d)
#define IDirectPlay3_SetGroupConnectionSettings(p,a,b,c)   ICOM_CALL3(SetGroupConnectionSettings,p,a,b,c)
#define IDirectPlay3_StartSession(p,a,b)                   ICOM_CALL2(StartSession,p,a,b)
#define IDirectPlay3_GetGroupFlags(p,a,b)                  ICOM_CALL2(GetGroupFlags,p,a,b)
#define IDirectPlay3_GetGroupParent(p,a,b)                 ICOM_CALL2(GetGroupParent,p,a,b)
#define IDirectPlay3_GetPlayerAccount(p,a,b,c,d)           ICOM_CALL4(GetPlayerAccount,p,a,b,c,d)
#define IDirectPlay3_GetPlayerFlags(p,a,b)                 ICOM_CALL2(GetPlayerFlags,p,a,b)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_DPLAY_H */
