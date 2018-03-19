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
#include "webservices.h"

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

static const WCHAR addressingNsUri[] = {
    'h','t','t','p',':','/','/',
    's','c','h','e','m','a','s','.','x','m','l','s','o','a','p','.','o','r','g','/',
    'w','s','/','2','0','0','4','/','0','8','/','a','d','d','r','e','s','s','i','n','g', 0 };

static const WCHAR discoveryNsUri[] = {
    'h','t','t','p',':','/','/',
    's','c','h','e','m','a','s','.','x','m','l','s','o','a','p','.','o','r','g','/',
    'w','s','/','2','0','0','5','/','0','4','/','d','i','s','c','o','v','e','r','y', 0 };

static const WCHAR envelopeNsUri[] = {
    'h','t','t','p',':','/','/',
    'w','w','w','.','w','3','.','o','r','g','/',
    '2','0','0','3','/','0','5','/','s','o','a','p','-','e','n','v','e','l','o','p','e', 0 };

static const WCHAR addressingPrefix[] = { 'w','s','a', 0 };
static const WCHAR discoveryPrefix[] = { 'w','s','d', 0 };
static const WCHAR envelopePrefix[] = { 's','o','a','p', 0 };

static char *wide_to_utf8(LPCWSTR wide_string, int *length)
{
    char *new_string = NULL;

    if (wide_string == NULL)
        return NULL;

    *length = WideCharToMultiByte(CP_UTF8, 0, wide_string, -1, NULL, 0, NULL, NULL);

    if (*length < 0)
        return NULL;

    new_string = heap_alloc(*length);
    WideCharToMultiByte(CP_UTF8, 0, wide_string, -1, new_string, *length, NULL, NULL);

    return new_string;
}

static WS_XML_STRING *populate_xml_string(LPCWSTR str)
{
    WS_XML_STRING *xml = heap_alloc_zero(sizeof(WS_XML_STRING));
    int utf8Length;

    if (xml == NULL)
        return NULL;

    xml->bytes = (BYTE *)wide_to_utf8(str, &utf8Length);

    if (xml->bytes == NULL)
    {
        heap_free(xml);
        return NULL;
    }

    xml->dictionary = NULL;
    xml->id = 0;
    xml->length = (xml->bytes == NULL) ? 0 : (utf8Length - 1);

    return xml;
}

static inline void free_xml_string(WS_XML_STRING *value)
{
    if (value == NULL)
        return;

    if (value->bytes != NULL)
        heap_free(value->bytes);

    heap_free(value);
}

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

static WSDXML_ELEMENT *create_soap_header_xml_elements(IWSDXMLContext *xml_context, WSD_SOAP_HEADER *header)
{
    /* TODO: Implement header generation */
    return NULL;
}

