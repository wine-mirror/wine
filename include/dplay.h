/* Header for Direct Play and Direct Play Lobby */
#ifndef __WINE_DPLAY_H
#define __WINE_DPLAY_H

#pragma pack(1)

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

DEFINE_GUID(IID_IDirectPlay2, 0x2b74f7c0, 0x9154, 0x11cf, 0xa9, 0xcd, 0x0, 0xaa, 0x0, 0x68, 0x86, 0xe3);
DEFINE_GUID(IID_IDirectPlay2A,0x9d460580, 0xa822, 0x11cf, 0x96, 0xc, 0x0, 0x80, 0xc7, 0x53, 0x4e, 0x82);

DEFINE_GUID(IID_IDirectPlay3, 0x133efe40, 0x32dc, 0x11d0, 0x9c, 0xfb, 0x0, 0xa0, 0xc9, 0xa, 0x43, 0xcb);
DEFINE_GUID(IID_IDirectPlay3A,0x133efe41, 0x32dc, 0x11d0, 0x9c, 0xfb, 0x0, 0xa0, 0xc9, 0xa, 0x43, 0xcb);

// {D1EB6D20-8923-11d0-9D97-00A0C90A43CB}
DEFINE_GUID(CLSID_DirectPlay,0xd1eb6d20, 0x8923, 0x11d0, 0x9d, 0x97, 0x0, 0xa0, 0xc9, 0xa, 0x43, 0xcb);


/* {AF465C71-9588-11cf-A020-00AA006157AC} */
DEFINE_GUID(IID_IDirectPlayLobby, 0xaf465c71, 0x9588, 0x11cf, 0xa0, 0x20, 0x0, 0xaa, 0x0, 0x61, 0x57, 0xac);
/* {26C66A70-B367-11cf-A024-00AA006157AC} */
DEFINE_GUID(IID_IDirectPlayLobbyA, 0x26c66a70, 0xb367, 0x11cf, 0xa0, 0x24, 0x0, 0xaa, 0x0, 0x61, 0x57, 0xac);
/* {0194C220-A303-11d0-9C4F-00A0C905425E} */
DEFINE_GUID(IID_IDirectPlayLobby2, 0x194c220, 0xa303, 0x11d0, 0x9c, 0x4f, 0x0, 0xa0, 0xc9, 0x5, 0x42, 0x5e);
/* {1BB4AF80-A303-11d0-9C4F-00A0C905425E} */
DEFINE_GUID(IID_IDirectPlayLobby2A, 0x1bb4af80, 0xa303, 0x11d0, 0x9c, 0x4f, 0x0, 0xa0, 0xc9, 0x5, 0x42, 0x5e);
/* {2FE8F810-B2A5-11d0-A787-0000F803ABFC} */
DEFINE_GUID(CLSID_DirectPlayLobby, 0x2fe8f810, 0xb2a5, 0x11d0, 0xa7, 0x87, 0x0, 0x0, 0xf8, 0x3, 0xab, 0xfc);

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

typedef struct IDirectPlay IDirectPlay, *LPDIRECTPLAY;

/* Direct Play 2 */
typedef struct IDirectPlay2       IDirectPlay2, *LPDIRECTPLAY2;
typedef struct IDirectPlay2       IDirectPlay2A, *LPDIRECTPLAY2A;

/* Direct Play 3 */
typedef struct IDirectPlay3       IDirectPlay3, *LPDIRECTPLAY3;
typedef struct IDirectPlay3       IDirectPlay3A, *LPDIRECTPLAY3A;

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

typedef struct tagDPCHAT
{
    DWORD               dwSize;
    DWORD               dwFlags;
    union
    {                          // Message string
        LPWSTR  lpszMessage;   // Unicode
        LPSTR   lpszMessageA;  // ANSI
    }msgstr;
} DPCHAT, *LPDPCHAT;

typedef struct tagDPSECURITYDESC
{
    DWORD dwSize;                   // Size of structure
    DWORD dwFlags;                  // Not used. Must be zero.
    union
    {                               // SSPI provider name
        LPWSTR  lpszSSPIProvider;   // Unicode
        LPSTR   lpszSSPIProviderA;  // ANSI
    }sspi;
    union
    {                               // CAPI provider name
        LPWSTR lpszCAPIProvider;    // Unicode
        LPSTR  lpszCAPIProviderA;   // ANSI
    }capi;
    DWORD dwCAPIProviderType;       // Crypto Service Provider type
    DWORD dwEncryptionAlgorithm;    // Encryption Algorithm type
} DPSECURITYDESC, *LPDPSECURITYDESC;

typedef const DPSECURITYDESC *LPCDPSECURITYDESC;

typedef struct tagDPCREDENTIALS
{
    DWORD dwSize;               // Size of structure
    DWORD dwFlags;              // Not used. Must be zero.
    union
    {                           // User name of the account
        LPWSTR  lpszUsername;   // Unicode
        LPSTR   lpszUsernameA;  // ANSI
    }name;
    union
    {                           // Password of the account
        LPWSTR  lpszPassword;   // Unicode
        LPSTR   lpszPasswordA;  // ANSI
    }pass;
    union
    {                           // Domain name of the account
        LPWSTR  lpszDomain;     // Unicode
        LPSTR   lpszDomainA;    // ANSI
    }domain;
} DPCREDENTIALS, *LPDPCREDENTIALS;

typedef const DPCREDENTIALS *LPCDPCREDENTIALS;



