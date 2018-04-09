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
#include "wine/heap.h"
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
    newString = heap_alloc_zero(memLength);

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

    buffer = heap_alloc(RECEIVE_BUFFER_SIZE);

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
                msgStorage->messages[msgStorage->messageCount] = heap_alloc(bytesReceived);

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

    heap_free(buffer);
    heap_free(parameter);

    return 0;
}

static void start_listening(messageStorage *msgStorage, const char *multicastAddress, const char *bindAddress)
{
    struct addrinfo *multicastAddr = NULL, *bindAddr = NULL, *interfaceAddr = NULL;
    listenerThreadParams *parameter = NULL;
    const DWORD receiveTimeout = 5000;
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

    /* Set a 5-second receive timeout */
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&receiveTimeout, sizeof(receiveTimeout)) == SOCKET_ERROR) goto cleanup;

    /* Allocate memory for thread parameters */
    parameter = heap_alloc(sizeof(listenerThreadParams));

    parameter->msgStorage = msgStorage;
    parameter->listeningSocket = s;

    hThread = CreateThread(NULL, 0, listening_thread, parameter, 0, NULL);
    if (hThread == NULL) goto cleanup;

    msgStorage->threadHandles[msgStorage->numThreadHandles] = hThread;
    msgStorage->numThreadHandles++;

    goto cleanup_addresses;

cleanup:
    closesocket(s);
    heap_free(parameter);

cleanup_addresses:
    freeaddrinfo(multicastAddr);
    freeaddrinfo(bindAddr);
    freeaddrinfo(interfaceAddr);
}

static BOOL start_listening_on_all_addresses(messageStorage *msgStorage, ULONG family)
{
    IP_ADAPTER_ADDRESSES *adapterAddresses = NULL, *adapterAddress;
    ULONG bufferSize = 0;
    LPSOCKADDR sockaddr;
    DWORD addressLength;
    char address[64];
    BOOL ret = FALSE;
    ULONG retVal;

    retVal = GetAdaptersAddresses(family, 0, NULL, NULL, &bufferSize); /* family should be AF_INET or AF_INET6 */
    if (retVal != ERROR_BUFFER_OVERFLOW) goto cleanup;

    /* Get size of buffer for adapters */
    adapterAddresses = (IP_ADAPTER_ADDRESSES *)heap_alloc(bufferSize);
    if (adapterAddresses == NULL) goto cleanup;

    /* Get list of adapters */
    retVal = GetAdaptersAddresses(family, 0, NULL, adapterAddresses, &bufferSize);
    if (retVal != ERROR_SUCCESS) goto cleanup;

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
    heap_free(adapterAddresses);
    return ret;
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

    trace("IWSDiscoveryPublisherNotifyImpl_AddRef called (%p, ref = %d)\n", This, ref);
    return ref;
}