static HRESULT create_soap_envelope(IWSDXMLContext *xml_context, WSD_SOAP_HEADER *header, WSDXML_ELEMENT *body_element,
    WS_HEAP **heap, char **output_xml, ULONG *xml_length, struct list *discovered_namespaces)
{
    WS_XML_STRING *actual_envelope_prefix = NULL, *envelope_uri_xmlstr = NULL;
    WSDXML_NAMESPACE *addressing_ns = NULL, *discovery_ns = NULL, *envelope_ns = NULL;
    WSDXML_ELEMENT *header_element = NULL;
    WS_XML_BUFFER *buffer = NULL;
    WS_XML_WRITER *writer = NULL;
    WS_XML_STRING envelope;
    HRESULT ret = E_OUTOFMEMORY;
    static BYTE envelopeString[] = "Envelope";

    /* Create the necessary XML prefixes */
    if (FAILED(IWSDXMLContext_AddNamespace(xml_context, addressingNsUri, addressingPrefix, &addressing_ns))) goto cleanup;

    if (FAILED(IWSDXMLContext_AddNamespace(xml_context, discoveryNsUri, discoveryPrefix, &discovery_ns))) goto cleanup;

    if (FAILED(IWSDXMLContext_AddNamespace(xml_context, envelopeNsUri, envelopePrefix, &envelope_ns))) goto cleanup;

    envelope.bytes = envelopeString;
    envelope.length = sizeof(envelopeString) - 1;
    envelope.dictionary = NULL;
    envelope.id = 0;

    actual_envelope_prefix = populate_xml_string(envelope_ns->PreferredPrefix);
    envelope_uri_xmlstr = populate_xml_string(envelope_ns->Uri);

    if ((actual_envelope_prefix == NULL) || (envelope_uri_xmlstr == NULL)) goto cleanup;

    /* Now try to create the appropriate WebServices buffers, etc */
    ret = WsCreateHeap(16384, 4096, NULL, 0, heap, NULL);
    if (FAILED(ret)) goto cleanup;

    ret = WsCreateXmlBuffer(*heap, NULL, 0, &buffer, NULL);
    if (FAILED(ret)) goto cleanup;

    ret = WsCreateWriter(NULL, 0, &writer, NULL);
    if (FAILED(ret)) goto cleanup;

    ret = WsSetOutputToBuffer(writer, buffer, NULL, 0, NULL);
    if (FAILED(ret)) goto cleanup;

    /* Create the header XML elements */
    header_element = create_soap_header_xml_elements(xml_context, header);
    if (header_element == NULL) goto cleanup;

    /* <s:Envelope> */
    ret = WsWriteStartElement(writer, actual_envelope_prefix, &envelope, envelope_uri_xmlstr, NULL);
    if (FAILED(ret)) goto cleanup;

    /* TODO: Write the header */

    ret = WsWriteEndElement(writer, NULL);
    if (FAILED(ret)) goto cleanup;

    /* </s:Envelope> */

    /* Generate the bytes of the document */
    ret = WsWriteXmlBufferToBytes(writer, buffer, NULL, NULL, 0, *heap, (void**)output_xml, xml_length, NULL);
    if (FAILED(ret)) goto cleanup;

cleanup:
    WSDFreeLinkedMemory(addressing_ns);
    WSDFreeLinkedMemory(discovery_ns);
    WSDFreeLinkedMemory(envelope_ns);

    WSDXMLCleanupElement(header_element);

    free_xml_string(actual_envelope_prefix);
    free_xml_string(envelope_uri_xmlstr);

    if (writer != NULL)
        WsFreeWriter(writer);

    /* Don't free the heap unless the operation has failed */
    if ((FAILED(ret)) && (*heap != NULL))
    {
        WsFreeHeap(*heap);
        *heap = NULL;
    }

    return ret;
}

static HRESULT write_and_send_message(IWSDiscoveryPublisherImpl *impl, WSD_SOAP_HEADER *header, WSDXML_ELEMENT *body_element,
    struct list *discovered_namespaces, IWSDUdpAddress *remote_address, int max_initial_delay)
{
    static const char xml_header[] = "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
    ULONG xml_length = 0, xml_header_len = sizeof(xml_header) - 1;
    WS_HEAP *heap = NULL;
    char *xml = NULL;
    char *full_xml;
    HRESULT ret;

    ret = create_soap_envelope(impl->xmlContext, header, NULL, &heap, &xml, &xml_length, NULL);
    if (ret != S_OK) return ret;

    /* Prefix the XML header */
    full_xml = heap_alloc(xml_length + xml_header_len + 1);

    if (full_xml == NULL)
    {
        WsFreeHeap(heap);
        return E_OUTOFMEMORY;
    }

    memcpy(full_xml, xml_header, xml_header_len);
    memcpy(full_xml + xml_header_len, xml, xml_length);
    full_xml[xml_length + xml_header_len] = 0;

    if (remote_address == NULL)
    {
        /* Send the message via UDP multicast */
        ret = send_udp_multicast(impl, full_xml, xml_length + xml_header_len + 1, max_initial_delay) ? S_OK : E_FAIL;
    }
    else
    {
        /* TODO: Send the message via UDP unicast */
        FIXME("TODO: Send the message via UDP unicast\n");
    }

    heap_free(full_xml);
    WsFreeHeap(heap);

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
