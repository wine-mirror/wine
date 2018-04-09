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
static const WCHAR headerString[] = { 'H','e','a','d','e','r', 0 };
static const WCHAR actionString[] = { 'A','c','t','i','o','n', 0 };
static const WCHAR messageIdString[] = { 'M','e','s','s','a','g','e','I','D', 0 };
static const WCHAR toString[] = { 'T','o', 0 };
static const WCHAR relatesToString[] = { 'R','e','l','a','t','e','s','T','o', 0 };
static const WCHAR appSequenceString[] = { 'A','p','p','S','e','q','u','e','n','c','e', 0 };
static const WCHAR instanceIdString[] = { 'I','n','s','t','a','n','c','e','I','d', 0 };
static const WCHAR messageNumberString[] = { 'M','e','s','s','a','g','e','N','u','m','b','e','r', 0 };
static const WCHAR sequenceIdString[] = { 'S','e','q','u','e','n','c','e','I','d', 0 };
static const WCHAR emptyString[] = { 0 };

struct discovered_namespace
{
    struct list entry;
    LPCWSTR prefix;
    LPCWSTR uri;
};

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

    heap_free(value->bytes);

    heap_free(value);
}

static HRESULT write_xml_attribute(WSDXML_ATTRIBUTE *attribute, WS_XML_WRITER *writer)
{
    WS_XML_STRING *local_name = NULL, *element_ns = NULL, *ns_prefix = NULL;
    WS_XML_UTF16_TEXT utf16_text;
    HRESULT ret = E_OUTOFMEMORY;
    int text_len;

    if (attribute == NULL)
        return S_OK;

    /* Start the attribute */
    local_name = populate_xml_string(attribute->Name->LocalName);
    if (local_name == NULL) goto cleanup;

    if (attribute->Name->Space == NULL)
    {
        element_ns = populate_xml_string(emptyString);
        if (element_ns == NULL) goto cleanup;

        ns_prefix = NULL;
    }
    else
    {
        element_ns = populate_xml_string(attribute->Name->Space->Uri);
        if (element_ns == NULL) goto cleanup;

        ns_prefix = populate_xml_string(attribute->Name->Space->PreferredPrefix);
        if (ns_prefix == NULL) goto cleanup;
    }

    ret = WsWriteStartAttribute(writer, ns_prefix, local_name, element_ns, FALSE, NULL);
    if (FAILED(ret)) goto cleanup;

    text_len = lstrlenW(attribute->Value);

    utf16_text.text.textType = WS_XML_TEXT_TYPE_UTF16;
    utf16_text.bytes = (BYTE *)attribute->Value;
    utf16_text.byteCount = min(WSD_MAX_TEXT_LENGTH, text_len) * sizeof(WCHAR);

    ret = WsWriteText(writer, (WS_XML_TEXT *)&utf16_text, NULL);
    if (FAILED(ret)) goto cleanup;

    ret = WsWriteEndAttribute(writer, NULL);
    if (FAILED(ret)) goto cleanup;

cleanup:
    free_xml_string(local_name);
    free_xml_string(element_ns);
    free_xml_string(ns_prefix);

    return ret;
}

