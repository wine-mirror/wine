/* DirectPlay Conformance Tests
 *
 * Copyright 2007 - Alessandro Pignotti
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

#include "wine/test.h"
#include <stdio.h>
#define INITGUID
#include <dplay.h>
#include <dplobby.h>


#define check(expected, result)                 \
    ok( (expected) == (result),                 \
        "expected=%d got=%d\n",                 \
        expected, result );
#define checkLP(expected, result)               \
    ok( (expected) == (result),                 \
        "expected=%p got=%p\n",                 \
        expected, result );
#define checkHR(expected, result)                       \
    ok( (expected) == (result),                         \
        "expected=%s got=%s\n",                         \
        dpResult2str(expected), dpResult2str(result) );
#define checkStr(expected, result)                              \
    ok( (result != NULL) && (!strcmp(expected, result)),        \
        "expected=%s got=%s\n",                                 \
        expected, result );
#define checkFlags(expected, result, flags)     \
    ok( (expected) == (result),                 \
        "expected=0x%08x(%s) got=0x%08x(%s)\n", \
        expected, dwFlags2str(expected, flags), \
        result, dwFlags2str(result, flags) );
#define checkGuid(expected, result)             \
    ok( IsEqualGUID(expected, result),          \
        "expected=%s got=%s\n",                 \
        Guid2str(expected), Guid2str(result) );


DEFINE_GUID(appGuid, 0xbdcfe03e, 0xf0ec, 0x415b, 0x82, 0x11, 0x6f, 0x86, 0xd8, 0x19, 0x7f, 0xe1);
DEFINE_GUID(appGuid2, 0x93417d3f, 0x7d26, 0x46ba, 0xb5, 0x76, 0xfe, 0x4b, 0x20, 0xbb, 0xad, 0x70);
DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);


typedef struct tagCallbackData
{
    LPDIRECTPLAY4 pDP;
    UINT dwCounter1, dwCounter2;
    DWORD dwFlags;
    char szTrace1[1024], szTrace2[1024];
    DPID *dpid;
    UINT dpidSize;
} CallbackData, *lpCallbackData;


static LPSTR get_temp_buffer(void)
{
    static UINT index = 0;
    static char buff[10][256];

    index = (index + 1) % 10;
    *buff[index] = 0;

    return buff[index];
}


static LPCSTR Guid2str(const GUID *guid)
{
    LPSTR buffer = get_temp_buffer();

    if (!guid) return "(null)";

    /* Service providers */
    if (IsEqualGUID(guid, &DPSPGUID_IPX))
        return "DPSPGUID_IPX";
    if (IsEqualGUID(guid, &DPSPGUID_TCPIP))
        return "DPSPGUID_TCPIP";
    if (IsEqualGUID(guid, &DPSPGUID_SERIAL))
        return "DPSPGUID_SERIAL";
    if (IsEqualGUID(guid, &DPSPGUID_MODEM))
        return "DPSPGUID_MODEM";
    /* DirectPlay Address ID's */
    if (IsEqualGUID(guid, &DPAID_TotalSize))
        return "DPAID_TotalSize";
    if (IsEqualGUID(guid, &DPAID_ServiceProvider))
        return "DPAID_ServiceProvider";
    if (IsEqualGUID(guid, &DPAID_LobbyProvider))
        return "DPAID_LobbyProvider";
    if (IsEqualGUID(guid, &DPAID_Phone))
        return "DPAID_Phone";
    if (IsEqualGUID(guid, &DPAID_PhoneW))
        return "DPAID_PhoneW";
    if (IsEqualGUID(guid, &DPAID_Modem))
        return "DPAID_Modem";
    if (IsEqualGUID(guid, &DPAID_ModemW))
        return "DPAID_ModemW";
    if (IsEqualGUID(guid, &DPAID_INet))
        return "DPAID_INet";
    if (IsEqualGUID(guid, &DPAID_INetW))
        return "DPAID_INetW";
    if (IsEqualGUID(guid, &DPAID_INetPort))
        return "DPAID_INetPort";
    if (IsEqualGUID(guid, &DPAID_ComPort))
        return "DPAID_ComPort";

    sprintf( buffer, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             guid->Data1, guid->Data2, guid->Data3,
             guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
             guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7] );
    return buffer;

}


static LPCSTR dpResult2str(HRESULT hr)
{
    switch (hr)
    {
    case DP_OK:                          return "DP_OK";
    case DPERR_ALREADYINITIALIZED:       return "DPERR_ALREADYINITIALIZED";
    case DPERR_ACCESSDENIED:             return "DPERR_ACCESSDENIED";
    case DPERR_ACTIVEPLAYERS:            return "DPERR_ACTIVEPLAYERS";
    case DPERR_BUFFERTOOSMALL:           return "DPERR_BUFFERTOOSMALL";
    case DPERR_CANTADDPLAYER:            return "DPERR_CANTADDPLAYER";
    case DPERR_CANTCREATEGROUP:          return "DPERR_CANTCREATEGROUP";
    case DPERR_CANTCREATEPLAYER:         return "DPERR_CANTCREATEPLAYER";
    case DPERR_CANTCREATESESSION:        return "DPERR_CANTCREATESESSION";
    case DPERR_CAPSNOTAVAILABLEYET:      return "DPERR_CAPSNOTAVAILABLEYET";
    case DPERR_EXCEPTION:                return "DPERR_EXCEPTION";
    case DPERR_GENERIC:                  return "DPERR_GENERIC";
    case DPERR_INVALIDFLAGS:             return "DPERR_INVALIDFLAGS";
    case DPERR_INVALIDOBJECT:            return "DPERR_INVALIDOBJECT";
    case DPERR_INVALIDPARAMS:            return "DPERR_INVALIDPARAMS";
        /*           symbol with the same value: DPERR_INVALIDPARAM */
    case DPERR_INVALIDPLAYER:            return "DPERR_INVALIDPLAYER";
    case DPERR_INVALIDGROUP:             return "DPERR_INVALIDGROUP";
    case DPERR_NOCAPS:                   return "DPERR_NOCAPS";
    case DPERR_NOCONNECTION:             return "DPERR_NOCONNECTION";
    case DPERR_NOMEMORY:                 return "DPERR_NOMEMORY";
        /*           symbol with the same value: DPERR_OUTOFMEMORY */
    case DPERR_NOMESSAGES:               return "DPERR_NOMESSAGES";
    case DPERR_NONAMESERVERFOUND:        return "DPERR_NONAMESERVERFOUND";
    case DPERR_NOPLAYERS:                return "DPERR_NOPLAYERS";
    case DPERR_NOSESSIONS:               return "DPERR_NOSESSIONS";
    case DPERR_PENDING:                  return "DPERR_PENDING";
    case DPERR_SENDTOOBIG:               return "DPERR_SENDTOOBIG";
    case DPERR_TIMEOUT:                  return "DPERR_TIMEOUT";
    case DPERR_UNAVAILABLE:              return "DPERR_UNAVAILABLE";
    case DPERR_UNSUPPORTED:              return "DPERR_UNSUPPORTED";
    case DPERR_BUSY:                     return "DPERR_BUSY";
    case DPERR_USERCANCEL:               return "DPERR_USERCANCEL";
    case DPERR_NOINTERFACE:              return "DPERR_NOINTERFACE";
    case DPERR_CANNOTCREATESERVER:       return "DPERR_CANNOTCREATESERVER";
    case DPERR_PLAYERLOST:               return "DPERR_PLAYERLOST";
    case DPERR_SESSIONLOST:              return "DPERR_SESSIONLOST";
    case DPERR_UNINITIALIZED:            return "DPERR_UNINITIALIZED";
    case DPERR_NONEWPLAYERS:             return "DPERR_NONEWPLAYERS";
    case DPERR_INVALIDPASSWORD:          return "DPERR_INVALIDPASSWORD";
    case DPERR_CONNECTING:               return "DPERR_CONNECTING";
    case DPERR_CONNECTIONLOST:           return "DPERR_CONNECTIONLOST";
    case DPERR_UNKNOWNMESSAGE:           return "DPERR_UNKNOWNMESSAGE";
    case DPERR_CANCELFAILED:             return "DPERR_CANCELFAILED";
    case DPERR_INVALIDPRIORITY:          return "DPERR_INVALIDPRIORITY";
    case DPERR_NOTHANDLED:               return "DPERR_NOTHANDLED";
    case DPERR_CANCELLED:                return "DPERR_CANCELLED";
    case DPERR_ABORTED:                  return "DPERR_ABORTED";
    case DPERR_BUFFERTOOLARGE:           return "DPERR_BUFFERTOOLARGE";
    case DPERR_CANTCREATEPROCESS:        return "DPERR_CANTCREATEPROCESS";
    case DPERR_APPNOTSTARTED:            return "DPERR_APPNOTSTARTED";
    case DPERR_INVALIDINTERFACE:         return "DPERR_INVALIDINTERFACE";
    case DPERR_NOSERVICEPROVIDER:        return "DPERR_NOSERVICEPROVIDER";
    case DPERR_UNKNOWNAPPLICATION:       return "DPERR_UNKNOWNAPPLICATION";
    case DPERR_NOTLOBBIED:               return "DPERR_NOTLOBBIED";
    case DPERR_SERVICEPROVIDERLOADED:    return "DPERR_SERVICEPROVIDERLOADED";
    case DPERR_ALREADYREGISTERED:        return "DPERR_ALREADYREGISTERED";
    case DPERR_NOTREGISTERED:            return "DPERR_NOTREGISTERED";
    case DPERR_AUTHENTICATIONFAILED:     return "DPERR_AUTHENTICATIONFAILED";
    case DPERR_CANTLOADSSPI:             return "DPERR_CANTLOADSSPI";
    case DPERR_ENCRYPTIONFAILED:         return "DPERR_ENCRYPTIONFAILED";
    case DPERR_SIGNFAILED:               return "DPERR_SIGNFAILED";
    case DPERR_CANTLOADSECURITYPACKAGE:  return "DPERR_CANTLOADSECURITYPACKAGE";
    case DPERR_ENCRYPTIONNOTSUPPORTED:   return "DPERR_ENCRYPTIONNOTSUPPORTED";
    case DPERR_CANTLOADCAPI:             return "DPERR_CANTLOADCAPI";
    case DPERR_NOTLOGGEDIN:              return "DPERR_NOTLOGGEDIN";
    case DPERR_LOGONDENIED:              return "DPERR_LOGONDENIED";
    case CLASS_E_NOAGGREGATION:          return "CLASS_E_NOAGGREGATION";

    default:
    {
        LPSTR buffer = get_temp_buffer();
        sprintf( buffer, "%d", HRESULT_CODE(hr) );
        return buffer;
    }
    }
}