typedef BOOL32 (CALLBACK* LPDPENUMDPCALLBACKW)(
    LPGUID      lpguidSP,
    LPWSTR      lpSPName,
    DWORD       dwMajorVersion,
    DWORD       dwMinorVersion,
    LPVOID      lpContext);

typedef BOOL32 (CALLBACK* LPDPENUMDPCALLBACKA)(
    LPGUID      lpguidSP,
    LPSTR       lpSPName,       /* ptr to str w/ driver description */
    DWORD       dwMajorVersion, /* Major # of driver spec in lpguidSP */
    DWORD       dwMinorVersion, /* Minor # of driver spec in lpguidSP */ 
    LPVOID      lpContext);     /* User given */

typedef const GUID   *LPCGUID;
typedef const DPNAME *LPCDPNAME;

typedef BOOL32 (CALLBACK* LPDPENUMCONNECTIONSCALLBACK)(
    LPCGUID     lpguidSP,
    LPVOID      lpConnection,
    DWORD       dwConnectionSize,
    LPCDPNAME   lpName,
    DWORD       dwFlags,
    LPVOID      lpContext);

typedef BOOL32 (CALLBACK* LPDPENUMSESSIONSCALLBACK)(
    LPDPSESSIONDESC lpDPSessionDesc,
    LPVOID      lpContext,
    LPDWORD     lpdwTimeOut,
    DWORD       dwFlags);


extern HRESULT WINAPI DirectPlayEnumerateA( LPDPENUMDPCALLBACKA, LPVOID );
extern HRESULT WINAPI DirectPlayEnumerateW( LPDPENUMDPCALLBACKW, LPVOID );
extern HRESULT WINAPI DirectPlayCreate( LPGUID lpGUID, LPDIRECTPLAY2 *lplpDP, IUnknown *pUnk);


/* Direct Play Lobby 1 */
typedef struct IDirectPlayLobby   IDirectPlayLobby, *LPDIRECTPLAYLOBBY;
typedef struct IDirectPlayLobby   IDirectPlayLobbyA, *LPDIRECTPLAYLOBBYA;

/* Direct Play Lobby 2 */
typedef struct IDirectPlayLobby2    IDirectPlayLobby2, *LPDIRECTPLAYLOBBY2;
typedef struct IDirectPlayLobby2    IDirectPlayLobby2A, *LPDIRECTPLAYLOBBY2A;

extern HRESULT WINAPI DirectPlayLobbyCreateW(LPGUID, LPDIRECTPLAYLOBBY *, IUnknown *, LPVOID, DWORD );
extern HRESULT WINAPI DirectPlayLobbyCreateA(LPGUID, LPDIRECTPLAYLOBBYA *, IUnknown *, LPVOID, DWORD );



typedef BOOL32 (CALLBACK* LPDPENUMADDRESSCALLBACK)(
    REFGUID         guidDataType,
    DWORD           dwDataSize,
    LPCVOID         lpData,
    LPVOID          lpContext );

typedef BOOL32 (CALLBACK* LPDPLENUMADDRESSTYPESCALLBACK)(
    REFGUID         guidDataType,
    LPVOID          lpContext,
    DWORD           dwFlags );

typedef BOOL32 (CALLBACK* LPDPLENUMLOCALAPPLICATIONSCALLBACK)(
    LPCDPLAPPINFO   lpAppInfo,
    LPVOID          lpContext,
    DWORD           dwFlags );

typedef BOOL32 (CALLBACK* LPDPENUMPLAYERSCALLBACK2)(
    DPID            dpId,
    DWORD           dwPlayerType,
    LPCDPNAME       lpName,
    DWORD           dwFlags,
    LPVOID          lpContext );

typedef BOOL32 (CALLBACK* LPDPENUMSESSIONSCALLBACK2)(
    LPCDPSESSIONDESC2   lpThisSD,
    LPDWORD             lpdwTimeOut,
    DWORD               dwFlags,
    LPVOID              lpContext );



#define STDMETHOD(xfn) HRESULT (CALLBACK *fn##xfn)
#define STDMETHOD_(ret,xfn) ret (CALLBACK *fn##xfn)
#define PURE
#define FAR
#define THIS_ THIS,

