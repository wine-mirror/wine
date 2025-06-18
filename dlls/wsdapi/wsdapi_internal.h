/*
 * Web Services on Devices
 * Internal header
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

#ifndef _WINE_WSDAPI_INTERNAL_H
#define _WINE_WSDAPI_INTERNAL_H

#include "winsock2.h"
#include "ws2tcpip.h"
#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "wsdapi.h"
#include "wine/list.h"

#define WSD_MAX_TEXT_LENGTH    8192

/* discovery.c */

struct notificationSink
{
    struct list entry;
    IWSDiscoveryPublisherNotify *notificationSink;
};

struct message_id
{
    struct list entry;
    LPWSTR id;
};

#define MAX_WSD_THREADS        20

typedef struct IWSDiscoveryPublisherImpl {
    IWSDiscoveryPublisher IWSDiscoveryPublisher_iface;
    LONG                  ref;
    IWSDXMLContext        *xmlContext;
    DWORD                 addressFamily;
    struct list           notificationSinks;
    CRITICAL_SECTION      notification_sink_critical_section;
    BOOL                  publisherStarted;
    HANDLE                thread_handles[MAX_WSD_THREADS];
    int                   num_thread_handles;
    struct list           message_ids;
    CRITICAL_SECTION      message_ids_critical_section;
} IWSDiscoveryPublisherImpl;

/* network.c */

BOOL init_networking(IWSDiscoveryPublisherImpl *impl);
void terminate_networking(IWSDiscoveryPublisherImpl *impl);
BOOL send_udp_multicast(IWSDiscoveryPublisherImpl *impl, char *data, int length, int max_initial_delay);
HRESULT send_udp_unicast(char *data, int length, IWSDUdpAddress *remote_addr, int max_initial_delay);

/* soap.c */

HRESULT send_hello_message(IWSDiscoveryPublisherImpl *impl, LPCWSTR id, ULONGLONG metadata_ver, ULONGLONG instance_id,
    ULONGLONG msg_num, LPCWSTR session_id, const WSD_NAME_LIST *types_list, const WSD_URI_LIST *scopes_list,
    const WSD_URI_LIST *xaddrs_list, const WSDXML_ELEMENT *hdr_any, const WSDXML_ELEMENT *ref_param_any,
    const WSDXML_ELEMENT *endpoint_ref_any, const WSDXML_ELEMENT *any);

HRESULT send_bye_message(IWSDiscoveryPublisherImpl *impl, LPCWSTR id, ULONGLONG instance_id, ULONGLONG msg_num,
    LPCWSTR session_id, const WSDXML_ELEMENT *any);

HRESULT send_probe_matches_message(IWSDiscoveryPublisherImpl *impl, const WSD_SOAP_MESSAGE *probe_msg,
    IWSDMessageParameters *message_params, LPCWSTR id, ULONGLONG metadata_ver, ULONGLONG instance_id,
    ULONGLONG msg_num, LPCWSTR session_id, const WSD_NAME_LIST *types_list, const WSD_URI_LIST *scopes_list,
    const WSD_URI_LIST *xaddrs_list, const WSDXML_ELEMENT *header_any, const WSDXML_ELEMENT *ref_param_any,
    const WSDXML_ELEMENT *endpoint_ref_any, const WSDXML_ELEMENT *any);

HRESULT register_namespaces(IWSDXMLContext *xml_context);

HRESULT read_message(IWSDiscoveryPublisherImpl *impl, const char *xml, int xml_length, WSD_SOAP_MESSAGE **out_msg,
    int *msg_type);

#define MSGTYPE_UNKNOWN     0
#define MSGTYPE_PROBE       1

/* xml.c */

WCHAR *duplicate_string(void *parent_memory_block, const WCHAR *value) __WINE_MALLOC;
WSDXML_NAME *duplicate_name(void *parent_memory_block, WSDXML_NAME *name) __WINE_MALLOC;
WSDXML_NAMESPACE *xml_context_find_namespace_by_prefix(IWSDXMLContext *context, LPCWSTR prefix);

#endif