static LPCSTR dwFlags2str(DWORD dwFlags, DWORD flagType)
{

#define FLAGS_DPCONNECTION     (1<<0)
#define FLAGS_DPENUMPLAYERS    (1<<1)
#define FLAGS_DPENUMGROUPS     (1<<2)
#define FLAGS_DPPLAYER         (1<<3)
#define FLAGS_DPGROUP          (1<<4)
#define FLAGS_DPENUMSESSIONS   (1<<5)
#define FLAGS_DPGETCAPS        (1<<6)
#define FLAGS_DPGET            (1<<7)
#define FLAGS_DPRECEIVE        (1<<8)
#define FLAGS_DPSEND           (1<<9)
#define FLAGS_DPSET            (1<<10)
#define FLAGS_DPMESSAGEQUEUE   (1<<11)
#define FLAGS_DPCONNECT        (1<<12)
#define FLAGS_DPOPEN           (1<<13)
#define FLAGS_DPSESSION        (1<<14)
#define FLAGS_DPLCONNECTION    (1<<15)
#define FLAGS_DPESC            (1<<16)
#define FLAGS_DPCAPS           (1<<17)

    LPSTR flags = get_temp_buffer();

    /* EnumConnections */

    if (flagType & FLAGS_DPCONNECTION)
    {
        if (dwFlags & DPCONNECTION_DIRECTPLAY)
            strcat(flags, "DPCONNECTION_DIRECTPLAY,");
        if (dwFlags & DPCONNECTION_DIRECTPLAYLOBBY)
            strcat(flags, "DPCONNECTION_DIRECTPLAYLOBBY,");
    }

    /* EnumPlayers,
       EnumGroups */

    if (flagType & FLAGS_DPENUMPLAYERS)
    {
        if (dwFlags == DPENUMPLAYERS_ALL)
            strcat(flags, "DPENUMPLAYERS_ALL,");
        if (dwFlags & DPENUMPLAYERS_LOCAL)
            strcat(flags, "DPENUMPLAYERS_LOCAL,");
        if (dwFlags & DPENUMPLAYERS_REMOTE)
            strcat(flags, "DPENUMPLAYERS_REMOTE,");
        if (dwFlags & DPENUMPLAYERS_GROUP)
            strcat(flags, "DPENUMPLAYERS_GROUP,");
        if (dwFlags & DPENUMPLAYERS_SESSION)
            strcat(flags, "DPENUMPLAYERS_SESSION,");
        if (dwFlags & DPENUMPLAYERS_SERVERPLAYER)
            strcat(flags, "DPENUMPLAYERS_SERVERPLAYER,");
        if (dwFlags & DPENUMPLAYERS_SPECTATOR)
            strcat(flags, "DPENUMPLAYERS_SPECTATOR,");
        if (dwFlags & DPENUMPLAYERS_OWNER)
            strcat(flags, "DPENUMPLAYERS_OWNER,");
    }
    if (flagType & FLAGS_DPENUMGROUPS)
    {
        if (dwFlags == DPENUMGROUPS_ALL)
            strcat(flags, "DPENUMGROUPS_ALL,");
        if (dwFlags & DPENUMPLAYERS_LOCAL)
            strcat(flags, "DPENUMGROUPS_LOCAL,");
        if (dwFlags & DPENUMPLAYERS_REMOTE)
            strcat(flags, "DPENUMGROUPS_REMOTE,");
        if (dwFlags & DPENUMPLAYERS_GROUP)
            strcat(flags, "DPENUMGROUPS_GROUP,");
        if (dwFlags & DPENUMPLAYERS_SESSION)
            strcat(flags, "DPENUMGROUPS_SESSION,");
        if (dwFlags & DPENUMGROUPS_SHORTCUT)
            strcat(flags, "DPENUMGROUPS_SHORTCUT,");
        if (dwFlags & DPENUMGROUPS_STAGINGAREA)
            strcat(flags, "DPENUMGROUPS_STAGINGAREA,");
        if (dwFlags & DPENUMGROUPS_HIDDEN)
            strcat(flags, "DPENUMGROUPS_HIDDEN,");
    }

    /* CreatePlayer */

    if (flagType & FLAGS_DPPLAYER)
    {
        if (dwFlags & DPPLAYER_SERVERPLAYER)
            strcat(flags, "DPPLAYER_SERVERPLAYER,");
        if (dwFlags & DPPLAYER_SPECTATOR)
            strcat(flags, "DPPLAYER_SPECTATOR,");
        if (dwFlags & DPPLAYER_LOCAL)
            strcat(flags, "DPPLAYER_LOCAL,");
        if (dwFlags & DPPLAYER_OWNER)
            strcat(flags, "DPPLAYER_OWNER,");
    }

    /* CreateGroup */

    if (flagType & FLAGS_DPGROUP)
    {
        if (dwFlags & DPGROUP_STAGINGAREA)
            strcat(flags, "DPGROUP_STAGINGAREA,");
        if (dwFlags & DPGROUP_LOCAL)
            strcat(flags, "DPGROUP_LOCAL,");
        if (dwFlags & DPGROUP_HIDDEN)
            strcat(flags, "DPGROUP_HIDDEN,");
    }

    /* EnumSessions */

    if (flagType & FLAGS_DPENUMSESSIONS)
    {
        if (dwFlags & DPENUMSESSIONS_AVAILABLE)
            strcat(flags, "DPENUMSESSIONS_AVAILABLE,");
        if (dwFlags &  DPENUMSESSIONS_ALL)
            strcat(flags, "DPENUMSESSIONS_ALL,");
        if (dwFlags & DPENUMSESSIONS_ASYNC)
            strcat(flags, "DPENUMSESSIONS_ASYNC,");
        if (dwFlags & DPENUMSESSIONS_STOPASYNC)
            strcat(flags, "DPENUMSESSIONS_STOPASYNC,");
        if (dwFlags & DPENUMSESSIONS_PASSWORDREQUIRED)
            strcat(flags, "DPENUMSESSIONS_PASSWORDREQUIRED,");
        if (dwFlags & DPENUMSESSIONS_RETURNSTATUS)
            strcat(flags, "DPENUMSESSIONS_RETURNSTATUS,");
    }

    /* GetCaps,
       GetPlayerCaps */

    if (flagType & FLAGS_DPGETCAPS)
    {
        if (dwFlags & DPGETCAPS_GUARANTEED)
            strcat(flags, "DPGETCAPS_GUARANTEED,");
    }

    /* GetGroupData,
       GetPlayerData */

    if (flagType & FLAGS_DPGET)
    {
        if (dwFlags == DPGET_REMOTE)
            strcat(flags, "DPGET_REMOTE,");
        if (dwFlags & DPGET_LOCAL)
            strcat(flags, "DPGET_LOCAL,");
    }

    /* Receive */

    if (flagType & FLAGS_DPRECEIVE)
    {
        if (dwFlags & DPRECEIVE_ALL)
            strcat(flags, "DPRECEIVE_ALL,");
        if (dwFlags & DPRECEIVE_TOPLAYER)
            strcat(flags, "DPRECEIVE_TOPLAYER,");
        if (dwFlags & DPRECEIVE_FROMPLAYER)
            strcat(flags, "DPRECEIVE_FROMPLAYER,");
        if (dwFlags & DPRECEIVE_PEEK)
            strcat(flags, "DPRECEIVE_PEEK,");
    }

    /* Send */

    if (flagType & FLAGS_DPSEND)
    {
        /*if (dwFlags == DPSEND_NONGUARANTEED)
          strcat(flags, "DPSEND_NONGUARANTEED,");*/
        if (dwFlags == DPSEND_MAX_PRIORITY) /* = DPSEND_MAX_PRI */
        {
            strcat(flags, "DPSEND_MAX_PRIORITY,");
        }
        else
        {
            if (dwFlags & DPSEND_GUARANTEED)
                strcat(flags, "DPSEND_GUARANTEED,");
            if (dwFlags & DPSEND_HIGHPRIORITY)
                strcat(flags, "DPSEND_HIGHPRIORITY,");
            if (dwFlags & DPSEND_OPENSTREAM)
                strcat(flags, "DPSEND_OPENSTREAM,");
            if (dwFlags & DPSEND_CLOSESTREAM)
                strcat(flags, "DPSEND_CLOSESTREAM,");
            if (dwFlags & DPSEND_SIGNED)
                strcat(flags, "DPSEND_SIGNED,");
            if (dwFlags & DPSEND_ENCRYPTED)
                strcat(flags, "DPSEND_ENCRYPTED,");
            if (dwFlags & DPSEND_LOBBYSYSTEMMESSAGE)
                strcat(flags, "DPSEND_LOBBYSYSTEMMESSAGE,");
            if (dwFlags & DPSEND_ASYNC)
                strcat(flags, "DPSEND_ASYNC,");
            if (dwFlags & DPSEND_NOSENDCOMPLETEMSG)
                strcat(flags, "DPSEND_NOSENDCOMPLETEMSG,");
        }
    }

    /* SetGroupData,
       SetGroupName,
       SetPlayerData,
       SetPlayerName,
       SetSessionDesc */

    if (flagType & FLAGS_DPSET)
    {
        if (dwFlags == DPSET_REMOTE)
            strcat(flags, "DPSET_REMOTE,");
        if (dwFlags & DPSET_LOCAL)
            strcat(flags, "DPSET_LOCAL,");
        if (dwFlags & DPSET_GUARANTEED)
            strcat(flags, "DPSET_GUARANTEED,");
    }

    /* GetMessageQueue */

    if (flagType & FLAGS_DPMESSAGEQUEUE)
    {
        if (dwFlags & DPMESSAGEQUEUE_SEND)
            strcat(flags, "DPMESSAGEQUEUE_SEND,");
        if (dwFlags & DPMESSAGEQUEUE_RECEIVE)
            strcat(flags, "DPMESSAGEQUEUE_RECEIVE,");
    }

    /* Connect */

    if (flagType & FLAGS_DPCONNECT)
    {
        if (dwFlags & DPCONNECT_RETURNSTATUS)
            strcat(flags, "DPCONNECT_RETURNSTATUS,");
    }

    /* Open */

    if (flagType & FLAGS_DPOPEN)
    {
        if (dwFlags & DPOPEN_JOIN)
            strcat(flags, "DPOPEN_JOIN,");
        if (dwFlags & DPOPEN_CREATE)
            strcat(flags, "DPOPEN_CREATE,");
        if (dwFlags & DPOPEN_RETURNSTATUS)
            strcat(flags, "DPOPEN_RETURNSTATUS,");
    }

    /* DPSESSIONDESC2 */

    if (flagType & FLAGS_DPSESSION)
    {
        if (dwFlags & DPSESSION_NEWPLAYERSDISABLED)
            strcat(flags, "DPSESSION_NEWPLAYERSDISABLED,");
        if (dwFlags & DPSESSION_MIGRATEHOST)
            strcat(flags, "DPSESSION_MIGRATEHOST,");
        if (dwFlags & DPSESSION_NOMESSAGEID)
            strcat(flags, "DPSESSION_NOMESSAGEID,");
        if (dwFlags & DPSESSION_JOINDISABLED)
            strcat(flags, "DPSESSION_JOINDISABLED,");
        if (dwFlags & DPSESSION_KEEPALIVE)
            strcat(flags, "DPSESSION_KEEPALIVE,");
        if (dwFlags & DPSESSION_NODATAMESSAGES)
            strcat(flags, "DPSESSION_NODATAMESSAGES,");
        if (dwFlags & DPSESSION_SECURESERVER)
            strcat(flags, "DPSESSION_SECURESERVER,");
        if (dwFlags & DPSESSION_PRIVATE)
            strcat(flags, "DPSESSION_PRIVATE,");
        if (dwFlags & DPSESSION_PASSWORDREQUIRED)
            strcat(flags, "DPSESSION_PASSWORDREQUIRED,");
        if (dwFlags & DPSESSION_MULTICASTSERVER)
            strcat(flags, "DPSESSION_MULTICASTSERVER,");
        if (dwFlags & DPSESSION_CLIENTSERVER)
            strcat(flags, "DPSESSION_CLIENTSERVER,");

        if (dwFlags & DPSESSION_DIRECTPLAYPROTOCOL)
            strcat(flags, "DPSESSION_DIRECTPLAYPROTOCOL,");
        if (dwFlags & DPSESSION_NOPRESERVEORDER)
            strcat(flags, "DPSESSION_NOPRESERVEORDER,");
        if (dwFlags & DPSESSION_OPTIMIZELATENCY)
            strcat(flags, "DPSESSION_OPTIMIZELATENCY,");

    }

    /* DPLCONNECTION */

    if (flagType & FLAGS_DPLCONNECTION)
    {
        if (dwFlags & DPLCONNECTION_CREATESESSION)
            strcat(flags, "DPLCONNECTION_CREATESESSION,");
        if (dwFlags & DPLCONNECTION_JOINSESSION)
            strcat(flags, "DPLCONNECTION_JOINSESSION,");
    }

    /* EnumSessionsCallback2 */

    if (flagType & FLAGS_DPESC)
    {
        if (dwFlags & DPESC_TIMEDOUT)
            strcat(flags, "DPESC_TIMEDOUT,");
    }

    /* GetCaps,
       GetPlayerCaps */

    if (flagType & FLAGS_DPCAPS)
    {
        if (dwFlags & DPCAPS_ISHOST)
            strcat(flags, "DPCAPS_ISHOST,");
        if (dwFlags & DPCAPS_GROUPOPTIMIZED)
            strcat(flags, "DPCAPS_GROUPOPTIMIZED,");
        if (dwFlags & DPCAPS_KEEPALIVEOPTIMIZED)
            strcat(flags, "DPCAPS_KEEPALIVEOPTIMIZED,");
        if (dwFlags & DPCAPS_GUARANTEEDOPTIMIZED)
            strcat(flags, "DPCAPS_GUARANTEEDOPTIMIZED,");
        if (dwFlags & DPCAPS_GUARANTEEDSUPPORTED)
            strcat(flags, "DPCAPS_GUARANTEEDSUPPORTED,");
        if (dwFlags & DPCAPS_SIGNINGSUPPORTED)
            strcat(flags, "DPCAPS_SIGNINGSUPPORTED,");
        if (dwFlags & DPCAPS_ENCRYPTIONSUPPORTED)
            strcat(flags, "DPCAPS_ENCRYPTIONSUPPORTED,");
        if (dwFlags & DPCAPS_ASYNCCANCELSUPPORTED)
            strcat(flags, "DPCAPS_ASYNCCANCELSUPPORTED,");
        if (dwFlags & DPCAPS_ASYNCCANCELALLSUPPORTED)
            strcat(flags, "DPCAPS_ASYNCCANCELALLSUPPORTED,");
        if (dwFlags & DPCAPS_SENDTIMEOUTSUPPORTED)
            strcat(flags, "DPCAPS_SENDTIMEOUTSUPPORTED,");
        if (dwFlags & DPCAPS_SENDPRIORITYSUPPORTED)
            strcat(flags, "DPCAPS_SENDPRIORITYSUPPORTED,");
        if (dwFlags & DPCAPS_ASYNCSUPPORTED)
            strcat(flags, "DPCAPS_ASYNCSUPPORTED,");

        if (dwFlags & DPPLAYERCAPS_LOCAL)
            strcat(flags, "DPPLAYERCAPS_LOCAL,");
    }

    if ((strlen(flags) == 0) && (dwFlags != 0))
        strcpy(flags, "UNKNOWN");
    else
        flags[strlen(flags)-1] = '\0';

    return flags;
}