#define THIS LPDIRECTPLAY2 this
typedef struct tagLPDIRECTPLAY2_VTABLE
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS)  PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    /*** IDirectPlay2 methods ***/
    STDMETHOD(AddPlayerToGroup)     (THIS_ DPID, DPID) PURE;
    STDMETHOD(Close)                (THIS) PURE;
    STDMETHOD(CreateGroup)          (THIS_ LPDPID,LPDPNAME,LPVOID,DWORD,DWORD) PURE;
    STDMETHOD(CreatePlayer)         (THIS_ LPDPID,LPDPNAME,HANDLE32,LPVOID,DWORD,DWORD) PURE;
    STDMETHOD(DeletePlayerFromGroup)(THIS_ DPID,DPID) PURE;
    STDMETHOD(DestroyGroup)         (THIS_ DPID) PURE;
    STDMETHOD(DestroyPlayer)        (THIS_ DPID) PURE;
    STDMETHOD(EnumGroupPlayers)     (THIS_ DPID,LPGUID,LPDPENUMPLAYERSCALLBACK2,LPVOID,DWORD) PURE;
    STDMETHOD(EnumGroups)           (THIS_ LPGUID,LPDPENUMPLAYERSCALLBACK2,LPVOID,DWORD) PURE;
    STDMETHOD(EnumPlayers)          (THIS_ LPGUID,LPDPENUMPLAYERSCALLBACK2,LPVOID,DWORD) PURE;
    STDMETHOD(EnumSessions)         (THIS_ LPDPSESSIONDESC2,DWORD,LPDPENUMSESSIONSCALLBACK2,LPVOID,DWORD) PURE;
    STDMETHOD(GetCaps)              (THIS_ LPDPCAPS,DWORD) PURE;
    STDMETHOD(GetGroupData)         (THIS_ DPID,LPVOID,LPDWORD,DWORD) PURE;
    STDMETHOD(GetGroupName)         (THIS_ DPID,LPVOID,LPDWORD) PURE;
    STDMETHOD(GetMessageCount)      (THIS_ DPID, LPDWORD) PURE;
    STDMETHOD(GetPlayerAddress)     (THIS_ DPID,LPVOID,LPDWORD) PURE;
    STDMETHOD(GetPlayerCaps)        (THIS_ DPID,LPDPCAPS,DWORD) PURE;
    STDMETHOD(GetPlayerData)        (THIS_ DPID,LPVOID,LPDWORD,DWORD) PURE;
    STDMETHOD(GetPlayerName)        (THIS_ DPID,LPVOID,LPDWORD) PURE;
    STDMETHOD(GetSessionDesc)       (THIS_ LPVOID,LPDWORD) PURE;
    STDMETHOD(Initialize)           (THIS_ LPGUID) PURE;
    STDMETHOD(Open)                 (THIS_ LPDPSESSIONDESC2,DWORD) PURE;
    STDMETHOD(Receive)              (THIS_ LPDPID,LPDPID,DWORD,LPVOID,LPDWORD) PURE;
    STDMETHOD(Send)                 (THIS_ DPID, DPID, DWORD, LPVOID, DWORD) PURE;
    STDMETHOD(SetGroupData)         (THIS_ DPID,LPVOID,DWORD,DWORD) PURE;
    STDMETHOD(SetGroupName)         (THIS_ DPID,LPDPNAME,DWORD) PURE;
    STDMETHOD(SetPlayerData)        (THIS_ DPID,LPVOID,DWORD,DWORD) PURE;
    STDMETHOD(SetPlayerName)        (THIS_ DPID,LPDPNAME,DWORD) PURE;
    STDMETHOD(SetSessionDesc)       (THIS_ LPDPSESSIONDESC2,DWORD) PURE;
} DIRECTPLAY2_VTABLE, *LPDIRECTPLAY2_VTABLE;
#undef THIS

