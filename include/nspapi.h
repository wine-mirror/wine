/* NSPAPI.H -- winsock 1.1
 * not supported on win95
 */

#ifndef _WINE_NSPAPI_
#define _WINE_NSPAPI_

#include "windef.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */
/*
 * constants
 */
#define XP_CONNECTIONLESS		0x00000001
#define XP_GUARANTEED_DELIVERY		0x00000002
#define XP_GUARANTEED_ORDER		0x00000004
#define XP_MESSAGE_ORIENTED		0x00000008
#define XP_PSEUDO_STREAM		0x00000010
#define XP_GRACEFUL_CLOSE		0x00000020
#define XP_EXPEDITED_DATA		0x00000040
#define XP_CONNECT_DATA			0x00000080
#define XP_DISCONNECT_DATA		0x00000100
#define XP_SUPPORTS_BROADCAST		0x00000200
#define XP_SUPPORTS_MULTICAST		0x00000400
#define XP_BANDWITH_ALLOCATION		0x00000800
#define XP_FRAGMENTATION		0x00001000
#define XP_ENCRYPTS			0x00002000

/*
 * structures
 */
typedef  struct _PROTOCOL_INFOA 
{
         DWORD   dwServiceFlags;
         INT     iAddressFamily;
         INT     iMaxSockAddr;
         INT     iMinSockAddr;
         INT     iSocketType;
         INT     iProtocol;
         DWORD   dwMessageSize;
         LPSTR   lpProtocol;
} PROTOCOL_INFOA;

typedef  struct _PROTOCOL_INFOW 
{
         DWORD   dwServiceFlags;
         INT     iAddressFamily;
         INT     iMaxSockAddr;
         INT     iMinSockAddr;
         INT     iSocketType;
         INT     iProtocol;
         DWORD   dwMessageSize;
         LPWSTR  lpProtocol;
} PROTOCOL_INFOW;


/*
 * function prototypes
 */



#ifdef __cplusplus
}      /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* _WINE_NSPAPI_ */