static char dpid2char(DPID* dpid, DWORD dpidSize, DPID idPlayer)
{
    UINT i;
    if ( idPlayer == DPID_SYSMSG )
        return 'S';
    for (i=0; i<dpidSize; i++)
    {
        if ( idPlayer == dpid[i] )
            return (char)(i+48);
    }
    return '?';
}

static void check_messages( LPDIRECTPLAY4 pDP,
                            DPID *dpid,
                            DWORD dpidSize,
                            lpCallbackData callbackData )
{
    /* Retrieves all messages from the queue of pDP, performing tests
     * to check if we are receiving what we expect.
     *
     * Information about the messages is stores in callbackData:
     *
     * callbackData->dwCounter1: Number of messages received.
     * callbackData->szTrace1: Traces for sender and receiver.
     *     We store the position a dpid holds in the dpid array.
     *     Example:
     *
     *       trace string: "01,02,03,14"
     *           expanded: [ '01', '02', '03', '14' ]
     *                         \     \     \     \
     *                          \     \     \     ) message 3: from 1 to 4
     *                           \     \     ) message 2: from 0 to 3
     *                            \     ) message 1: from 0 to 2
     *                             ) message 0: from 0 to 1
     *
     *     In general terms:
     *       sender of message i   = character in place 3*i of the array
     *       receiver of message i = character in place 3*i+1 of the array
     *
     *     A sender value of 'S' means DPID_SYSMSG, this is, a system message.
     *
     * callbackData->szTrace2: Traces for message sizes.
     */

    DPID idFrom, idTo;
    UINT i;
    DWORD dwDataSize = 1024;
    LPVOID lpData = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwDataSize );
    HRESULT hr;
    char temp[5];

    callbackData->szTrace2[0] = '\0';

    i = 0;
    while ( DP_OK == (hr = IDirectPlayX_Receive( pDP, &idFrom, &idTo, 0,
                                                 lpData, &dwDataSize )) )
    {

        callbackData->szTrace1[ 3*i   ] = dpid2char( dpid, dpidSize, idFrom );
        callbackData->szTrace1[ 3*i+1 ] = dpid2char( dpid, dpidSize, idTo );
        callbackData->szTrace1[ 3*i+2 ] = ',';

        sprintf( temp, "%d,", dwDataSize );
        strcat( callbackData->szTrace2, temp );

        dwDataSize = 1024;
        ++i;
    }

    checkHR( DPERR_NOMESSAGES, hr );

    callbackData->szTrace1[ 3*i ] = '\0';
    callbackData->dwCounter1 = i;


    HeapFree( GetProcessHeap(), 0, lpData );
}

static void init_TCPIP_provider( LPDIRECTPLAY4 pDP,
                                 LPCSTR strIPAddressString,
                                 WORD port )
{

    DPCOMPOUNDADDRESSELEMENT addressElements[3];
    LPVOID pAddress = NULL;
    DWORD dwAddressSize = 0;
    LPDIRECTPLAYLOBBY3 pDPL;
    HRESULT hr;

    CoCreateInstance( &CLSID_DirectPlayLobby, NULL, CLSCTX_ALL,
                      &IID_IDirectPlayLobby3A, (LPVOID*) &pDPL );

    /* Service provider */
    addressElements[0].guidDataType = DPAID_ServiceProvider;
    addressElements[0].dwDataSize   = sizeof(GUID);
    addressElements[0].lpData       = (LPVOID) &DPSPGUID_TCPIP;

    /* IP address string */
    addressElements[1].guidDataType = DPAID_INet;
    addressElements[1].dwDataSize   = lstrlen(strIPAddressString) + 1;
    addressElements[1].lpData       = (LPVOID) strIPAddressString;

    /* Optional Port number */
    if( port > 0 )
    {
        addressElements[2].guidDataType = DPAID_INetPort;
        addressElements[2].dwDataSize   = sizeof(WORD);
        addressElements[2].lpData       = &port;
    }


    hr = IDirectPlayLobby_CreateCompoundAddress( pDPL, addressElements, 2,
                                                 NULL, &dwAddressSize );
    checkHR( DPERR_BUFFERTOOSMALL, hr );

    if( hr == DPERR_BUFFERTOOSMALL )
    {
        pAddress = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwAddressSize );
        hr = IDirectPlayLobby_CreateCompoundAddress( pDPL, addressElements, 2,
                                                     pAddress, &dwAddressSize );
        checkHR( DP_OK, hr );
    }

    hr = IDirectPlayX_InitializeConnection( pDP, pAddress, 0 );
    todo_wine checkHR( DP_OK, hr );

}

static BOOL FAR PASCAL EnumSessions_cb_join( LPCDPSESSIONDESC2 lpThisSD,
                                             LPDWORD lpdwTimeOut,
                                             DWORD dwFlags,
                                             LPVOID lpContext )
{
    LPDIRECTPLAY4 pDP = (LPDIRECTPLAY4) lpContext;
    DPSESSIONDESC2 dpsd;
    HRESULT hr;

    if (dwFlags & DPESC_TIMEDOUT)
    {
        return FALSE;
    }

    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    dpsd.guidInstance = lpThisSD->guidInstance;

    hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_JOIN );
    checkHR( DP_OK, hr );

    return TRUE;
}


/* DirectPlayCreate */

static void test_DirectPlayCreate(void)
{

    LPDIRECTPLAY pDP;
    HRESULT hr;

    /* TODO: Check how it behaves with pUnk!=NULL */

    /* pDP==NULL */
    hr = DirectPlayCreate( NULL, NULL, NULL );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = DirectPlayCreate( (LPGUID) &GUID_NULL, NULL, NULL );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = DirectPlayCreate( (LPGUID) &DPSPGUID_TCPIP, NULL, NULL );
    checkHR( DPERR_INVALIDPARAMS, hr );

    /* pUnk==NULL, pDP!=NULL */
    hr = DirectPlayCreate( NULL, &pDP, NULL );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = DirectPlayCreate( (LPGUID) &GUID_NULL, &pDP, NULL );
    checkHR( DP_OK, hr );
    if ( hr == DP_OK )
        IDirectPlayX_Release( pDP );
    hr = DirectPlayCreate( (LPGUID) &DPSPGUID_TCPIP, &pDP, NULL );
    todo_wine checkHR( DP_OK, hr );
    if ( hr == DP_OK )
        IDirectPlayX_Release( pDP );

}

/* EnumConnections */

static BOOL FAR PASCAL EnumAddress_cb2( REFGUID guidDataType,
                                        DWORD dwDataSize,
                                        LPCVOID lpData,
                                        LPVOID lpContext )
{
    lpCallbackData callbackData = (lpCallbackData) lpContext;

    static REFGUID types[] = { &DPAID_TotalSize,
                               &DPAID_ServiceProvider,
                               &GUID_NULL };
    static DWORD sizes[] = { 4, 16, 0  };
    static REFGUID sps[] = { &DPSPGUID_SERIAL, &DPSPGUID_MODEM,
                             &DPSPGUID_IPX, &DPSPGUID_TCPIP };


    checkGuid( types[ callbackData->dwCounter2 ], guidDataType );
    check( sizes[ callbackData->dwCounter2 ], dwDataSize );

    if ( IsEqualGUID( types[0], guidDataType ) )
    {
        todo_wine check( 80, *((LPDWORD) lpData) );
    }
    else if ( IsEqualGUID( types[1], guidDataType ) )
    {
        todo_wine checkGuid( sps[ callbackData->dwCounter1 ], (LPGUID) lpData );
    }

    callbackData->dwCounter2++;

    return TRUE;
}

static BOOL CALLBACK EnumConnections_cb( LPCGUID lpguidSP,
                                         LPVOID lpConnection,
                                         DWORD dwConnectionSize,
                                         LPCDPNAME lpName,
                                         DWORD dwFlags,
                                         LPVOID lpContext )
{

    lpCallbackData callbackData = (lpCallbackData) lpContext;
    LPDIRECTPLAYLOBBY pDPL;


    if (!callbackData->dwFlags)
    {
        callbackData->dwFlags = DPCONNECTION_DIRECTPLAY;
    }

    checkFlags( callbackData->dwFlags, dwFlags, FLAGS_DPCONNECTION );

    /* Get info from lpConnection */
    CoCreateInstance( &CLSID_DirectPlayLobby, NULL, CLSCTX_ALL,
                      &IID_IDirectPlayLobby3A, (LPVOID*) &pDPL );

    callbackData->dwCounter2 = 0;
    IDirectPlayLobby_EnumAddress( pDPL, EnumAddress_cb2,
                                  (LPCVOID) lpConnection,
                                  dwConnectionSize,
                                  (LPVOID) callbackData );
    todo_wine check( 3, callbackData->dwCounter2 );

    callbackData->dwCounter1++;

    return TRUE;
}

static void test_EnumConnections(void)
{

    LPDIRECTPLAY4 pDP;
    CallbackData callbackData;
    HRESULT hr;


    CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                      &IID_IDirectPlay4A, (LPVOID*) &pDP );


    callbackData.dwCounter1 = 0;
    callbackData.dwFlags = 0;
    hr = IDirectPlayX_EnumConnections( pDP, &appGuid, EnumConnections_cb,
                                       (LPVOID) &callbackData,
                                       callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 4, callbackData.dwCounter1 );

    callbackData.dwCounter1 = 0;
    callbackData.dwFlags = 0;
    hr = IDirectPlayX_EnumConnections( pDP, NULL, EnumConnections_cb,
                                       (LPVOID) &callbackData,
                                       callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 4, callbackData.dwCounter1 );

    callbackData.dwCounter1 = 0;
    callbackData.dwFlags = 0;
    hr = IDirectPlayX_EnumConnections( pDP, &appGuid, NULL,
                                       (LPVOID) &callbackData,
                                       callbackData.dwFlags );
    checkHR( DPERR_INVALIDPARAMS, hr );
    check( 0, callbackData.dwCounter1 );


    /* Flag tests */
    callbackData.dwCounter1 = 0;
    callbackData.dwFlags = DPCONNECTION_DIRECTPLAY;
    hr = IDirectPlayX_EnumConnections( pDP, &appGuid, EnumConnections_cb,
                                       (LPVOID) &callbackData,
                                       callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 4, callbackData.dwCounter1 );

    callbackData.dwCounter1 = 0;
    callbackData.dwFlags = DPCONNECTION_DIRECTPLAYLOBBY;
    hr = IDirectPlayX_EnumConnections( pDP, &appGuid, EnumConnections_cb,
                                       (LPVOID) &callbackData,
                                       callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 0, callbackData.dwCounter1 );

    callbackData.dwCounter1 = 0;
    callbackData.dwFlags = ( DPCONNECTION_DIRECTPLAY |
                             DPCONNECTION_DIRECTPLAYLOBBY );
    hr = IDirectPlayX_EnumConnections( pDP, &appGuid, EnumConnections_cb,
                                       (LPVOID) &callbackData,
                                       callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 4, callbackData.dwCounter1 );

    callbackData.dwCounter1 = 0;
    callbackData.dwFlags = ~( DPCONNECTION_DIRECTPLAY |
                              DPCONNECTION_DIRECTPLAYLOBBY );
    hr = IDirectPlayX_EnumConnections( pDP, &appGuid, EnumConnections_cb,
                                       (LPVOID) &callbackData,
                                       callbackData.dwFlags );
    checkHR( DPERR_INVALIDFLAGS, hr );
    check( 0, callbackData.dwCounter1 );


    IDirectPlayX_Release( pDP );
}