#define THIS LPDIRECTPLAY3 this
typedef struct tagLPDIRECTPLAY3_VTABLE
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS)  PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    /*** IDirectPlay2 methods ***/
    STDMETHOD(AddPlayerToGroup)     (THIS_ DPID, DPID) PURE;
    STDMETHOD(Close)                (THIS) PURE;
    STDMETHOD(CreateGroup)          (THIS_ LPDPID,LPDPNAME,LPVOID,DWORD,DWORD) PURE;
    STDMETHOD(CreatePlayer)         (THIS_ LPDPID,LPDPNAME,HANDLE32,LPVOID,DWORD,DWORD) PURE;
    STDMETHOD(DeletePlayerFromGroup)(THIS_ DPID,DPID) PURE;
    STDMETHOD(DestroyGroup)         (THIS_ DPID) PURE;
    STDMETHOD(DestroyPlayer)        (THIS_ DPID) PURE;
    STDMETHOD(EnumGroupPlayers)     (THIS_ DPID,LPGUID,LPDPENUMPLAYERSCALLBACK2,LPVOID,DWORD) PURE;
    STDMETHOD(EnumGroups)           (THIS_ LPGUID,LPDPENUMPLAYERSCALLBACK2,LPVOID,DWORD) PURE;
    STDMETHOD(EnumPlayers)          (THIS_ LPGUID,LPDPENUMPLAYERSCALLBACK2,LPVOID,DWORD) PURE;
    STDMETHOD(EnumSessions)         (THIS_ LPDPSESSIONDESC2,DWORD,LPDPENUMSESSIONSCALLBACK2,LPVOID,DWORD) PURE;
    STDMETHOD(GetCaps)              (THIS_ LPDPCAPS,DWORD) PURE;
    STDMETHOD(GetGroupData)         (THIS_ DPID,LPVOID,LPDWORD,DWORD) PURE;
    STDMETHOD(GetGroupName)         (THIS_ DPID,LPVOID,LPDWORD) PURE;
    STDMETHOD(GetMessageCount)      (THIS_ DPID, LPDWORD) PURE;
    STDMETHOD(GetPlayerAddress)     (THIS_ DPID,LPVOID,LPDWORD) PURE;
    STDMETHOD(GetPlayerCaps)        (THIS_ DPID,LPDPCAPS,DWORD) PURE;
    STDMETHOD(GetPlayerData)        (THIS_ DPID,LPVOID,LPDWORD,DWORD) PURE;
    STDMETHOD(GetPlayerName)        (THIS_ DPID,LPVOID,LPDWORD) PURE;
    STDMETHOD(GetSessionDesc)       (THIS_ LPVOID,LPDWORD) PURE;
    STDMETHOD(Initialize)           (THIS_ LPGUID) PURE;
    STDMETHOD(Open)                 (THIS_ LPDPSESSIONDESC2,DWORD) PURE;
    STDMETHOD(Receive)              (THIS_ LPDPID,LPDPID,DWORD,LPVOID,LPDWORD) PURE;
    STDMETHOD(Send)                 (THIS_ DPID, DPID, DWORD, LPVOID, DWORD) PURE;
    STDMETHOD(SetGroupData)         (THIS_ DPID,LPVOID,DWORD,DWORD) PURE;
    STDMETHOD(SetGroupName)         (THIS_ DPID,LPDPNAME,DWORD) PURE;
    STDMETHOD(SetPlayerData)        (THIS_ DPID,LPVOID,DWORD,DWORD) PURE;
    STDMETHOD(SetPlayerName)        (THIS_ DPID,LPDPNAME,DWORD) PURE;
    STDMETHOD(SetSessionDesc)       (THIS_ LPDPSESSIONDESC2,DWORD) PURE;

    /*** IDirectPlay3 methods ***/
    STDMETHOD(AddGroupToGroup)            (THIS_ DPID, DPID) PURE;
    STDMETHOD(CreateGroupInGroup)         (THIS_ DPID,LPDPID,LPDPNAME,LPVOID,DWORD,DWORD) PURE;
    STDMETHOD(DeleteGroupFromGroup)       (THIS_ DPID,DPID) PURE;
    STDMETHOD(EnumConnections)            (THIS_ LPCGUID,LPDPENUMCONNECTIONSCALLBACK,LPVOID,DWORD) PURE;
    STDMETHOD(EnumGroupsInGroup)          (THIS_ DPID,LPGUID,LPDPENUMPLAYERSCALLBACK2,LPVOID,DWORD) PURE;
    STDMETHOD(GetGroupConnectionSettings) (THIS_ DWORD, DPID, LPVOID, LPDWORD) PURE;
    STDMETHOD(InitializeConnection)       (THIS_ LPVOID,DWORD) PURE;
    STDMETHOD(SecureOpen)                 (THIS_ LPCDPSESSIONDESC2,DWORD,LPCDPSECURITYDESC,LPCDPCREDENTIALS) PURE;
    STDMETHOD(SendChatMessage)            (THIS_ DPID,DPID,DWORD,LPDPCHAT);
    STDMETHOD(SetGroupConnectionSettings) (THIS_ DWORD,DPID,LPDPLCONNECTION) PURE;
    STDMETHOD(StartSession)               (THIS_ DWORD,DPID);
    STDMETHOD(GetGroupFlags)              (THIS_ DPID,LPDWORD);
    STDMETHOD(GetGroupParent)             (THIS_ DPID,LPDPID);
    STDMETHOD(GetPlayerAccount)           (THIS_ DPID, DWORD, LPVOID, LPDWORD) PURE;
    STDMETHOD(GetPlayerFlags)             (THIS_ DPID,LPDWORD);
} DIRECTPLAY3_VTABLE, *LPDIRECTPLAY3_VTABLE;
#undef THIS


/**********************************************************************************
 *
 * Macros for a nicer interface to DirectPlay 
 *
 **********************************************************************************/

/* COM Interface */
#define IDirectPlay_QueryInterface(p,a,b)           (p)->lpVtbl->fnQueryInterface(p,a,b)
#define IDirectPlay_AddRef(p)                       (p)->lpVtbl->fnAddRef(p)
#define IDirectPlay_Release(p)                      (p)->lpVtbl->fnRelease(p)

#define IDirectPlay2_QueryInterface(p,a,b)          (p)->lpVtbl->fnQueryInterface(p,a,b)
#define IDirectPlay2_AddRef(p)                      (p)->lpVtbl->fnAddRef(p)
#define IDirectPlay2_Release(p)                     (p)->lpVtbl->fnRelease(p)

#define IDirectPlay3_QueryInterface(p,a,b)          (p)->lpVtbl->fnQueryInterface(p,a,b)
#define IDirectPlay3_AddRef(p)                      (p)->lpVtbl->fnAddRef(p)
#define IDirectPlay3_Release(p)                     (p)->lpVtbl->fnRelease(p)

