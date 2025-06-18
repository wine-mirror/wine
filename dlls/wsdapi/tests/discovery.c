/*
 * Web Services on Devices
 * Discovery tests
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

#define COBJMACROS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include "wine/test.h"
#include "initguid.h"
#include "objbase.h"
#include "wsdapi.h"
#include <netfw.h>
#include <iphlpapi.h>
#include <stdio.h>

#define SEND_ADDRESS_IPV4   "239.255.255.250"
#define SEND_ADDRESS_IPV6   "FF02::C"
#define SEND_PORT           "3702"

static const char *publisherId = "urn:uuid:3AE5617D-790F-408A-9374-359A77F924A3";
static const char *sequenceId = "urn:uuid:b14de351-72fc-4453-96f9-e58b0c9faf38";

static const char testProbeMessage[] = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
    "<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\" "
    "xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" "
    "xmlns:wsd=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" "
    "xmlns:grog=\"http://more.tests/\"><soap:Header><wsa:To>urn:schemas-xmlsoap-org:ws:2005:04:discovery</wsa:To>"
    "<wsa:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</wsa:Action>"
    "<wsa:MessageID>urn:uuid:%s</wsa:MessageID>"
    "<wsd:AppSequence InstanceId=\"21\" SequenceId=\"urn:uuid:638abee8-124d-4b6a-8b85-8cf2837a2fd2\" MessageNumber=\"14\"></wsd:AppSequence>"
    "<grog:Perry>ExtraInfo</grog:Perry></soap:Header>"
    "<soap:Body><wsd:Probe><wsd:Types>grog:Cider</wsd:Types><grog:Lager>MoreInfo</grog:Lager></wsd:Probe></soap:Body></soap:Envelope>";

static const WCHAR *uri_more_tests = L"http://more.tests/";
static const WCHAR *uri_more_tests_no_slash = L"http://more.tests";
static const WCHAR *prefix_grog = L"grog";
static const WCHAR *name_cider = L"Cider";

static HANDLE probe_event = NULL;
static UUID probe_message_id;

static IWSDiscoveryPublisher *publisher_instance = NULL;

#define MAX_CACHED_MESSAGES     5
#define MAX_LISTENING_THREADS  20

typedef struct messageStorage {
    BOOL                  running;
    CRITICAL_SECTION      criticalSection;
    char*                 messages[MAX_CACHED_MESSAGES];
    int                   messageCount;
    HANDLE                threadHandles[MAX_LISTENING_THREADS];
    int                   numThreadHandles;
} messageStorage;

static LPWSTR utf8_to_wide(const char *utf8String)
{
    int sizeNeeded = 0, utf8StringLength = 0, memLength = 0;
    LPWSTR newString = NULL;

    if (utf8String == NULL) return NULL;
    utf8StringLength = lstrlenA(utf8String);

    sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, utf8String, utf8StringLength, NULL, 0);
    if (sizeNeeded <= 0) return NULL;

    memLength = sizeof(WCHAR) * (sizeNeeded + 1);
    newString = calloc(1, memLength);

    MultiByteToWideChar(CP_UTF8, 0, utf8String, utf8StringLength, newString, sizeNeeded);
    return newString;
}

static int join_multicast_group(SOCKET s, struct addrinfo *group, struct addrinfo *iface)
{
    int level, optname, optlen;
    struct ipv6_mreq mreqv6;
    struct ip_mreq mreqv4;
    char *optval;

    if (group->ai_family == AF_INET6)
    {
        level = IPPROTO_IPV6;
        optname = IPV6_ADD_MEMBERSHIP;
        optval = (char *)&mreqv6;
        optlen = sizeof(mreqv6);

        mreqv6.ipv6mr_multiaddr = ((SOCKADDR_IN6 *)group->ai_addr)->sin6_addr;
        mreqv6.ipv6mr_interface = ((SOCKADDR_IN6 *)iface->ai_addr)->sin6_scope_id;
    }
    else
    {
        level = IPPROTO_IP;
        optname = IP_ADD_MEMBERSHIP;
        optval = (char *)&mreqv4;
        optlen = sizeof(mreqv4);

        mreqv4.imr_multiaddr.s_addr = ((SOCKADDR_IN *)group->ai_addr)->sin_addr.s_addr;
        mreqv4.imr_interface.s_addr = ((SOCKADDR_IN *)iface->ai_addr)->sin_addr.s_addr;
    }

    return setsockopt(s, level, optname, optval, optlen);
}

static int set_send_interface(SOCKET s, struct addrinfo *iface)
{
    int level, optname, optlen;
    char *optval = NULL;

    if (iface->ai_family == AF_INET6)
    {
        level = IPPROTO_IPV6;
        optname = IPV6_MULTICAST_IF;
        optval = (char *) &((SOCKADDR_IN6 *)iface->ai_addr)->sin6_scope_id;
        optlen = sizeof(((SOCKADDR_IN6 *)iface->ai_addr)->sin6_scope_id);
    }
    else
    {
        level = IPPROTO_IP;
        optname = IP_MULTICAST_IF;
        optval = (char *) &((SOCKADDR_IN *)iface->ai_addr)->sin_addr.s_addr;
        optlen = sizeof(((SOCKADDR_IN *)iface->ai_addr)->sin_addr.s_addr);
    }

    return setsockopt(s, level, optname, optval, optlen);
}

static struct addrinfo *resolve_address(const char *address, const char *port, int family, int type, int protocol)
{
    struct addrinfo hints, *result = NULL;

    ZeroMemory(&hints, sizeof(hints));

    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = family;
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;

    return getaddrinfo(address, port, &hints, &result) == 0 ? result : NULL;
}

typedef struct listenerThreadParams
{
    messageStorage *msgStorage;
    SOCKET listeningSocket;
} listenerThreadParams;

#define RECEIVE_BUFFER_SIZE        65536

static DWORD WINAPI listening_thread(LPVOID lpParam)
{
    listenerThreadParams *parameter = (listenerThreadParams *)lpParam;
    messageStorage *msgStorage = parameter->msgStorage;
    int bytesReceived;
    char *buffer;

    buffer = malloc(RECEIVE_BUFFER_SIZE);

    while (parameter->msgStorage->running)
    {
        ZeroMemory(buffer, RECEIVE_BUFFER_SIZE);
        bytesReceived = recv(parameter->listeningSocket, buffer, RECEIVE_BUFFER_SIZE, 0);

        if (bytesReceived == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSAETIMEDOUT)
                return 0;
        }
        else
        {
            EnterCriticalSection(&msgStorage->criticalSection);

            if (msgStorage->messageCount < MAX_CACHED_MESSAGES)
            {
                msgStorage->messages[msgStorage->messageCount] = malloc(bytesReceived);

                if (msgStorage->messages[msgStorage->messageCount] != NULL)
                {
                    memcpy(msgStorage->messages[msgStorage->messageCount], buffer, bytesReceived);
                    msgStorage->messageCount++;
                }
            }

            LeaveCriticalSection(&msgStorage->criticalSection);

            if (msgStorage->messageCount >= MAX_CACHED_MESSAGES)
            {
                /* Stop all threads */
                msgStorage->running = FALSE;
                break;
            }
        }
    }

    closesocket(parameter->listeningSocket);

    free(buffer);
    free(parameter);

    return 0;
}