/* InitializeConnection */

static BOOL CALLBACK EnumConnections_cb2( LPCGUID lpguidSP,
                                          LPVOID lpConnection,
                                          DWORD dwConnectionSize,
                                          LPCDPNAME lpName,
                                          DWORD dwFlags,
                                          LPVOID lpContext )
{
    LPDIRECTPLAY4 pDP = (LPDIRECTPLAY4) lpContext;
    HRESULT hr;

    /* Incorrect parameters */
    hr = IDirectPlayX_InitializeConnection( pDP, NULL, 1 );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = IDirectPlayX_InitializeConnection( pDP, lpConnection, 1 );
    checkHR( DPERR_INVALIDFLAGS, hr );

    /* Normal operation.
       We're only interested in ensuring that the TCP/IP provider works */

    if( IsEqualGUID(lpguidSP, &DPSPGUID_TCPIP) )
    {
        hr = IDirectPlayX_InitializeConnection( pDP, lpConnection, 0 );
        todo_wine checkHR( DP_OK, hr );
        hr = IDirectPlayX_InitializeConnection( pDP, lpConnection, 0 );
        todo_wine checkHR( DPERR_ALREADYINITIALIZED, hr );
    }

    return TRUE;
}

static void test_InitializeConnection(void)
{

    LPDIRECTPLAY4 pDP;

    CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                      &IID_IDirectPlay4A, (LPVOID*) &pDP );

    IDirectPlayX_EnumConnections( pDP, &appGuid, EnumConnections_cb2,
                                  (LPVOID) pDP, 0 );

    IDirectPlayX_Release( pDP );
}

/* GetCaps */

static void test_GetCaps(void)
{

    LPDIRECTPLAY4 pDP;
    DPCAPS dpcaps;
    DWORD dwFlags;
    HRESULT hr;


    CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                      &IID_IDirectPlay4A, (LPVOID*) &pDP );
    ZeroMemory( &dpcaps, sizeof(DPCAPS) );

    /* Service provider not ininitialized */
    hr = IDirectPlayX_GetCaps( pDP, &dpcaps, 0 );
    checkHR( DPERR_UNINITIALIZED, hr );

    init_TCPIP_provider( pDP, "127.0.0.1", 0 );

    /* dpcaps not ininitialized */
    hr = IDirectPlayX_GetCaps( pDP, &dpcaps, 0 );
    todo_wine checkHR( DPERR_INVALIDPARAMS, hr );

    dpcaps.dwSize = sizeof(DPCAPS);

    for (dwFlags=0;
         dwFlags<=DPGETCAPS_GUARANTEED;
         dwFlags+=DPGETCAPS_GUARANTEED)
    {

        hr = IDirectPlayX_GetCaps( pDP, &dpcaps, dwFlags );
        todo_wine checkHR( DP_OK, hr );


        if ( hr == DP_OK )
        {
            check( sizeof(DPCAPS), dpcaps.dwSize );
            check( DPCAPS_ASYNCSUPPORTED |
                   DPCAPS_GUARANTEEDOPTIMIZED |
                   DPCAPS_GUARANTEEDSUPPORTED,
                   dpcaps.dwFlags );
            check( 0,     dpcaps.dwMaxQueueSize );
            check( 0,     dpcaps.dwHundredBaud );
            check( 500,   dpcaps.dwLatency );
            check( 65536, dpcaps.dwMaxLocalPlayers );
            check( 20,    dpcaps.dwHeaderLength );
            check( 5000,  dpcaps.dwTimeout );

            switch (dwFlags)
            {
            case 0:
                check( 65479,   dpcaps.dwMaxBufferSize );
                check( 65536,   dpcaps.dwMaxPlayers );
                break;
            case DPGETCAPS_GUARANTEED:
                check( 1048547, dpcaps.dwMaxBufferSize );
                check( 64,      dpcaps.dwMaxPlayers );
                break;
            default: break;
            }
        }
    }

    IDirectPlayX_Release( pDP );
}

/* Open */

static BOOL FAR PASCAL EnumSessions_cb2( LPCDPSESSIONDESC2 lpThisSD,
                                         LPDWORD lpdwTimeOut,
                                         DWORD dwFlags,
                                         LPVOID lpContext )
{
    LPDIRECTPLAY4 pDP = (LPDIRECTPLAY4) lpContext;
    DPSESSIONDESC2 dpsd;
    HRESULT hr;

    if (dwFlags & DPESC_TIMEDOUT)
        return FALSE;


    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    dpsd.guidInstance = lpThisSD->guidInstance;

    if ( lpThisSD->dwFlags & DPSESSION_PASSWORDREQUIRED )
    {
        /* Incorrect password */
        dpsd.lpszPasswordA = (LPSTR) "sonic boom";
        hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_JOIN );
        checkHR( DPERR_INVALIDPASSWORD, hr );

        /* Correct password */
        dpsd.lpszPasswordA = (LPSTR) "hadouken";
        hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_JOIN );
        checkHR( DP_OK, hr );
    }
    else
    {
        hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_JOIN );
        checkHR( DP_OK, hr );
    }

    hr = IDirectPlayX_Close( pDP );
    checkHR( DP_OK, hr );

    return TRUE;
}

static void test_Open(void)
{

    LPDIRECTPLAY4 pDP, pDP_server;
    DPSESSIONDESC2 dpsd, dpsd_server;
    HRESULT hr;


    CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                      &IID_IDirectPlay4A, (LPVOID*) &pDP_server );
    CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                      &IID_IDirectPlay4A, (LPVOID*) &pDP );
    ZeroMemory( &dpsd_server, sizeof(DPSESSIONDESC2) );
    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );

    /* Service provider not initialized */
    hr = IDirectPlayX_Open( pDP_server, &dpsd_server, DPOPEN_CREATE );
    todo_wine checkHR( DPERR_INVALIDPARAMS, hr );

    init_TCPIP_provider( pDP_server, "127.0.0.1", 0 );
    init_TCPIP_provider( pDP, "127.0.0.1", 0 );

    /* Uninitialized  dpsd */
    hr = IDirectPlayX_Open( pDP_server, &dpsd_server, DPOPEN_CREATE );
    todo_wine checkHR( DPERR_INVALIDPARAMS, hr );


    dpsd_server.dwSize = sizeof(DPSESSIONDESC2);
    dpsd_server.guidApplication = appGuid;
    dpsd_server.dwMaxPlayers = 10;


    /* Regular operation */
    hr = IDirectPlayX_Open( pDP_server, &dpsd_server, DPOPEN_CREATE );
    todo_wine checkHR( DP_OK, hr );

    /* Opening twice */
    hr = IDirectPlayX_Open( pDP_server, &dpsd_server, DPOPEN_CREATE );
    todo_wine checkHR( DPERR_ALREADYINITIALIZED, hr );

    /* Session flags */
    IDirectPlayX_Close( pDP_server );

    dpsd_server.dwFlags = DPSESSION_CLIENTSERVER | DPSESSION_MIGRATEHOST;
    hr = IDirectPlayX_Open( pDP_server, &dpsd_server, DPOPEN_CREATE );
    todo_wine checkHR( DPERR_INVALIDFLAGS, hr );

    dpsd_server.dwFlags = DPSESSION_MULTICASTSERVER | DPSESSION_MIGRATEHOST;
    hr = IDirectPlayX_Open( pDP_server, &dpsd_server, DPOPEN_CREATE );
    todo_wine checkHR( DPERR_INVALIDFLAGS, hr );

    dpsd_server.dwFlags = DPSESSION_SECURESERVER | DPSESSION_MIGRATEHOST;
    hr = IDirectPlayX_Open( pDP_server, &dpsd_server, DPOPEN_CREATE );
    todo_wine checkHR( DPERR_INVALIDFLAGS, hr );


    /* Joining sessions */
    /* - Checking how strict dplay is with sizes */
    dpsd.dwSize = 0;
    hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_JOIN );
    todo_wine checkHR( DPERR_INVALIDPARAMS, hr );

    dpsd.dwSize = sizeof(DPSESSIONDESC2)-1;
    hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_JOIN );
    todo_wine checkHR( DPERR_INVALIDPARAMS, hr );

    dpsd.dwSize = sizeof(DPSESSIONDESC2)+1;
    hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_JOIN );
    todo_wine checkHR( DPERR_INVALIDPARAMS, hr );

    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_JOIN );
    todo_wine checkHR( DPERR_NOSESSIONS, hr ); /* Only checks for size, not guids */


    dpsd.guidApplication = appGuid;
    dpsd.guidInstance = appGuid;


    hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_JOIN );
    todo_wine checkHR( DPERR_NOSESSIONS, hr );
    hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_JOIN | DPOPEN_CREATE );
    todo_wine checkHR( DPERR_NOSESSIONS, hr ); /* Second flag is ignored */

    dpsd_server.dwFlags = 0;


    /* Join to normal session */
    hr = IDirectPlayX_Open( pDP_server, &dpsd_server, DPOPEN_CREATE );
    todo_wine checkHR( DP_OK, hr );

    IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb2, pDP, 0 );


    /* Already initialized session */
    hr = IDirectPlayX_Open( pDP_server, &dpsd_server, DPOPEN_CREATE );
    todo_wine checkHR( DPERR_ALREADYINITIALIZED, hr );


    /* Checking which is the error checking order */
    dpsd_server.dwSize = 0;

    hr = IDirectPlayX_Open( pDP_server, &dpsd_server, DPOPEN_CREATE );
    todo_wine checkHR( DPERR_INVALIDPARAMS, hr );

    dpsd_server.dwSize = sizeof(DPSESSIONDESC2);


    /* Join to protected session */
    IDirectPlayX_Close( pDP_server );
    dpsd_server.lpszPasswordA = (LPSTR) "hadouken";
    hr = IDirectPlayX_Open( pDP_server, &dpsd_server, DPOPEN_CREATE );
    todo_wine checkHR( DP_OK, hr );

    IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb2,
                               pDP, DPENUMSESSIONS_PASSWORDREQUIRED );


    IDirectPlayX_Release( pDP );
    IDirectPlayX_Release( pDP_server );

}

/* EnumSessions */

static BOOL FAR PASCAL EnumSessions_cb( LPCDPSESSIONDESC2 lpThisSD,
                                        LPDWORD lpdwTimeOut,
                                        DWORD dwFlags,
                                        LPVOID lpContext )
{
    lpCallbackData callbackData = (lpCallbackData) lpContext;
    callbackData->dwCounter1++;

    if ( dwFlags & DPESC_TIMEDOUT )
    {
        check( TRUE, lpThisSD == NULL );
        return FALSE;
    }
    check( FALSE, lpThisSD == NULL );


    if ( lpThisSD->lpszPasswordA != NULL )
    {
        check( TRUE, (lpThisSD->dwFlags & DPSESSION_PASSWORDREQUIRED) != 0 );
    }

    if ( lpThisSD->dwFlags & DPSESSION_NEWPLAYERSDISABLED )
    {
        check( 0, lpThisSD->dwCurrentPlayers );
    }

    check( sizeof(*lpThisSD), lpThisSD->dwSize );
    checkLP( NULL, lpThisSD->lpszPasswordA );

    return TRUE;
}