/* Direct Play 1&2 Interface */
#define IDirectPlay_AddPlayerToGroup(p,a,b)         (p)->lpVtbl->fnAddPlayerToGroup(p,a,b)
#define IDirectPlay_Close(p)                        (p)->lpVtbl->fnClose(p)
#define IDirectPlay_CreateGroup(p,a,b,c)            (p)->lpVtbl->fnCreateGroup(p,a,b,c)
#define IDirectPlay_CreatePlayer(p,a,b,c,d)         (p)->lpVtbl->fnCreatePlayer(p,a,b,c,d)
#define IDirectPlay_DeletePlayerFromGroup(p,a,b)    (p)->lpVtbl->fnDeletePlayerFromGroup(p,a,b)
#define IDirectPlay_DestroyGroup(p,a)               (p)->lpVtbl->fnDestroyGroup(p,a)
#define IDirectPlay_DestroyPlayer(p,a)              (p)->lpVtbl->fnDestroyPlayer(p,a)
#define IDirectPlay_EnableNewPlayers(p,a)           (p)->lpVtbl->fnEnableNewPlayers(p,a)
#define IDirectPlay_EnumGroupPlayers(p,a,b,c,d)     (p)->lpVtbl->fnEnumGroupPlayers(p,a,b,c,d)
#define IDirectPlay_EnumGroups(p,a,b,c,d)           (p)->lpVtbl->fnEnumGroups(p,a,b,c,d)
#define IDirectPlay_EnumPlayers(p,a,b,c,d)          (p)->lpVtbl->fnEnumPlayers(p,a,b,c,d)
#define IDirectPlay_EnumSessions(p,a,b,c,d,e)       (p)->lpVtbl->fnEnumSessions(p,a,b,c,d,e)
#define IDirectPlay_GetCaps(p,a)                    (p)->lpVtbl->fnGetCaps(p,a)
#define IDirectPlay_GetMessageCount(p,a,b)          (p)->lpVtbl->fnGetMessageCount(p,a,b)
#define IDirectPlay_GetPlayerCaps(p,a,b)            (p)->lpVtbl->fnGetPlayerCaps(p,a,b)
#define IDirectPlay_GetPlayerName(p,a,b,c,d,e)      (p)->lpVtbl->fnGetPlayerName(p,a,b,c,d,e)
#define IDirectPlay_Initialize(p,a)                 (p)->lpVtbl->fnInitialize(p,a)
#define IDirectPlay_Open(p,a)                       (p)->lpVtbl->fnOpen(p,a)
#define IDirectPlay_Receive(p,a,b,c,d,e)            (p)->lpVtbl->fnReceive(p,a,b,c,d,e)
#define IDirectPlay_SaveSession(p,a)                (p)->lpVtbl->fnSaveSession(p,a)
#define IDirectPlay_Send(p,a,b,c,d,e)               (p)->lpVtbl->fnSend(p,a,b,c,d,e)
#define IDirectPlay_SetPlayerName(p,a,b,c)          (p)->lpVtbl->fnSetPlayerName(p,a,b,c)

#define IDirectPlay2_AddPlayerToGroup(p,a,b)        (p)->lpVtbl->fnAddPlayerToGroup(p,a,b)
#define IDirectPlay2_Close(p)                       (p)->lpVtbl->fnClose(p)
#define IDirectPlay2_CreateGroup(p,a,b,c,d,e)       (p)->lpVtbl->fnCreateGroup(p,a,b,c,d,e)
#define IDirectPlay2_CreatePlayer(p,a,b,c,d,e,f)    (p)->lpVtbl->fnCreatePlayer(p,a,b,c,d,e,f)
#define IDirectPlay2_DeletePlayerFromGroup(p,a,b)   (p)->lpVtbl->fnDeletePlayerFromGroup(p,a,b)
#define IDirectPlay2_DestroyGroup(p,a)              (p)->lpVtbl->fnDestroyGroup(p,a)
#define IDirectPlay2_DestroyPlayer(p,a)             (p)->lpVtbl->fnDestroyPlayer(p,a)
#define IDirectPlay2_EnumGroupPlayers(p,a,b,c,d,e)  (p)->lpVtbl->fnEnumGroupPlayers(p,a,b,c,d,e)
#define IDirectPlay2_EnumGroups(p,a,b,c,d)          (p)->lpVtbl->fnEnumGroups(p,a,b,c,d)
#define IDirectPlay2_EnumPlayers(p,a,b,c,d)         (p)->lpVtbl->fnEnumPlayers(p,a,b,c,d)
#define IDirectPlay2_EnumSessions(p,a,b,c,d,e)      (p)->lpVtbl->fnEnumSessions(p,a,b,c,d,e)
#define IDirectPlay2_GetCaps(p,a,b)                 (p)->lpVtbl->fnGetCaps(p,a,b)
#define IDirectPlay2_GetMessageCount(p,a,b)         (p)->lpVtbl->fnGetMessageCount(p,a,b)
#define IDirectPlay2_GetGroupData(p,a,b,c,d)        (p)->lpVtbl->fnGetGroupData(p,a,b,c,d)
#define IDirectPlay2_GetGroupName(p,a,b,c)          (p)->lpVtbl->fnGetGroupName(p,a,b,c)
#define IDirectPlay2_GetPlayerAddress(p,a,b,c)      (p)->lpVtbl->fnGetPlayerAddress(p,a,b,c)
#define IDirectPlay2_GetPlayerCaps(p,a,b,c)         (p)->lpVtbl->fnGetPlayerCaps(p,a,b,c)
#define IDirectPlay2_GetPlayerData(p,a,b,c,d)       (p)->lpVtbl->fnGetPlayerData(p,a,b,c,d)
#define IDirectPlay2_GetPlayerName(p,a,b,c)         (p)->lpVtbl->fnGetPlayerName(p,a,b,c)
#define IDirectPlay2_GetSessionDesc(p,a,b)          (p)->lpVtbl->fnGetSessionDesc(p,a,b)
#define IDirectPlay2_Initialize(p,a)                (p)->lpVtbl->fnInitialize(p,a)
#define IDirectPlay2_Open(p,a,b)                    (p)->lpVtbl->fnOpen(p,a,b)
#define IDirectPlay2_Receive(p,a,b,c,d,e)           (p)->lpVtbl->fnReceive(p,a,b,c,d,e)
#define IDirectPlay2_Send(p,a,b,c,d,e)              (p)->lpVtbl->fnSend(p,a,b,c,d,e)
#define IDirectPlay2_SetGroupData(p,a,b,c,d)        (p)->lpVtbl->fnSetGroupData(p,a,b,c,d)
#define IDirectPlay2_SetGroupName(p,a,b,c)          (p)->lpVtbl->fnSetGroupName(p,a,b,c)
#define IDirectPlay2_SetPlayerData(p,a,b,c,d)       (p)->lpVtbl->fnSetPlayerData(p,a,b,c,d)
#define IDirectPlay2_SetPlayerName(p,a,b,c)         (p)->lpVtbl->fnSetPlayerName(p,a,b,c)
#define IDirectPlay2_SetSessionDesc(p,a,b)          (p)->lpVtbl->fnSetSessionDesc(p,a,b)