static HRESULT write_xml_element(WSDXML_ELEMENT *element, WS_XML_WRITER *writer)
{
    WS_XML_STRING *local_name = NULL, *element_ns = NULL, *ns_prefix = NULL;
    WSDXML_ATTRIBUTE *current_attribute;
    WS_XML_UTF16_TEXT utf16_text;
    WSDXML_NODE *current_child;
    WSDXML_TEXT *node_as_text;
    int text_len;
    HRESULT ret = E_OUTOFMEMORY;

    if (element == NULL)
        return S_OK;

    /* Start the element */
    local_name = populate_xml_string(element->Name->LocalName);
    if (local_name == NULL) goto cleanup;

    element_ns = populate_xml_string(element->Name->Space->Uri);
    if (element_ns == NULL) goto cleanup;

    ns_prefix = populate_xml_string(element->Name->Space->PreferredPrefix);
    if (ns_prefix == NULL) goto cleanup;

    ret = WsWriteStartElement(writer, ns_prefix, local_name, element_ns, NULL);
    if (FAILED(ret)) goto cleanup;

    /* Write attributes */
    current_attribute = element->FirstAttribute;

    while (current_attribute != NULL)
    {
        ret = write_xml_attribute(current_attribute, writer);
        if (FAILED(ret)) goto cleanup;
        current_attribute = current_attribute->Next;
    }

    /* Write child elements */
    current_child = element->FirstChild;

    while (current_child != NULL)
    {
        if (current_child->Type == ElementType)
        {
            ret = write_xml_element((WSDXML_ELEMENT *)current_child, writer);
            if (FAILED(ret)) goto cleanup;
        }
        else if (current_child->Type == TextType)
        {
            node_as_text = (WSDXML_TEXT *)current_child;
            text_len = lstrlenW(node_as_text->Text);

            utf16_text.text.textType = WS_XML_TEXT_TYPE_UTF16;
            utf16_text.byteCount = min(WSD_MAX_TEXT_LENGTH, text_len) * sizeof(WCHAR);
            utf16_text.bytes = (BYTE *)node_as_text->Text;

            ret = WsWriteText(writer, (WS_XML_TEXT *)&utf16_text, NULL);
            if (FAILED(ret)) goto cleanup;
        }

        current_child = current_child->Next;
    }

    /* End the element */
    ret = WsWriteEndElement(writer, NULL);

cleanup:
    free_xml_string(local_name);
    free_xml_string(element_ns);
    free_xml_string(ns_prefix);

    return ret;
}