static LPDIRECTPLAY4 create_session(DPSESSIONDESC2 *lpdpsd)
{

    LPDIRECTPLAY4 pDP;
    DPNAME name;
    DPID dpid;
    HRESULT hr;

    CoInitialize(NULL);

    CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                      &IID_IDirectPlay4A, (LPVOID*) &pDP );

    init_TCPIP_provider( pDP, "127.0.0.1", 0 );

    hr = IDirectPlayX_Open( pDP, lpdpsd, DPOPEN_CREATE );
    todo_wine checkHR( DP_OK, hr );

    if ( ! (lpdpsd->dwFlags & DPSESSION_NEWPLAYERSDISABLED) )
    {
        ZeroMemory( &name, sizeof(DPNAME) );
        name.dwSize = sizeof(DPNAME);
        name.lpszShortNameA = (LPSTR) "bofh";

        hr = IDirectPlayX_CreatePlayer( pDP, &dpid, &name, NULL, NULL,
                                        0, DPPLAYER_SERVERPLAYER );
        todo_wine checkHR( DP_OK, hr );
    }

    return pDP;

}

static void test_EnumSessions(void)
{

#define N_SESSIONS 6

    LPDIRECTPLAY4 pDP, pDPserver[N_SESSIONS];
    DPSESSIONDESC2 dpsd, dpsd_server[N_SESSIONS];
    CallbackData callbackData;
    HRESULT hr;
    UINT i;


    CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                      &IID_IDirectPlay4A, (LPVOID*) &pDP );
    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    callbackData.dwCounter1 = -1; /* So that after a call to EnumSessions
                                     we get the exact number of sessions */
    callbackData.dwFlags = 0;


    /* Service provider not initialized */
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData, 0 );
    checkHR( DPERR_UNINITIALIZED, hr );


    init_TCPIP_provider( pDP, "127.0.0.1", 0 );


    /* Session with no size */
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData, 0 );
    todo_wine checkHR( DPERR_INVALIDPARAMS, hr );

    if ( hr == DPERR_UNINITIALIZED )
    {
        skip( "EnumSessions not implemented\n" );
        return;
    }

    dpsd.dwSize = sizeof(DPSESSIONDESC2);


    /* No sessions */
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData, 0 );
    checkHR( DP_OK, hr );
    check( 0, callbackData.dwCounter1 );


    dpsd.guidApplication = appGuid;

    /* Set up sessions */
    for (i=0; i<N_SESSIONS; i++)
    {
        memcpy( &dpsd_server[i], &dpsd, sizeof(DPSESSIONDESC2) );
    }

    dpsd_server[0].lpszSessionNameA = (LPSTR) "normal";
    dpsd_server[0].dwFlags = ( DPSESSION_CLIENTSERVER |
                               DPSESSION_DIRECTPLAYPROTOCOL );
    dpsd_server[0].dwMaxPlayers = 10;

    dpsd_server[1].lpszSessionNameA = (LPSTR) "full";
    dpsd_server[1].dwFlags = ( DPSESSION_CLIENTSERVER |
                               DPSESSION_DIRECTPLAYPROTOCOL );
    dpsd_server[1].dwMaxPlayers = 1;

    dpsd_server[2].lpszSessionNameA = (LPSTR) "no new";
    dpsd_server[2].dwFlags = ( DPSESSION_CLIENTSERVER |
                               DPSESSION_DIRECTPLAYPROTOCOL |
                               DPSESSION_NEWPLAYERSDISABLED );
    dpsd_server[2].dwMaxPlayers = 10;

    dpsd_server[3].lpszSessionNameA = (LPSTR) "no join";
    dpsd_server[3].dwFlags = ( DPSESSION_CLIENTSERVER |
                               DPSESSION_DIRECTPLAYPROTOCOL |
                               DPSESSION_JOINDISABLED );
    dpsd_server[3].dwMaxPlayers = 10;

    dpsd_server[4].lpszSessionNameA = (LPSTR) "private";
    dpsd_server[4].dwFlags = ( DPSESSION_CLIENTSERVER |
                               DPSESSION_DIRECTPLAYPROTOCOL |
                               DPSESSION_PRIVATE );
    dpsd_server[4].dwMaxPlayers = 10;
    dpsd_server[4].lpszPasswordA = (LPSTR) "password";

    dpsd_server[5].lpszSessionNameA = (LPSTR) "protected";
    dpsd_server[5].dwFlags = ( DPSESSION_CLIENTSERVER |
                               DPSESSION_DIRECTPLAYPROTOCOL |
                               DPSESSION_PASSWORDREQUIRED );
    dpsd_server[5].dwMaxPlayers = 10;
    dpsd_server[5].lpszPasswordA = (LPSTR) "password";


    for (i=0; i<N_SESSIONS; i++)
    {
        pDPserver[i] = create_session( &dpsd_server[i] );
    }


    /* Invalid params */
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData, -1 );
    checkHR( DPERR_INVALIDPARAMS, hr );

    hr = IDirectPlayX_EnumSessions( pDP, NULL, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );

    check( -1, callbackData.dwCounter1 );


    /* Flag tests */
    callbackData.dwFlags = DPENUMSESSIONS_ALL; /* Doesn't list private,
                                                  protected */
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData,
                                    callbackData.dwFlags );
    check( N_SESSIONS-2, callbackData.dwCounter1 );

    /* Doesn't list private */
    callbackData.dwFlags = ( DPENUMSESSIONS_ALL |
                             DPENUMSESSIONS_PASSWORDREQUIRED );
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData,
                                    callbackData.dwFlags );
    check( N_SESSIONS-1, callbackData.dwCounter1 );

    /* Doesn't list full, no new, no join, private, protected */
    callbackData.dwFlags = DPENUMSESSIONS_AVAILABLE;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData,
                                    callbackData.dwFlags );
    check( N_SESSIONS-5, callbackData.dwCounter1 );

    /* Like with DPENUMSESSIONS_AVAILABLE */
    callbackData.dwFlags = 0;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData,
                                    callbackData.dwFlags );
    check( N_SESSIONS-5, callbackData.dwCounter1 );

    /* Doesn't list full, no new, no join, private */
    callbackData.dwFlags = DPENUMSESSIONS_PASSWORDREQUIRED;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData,
                                    callbackData.dwFlags );
    check( N_SESSIONS-4, callbackData.dwCounter1 );


    /* Async enumeration */
    callbackData.dwFlags = DPENUMSESSIONS_ASYNC;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData,
                                    callbackData.dwFlags );
    check( N_SESSIONS-4, callbackData.dwCounter1 ); /* Read cache of last
                                                       sync enumeration */

    callbackData.dwFlags = DPENUMSESSIONS_STOPASYNC;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData,
                                    callbackData.dwFlags );
    check( 0, callbackData.dwCounter1 ); /* Stop enumeration */

    callbackData.dwFlags = DPENUMSESSIONS_ASYNC;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData,
                                    callbackData.dwFlags );
    check( 0, callbackData.dwCounter1 ); /* Start enumeration */

    Sleep(500); /* Give time to fill the cache */

    callbackData.dwFlags = DPENUMSESSIONS_ASYNC;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData,
                                    callbackData.dwFlags );
    check( N_SESSIONS-5, callbackData.dwCounter1 ); /* Retrieve results */

    callbackData.dwFlags = DPENUMSESSIONS_STOPASYNC;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData,
                                    callbackData.dwFlags );
    check( 0, callbackData.dwCounter1 ); /* Stop enumeration */


    /* Specific tests for passworded sessions */

    for (i=0; i<N_SESSIONS; i++)
    {
        IDirectPlayX_Release( pDPserver[i] );
    }

    /* - Only session password set */
    for (i=4;i<=5;i++)
    {
        dpsd_server[i].lpszPasswordA = (LPSTR) "password";
        dpsd_server[i].dwFlags = 0;
        pDPserver[i] = create_session( &dpsd_server[i] );
    }

    callbackData.dwFlags = 0;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData,
                                    callbackData.dwFlags );
    check( 0, callbackData.dwCounter1 );

    callbackData.dwFlags = DPENUMSESSIONS_PASSWORDREQUIRED;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData,
                                    callbackData.dwFlags );
    check( 2, callbackData.dwCounter1 ); /* Both sessions automatically
                                            set DPSESSION_PASSWORDREQUIRED */

    /* - Only session flag set */
    for (i=4; i<=5; i++)
    {
        IDirectPlayX_Release( pDPserver[i] );
        dpsd_server[i].lpszPasswordA = NULL;
    }
    dpsd_server[4].dwFlags = DPSESSION_PRIVATE;
    dpsd_server[5].dwFlags = DPSESSION_PASSWORDREQUIRED;
    for (i=4; i<=5; i++)
    {
        pDPserver[i] = create_session( &dpsd_server[i] );
    }

    callbackData.dwFlags = 0;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData,
                                    callbackData.dwFlags );
    check( 2, callbackData.dwCounter1 ); /* Without password,
                                            the flag is ignored */

    /* - Both session flag and password set */
    for (i=4; i<=5; i++)
    {
        IDirectPlayX_Release( pDPserver[i] );
        dpsd_server[i].lpszPasswordA = (LPSTR) "password";
    }
    dpsd_server[4].dwFlags = DPSESSION_PRIVATE;
    dpsd_server[5].dwFlags = DPSESSION_PASSWORDREQUIRED;
    for (i=4; i<=5; i++)
    {
        pDPserver[i] = create_session( &dpsd_server[i] );
    }

    /* - Listing without password */
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData,
                                    callbackData.dwFlags );
    check( 0, callbackData.dwCounter1 );

    callbackData.dwFlags = DPENUMSESSIONS_PASSWORDREQUIRED;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData,
                                    callbackData.dwFlags );
    check( 1, callbackData.dwCounter1 );

    /* - Listing with incorrect password */
    dpsd.lpszPasswordA = (LPSTR) "bad_password";
    callbackData.dwFlags = 0;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData,
                                    callbackData.dwFlags );
    check( 0, callbackData.dwCounter1 );

    callbackData.dwFlags = DPENUMSESSIONS_PASSWORDREQUIRED;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData,
                                    callbackData.dwFlags );
    check( 1, callbackData.dwCounter1 );

    /* - Listing with  correct password */
    dpsd.lpszPasswordA = (LPSTR) "password";
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData,
                                    callbackData.dwFlags );
    check( 2, callbackData.dwCounter1 );


    dpsd.lpszPasswordA = NULL;
    callbackData.dwFlags = DPENUMSESSIONS_ASYNC;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData,
                                    callbackData.dwFlags );
    check( 2, callbackData.dwCounter1 ); /* Read cache of last sync enumeration,
                                            even private sessions */


    /* GUID tests */

    /* - Creating two servers with different application GUIDs */
    for (i=4; i<=5; i++)
    {
        IDirectPlayX_Release( pDPserver[i] );
        dpsd_server[i].dwFlags = ( DPSESSION_CLIENTSERVER |
                                   DPSESSION_DIRECTPLAYPROTOCOL );
        dpsd_server[i].lpszPasswordA = NULL;
        dpsd_server[i].dwMaxPlayers = 10;
    }
    dpsd_server[4].lpszSessionNameA = (LPSTR) "normal1";
    dpsd_server[4].guidApplication = appGuid;
    dpsd_server[5].lpszSessionNameA = (LPSTR) "normal2";
    dpsd_server[5].guidApplication = appGuid2;
    for (i=4; i<=5; i++)
    {
        pDPserver[i] = create_session( &dpsd_server[i] );
    }

    callbackData.dwFlags = 0;

    dpsd.guidApplication = appGuid2;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData,
                                    callbackData.dwFlags );
    check( 1, callbackData.dwCounter1 ); /* Only one of the sessions */

    dpsd.guidApplication = appGuid;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData,
                                    callbackData.dwFlags );
    check( 1, callbackData.dwCounter1 ); /* The other session */
    /* FIXME:
       For some reason, if we enum 1st with appGuid and 2nd with appGuid2,
       in the second enum we get the 2 sessions. Dplay fault? Elves? */

    dpsd.guidApplication = GUID_NULL;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    (LPVOID) &callbackData,
                                    callbackData.dwFlags );
    check( 2, callbackData.dwCounter1 ); /* Both sessions */

    for (i=4; i<=5; i++)
    {
        IDirectPlayX_Release( pDPserver[i] );
    }
    IDirectPlayX_Release( pDP );

}