#define IDirectPlay3_AddPlayerToGroup(p,a,b)        (p)->lpVtbl->fnAddPlayerToGroup(p,a,b)
#define IDirectPlay3_Close(p)                       (p)->lpVtbl->fnClose(p)
#define IDirectPlay3_CreateGroup(p,a,b,c,d,e)       (p)->lpVtbl->fnCreateGroup(p,a,b,c,d,e)
#define IDirectPlay3_CreatePlayer(p,a,b,c,d,e,f)    (p)->lpVtbl->fnCreatePlayer(p,a,b,c,d,e,f)
#define IDirectPlay3_DeletePlayerFromGroup(p,a,b)   (p)->lpVtbl->fnDeletePlayerFromGroup(p,a,b)
#define IDirectPlay3_DestroyGroup(p,a)              (p)->lpVtbl->fnDestroyGroup(p,a)
#define IDirectPlay3_DestroyPlayer(p,a)             (p)->lpVtbl->fnDestroyPlayer(p,a)
#define IDirectPlay3_EnumGroupPlayers(p,a,b,c,d,e)  (p)->lpVtbl->fnEnumGroupPlayers(p,a,b,c,d,e)
#define IDirectPlay3_EnumGroups(p,a,b,c,d)          (p)->lpVtbl->fnEnumGroups(p,a,b,c,d)
#define IDirectPlay3_EnumPlayers(p,a,b,c,d)         (p)->lpVtbl->fnEnumPlayers(p,a,b,c,d)
#define IDirectPlay3_EnumSessions(p,a,b,c,d,e)      (p)->lpVtbl->fnEnumSessions(p,a,b,c,d,e)
#define IDirectPlay3_GetCaps(p,a,b)                 (p)->lpVtbl->fnGetCaps(p,a,b)
#define IDirectPlay3_GetMessageCount(p,a,b)         (p)->lpVtbl->fnGetMessageCount(p,a,b)
#define IDirectPlay3_GetGroupData(p,a,b,c,d)        (p)->lpVtbl->fnGetGroupData(p,a,b,c,d)
#define IDirectPlay3_GetGroupName(p,a,b,c)          (p)->lpVtbl->fnGetGroupName(p,a,b,c)
#define IDirectPlay3_GetPlayerAddress(p,a,b,c)      (p)->lpVtbl->fnGetPlayerAddress(p,a,b,c)
#define IDirectPlay3_GetPlayerCaps(p,a,b,c)         (p)->lpVtbl->fnGetPlayerCaps(p,a,b,c)
#define IDirectPlay3_GetPlayerData(p,a,b,c,d)       (p)->lpVtbl->fnGetPlayerData(p,a,b,c,d)
#define IDirectPlay3_GetPlayerName(p,a,b,c)         (p)->lpVtbl->fnGetPlayerName(p,a,b,c)
#define IDirectPlay3_GetSessionDesc(p,a,b)          (p)->lpVtbl->fnGetSessionDesc(p,a,b)
#define IDirectPlay3_Initialize(p,a)                (p)->lpVtbl->fnInitialize(p,a)
#define IDirectPlay3_Open(p,a,b)                    (p)->lpVtbl->fnOpen(p,a,b)
#define IDirectPlay3_Receive(p,a,b,c,d,e)           (p)->lpVtbl->fnReceive(p,a,b,c,d,e)
#define IDirectPlay3_Send(p,a,b,c,d,e)              (p)->lpVtbl->fnSend(p,a,b,c,d,e)
#define IDirectPlay3_SetGroupData(p,a,b,c,d)        (p)->lpVtbl->fnSetGroupData(p,a,b,c,d)
#define IDirectPlay3_SetGroupName(p,a,b,c)          (p)->lpVtbl->fnSetGroupName(p,a,b,c)
#define IDirectPlay3_SetPlayerData(p,a,b,c,d)       (p)->lpVtbl->fnSetPlayerData(p,a,b,c,d)
#define IDirectPlay3_SetPlayerName(p,a,b,c)         (p)->lpVtbl->fnSetPlayerName(p,a,b,c)
#define IDirectPlay3_SetSessionDesc(p,a,b)          (p)->lpVtbl->fnSetSessionDesc(p,a,b)


/* Direct Play 3 Interface. */

