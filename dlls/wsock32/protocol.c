/*
 * WSOCK32 specific functions
 *
 * Copyright (C) 2001 Stefan Leichter
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* All we need are a couple constants for EnumProtocols. Once it is
 * moved to ws2_32 we may no longer need it
 */
#define USE_WS_PREFIX

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wtypes.h"
#include "nspapi.h"
#include "winsock2.h"
#include "wsipx.h"
#include "wshisotp.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winsock);

/* name of the protocols
 */
static WCHAR NameIpx[]   = {'I', 'P', 'X', '\0'};
static WCHAR NameSpx[]   = {'S', 'P', 'X', '\0'};
static WCHAR NameSpxII[] = {'S', 'P', 'X', ' ', 'I', 'I', '\0'};
static WCHAR NameTcp[]   = {'T', 'C', 'P', '/', 'I', 'P', '\0'};
static WCHAR NameUdp[]   = {'U', 'D', 'P', '/', 'I', 'P', '\0'};

/*****************************************************************************
 *          WSOCK32_EnterSingleProtocol [internal]
 *
 *    enters the protocol informations of one given protocol into the
 *    given buffer. If the given buffer is too small only the required size for
 *    the protocols are returned.
 *
 * RETURNS
 *    The number of protocols entered into the buffer
 *
 * BUGS
 *    - only implemented for IPX, SPX, SPXII, TCP, UDP
 *    - there is no check that the operating system supports the returned
 *      protocols
 */
