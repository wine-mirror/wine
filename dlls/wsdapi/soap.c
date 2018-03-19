/*
 * Web Services on Devices
 *
 * Copyright 2017-2018 Owen Rudge for CodeWeavers
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
#include <limits.h>

#define COBJMACROS

#include "wsdapi_internal.h"
#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(wsdapi);

#define APP_MAX_DELAY                       500

static const WCHAR discoveryTo[] = {
    'u','r','n',':',
    's','c','h','e','m','a','s','-','x','m','l','s','o','a','p','-','o','r','g',':',
    'w','s',':','2','0','0','5',':','0','4',':',
    'd','i','s','c','o','v','e','r','y', 0 };

static const WCHAR actionHello[] = {
    'h','t','t','p',':','/','/',
    's','c','h','e','m','a','s','.','x','m','l','s','o','a','p','.','o','r','g','/',
    'w','s','/','2','0','0','5','/','0','4','/',
    'd','i','s','c','o','v','e','r','y','/',
    'H','e','l','l','o', 0 };

static BOOL create_guid(LPWSTR buffer)
{
    const WCHAR formatString[] = { 'u','r','n',':','u','u','i','d',':','%','s', 0 };

    WCHAR* uuidString = NULL;
    UUID uuid;

    if (UuidCreate(&uuid) != RPC_S_OK)
        return FALSE;

    UuidToStringW(&uuid, (RPC_WSTR*)&uuidString);

    if (uuidString == NULL)
        return FALSE;

    wsprintfW(buffer, formatString, uuidString);
    RpcStringFreeW((RPC_WSTR*)&uuidString);

    return TRUE;
}

static void populate_soap_header(WSD_SOAP_HEADER *header, LPCWSTR to, LPCWSTR action, LPCWSTR message_id,
    WSD_APP_SEQUENCE *sequence, const WSDXML_ELEMENT *any_headers)
{
    ZeroMemory(header, sizeof(WSD_SOAP_HEADER));

    header->To = to;
    header->Action = action;
    header->MessageID = message_id;
    header->AppSequence = sequence;
    header->AnyHeaders = (WSDXML_ELEMENT *)any_headers;

    /* TODO: Implement RelatesTo, ReplyTo, From, FaultTo */
}

static HRESULT write_and_send_message(IWSDiscoveryPublisherImpl *impl, WSD_SOAP_HEADER *header, WSDXML_ELEMENT *body_element,
    struct list *discovered_namespaces, IWSDUdpAddress *remote_address, int max_initial_delay)
{
    static const char xml_header[] = "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
    ULONG xml_header_len = sizeof(xml_header) - 1;
    HRESULT ret = E_FAIL;
    char *full_xml;

    /* TODO: Create SOAP envelope */

    /* Prefix the XML header */
    full_xml = heap_alloc(xml_header_len + 1);

    if (full_xml == NULL)
        return E_OUTOFMEMORY;

    memcpy(full_xml, xml_header, xml_header_len);
    full_xml[xml_header_len] = 0;

    if (remote_address == NULL)
    {
        /* Send the message via UDP multicast */
        if (send_udp_multicast(impl, full_xml, sizeof(xml_header), max_initial_delay))
            ret = S_OK;
    }
    else
    {
        /* TODO: Send the message via UDP unicast */
        FIXME("TODO: Send the message via UDP unicast\n");
    }

    heap_free(full_xml);

    return ret;
}

HRESULT send_hello_message(IWSDiscoveryPublisherImpl *impl, LPCWSTR id, ULONGLONG metadata_ver, ULONGLONG instance_id,
    ULONGLONG msg_num, LPCWSTR session_id, const WSD_NAME_LIST *types_list, const WSD_URI_LIST *scopes_list,
    const WSD_URI_LIST *xaddrs_list, const WSDXML_ELEMENT *hdr_any, const WSDXML_ELEMENT *ref_param_any,
    const WSDXML_ELEMENT *endpoint_ref_any, const WSDXML_ELEMENT *any)
{
    WSD_SOAP_HEADER soapHeader;
    WSD_APP_SEQUENCE sequence;
    WCHAR message_id[64];
    HRESULT ret = E_OUTOFMEMORY;

    sequence.InstanceId = instance_id;
    sequence.MessageNumber = msg_num;
    sequence.SequenceId = session_id;

    if (!create_guid(message_id)) goto cleanup;

    populate_soap_header(&soapHeader, discoveryTo, actionHello, message_id, &sequence, hdr_any);

    /* TODO: Populate message body */

    /* Write and send the message */
    ret = write_and_send_message(impl, &soapHeader, NULL, NULL, NULL, APP_MAX_DELAY);

cleanup:
    return ret;
}