/* SetSessionDesc
   GetSessionDesc */

static void test_SessionDesc(void)
{

    LPDIRECTPLAY4 pDP[2];
    DPSESSIONDESC2 dpsd;
    LPDPSESSIONDESC2 lpData[2];
    LPVOID lpDataMsg;
    DPID dpid[2];
    DWORD dwDataSize;
    HRESULT hr;
    UINT i;
    CallbackData callbackData;


    for (i=0; i<2; i++)
    {
        CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                          &IID_IDirectPlay4A, (LPVOID*) &pDP[i] );
    }
    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );

    /* Service provider not initialized */
    hr = IDirectPlayX_SetSessionDesc( pDP[0], NULL, 0 );
    checkHR( DPERR_UNINITIALIZED, hr );

    hr = IDirectPlayX_GetSessionDesc( pDP[0], NULL, NULL );
    checkHR( DPERR_UNINITIALIZED, hr );


    init_TCPIP_provider( pDP[0], "127.0.0.1", 0 );
    init_TCPIP_provider( pDP[1], "127.0.0.1", 0 );


    /* No sessions open */
    hr = IDirectPlayX_SetSessionDesc( pDP[0], NULL, 0 );
    todo_wine checkHR( DPERR_NOSESSIONS, hr );

    if ( hr == DPERR_UNINITIALIZED )
    {
        skip("Get/SetSessionDesc not implemented\n");
        return;
    }

    hr = IDirectPlayX_GetSessionDesc( pDP[0], NULL, NULL );
    checkHR( DPERR_NOSESSIONS, hr );


    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    dpsd.dwMaxPlayers = 10;


    /* Host */
    IDirectPlayX_Open( pDP[0], &dpsd, DPOPEN_CREATE );
    /* Peer */
    IDirectPlayX_EnumSessions( pDP[1], &dpsd, 0, EnumSessions_cb_join,
                               (LPVOID)pDP[1], 0 );

    for (i=0; i<2; i++)
    {
        /* Players, only to receive messages */
        IDirectPlayX_CreatePlayer( pDP[i], &dpid[i], NULL, NULL, NULL, 0, 0 );

        lpData[i] = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 1024 );
    }
    lpDataMsg = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 1024 );


    /* Incorrect parameters */
    hr = IDirectPlayX_SetSessionDesc( pDP[0], NULL, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = IDirectPlayX_GetSessionDesc( pDP[0], NULL, NULL );
    checkHR( DPERR_INVALIDPARAM, hr );
    hr = IDirectPlayX_GetSessionDesc( pDP[0], lpData[0], NULL );
    checkHR( DPERR_INVALIDPARAM, hr );
    dwDataSize=-1;
    hr = IDirectPlayX_GetSessionDesc( pDP[0], lpData[0], &dwDataSize );
    checkHR( DPERR_INVALIDPARAMS, hr );
    check( -1, dwDataSize );

    /* Get: Insufficient buffer size */
    dwDataSize=0;
    hr = IDirectPlayX_GetSessionDesc( pDP[0], lpData[0], &dwDataSize );
    checkHR( DPERR_BUFFERTOOSMALL, hr );
    check( dpsd.dwSize, dwDataSize );
    dwDataSize=4;
    hr = IDirectPlayX_GetSessionDesc( pDP[0], lpData[0], &dwDataSize );
    checkHR( DPERR_BUFFERTOOSMALL, hr );
    check( dpsd.dwSize, dwDataSize );
    dwDataSize=1024;
    hr = IDirectPlayX_GetSessionDesc( pDP[0], NULL, &dwDataSize );
    checkHR( DPERR_BUFFERTOOSMALL, hr );
    check( dpsd.dwSize, dwDataSize );

    /* Get: Regular operation
     *  i=0: Local session
     *  i=1: Remote session */
    for (i=0; i<2; i++)
    {
        hr = IDirectPlayX_GetSessionDesc( pDP[i], lpData[i], &dwDataSize );
        checkHR( DP_OK, hr );
        check( sizeof(DPSESSIONDESC2), dwDataSize );
        check( sizeof(DPSESSIONDESC2), lpData[i]->dwSize );
        checkGuid( &appGuid, &lpData[i]->guidApplication );
        check( dpsd.dwMaxPlayers, lpData[i]->dwMaxPlayers );
    }

    checkGuid( &lpData[0]->guidInstance, &lpData[1]->guidInstance );

    /* Set: Regular operation */
    dpsd.lpszSessionNameA = (LPSTR) "Wahaa";
    hr = IDirectPlayX_SetSessionDesc( pDP[0], &dpsd, 0 );
    checkHR( DP_OK, hr );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetSessionDesc( pDP[1], lpData[1], &dwDataSize );
    checkHR( DP_OK, hr );
    checkStr( dpsd.lpszSessionNameA, lpData[1]->lpszSessionNameA );


    /* Set: Failing to modify a remote session */
    hr = IDirectPlayX_SetSessionDesc( pDP[1], &dpsd, 0 );
    checkHR( DPERR_ACCESSDENIED, hr );

    /* Trying to change inmutable properties */
    /*  Flags */
    hr = IDirectPlayX_SetSessionDesc( pDP[0], &dpsd, 0 );
    checkHR( DP_OK, hr );
    dpsd.dwFlags = DPSESSION_SECURESERVER;
    hr = IDirectPlayX_SetSessionDesc( pDP[0], &dpsd, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );
    dpsd.dwFlags = 0;
    hr = IDirectPlayX_SetSessionDesc( pDP[0], &dpsd, 0 );
    checkHR( DP_OK, hr );
    /*  Size */
    dpsd.dwSize = 2048;
    hr = IDirectPlayX_SetSessionDesc( pDP[0], &dpsd, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );
    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    hr = IDirectPlayX_SetSessionDesc( pDP[0], &dpsd, 0 );
    checkHR( DP_OK, hr );

    /* Changing the GUIDs and size is ignored */
    dpsd.guidApplication = appGuid2;
    hr = IDirectPlayX_SetSessionDesc( pDP[0], &dpsd, 0 );
    checkHR( DP_OK, hr );
    dpsd.guidInstance = appGuid2;
    hr = IDirectPlayX_SetSessionDesc( pDP[0], &dpsd, 0 );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_GetSessionDesc( pDP[0], lpData[0], &dwDataSize );
    checkHR( DP_OK, hr );
    checkGuid( &appGuid, &lpData[0]->guidApplication );
    checkGuid( &lpData[1]->guidInstance, &lpData[0]->guidInstance );
    check( sizeof(DPSESSIONDESC2), lpData[0]->dwSize );


    /* Checking system messages */
    check_messages( pDP[0], dpid, 2, &callbackData );
    checkStr( "S0,S0,S0,S0,S0,S0,S0,", callbackData.szTrace1 );
    checkStr( "48,90,90,90,90,90,90,", callbackData.szTrace2 );
    check_messages( pDP[1], dpid, 2, &callbackData );
    checkStr( "S1,S1,S1,S1,S1,S1,", callbackData.szTrace1 );
    checkStr( "90,90,90,90,90,90,", callbackData.szTrace2 );

    for (i=0; i<2; i++)
    {
        HeapFree( GetProcessHeap(), 0, lpData[i] );
        IDirectPlayX_Release( pDP[i] );
    }

}

/* CreatePlayer */

static void test_CreatePlayer(void)
{

    LPDIRECTPLAY4 pDP[2];
    DPSESSIONDESC2 dpsd;
    DPNAME name;
    DPID dpid;
    HRESULT hr;


    CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                      &IID_IDirectPlay4A, (LPVOID*) &pDP[0] );
    CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                      &IID_IDirectPlay4A, (LPVOID*) &pDP[1] );
    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    ZeroMemory( &name, sizeof(DPNAME) );


    /* Connection not initialized */
    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL, 0, 0 );
    checkHR( DPERR_UNINITIALIZED, hr );


    init_TCPIP_provider( pDP[0], "127.0.0.1", 0 );
    init_TCPIP_provider( pDP[1], "127.0.0.1", 0 );


    /* Session not open */
    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL, 0, 0 );
    todo_wine checkHR( DPERR_INVALIDPARAMS, hr );

    if ( hr == DPERR_UNINITIALIZED )
    {
        skip( "CreatePlayer not implemented\n" );
        return;
    }

    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    IDirectPlayX_Open( pDP[0], &dpsd, DPOPEN_CREATE );


    /* Player name */
    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );


    name.dwSize = -1;


    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, &name, NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );


    name.dwSize = sizeof(DPNAME);
    name.lpszShortNameA = (LPSTR) "test";
    name.lpszLongNameA = NULL;


    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, &name, NULL, NULL,
                                    0, 0 );
    checkHR( DP_OK, hr );


    /* Null dpid */
    hr = IDirectPlayX_CreatePlayer( pDP[0], NULL, NULL, NULL, NULL,
                                    0, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );


    /* There can only be one server player */
    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL,
                                    0, DPPLAYER_SERVERPLAYER );
    checkHR( DP_OK, hr );
    check( DPID_SERVERPLAYER, dpid );

    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL,
                                    0, DPPLAYER_SERVERPLAYER );
    checkHR( DPERR_CANTCREATEPLAYER, hr );

    IDirectPlayX_DestroyPlayer( pDP[0], dpid );

    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL,
                                    0, DPPLAYER_SERVERPLAYER );
    checkHR( DP_OK, hr );
    check( DPID_SERVERPLAYER, dpid );
    IDirectPlayX_DestroyPlayer( pDP[0], dpid );


    /* Flags */
    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL,
                                    0, 0 );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL,
                                    0, DPPLAYER_SERVERPLAYER );
    checkHR( DP_OK, hr );
    check( DPID_SERVERPLAYER, dpid );
    IDirectPlayX_DestroyPlayer( pDP[0], dpid );

    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL,
                                    0, DPPLAYER_SPECTATOR );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL,
                                    0, ( DPPLAYER_SERVERPLAYER |
                                         DPPLAYER_SPECTATOR ) );
    checkHR( DP_OK, hr );
    check( DPID_SERVERPLAYER, dpid );
    IDirectPlayX_DestroyPlayer( pDP[0], dpid );


    /* Session with DPSESSION_NEWPLAYERSDISABLED */
    IDirectPlayX_Close( pDP[0] );
    dpsd.dwFlags = DPSESSION_NEWPLAYERSDISABLED;
    hr = IDirectPlayX_Open( pDP[0], &dpsd, DPOPEN_CREATE );
    checkHR( DP_OK, hr );


    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL,
                                    0, 0 );
    checkHR( DPERR_CANTCREATEPLAYER, hr );

    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL,
                                    0, DPPLAYER_SERVERPLAYER );
    checkHR( DPERR_CANTCREATEPLAYER, hr );

    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL,
                                    0, DPPLAYER_SPECTATOR );
    checkHR( DPERR_CANTCREATEPLAYER, hr );


    /* Creating players in a Client/Server session */
    IDirectPlayX_Close( pDP[0] );
    dpsd.dwFlags = DPSESSION_CLIENTSERVER;
    hr = IDirectPlayX_Open( pDP[0], &dpsd, DPOPEN_CREATE );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_EnumSessions( pDP[1], &dpsd, 0, EnumSessions_cb_join,
                                    (LPVOID) pDP[1], 0 );
    checkHR( DP_OK, hr );


    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL,
                                    0, 0 );
    checkHR( DPERR_ACCESSDENIED, hr );

    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL,
                                    0, DPPLAYER_SERVERPLAYER );
    checkHR( DP_OK, hr );
    check( DPID_SERVERPLAYER, dpid );

    hr = IDirectPlayX_CreatePlayer( pDP[1], &dpid, NULL, NULL, NULL,
                                    0, DPPLAYER_SERVERPLAYER );
    checkHR( DPERR_INVALIDFLAGS, hr );

    hr = IDirectPlayX_CreatePlayer( pDP[1], &dpid, NULL, NULL, NULL,
                                    0, 0 );
    checkHR( DP_OK, hr );


    IDirectPlayX_Release( pDP[0] );
    IDirectPlayX_Release( pDP[1] );

}