static INT WSOCK32_EnterSingleProtocol( INT iProtocol,
                                        PROTOCOL_INFOA* lpBuffer,
                                        LPDWORD lpSize, BOOL unicode)
{ DWORD  dwLength = 0, dwOldSize = *lpSize;
  INT    iAnz = 1;
  WCHAR  *lpProtName = NULL;

  *lpSize = sizeof( PROTOCOL_INFOA);
  switch (iProtocol) {
    case WS_IPPROTO_TCP :
        dwLength = (unicode) ? sizeof(WCHAR) * (strlenW(NameTcp)+1) :
                               WideCharToMultiByte( CP_ACP, 0, NameTcp, -1,
                                                    NULL, 0, NULL, NULL);
      break;
    case WS_IPPROTO_UDP :
        dwLength = (unicode) ? sizeof(WCHAR) * (strlenW(NameUdp)+1) :
                               WideCharToMultiByte( CP_ACP, 0, NameUdp, -1,
                                                    NULL, 0, NULL, NULL);
      break;
    case NSPROTO_IPX :
        dwLength = (unicode) ? sizeof(WCHAR) * (strlenW(NameIpx)+1) :
                               WideCharToMultiByte( CP_ACP, 0, NameIpx, -1,
                                                    NULL, 0, NULL, NULL);
      break;
    case NSPROTO_SPX :
        dwLength = (unicode) ? sizeof(WCHAR) * (strlenW(NameSpx)+1) :
                               WideCharToMultiByte( CP_ACP, 0, NameSpx, -1,
                                                    NULL, 0, NULL, NULL);
      break;
    case NSPROTO_SPXII :
        dwLength = (unicode) ? sizeof(WCHAR) * (strlenW(NameSpxII)+1) :
                               WideCharToMultiByte( CP_ACP, 0, NameSpxII, -1,
                                                    NULL, 0, NULL, NULL);
      break;
    default:
        *lpSize = 0;
        if ((iProtocol == ISOPROTO_TP4) || (iProtocol == NSPROTO_SPX))
          FIXME("Protocol <%s> not implemented\n",
                (iProtocol == ISOPROTO_TP4) ? "ISOPROTO_TP4" : "NSPROTO_SPX");
        else
          FIXME("unknown Protocol <0x%08x>\n", iProtocol);
      break;
  }
  *lpSize += dwLength;

  if ( !lpBuffer || !*lpSize || (*lpSize > dwOldSize))
     return 0;

  memset( lpBuffer, 0, dwOldSize);

  lpBuffer->lpProtocol = (LPSTR) &lpBuffer[ iAnz];
  lpBuffer->iProtocol  = iProtocol;

  switch (iProtocol) {
    case WS_IPPROTO_TCP :
        lpBuffer->dwServiceFlags = XP_FRAGMENTATION      | XP_EXPEDITED_DATA     |
                                   XP_GRACEFUL_CLOSE     | XP_GUARANTEED_ORDER   |
                                   XP_GUARANTEED_DELIVERY;
        lpBuffer->iAddressFamily = WS_AF_INET;
        lpBuffer->iMaxSockAddr   = 0x10;  /* NT4 SP5 */
        lpBuffer->iMinSockAddr   = 0x10;  /* NT4 SP5 */
        lpBuffer->iSocketType    = SOCK_STREAM;
        lpBuffer->dwMessageSize  = 0;
        lpProtName = NameTcp;
      break;
    case WS_IPPROTO_UDP :
        lpBuffer->dwServiceFlags = XP_FRAGMENTATION      | XP_SUPPORTS_BROADCAST |
                                   XP_SUPPORTS_MULTICAST | XP_MESSAGE_ORIENTED   |
                                   XP_CONNECTIONLESS;
        lpBuffer->iAddressFamily = WS_AF_INET;
        lpBuffer->iMaxSockAddr   = 0x10;  /* NT4 SP5 */
        lpBuffer->iMinSockAddr   = 0x10;  /* NT4 SP5 */
        lpBuffer->iSocketType    = SOCK_DGRAM;
        lpBuffer->dwMessageSize  = 65457; /* NT4 SP5 */
        lpProtName = NameUdp;
      break;
    case NSPROTO_IPX :
        lpBuffer->dwServiceFlags = XP_FRAGMENTATION      | XP_SUPPORTS_BROADCAST |
                                   XP_SUPPORTS_MULTICAST | XP_MESSAGE_ORIENTED   |
                                   XP_CONNECTIONLESS;
        lpBuffer->iAddressFamily = WS_AF_IPX;
        lpBuffer->iMaxSockAddr   = 0x10;  /* NT4 SP5 */
        lpBuffer->iMinSockAddr   = 0x0e;  /* NT4 SP5 */
        lpBuffer->iSocketType    = SOCK_DGRAM;
        lpBuffer->dwMessageSize  = 576;   /* NT4 SP5 */
        lpProtName = NameIpx;
      break;
    case NSPROTO_SPX :
        lpBuffer->dwServiceFlags = XP_FRAGMENTATION      |
                                   XP_PSEUDO_STREAM      | XP_MESSAGE_ORIENTED   |
                                   XP_GUARANTEED_ORDER   | XP_GUARANTEED_DELIVERY;
        lpBuffer->iAddressFamily = WS_AF_IPX;
        lpBuffer->iMaxSockAddr   = 0x10;  /* NT4 SP5 */
        lpBuffer->iMinSockAddr   = 0x0e;  /* NT4 SP5 */
        lpBuffer->iSocketType    = 5;
        lpBuffer->dwMessageSize  = -1;    /* NT4 SP5 */
        lpProtName = NameSpx;
      break;
    case NSPROTO_SPXII :
        lpBuffer->dwServiceFlags = XP_FRAGMENTATION      | XP_GRACEFUL_CLOSE     |
                                   XP_PSEUDO_STREAM      | XP_MESSAGE_ORIENTED   |
                                   XP_GUARANTEED_ORDER   | XP_GUARANTEED_DELIVERY;
        lpBuffer->iAddressFamily = WS_AF_IPX;
        lpBuffer->iMaxSockAddr   = 0x10;  /* NT4 SP5 */
        lpBuffer->iMinSockAddr   = 0x0e;  /* NT4 SP5 */
        lpBuffer->iSocketType    = 5;
        lpBuffer->dwMessageSize  = -1;    /* NT4 SP5 */
        lpProtName = NameSpxII;
      break;
  }
  if (unicode)
    strcpyW( (LPWSTR)lpBuffer->lpProtocol, lpProtName);
  else
    WideCharToMultiByte( CP_ACP, 0, lpProtName, -1, lpBuffer->lpProtocol,
                         dwOldSize - iAnz * sizeof( PROTOCOL_INFOA), NULL, NULL);

  return iAnz;
}

/* FIXME: EnumProtocols should be moved to winsock2, and this should be
 * implemented by calling out to WSAEnumProtocols. See:
 * http://support.microsoft.com/support/kb/articles/Q129/3/15.asp
 */