static HRESULT add_child_element(IWSDXMLContext *xml_context, WSDXML_ELEMENT *parent, LPCWSTR ns_uri,
    LPCWSTR name, LPCWSTR text, WSDXML_ELEMENT **out)
{
    WSDXML_ELEMENT *element_obj;
    WSDXML_NAME *name_obj;
    HRESULT ret;

    ret = IWSDXMLContext_AddNameToNamespace(xml_context, ns_uri, name, &name_obj);
    if (FAILED(ret)) return ret;

    ret = WSDXMLBuildAnyForSingleElement(name_obj, text, &element_obj);
    WSDFreeLinkedMemory(name_obj);

    if (FAILED(ret)) return ret;

    /* Add the element as a child - this will link the element's memory allocation to the parent's */
    ret = WSDXMLAddChild(parent, element_obj);

    if (FAILED(ret))
    {
        WSDFreeLinkedMemory(element_obj);
        return ret;
    }

    *out = element_obj;
    return ret;
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

#define MAX_ULONGLONG_STRING_SIZE    25

static LPWSTR ulonglong_to_string(void *parent, ULONGLONG value)
{
    WCHAR formatString[] = { '%','I','6','4','u', 0 };
    LPWSTR ret;

    ret = WSDAllocateLinkedMemory(parent, MAX_ULONGLONG_STRING_SIZE * sizeof(WCHAR));

    if (ret == NULL)
        return NULL;

    wsprintfW(ret, formatString, value);
    return ret;
}

static WSDXML_ATTRIBUTE *add_attribute(IWSDXMLContext *xml_context, WSDXML_ELEMENT *parent, LPCWSTR ns_uri, LPCWSTR name)
{
    WSDXML_ATTRIBUTE *attribute, *cur_attrib;
    WSDXML_NAME *name_obj = NULL;

    if (ns_uri == NULL)
    {
        name_obj = WSDAllocateLinkedMemory(NULL, sizeof(WSDXML_NAME));
        name_obj->LocalName = duplicate_string(name_obj, name);
        name_obj->Space = NULL;
    }
    else
    {
        if (FAILED(IWSDXMLContext_AddNameToNamespace(xml_context, ns_uri, name, &name_obj)))
            return NULL;
    }

    attribute = WSDAllocateLinkedMemory(parent, sizeof(WSDXML_ATTRIBUTE));

    if (attribute == NULL)
    {
        WSDFreeLinkedMemory(name_obj);
        return NULL;
    }

    attribute->Element = parent;
    attribute->Name = name_obj;
    attribute->Next = NULL;
    attribute->Value = NULL;

    if (name_obj != NULL)
        WSDAttachLinkedMemory(attribute, name_obj);

    if (parent->FirstAttribute == NULL)
    {
        /* Make this the first attribute of the parent */
        parent->FirstAttribute = attribute;
    }
    else
    {
        /* Find the last attribute and add this as the next one */
        cur_attrib = parent->FirstAttribute;

        while (cur_attrib->Next != NULL)
        {
            cur_attrib = cur_attrib->Next;
        }

        cur_attrib->Next = attribute;
    }

    return attribute;
}

static void remove_attribute(WSDXML_ELEMENT *parent, WSDXML_ATTRIBUTE *attribute)
{
    WSDXML_ATTRIBUTE *cur_attrib;

    /* Find the last attribute and add this as the next one */
    cur_attrib = parent->FirstAttribute;

    if (cur_attrib == attribute)
        parent->FirstAttribute = cur_attrib->Next;
    else
    {
        while (cur_attrib != NULL)
        {
            /* Is our attribute the next attribute? */
            if (cur_attrib->Next == attribute)
            {
                /* Remove it from the list */
                cur_attrib->Next = attribute->Next;
                break;
            }

            cur_attrib = cur_attrib->Next;
        }
    }

    WSDFreeLinkedMemory(attribute);
}

static HRESULT add_ulonglong_attribute(IWSDXMLContext *xml_context, WSDXML_ELEMENT *parent, LPCWSTR ns_uri, LPCWSTR name,
    ULONGLONG value)
{
    WSDXML_ATTRIBUTE *attribute = add_attribute(xml_context, parent, ns_uri, name);

    if (attribute == NULL)
        return E_FAIL;

    attribute->Value = ulonglong_to_string(attribute, value);

    if (attribute->Value == NULL)
    {
        remove_attribute(parent, attribute);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT add_string_attribute(IWSDXMLContext *xml_context, WSDXML_ELEMENT *parent, LPCWSTR ns_uri, LPCWSTR name,
    LPCWSTR value)
{
    WSDXML_ATTRIBUTE *attribute = add_attribute(xml_context, parent, ns_uri, name);

    if (attribute == NULL)
        return E_FAIL;

    attribute->Value = duplicate_string(attribute, value);

    if (attribute->Value == NULL)
    {
        remove_attribute(parent, attribute);
        return E_FAIL;
    }

    return S_OK;
}

static BOOL add_discovered_namespace(struct list *namespaces, WSDXML_NAMESPACE *discovered_ns)
{
    struct discovered_namespace *ns;

    LIST_FOR_EACH_ENTRY(ns, namespaces, struct discovered_namespace, entry)
    {
        if (lstrcmpW(ns->uri, discovered_ns->Uri) == 0)
            return TRUE; /* Already added */
    }

    ns = WSDAllocateLinkedMemory(namespaces, sizeof(struct discovered_namespace));

    if (ns == NULL)
        return FALSE;

    ns->prefix = duplicate_string(ns, discovered_ns->PreferredPrefix);
    ns->uri = duplicate_string(ns, discovered_ns->Uri);

    if ((ns->prefix == NULL) || (ns->uri == NULL))
        return FALSE;

    list_add_tail(namespaces, &ns->entry);
    return TRUE;
}

static HRESULT duplicate_element(WSDXML_ELEMENT *parent, const WSDXML_ELEMENT *node, struct list *namespaces)
{
    WSDXML_ATTRIBUTE *cur_attribute, *new_attribute, *last_attribute = NULL;
    WSDXML_ELEMENT *new_element;
    WSDXML_TEXT *text_node;
    WSDXML_NODE *cur_node;
    HRESULT ret;

    /* First record the namespace in the discovered namespaces list */
    if (!add_discovered_namespace(namespaces, node->Name->Space))
        return E_FAIL;

    ret = WSDXMLBuildAnyForSingleElement(node->Name, NULL, &new_element);
    if (FAILED(ret)) return ret;

    /* Duplicate the nodes */
    cur_node = node->FirstChild;

    while (cur_node != NULL)
    {
        if (cur_node->Type == ElementType)
        {
            ret = duplicate_element(new_element, (WSDXML_ELEMENT *)cur_node, namespaces);
            if (FAILED(ret)) goto cleanup;
        }
        else if (cur_node->Type == TextType)
        {
            text_node = WSDAllocateLinkedMemory(new_element, sizeof(WSDXML_TEXT));
            if (text_node == NULL) goto failed;

            text_node->Node.Next = NULL;
            text_node->Node.Type = TextType;
            text_node->Text = duplicate_string(text_node, ((WSDXML_TEXT *)cur_node)->Text);

            if (text_node->Text == NULL) goto failed;

            ret = WSDXMLAddChild(new_element, (WSDXML_ELEMENT *)text_node);
            if (FAILED(ret)) goto cleanup;
        }

        cur_node = cur_node->Next;
    }

    /* Duplicate the attributes */
    cur_attribute = node->FirstAttribute;

    while (cur_attribute != NULL)
    {
        if ((cur_attribute->Name->Space != NULL) && (!add_discovered_namespace(namespaces, cur_attribute->Name->Space)))
            goto failed;

        new_attribute = WSDAllocateLinkedMemory(new_element, sizeof(WSDXML_ATTRIBUTE));
        if (new_attribute == NULL) goto failed;

        new_attribute->Element = new_element;
        new_attribute->Name = duplicate_name(new_attribute, cur_attribute->Name);
        new_attribute->Value = duplicate_string(new_attribute, cur_attribute->Value);
        new_attribute->Next = NULL;

        if ((new_attribute->Name == NULL) || (new_attribute->Value == NULL)) goto failed;

        if (last_attribute == NULL)
            new_element->FirstAttribute = new_attribute;
        else
            last_attribute->Next = new_attribute;

        last_attribute = new_attribute;
        cur_attribute = cur_attribute->Next;
    }

    ret = WSDXMLAddChild(parent, new_element);
    if (FAILED(ret)) goto cleanup;

    return ret;

failed:
    ret = E_FAIL;

cleanup:
    WSDXMLCleanupElement(new_element);
    return ret;
}

static HRESULT create_soap_header_xml_elements(IWSDXMLContext *xml_context, WSD_SOAP_HEADER *header,
    struct list *discovered_namespaces, WSDXML_ELEMENT **out_element)
{
    WSDXML_ELEMENT *header_element = NULL, *app_sequence_element = NULL, *temp_element;
    WSDXML_NAME *header_name = NULL;
    HRESULT ret;

    /* <s:Header> */
    ret = IWSDXMLContext_AddNameToNamespace(xml_context, envelopeNsUri, headerString, &header_name);
    if (FAILED(ret)) goto cleanup;

    ret = WSDXMLBuildAnyForSingleElement(header_name, NULL, &header_element);
    if (FAILED(ret)) goto cleanup;

    WSDFreeLinkedMemory(header_name);

    /* <a:Action> */
    ret = add_child_element(xml_context, header_element, addressingNsUri, actionString, header->Action, &temp_element);
    if (FAILED(ret)) goto cleanup;

    /* <a:MessageId> */
    ret = add_child_element(xml_context, header_element, addressingNsUri, messageIdString, header->MessageID, &temp_element);
    if (FAILED(ret)) goto cleanup;

    /* <a:To> */
    ret = add_child_element(xml_context, header_element, addressingNsUri, toString, header->To, &temp_element);
    if (FAILED(ret)) goto cleanup;

    /* <a:RelatesTo> */
    if (header->RelatesTo.MessageID != NULL)
    {
        ret = add_child_element(xml_context, header_element, addressingNsUri, relatesToString,
            header->RelatesTo.MessageID, &temp_element);
        if (FAILED(ret)) goto cleanup;
    }

    /* <d:AppSequence> */
    ret = add_child_element(xml_context, header_element, discoveryNsUri, appSequenceString, emptyString, &app_sequence_element);
    if (FAILED(ret)) goto cleanup;

    /* InstanceId attribute */
    ret = add_ulonglong_attribute(xml_context, app_sequence_element, NULL, instanceIdString, min(UINT_MAX,
        header->AppSequence->InstanceId));
    if (FAILED(ret)) goto cleanup;

    /* SequenceID attribute */
    if (header->AppSequence->SequenceId != NULL)
    {
        ret = add_string_attribute(xml_context, app_sequence_element, NULL, sequenceIdString, header->AppSequence->SequenceId);
        if (FAILED(ret)) goto cleanup;
    }

    /* MessageNumber attribute */
    ret = add_ulonglong_attribute(xml_context, app_sequence_element, NULL, messageNumberString, min(UINT_MAX,
        header->AppSequence->MessageNumber));
    if (FAILED(ret)) goto cleanup;

    /* </d:AppSequence> */

    /* Write any headers */
    if (header->AnyHeaders != NULL)
    {
        ret = duplicate_element(header_element, header->AnyHeaders, discovered_namespaces);
        if (FAILED(ret)) goto cleanup;
    }

    /* </s:Header> */

    *out_element = header_element;
    return ret;

cleanup:
    if (header_name != NULL) WSDFreeLinkedMemory(header_name);
    WSDXMLCleanupElement(header_element);

    return ret;
}

static HRESULT create_soap_envelope(IWSDXMLContext *xml_context, WSD_SOAP_HEADER *header, WSDXML_ELEMENT *body_element,
    WS_HEAP **heap, char **output_xml, ULONG *xml_length, struct list *discovered_namespaces)
{
    WS_XML_STRING *actual_envelope_prefix = NULL, *envelope_uri_xmlstr = NULL, *tmp_prefix = NULL, *tmp_uri = NULL;
    WSDXML_NAMESPACE *addressing_ns = NULL, *discovery_ns = NULL, *envelope_ns = NULL;
    WSDXML_ELEMENT *header_element = NULL;
    struct discovered_namespace *ns;
    WS_XML_BUFFER *buffer = NULL;
    WS_XML_WRITER *writer = NULL;
    WS_XML_STRING envelope;
    HRESULT ret = E_OUTOFMEMORY;
    static BYTE envelopeString[] = "Envelope";

    /* Create the necessary XML prefixes */
    if (FAILED(IWSDXMLContext_AddNamespace(xml_context, addressingNsUri, addressingPrefix, &addressing_ns))) goto cleanup;
    if (!add_discovered_namespace(discovered_namespaces, addressing_ns)) goto cleanup;

    if (FAILED(IWSDXMLContext_AddNamespace(xml_context, discoveryNsUri, discoveryPrefix, &discovery_ns))) goto cleanup;
    if (!add_discovered_namespace(discovered_namespaces, discovery_ns)) goto cleanup;

    if (FAILED(IWSDXMLContext_AddNamespace(xml_context, envelopeNsUri, envelopePrefix, &envelope_ns))) goto cleanup;
    if (!add_discovered_namespace(discovered_namespaces, envelope_ns)) goto cleanup;

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
    ret = create_soap_header_xml_elements(xml_context, header, discovered_namespaces, &header_element);
    if (FAILED(ret)) goto cleanup;

    /* <s:Envelope> */
    ret = WsWriteStartElement(writer, actual_envelope_prefix, &envelope, envelope_uri_xmlstr, NULL);
    if (FAILED(ret)) goto cleanup;

    LIST_FOR_EACH_ENTRY(ns, discovered_namespaces, struct discovered_namespace, entry)
    {
        tmp_prefix = populate_xml_string(ns->prefix);
        tmp_uri = populate_xml_string(ns->uri);

        if ((tmp_prefix == NULL) || (tmp_uri == NULL)) goto cleanup;

        ret = WsWriteXmlnsAttribute(writer, tmp_prefix, tmp_uri, FALSE, NULL);
        if (FAILED(ret)) goto cleanup;

        free_xml_string(tmp_prefix);
        free_xml_string(tmp_uri);
    }

    tmp_prefix = NULL;
    tmp_uri = NULL;

    /* Write the header */
    ret = write_xml_element(header_element, writer);
    if (FAILED(ret)) goto cleanup;

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

    ret = create_soap_envelope(impl->xmlContext, header, NULL, &heap, &xml, &xml_length, discovered_namespaces);
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
    struct list *discoveredNamespaces = NULL;
    WSD_SOAP_HEADER soapHeader;
    WSD_APP_SEQUENCE sequence;
    WCHAR message_id[64];
    HRESULT ret = E_OUTOFMEMORY;

    sequence.InstanceId = instance_id;
    sequence.MessageNumber = msg_num;
    sequence.SequenceId = session_id;

    if (!create_guid(message_id)) goto cleanup;

    discoveredNamespaces = WSDAllocateLinkedMemory(NULL, sizeof(struct list));
    if (!discoveredNamespaces) goto cleanup;

    list_init(discoveredNamespaces);

    populate_soap_header(&soapHeader, discoveryTo, actionHello, message_id, &sequence, hdr_any);

    /* TODO: Populate message body */

    /* Write and send the message */
    ret = write_and_send_message(impl, &soapHeader, NULL, discoveredNamespaces, NULL, APP_MAX_DELAY);

cleanup:
    WSDFreeLinkedMemory(discoveredNamespaces);

    return ret;
}
