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
#define checkHR(expected, result)                       \
    ok( (expected) == (result),                         \
        "expected=%s got=%s\n",                         \
        dpResult2str(expected), dpResult2str(result) );
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


START_TEST(dplayx)
{
    CoInitialize( NULL );

    test_DirectPlayCreate();
    test_EnumConnections();
    test_InitializeConnection();

    test_GetCaps();

    CoUninitialize();
}