/*****************************************************************************
 *          WSOCK32_EnumProtocol [internal]
 *
 *    Enters the information about installed protocols into a given buffer
 *
 * RETURNS
 *    SOCKET_ERROR if the buffer is to small for the requested protocol infos
 *    on success the number of protocols inside the buffer
 *
 * NOTE
 *    NT4SP5 does not return SPX if lpiProtocols == NULL
 *
 * BUGS
 *    - NT4SP5 returns in addition these list of NETBIOS protocols
 *      (address family 17), each entry two times one for socket type 2 and 5
 *
 *      iProtocol   lpProtocol
 *      0x80000000  \Device\NwlnkNb
 *      0xfffffffa  \Device\NetBT_CBENT7
 *      0xfffffffb  \Device\Nbf_CBENT7
 *      0xfffffffc  \Device\NetBT_NdisWan5
 *      0xfffffffd  \Device\NetBT_El9202
 *      0xfffffffe  \Device\Nbf_El9202
 *      0xffffffff  \Device\Nbf_NdisWan4
 *
 *    - there is no check that the operating system supports the returned
 *      protocols
 */
static INT WSOCK32_EnumProtocol( LPINT lpiProtocols, PROTOCOL_INFOA* lpBuffer,
                                 LPDWORD lpdwLength, BOOL unicode)
{ DWORD dwCurSize, dwOldSize = *lpdwLength, dwTemp;
  INT   anz = 0, i;
  INT   iLocal[] = { WS_IPPROTO_TCP, WS_IPPROTO_UDP, NSPROTO_IPX, NSPROTO_SPXII, 0};

  if (!lpiProtocols) lpiProtocols = iLocal;

  *lpdwLength = 0;
  while ( *lpiProtocols )
  { dwCurSize = 0;
    WSOCK32_EnterSingleProtocol( *lpiProtocols, NULL, &dwCurSize, unicode);

    if ( lpBuffer && dwCurSize && ((*lpdwLength + dwCurSize) <= dwOldSize))
    { /* reserve the required space for the current protocol_info after the
       * last protocol_info before the start of the string buffer and adjust
       * the references into the string buffer
       */
      memmove( &((char*)&lpBuffer[ anz])[dwCurSize],
		  &lpBuffer[ anz],
               *lpdwLength - anz * sizeof( PROTOCOL_INFOA));
      for (i=0; i < anz; i++)
        lpBuffer[i].lpProtocol += dwCurSize;

      dwTemp = dwCurSize;
      anz += WSOCK32_EnterSingleProtocol( *lpiProtocols, &lpBuffer[anz],
                                          &dwTemp, unicode);
    }

    *lpdwLength += dwCurSize;
    lpiProtocols++;
  }

  if (dwOldSize < *lpdwLength) anz = SOCKET_ERROR;

  return anz;
}

/*****************************************************************************
 *          EnumProtocolsA       [WSOCK32.1111]
 *
 *    see function WSOCK32_EnumProtocol for RETURNS, BUGS
 */
INT WINAPI EnumProtocolsA( LPINT lpiProtocols, LPVOID lpBuffer,
                           LPDWORD lpdwLength)
{
   return WSOCK32_EnumProtocol( lpiProtocols, (PROTOCOL_INFOA*) lpBuffer,
                                lpdwLength, FALSE);
}

/*****************************************************************************
 *          EnumProtocolsW       [WSOCK32.1112]
 *
 *    see function WSOCK32_EnumProtocol for RETURNS, BUGS
 */
INT WINAPI EnumProtocolsW( LPINT lpiProtocols, LPVOID lpBuffer,
                           LPDWORD lpdwLength)
{
   return WSOCK32_EnumProtocol( lpiProtocols, (PROTOCOL_INFOA*) lpBuffer,
                                lpdwLength, TRUE);
}

/*****************************************************************************
 *          inet_network       [WSOCK32.1100]
 */
UINT WINAPI WSOCK32_inet_network(const char *cp)
{
#ifdef HAVE_INET_NETWORK
    return inet_network(cp);
#else
    return 0;
#endif
}

/*****************************************************************************
 *          getnetbyname       [WSOCK32.1101]
 */
struct netent * WINAPI WSOCK32_getnetbyname(const char *name)
{
#ifdef HAVE_GETNETBYNAME
    return getnetbyname(name);
#else
    return NULL;
#endif
}
