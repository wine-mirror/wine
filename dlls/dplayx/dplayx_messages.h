
#ifndef __WINE_DPLAYX_MESSAGES__
#define __WINE_DPLAYX_MESSAGES__

#include "windef.h"
#include "dplay.h"
#include "rpc.h" /* For GUID */

DWORD CreateLobbyMessageReceptionThread( HANDLE hNotifyEvent, HANDLE hStart, 
                                         HANDLE hDeath, HANDLE hConnRead );


/* Message types etc. */
#include "pshpack1.h"

/* Non provided messages for DPLAY - guess work which may be wrong :( */
#define DPMSGCMD_ENUMSESSIONSREPLY    1
#define DPMSGCMD_ENUMSESSIONSREQUEST  2

#define DPMSGCMD_GETSETNAMETABLE      3 /* Request info from NS about 
                                           existing players/groups etc. Is
                                           also used for reply */

#define DPMSGCMD_REQUESTNEWPLAYERID   5

#define DPMSGCMD_NEWPLAYERIDREPLY     7
#define DPMSGCMD_CREATESESSION        8  
#define DPMSGCMD_CREATENEWPLAYER      9
#define DPMSGCMD_SYSTEMMESSAGE        10 

#define DPMSGCMD_DELETEGROUP          12

#define DPMSGCMD_ENUMGROUPS           17

/* This is what DP 6 defines it as. Don't know what it means. All messages
 * defined below are DPMSGVER_DP6.
 */
#define DPMSGVER_DP6 11

/* MAGIC number at the start of all dplay packets ("play" in ASCII) */
#define DPMSGMAGIC_DPLAYMSG  0x79616c70

/* All messages sent from the system are sent with this at the beginning of
 * the message.
 */

/* Size is 4 bytes */
typedef struct tagDPMSG_SENDENVELOPE
{
  DWORD dwMagic;
  WORD  wCommandId;
  WORD  wVersion;
} DPMSG_SENDENVELOPE, *LPDPMSG_SENDENVELOPE;
typedef const DPMSG_SENDENVELOPE* LPCDPMSG_SENDENVELOPE;

typedef struct tagDPMSG_SYSMSGENVELOPE
{
  DWORD dwPlayerFrom;
  DWORD dwPlayerTo;
} DPMSG_SYSMSGENVELOPE, *LPDPMSG_SYSMSGENVELOPE;
typedef const DPMSG_SYSMSGENVELOPE* LPCDPMSG_SYSMSGENVELOPE;


typedef struct tagDPMSG_ENUMSESSIONSREPLY
{
  DPMSG_SENDENVELOPE envelope;

#if 0
  DWORD dwSize;  /* Size of DPSESSIONDESC2 struct */
  DWORD dwFlags; /* Sessions flags */

  GUID guidInstance; /* Not 100% sure this is what it is... */

  GUID guidApplication;

  DWORD dwMaxPlayers;
  DWORD dwCurrentPlayers;

  BYTE unknown[36];
#else
  DPSESSIONDESC2 sd;
#endif

  DWORD dwUnknown;  /* Seems to be equal to 0x5c which is a "\\" */
                    /* Encryption package string? */

  /* At the end we have ... */
  /* WCHAR wszSessionName[1];  Var length with NULL terminal */

} DPMSG_ENUMSESSIONSREPLY, *LPDPMSG_ENUMSESSIONSREPLY;
typedef const DPMSG_ENUMSESSIONSREPLY* LPCDPMSG_ENUMSESSIONSREPLY;

typedef struct tagDPMSG_ENUMSESSIONSREQUEST
{
  DPMSG_SENDENVELOPE envelope;

  GUID  guidApplication;

  DWORD dwPasswordSize; /* A Guess. This is normally 0x00000000. */
                        /* This might be the name server DPID which
                           is needed for the reply */

  DWORD dwFlags; /* dwFlags from EnumSessions */

} DPMSG_ENUMSESSIONSREQUEST, *LPDPMSG_ENUMSESSIONSREQUEST;
typedef const DPMSG_ENUMSESSIONSREQUEST* LPCDPMSG_ENUMSESSIONSREQUEST;

/* Size is 146 received - with 18 or 20 bytes header = ~128 bytes */
typedef struct tagDPMSG_CREATESESSION
{
  DPMSG_SENDENVELOPE envelope;
} DPMSG_CREATESESSION, *LPDPMSG_CREATESESSION;
typedef const DPMSG_CREATESESSION* LPCDPMSG_CREATESESSION;

/* 28 bytes - ~18 header ~= 10 bytes msg */
typedef struct tagDPMSG_REQUESTNEWPLAYERID
{
  DPMSG_SENDENVELOPE envelope;

  

} DPMSG_REQUESTNEWPLAYERID, *LPDPMSG_REQUESTNEWPLAYERID;
typedef const DPMSG_REQUESTNEWPLAYERID* LPCDPMSG_REQUESTNEWPLAYERID;

/* 64 byte - ~18 header ~= 46 bytes msg */
typedef struct tagDPMSG_NEWPLAYERIDREPLY
{
  DPMSG_SENDENVELOPE envelope;

} DPMSG_NEWPLAYERIDREPLY, *LPDPMSG_NEWPLAYERIDREPLY;
typedef const DPMSG_NEWPLAYERIDREPLY* LPCDPMSG_NEWPLAYERIDREPLY;


#include "poppack.h"

#endif