static BOOL start_listening_udp_unicast(messageStorage *msgStorage, struct sockaddr_in *address)
{
    listenerThreadParams *parameter = NULL;
    const DWORD receive_timeout = 500;
    const UINT reuse_addr = 1;
    HANDLE hThread;
    SOCKET s = 0;

    /* Create the socket */
    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) goto cleanup;

    /* Ensure the socket can be reused */
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse_addr, sizeof(reuse_addr)) == SOCKET_ERROR) goto cleanup;

    /* Bind the socket to the local interface so we can receive data */
    if (bind(s, (struct sockaddr *) address, sizeof(struct sockaddr_in)) == SOCKET_ERROR) goto cleanup;

    /* Set a 500ms receive timeout */
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *) &receive_timeout, sizeof(receive_timeout)) == SOCKET_ERROR)
        goto cleanup;

    /* Allocate memory for thread parameters */
    parameter = malloc(sizeof(*parameter));

    parameter->msgStorage = msgStorage;
    parameter->listeningSocket = s;

    hThread = CreateThread(NULL, 0, listening_thread, parameter, 0, NULL);
    if (hThread == NULL) goto cleanup;

    msgStorage->threadHandles[msgStorage->numThreadHandles] = hThread;
    msgStorage->numThreadHandles++;

    return TRUE;

cleanup:
    closesocket(s);
    free(parameter);

    return FALSE;
}

static void start_listening(messageStorage *msgStorage, const char *multicastAddress, const char *bindAddress)
{
    struct addrinfo *multicastAddr = NULL, *bindAddr = NULL, *interfaceAddr = NULL;
    listenerThreadParams *parameter = NULL;
    const DWORD receiveTimeout = 500;
    const UINT reuseAddr = 1;
    HANDLE hThread;
    SOCKET s = 0;

    /* Resolve the multicast address */
    multicastAddr = resolve_address(multicastAddress, SEND_PORT, AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP);
    if (multicastAddr == NULL) goto cleanup;

    /* Resolve the binding address */
    bindAddr = resolve_address(bindAddress, SEND_PORT, multicastAddr->ai_family, multicastAddr->ai_socktype, multicastAddr->ai_protocol);
    if (bindAddr == NULL) goto cleanup;

    /* Resolve the multicast interface */
    interfaceAddr = resolve_address(bindAddress, "0", multicastAddr->ai_family, multicastAddr->ai_socktype, multicastAddr->ai_protocol);
    if (interfaceAddr == NULL) goto cleanup;

    /* Create the socket */
    s = socket(multicastAddr->ai_family, multicastAddr->ai_socktype, multicastAddr->ai_protocol);
    if (s == INVALID_SOCKET) goto cleanup;

    /* Ensure the socket can be reused */
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuseAddr, sizeof(reuseAddr)) == SOCKET_ERROR) goto cleanup;

    /* Bind the socket to the local interface so we can receive data */
    if (bind(s, bindAddr->ai_addr, bindAddr->ai_addrlen) == SOCKET_ERROR) goto cleanup;

    /* Join the multicast group */
    if (join_multicast_group(s, multicastAddr, interfaceAddr) == SOCKET_ERROR) goto cleanup;

    /* Set the outgoing interface */
    if (set_send_interface(s, interfaceAddr) == SOCKET_ERROR) goto cleanup;

    /* For IPv6, ensure the scope ID is zero */
    if (multicastAddr->ai_family == AF_INET6)
        ((SOCKADDR_IN6 *)multicastAddr->ai_addr)->sin6_scope_id = 0;

    /* Set a 500ms receive timeout */
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&receiveTimeout, sizeof(receiveTimeout)) == SOCKET_ERROR) goto cleanup;

    /* Allocate memory for thread parameters */
    parameter = malloc(sizeof(*parameter));

    parameter->msgStorage = msgStorage;
    parameter->listeningSocket = s;

    hThread = CreateThread(NULL, 0, listening_thread, parameter, 0, NULL);
    if (hThread == NULL) goto cleanup;

    msgStorage->threadHandles[msgStorage->numThreadHandles] = hThread;
    msgStorage->numThreadHandles++;

    goto cleanup_addresses;

cleanup:
    closesocket(s);
    free(parameter);

cleanup_addresses:
    freeaddrinfo(multicastAddr);
    freeaddrinfo(bindAddr);
    freeaddrinfo(interfaceAddr);
}

static IP_ADAPTER_ADDRESSES *get_adapters( ULONG family )
{
    ULONG err, size = 4096;
    IP_ADAPTER_ADDRESSES *tmp, *ret;

    if (!(ret = malloc( size ))) return NULL;
    err = GetAdaptersAddresses( family, 0, NULL, ret, &size );
    while (err == ERROR_BUFFER_OVERFLOW)
    {
        if (!(tmp = realloc( ret, size ))) break;
        ret = tmp;
        err = GetAdaptersAddresses( family, 0, NULL, ret, &size );
    }
    if (err == ERROR_SUCCESS) return ret;
    free( ret );
    return NULL;
}

static BOOL start_listening_on_all_addresses(messageStorage *msgStorage, ULONG family)
{
    IP_ADAPTER_ADDRESSES *adapterAddresses, *adapterAddress;
    LPSOCKADDR sockaddr;
    DWORD addressLength;
    char address[64];
    BOOL ret = FALSE;

    adapterAddresses = get_adapters(family); /* family should be AF_INET or AF_INET6 */
    if (!adapterAddresses) return FALSE;

    for (adapterAddress = adapterAddresses; adapterAddress != NULL; adapterAddress = adapterAddress->Next)
    {
        if (msgStorage->numThreadHandles >= MAX_LISTENING_THREADS)
        {
            ret = TRUE;
            goto cleanup;
        }

        if (adapterAddress->FirstUnicastAddress == NULL) continue;

        sockaddr = adapterAddress->FirstUnicastAddress->Address.lpSockaddr;
        addressLength = sizeof(address);
        WSAAddressToStringA(sockaddr, adapterAddress->FirstUnicastAddress->Address.iSockaddrLength, NULL, address, &addressLength);

        start_listening(msgStorage, adapterAddress->FirstUnicastAddress->Address.lpSockaddr->sa_family == AF_INET ? SEND_ADDRESS_IPV4 : SEND_ADDRESS_IPV6, address);
    }

    ret = TRUE;

cleanup:
    free(adapterAddresses);
    return ret;
}

