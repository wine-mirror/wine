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
#include "webservices.h"

WINE_DEFAULT_DEBUG_CHANNEL(wsdapi);

#define APP_MAX_DELAY                       500

static const WCHAR *discoveryTo = L"urn:schemas-xmlsoap-org:ws:2005:04:discovery";

static const WCHAR *actionProbe = L"http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe";

static const WCHAR *addressingNsUri = L"http://schemas.xmlsoap.org/ws/2004/08/addressing";
static const WCHAR *discoveryNsUri = L"http://schemas.xmlsoap.org/ws/2005/04/discovery";
static const WCHAR *envelopeNsUri = L"http://www.w3.org/2003/05/soap-envelope";

static const WCHAR *addressingPrefix = L"wsa";
static const WCHAR *discoveryPrefix = L"wsd";
static const WCHAR *envelopePrefix = L"soap";
static const WCHAR *headerString = L"Header";
static const WCHAR *actionString = L"Action";
static const WCHAR *messageIdString = L"MessageID";
static const WCHAR *toString = L"To";
static const WCHAR *relatesToString = L"RelatesTo";
static const WCHAR *appSequenceString = L"AppSequence";
static const WCHAR *instanceIdString = L"InstanceId";
static const WCHAR *messageNumberString = L"MessageNumber";
static const WCHAR *sequenceIdString = L"SequenceId";
static const WCHAR *bodyString = L"Body";
static const WCHAR *helloString = L"Hello";
static const WCHAR *probeString = L"Probe";
static const WCHAR *probeMatchString = L"ProbeMatch";
static const WCHAR *probeMatchesString = L"ProbeMatches";
static const WCHAR *byeString = L"Bye";
static const WCHAR *endpointReferenceString = L"EndpointReference";
static const WCHAR *addressString = L"Address";
static const WCHAR *referenceParametersString = L"ReferenceParameters";
static const WCHAR *typesString = L"Types";
static const WCHAR *scopesString = L"Scopes";
static const WCHAR *xAddrsString = L"XAddrs";
static const WCHAR *metadataVersionString = L"MetadataVersion";

struct discovered_namespace
{
    struct list entry;
    LPCWSTR prefix;
    LPCWSTR uri;
};

static LPWSTR utf8_to_wide(void *parent, const char *utf8_str, int length)
{
    int utf8_str_len = 0, chars_needed = 0, bytes_needed = 0;
    LPWSTR new_str = NULL;

    if (utf8_str == NULL) return NULL;

    utf8_str_len = (length < 0) ? lstrlenA(utf8_str) : length;
    chars_needed = MultiByteToWideChar(CP_UTF8, 0, utf8_str, utf8_str_len, NULL, 0);

    if (chars_needed <= 0) return NULL;

    bytes_needed = sizeof(WCHAR) * (chars_needed + 1);
    new_str = WSDAllocateLinkedMemory(parent, bytes_needed);

    MultiByteToWideChar(CP_UTF8, 0, utf8_str, utf8_str_len, new_str, chars_needed);
    new_str[chars_needed] = 0;

    return new_str;
}

static char *wide_to_utf8(LPCWSTR wide_string, int *length)
{
    char *new_string = NULL;

    if (wide_string == NULL)
        return NULL;

    *length = WideCharToMultiByte(CP_UTF8, 0, wide_string, -1, NULL, 0, NULL, NULL);

    if (*length < 0)
        return NULL;

    new_string = malloc(*length);
    WideCharToMultiByte(CP_UTF8, 0, wide_string, -1, new_string, *length, NULL, NULL);

    return new_string;
}