#define IDirectPlay3_AddGroupToGroup(p,a,b)             (p)->lpVtbl->fnAddGroupToGroup(p,a,b)
#define IDirectPlay3_CreateGroupInGroup(p,a,b,c,d,e,f) (p)->lpVtbl->fnCreateGroupInGroup(p,a,b,c,d,e,f)
#define IDirectPlay3_DeleteGroupFromGroup(p,a,b)        (p)->lpVtbl->fnDeleteGroupFromGroup(p,a,b)
#define IDirectPlay3_EnumConnections(p,a,b,c,d)         (p)->lpVtbl->fnEnumConnections(p,a,b,c,d)
#define IDirectPlay3_EnumGroupsInGroup(p,a,b,c,d,e) (p)->lpVtbl->fnEnumGroupsInGroup(p,a,b,c,d,e)
#define IDirectPlay3_GetGroupConnectionSettings(p,a,b,c,d) (p)->lpVtbl->fnGetGroupConnectionSettings(p,a,b,c,d)
#define IDirectPlay3_InitializeConnection(p,a,b)        (p)->lpVtbl->fnInitializeConnection(p,a,b)
#define IDirectPlay3_SecureOpen(p,a,b,c,d)          (p)->lpVtbl->fnSecureOpen(p,a,b,c,d)
#define IDirectPlay3_SendChatMessage(p,a,b,c,d)     (p)->lpVtbl->fnSendChatMessage(p,a,b,c,d)
#define IDirectPlay3_SetGroupConnectionSettings(p,a,b,c) (p)->lpVtbl->fnSetGroupConnectionSettings(p,a,b,c)
#define IDirectPlay3_StartSession(p,a,b)            (p)->lpVtbl->fnStartSession(p,a,b)
#define IDirectPlay3_GetGroupFlags(p,a,b)           (p)->lpVtbl->fnGetGroupFlags(p,a,b)
#define IDirectPlay3_GetGroupParent(p,a,b)          (p)->lpVtbl->fnGetGroupParent(p,a,b)
#define IDirectPlay3_GetPlayerAccount(p,a,b,c,d)    (p)->lpVtbl->fnGetPlayerAccount(p,a,b,c,d)
#define IDirectPlay3_GetPlayerFlags(p,a,b)          (p)->lpVtbl->fnGetPlayerFlags(p,a,b)


/****************************************************************************
 *
 * DIRECT PLAY LOBBY VIRTUAL TABLE DEFINITIONS 
 *
 ****************************************************************************/


#define THIS LPDIRECTPLAYLOBBY this
typedef struct tagLPDIRECTPLAYLOBBY_VTABLE 
{
    /*  IUnknown Methods "Inherited Methods" */
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    /*  IDirectPlayLobby Methods */
    STDMETHOD(Connect)              (THIS_ DWORD, LPDIRECTPLAY2 *, IUnknown *) PURE;
    STDMETHOD(CreateAddress)        (THIS_ REFGUID, REFGUID, LPCVOID, DWORD, LPVOID, LPDWORD) PURE;       
    STDMETHOD(EnumAddress)          (THIS_ LPDPENUMADDRESSCALLBACK, LPCVOID, DWORD, LPVOID) PURE;         
    STDMETHOD(EnumAddressTypes)     (THIS_ LPDPLENUMADDRESSTYPESCALLBACK, REFGUID, LPVOID, DWORD) PURE;   
    STDMETHOD(EnumLocalApplications)(THIS_ LPDPLENUMLOCALAPPLICATIONSCALLBACK, LPVOID, DWORD) PURE;       
    STDMETHOD(GetConnectionSettings)(THIS_ DWORD, LPVOID, LPDWORD) PURE;                                  
    STDMETHOD(ReceiveLobbyMessage)  (THIS_ DWORD, DWORD, LPDWORD, LPVOID, LPDWORD) PURE;                  
    STDMETHOD(RunApplication)       (THIS_ DWORD, LPDWORD, LPDPLCONNECTION, HANDLE32) PURE;               
    STDMETHOD(SendLobbyMessage)     (THIS_ DWORD, DWORD, LPVOID, DWORD) PURE;                             
    STDMETHOD(SetConnectionSettings)(THIS_ DWORD, DWORD, LPDPLCONNECTION) PURE;                           
    STDMETHOD(SetLobbyMessageEvent) (THIS_ DWORD, DWORD, HANDLE32) PURE;


} DIRECTPLAYLOBBY_VTABLE, *LPDIRECTPLAYLOBBY_VTABLE;
#undef THIS

#define THIS LPDIRECTPLAYLOBBY2 this
typedef struct tagLPDIRECTPLAYLOBBY2_VTABLE 
{
    /*  IUnknown Methods "Inherited Methods" */
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    /*  IDirectPlayLobby Methods */
    STDMETHOD(Connect)              (THIS_ DWORD, LPDIRECTPLAY2 *, IUnknown *) PURE;
    STDMETHOD(CreateAddress)        (THIS_ REFGUID, REFGUID, LPCVOID, DWORD, LPVOID, LPDWORD) PURE;       
    STDMETHOD(EnumAddress)          (THIS_ LPDPENUMADDRESSCALLBACK, LPCVOID, DWORD, LPVOID) PURE;         
    STDMETHOD(EnumAddressTypes)     (THIS_ LPDPLENUMADDRESSTYPESCALLBACK, REFGUID, LPVOID, DWORD) PURE;   
    STDMETHOD(EnumLocalApplications)(THIS_ LPDPLENUMLOCALAPPLICATIONSCALLBACK, LPVOID, DWORD) PURE;       
    STDMETHOD(GetConnectionSettings)(THIS_ DWORD, LPVOID, LPDWORD) PURE;                                  
    STDMETHOD(ReceiveLobbyMessage)  (THIS_ DWORD, DWORD, LPDWORD, LPVOID, LPDWORD) PURE;                  
    STDMETHOD(RunApplication)       (THIS_ DWORD, LPDWORD, LPDPLCONNECTION, HANDLE32) PURE;               
    STDMETHOD(SendLobbyMessage)     (THIS_ DWORD, DWORD, LPVOID, DWORD) PURE;                             
    STDMETHOD(SetConnectionSettings)(THIS_ DWORD, DWORD, LPDPLCONNECTION) PURE;                           
    STDMETHOD(SetLobbyMessageEvent) (THIS_ DWORD, DWORD, HANDLE32) PURE;

    /*  IDirectPlayLobby2 Methods */
    STDMETHOD(CreateCompoundAddress)(THIS_ LPCDPCOMPOUNDADDRESSELEMENT, DWORD, LPVOID, LPDWORD) PURE;

} DIRECTPLAYLOBBY2_VTABLE, *LPDIRECTPLAYLOBBY2_VTABLE;
#undef THIS