static ULONG WINAPI IWSDiscoveryPublisherNotifyImpl_Release(IWSDiscoveryPublisherNotify *iface)
{
    IWSDiscoveryPublisherNotifyImpl *This = impl_from_IWSDiscoveryPublisherNotify(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    trace("IWSDiscoveryPublisherNotifyImpl_Release called (%p, ref = %d)\n", This, ref);

    if (ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI IWSDiscoveryPublisherNotifyImpl_ProbeHandler(IWSDiscoveryPublisherNotify *This, const WSD_SOAP_MESSAGE *pSoap, IWSDMessageParameters *pMessageParameters)
{
    trace("IWSDiscoveryPublisherNotifyImpl_ProbeHandler called (%p, %p, %p)\n", This, pSoap, pMessageParameters);
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

    obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*obj));

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
    ok((rc == E_POINTER) || (rc == E_INVALIDARG), "WSDCreateDiscoveryPublisher(NULL, NULL) failed: %08x\n", rc);

    rc = WSDCreateDiscoveryPublisher(NULL, &publisher);
    ok(rc == S_OK, "WSDCreateDiscoveryPublisher(NULL, &publisher) failed: %08x\n", rc);
    ok(publisher != NULL, "WSDCreateDiscoveryPublisher(NULL, &publisher) failed: publisher == NULL\n");

    /* Try to query for objects */
    rc = IWSDiscoveryPublisher_QueryInterface(publisher, &IID_IUnknown, (LPVOID*)&unknown);
    ok(rc == S_OK,"IWSDiscoveryPublisher_QueryInterface(IID_IUnknown) failed: %08x\n", rc);

    if (rc == S_OK)
        IUnknown_Release(unknown);

    rc = IWSDiscoveryPublisher_QueryInterface(publisher, &IID_IWSDiscoveryPublisher, (LPVOID*)&publisher2);
    ok(rc == S_OK,"IWSDiscoveryPublisher_QueryInterface(IID_IWSDiscoveryPublisher) failed: %08x\n", rc);

    if (rc == S_OK)
        IWSDiscoveryPublisher_Release(publisher2);

    ref = IWSDiscoveryPublisher_Release(publisher);
    ok(ref == 0, "IWSDiscoveryPublisher_Release() has %d references, should have 0\n", ref);
}

static void CreateDiscoveryPublisher_XMLContext_tests(void)
{
    IWSDiscoveryPublisher *publisher = NULL;
    IWSDXMLContext *xmlContext, *returnedContext;
    HRESULT rc;
    int ref;

    /* Test creating an XML context and supplying it to WSDCreateDiscoveryPublisher */
    rc = WSDXMLCreateContext(&xmlContext);
    ok(rc == S_OK, "WSDXMLCreateContext failed: %08x\n", rc);

    rc = WSDCreateDiscoveryPublisher(xmlContext, &publisher);
    ok(rc == S_OK, "WSDCreateDiscoveryPublisher(xmlContext, &publisher) failed: %08x\n", rc);
    ok(publisher != NULL, "WSDCreateDiscoveryPublisher(xmlContext, &publisher) failed: publisher == NULL\n");

    rc = IWSDiscoveryPublisher_GetXMLContext(publisher, NULL);
    ok(rc == E_INVALIDARG, "GetXMLContext returned unexpected value with NULL argument: %08x\n", rc);

    rc = IWSDiscoveryPublisher_GetXMLContext(publisher, &returnedContext);
    ok(rc == S_OK, "GetXMLContext failed: %08x\n", rc);

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
    ok(rc == S_OK, "WSDCreateDiscoveryPublisher(NULL, &publisher) failed: %08x\n", rc);
    ok(publisher != NULL, "WSDCreateDiscoveryPublisher(NULL, &publisher) failed: publisher == NULL\n");

    rc = IWSDiscoveryPublisher_GetXMLContext(publisher, &returnedContext);
    ok(rc == S_OK, "GetXMLContext failed: %08x\n", rc);

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
    BOOL metadata_version_seen = FALSE, any_header_seen = FALSE, wine_ns_seen = FALSE;
    int ret, i;
    HRESULT rc;
    ULONG ref;
    char *msg;
    WSDXML_ELEMENT *header_any_element;
    WSDXML_NAME header_any_name;
    WSDXML_NAMESPACE ns;
    WCHAR header_any_name_text[] = {'B','e','e','r',0};
    static const WCHAR header_any_text[] = {'P','u','b','l','i','s','h','T','e','s','t',0};
    static const WCHAR uri[] = {'h','t','t','p',':','/','/','w','i','n','e','.','t','e','s','t','/',0};
    static const WCHAR prefix[] = {'w','i','n','e',0};

    rc = WSDCreateDiscoveryPublisher(NULL, &publisher);
    ok(rc == S_OK, "WSDCreateDiscoveryPublisher(NULL, &publisher) failed: %08x\n", rc);
    ok(publisher != NULL, "WSDCreateDiscoveryPublisher(NULL, &publisher) failed: publisher == NULL\n");

    /* Test SetAddressFamily */
    rc = IWSDiscoveryPublisher_SetAddressFamily(publisher, 12345);
    ok(rc == E_INVALIDARG, "IWSDiscoveryPublisher_SetAddressFamily(12345) returned unexpected result: %08x\n", rc);

    rc = IWSDiscoveryPublisher_SetAddressFamily(publisher, WSDAPI_ADDRESSFAMILY_IPV4);
    ok(rc == S_OK, "IWSDiscoveryPublisher_SetAddressFamily(WSDAPI_ADDRESSFAMILY_IPV4) failed: %08x\n", rc);

    /* Try to update the address family after already setting it */
    rc = IWSDiscoveryPublisher_SetAddressFamily(publisher, WSDAPI_ADDRESSFAMILY_IPV6);
    ok(rc == STG_E_INVALIDFUNCTION, "IWSDiscoveryPublisher_SetAddressFamily(WSDAPI_ADDRESSFAMILY_IPV6) returned unexpected result: %08x\n", rc);

    /* Create notification sinks */
    ok(create_discovery_publisher_notify(&sink1) == TRUE, "create_discovery_publisher_notify failed\n");
    ok(create_discovery_publisher_notify(&sink2) == TRUE, "create_discovery_publisher_notify failed\n");

    /* Get underlying implementation so we can check the ref count */
    sink1Impl = impl_from_IWSDiscoveryPublisherNotify(sink1);
    sink2Impl = impl_from_IWSDiscoveryPublisherNotify(sink2);

    /* Attempt to unregister sink before registering it */
    rc = IWSDiscoveryPublisher_UnRegisterNotificationSink(publisher, sink1);
    ok(rc == E_FAIL, "IWSDiscoveryPublisher_UnRegisterNotificationSink returned unexpected result: %08x\n", rc);

    /* Register notification sinks */
    rc = IWSDiscoveryPublisher_RegisterNotificationSink(publisher, sink1);
    ok(rc == S_OK, "IWSDiscoveryPublisher_RegisterNotificationSink failed: %08x\n", rc);
    ok(sink1Impl->ref == 2, "Ref count for sink 1 is not as expected: %d\n", sink1Impl->ref);

    rc = IWSDiscoveryPublisher_RegisterNotificationSink(publisher, sink2);
    ok(rc == S_OK, "IWSDiscoveryPublisher_RegisterNotificationSink failed: %08x\n", rc);
    ok(sink2Impl->ref == 2, "Ref count for sink 2 is not as expected: %d\n", sink2Impl->ref);

    /* Unregister the first sink */
    rc = IWSDiscoveryPublisher_UnRegisterNotificationSink(publisher, sink1);
    ok(rc == S_OK, "IWSDiscoveryPublisher_UnRegisterNotificationSink failed: %08x\n", rc);
    ok(sink1Impl->ref == 1, "Ref count for sink 1 is not as expected: %d\n", sink1Impl->ref);

    /* Set up network listener */
    publisherIdW = utf8_to_wide(publisherId);
    if (publisherIdW == NULL) goto after_publish_test;

    sequenceIdW = utf8_to_wide(sequenceId);
    if (sequenceIdW == NULL) goto after_publish_test;

    msgStorage = heap_alloc_zero(sizeof(messageStorage));
    if (msgStorage == NULL) goto after_publish_test;

    msgStorage->running = TRUE;
    InitializeCriticalSection(&msgStorage->criticalSection);

    ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
    ok(ret == 0, "WSAStartup failed (ret = %d)\n", ret);

    ret = start_listening_on_all_addresses(msgStorage, AF_INET);
    ok(ret == TRUE, "Unable to listen on IPv4 addresses (ret == %d)\n", ret);

    /* Create "any" elements for header */
    ns.Uri = uri;
    ns.PreferredPrefix = prefix;

    header_any_name.LocalName = header_any_name_text;
    header_any_name.Space = &ns;

    rc = WSDXMLBuildAnyForSingleElement(&header_any_name, header_any_text, &header_any_element);
    ok(rc == S_OK, "WSDXMLBuildAnyForSingleElement failed with %08x\n", rc);

    /* Publish the service */
    rc = IWSDiscoveryPublisher_PublishEx(publisher, publisherIdW, 1, 1, 1, sequenceIdW, NULL, NULL, NULL,
        header_any_element, NULL, NULL, NULL, NULL);

    WSDFreeLinkedMemory(header_any_element);

    ok(rc == S_OK, "Publish failed: %08x\n", rc);

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

    sprintf(endpointReferenceString, "<wsa:EndpointReference><wsa:Address>%s</wsa:Address></wsa:EndpointReference>", publisherId);
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
        messageOK = hello_message_seen && endpoint_reference_seen && app_sequence_seen && metadata_version_seen &&
            any_header_seen && wine_ns_seen;

        if (messageOK) break;
    }

    for (i = 0; i < msgStorage->messageCount; i++)
    {
        heap_free(msgStorage->messages[i]);
    }

    heap_free(msgStorage);

    ok(hello_message_seen == TRUE, "Hello message not received\n");
    todo_wine ok(endpoint_reference_seen == TRUE, "EndpointReference not received\n");
    ok(app_sequence_seen == TRUE, "AppSequence not received\n");
    todo_wine ok(metadata_version_seen == TRUE, "MetadataVersion not received\n");
    todo_wine ok(messageOK == TRUE, "Hello message metadata not received\n");
    ok(any_header_seen == TRUE, "Custom header not received\n");
    ok(wine_ns_seen == TRUE, "Wine namespace not received\n");

after_publish_test:

    heap_free(publisherIdW);
    heap_free(sequenceIdW);

    ref = IWSDiscoveryPublisher_Release(publisher);
    ok(ref == 0, "IWSDiscoveryPublisher_Release() has %d references, should have 0\n", ref);

    /* Check that the sinks have been released by the publisher */
    ok(sink1Impl->ref == 1, "Ref count for sink 1 is not as expected: %d\n", sink1Impl->ref);
    ok(sink2Impl->ref == 1, "Ref count for sink 2 is not as expected: %d\n", sink2Impl->ref);

    /* Release the sinks */
    IWSDiscoveryPublisherNotify_Release(sink1);
    IWSDiscoveryPublisherNotify_Release(sink2);

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
    ok( hr == S_OK, "got %08x\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwMgr_get_LocalPolicy( mgr, &policy );
    ok( hr == S_OK, "got %08x\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwPolicy_get_CurrentProfile( policy, &profile );
    if (hr != S_OK) goto done;

    hr = INetFwProfile_get_FirewallEnabled( profile, &enabled );
    ok( hr == S_OK, "got %08x\n", hr );

done:
    if (policy) INetFwPolicy_Release( policy );
    if (profile) INetFwProfile_Release( profile );
    if (mgr) INetFwMgr_Release( mgr );
    if (SUCCEEDED( init )) CoUninitialize();
    return (enabled == VARIANT_TRUE);
}

static HRESULT set_firewall( enum firewall_op op )
{
    static const WCHAR testW[] = {'w','s','d','a','p','i','_','t','e','s','t',0};
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
    ok( hr == S_OK, "got %08x\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwMgr_get_LocalPolicy( mgr, &policy );
    ok( hr == S_OK, "got %08x\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwPolicy_get_CurrentProfile( policy, &profile );
    if (hr != S_OK) goto done;

    hr = INetFwProfile_get_AuthorizedApplications( profile, &apps );
    ok( hr == S_OK, "got %08x\n", hr );
    if (hr != S_OK) goto done;

    hr = CoCreateInstance( &CLSID_NetFwAuthorizedApplication, NULL, CLSCTX_INPROC_SERVER,
                           &IID_INetFwAuthorizedApplication, (void **)&app );
    ok( hr == S_OK, "got %08x\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwAuthorizedApplication_put_ProcessImageFileName( app, image );
    if (hr != S_OK) goto done;

    name = SysAllocString( testW );
    hr = INetFwAuthorizedApplication_put_Name( app, name );
    SysFreeString( name );
    ok( hr == S_OK, "got %08x\n", hr );
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
            skip("can't authorize app in firewall %08x\n", hr);
            return;
        }
    }

    CoInitialize(NULL);

    CreateDiscoveryPublisher_tests();
    CreateDiscoveryPublisher_XMLContext_tests();
    Publish_tests();

    CoUninitialize();
    if (firewall_enabled) set_firewall(APP_REMOVE);
}