static WS_XML_STRING *populate_xml_string(LPCWSTR str)
{
    WS_XML_STRING *xml = calloc(1, sizeof(*xml));
    int utf8Length;

    if (xml == NULL)
        return NULL;

    xml->bytes = (BYTE *)wide_to_utf8(str, &utf8Length);

    if (xml->bytes == NULL)
    {
        free(xml);
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

    free(value->bytes);

    free(value);
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
        element_ns = populate_xml_string(L"");
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

    if (out != NULL) *out = element_obj;
    return ret;
}

HRESULT register_namespaces(IWSDXMLContext *xml_context)
{
    HRESULT ret;

    ret = IWSDXMLContext_AddNamespace(xml_context, addressingNsUri, addressingPrefix, NULL);
    if (FAILED(ret)) return ret;

    ret = IWSDXMLContext_AddNamespace(xml_context, discoveryNsUri, discoveryPrefix, NULL);
    if (FAILED(ret)) return ret;

    return IWSDXMLContext_AddNamespace(xml_context, envelopeNsUri, envelopePrefix, NULL);
}

static BOOL create_guid(LPWSTR buffer)
{
    WCHAR* uuidString = NULL;
    UUID uuid;

    if (UuidCreate(&uuid) != RPC_S_OK)
        return FALSE;

    UuidToStringW(&uuid, (RPC_WSTR*)&uuidString);

    if (uuidString == NULL)
        return FALSE;

    wsprintfW(buffer, L"urn:uuid:%s", uuidString);
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
    LPWSTR ret;

    ret = WSDAllocateLinkedMemory(parent, MAX_ULONGLONG_STRING_SIZE * sizeof(WCHAR));

    if (ret == NULL)
        return NULL;

    wsprintfW(ret, L"%I64u", value);
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

static HRESULT build_types_list(LPWSTR buffer, size_t buffer_size, const WSD_NAME_LIST *list, struct list *namespaces)
{
    LPWSTR current_buf_pos = buffer;
    size_t memory_needed = 0;
    const WSD_NAME_LIST *cur = list;

    do
    {
        /* Calculate space needed, including NULL character, colon and potential trailing space */
        memory_needed = sizeof(WCHAR) * (lstrlenW(cur->Element->LocalName) +
            lstrlenW(cur->Element->Space->PreferredPrefix) + 3);

        if (current_buf_pos + memory_needed > buffer + buffer_size)
            return E_INVALIDARG;

        if (cur != list)
            *current_buf_pos++ = ' ';

        current_buf_pos += wsprintfW(current_buf_pos, L"%s:%s", cur->Element->Space->PreferredPrefix,
            cur->Element->LocalName);

        /* Record the namespace in the discovered namespaces list */
        if (!add_discovered_namespace(namespaces, cur->Element->Space))
            return E_FAIL;

        cur = cur->Next;
    } while (cur != NULL);

    return S_OK;
}

static HRESULT build_uri_list(LPWSTR buffer, size_t buffer_size, const WSD_URI_LIST *list)
{
    size_t memory_needed = 0, string_len = 0;
    const WSD_URI_LIST *cur = list;
    LPWSTR cur_buf_pos = buffer;

    do
    {
        /* Calculate space needed, including trailing space */
        string_len = lstrlenW(cur->Element);
        memory_needed = (string_len + 1) * sizeof(WCHAR);

        if (cur_buf_pos + memory_needed > buffer + buffer_size)
            return E_INVALIDARG;

        if (cur != list)
            *cur_buf_pos++ = ' ';

        memcpy(cur_buf_pos, cur->Element, memory_needed);
        cur_buf_pos += string_len;

        cur = cur->Next;
    } while (cur != NULL);

    return S_OK;
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

            text_node->Node.Parent = NULL;
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
    ret = add_child_element(xml_context, header_element, discoveryNsUri, appSequenceString, L"", &app_sequence_element);
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

    /* Write the body */
    ret = write_xml_element(body_element, writer);
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

    ret = create_soap_envelope(impl->xmlContext, header, body_element, &heap, &xml, &xml_length, discovered_namespaces);
    if (ret != S_OK) return ret;

    /* Prefix the XML header */
    full_xml = malloc(xml_length + xml_header_len + 1);

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
        ret = send_udp_multicast(impl, full_xml, xml_length + xml_header_len, max_initial_delay) ? S_OK : E_FAIL;
    }
    else
    {
        /* Send the message via UDP unicast */
        ret = send_udp_unicast(full_xml, xml_length + xml_header_len, remote_address, max_initial_delay);
    }

    free(full_xml);
    WsFreeHeap(heap);

    return ret;
}

HRESULT send_hello_message(IWSDiscoveryPublisherImpl *impl, LPCWSTR id, ULONGLONG metadata_ver, ULONGLONG instance_id,
    ULONGLONG msg_num, LPCWSTR session_id, const WSD_NAME_LIST *types_list, const WSD_URI_LIST *scopes_list,
    const WSD_URI_LIST *xaddrs_list, const WSDXML_ELEMENT *hdr_any, const WSDXML_ELEMENT *ref_param_any,
    const WSDXML_ELEMENT *endpoint_ref_any, const WSDXML_ELEMENT *any)
{
    WSDXML_ELEMENT *body_element = NULL, *hello_element, *endpoint_reference_element, *ref_params_element;
    struct list *discoveredNamespaces = NULL;
    WSDXML_NAME *body_name = NULL;
    WSD_SOAP_HEADER soapHeader;
    WSD_APP_SEQUENCE sequence;
    WCHAR message_id[64];
    HRESULT ret = E_OUTOFMEMORY;
    LPWSTR buffer;

    sequence.InstanceId = instance_id;
    sequence.MessageNumber = msg_num;
    sequence.SequenceId = session_id;

    if (!create_guid(message_id)) goto failed;

    discoveredNamespaces = WSDAllocateLinkedMemory(NULL, sizeof(struct list));
    if (!discoveredNamespaces) goto failed;

    list_init(discoveredNamespaces);

    populate_soap_header(&soapHeader, discoveryTo, L"http://schemas.xmlsoap.org/ws/2005/04/discovery/Hello", message_id, &sequence, hdr_any);

    ret = IWSDXMLContext_AddNameToNamespace(impl->xmlContext, envelopeNsUri, bodyString, &body_name);
    if (FAILED(ret)) goto cleanup;

    /* <soap:Body>, <wsd:Hello> */
    ret = WSDXMLBuildAnyForSingleElement(body_name, NULL, &body_element);
    if (FAILED(ret)) goto cleanup;

    ret = add_child_element(impl->xmlContext, body_element, discoveryNsUri, helloString, NULL, &hello_element);
    if (FAILED(ret)) goto cleanup;

    /* <wsa:EndpointReference>, <wsa:Address> */
    ret = add_child_element(impl->xmlContext, hello_element, addressingNsUri, endpointReferenceString, NULL,
        &endpoint_reference_element);
    if (FAILED(ret)) goto cleanup;

    ret = add_child_element(impl->xmlContext, endpoint_reference_element, addressingNsUri, addressString, id, NULL);
    if (FAILED(ret)) goto cleanup;

    /* Write any reference parameters */
    if (ref_param_any != NULL)
    {
        ret = add_child_element(impl->xmlContext, endpoint_reference_element, addressingNsUri, referenceParametersString,
            NULL, &ref_params_element);
        if (FAILED(ret)) goto cleanup;

        ret = duplicate_element(ref_params_element, ref_param_any, discoveredNamespaces);
        if (FAILED(ret)) goto cleanup;
    }

    /* Write any endpoint reference headers */
    if (endpoint_ref_any != NULL)
    {
        ret = duplicate_element(endpoint_reference_element, endpoint_ref_any, discoveredNamespaces);
        if (FAILED(ret)) goto cleanup;
    }

    /* <wsd:Types> */
    if (types_list != NULL)
    {
        buffer = WSDAllocateLinkedMemory(hello_element, WSD_MAX_TEXT_LENGTH * sizeof(WCHAR));
        if (buffer == NULL) goto failed;

        ret = build_types_list(buffer, WSD_MAX_TEXT_LENGTH * sizeof(WCHAR), types_list, discoveredNamespaces);
        if (FAILED(ret)) goto cleanup;

        ret = add_child_element(impl->xmlContext, hello_element, discoveryNsUri, typesString, buffer, NULL);
        if (FAILED(ret)) goto cleanup;
    }

    /* <wsd:Scopes> */
    if (scopes_list != NULL)
    {
        buffer = WSDAllocateLinkedMemory(hello_element, WSD_MAX_TEXT_LENGTH * sizeof(WCHAR));
        if (buffer == NULL) goto failed;

        ret = build_uri_list(buffer, WSD_MAX_TEXT_LENGTH * sizeof(WCHAR), scopes_list);
        if (FAILED(ret)) goto cleanup;

        ret = add_child_element(impl->xmlContext, hello_element, discoveryNsUri, scopesString, buffer, NULL);
        if (FAILED(ret)) goto cleanup;
    }

    /* <wsd:XAddrs> */
    if (xaddrs_list != NULL)
    {
        buffer = WSDAllocateLinkedMemory(hello_element, WSD_MAX_TEXT_LENGTH * sizeof(WCHAR));
        if (buffer == NULL) goto failed;

        ret = build_uri_list(buffer, WSD_MAX_TEXT_LENGTH * sizeof(WCHAR), xaddrs_list);
        if (FAILED(ret)) goto cleanup;

        ret = add_child_element(impl->xmlContext, hello_element, discoveryNsUri, xAddrsString, buffer, NULL);
        if (FAILED(ret)) goto cleanup;
    }

    /* <wsd:MetadataVersion> */
    ret = add_child_element(impl->xmlContext, hello_element, discoveryNsUri, metadataVersionString,
        ulonglong_to_string(hello_element, min(UINT_MAX, metadata_ver)), NULL);

    if (FAILED(ret)) goto cleanup;

    /* Write any body elements */
    if (any != NULL)
    {
        ret = duplicate_element(hello_element, any, discoveredNamespaces);
        if (FAILED(ret)) goto cleanup;
    }

    /* Write and send the message */
    ret = write_and_send_message(impl, &soapHeader, body_element, discoveredNamespaces, NULL, APP_MAX_DELAY);
    goto cleanup;

failed:
    ret = E_OUTOFMEMORY;

cleanup:
    WSDFreeLinkedMemory(body_name);
    WSDFreeLinkedMemory(body_element);
    WSDFreeLinkedMemory(discoveredNamespaces);

    return ret;
}

HRESULT send_bye_message(IWSDiscoveryPublisherImpl *impl, LPCWSTR id, ULONGLONG instance_id, ULONGLONG msg_num,
    LPCWSTR session_id, const WSDXML_ELEMENT *any)
{
    WSDXML_ELEMENT *body_element = NULL, *bye_element, *endpoint_reference_element;
    struct list *discovered_namespaces = NULL;
    WSDXML_NAME *body_name = NULL;
    WSD_SOAP_HEADER soap_header;
    WSD_APP_SEQUENCE sequence;
    WCHAR message_id[64];
    HRESULT ret = E_OUTOFMEMORY;

    sequence.InstanceId = instance_id;
    sequence.MessageNumber = msg_num;
    sequence.SequenceId = session_id;

    if (!create_guid(message_id)) goto failed;

    discovered_namespaces = WSDAllocateLinkedMemory(NULL, sizeof(struct list));
    if (!discovered_namespaces) goto failed;

    list_init(discovered_namespaces);

    populate_soap_header(&soap_header, discoveryTo, L"http://schemas.xmlsoap.org/ws/2005/04/discovery/Bye", message_id, &sequence, NULL);

    ret = IWSDXMLContext_AddNameToNamespace(impl->xmlContext, envelopeNsUri, bodyString, &body_name);
    if (FAILED(ret)) goto cleanup;

    /* <soap:Body>, <wsd:Bye> */
    ret = WSDXMLBuildAnyForSingleElement(body_name, NULL, &body_element);
    if (FAILED(ret)) goto cleanup;

    ret = add_child_element(impl->xmlContext, body_element, discoveryNsUri, byeString, NULL, &bye_element);
    if (FAILED(ret)) goto cleanup;

    /* <wsa:EndpointReference>, <wsa:Address> */
    ret = add_child_element(impl->xmlContext, bye_element, addressingNsUri, endpointReferenceString, NULL,
        &endpoint_reference_element);
    if (FAILED(ret)) goto cleanup;

    ret = add_child_element(impl->xmlContext, endpoint_reference_element, addressingNsUri, addressString, id, NULL);
    if (FAILED(ret)) goto cleanup;

    /* Write any body elements */
    if (any != NULL)
    {
        ret = duplicate_element(bye_element, any, discovered_namespaces);
        if (FAILED(ret)) goto cleanup;
    }

    /* Write and send the message */
    ret = write_and_send_message(impl, &soap_header, body_element, discovered_namespaces, NULL, 0);
    goto cleanup;

failed:
    ret = E_OUTOFMEMORY;

cleanup:
    WSDFreeLinkedMemory(body_name);
    WSDFreeLinkedMemory(body_element);
    WSDFreeLinkedMemory(discovered_namespaces);

    return ret;
}

HRESULT send_probe_matches_message(IWSDiscoveryPublisherImpl *impl, const WSD_SOAP_MESSAGE *probe_msg,
    IWSDMessageParameters *message_params, LPCWSTR id, ULONGLONG metadata_ver, ULONGLONG instance_id,
    ULONGLONG msg_num, LPCWSTR session_id, const WSD_NAME_LIST *types_list, const WSD_URI_LIST *scopes_list,
    const WSD_URI_LIST *xaddrs_list, const WSDXML_ELEMENT *header_any, const WSDXML_ELEMENT *ref_param_any,
    const WSDXML_ELEMENT *endpoint_ref_any, const WSDXML_ELEMENT *any)
{
    WSDXML_ELEMENT *body_element = NULL, *probe_matches_element, *probe_match_element, *endpoint_ref_element;
    WSDXML_ELEMENT *ref_params_element = NULL;
    struct list *discovered_namespaces = NULL;
    IWSDUdpAddress *remote_udp_addr = NULL;
    IWSDAddress *remote_addr = NULL;
    WSDXML_NAME *body_name = NULL;
    WSD_SOAP_HEADER soap_header;
    WSD_APP_SEQUENCE sequence;
    WCHAR msg_id[64];
    LPWSTR buffer;
    HRESULT ret;

    ret = IWSDMessageParameters_GetRemoteAddress(message_params, &remote_addr);

    if (FAILED(ret))
    {
        WARN("Unable to retrieve remote address from IWSDMessageParameters\n");
        return ret;
    }

    ret = IWSDAddress_QueryInterface(remote_addr, &IID_IWSDUdpAddress, (LPVOID *) &remote_udp_addr);

    if (FAILED(ret))
    {
        WARN("Remote address is not a UDP address\n");
        goto cleanup;
    }

    sequence.InstanceId = instance_id;
    sequence.MessageNumber = msg_num;
    sequence.SequenceId = session_id;

    if (!create_guid(msg_id)) goto failed;

    discovered_namespaces = WSDAllocateLinkedMemory(NULL, sizeof(struct list));
    if (!discovered_namespaces) goto failed;

    list_init(discovered_namespaces);

    populate_soap_header(&soap_header, L"http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous",
        L"http://schemas.xmlsoap.org/ws/2005/04/discovery/ProbeMatches", msg_id, &sequence, header_any);
    soap_header.RelatesTo.MessageID = probe_msg->Header.MessageID;

    ret = IWSDXMLContext_AddNameToNamespace(impl->xmlContext, envelopeNsUri, bodyString, &body_name);
    if (FAILED(ret)) goto cleanup;

    /* <soap:Body>, <wsd:ProbeMatches> */
    ret = WSDXMLBuildAnyForSingleElement(body_name, NULL, &body_element);
    if (FAILED(ret)) goto cleanup;

    ret = add_child_element(impl->xmlContext, body_element, discoveryNsUri, probeMatchesString, NULL,
        &probe_matches_element);
    if (FAILED(ret)) goto cleanup;

    /* <wsd:ProbeMatch> */
    ret = add_child_element(impl->xmlContext, probe_matches_element, discoveryNsUri, probeMatchString, NULL,
        &probe_match_element);
    if (FAILED(ret)) goto cleanup;

    /* <wsa:EndpointReference>, <wsa:Address> */
    ret = add_child_element(impl->xmlContext, probe_match_element, addressingNsUri, endpointReferenceString, NULL,
        &endpoint_ref_element);
    if (FAILED(ret)) goto cleanup;

    ret = add_child_element(impl->xmlContext, endpoint_ref_element, addressingNsUri, addressString, id, NULL);
    if (FAILED(ret)) goto cleanup;

    /* Write any reference parameters */
    if (ref_param_any != NULL)
    {
        ret = add_child_element(impl->xmlContext, endpoint_ref_element, addressingNsUri, referenceParametersString,
            NULL, &ref_params_element);
        if (FAILED(ret)) goto cleanup;

        ret = duplicate_element(ref_params_element, ref_param_any, discovered_namespaces);
        if (FAILED(ret)) goto cleanup;
    }

    /* Write any endpoint reference headers */
    if (endpoint_ref_any != NULL)
    {
        ret = duplicate_element(endpoint_ref_element, endpoint_ref_any, discovered_namespaces);
        if (FAILED(ret)) goto cleanup;
    }

    /* <wsd:Types> */
    if (types_list != NULL)
    {
        buffer = WSDAllocateLinkedMemory(probe_match_element, WSD_MAX_TEXT_LENGTH * sizeof(WCHAR));
        if (buffer == NULL) goto failed;

        ret = build_types_list(buffer, WSD_MAX_TEXT_LENGTH * sizeof(WCHAR), types_list, discovered_namespaces);
        if (FAILED(ret)) goto cleanup;

        ret = add_child_element(impl->xmlContext, probe_match_element, discoveryNsUri, typesString, buffer, NULL);
        if (FAILED(ret)) goto cleanup;
    }

    /* <wsd:Scopes> */
    if (scopes_list != NULL)
    {
        buffer = WSDAllocateLinkedMemory(probe_match_element, WSD_MAX_TEXT_LENGTH * sizeof(WCHAR));
        if (buffer == NULL) goto failed;

        ret = build_uri_list(buffer, WSD_MAX_TEXT_LENGTH * sizeof(WCHAR), scopes_list);
        if (FAILED(ret)) goto cleanup;

        ret = add_child_element(impl->xmlContext, probe_match_element, discoveryNsUri, scopesString, buffer, NULL);
        if (FAILED(ret)) goto cleanup;
    }

    /* <wsd:XAddrs> */
    if (xaddrs_list != NULL)
    {
        buffer = WSDAllocateLinkedMemory(probe_match_element, WSD_MAX_TEXT_LENGTH * sizeof(WCHAR));
        if (buffer == NULL) goto failed;

        ret = build_uri_list(buffer, WSD_MAX_TEXT_LENGTH * sizeof(WCHAR), xaddrs_list);
        if (FAILED(ret)) goto cleanup;

        ret = add_child_element(impl->xmlContext, probe_match_element, discoveryNsUri, xAddrsString, buffer, NULL);
        if (FAILED(ret)) goto cleanup;
    }

    /* <wsd:MetadataVersion> */
    ret = add_child_element(impl->xmlContext, probe_match_element, discoveryNsUri, metadataVersionString,
        ulonglong_to_string(probe_match_element, min(UINT_MAX, metadata_ver)), NULL);
    if (FAILED(ret)) goto cleanup;

    /* Write any body elements */
    if (any != NULL)
    {
        ret = duplicate_element(probe_match_element, any, discovered_namespaces);
        if (FAILED(ret)) goto cleanup;
    }

    /* Write and send the message */
    ret = write_and_send_message(impl, &soap_header, body_element, discovered_namespaces, remote_udp_addr, APP_MAX_DELAY);
    goto cleanup;

failed:
    ret = E_FAIL;

cleanup:
    WSDFreeLinkedMemory(body_name);
    WSDFreeLinkedMemory(body_element);
    WSDFreeLinkedMemory(discovered_namespaces);

    if (remote_udp_addr != NULL) IWSDUdpAddress_Release(remote_udp_addr);
    if (remote_addr != NULL) IWSDAddress_Release(remote_addr);

    return ret;
}

static LPWSTR xml_text_to_wide_string(void *parent_memory, WS_XML_TEXT *text)
{
    if (text->textType == WS_XML_TEXT_TYPE_UTF8)
    {
        WS_XML_UTF8_TEXT *utf8_text = (WS_XML_UTF8_TEXT *) text;
        return utf8_to_wide(parent_memory, (const char *) utf8_text->value.bytes, utf8_text->value.length);
    }
    else if (text->textType == WS_XML_TEXT_TYPE_UTF16)
    {
        WS_XML_UTF16_TEXT *utf_16_text = (WS_XML_UTF16_TEXT *) text;
        return duplicate_string(parent_memory, (LPCWSTR) utf_16_text->bytes);
    }

    FIXME("Support for text type %d not implemented.\n", text->textType);
    return NULL;
}

static inline BOOL read_isspace(unsigned int ch)
{
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

static HRESULT str_to_uint64(const unsigned char *str, ULONG len, UINT64 max, UINT64 *ret)
{
    const unsigned char *ptr = str;

    *ret = 0;
    while (len && read_isspace(*ptr)) { ptr++; len--; }
    while (len && read_isspace(ptr[len - 1])) { len--; }
    if (!len) return WS_E_INVALID_FORMAT;

    while (len--)
    {
        unsigned int val;

        if (!isdigit(*ptr)) return WS_E_INVALID_FORMAT;
        val = *ptr - '0';

        if ((*ret > max / 10 || *ret * 10 > max - val)) return WS_E_NUMERIC_OVERFLOW;
        *ret = *ret * 10 + val;
        ptr++;
    }

    return S_OK;
}

#define MAX_UINT64  (((UINT64)0xffffffff << 32) | 0xffffffff)

static HRESULT wide_text_to_ulonglong(LPCWSTR text, ULONGLONG *value)
{
    char *utf8_text;
    int utf8_length;
    HRESULT ret;

    utf8_text = wide_to_utf8(text, &utf8_length);

    if (utf8_text == NULL) return E_OUTOFMEMORY;
    if (utf8_length == 1) return E_FAIL;

    ret = str_to_uint64((const unsigned char *) utf8_text, utf8_length - 1, MAX_UINT64, value);
    free(utf8_text);

    return ret;
}

static HRESULT move_to_element(WS_XML_READER *reader, const char *element_name, WS_XML_STRING *uri)
{
    WS_XML_STRING envelope;
    BOOL found = FALSE;
    HRESULT ret;

    envelope.bytes = (BYTE *) element_name;
    envelope.length = strlen(element_name);
    envelope.dictionary = NULL;
    envelope.id = 0;

    ret = WsReadToStartElement(reader, &envelope, uri, &found, NULL);
    if (FAILED(ret)) return ret;

    return found ? ret : E_FAIL;
}

static void trim_trailing_slash(LPWSTR uri)
{
    /* Trim trailing slash from URI */
    int uri_len = lstrlenW(uri);
    if (uri_len > 0 && uri[uri_len - 1] == '/') uri[uri_len - 1] = 0;
}

static HRESULT ws_element_to_wsdxml_element(WS_XML_READER *reader, IWSDXMLContext *context, WSDXML_ELEMENT *parent_element)
{
    WSDXML_ATTRIBUTE *cur_wsd_attrib = NULL, *new_wsd_attrib = NULL;
    const WS_XML_ELEMENT_NODE *element_node = NULL;
    WSDXML_ELEMENT *cur_element = parent_element;
    const WS_XML_TEXT_NODE *text_node = NULL;
    LPWSTR uri = NULL, element_name = NULL;
    WS_XML_STRING *ns_string = NULL;
    WS_XML_ATTRIBUTE *attrib = NULL;
    WSDXML_ELEMENT *element = NULL;
    const WS_XML_NODE *node = NULL;
    WSDXML_NAME *name = NULL;
    WSDXML_TEXT *text = NULL;
    HRESULT ret;
    int i;

    for (;;)
    {
        if (cur_element == NULL) break;

        ret = WsReadNode(reader, NULL);
        if (FAILED(ret)) goto cleanup;

        ret = WsGetReaderNode(reader, &node, NULL);
        if (FAILED(ret)) goto cleanup;

        switch (node->nodeType)
        {
            case WS_XML_NODE_TYPE_ELEMENT:
                element_node = (const WS_XML_ELEMENT_NODE *) node;

                uri = utf8_to_wide(NULL, (const char *) element_node->ns->bytes, element_node->ns->length);
                if (uri == NULL) goto outofmemory;

                /* Link element_name to uri so they will be freed at the same time */
                element_name = utf8_to_wide(uri, (const char *) element_node->localName->bytes,
                    element_node->localName->length);
                if (element_name == NULL) goto outofmemory;

                trim_trailing_slash(uri);

                ret = IWSDXMLContext_AddNameToNamespace(context, uri, element_name, &name);
                if (FAILED(ret)) goto cleanup;

                WSDFreeLinkedMemory(uri);
                uri = NULL;

                ret = WSDXMLBuildAnyForSingleElement(name, NULL, &element);
                if (FAILED(ret)) goto cleanup;
                WSDXMLAddChild(cur_element, element);

                WSDFreeLinkedMemory(name);
                name = NULL;

                cur_wsd_attrib = NULL;

                /* Add attributes */
                for (i = 0; i < element_node->attributeCount; i++)
                {
                    attrib = element_node->attributes[i];
                    if (attrib->isXmlNs) continue;

                    new_wsd_attrib = WSDAllocateLinkedMemory(element, sizeof(WSDXML_ATTRIBUTE));
                    if (new_wsd_attrib == NULL) goto outofmemory;

                    ns_string = attrib->ns;
                    if (ns_string->length == 0) ns_string = element_node->ns;

                    uri = utf8_to_wide(NULL, (const char *) ns_string->bytes, ns_string->length);
                    if (uri == NULL) goto outofmemory;

                    trim_trailing_slash(uri);

                    /* Link element_name to uri so they will be freed at the same time */
                    element_name = utf8_to_wide(uri, (const char *) attrib->localName->bytes, attrib->localName->length);
                    if (element_name == NULL) goto outofmemory;

                    ret = IWSDXMLContext_AddNameToNamespace(context, uri, element_name, &name);
                    if (FAILED(ret)) goto cleanup;

                    WSDFreeLinkedMemory(uri);
                    uri = NULL;

                    new_wsd_attrib->Value = xml_text_to_wide_string(new_wsd_attrib, attrib->value);
                    if (new_wsd_attrib->Value == NULL) goto outofmemory;

                    new_wsd_attrib->Name = name;
                    new_wsd_attrib->Element = cur_element;
                    new_wsd_attrib->Next = NULL;

                    WSDAttachLinkedMemory(new_wsd_attrib, name);
                    name = NULL;

                    if (cur_wsd_attrib == NULL)
                        element->FirstAttribute = new_wsd_attrib;
                    else
                        cur_wsd_attrib->Next = new_wsd_attrib;

                    cur_wsd_attrib = new_wsd_attrib;
                }

                cur_element = element;
                break;

            case WS_XML_NODE_TYPE_TEXT:
                text_node = (const WS_XML_TEXT_NODE *) node;

                if (cur_element == NULL)
                {
                    WARN("No parent element open but encountered text element!\n");
                    continue;
                }

                if (cur_element->FirstChild != NULL)
                {
                    WARN("Text node encountered but parent already has child!\n");
                    continue;
                }

                text = WSDAllocateLinkedMemory(element, sizeof(WSDXML_TEXT));
                if (text == NULL) goto outofmemory;

                text->Node.Parent = element;
                text->Node.Next = NULL;
                text->Node.Type = TextType;
                text->Text = xml_text_to_wide_string(text, text_node->text);

                if (text->Text == NULL)
                {
                    WARN("Text node returned null string.\n");
                    WSDFreeLinkedMemory(text);
                    continue;
                }

                cur_element->FirstChild = (WSDXML_NODE *) text;
                break;

            case WS_XML_NODE_TYPE_END_ELEMENT:
                /* Go up a level to the parent element */
                cur_element = cur_element->Node.Parent;
                break;

            default:
                break;
        }
    }

    return S_OK;

outofmemory:
    ret = E_OUTOFMEMORY;

cleanup:
    /* Free uri and element_name if applicable */
    WSDFreeLinkedMemory(uri);
    WSDFreeLinkedMemory(name);
    return ret;
}

static WSDXML_ELEMENT *find_element(WSDXML_ELEMENT *parent, LPCWSTR name, LPCWSTR ns_uri)
{
    WSDXML_ELEMENT *cur = (WSDXML_ELEMENT *) parent->FirstChild;

    while (cur != NULL)
    {
        if ((lstrcmpW(cur->Name->LocalName, name) == 0) && (lstrcmpW(cur->Name->Space->Uri, ns_uri) == 0))
            return cur;

        cur = (WSDXML_ELEMENT *) cur->Node.Next;
    }

    return NULL;
}

static void remove_element(WSDXML_ELEMENT *element)
{
    WSDXML_NODE *cur;

    if (element == NULL)
        return;

    if (element->Node.Parent->FirstChild == (WSDXML_NODE *) element)
        element->Node.Parent->FirstChild = element->Node.Next;
    else
    {
        cur = element->Node.Parent->FirstChild;

        while (cur != NULL)
        {
            if (cur->Next == (WSDXML_NODE *) element)
            {
                cur->Next = element->Node.Next;
                break;
            }

            cur = cur->Next;
        }
    }

    WSDDetachLinkedMemory(element);
    WSDFreeLinkedMemory(element);
}

static WSD_NAME_LIST *build_types_list_from_string(IWSDXMLContext *context, LPCWSTR buffer, void *parent)
{
    WSD_NAME_LIST *list = NULL, *cur_list = NULL, *prev_list = NULL;
    LPWSTR name_start = NULL, temp_buffer = NULL;
    LPCWSTR prefix_start = buffer;
    WSDXML_NAMESPACE *ns;
    WSDXML_NAME *name;
    int buffer_len, i;

    if (buffer == NULL)
        return NULL;

    temp_buffer = duplicate_string(parent, buffer);
    if (temp_buffer == NULL) goto cleanup;

    buffer_len = lstrlenW(temp_buffer);

    list = WSDAllocateLinkedMemory(parent, sizeof(WSD_NAME_LIST));
    if (list == NULL) goto cleanup;

    ZeroMemory(list, sizeof(WSD_NAME_LIST));
    prefix_start = temp_buffer;

    for (i = 0; i < buffer_len; i++)
    {
        if (temp_buffer[i] == ':')
        {
            temp_buffer[i] = 0;
            name_start = &temp_buffer[i + 1];
        }
        else if ((temp_buffer[i] == ' ') || (i == buffer_len - 1))
        {
            WSDXML_NAMESPACE *known_ns;

            if (temp_buffer[i] == ' ')
                temp_buffer[i] = 0;

            if (cur_list == NULL)
                cur_list = list;
            else
            {
                cur_list = WSDAllocateLinkedMemory(parent, sizeof(WSD_NAME_LIST));
                if (cur_list == NULL) goto cleanup;

                prev_list->Next = cur_list;
            }

            name = WSDAllocateLinkedMemory(cur_list, sizeof(WSDXML_NAME));
            if (name == NULL) goto cleanup;

            ns = WSDAllocateLinkedMemory(cur_list, sizeof(WSDXML_NAMESPACE));
            if (ns == NULL) goto cleanup;

            ZeroMemory(ns, sizeof(WSDXML_NAMESPACE));
            ns->PreferredPrefix = duplicate_string(ns, prefix_start);

            known_ns = xml_context_find_namespace_by_prefix(context, ns->PreferredPrefix);

            if (known_ns != NULL)
                ns->Uri = duplicate_string(ns, known_ns->Uri);

            name->Space = ns;
            name->LocalName = duplicate_string(name, name_start);

            cur_list->Element = name;
            prefix_start = &temp_buffer[i + 1];
            name_start = NULL;
        }
    }

    WSDFreeLinkedMemory(temp_buffer);
    return list;

cleanup:
    WSDFreeLinkedMemory(list);
    WSDFreeLinkedMemory(temp_buffer);

    return NULL;
}

static WSDXML_TYPE *generate_type(LPCWSTR uri, void *parent)
{
    WSDXML_TYPE *type = WSDAllocateLinkedMemory(parent, sizeof(WSDXML_TYPE));

    if (type == NULL)
        return NULL;

    type->Uri = duplicate_string(parent, uri);
    type->Table = NULL;

    return type;
}

static BOOL is_duplicate_message(IWSDiscoveryPublisherImpl *impl, LPCWSTR id)
{
    struct message_id *msg_id, *msg_id_cursor;
    BOOL ret = FALSE;
    int len;

    EnterCriticalSection(&impl->message_ids_critical_section);

    LIST_FOR_EACH_ENTRY_SAFE(msg_id, msg_id_cursor, &impl->message_ids, struct message_id, entry)
    {
        if (lstrcmpW(msg_id->id, id) == 0)
        {
            ret = TRUE;
            goto end;
        }
    }

    msg_id = malloc(sizeof(*msg_id));
    if (!msg_id) goto end;

    len = (lstrlenW(id) + 1) * sizeof(WCHAR);
    msg_id->id = malloc(len);

    if (!msg_id->id)
    {
        free(msg_id);
        goto end;
    }

    memcpy(msg_id->id, id, len);
    list_add_tail(&impl->message_ids, &msg_id->entry);

end:
    LeaveCriticalSection(&impl->message_ids_critical_section);
    return ret;
}

HRESULT read_message(IWSDiscoveryPublisherImpl *impl, const char *xml, int xml_length, WSD_SOAP_MESSAGE **out_msg, int *msg_type)
{
    WSDXML_ELEMENT *envelope = NULL, *header_element, *appsequence_element, *body_element;
    WS_XML_READER_TEXT_ENCODING encoding;
    WS_XML_ELEMENT_NODE *envelope_node;
    WSD_SOAP_MESSAGE *soap_msg = NULL;
    WS_XML_READER_BUFFER_INPUT input;
    WS_XML_ATTRIBUTE *attrib = NULL;
    IWSDXMLContext *context = NULL;
    WS_XML_STRING *soap_uri = NULL;
    const WS_XML_NODE *node;
    WS_XML_READER *reader = NULL;
    LPCWSTR value = NULL;
    LPWSTR uri, prefix;
    WS_HEAP *heap = NULL;
    HRESULT ret;
    int i;

    *msg_type = MSGTYPE_UNKNOWN;

    ret = WsCreateHeap(16384, 4096, NULL, 0, &heap, NULL);
    if (FAILED(ret)) goto cleanup;

    ret = WsCreateReader(NULL, 0, &reader, NULL);
    if (FAILED(ret)) goto cleanup;

    encoding.encoding.encodingType = WS_XML_READER_ENCODING_TYPE_TEXT;
    encoding.charSet = WS_CHARSET_AUTO;

    input.input.inputType = WS_XML_READER_INPUT_TYPE_BUFFER;
    input.encodedData = (char *) xml;
    input.encodedDataSize = xml_length;

    ret = WsSetInput(reader, (WS_XML_READER_ENCODING *) &encoding, (WS_XML_READER_INPUT *) &input, NULL, 0, NULL);
    if (FAILED(ret)) goto cleanup;

    soap_uri = populate_xml_string(envelopeNsUri);
    if (soap_uri == NULL) goto outofmemory;

    ret = move_to_element(reader, "Envelope", soap_uri);
    if (FAILED(ret)) goto cleanup;

    ret = WsGetReaderNode(reader, &node, NULL);
    if (FAILED(ret)) goto cleanup;

    if (node->nodeType != WS_XML_NODE_TYPE_ELEMENT)
    {
        WARN("Unexpected node type (%d)\n", node->nodeType);
        ret = E_FAIL;
        goto cleanup;
    }

    envelope_node = (WS_XML_ELEMENT_NODE *) node;

    ret = WSDXMLCreateContext(&context);
    if (FAILED(ret)) goto cleanup;

    /* Find XML namespaces from the envelope element's attributes */
    for (i = 0; i < envelope_node->attributeCount; i++)
    {
        attrib = envelope_node->attributes[i];

        if (attrib->isXmlNs)
        {
            uri = utf8_to_wide(NULL, (const char *) attrib->ns->bytes, attrib->ns->length);
            if (uri == NULL) continue;

            trim_trailing_slash(uri);

            prefix = utf8_to_wide(uri, (const char *) attrib->localName->bytes, attrib->localName->length);

            if (prefix == NULL)
            {
                WSDFreeLinkedMemory(uri);
                continue;
            }

            IWSDXMLContext_AddNamespace(context, uri, prefix, NULL);
            WSDFreeLinkedMemory(uri);
        }
    }

    /* Create the SOAP message to return to the caller */
    soap_msg = WSDAllocateLinkedMemory(NULL, sizeof(WSD_SOAP_MESSAGE));
    if (soap_msg == NULL) goto outofmemory;

    ZeroMemory(soap_msg, sizeof(WSD_SOAP_MESSAGE));

    envelope = WSDAllocateLinkedMemory(soap_msg, sizeof(WSDXML_ELEMENT));
    if (envelope == NULL) goto outofmemory;

    ZeroMemory(envelope, sizeof(WSDXML_ELEMENT));

    ret = ws_element_to_wsdxml_element(reader, context, envelope);
    if (FAILED(ret)) goto cleanup;

    /* Find the header element */
    header_element = find_element(envelope, headerString, envelopeNsUri);

    if (header_element == NULL)
    {
        WARN("Unable to find header element in received SOAP message\n");
        ret = E_FAIL;
        goto cleanup;
    }

    ret = WSDXMLGetValueFromAny(addressingNsUri, actionString, (WSDXML_ELEMENT *) header_element->FirstChild, &value);
    if (FAILED(ret)) goto cleanup;
    soap_msg->Header.Action = duplicate_string(soap_msg, value);
    if (soap_msg->Header.Action == NULL) goto outofmemory;

    ret = WSDXMLGetValueFromAny(addressingNsUri, toString, (WSDXML_ELEMENT *) header_element->FirstChild, &value);
    if (FAILED(ret)) goto cleanup;
    soap_msg->Header.To = duplicate_string(soap_msg, value);
    if (soap_msg->Header.To == NULL) goto outofmemory;

    ret = WSDXMLGetValueFromAny(addressingNsUri, messageIdString, (WSDXML_ELEMENT *) header_element->FirstChild, &value);
    if (FAILED(ret)) goto cleanup;

    /* Detect duplicate messages */
    if (is_duplicate_message(impl, value))
    {
        ret = E_FAIL;
        goto cleanup;
    }

    soap_msg->Header.MessageID = duplicate_string(soap_msg, value);
    if (soap_msg->Header.MessageID == NULL) goto outofmemory;

    /* Look for optional AppSequence element */
    appsequence_element = find_element(header_element, appSequenceString, discoveryNsUri);

    if (appsequence_element != NULL)
    {
        WSDXML_ATTRIBUTE *current_attrib;

        soap_msg->Header.AppSequence = WSDAllocateLinkedMemory(soap_msg, sizeof(WSD_APP_SEQUENCE));
        if (soap_msg->Header.AppSequence == NULL) goto outofmemory;

        ZeroMemory(soap_msg->Header.AppSequence, sizeof(WSD_APP_SEQUENCE));

        current_attrib = appsequence_element->FirstAttribute;

        while (current_attrib != NULL)
        {
            if (lstrcmpW(current_attrib->Name->Space->Uri, discoveryNsUri) != 0)
            {
                current_attrib = current_attrib->Next;
                continue;
            }

            if (lstrcmpW(current_attrib->Name->LocalName, instanceIdString) == 0)
            {
                ret = wide_text_to_ulonglong(current_attrib->Value, &soap_msg->Header.AppSequence->InstanceId);
                if (FAILED(ret)) goto cleanup;
            }
            else if (lstrcmpW(current_attrib->Name->LocalName, messageNumberString) == 0)
            {
                ret = wide_text_to_ulonglong(current_attrib->Value, &soap_msg->Header.AppSequence->MessageNumber);
                if (FAILED(ret)) goto cleanup;
            }
            else if (lstrcmpW(current_attrib->Name->LocalName, sequenceIdString) == 0)
            {
                soap_msg->Header.AppSequence->SequenceId = duplicate_string(soap_msg, current_attrib->Value);
                if (soap_msg->Header.AppSequence->SequenceId == NULL) goto outofmemory;
            }

            current_attrib = current_attrib->Next;
        }
    }

    /* Now detach and free known headers to leave the "any" elements */
    remove_element(find_element(header_element, actionString, addressingNsUri));
    remove_element(find_element(header_element, toString, addressingNsUri));
    remove_element(find_element(header_element, messageIdString, addressingNsUri));
    remove_element(find_element(header_element, appSequenceString, discoveryNsUri));

    soap_msg->Header.AnyHeaders = (WSDXML_ELEMENT *) header_element->FirstChild;

    if (soap_msg->Header.AnyHeaders != NULL)
        soap_msg->Header.AnyHeaders->Node.Parent = NULL;

    /* Find the body element */
    body_element = find_element(envelope, bodyString, envelopeNsUri);

    if (body_element == NULL)
    {
        WARN("Unable to find body element in received SOAP message\n");
        ret = E_FAIL;
        goto cleanup;
    }

    /* Now figure out which message we've been sent */
    if (lstrcmpW(soap_msg->Header.Action, actionProbe) == 0)
    {
        WSDXML_ELEMENT *probe_element;
        WSD_PROBE *probe = NULL;

        probe_element = find_element(body_element, probeString, discoveryNsUri);
        if (probe_element == NULL) goto cleanup;

        probe = WSDAllocateLinkedMemory(soap_msg, sizeof(WSD_PROBE));
        if (probe == NULL) goto cleanup;

        ZeroMemory(probe, sizeof(WSD_PROBE));

        /* Check for the "types" element */
        ret = WSDXMLGetValueFromAny(discoveryNsUri, typesString, (WSDXML_ELEMENT *) probe_element->FirstChild, &value);

        if (FAILED(ret))
        {
            WARN("Unable to find Types element in received Probe message\n");
            goto cleanup;
        }

        probe->Types = build_types_list_from_string(context, value, probe);

        /* Now detach and free known headers to leave the "any" elements */
        remove_element(find_element(probe_element, typesString, discoveryNsUri));
        remove_element(find_element(probe_element, scopesString, discoveryNsUri));

        probe->Any = (WSDXML_ELEMENT *) probe_element->FirstChild;

        if (probe->Any != NULL)
            probe->Any->Node.Parent = NULL;

        soap_msg->Body = probe;
        soap_msg->BodyType = generate_type(actionProbe, soap_msg);
        if (soap_msg->BodyType == NULL) goto cleanup;

        *out_msg = soap_msg;
        soap_msg = NULL; /* caller will clean this up */
        *msg_type = MSGTYPE_PROBE;
    }

    goto cleanup;

outofmemory:
    ret = E_OUTOFMEMORY;

cleanup:
    free_xml_string(soap_uri);
    WSDFreeLinkedMemory(soap_msg);
    if (context != NULL) IWSDXMLContext_Release(context);
    if (reader != NULL) WsFreeReader(reader);
    if (heap != NULL) WsFreeHeap(heap);

    return ret;
}