/* GetPlayerCaps */

static void test_GetPlayerCaps(void)
{

    LPDIRECTPLAY4 pDP[2];
    DPSESSIONDESC2 dpsd;
    DPID dpid[2];
    HRESULT hr;
    UINT i;

    DPCAPS playerCaps;
    DWORD dwFlags;


    for (i=0; i<2; i++)
    {
        CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                          &IID_IDirectPlay4A, (LPVOID*) &pDP[i] );
    }
    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    dpsd.dwMaxPlayers = 10;

    ZeroMemory( &playerCaps, sizeof(DPCAPS) );


    /* Uninitialized service provider */
    playerCaps.dwSize = 0;
    hr = IDirectPlayX_GetPlayerCaps( pDP[0], 0, &playerCaps, 0 );
    checkHR( DPERR_UNINITIALIZED, hr );

    playerCaps.dwSize = sizeof(DPCAPS);
    hr = IDirectPlayX_GetPlayerCaps( pDP[0], 0, &playerCaps, 0 );
    checkHR( DPERR_UNINITIALIZED, hr );


    init_TCPIP_provider( pDP[0], "127.0.0.1", 0 );
    init_TCPIP_provider( pDP[1], "127.0.0.1", 0 );


    /* No session */
    playerCaps.dwSize = 0;

    hr = IDirectPlayX_GetPlayerCaps( pDP[0], 0, &playerCaps, 0 );
    todo_wine checkHR( DPERR_INVALIDPARAMS, hr );

    if ( hr == DPERR_UNINITIALIZED )
    {
        skip( "GetPlayerCaps not implemented\n" );
        return;
    }

    playerCaps.dwSize = sizeof(DPCAPS);

    hr = IDirectPlayX_GetPlayerCaps( pDP[0], 0, &playerCaps, 0 );
    checkHR( DPERR_INVALIDPLAYER, hr );

    hr = IDirectPlayX_GetPlayerCaps( pDP[0], 2, &playerCaps, 0 );
    checkHR( DPERR_INVALIDPLAYER, hr );


    hr = IDirectPlayX_Open( pDP[0], &dpsd, DPOPEN_CREATE );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_EnumSessions( pDP[1], &dpsd, 0, EnumSessions_cb_join,
                                    (LPVOID) pDP[1], 0 );
    checkHR( DP_OK, hr );

    for (i=0; i<2; i++)
    {
        hr = IDirectPlayX_CreatePlayer( pDP[i], &dpid[i],
                                        NULL, NULL, NULL, 0, 0 );
        checkHR( DP_OK, hr );
    }


    /* Uninitialized playerCaps */
    playerCaps.dwSize = 0;

    hr = IDirectPlayX_GetPlayerCaps( pDP[0], 0, &playerCaps, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );

    hr = IDirectPlayX_GetPlayerCaps( pDP[0], 2, &playerCaps, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );

    hr = IDirectPlayX_GetPlayerCaps( pDP[0], dpid[0], &playerCaps, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );


    /* Invalid player */
    playerCaps.dwSize = sizeof(DPCAPS);

    hr = IDirectPlayX_GetPlayerCaps( pDP[0], 0, &playerCaps, 0 );
    checkHR( DPERR_INVALIDPLAYER, hr );

    hr = IDirectPlayX_GetPlayerCaps( pDP[0], 2, &playerCaps, 0 );
    checkHR( DPERR_INVALIDPLAYER, hr );

    hr = IDirectPlayX_GetPlayerCaps( pDP[0], dpid[0], &playerCaps, 0 );
    checkHR( DP_OK, hr );


    /* Regular parameters */
    for (i=0; i<2; i++)
    {
        for (dwFlags=0;
             dwFlags<=DPGETCAPS_GUARANTEED;
             dwFlags+=DPGETCAPS_GUARANTEED)
        {

            hr = IDirectPlayX_GetPlayerCaps( pDP[0], dpid[i],
                                             &playerCaps, dwFlags );
            checkHR( DP_OK, hr );


            check( sizeof(DPCAPS), playerCaps.dwSize );
            check( 40,    playerCaps.dwSize );
            check( 0,     playerCaps.dwMaxQueueSize );
            check( 0,     playerCaps.dwHundredBaud );
            check( 0,     playerCaps.dwLatency );
            check( 65536, playerCaps.dwMaxLocalPlayers );
            check( 20,    playerCaps.dwHeaderLength );

            if ( i == 0 )
            {
                checkFlags( DPCAPS_ISHOST |
                            DPCAPS_GUARANTEEDOPTIMIZED |
                            DPCAPS_GUARANTEEDSUPPORTED |
                            DPCAPS_ASYNCSUPPORTED |
                            DPPLAYERCAPS_LOCAL,
                            playerCaps.dwFlags, FLAGS_DPCAPS );
            }
            else
                checkFlags( DPCAPS_ISHOST |
                            DPCAPS_GUARANTEEDOPTIMIZED |
                            DPCAPS_GUARANTEEDSUPPORTED |
                            DPCAPS_ASYNCSUPPORTED,
                            playerCaps.dwFlags, FLAGS_DPCAPS );

            if ( dwFlags == DPGETCAPS_GUARANTEED )
            {
                check( 1048547, playerCaps.dwMaxBufferSize );
                check( 64,      playerCaps.dwMaxPlayers );
            }
            else
            {
                check( 65479, playerCaps.dwMaxBufferSize );
                check( 65536, playerCaps.dwMaxPlayers );
            }

        }
    }


    IDirectPlayX_Release( pDP[0] );
    IDirectPlayX_Release( pDP[1] );

}

/* SetPlayerData
   GetPlayerData */

static void test_PlayerData(void)
{
    LPDIRECTPLAY4 pDP;
    DPSESSIONDESC2 dpsd;
    DPID dpid;
    HRESULT hr;

    /* lpDataFake has to be bigger than the rest, limits lpDataGet size */
    LPCSTR lpDataFake     = "big_fake_data_chunk";
    DWORD dwDataSizeFake  = strlen(lpDataFake)+1;

    LPCSTR lpData         = "remote_data";
    DWORD dwDataSize      = strlen(lpData)+1;

    LPCSTR lpDataLocal    = "local_data";
    DWORD dwDataSizeLocal = strlen(lpDataLocal)+1;

    LPSTR lpDataGet       = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                       dwDataSizeFake );
    DWORD dwDataSizeGet   = dwDataSizeFake;


    CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                      &IID_IDirectPlay4A, (LPVOID*) &pDP );

    /* No service provider */
    hr = IDirectPlayX_SetPlayerData( pDP, 0, (LPVOID) lpData,
                                     dwDataSize, 0 );
    checkHR( DPERR_UNINITIALIZED, hr );

    hr = IDirectPlayX_GetPlayerData( pDP, 0, (LPVOID) lpDataGet,
                                     &dwDataSizeGet, 0 );
    checkHR( DPERR_UNINITIALIZED, hr );


    init_TCPIP_provider( pDP, "127.0.0.1", 0 );

    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    IDirectPlayX_Open( pDP, &dpsd, DPOPEN_CREATE );


    /* Invalid player */
    hr = IDirectPlayX_SetPlayerData( pDP, 0, (LPVOID) lpData,
                                     dwDataSize, 0 );
    todo_wine checkHR( DPERR_INVALIDPLAYER, hr );

    hr = IDirectPlayX_GetPlayerData( pDP, 0, (LPVOID) lpDataGet,
                                     &dwDataSizeGet, 0 );
    todo_wine checkHR( DPERR_INVALIDPLAYER, hr );

    if ( hr == DPERR_UNINITIALIZED )
    {
        skip( "Get/SetPlayerData not implemented\n" );
        return;
    }

    /* Create the player */
    /* By default, the data is remote */
    hr = IDirectPlayX_CreatePlayer( pDP, &dpid, NULL, NULL, (LPVOID) lpData,
                                    dwDataSize, 0 );
    checkHR( DP_OK, hr );

    /* Invalid parameters */
    hr = IDirectPlayX_SetPlayerData( pDP, dpid, NULL,
                                     dwDataSize, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = IDirectPlayX_SetPlayerData( pDP, dpid, (LPVOID) lpDataGet,
                                     -1, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );

    hr = IDirectPlayX_GetPlayerData( pDP, dpid, (LPVOID) lpDataGet,
                                     NULL, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );


    /*
     * Remote data (default)
     */


    /* Buffer redimension */
    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, NULL,
                                     &dwDataSizeGet, 0 );
    check( DPERR_BUFFERTOOSMALL, hr );
    check( dwDataSize, dwDataSizeGet );
    checkStr( lpDataFake, lpDataGet );

    dwDataSizeGet = 2;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, (LPVOID) lpDataGet,
                                     &dwDataSizeGet, 0 );
    check( DPERR_BUFFERTOOSMALL, hr );
    check( dwDataSize, dwDataSizeGet );

    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, (LPVOID) lpDataGet,
                                     &dwDataSizeGet, 0 );
    checkHR( DP_OK, hr );
    check( dwDataSize, dwDataSizeGet );
    checkStr( lpData, lpDataGet );

    /* Normal operation */
    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, (LPVOID) lpDataGet,
                                     &dwDataSizeGet, 0 );
    checkHR( DP_OK, hr );
    check( dwDataSize, dwDataSizeGet );
    checkStr( lpData, lpDataGet );

    /* Flag tests */
    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, (LPVOID) lpDataGet,
                                     &dwDataSizeGet, 0 );
    checkHR( DP_OK, hr );
    check( dwDataSize, dwDataSizeGet ); /* Remote: works as expected */
    checkStr( lpData, lpDataGet );

    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, (LPVOID) lpDataGet,
                                     &dwDataSizeGet, DPGET_REMOTE );
    checkHR( DP_OK, hr );
    check( dwDataSize, dwDataSizeGet ); /* Same behaviour as in previous test */
    checkStr( lpData, lpDataGet );

    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, (LPVOID) lpDataGet,
                                     &dwDataSizeGet, DPGET_LOCAL );
    checkHR( DP_OK, hr );
    check( 0, dwDataSizeGet ); /* Sets size to 0 (as local data doesn't exist) */
    checkStr( lpDataFake, lpDataGet );

    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, (LPVOID) lpDataGet,
                                     &dwDataSizeGet,
                                     DPGET_LOCAL | DPGET_REMOTE );
    checkHR( DP_OK, hr );
    check( 0, dwDataSizeGet ); /* Same behaviour as in previous test */
    checkStr( lpDataFake, lpDataGet );

    /* Getting local data (which doesn't exist), buffer size is ignored */
    dwDataSizeGet = 0;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, (LPVOID) lpDataGet,
                                     &dwDataSizeGet, DPGET_LOCAL );
    checkHR( DP_OK, hr );
    check( 0, dwDataSizeGet ); /* Sets size to 0 */
    checkStr( lpDataFake, lpDataGet );

    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, NULL,
                                     &dwDataSizeGet, DPGET_LOCAL );
    checkHR( DP_OK, hr );
    check( 0, dwDataSizeGet ); /* Sets size to 0 */
    checkStr( lpDataFake, lpDataGet );


    /*
     * Local data
     */


    /* Invalid flags */
    hr = IDirectPlayX_SetPlayerData( pDP, dpid, (LPVOID) lpDataLocal,
                                     dwDataSizeLocal,
                                     DPSET_LOCAL | DPSET_GUARANTEED );
    checkHR( DPERR_INVALIDPARAMS, hr );

    /* Correct parameters */
    hr = IDirectPlayX_SetPlayerData( pDP, dpid, (LPVOID) lpDataLocal,
                                     dwDataSizeLocal, DPSET_LOCAL );
    checkHR( DP_OK, hr );

    /* Flag tests (again) */
    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, (LPVOID) lpDataGet,
                                     &dwDataSizeGet, 0 );
    checkHR( DP_OK, hr );
    check( dwDataSize, dwDataSizeGet ); /* Remote: works as expected */
    checkStr( lpData, lpDataGet );

    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, (LPVOID) lpDataGet,
                                     &dwDataSizeGet, DPGET_REMOTE );
    checkHR( DP_OK, hr );
    check( dwDataSize, dwDataSizeGet ); /* Like in previous test */
    checkStr( lpData, lpDataGet );

    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, (LPVOID) lpDataGet,
                                     &dwDataSizeGet, DPGET_LOCAL );
    checkHR( DP_OK, hr );
    check( dwDataSizeLocal, dwDataSizeGet ); /* Local: works as expected */
    checkStr( lpDataLocal, lpDataGet );

    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, (LPVOID) lpDataGet,
                                     &dwDataSizeGet,
                                     DPGET_LOCAL | DPGET_REMOTE );
    checkHR( DP_OK, hr );
    check( dwDataSizeLocal, dwDataSizeGet ); /* Like in previous test */
    checkStr( lpDataLocal, lpDataGet );

    /* Small buffer works as expected again */
    dwDataSizeGet = 0;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, (LPVOID) lpDataGet,
                                     &dwDataSizeGet, DPGET_LOCAL );
    checkHR( DPERR_BUFFERTOOSMALL, hr );
    check( dwDataSizeLocal, dwDataSizeGet );
    checkStr( lpDataFake, lpDataGet );

    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, NULL,
                                     &dwDataSizeGet, DPGET_LOCAL );
    check( DPERR_BUFFERTOOSMALL, hr );
    check( dwDataSizeLocal, dwDataSizeGet );
    checkStr( lpDataFake, lpDataGet );


    /*
     * Changing remote data
     */


    /* Remote data := local data */
    hr = IDirectPlayX_SetPlayerData( pDP, dpid, (LPVOID) lpDataLocal,
                                     dwDataSizeLocal,
                                     DPSET_GUARANTEED | DPSET_REMOTE );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_SetPlayerData( pDP, dpid, (LPVOID) lpDataLocal,
                                     dwDataSizeLocal, 0 );
    checkHR( DP_OK, hr );

    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, (LPVOID) lpDataGet,
                                     &dwDataSizeGet, 0 );
    checkHR( DP_OK, hr );
    check( dwDataSizeLocal, dwDataSizeGet );
    checkStr( lpDataLocal, lpDataGet );

    /* Remote data := fake data */
    hr = IDirectPlayX_SetPlayerData( pDP, dpid, (LPVOID) lpDataFake,
                                     dwDataSizeFake, DPSET_REMOTE );
    checkHR( DP_OK, hr );

    dwDataSizeGet = dwDataSizeFake + 1;
    strcpy(lpDataGet, lpData);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, (LPVOID) lpDataGet,
                                     &dwDataSizeGet, 0 );
    checkHR( DP_OK, hr );
    check( dwDataSizeFake, dwDataSizeGet );
    checkStr( lpDataFake, lpDataGet );


    HeapFree( GetProcessHeap(), 0, lpDataGet );
    IDirectPlayX_Release( pDP );
}