static BOOL send_udp_multicast_of_type(const char *data, int length, ULONG family)
{
    IP_ADAPTER_ADDRESSES *adapter_addresses, *adapter_addr;
    static const struct in6_addr i_addr_zero;
    struct addrinfo *multi_address;
    LPSOCKADDR sockaddr;
    const char ttl = 8;
    SOCKET s;

   /* Resolve the multicast address */
    if (family == AF_INET6)
        multi_address = resolve_address(SEND_ADDRESS_IPV6, SEND_PORT, AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    else
        multi_address = resolve_address(SEND_ADDRESS_IPV4, SEND_PORT, AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (multi_address == NULL)
        return FALSE;

    adapter_addresses = get_adapters(family);
    if (!adapter_addresses) return FALSE;

    for (adapter_addr = adapter_addresses; adapter_addr != NULL; adapter_addr = adapter_addr->Next)
    {
        if (adapter_addr->FirstUnicastAddress == NULL) continue;

        sockaddr = adapter_addr->FirstUnicastAddress->Address.lpSockaddr;

        /* Create a socket and bind to the adapter address */
        s = socket(family, SOCK_DGRAM, IPPROTO_UDP);
        if (s == INVALID_SOCKET) continue;

        if (bind(s, sockaddr, adapter_addr->FirstUnicastAddress->Address.iSockaddrLength) == SOCKET_ERROR)
        {
            closesocket(s);
            continue;
        }

        /* Set the multicast interface and TTL value */
        setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, (char *) &i_addr_zero,
            (family == AF_INET6) ? sizeof(struct in6_addr) : sizeof(struct in_addr));
        setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

        sendto(s, data, length, 0, (SOCKADDR *) multi_address->ai_addr, multi_address->ai_addrlen);
        closesocket(s);
    }

    freeaddrinfo(multi_address);
    free(adapter_addresses);
    return TRUE;
}

typedef struct IWSDiscoveryPublisherNotifyImpl {
    IWSDiscoveryPublisherNotify IWSDiscoveryPublisherNotify_iface;
    LONG                  ref;
} IWSDiscoveryPublisherNotifyImpl;

static inline IWSDiscoveryPublisherNotifyImpl *impl_from_IWSDiscoveryPublisherNotify(IWSDiscoveryPublisherNotify *iface)
{
    return CONTAINING_RECORD(iface, IWSDiscoveryPublisherNotifyImpl, IWSDiscoveryPublisherNotify_iface);
}

static HRESULT WINAPI IWSDiscoveryPublisherNotifyImpl_QueryInterface(IWSDiscoveryPublisherNotify *iface, REFIID riid, void **ppv)
{
    IWSDiscoveryPublisherNotifyImpl *This = impl_from_IWSDiscoveryPublisherNotify(iface);

    if (!ppv)
    {
        return E_INVALIDARG;
    }

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IWSDiscoveryPublisherNotify))
    {
        *ppv = &This->IWSDiscoveryPublisherNotify_iface;
    }
    else
    {
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI IWSDiscoveryPublisherNotifyImpl_AddRef(IWSDiscoveryPublisherNotify *iface)
{
    IWSDiscoveryPublisherNotifyImpl *This = impl_from_IWSDiscoveryPublisherNotify(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    trace("IWSDiscoveryPublisherNotifyImpl_AddRef called (%p, ref = %ld)\n", This, ref);
    return ref;
}

static ULONG WINAPI IWSDiscoveryPublisherNotifyImpl_Release(IWSDiscoveryPublisherNotify *iface)
{
    IWSDiscoveryPublisherNotifyImpl *This = impl_from_IWSDiscoveryPublisherNotify(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    trace("IWSDiscoveryPublisherNotifyImpl_Release called (%p, ref = %ld)\n", This, ref);

    if (ref == 0)
    {
        free(This);
    }

    return ref;
}

static void verify_wsdxml_name(const char *debug_prefix, WSDXML_NAME *name, LPCWSTR uri, LPCWSTR prefix,
    LPCWSTR local_name)
{
    ok(name != NULL, "%s: name == NULL\n", debug_prefix);
    if (name == NULL) return;

    ok(name->LocalName != NULL && lstrcmpW(name->LocalName, local_name) == 0,
        "%s: Local name = '%s'\n", debug_prefix, wine_dbgstr_w(name->LocalName));

    ok(name->Space != NULL, "%s: Space == NULL\n", debug_prefix);
    if (name->Space == NULL) return;

    ok(name->Space->Uri != NULL && lstrcmpW(name->Space->Uri, uri) == 0,
        "%s: URI == '%s'\n", debug_prefix, wine_dbgstr_w(name->Space->Uri));
    ok(name->Space->PreferredPrefix != NULL && lstrcmpW(name->Space->PreferredPrefix, prefix) == 0,
        "%s: Prefix = '%s'\n", debug_prefix, wine_dbgstr_w(name->Space->PreferredPrefix));
}

static void verify_wsdxml_any_text(const char *debug_prefix, WSDXML_ELEMENT *any, LPCWSTR uri, LPCWSTR prefix,
    LPCWSTR local_name, LPCWSTR value)
{
    WSDXML_TEXT *child;

    ok(any != NULL, "%s: any == NULL\n", debug_prefix);
    if (any == NULL) return;

    child = (WSDXML_TEXT *) any->FirstChild;

    ok(any->Node.Type == ElementType, "%s: Node type == %d\n", debug_prefix, any->Node.Type);
    ok(any->Node.Parent == NULL, "%s: Parent == %p\n", debug_prefix, any->Node.Parent);
    ok(any->Node.Next == NULL, "%s: Next == %p\n", debug_prefix, any->Node.Next);
    verify_wsdxml_name(debug_prefix, any->Name, uri, prefix, local_name);

    ok(child != NULL, "%s: First child == NULL\n", debug_prefix);

    if (child != NULL)
    {
        ok(child->Node.Type == TextType, "%s: Node type == %d\n", debug_prefix, child->Node.Type);
        ok(child->Node.Parent == any, "%s: Parent == %p\n", debug_prefix, child->Node.Parent);
        ok(child->Node.Next == NULL, "%s: Next == %p\n", debug_prefix, child->Node.Next);

        if (child->Node.Type == TextType)
            ok(child->Text != NULL && lstrcmpW(child->Text, value) == 0,
                "%s: Text == '%s'\n", debug_prefix, wine_dbgstr_w(child->Text));
    }
}

static HRESULT WINAPI IWSDiscoveryPublisherNotifyImpl_ProbeHandler(IWSDiscoveryPublisherNotify *This, const WSD_SOAP_MESSAGE *pSoap, IWSDMessageParameters *pMessageParameters)
{
    trace("IWSDiscoveryPublisherNotifyImpl_ProbeHandler called (%p, %p, %p)\n", This, pSoap, pMessageParameters);

    if (probe_event == NULL)
    {
        /* We may have received an unrelated probe on the network */
        return S_OK;
    }

    ok(pSoap != NULL, "pSoap == NULL\n");
    ok(pMessageParameters != NULL, "pMessageParameters == NULL\n");

    if (pSoap != NULL)
    {
        WSD_PROBE *probe_msg = (WSD_PROBE *) pSoap->Body;
        WSD_APP_SEQUENCE *appseq = (WSD_APP_SEQUENCE *) pSoap->Header.AppSequence;

        ok(pSoap->Body != NULL, "pSoap->Body == NULL\n");
        ok(pSoap->Header.To != NULL && lstrcmpW(pSoap->Header.To, L"urn:schemas-xmlsoap-org:ws:2005:04:discovery") == 0,
            "pSoap->Header.To == '%s'\n", wine_dbgstr_w(pSoap->Header.To));
        ok(pSoap->Header.Action != NULL && lstrcmpW(pSoap->Header.Action, L"http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe") == 0,
            "pSoap->Header.Action == '%s'\n", wine_dbgstr_w(pSoap->Header.Action));

        ok(pSoap->Header.MessageID != NULL, "pSoap->Header.MessageID == NULL\n");

        /* Ensure the message ID is at least 9 characters long (to skip past the 'urn:uuid:' prefix) */
        if ((pSoap->Header.MessageID != NULL) && (lstrlenW(pSoap->Header.MessageID) > 9))
        {
            UUID uuid;
            RPC_STATUS ret = UuidFromStringW((LPWSTR)pSoap->Header.MessageID + 9, &uuid);

            trace("Received message with UUID '%s' (expected UUID '%s')\n", wine_dbgstr_guid(&uuid),
                wine_dbgstr_guid(&probe_message_id));

            /* Check if we've either received a message without a UUID, or the UUID isn't the one we sent. If so,
               ignore it and wait for another message. */
            if ((ret != RPC_S_OK) || (UuidEqual(&uuid, &probe_message_id, &ret) == FALSE)) return S_OK;
        }

        ok(appseq != NULL, "pSoap->Header.AppSequence == NULL\n");

        if (appseq != NULL)
        {
            ok(appseq->InstanceId == 21, "pSoap->Header.AppSequence->InstanceId = %s\n",
                wine_dbgstr_longlong(appseq->InstanceId));
            ok(lstrcmpW(appseq->SequenceId, L"urn:uuid:638abee8-124d-4b6a-8b85-8cf2837a2fd2") == 0, "pSoap->Header.AppSequence->SequenceId = '%s'\n",
                wine_dbgstr_w(appseq->SequenceId));
            ok(appseq->MessageNumber == 14, "pSoap->Header.AppSequence->MessageNumber = %s\n",
                wine_dbgstr_longlong(appseq->MessageNumber));
        }

        verify_wsdxml_any_text("pSoap->Header.AnyHeaders", pSoap->Header.AnyHeaders, uri_more_tests_no_slash,
            prefix_grog, L"Perry", L"ExtraInfo");

        if (probe_msg != NULL)
        {
            IWSDUdpAddress *remote_addr = NULL;
            HRESULT rc;

            ok(probe_msg->Types != NULL, "Probe message Types == NULL\n");

            if (probe_msg->Types != NULL)
            {
                verify_wsdxml_name("probe_msg->Types->Element", probe_msg->Types->Element, uri_more_tests_no_slash,
                    prefix_grog, name_cider);
                ok(probe_msg->Types->Next == NULL, "probe_msg->Types->Next == %p\n", probe_msg->Types->Next);
            }

            ok(probe_msg->Scopes == NULL, "Probe message Scopes != NULL\n");
            verify_wsdxml_any_text("probe_msg->Any", probe_msg->Any, uri_more_tests_no_slash, prefix_grog, L"Lager", L"MoreInfo");

            rc = IWSDMessageParameters_GetRemoteAddress(pMessageParameters, (IWSDAddress **) &remote_addr);
            ok(rc == S_OK, "IWSDMessageParameters_GetRemoteAddress returned %08lx\n", rc);

            if (remote_addr != NULL)
            {
                messageStorage *msg_storage;
                char endpoint_reference_string[MAX_PATH], app_sequence_string[MAX_PATH];
                LPWSTR publisherIdW = NULL, sequenceIdW = NULL;
                SOCKADDR_STORAGE remote_sock;
                WSDXML_ELEMENT *header_any_element, *body_any_element, *endpoint_any_element, *ref_param_any_element;
                WSDXML_NAME header_any_name;
                WSDXML_NAMESPACE ns;
                BOOL probe_matches_message_seen = FALSE, endpoint_reference_seen = FALSE, app_sequence_seen = FALSE;
                BOOL metadata_version_seen = FALSE, wine_ns_seen = FALSE, body_probe_matches_seen = FALSE;
                BOOL types_seen = FALSE, any_header_seen = FALSE, any_body_seen = FALSE;
                BOOL message_ok;
                char *msg;
                int i;

                rc = IWSDUdpAddress_GetSockaddr(remote_addr, &remote_sock);
                ok(rc == S_OK, "IWSDMessageParameters_GetRemoteAddress returned %08lx\n", rc);

                IWSDUdpAddress_Release(remote_addr);

                msg_storage = calloc(1, sizeof(*msg_storage));
                if (msg_storage == NULL) goto after_matchprobe_test;

                msg_storage->running = TRUE;
                InitializeCriticalSection(&msg_storage->criticalSection);

                ok(start_listening_udp_unicast(msg_storage, (struct sockaddr_in *) &remote_sock) == TRUE,
                    "Unable to listen on local socket for UDP messages\n");

                publisherIdW = utf8_to_wide(publisherId);
                sequenceIdW = utf8_to_wide(sequenceId);

                /* Create "any" elements for header */
                ns.Uri = L"http://wine.test/";
                ns.PreferredPrefix = L"wine";

                header_any_name.LocalName = (WCHAR *) L"Beer";
                header_any_name.Space = &ns;

                rc = WSDXMLBuildAnyForSingleElement(&header_any_name, L"PublishTest", &header_any_element);
                ok(rc == S_OK, "WSDXMLBuildAnyForSingleElement failed with %08lx\n", rc);

                rc = WSDXMLBuildAnyForSingleElement(&header_any_name, L"BodyTest", &body_any_element);
                ok(rc == S_OK, "WSDXMLBuildAnyForSingleElement failed with %08lx\n", rc);

                rc = WSDXMLBuildAnyForSingleElement(&header_any_name, L"EndPTest", &endpoint_any_element);
                ok(rc == S_OK, "WSDXMLBuildAnyForSingleElement failed with %08lx\n", rc);

                rc = WSDXMLBuildAnyForSingleElement(&header_any_name, L"RefPTest", &ref_param_any_element);
                ok(rc == S_OK, "WSDXMLBuildAnyForSingleElement failed with %08lx\n", rc);

                rc = IWSDiscoveryPublisher_MatchProbeEx(publisher_instance, pSoap, pMessageParameters, publisherIdW, 1, 1, 1,
                    sequenceIdW, probe_msg->Types, NULL, NULL, header_any_element, ref_param_any_element, NULL,
                    endpoint_any_element, body_any_element);
                ok(rc == S_OK, "IWSDiscoveryPublisher_MatchProbeEx failed with %08lx\n", rc);

                WSDFreeLinkedMemory(header_any_element);
                WSDFreeLinkedMemory(body_any_element);
                WSDFreeLinkedMemory(endpoint_any_element);
                WSDFreeLinkedMemory(ref_param_any_element);

                /* Wait up to 2 seconds for messages to be received */
                if (WaitForMultipleObjects(msg_storage->numThreadHandles, msg_storage->threadHandles, TRUE, 5000) == WAIT_TIMEOUT)
                {
                    /* Wait up to 1 more second for threads to terminate */
                    msg_storage->running = FALSE;
                    WaitForMultipleObjects(msg_storage->numThreadHandles, msg_storage->threadHandles, TRUE, 1000);
                }

                for (i = 0; i < msg_storage->numThreadHandles; i++)
                {
                    CloseHandle(msg_storage->threadHandles[i]);
                }

                DeleteCriticalSection(&msg_storage->criticalSection);

                /* Verify we've received a message */
                ok(msg_storage->messageCount >= 1, "No messages received\n");

                sprintf(endpoint_reference_string, "<wsa:EndpointReference><wsa:Address>%s</wsa:Address>"
                    "<wsa:ReferenceParameters><wine:Beer>RefPTest</wine:Beer></wsa:ReferenceParameters>"
                    "<wine:Beer>EndPTest</wine:Beer></wsa:EndpointReference>", publisherId);

                sprintf(app_sequence_string, "<wsd:AppSequence InstanceId=\"1\" SequenceId=\"%s\" MessageNumber=\"1\"></wsd:AppSequence>",
                    sequenceId);

                message_ok = FALSE;

                /* Check we're received the correct message */
                for (i = 0; i < msg_storage->messageCount; i++)
                {
                    msg = msg_storage->messages[i];
                    message_ok = FALSE;

                    probe_matches_message_seen = (strstr(msg,
                        "<wsa:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/ProbeMatches</wsa:Action>") != NULL);
                    endpoint_reference_seen = (strstr(msg, endpoint_reference_string) != NULL);
                    app_sequence_seen = (strstr(msg, app_sequence_string) != NULL);
                    metadata_version_seen = (strstr(msg, "<wsd:MetadataVersion>1</wsd:MetadataVersion>") != NULL);
                    any_header_seen = (strstr(msg, "<wine:Beer>PublishTest</wine:Beer>") != NULL);
                    wine_ns_seen = (strstr(msg, "xmlns:grog=\"http://more.tests\"") != NULL);
                    body_probe_matches_seen = (strstr(msg, "<soap:Body><wsd:ProbeMatches") != NULL);
                    any_body_seen = (strstr(msg, "<wine:Beer>BodyTest</wine:Beer>") != NULL);
                    types_seen = (strstr(msg, "<wsd:Types>grog:Cider</wsd:Types>") != NULL);
                    message_ok = probe_matches_message_seen && endpoint_reference_seen && app_sequence_seen &&
                        metadata_version_seen && body_probe_matches_seen && types_seen;

                    if (message_ok) break;
                }

                for (i = 0; i < msg_storage->messageCount; i++)
                {
                    free(msg_storage->messages[i]);
                }

                ok(probe_matches_message_seen == TRUE, "Probe matches message not received\n");
                ok(endpoint_reference_seen == TRUE, "EndpointReference not received\n");
                ok(app_sequence_seen == TRUE, "AppSequence not received\n");
                ok(metadata_version_seen == TRUE, "MetadataVersion not received\n");
                ok(message_ok == TRUE, "ProbeMatches message metadata not received\n");
                ok(any_header_seen == TRUE, "Custom header not received\n");
                ok(wine_ns_seen == TRUE, "Wine namespace not received\n");
                ok(body_probe_matches_seen == TRUE, "Body and Probe Matches elements not received\n");
                ok(any_body_seen == TRUE, "Custom body element not received\n");
                ok(types_seen == TRUE, "Types not received\n");

after_matchprobe_test:
                free(publisherIdW);
                free(sequenceIdW);
                free(msg_storage);
            }
        }
    }

    SetEvent(probe_event);
    return S_OK;
}

static HRESULT WINAPI IWSDiscoveryPublisherNotifyImpl_ResolveHandler(IWSDiscoveryPublisherNotify *This, const WSD_SOAP_MESSAGE *pSoap, IWSDMessageParameters *pMessageParameters)
{
    trace("IWSDiscoveryPublisherNotifyImpl_ResolveHandler called (%p, %p, %p)\n", This, pSoap, pMessageParameters);
    return S_OK;
}

static const IWSDiscoveryPublisherNotifyVtbl publisherNotify_vtbl =
{
    IWSDiscoveryPublisherNotifyImpl_QueryInterface,
    IWSDiscoveryPublisherNotifyImpl_AddRef,
    IWSDiscoveryPublisherNotifyImpl_Release,
    IWSDiscoveryPublisherNotifyImpl_ProbeHandler,
    IWSDiscoveryPublisherNotifyImpl_ResolveHandler
};

static BOOL create_discovery_publisher_notify(IWSDiscoveryPublisherNotify **publisherNotify)
{
    IWSDiscoveryPublisherNotifyImpl *obj;

    *publisherNotify = NULL;

    obj = calloc(1, sizeof(*obj));

    if (!obj)
    {
        trace("Out of memory creating IWSDiscoveryPublisherNotify\n");
        return FALSE;
    }

    obj->IWSDiscoveryPublisherNotify_iface.lpVtbl = &publisherNotify_vtbl;
    obj->ref = 1;

    *publisherNotify = &obj->IWSDiscoveryPublisherNotify_iface;

    return TRUE;
}

static void CreateDiscoveryPublisher_tests(void)
{
    IWSDiscoveryPublisher *publisher = NULL;
    IWSDiscoveryPublisher *publisher2;
    IUnknown *unknown;
    HRESULT rc;
    ULONG ref;

    rc = WSDCreateDiscoveryPublisher(NULL, NULL);
    ok((rc == E_POINTER) || (rc == E_INVALIDARG), "WSDCreateDiscoveryPublisher(NULL, NULL) failed: %08lx\n", rc);

    rc = WSDCreateDiscoveryPublisher(NULL, &publisher);
    ok(rc == S_OK, "WSDCreateDiscoveryPublisher(NULL, &publisher) failed: %08lx\n", rc);
    ok(publisher != NULL, "WSDCreateDiscoveryPublisher(NULL, &publisher) failed: publisher == NULL\n");

    /* Try to query for objects */
    rc = IWSDiscoveryPublisher_QueryInterface(publisher, &IID_IUnknown, (LPVOID*)&unknown);
    ok(rc == S_OK,"IWSDiscoveryPublisher_QueryInterface(IID_IUnknown) failed: %08lx\n", rc);

    if (rc == S_OK)
        IUnknown_Release(unknown);

    rc = IWSDiscoveryPublisher_QueryInterface(publisher, &IID_IWSDiscoveryPublisher, (LPVOID*)&publisher2);
    ok(rc == S_OK,"IWSDiscoveryPublisher_QueryInterface(IID_IWSDiscoveryPublisher) failed: %08lx\n", rc);

    if (rc == S_OK)
        IWSDiscoveryPublisher_Release(publisher2);

    ref = IWSDiscoveryPublisher_Release(publisher);
    ok(ref == 0, "IWSDiscoveryPublisher_Release() has %ld references, should have 0\n", ref);
}

static void CreateDiscoveryPublisher_XMLContext_tests(void)
{
    IWSDiscoveryPublisher *publisher = NULL;
    IWSDXMLContext *xmlContext, *returnedContext;
    HRESULT rc;
    int ref;

    /* Test creating an XML context and supplying it to WSDCreateDiscoveryPublisher */
    rc = WSDXMLCreateContext(&xmlContext);
    ok(rc == S_OK, "WSDXMLCreateContext failed: %08lx\n", rc);

    rc = WSDCreateDiscoveryPublisher(xmlContext, &publisher);
    ok(rc == S_OK, "WSDCreateDiscoveryPublisher(xmlContext, &publisher) failed: %08lx\n", rc);
    ok(publisher != NULL, "WSDCreateDiscoveryPublisher(xmlContext, &publisher) failed: publisher == NULL\n");

    rc = IWSDiscoveryPublisher_GetXMLContext(publisher, NULL);
    ok(rc == E_INVALIDARG, "GetXMLContext returned unexpected value with NULL argument: %08lx\n", rc);

    rc = IWSDiscoveryPublisher_GetXMLContext(publisher, &returnedContext);
    ok(rc == S_OK, "GetXMLContext failed: %08lx\n", rc);

    ok(xmlContext == returnedContext, "GetXMLContext returned unexpected value: returnedContext == %p\n", returnedContext);

    ref = IWSDXMLContext_Release(returnedContext);
    ok(ref == 2, "IWSDXMLContext_Release() has %d references, should have 2\n", ref);

    ref = IWSDiscoveryPublisher_Release(publisher);
    ok(ref == 0, "IWSDiscoveryPublisher_Release() has %d references, should have 0\n", ref);

    ref = IWSDXMLContext_Release(returnedContext);
    ok(ref == 0, "IWSDXMLContext_Release() has %d references, should have 0\n", ref);

    /* Test using a default XML context */
    publisher = NULL;
    returnedContext = NULL;

    rc = WSDCreateDiscoveryPublisher(NULL, &publisher);
    ok(rc == S_OK, "WSDCreateDiscoveryPublisher(NULL, &publisher) failed: %08lx\n", rc);
    ok(publisher != NULL, "WSDCreateDiscoveryPublisher(NULL, &publisher) failed: publisher == NULL\n");

    rc = IWSDiscoveryPublisher_GetXMLContext(publisher, &returnedContext);
    ok(rc == S_OK, "GetXMLContext failed: %08lx\n", rc);

    ref = IWSDXMLContext_Release(returnedContext);
    ok(ref == 1, "IWSDXMLContext_Release() has %d references, should have 1\n", ref);

    ref = IWSDiscoveryPublisher_Release(publisher);
    ok(ref == 0, "IWSDiscoveryPublisher_Release() has %d references, should have 0\n", ref);
}

static void Publish_tests(void)
{
    IWSDiscoveryPublisher *publisher = NULL;
    IWSDiscoveryPublisherNotify *sink1 = NULL, *sink2 = NULL;
    IWSDiscoveryPublisherNotifyImpl *sink1Impl = NULL, *sink2Impl = NULL;
    char endpointReferenceString[MAX_PATH], app_sequence_string[MAX_PATH];
    LPWSTR publisherIdW = NULL, sequenceIdW = NULL;
    messageStorage *msgStorage;
    WSADATA wsaData;
    BOOL messageOK, hello_message_seen = FALSE, endpoint_reference_seen = FALSE, app_sequence_seen = FALSE;
    BOOL metadata_version_seen = FALSE, any_header_seen = FALSE, wine_ns_seen = FALSE, body_hello_seen = FALSE;
    BOOL any_body_seen = FALSE, types_seen = FALSE, xml_namespaces_seen = FALSE, scopes_seen = FALSE;
    BOOL xaddrs_seen = FALSE;
    int ret, i;
    HRESULT rc;
    ULONG ref;
    char *msg;
    WSDXML_ELEMENT *header_any_element, *body_any_element, *endpoint_any_element, *ref_param_any_element;
    WSDXML_NAME header_any_name, another_name;
    WSDXML_NAMESPACE ns, ns2;
    static const WCHAR *uri = L"http://wine.test/";
    WSD_NAME_LIST types_list;
    WSD_URI_LIST scopes_list, xaddrs_list;
    unsigned char *probe_uuid_str;

    rc = WSDCreateDiscoveryPublisher(NULL, &publisher);
    ok(rc == S_OK, "WSDCreateDiscoveryPublisher(NULL, &publisher) failed: %08lx\n", rc);
    ok(publisher != NULL, "WSDCreateDiscoveryPublisher(NULL, &publisher) failed: publisher == NULL\n");

    publisher_instance = publisher;

    /* Test SetAddressFamily */
    rc = IWSDiscoveryPublisher_SetAddressFamily(publisher, 12345);
    ok(rc == E_INVALIDARG, "IWSDiscoveryPublisher_SetAddressFamily(12345) returned unexpected result: %08lx\n", rc);

    rc = IWSDiscoveryPublisher_SetAddressFamily(publisher, WSDAPI_ADDRESSFAMILY_IPV4);
    ok(rc == S_OK, "IWSDiscoveryPublisher_SetAddressFamily(WSDAPI_ADDRESSFAMILY_IPV4) failed: %08lx\n", rc);

    /* Try to update the address family after already setting it */
    rc = IWSDiscoveryPublisher_SetAddressFamily(publisher, WSDAPI_ADDRESSFAMILY_IPV6);
    ok(rc == STG_E_INVALIDFUNCTION, "IWSDiscoveryPublisher_SetAddressFamily(WSDAPI_ADDRESSFAMILY_IPV6) returned unexpected result: %08lx\n", rc);

    /* Create notification sinks */
    ok(create_discovery_publisher_notify(&sink1) == TRUE, "create_discovery_publisher_notify failed\n");
    ok(create_discovery_publisher_notify(&sink2) == TRUE, "create_discovery_publisher_notify failed\n");

    /* Get underlying implementation so we can check the ref count */
    sink1Impl = impl_from_IWSDiscoveryPublisherNotify(sink1);
    sink2Impl = impl_from_IWSDiscoveryPublisherNotify(sink2);

    /* Attempt to unregister sink before registering it */
    rc = IWSDiscoveryPublisher_UnRegisterNotificationSink(publisher, sink1);
    ok(rc == E_FAIL, "IWSDiscoveryPublisher_UnRegisterNotificationSink returned unexpected result: %08lx\n", rc);

    /* Register notification sinks */
    rc = IWSDiscoveryPublisher_RegisterNotificationSink(publisher, sink1);
    ok(rc == S_OK, "IWSDiscoveryPublisher_RegisterNotificationSink failed: %08lx\n", rc);
    ok(sink1Impl->ref == 2, "Ref count for sink 1 is not as expected: %ld\n", sink1Impl->ref);

    rc = IWSDiscoveryPublisher_RegisterNotificationSink(publisher, sink2);
    ok(rc == S_OK, "IWSDiscoveryPublisher_RegisterNotificationSink failed: %08lx\n", rc);
    ok(sink2Impl->ref == 2, "Ref count for sink 2 is not as expected: %ld\n", sink2Impl->ref);

    /* Unregister the first sink */
    rc = IWSDiscoveryPublisher_UnRegisterNotificationSink(publisher, sink1);
    ok(rc == S_OK, "IWSDiscoveryPublisher_UnRegisterNotificationSink failed: %08lx\n", rc);
    ok(sink1Impl->ref == 1, "Ref count for sink 1 is not as expected: %ld\n", sink1Impl->ref);

    /* Set up network listener */
    publisherIdW = utf8_to_wide(publisherId);
    if (publisherIdW == NULL) goto after_publish_test;

    sequenceIdW = utf8_to_wide(sequenceId);
    if (sequenceIdW == NULL) goto after_publish_test;

    msgStorage = calloc(1, sizeof(*msgStorage));
    if (msgStorage == NULL) goto after_publish_test;

    msgStorage->running = TRUE;
    InitializeCriticalSection(&msgStorage->criticalSection);

    ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
    ok(ret == 0, "WSAStartup failed (ret = %d)\n", ret);

    ret = start_listening_on_all_addresses(msgStorage, AF_INET);
    ok(ret == TRUE, "Unable to listen on IPv4 addresses (ret == %d)\n", ret);

    /* Create "any" elements for header */
    ns.Uri = uri;
    ns.PreferredPrefix = L"wine";

    header_any_name.LocalName = (WCHAR *) L"Beer";
    header_any_name.Space = &ns;

    rc = WSDXMLBuildAnyForSingleElement(&header_any_name, L"PublishTest", &header_any_element);
    ok(rc == S_OK, "WSDXMLBuildAnyForSingleElement failed with %08lx\n", rc);

    rc = WSDXMLBuildAnyForSingleElement(&header_any_name, L"BodyTest", &body_any_element);
    ok(rc == S_OK, "WSDXMLBuildAnyForSingleElement failed with %08lx\n", rc);

    rc = WSDXMLBuildAnyForSingleElement(&header_any_name, L"EndPTest", &endpoint_any_element);
    ok(rc == S_OK, "WSDXMLBuildAnyForSingleElement failed with %08lx\n", rc);

    rc = WSDXMLBuildAnyForSingleElement(&header_any_name, L"RefPTest", &ref_param_any_element);
    ok(rc == S_OK, "WSDXMLBuildAnyForSingleElement failed with %08lx\n", rc);

    /* Create types list */
    ns2.Uri = uri_more_tests;
    ns2.PreferredPrefix = prefix_grog;

    another_name.LocalName = (WCHAR *) name_cider;
    another_name.Space = &ns2;

    types_list.Next = malloc(sizeof(WSD_NAME_LIST));
    types_list.Element = &another_name;

    types_list.Next->Next = NULL;
    types_list.Next->Element = &header_any_name;

    /* Create scopes and xaddrs lists */
    scopes_list.Next = malloc(sizeof(WSD_URI_LIST));
    scopes_list.Element = uri;

    scopes_list.Next->Next = NULL;
    scopes_list.Next->Element = uri_more_tests;

    xaddrs_list.Next = malloc(sizeof(WSD_URI_LIST));
    xaddrs_list.Element = uri_more_tests;

    xaddrs_list.Next->Next = NULL;
    xaddrs_list.Next->Element = L"http://third.url/";

    /* Publish the service */
    rc = IWSDiscoveryPublisher_PublishEx(publisher, publisherIdW, 1, 1, 1, sequenceIdW, &types_list, &scopes_list,
        &xaddrs_list, header_any_element, ref_param_any_element, NULL, endpoint_any_element, body_any_element);

    WSDFreeLinkedMemory(header_any_element);
    WSDFreeLinkedMemory(body_any_element);
    WSDFreeLinkedMemory(endpoint_any_element);
    WSDFreeLinkedMemory(ref_param_any_element);
    free(types_list.Next);
    free(scopes_list.Next);
    free(xaddrs_list.Next);

    ok(rc == S_OK, "Publish failed: %08lx\n", rc);

    /* Wait up to 2 seconds for messages to be received */
    if (WaitForMultipleObjects(msgStorage->numThreadHandles, msgStorage->threadHandles, TRUE, 2000) == WAIT_TIMEOUT)
    {
        /* Wait up to 1 more second for threads to terminate */
        msgStorage->running = FALSE;
        WaitForMultipleObjects(msgStorage->numThreadHandles, msgStorage->threadHandles, TRUE, 1000);
    }

    DeleteCriticalSection(&msgStorage->criticalSection);

    /* Verify we've received a message */
    ok(msgStorage->messageCount >= 1, "No messages received\n");

    sprintf(endpointReferenceString, "<wsa:EndpointReference><wsa:Address>%s</wsa:Address><wsa:ReferenceParameters>"
        "<wine:Beer>RefPTest</wine:Beer></wsa:ReferenceParameters><wine:Beer>EndPTest</wine:Beer>"
        "</wsa:EndpointReference>", publisherId);

    sprintf(app_sequence_string, "<wsd:AppSequence InstanceId=\"1\" SequenceId=\"%s\" MessageNumber=\"1\"></wsd:AppSequence>",
        sequenceId);

    messageOK = FALSE;

    /* Check we're received the correct message */
    for (i = 0; i < msgStorage->messageCount; i++)
    {
        msg = msgStorage->messages[i];
        messageOK = FALSE;

        hello_message_seen = (strstr(msg, "<wsa:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/Hello</wsa:Action>") != NULL);
        endpoint_reference_seen = (strstr(msg, endpointReferenceString) != NULL);
        app_sequence_seen = (strstr(msg, app_sequence_string) != NULL);
        metadata_version_seen = (strstr(msg, "<wsd:MetadataVersion>1</wsd:MetadataVersion>") != NULL);
        any_header_seen = (strstr(msg, "<wine:Beer>PublishTest</wine:Beer>") != NULL);
        wine_ns_seen = (strstr(msg, "xmlns:wine=\"http://wine.test/\"") != NULL);
        body_hello_seen = (strstr(msg, "<soap:Body><wsd:Hello") != NULL);
        any_body_seen = (strstr(msg, "<wine:Beer>BodyTest</wine:Beer>") != NULL);
        types_seen = (strstr(msg, "<wsd:Types>grog:Cider wine:Beer</wsd:Types>") != NULL);
        scopes_seen = (strstr(msg, "<wsd:Scopes>http://wine.test/ http://more.tests/</wsd:Scopes>") != NULL);
        xaddrs_seen = (strstr(msg, "<wsd:XAddrs>http://more.tests/ http://third.url/</wsd:XAddrs>") != NULL);
        xml_namespaces_seen = (strstr(msg, "xmlns:wine=\"http://wine.test/\" xmlns:grog=\"http://more.tests/\"") != NULL);
        messageOK = hello_message_seen && endpoint_reference_seen && app_sequence_seen && metadata_version_seen &&
            any_header_seen && wine_ns_seen && body_hello_seen && any_body_seen && types_seen && xml_namespaces_seen &&
            scopes_seen && xaddrs_seen;

        if (messageOK) break;
    }

    for (i = 0; i < msgStorage->messageCount; i++)
    {
        free(msgStorage->messages[i]);
    }

    free(msgStorage);

    ok(hello_message_seen == TRUE, "Hello message not received\n");
    ok(endpoint_reference_seen == TRUE, "EndpointReference not received\n");
    ok(app_sequence_seen == TRUE, "AppSequence not received\n");
    ok(metadata_version_seen == TRUE, "MetadataVersion not received\n");
    ok(messageOK == TRUE, "Hello message metadata not received\n");
    ok(any_header_seen == TRUE, "Custom header not received\n");
    ok(wine_ns_seen == TRUE, "Wine namespace not received\n");
    ok(body_hello_seen == TRUE, "Body and Hello elements not received\n");
    ok(any_body_seen == TRUE, "Custom body element not received\n");
    ok(types_seen == TRUE, "Types not received\n");
    ok(xml_namespaces_seen == TRUE, "XML namespaces not received\n");
    ok(scopes_seen == TRUE, "Scopes not received\n");
    ok(xaddrs_seen == TRUE, "XAddrs not received\n");

after_publish_test:

    free(publisherIdW);
    free(sequenceIdW);

    /* Test the receiving of a probe message */
    probe_event = CreateEventW(NULL, TRUE, FALSE, NULL);

    UuidCreate(&probe_message_id);
    UuidToStringA(&probe_message_id, &probe_uuid_str);

    ok(probe_uuid_str != NULL, "Failed to create UUID for probe message\n");

    if (probe_uuid_str != NULL)
    {
        char probe_message[sizeof(testProbeMessage) + 50];
        sprintf(probe_message, testProbeMessage, probe_uuid_str);

        ok(send_udp_multicast_of_type(probe_message, strlen(probe_message), AF_INET) == TRUE, "Sending Probe message failed\n");
        ok(WaitForSingleObject(probe_event, 10000) == WAIT_OBJECT_0, "Probe message not received\n");

        RpcStringFreeA(&probe_uuid_str);
    }

    CloseHandle(probe_event);

    ref = IWSDiscoveryPublisher_Release(publisher);
    ok(ref == 0, "IWSDiscoveryPublisher_Release() has %ld references, should have 0\n", ref);

    /* Check that the sinks have been released by the publisher */
    ok(sink1Impl->ref == 1, "Ref count for sink 1 is not as expected: %ld\n", sink1Impl->ref);
    ok(sink2Impl->ref == 1, "Ref count for sink 2 is not as expected: %ld\n", sink2Impl->ref);

    /* Release the sinks */
    IWSDiscoveryPublisherNotify_Release(sink1);
    IWSDiscoveryPublisherNotify_Release(sink2);

    WSACleanup();
}

static void UnPublish_tests(void)
{
    IWSDiscoveryPublisher *publisher = NULL;
    IWSDiscoveryPublisherNotify *sink1 = NULL;
    char endpoint_reference_string[MAX_PATH], app_sequence_string[MAX_PATH];
    LPWSTR publisherIdW = NULL, sequenceIdW = NULL;
    messageStorage *msg_storage;
    WSADATA wsa_data;
    BOOL message_ok, hello_message_seen = FALSE, endpoint_reference_seen = FALSE, app_sequence_seen = FALSE;
    BOOL wine_ns_seen = FALSE, body_hello_seen = FALSE, any_body_seen = FALSE;
    int ret, i;
    HRESULT rc;
    ULONG ref;
    char *msg;
    WSDXML_ELEMENT *body_any_element;
    WSDXML_NAME body_any_name;
    WSDXML_NAMESPACE ns;

    rc = WSDCreateDiscoveryPublisher(NULL, &publisher);
    ok(rc == S_OK, "WSDCreateDiscoveryPublisher(NULL, &publisher) failed: %08lx\n", rc);
    ok(publisher != NULL, "WSDCreateDiscoveryPublisher(NULL, &publisher) failed: publisher == NULL\n");

    rc = IWSDiscoveryPublisher_SetAddressFamily(publisher, WSDAPI_ADDRESSFAMILY_IPV4);
    ok(rc == S_OK, "IWSDiscoveryPublisher_SetAddressFamily(WSDAPI_ADDRESSFAMILY_IPV4) failed: %08lx\n", rc);

    /* Create notification sink */
    ok(create_discovery_publisher_notify(&sink1) == TRUE, "create_discovery_publisher_notify failed\n");
    rc = IWSDiscoveryPublisher_RegisterNotificationSink(publisher, sink1);
    ok(rc == S_OK, "IWSDiscoveryPublisher_RegisterNotificationSink failed: %08lx\n", rc);

    /* Set up network listener */
    publisherIdW = utf8_to_wide(publisherId);
    if (publisherIdW == NULL) goto after_unpublish_test;

    sequenceIdW = utf8_to_wide(sequenceId);
    if (sequenceIdW == NULL) goto after_unpublish_test;

    msg_storage = calloc(1, sizeof(*msg_storage));
    if (msg_storage == NULL) goto after_unpublish_test;

    msg_storage->running = TRUE;
    InitializeCriticalSection(&msg_storage->criticalSection);

    ret = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    ok(ret == 0, "WSAStartup failed (ret = %d)\n", ret);

    ret = start_listening_on_all_addresses(msg_storage, AF_INET);
    ok(ret == TRUE, "Unable to listen on IPv4 addresses (ret == %d)\n", ret);

    /* Create "any" elements for header */
    ns.Uri = L"http://wine.test/";
    ns.PreferredPrefix = L"wine";

    body_any_name.LocalName = (WCHAR *) L"Beer";
    body_any_name.Space = &ns;

    rc = WSDXMLBuildAnyForSingleElement(&body_any_name, L"BodyTest", &body_any_element);
    ok(rc == S_OK, "WSDXMLBuildAnyForSingleElement failed with %08lx\n", rc);

    /* Unpublish the service */
    rc = IWSDiscoveryPublisher_UnPublish(publisher, publisherIdW, 1, 1, sequenceIdW, body_any_element);

    WSDFreeLinkedMemory(body_any_element);

    ok(rc == S_OK, "Unpublish failed: %08lx\n", rc);

    /* Wait up to 2 seconds for messages to be received */
    if (WaitForMultipleObjects(msg_storage->numThreadHandles, msg_storage->threadHandles, TRUE, 2000) == WAIT_TIMEOUT)
    {
        /* Wait up to 1 more second for threads to terminate */
        msg_storage->running = FALSE;
        WaitForMultipleObjects(msg_storage->numThreadHandles, msg_storage->threadHandles, TRUE, 1000);
    }

    DeleteCriticalSection(&msg_storage->criticalSection);

    /* Verify we've received a message */
    ok(msg_storage->messageCount >= 1, "No messages received\n");

    sprintf(endpoint_reference_string, "<wsa:EndpointReference><wsa:Address>%s</wsa:Address></wsa:EndpointReference>",
        publisherId);
    sprintf(app_sequence_string, "<wsd:AppSequence InstanceId=\"1\" SequenceId=\"%s\" MessageNumber=\"1\"></wsd:AppSequence>",
        sequenceId);

    message_ok = FALSE;

    /* Check we're received the correct message */
    for (i = 0; i < msg_storage->messageCount; i++)
    {
        msg = msg_storage->messages[i];
        message_ok = FALSE;

        hello_message_seen = (strstr(msg, "<wsa:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/Bye</wsa:Action>") != NULL);
        endpoint_reference_seen = (strstr(msg, endpoint_reference_string) != NULL);
        app_sequence_seen = (strstr(msg, app_sequence_string) != NULL);
        wine_ns_seen = (strstr(msg, "xmlns:wine=\"http://wine.test/\"") != NULL);
        body_hello_seen = (strstr(msg, "<soap:Body><wsd:Bye") != NULL);
        any_body_seen = (strstr(msg, "<wine:Beer>BodyTest</wine:Beer>") != NULL);
        message_ok = hello_message_seen && endpoint_reference_seen && app_sequence_seen && wine_ns_seen &&
            body_hello_seen && any_body_seen;

        if (message_ok) break;
    }

    for (i = 0; i < msg_storage->messageCount; i++)
    {
        free(msg_storage->messages[i]);
    }

    free(msg_storage);

    ok(hello_message_seen == TRUE, "Bye message not received\n");
    ok(endpoint_reference_seen == TRUE, "EndpointReference not received\n");
    ok(app_sequence_seen == TRUE, "AppSequence not received\n");
    ok(message_ok == TRUE, "Bye message metadata not received\n");
    ok(wine_ns_seen == TRUE, "Wine namespace not received\n");
    ok(body_hello_seen == TRUE, "Body and Bye elements not received\n");
    ok(any_body_seen == TRUE, "Custom body element not received\n");

after_unpublish_test:

    free(publisherIdW);
    free(sequenceIdW);

    ref = IWSDiscoveryPublisher_Release(publisher);
    ok(ref == 0, "IWSDiscoveryPublisher_Release() has %ld references, should have 0\n", ref);

    /* Release the sinks */
    IWSDiscoveryPublisherNotify_Release(sink1);

    WSACleanup();
}

enum firewall_op
{
    APP_ADD,
    APP_REMOVE
};

static BOOL is_process_elevated(void)
{
    HANDLE token;
    if (OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &token ))
    {
        TOKEN_ELEVATION_TYPE type;
        DWORD size;
        BOOL ret;

        ret = GetTokenInformation( token, TokenElevationType, &type, sizeof(type), &size );
        CloseHandle( token );
        return (ret && type == TokenElevationTypeFull);
    }
    return FALSE;
}

static BOOL is_firewall_enabled(void)
{
    HRESULT hr, init;
    INetFwMgr *mgr = NULL;
    INetFwPolicy *policy = NULL;
    INetFwProfile *profile = NULL;
    VARIANT_BOOL enabled = VARIANT_FALSE;

    init = CoInitializeEx( 0, COINIT_APARTMENTTHREADED );

    hr = CoCreateInstance( &CLSID_NetFwMgr, NULL, CLSCTX_INPROC_SERVER, &IID_INetFwMgr,
                           (void **)&mgr );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwMgr_get_LocalPolicy( mgr, &policy );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwPolicy_get_CurrentProfile( policy, &profile );
    if (hr != S_OK) goto done;

    hr = INetFwProfile_get_FirewallEnabled( profile, &enabled );
    ok( hr == S_OK, "got %08lx\n", hr );

done:
    if (policy) INetFwPolicy_Release( policy );
    if (profile) INetFwProfile_Release( profile );
    if (mgr) INetFwMgr_Release( mgr );
    if (SUCCEEDED( init )) CoUninitialize();
    return (enabled == VARIANT_TRUE);
}

static HRESULT set_firewall( enum firewall_op op )
{
    HRESULT hr, init;
    INetFwMgr *mgr = NULL;
    INetFwPolicy *policy = NULL;
    INetFwProfile *profile = NULL;
    INetFwAuthorizedApplication *app = NULL;
    INetFwAuthorizedApplications *apps = NULL;
    BSTR name, image = SysAllocStringLen( NULL, MAX_PATH );

    if (!GetModuleFileNameW( NULL, image, MAX_PATH ))
    {
        SysFreeString( image );
        return E_FAIL;
    }
    init = CoInitializeEx( 0, COINIT_APARTMENTTHREADED );

    hr = CoCreateInstance( &CLSID_NetFwMgr, NULL, CLSCTX_INPROC_SERVER, &IID_INetFwMgr,
                           (void **)&mgr );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwMgr_get_LocalPolicy( mgr, &policy );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwPolicy_get_CurrentProfile( policy, &profile );
    if (hr != S_OK) goto done;

    hr = INetFwProfile_get_AuthorizedApplications( profile, &apps );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    hr = CoCreateInstance( &CLSID_NetFwAuthorizedApplication, NULL, CLSCTX_INPROC_SERVER,
                           &IID_INetFwAuthorizedApplication, (void **)&app );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwAuthorizedApplication_put_ProcessImageFileName( app, image );
    if (hr != S_OK) goto done;

    name = SysAllocString( L"wsdapi_test" );
    hr = INetFwAuthorizedApplication_put_Name( app, name );
    SysFreeString( name );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    if (op == APP_ADD)
        hr = INetFwAuthorizedApplications_Add( apps, app );
    else if (op == APP_REMOVE)
        hr = INetFwAuthorizedApplications_Remove( apps, image );
    else
        hr = E_INVALIDARG;

done:
    if (app) INetFwAuthorizedApplication_Release( app );
    if (apps) INetFwAuthorizedApplications_Release( apps );
    if (policy) INetFwPolicy_Release( policy );
    if (profile) INetFwProfile_Release( profile );
    if (mgr) INetFwMgr_Release( mgr );
    if (SUCCEEDED( init )) CoUninitialize();
    SysFreeString( image );
    return hr;
}

START_TEST(discovery)
{
    BOOL firewall_enabled = is_firewall_enabled();
    HRESULT hr;

    if (firewall_enabled)
    {
        if (!is_process_elevated())
        {
            skip("no privileges, skipping tests to avoid firewall dialog\n");
            return;
        }
        if ((hr = set_firewall(APP_ADD)) != S_OK)
        {
            skip("can't authorize app in firewall %08lx\n", hr);
            return;
        }
    }

    CoInitialize(NULL);

    CreateDiscoveryPublisher_tests();
    CreateDiscoveryPublisher_XMLContext_tests();
    Publish_tests();
    UnPublish_tests();

    CoUninitialize();
    if (firewall_enabled) set_firewall(APP_REMOVE);
}