/**********************************************************************************
 *
 * Macros for a nicer interface to DirectPlayLobby
 *
 **********************************************************************************/

/* COM Interface */

#define IDirectPlayLobby_QueryInterface(p,a,b)              (p)->lpVtbl->fnQueryInterface(p,a,b)
#define IDirectPlayLobby_AddRef(p)                          (p)->lpVtbl->fnAddRef(p)
#define IDirectPlayLobby_Release(p)                         (p)->lpVtbl->fnRelease(p)

#define IDirectPlayLobby2_QueryInterface(p,a,b)             (p)->lpVtbl->fnQueryInterface(p,a,b)
#define IDirectPlayLobby2_AddRef(p)                         (p)->lpVtbl->fnAddRef(p)
#define IDirectPlayLobby2_Release(p)                        (p)->lpVtbl->fnRelease(p)

/* Direct Play Lobby 1 */

#define IDirectPlayLobby_Connect(p,a,b,c)                   (p)->lpVtbl->fnConnect(p,a,b,c)
#define IDirectPlayLobby_CreateAddress(p,a,b,c,d,e,f)       (p)->lpVtbl->fnCreateAddress(p,a,b,c,d,e,f)
#define IDirectPlayLobby_EnumAddress(p,a,b,c,d)             (p)->lpVtbl->fnEnumAddress(p,a,b,c,d)
#define IDirectPlayLobby_EnumAddressTypes(p,a,b,c,d)        (p)->lpVtbl->fnEnumAddressTypes(p,a,b,c,d)
#define IDirectPlayLobby_EnumLocalApplications(p,a,b,c)     (p)->lpVtbl->fnEnumLocalApplications(p,a,b,c)
#define IDirectPlayLobby_GetConnectionSettings(p,a,b,c)     (p)->lpVtbl->fnGetConnectionSettings(p,a,b,c)
#define IDirectPlayLobby_ReceiveLobbyMessage(p,a,b,c,d,e)   (p)->lpVtbl->fnReceiveLobbyMessage(p,a,b,c,d,e)
#define IDirectPlayLobby_RunApplication(p,a,b,c,d)          (p)->lpVtbl->fnRunApplication(p,a,b,c,d)
#define IDirectPlayLobby_SendLobbyMessage(p,a,b,c,d)        (p)->lpVtbl->fnSendLobbyMessage(p,a,b,c,d)
#define IDirectPlayLobby_SetConnectionSettings(p,a,b,c)     (p)->lpVtbl->fnSetConnectionSettings(p,a,b,c)
#define IDirectPlayLobby_SetLobbyMessageEvent(p,a,b,c)      (p)->lpVtbl->fnSetLobbyMessageEvent(p,a,b,c)

#define IDirectPlayLobby2_Connect(p,a,b,c)                  (p)->lpVtbl->fnConnect(p,a,b,c)
#define IDirectPlayLobby2_CreateAddress(p,a,b,c,d,e,f)      (p)->lpVtbl->fnCreateAddress(p,a,b,c,d,e,f)
#define IDirectPlayLobby2_EnumAddress(p,a,b,c,d)            (p)->lpVtbl->fnEnumAddress(p,a,b,c,d)
#define IDirectPlayLobby2_EnumAddressTypes(p,a,b,c,d)       (p)->lpVtbl->fnEnumAddressTypes(p,a,b,c,d)
#define IDirectPlayLobby2_EnumLocalApplications(p,a,b,c)    (p)->lpVtbl->fnEnumLocalApplications(p,a,b,c)
#define IDirectPlayLobby2_GetConnectionSettings(p,a,b,c)    (p)->lpVtbl->fnGetConnectionSettings(p,a,b,c)
#define IDirectPlayLobby2_ReceiveLobbyMessage(p,a,b,c,d,e)  (p)->lpVtbl->fnReceiveLobbyMessage(p,a,b,c,d,e)
#define IDirectPlayLobby2_RunApplication(p,a,b,c,d)         (p)->lpVtbl->fnRunApplication(p,a,b,c,d)
#define IDirectPlayLobby2_SendLobbyMessage(p,a,b,c,d)       (p)->lpVtbl->fnSendLobbyMessage(p,a,b,c,d)
#define IDirectPlayLobby2_SetConnectionSettings(p,a,b,c)    (p)->lpVtbl->fnSetConnectionSettings(p,a,b,c)
#define IDirectPlayLobby2_SetLobbyMessageEvent(p,a,b,c)     (p)->lpVtbl->fnSetLobbyMessageEvent(p,a,b,c)


/* Direct Play Lobby 2 */

#define IDirectPlayLobby2_CreateCompoundAddress(p,a,b,c,d)   (p)->lpVtbl->fnCreateCompoundAddress(p,a,b,c,d)

#pragma pack(4)

#undef STDMETHOD
#undef STDMETHOD_
#undef PURE
#undef FAR
#undef THIS_

#endif