/* GetPlayerName
   SetPlayerName */

static void test_PlayerName(void)
{

    LPDIRECTPLAY4 pDP[2];
    DPSESSIONDESC2 dpsd;
    DPID dpid[2];
    HRESULT hr;
    UINT i;

    DPNAME playerName;
    DWORD dwDataSize = 1024;
    LPVOID lpData = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwDataSize );
    CallbackData callbackData;


    for (i=0; i<2; i++)
    {
        CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                          &IID_IDirectPlay4A, (LPVOID*) &pDP[i] );
    }
    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    ZeroMemory( &playerName, sizeof(DPNAME) );


    /* Service provider not initialized */
    hr = IDirectPlayX_SetPlayerName( pDP[0], 0, &playerName, 0 );
    checkHR( DPERR_UNINITIALIZED, hr );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[0], 0, lpData, &dwDataSize );
    checkHR( DPERR_UNINITIALIZED, hr );
    check( 1024, dwDataSize );


    init_TCPIP_provider( pDP[0], "127.0.0.1", 0 );
    init_TCPIP_provider( pDP[1], "127.0.0.1", 0 );


    /* Session not initialized */
    hr = IDirectPlayX_SetPlayerName( pDP[0], 0, &playerName, 0 );
    todo_wine checkHR( DPERR_INVALIDPLAYER, hr );

    if ( hr == DPERR_UNINITIALIZED )
    {
        skip( "Get/SetPlayerName not implemented\n" );
        return;
    }

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[0], 0, lpData, &dwDataSize );
    checkHR( DPERR_INVALIDPLAYER, hr );
    check( 1024, dwDataSize );


    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    dpsd.dwMaxPlayers = 10;
    IDirectPlayX_Open( pDP[0], &dpsd, DPOPEN_CREATE );
    IDirectPlayX_EnumSessions( pDP[1], &dpsd, 0, EnumSessions_cb_join,
                               (LPVOID) pDP[1], 0 );

    IDirectPlayX_CreatePlayer( pDP[0], &dpid[0], NULL, NULL, NULL, 0, 0 );
    IDirectPlayX_CreatePlayer( pDP[1], &dpid[1], NULL, NULL, NULL, 0, 0 );


    /* Name not initialized */
    playerName.dwSize = -1;
    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[0], &playerName, 0 );
    checkHR( DP_OK, hr );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[0], 0, lpData, &dwDataSize );
    checkHR( DPERR_INVALIDPLAYER, hr );
    check( 1024, dwDataSize );


    playerName.dwSize = sizeof(DPNAME);
    playerName.lpszShortNameA = (LPSTR) "player_name";
    playerName.lpszLongNameA = (LPSTR) "player_long_name";


    /* Invalid parameters */
    hr = IDirectPlayX_SetPlayerName( pDP[0], -1, &playerName, 0 );
    checkHR( DPERR_INVALIDPLAYER, hr );
    hr = IDirectPlayX_SetPlayerName( pDP[0], 0, &playerName, 0 );
    checkHR( DPERR_INVALIDPLAYER, hr );
    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[0], &playerName, -1 );
    checkHR( DPERR_INVALIDPARAMS, hr );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[0], 0, lpData, &dwDataSize );
    checkHR( DPERR_INVALIDPLAYER, hr );
    check( 1024, dwDataSize );

    dwDataSize = -1;
    hr = IDirectPlayX_GetPlayerName( pDP[0], dpid[0], lpData, &dwDataSize );
    checkHR( DPERR_INVALIDPARAMS, hr );
    check( -1, dwDataSize );

    hr = IDirectPlayX_GetPlayerName( pDP[0], dpid[0], lpData, NULL );
    checkHR( DPERR_INVALIDPARAMS, hr );

    /* Trying to modify remote player */
    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[1], &playerName, 0 );
    checkHR( DPERR_ACCESSDENIED, hr );


    /* Regular operation */
    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[0], &playerName, 0 );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[0], dpid[0], lpData, &dwDataSize );
    checkHR( DP_OK, hr );
    check( 45, dwDataSize );
    checkStr( playerName.lpszShortNameA, ((LPDPNAME)lpData)->lpszShortNameA );
    checkStr( playerName.lpszLongNameA,  ((LPDPNAME)lpData)->lpszLongNameA );
    check( 0,                            ((LPDPNAME)lpData)->dwFlags );

    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[0], NULL, 0 );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[0], dpid[0], lpData, &dwDataSize );
    checkHR( DP_OK, hr );
    check( 16, dwDataSize );
    checkLP( NULL, ((LPDPNAME)lpData)->lpszShortNameA );
    checkLP( NULL, ((LPDPNAME)lpData)->lpszLongNameA );
    check( 0,      ((LPDPNAME)lpData)->dwFlags );


    /* Small buffer in get operation */
    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[0], dpid[0], NULL, &dwDataSize );
    checkHR( DPERR_BUFFERTOOSMALL, hr );
    check( 16, dwDataSize );

    dwDataSize = 0;
    hr = IDirectPlayX_GetPlayerName( pDP[0], dpid[0], lpData, &dwDataSize );
    checkHR( DPERR_BUFFERTOOSMALL, hr );
    check( 16, dwDataSize );

    hr = IDirectPlayX_GetPlayerName( pDP[0], dpid[0], lpData, &dwDataSize );
    checkHR( DP_OK, hr );
    check( 16, dwDataSize );
    checkLP( NULL, ((LPDPNAME)lpData)->lpszShortNameA );
    checkLP( NULL, ((LPDPNAME)lpData)->lpszLongNameA );
    check( 0, ((LPDPNAME)lpData)->dwFlags );


    /* Flags */
    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[0], &playerName,
                                     DPSET_GUARANTEED );
    checkHR( DP_OK, hr );

    /* - Local (no propagation) */
    playerName.lpszShortNameA = (LPSTR) "no_propagation";
    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[0], &playerName,
                                     DPSET_LOCAL );
    checkHR( DP_OK, hr );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[0], dpid[0],
                                     lpData, &dwDataSize ); /* Local fetch */
    checkHR( DP_OK, hr );
    check( 48, dwDataSize );
    checkStr( "no_propagation", ((LPDPNAME)lpData)->lpszShortNameA );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[1], dpid[0],
                                     lpData, &dwDataSize ); /* Remote fetch */
    checkHR( DP_OK, hr );
    check( 45, dwDataSize );
    checkStr( "player_name", ((LPDPNAME)lpData)->lpszShortNameA );

    /* -- 2 */

    playerName.lpszShortNameA = (LPSTR) "no_propagation_2";
    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[0], &playerName,
                                     DPSET_LOCAL | DPSET_REMOTE );
    checkHR( DP_OK, hr );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[0], dpid[0],
                                     lpData, &dwDataSize ); /* Local fetch */
    checkHR( DP_OK, hr );
    check( 50, dwDataSize );
    checkStr( "no_propagation_2", ((LPDPNAME)lpData)->lpszShortNameA );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[1], dpid[0],
                                     lpData, &dwDataSize ); /* Remote fetch */
    checkHR( DP_OK, hr );
    check( 45, dwDataSize );
    checkStr( "player_name", ((LPDPNAME)lpData)->lpszShortNameA );

    /* - Remote (propagation, default) */
    playerName.lpszShortNameA = (LPSTR) "propagation";
    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[0], &playerName,
                                     DPSET_REMOTE );
    checkHR( DP_OK, hr );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[1], dpid[0],
                                     lpData, &dwDataSize ); /* Remote fetch */
    checkHR( DP_OK, hr );
    check( 45, dwDataSize );
    checkStr( "propagation", ((LPDPNAME)lpData)->lpszShortNameA );

    /* -- 2 */
    playerName.lpszShortNameA = (LPSTR) "propagation_2";
    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[0], &playerName,
                                     0 );
    checkHR( DP_OK, hr );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[1], dpid[0],
                                     lpData, &dwDataSize ); /* Remote fetch */
    checkHR( DP_OK, hr );
    check( 47, dwDataSize );
    checkStr( "propagation_2", ((LPDPNAME)lpData)->lpszShortNameA );


    /* Checking system messages */
    check_messages( pDP[0], dpid, 2, &callbackData );
    checkStr( "S0,S0,S0,S0,S0,S0,S0,", callbackData.szTrace1 );
    checkStr( "48,28,57,28,57,57,59,", callbackData.szTrace2 );
    check_messages( pDP[1], dpid, 2, &callbackData );
    checkStr( "S1,S1,S1,S1,S1,S1,", callbackData.szTrace1 );
    checkStr( "28,57,28,57,57,59,", callbackData.szTrace2 );


    HeapFree( GetProcessHeap(), 0, lpData );
    IDirectPlayX_Release( pDP[0] );
    IDirectPlayX_Release( pDP[1] );

}


START_TEST(dplayx)
{
    CoInitialize( NULL );

    test_DirectPlayCreate();
    test_EnumConnections();
    test_InitializeConnection();

    test_GetCaps();
    test_Open();
    test_EnumSessions();
    test_SessionDesc();

    test_CreatePlayer();
    test_GetPlayerCaps();
    test_PlayerData();
    test_PlayerName();

    CoUninitialize();
}
