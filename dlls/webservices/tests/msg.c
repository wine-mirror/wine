/*
 * Copyright 2016 Hans Leidekker for CodeWeavers
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

#include <stdio.h>
#include "windows.h"
#include "webservices.h"
#include "wine/test.h"

static void test_WsCreateMessage(void)
{
    HRESULT hr;
    WS_MESSAGE *msg;
    WS_MESSAGE_STATE state;
    WS_ENVELOPE_VERSION env_version;
    WS_ADDRESSING_VERSION addr_version;
    WS_MESSAGE_PROPERTY prop;

    hr = WsCreateMessage( 0, 0, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateMessage( 0, 0, NULL, 0, &msg, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_1, 0, NULL, 0, &msg, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateMessage( 0, WS_ADDRESSING_VERSION_0_9, NULL, 0, &msg, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    env_version = WS_ENVELOPE_VERSION_SOAP_1_1;
    prop.id = WS_MESSAGE_PROPERTY_ENVELOPE_VERSION;
    prop.value = &env_version;
    prop.valueSize = sizeof(env_version);
    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_1, WS_ADDRESSING_VERSION_0_9, &prop,
                          1, &msg, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_1, WS_ADDRESSING_VERSION_0_9, NULL,
                          0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    state = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( state == WS_MESSAGE_STATE_EMPTY, "got %u\n", state );

    env_version = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_ENVELOPE_VERSION, &env_version,
                               sizeof(env_version), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( env_version == WS_ENVELOPE_VERSION_SOAP_1_1, "got %u\n", env_version );

    addr_version = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_ADDRESSING_VERSION, &addr_version,
                               sizeof(addr_version), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( addr_version == WS_ADDRESSING_VERSION_0_9, "got %u\n", addr_version );

    state = WS_MESSAGE_STATE_EMPTY;
    hr = WsSetMessageProperty( msg, WS_MESSAGE_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    env_version = WS_ENVELOPE_VERSION_SOAP_1_1;
    hr = WsSetMessageProperty( msg, WS_MESSAGE_PROPERTY_ENVELOPE_VERSION, &env_version,
                               sizeof(env_version), NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    addr_version = WS_ADDRESSING_VERSION_0_9;
    hr = WsSetMessageProperty( msg, WS_MESSAGE_PROPERTY_ADDRESSING_VERSION, &addr_version,
                               sizeof(addr_version), NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );
    WsFreeMessage( msg );
}

static void test_WsCreateMessageForChannel(void)
{
    HRESULT hr;
    WS_CHANNEL *channel;
    WS_MESSAGE *msg;
    WS_MESSAGE_STATE state;
    WS_ENVELOPE_VERSION env_version;
    WS_ADDRESSING_VERSION addr_version;
    WS_CHANNEL_PROPERTY prop;
    BOOL addressed;

    hr = WsCreateMessageForChannel( NULL, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateMessageForChannel( NULL, NULL, 0, &msg, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    /* HTTP */
    hr = WsCreateChannel( WS_CHANNEL_TYPE_REQUEST, WS_HTTP_CHANNEL_BINDING, NULL, 0, NULL,
                          &channel, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    state = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( state == WS_MESSAGE_STATE_EMPTY, "got %u\n", state );

    env_version = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_ENVELOPE_VERSION, &env_version,
                               sizeof(env_version), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( env_version == WS_ENVELOPE_VERSION_SOAP_1_2, "got %u\n", env_version );

    addr_version = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_ADDRESSING_VERSION, &addr_version,
                               sizeof(addr_version), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( addr_version == WS_ADDRESSING_VERSION_1_0, "got %u\n", addr_version );

    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_IS_ADDRESSED, &addressed, sizeof(addressed),
                               NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    WsFreeChannel( channel );
    WsFreeMessage( msg );

    env_version = WS_ENVELOPE_VERSION_SOAP_1_1;
    prop.id = WS_CHANNEL_PROPERTY_ENVELOPE_VERSION;
    prop.value = &env_version;
    prop.valueSize = sizeof(env_version);
    hr = WsCreateChannel( WS_CHANNEL_TYPE_REQUEST, WS_HTTP_CHANNEL_BINDING, &prop, 1, NULL,
                          &channel, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    env_version = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_ENVELOPE_VERSION, &env_version,
                               sizeof(env_version), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( env_version == WS_ENVELOPE_VERSION_SOAP_1_1, "got %u\n", env_version );

    WsFreeChannel( channel );
    WsFreeMessage( msg );

    /* TCP */
    hr = WsCreateChannel( WS_CHANNEL_TYPE_DUPLEX_SESSION, WS_TCP_CHANNEL_BINDING, NULL, 0, NULL,
                          &channel, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    state = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( state == WS_MESSAGE_STATE_EMPTY, "got %u\n", state );

    env_version = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_ENVELOPE_VERSION, &env_version,
                               sizeof(env_version), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( env_version == WS_ENVELOPE_VERSION_SOAP_1_2, "got %u\n", env_version );

    addr_version = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_ADDRESSING_VERSION, &addr_version,
                               sizeof(addr_version), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( addr_version == WS_ADDRESSING_VERSION_1_0, "got %u\n", addr_version );

    WsFreeChannel( channel );
    WsFreeMessage( msg );

    /* UDP */
    hr = WsCreateChannel( WS_CHANNEL_TYPE_DUPLEX, WS_UDP_CHANNEL_BINDING, NULL, 0, NULL,
                          &channel, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    env_version = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_ENVELOPE_VERSION, &env_version,
                               sizeof(env_version), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( env_version == WS_ENVELOPE_VERSION_SOAP_1_2, "got %u\n", env_version );

    addr_version = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_ADDRESSING_VERSION, &addr_version,
                               sizeof(addr_version), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( addr_version == WS_ADDRESSING_VERSION_1_0, "got %u\n", addr_version );

    WsFreeChannel( channel );
    WsFreeMessage( msg );
}

static void test_WsInitializeMessage(void)
{
    HRESULT hr;
    WS_MESSAGE *msg;
    WS_MESSAGE_STATE state;
    WS_ENVELOPE_VERSION env_version;
    WS_ADDRESSING_VERSION addr_version;
    BOOL addressed;

    hr = WsInitializeMessage( NULL, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_1, WS_ADDRESSING_VERSION_0_9, NULL,
                          0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsInitializeMessage( msg, 0xdeadbeef, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    state = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( state == WS_MESSAGE_STATE_INITIALIZED, "got %u\n", state );

    addressed = -1;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_IS_ADDRESSED, &addressed, sizeof(addressed),
                               NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !addressed, "unexpected value %d\n", addressed );

    state = WS_MESSAGE_STATE_EMPTY;
    hr = WsSetMessageProperty( msg, WS_MESSAGE_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    env_version = WS_ENVELOPE_VERSION_SOAP_1_1;
    hr = WsSetMessageProperty( msg, WS_MESSAGE_PROPERTY_ENVELOPE_VERSION, &env_version,
                               sizeof(env_version), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    addr_version = WS_ADDRESSING_VERSION_0_9;
    hr = WsSetMessageProperty( msg, WS_MESSAGE_PROPERTY_ADDRESSING_VERSION, &addr_version,
                               sizeof(addr_version), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );
    WsFreeMessage( msg );
}

static void test_WsAddressMessage(void)
{
    HRESULT hr;
    WS_MESSAGE *msg;
    WS_ENDPOINT_ADDRESS endpoint;
    BOOL addressed;

    hr = WsAddressMessage( NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_1, WS_ADDRESSING_VERSION_0_9, NULL,
                          0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_IS_ADDRESSED, &addressed, sizeof(addressed),
                               NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsAddressMessage( msg, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    addressed = -1;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_IS_ADDRESSED, &addressed, sizeof(addressed),
                               NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !addressed, "unexpected value %d\n", addressed );

    hr = WsAddressMessage( msg, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    addressed = -1;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_IS_ADDRESSED, &addressed, sizeof(addressed),
                               NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( addressed == TRUE, "unexpected value %d\n", addressed );
    WsFreeMessage( msg );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_1, WS_ADDRESSING_VERSION_0_9, NULL,
                          0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    memset( &endpoint, 0, sizeof(endpoint) );
    endpoint.url.chars  = (WCHAR *)L"http://localhost/";
    endpoint.url.length = ARRAY_SIZE( L"http://localhost/" ) - 1;
    hr = WsAddressMessage( msg, &endpoint, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    addressed = -1;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_IS_ADDRESSED, &addressed, sizeof(addressed),
                               NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( addressed == TRUE, "unexpected value %d\n", addressed );
    WsFreeMessage( msg );
}

static HRESULT set_output( WS_XML_WRITER *writer )
{
    WS_XML_WRITER_TEXT_ENCODING text = { {WS_XML_WRITER_ENCODING_TYPE_TEXT}, WS_CHARSET_UTF8 };
    WS_XML_WRITER_BUFFER_OUTPUT buf = { {WS_XML_WRITER_OUTPUT_TYPE_BUFFER} };
    return WsSetOutput( writer, &text.encoding, &buf.output, NULL, 0, NULL );
}

static void check_output( WS_XML_WRITER *writer, const char *expected, int len, unsigned int skip_start,
                          unsigned int skip_len, unsigned int line )
{
    WS_BYTES bytes;
    HRESULT hr;

    if (len == -1) len = strlen( expected );
    memset( &bytes, 0, sizeof(bytes) );
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_BYTES, &bytes, sizeof(bytes), NULL );
    ok( hr == S_OK, "%u: got %#lx\n", line, hr );
    ok( bytes.length == len, "%u: got %lu expected %d\n", line, bytes.length, len );
    if (bytes.length != len) return;
    if (skip_start)
    {
        ok( !memcmp( bytes.bytes, expected, skip_start ), "%u: got %s expected %s\n", line,
            bytes.bytes, expected );
        ok( !memcmp( bytes.bytes + skip_start + skip_len, expected + skip_start + skip_len,
            len - skip_start - skip_len), "%u: got %s expected %s\n", line, bytes.bytes, expected );
    }
    else
        ok( !memcmp( bytes.bytes, expected, len ), "%u: got %s expected %s\n", line, bytes.bytes,
            expected );
}

static void check_output_header( WS_MESSAGE *msg, const char *expected, int len, unsigned int skip_start,
                                 unsigned int skip_len, unsigned int line )
{
    WS_XML_WRITER *writer;
    WS_XML_BUFFER *buf;
    HRESULT hr;

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_HEADER_BUFFER, &buf, sizeof(buf), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteXmlBuffer( writer, buf, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    check_output( writer, expected, len, skip_start, skip_len, line );
    WsFreeWriter( writer );
}

static void test_WsWriteEnvelopeStart(void)
{
    static const char expected[] =
        "<s:Envelope xmlns:a=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" "
        "xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Header>"
        "<a:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</a:MessageID>"
        "<a:ReplyTo><a:Address>http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous</a:Address>"
        "</a:ReplyTo></s:Header>";
    static const char expected2[] =
        "<s:Envelope xmlns:a=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" "
        "xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Header>"
        "<a:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</a:MessageID>"
        "<a:ReplyTo><a:Address>http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous</a:Address>"
        "</a:ReplyTo></s:Header><s:Body/></s:Envelope>";
    static const char expected3[] =
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Header/><s:Body/></s:Envelope>";
    static const char expected4[] =
        "<Envelope><Header/><Body/></Envelope>";
    static const char expected5[] =
        "<s:Envelope xmlns:a=\"http://www.w3.org/2005/08/addressing\" "
        "xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Header>"
        "<a:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</a:MessageID>"
        "</s:Header><s:Body/></s:Envelope>";
    static const char expected6[] =
        "<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Header/><s:Body/></s:Envelope>";
    static const char expected7[] =
        "<s:Envelope xmlns:a=\"http://www.w3.org/2005/08/addressing\" "
        "xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Header>"
        "<a:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</a:MessageID>"
        "</s:Header><s:Body/></s:Envelope>";
    static const char expected8[] =
        "<s:Envelope xmlns:a=\"http://www.w3.org/2005/08/addressing\" "
        "xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Header/><s:Body/></s:Envelope>";
    static const char expected9[] =
        "<s:Envelope xmlns:a=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" "
        "xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Header>"
        "<a:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</a:MessageID>"
        "<a:ReplyTo><a:Address>http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous</a:Address>"
        "</a:ReplyTo><a:To s:mustUnderstand=\"1\">http://localhost/</a:To></s:Header><s:Body/></s:Envelope>";
    static const char expected10[] =
        "<s:Envelope xmlns:a=\"http://www.w3.org/2005/08/addressing\" "
        "xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Header>"
        "<a:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</a:MessageID>"
        "<a:To s:mustUnderstand=\"1\">http://localhost/</a:To></s:Header><s:Body/></s:Envelope>";
    HRESULT hr;
    WS_MESSAGE *msg;
    WS_XML_WRITER *writer;
    WS_MESSAGE_STATE state;
    WS_ENDPOINT_ADDRESS addr;

    memset( &addr, 0, sizeof(addr) );
    addr.url.chars  = (WCHAR *) L"http://localhost/";
    addr.url.length = 17;

    hr = WsWriteEnvelopeStart( NULL, NULL, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEnvelopeStart( NULL, writer, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_1, WS_ADDRESSING_VERSION_0_9, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEnvelopeStart( msg, writer, NULL, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEnvelopeStart( msg, writer, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, expected, -1, strstr(expected, "urn:uuid:") - expected, 46, __LINE__ );
    check_output_header( msg, expected2, -1, strstr(expected2, "urn:uuid:") - expected2, 46, __LINE__ );

    state = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( state == WS_MESSAGE_STATE_WRITING, "got %u\n", state );
    WsFreeMessage( msg );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_1, WS_ADDRESSING_VERSION_TRANSPORT, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEnvelopeStart( msg, writer, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_header( msg, expected3, -1, 0, 0, __LINE__ );
    WsFreeMessage( msg );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_NONE, WS_ADDRESSING_VERSION_TRANSPORT, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEnvelopeStart( msg, writer, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_header( msg, expected4, -1, 0, 0, __LINE__ );
    WsFreeMessage( msg );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_2, WS_ADDRESSING_VERSION_1_0, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEnvelopeStart( msg, writer, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_header( msg, expected5, -1, strstr(expected5, "urn:uuid:") - expected5, 46, __LINE__ );
    WsFreeMessage( msg );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_2, WS_ADDRESSING_VERSION_TRANSPORT, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEnvelopeStart( msg, writer, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_header( msg, expected6, -1, 0, 0, __LINE__ );
    WsFreeMessage( msg );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_1, WS_ADDRESSING_VERSION_1_0, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEnvelopeStart( msg, writer, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_header( msg, expected7, -1, strstr(expected7, "urn:uuid:") - expected7, 46, __LINE__ );
    WsFreeMessage( msg );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_1, WS_ADDRESSING_VERSION_1_0, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsInitializeMessage( msg, WS_BLANK_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEnvelopeStart( msg, writer, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_header( msg, expected8, -1, 0, 0, __LINE__ );
    WsFreeMessage( msg );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_1, WS_ADDRESSING_VERSION_0_9, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsAddressMessage( msg, &addr, NULL );
    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEnvelopeStart( msg, writer, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_header( msg, expected9, -1, strstr(expected9, "urn:uuid:") - expected9, 46, __LINE__ );
    WsFreeMessage( msg );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_1, WS_ADDRESSING_VERSION_1_0, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsAddressMessage( msg, &addr, NULL );
    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEnvelopeStart( msg, writer, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_header( msg, expected10, -1, strstr(expected10, "urn:uuid:") - expected10, 46, __LINE__ );
    WsFreeMessage( msg );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_2, WS_ADDRESSING_VERSION_1_0, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    WsFreeMessage( msg );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_NONE, WS_ADDRESSING_VERSION_0_9, NULL, 0, &msg, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );
    hr = WsCreateMessage( WS_ENVELOPE_VERSION_NONE, WS_ADDRESSING_VERSION_1_0, NULL, 0, &msg, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );
    WsFreeWriter( writer );
}

static void test_WsWriteEnvelopeEnd(void)
{
    static const char expected[] =
        "<s:Envelope xmlns:a=\"http://www.w3.org/2005/08/addressing\" "
        "xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Header>"
        "<a:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</a:MessageID>"
        "</s:Header><s:Body/></s:Envelope>";
    HRESULT hr;
    WS_MESSAGE *msg;
    WS_XML_WRITER *writer;
    WS_MESSAGE_STATE state;

    hr = WsWriteEnvelopeEnd( NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_2, WS_ADDRESSING_VERSION_1_0, NULL, 0, &msg,
                          NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEnvelopeEnd( msg, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEnvelopeEnd( msg, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEnvelopeStart( msg, writer, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEnvelopeEnd( msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, expected, -1, strstr(expected, "urn:uuid:") - expected, 46, __LINE__ );
    check_output_header( msg, expected, -1, strstr(expected, "urn:uuid:") - expected, 46, __LINE__ );

    state = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( state == WS_MESSAGE_STATE_DONE, "got %u\n", state );

    WsFreeMessage( msg );
    WsFreeWriter( writer );
}

static void test_WsWriteBody(void)
{
    static char expected[] =
        "<s:Envelope xmlns:a=\"http://www.w3.org/2005/08/addressing\" "
        "xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Header>"
        "<a:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</a:MessageID>"
        "</s:Header><s:Body><u xmlns=\"ns\"><val>1</val></u></s:Body></s:Envelope>";
    static char expected2[] =
        "<s:Envelope xmlns:a=\"http://www.w3.org/2005/08/addressing\" "
        "xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Header>"
        "<a:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</a:MessageID>"
        "</s:Header><s:Body/></s:Envelope>";
    WS_XML_STRING localname = {1, (BYTE *)"t"}, localname2 = {1, (BYTE *)"u"};
    WS_XML_STRING val = {3, (BYTE *)"val"}, ns = {2, (BYTE *)"ns"};
    HRESULT hr;
    WS_MESSAGE *msg;
    WS_XML_WRITER *writer;
    WS_MESSAGE_STATE state;
    WS_ELEMENT_DESCRIPTION desc;
    WS_STRUCT_DESCRIPTION s;
    WS_FIELD_DESCRIPTION f, *fields[1];
    struct test
    {
        UINT32 val;
    } test, *ptr;

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_2, WS_ADDRESSING_VERSION_1_0, NULL, 0, &msg,
                          NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEnvelopeStart( msg, writer, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteBody( NULL, NULL, 0, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsWriteBody( msg, NULL, 0, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    memset( &f, 0, sizeof(f) );
    f.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f.localName = &val;
    f.ns        = &ns;
    f.type      = WS_UINT32_TYPE;
    fields[0] = &f;

    memset( &s, 0, sizeof(s) );
    s.size          = sizeof(struct test);
    s.alignment     = TYPE_ALIGNMENT(struct test);
    s.fields        = fields;
    s.fieldCount    = 1;
    s.typeLocalName = &localname;
    s.typeNs        = &ns;

    desc.elementLocalName = &localname2;
    desc.elementNs        = &ns;
    desc.type             = WS_STRUCT_TYPE;
    desc.typeDescription  = &s;

    ptr = &test;
    test.val = 1;
    hr = WsWriteBody( msg, &desc, WS_WRITE_REQUIRED_POINTER, &ptr, sizeof(ptr), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, expected, 240, strstr(expected, "urn:uuid:") - expected, 46, __LINE__ );
    check_output_header( msg, expected2, -1, strstr(expected2, "urn:uuid:") - expected2, 46, __LINE__ );

    state = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( state == WS_MESSAGE_STATE_WRITING, "got %u\n", state );

    hr = WsWriteEnvelopeEnd( msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, expected, -1, strstr(expected, "urn:uuid:") - expected, 46, __LINE__ );
    check_output_header( msg, expected2, -1, strstr(expected2, "urn:uuid:") - expected2, 46, __LINE__ );

    hr = WsWriteBody( msg, &desc, WS_WRITE_REQUIRED_POINTER, &ptr, sizeof(ptr), NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    WsFreeMessage( msg );
    WsFreeWriter( writer );
}

static void test_WsSetHeader(void)
{
    static const char expected[] =
        "<s:Envelope xmlns:a=\"http://www.w3.org/2005/08/addressing\" "
        "xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Header>"
        "<a:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</a:MessageID>"
        "<a:Action s:mustUnderstand=\"1\">action</a:Action></s:Header>"
        "<s:Body/></s:Envelope>";
   static const char expected2[] =
        "<s:Envelope xmlns:a=\"http://www.w3.org/2005/08/addressing\" "
        "xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Header>"
        "<a:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</a:MessageID>"
        "<a:Action s:mustUnderstand=\"1\">action2</a:Action></s:Header>"
        "<s:Body/></s:Envelope>";
    static const WS_XML_STRING action2 = {7, (BYTE *)"action2"};
    HRESULT hr;
    WS_MESSAGE *msg;
    WS_XML_WRITER *writer;
    const WCHAR *ptr = L"action";

    hr = WsSetHeader( NULL, 0, 0, 0, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_2, WS_ADDRESSING_VERSION_1_0, NULL, 0, &msg,
                          NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetHeader( msg, 0, 0, 0, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsSetHeader( msg, WS_ACTION_HEADER, WS_WSZ_TYPE, WS_WRITE_REQUIRED_POINTER, &ptr,
                      sizeof(ptr), NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetHeader( msg, 0, WS_WSZ_TYPE, WS_WRITE_REQUIRED_POINTER, &ptr, sizeof(ptr), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsSetHeader( msg, WS_ACTION_HEADER, WS_WSZ_TYPE, WS_WRITE_REQUIRED_POINTER, &ptr,
                      sizeof(ptr), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_header( msg, expected, -1, strstr(expected, "urn:uuid:") - expected, 46, __LINE__ );

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEnvelopeStart( msg, writer, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, expected, 250, strstr(expected, "urn:uuid:") - expected, 46, __LINE__ );
    check_output_header( msg, expected, -1, strstr(expected, "urn:uuid:") - expected, 46, __LINE__ );

    /* called after WsWriteEnvelopeStart */
    hr = WsSetHeader( msg, WS_ACTION_HEADER, WS_XML_STRING_TYPE, WS_WRITE_REQUIRED_VALUE, &action2,
                      sizeof(action2), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, expected, 250, strstr(expected, "urn:uuid:") - expected, 46, __LINE__ );
    check_output_header( msg, expected2, -1, strstr(expected2, "urn:uuid:") - expected2, 46, __LINE__ );

    WsFreeMessage( msg );
    WsFreeWriter( writer );
}

static void test_WsRemoveHeader(void)
{
    static const char expected[] =
        "<s:Envelope xmlns:a=\"http://www.w3.org/2005/08/addressing\" "
        "xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Header>"
        "<a:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</a:MessageID>"
        "<a:Action s:mustUnderstand=\"1\">action</a:Action></s:Header>"
        "<s:Body/></s:Envelope>";
   static const char expected2[] =
        "<s:Envelope xmlns:a=\"http://www.w3.org/2005/08/addressing\" "
        "xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Header>"
        "<a:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</a:MessageID>"
        "</s:Header><s:Body/></s:Envelope>";
    static const WS_XML_STRING action = {6, (BYTE *)"action"};
    HRESULT hr;
    WS_MESSAGE *msg;

    hr = WsSetHeader( NULL, 0, 0, 0, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_2, WS_ADDRESSING_VERSION_1_0, NULL, 0, &msg,
                          NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsRemoveHeader( NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsRemoveHeader( msg, 0, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsRemoveHeader( msg, WS_ACTION_HEADER, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_header( msg, expected2, -1, strstr(expected2, "urn:uuid:") - expected2, 46, __LINE__ );

    hr = WsRemoveHeader( msg, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsRemoveHeader( msg, WS_ACTION_HEADER, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_header( msg, expected2, -1, strstr(expected2, "urn:uuid:") - expected2, 46, __LINE__ );

    hr = WsSetHeader( msg, WS_ACTION_HEADER, WS_XML_STRING_TYPE, WS_WRITE_REQUIRED_VALUE, &action,
                      sizeof(action), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_header( msg, expected, -1, strstr(expected, "urn:uuid:") - expected, 46, __LINE__ );

    hr = WsRemoveHeader( msg, WS_ACTION_HEADER, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_header( msg, expected2, -1, strstr(expected2, "urn:uuid:") - expected2, 46, __LINE__ );

    /* again */
    hr = WsRemoveHeader( msg, WS_ACTION_HEADER, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_header( msg, expected2, -1, strstr(expected2, "urn:uuid:") - expected2, 46, __LINE__ );

    WsFreeMessage( msg );
}

static void test_WsAddMappedHeader(void)
{
    static const WS_XML_STRING header = {6, (BYTE *)"Header"}, value = {5, (BYTE *)"value"};
    WS_MESSAGE *msg;
    HRESULT hr;

    hr = WsAddMappedHeader( NULL, NULL, 0, 0, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_2, WS_ADDRESSING_VERSION_1_0, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsAddMappedHeader( msg, NULL, 0, 0, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsAddMappedHeader( msg, &header, 0, 0, NULL, 0, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsAddMappedHeader( msg, &header, WS_XML_STRING_TYPE, WS_WRITE_REQUIRED_VALUE, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsAddMappedHeader( msg, &header, WS_XML_STRING_TYPE, WS_WRITE_REQUIRED_VALUE, &value, sizeof(value), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* again */
    hr = WsAddMappedHeader( msg, &header, WS_XML_STRING_TYPE, WS_WRITE_REQUIRED_VALUE, &value, sizeof(value), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    WsFreeMessage( msg );
}

static void test_WsRemoveMappedHeader(void)
{
    static const WS_XML_STRING header = {6, (BYTE *)"Header"}, value = {5, (BYTE *)"value"};
    WS_MESSAGE *msg;
    HRESULT hr;

    hr = WsRemoveMappedHeader( NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_2, WS_ADDRESSING_VERSION_1_0, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsRemoveMappedHeader( msg, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsRemoveMappedHeader( msg, &header, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsAddMappedHeader( msg, &header, WS_XML_STRING_TYPE, WS_WRITE_REQUIRED_VALUE, &value, sizeof(value), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsRemoveMappedHeader( msg, &header, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* again */
    hr = WsRemoveMappedHeader( msg, &header, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    WsFreeMessage( msg );
}

static void test_WsAddCustomHeader(void)
{
   static const char expected[] =
        "<s:Envelope xmlns:a=\"http://www.w3.org/2005/08/addressing\" "
        "xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Header>"
        "<a:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</a:MessageID>"
        "<header xmlns=\"ns\">value</header></s:Header><s:Body/></s:Envelope>";
   static const char expected2[] =
        "<s:Envelope xmlns:a=\"http://www.w3.org/2005/08/addressing\" "
        "xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Header>"
        "<a:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</a:MessageID>"
        "</s:Header><s:Body/></s:Envelope>";
    static const char expected3[] =
        "<s:Envelope xmlns:a=\"http://www.w3.org/2005/08/addressing\" "
        "xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Header>"
        "<a:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</a:MessageID>"
        "<header xmlns=\"ns\">value</header><header xmlns=\"ns\">value2</header>"
        "</s:Header><s:Body/></s:Envelope>";
    static const char expected4[] =
        "<s:Envelope xmlns:a=\"http://www.w3.org/2005/08/addressing\" "
        "xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Header>"
        "<a:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</a:MessageID>"
        "<header xmlns=\"ns\">value</header><header xmlns=\"ns\">value2</header>"
        "<header2 xmlns=\"ns\">value2</header2></s:Header><s:Body/></s:Envelope>";
    static WS_XML_STRING header = {6, (BYTE *)"header"}, ns = {2, (BYTE *)"ns"};
    static WS_XML_STRING header2 = {7, (BYTE *)"header2"};
    HRESULT hr;
    WS_MESSAGE *msg;
    WS_ELEMENT_DESCRIPTION desc;
    WS_STRUCT_DESCRIPTION s;
    WS_FIELD_DESCRIPTION f, *fields[1];
    struct header
    {
        const WCHAR *value;
    } test;

    hr = WsAddCustomHeader( NULL, NULL, 0, NULL, 0, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_2, WS_ADDRESSING_VERSION_1_0, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsAddCustomHeader( msg, NULL, 0, NULL, 0, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    memset( &f, 0, sizeof(f) );
    f.mapping = WS_TEXT_FIELD_MAPPING;
    f.type    = WS_WSZ_TYPE;
    fields[0] = &f;

    memset( &s, 0, sizeof(s) );
    s.size       = sizeof(struct header);
    s.alignment  = TYPE_ALIGNMENT(struct header);
    s.fields     = fields;
    s.fieldCount = 1;

    desc.elementLocalName = &header;
    desc.elementNs        = &ns;
    desc.type             = WS_STRUCT_TYPE;
    desc.typeDescription  = &s;
    hr = WsAddCustomHeader( msg, &desc, 0, NULL, 0, 0, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_header( msg, expected2, -1, strstr(expected2, "urn:uuid:") - expected2, 46, __LINE__ );

    test.value = L"value";
    hr = WsAddCustomHeader( msg, &desc, WS_WRITE_REQUIRED_VALUE, &test, sizeof(test), 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_header( msg, expected, -1, strstr(expected, "urn:uuid:") - expected, 46, __LINE__ );

    test.value = L"value2";
    hr = WsAddCustomHeader( msg, &desc, WS_WRITE_REQUIRED_VALUE, &test, sizeof(test), 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_header( msg, expected3, -1, strstr(expected3, "urn:uuid:") - expected3, 46, __LINE__ );

    desc.elementLocalName = &header2;
    hr = WsAddCustomHeader( msg, &desc, WS_WRITE_REQUIRED_VALUE, &test, sizeof(test), 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_header( msg, expected4, -1, strstr(expected4, "urn:uuid:") - expected4, 46, __LINE__ );

    hr = WsAddCustomHeader( msg, &desc, WS_WRITE_REQUIRED_VALUE, NULL, 0, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    WsFreeMessage( msg );
}

static void test_WsRemoveCustomHeader(void)
{
   static const char expected[] =
        "<s:Envelope xmlns:a=\"http://www.w3.org/2005/08/addressing\" "
        "xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Header>"
        "<a:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</a:MessageID>"
        "<test xmlns=\"ns\">value</test></s:Header><s:Body/></s:Envelope>";
   static const char expected2[] =
        "<s:Envelope xmlns:a=\"http://www.w3.org/2005/08/addressing\" "
        "xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Header>"
        "<a:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</a:MessageID>"
        "</s:Header><s:Body/></s:Envelope>";
    static const char expected3[] =
        "<s:Envelope xmlns:a=\"http://www.w3.org/2005/08/addressing\" "
        "xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Header>"
        "<a:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</a:MessageID>"
        "<test xmlns=\"ns\">value</test><test xmlns=\"ns\">value2</test>"
        "</s:Header><s:Body/></s:Envelope>";
    static WS_XML_STRING localname = {4, (BYTE *)"test"}, ns = {2, (BYTE *)"ns"};
    static const WS_XML_STRING value = {5, (BYTE *)"value"};
    static const WS_XML_STRING value2 = {6, (BYTE *)"value2"};
    HRESULT hr;
    WS_MESSAGE *msg;
    WS_ELEMENT_DESCRIPTION desc;

    hr = WsRemoveCustomHeader( NULL, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_2, WS_ADDRESSING_VERSION_1_0, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsRemoveCustomHeader( msg, &localname, &ns, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_header( msg, expected2, -1, strstr(expected2, "urn:uuid:") - expected2, 46, __LINE__ );

    desc.elementLocalName = &localname;
    desc.elementNs        = &ns;
    desc.type             = WS_XML_STRING_TYPE;
    desc.typeDescription  = NULL;
    hr = WsAddCustomHeader( msg, &desc, WS_WRITE_REQUIRED_VALUE, &value, sizeof(value), 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_header( msg, expected, -1, strstr(expected, "urn:uuid:") - expected, 46, __LINE__ );

    hr = WsAddCustomHeader( msg, &desc, WS_WRITE_REQUIRED_VALUE, &value2, sizeof(value2), 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_header( msg, expected3, -1, strstr(expected3, "urn:uuid:") - expected3, 46, __LINE__ );

    hr = WsRemoveCustomHeader( msg, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsRemoveCustomHeader( msg, &localname, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsRemoveCustomHeader( msg, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_header( msg, expected2, -1, strstr(expected2, "urn:uuid:") - expected2, 46, __LINE__ );

    /* again */
    hr = WsRemoveCustomHeader( msg, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    WsFreeMessage( msg );
}

static HRESULT set_input( WS_XML_READER *reader, const char *data, ULONG size )
{
    WS_XML_READER_TEXT_ENCODING text = {{WS_XML_READER_ENCODING_TYPE_TEXT}, WS_CHARSET_AUTO};
    WS_XML_READER_BUFFER_INPUT buf;

    buf.input.inputType = WS_XML_READER_INPUT_TYPE_BUFFER;
    buf.encodedData     = (void *)data;
    buf.encodedDataSize = size;
    return WsSetInput( reader, &text.encoding, &buf.input, NULL, 0, NULL );
}

static void test_WsReadEnvelopeStart(void)
{
    static const char xml[] =
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body/></s:Envelope>";
    static const char faultxml[] =
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body>"
        "<s:Fault/></s:Body></s:Envelope>";
    WS_MESSAGE *msg, *msg2, *msg3;
    WS_XML_READER *reader;
    WS_MESSAGE_STATE state;
    const WS_XML_NODE *node;
    BOOL isfault;
    HRESULT hr;

    hr = WsReadEnvelopeStart( NULL, NULL, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_1, WS_ADDRESSING_VERSION_0_9, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadEnvelopeStart( msg, NULL, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadEnvelopeStart( msg, reader, NULL, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_1, WS_ADDRESSING_VERSION_0_9, NULL, 0, &msg2, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_input( reader, xml, strlen(xml) );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadEnvelopeStart( msg2, reader, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    state = 0xdeadbeef;
    hr = WsGetMessageProperty( msg2, WS_MESSAGE_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( state == WS_MESSAGE_STATE_READING, "got %u\n", state );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadEndElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadEndElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_1, WS_ADDRESSING_VERSION_0_9, NULL, 0, &msg3, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_input( reader, faultxml, strlen(faultxml) );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadEnvelopeStart( msg3, reader, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    isfault = FALSE;
    hr = WsGetMessageProperty( msg3, WS_MESSAGE_PROPERTY_IS_FAULT, &isfault, sizeof(isfault), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( isfault, "isfault == FALSE\n" );

    WsFreeMessage( msg );
    WsFreeMessage( msg2 );
    WsFreeMessage( msg3 );
    WsFreeReader( reader );
}

static void test_WsReadEnvelopeEnd(void)
{
    static const char xml[] =
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body></s:Body></s:Envelope>";
    WS_MESSAGE *msg, *msg2;
    WS_XML_READER *reader;
    WS_MESSAGE_STATE state;
    HRESULT hr;

    hr = WsReadEnvelopeEnd( NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_1, WS_ADDRESSING_VERSION_0_9, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_1, WS_ADDRESSING_VERSION_0_9, NULL, 0, &msg2, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_input( reader, xml, strlen(xml) );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadEnvelopeEnd( msg2, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsReadEnvelopeStart( msg2, reader, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadEnvelopeEnd( msg2, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    state = 0xdeadbeef;
    hr = WsGetMessageProperty( msg2, WS_MESSAGE_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( state == WS_MESSAGE_STATE_DONE, "got %u\n", state );

    WsFreeMessage( msg );
    WsFreeMessage( msg2 );
    WsFreeReader( reader );
}

static void test_WsReadBody(void)
{
    static const char xml[] =
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body>"
        "<u xmlns=\"ns\"><val>1</val></u></s:Body></s:Envelope>";
    static const char faultxml[] =
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body>"
        "<s:Fault><faultcode>s:Client</faultcode><faultstring>OLS Exception</faultstring></s:Fault>"
        "</s:Body></s:Envelope>";
    static const WS_XML_STRING faultcode = WS_XML_STRING_VALUE( "Client" );
    static const WS_STRING faultstring = WS_STRING_VALUE( L"OLS Exception" );
    WS_HEAP *heap;
    WS_MESSAGE *msg, *msg2, *msg3;
    WS_XML_READER *reader;
    WS_MESSAGE_STATE state;
    BOOL isfault;
    WS_XML_STRING localname = {1, (BYTE *)"t"}, localname2 = {1, (BYTE *)"u"};
    WS_XML_STRING val = {3, (BYTE *)"val"}, ns = {2, (BYTE *)"ns"};
    WS_ELEMENT_DESCRIPTION desc, faultdesc;
    WS_STRUCT_DESCRIPTION s;
    WS_FIELD_DESCRIPTION f, *fields[1];
    struct test
    {
        UINT32 val;
    } test;
    WS_FAULT fault;
    HRESULT hr;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadBody( NULL, NULL, 0, NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_1, WS_ADDRESSING_VERSION_0_9, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_1, WS_ADDRESSING_VERSION_0_9, NULL, 0, &msg2, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadBody( msg2, NULL, 0, NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = set_input( reader, xml, strlen(xml) );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadEnvelopeStart( msg2, reader, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadBody( msg2, NULL, WS_READ_REQUIRED_VALUE, heap, &test, sizeof(test), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    memset( &f, 0, sizeof(f) );
    f.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f.localName = &val;
    f.ns        = &ns;
    f.type      = WS_UINT32_TYPE;
    fields[0] = &f;

    memset( &s, 0, sizeof(s) );
    s.size          = sizeof(struct test);
    s.alignment     = TYPE_ALIGNMENT(struct test);
    s.fields        = fields;
    s.fieldCount    = 1;
    s.typeLocalName = &localname;
    s.typeNs        = &ns;

    desc.elementLocalName = &localname2;
    desc.elementNs        = &ns;
    desc.type             = WS_STRUCT_TYPE;
    desc.typeDescription  = &s;

    memset( &test, 0, sizeof(test) );
    hr = WsReadBody( msg2, &desc, WS_READ_REQUIRED_VALUE, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test.val == 1, "got %u\n", test.val );

    state = 0xdeadbeef;
    hr = WsGetMessageProperty( msg2, WS_MESSAGE_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( state == WS_MESSAGE_STATE_READING, "got %u\n", state );

    hr = WsReadEnvelopeEnd( msg2, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_1, WS_ADDRESSING_VERSION_0_9, NULL, 0, &msg3, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_input( reader, faultxml, strlen(faultxml) );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadEnvelopeStart( msg3, reader, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    isfault = FALSE;
    hr = WsGetMessageProperty( msg3, WS_MESSAGE_PROPERTY_IS_FAULT, &isfault, sizeof(isfault), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( isfault, "isfault == FALSE\n" );

    faultdesc.elementLocalName = NULL;
    faultdesc.elementNs        = NULL;
    faultdesc.type             = WS_FAULT_TYPE;
    faultdesc.typeDescription  = NULL;

    memset( &fault, 0, sizeof(fault) );
    hr = WsReadBody( msg3, &faultdesc, WS_READ_REQUIRED_VALUE, heap, &fault, sizeof(fault), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( fault.code->value.localName.length == faultcode.length, "got %lu\n", fault.code->value.localName.length );
    ok( !memcmp( fault.code->value.localName.bytes, faultcode.bytes, faultcode.length ), "wrong fault code\n" );
    ok( !fault.code->subCode, "subcode is not NULL\n" );
    ok( fault.reasonCount == 1, "got %lu\n", fault.reasonCount );
    ok( fault.reasons[0].text.length == faultstring.length, "got %lu\n", fault.reasons[0].text.length );
    ok( !memcmp( fault.reasons[0].text.chars, faultstring.chars, faultstring.length * sizeof(WCHAR) ),
        "wrong fault string\n" );

    hr = WsReadEnvelopeEnd( msg3, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );


    WsFreeMessage( msg );
    WsFreeMessage( msg2 );
    WsFreeMessage( msg3 );
    WsFreeReader( reader );
    WsFreeHeap( heap );
}

static void test_WsResetMessage(void)
{
    WS_MESSAGE *msg;
    WS_MESSAGE_STATE state;
    WS_ENVELOPE_VERSION env_version;
    WS_ADDRESSING_VERSION addr_version;
    HRESULT hr;

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_1, WS_ADDRESSING_VERSION_0_9, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    state = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( state == WS_MESSAGE_STATE_EMPTY, "got %u\n", state );

    hr = WsInitializeMessage( msg,  WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    state = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( state == WS_MESSAGE_STATE_INITIALIZED, "got %u\n", state );

    hr = WsResetMessage( msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    state = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( state == WS_MESSAGE_STATE_EMPTY, "got %u\n", state );

    env_version = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_ENVELOPE_VERSION, &env_version,
                               sizeof(env_version), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( env_version == WS_ENVELOPE_VERSION_SOAP_1_1, "got %u\n", env_version );

    addr_version = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_ADDRESSING_VERSION, &addr_version,
                               sizeof(addr_version), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( addr_version == WS_ADDRESSING_VERSION_0_9, "got %u\n", addr_version );

    WsFreeMessage( msg );
}

static void test_WsGetHeader(void)
{
    static char expected[] =
        "<s:Envelope xmlns:a=\"http://www.w3.org/2005/08/addressing\" "
        "xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Header>"
        "<a:MessageID>urn:uuid:00000000-0000-0000-0000-000000000000</a:MessageID>"
        "<a:Action s:mustUnderstand=\"1\">action</a:Action></s:Header><s:Body/></s:Envelope>";
    static char expected2[] =
        "<Envelope><Header><Action mustUnderstand=\"1\" "
        "xmlns=\"http://schemas.microsoft.com/ws/2005/05/addressing/none\">action</Action>"
        "</Header><Body/></Envelope>";
    static char expected3[] =
        "<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Header>"
        "<Action s:mustUnderstand=\"1\" "
        "xmlns=\"http://schemas.microsoft.com/ws/2005/05/addressing/none\">action</Action>"
        "</s:Header><s:Body/></s:Envelope>";
    WS_MESSAGE *msg;
    const WCHAR *ptr;
    HRESULT hr;

    hr = WsGetHeader( NULL, 0, 0, 0, NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_2, WS_ADDRESSING_VERSION_1_0, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetHeader( msg, 0, 0, 0, NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsInitializeMessage( msg,  WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetHeader( msg, 0, 0, 0, NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsGetHeader( msg, WS_ACTION_HEADER, 0, 0, NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsGetHeader( msg, WS_ACTION_HEADER, WS_WSZ_TYPE, 0, NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsGetHeader( msg, WS_ACTION_HEADER, WS_WSZ_TYPE, WS_READ_NILLABLE_POINTER, NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsGetHeader( msg, WS_ACTION_HEADER, WS_WSZ_TYPE, WS_READ_REQUIRED_POINTER, NULL, NULL, 0, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    ptr = L"action";
    hr = WsSetHeader( msg, WS_ACTION_HEADER, WS_WSZ_TYPE, WS_WRITE_REQUIRED_POINTER, &ptr, sizeof(ptr), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_header( msg, expected, -1, strstr(expected, "urn:uuid:") - expected, 46, __LINE__ );

    hr = WsGetHeader( msg, WS_ACTION_HEADER, WS_WSZ_TYPE, WS_READ_REQUIRED_POINTER, NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    ptr = NULL;
    hr = WsGetHeader( msg, WS_ACTION_HEADER, WS_WSZ_TYPE, WS_READ_REQUIRED_POINTER, NULL, &ptr, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    ptr = NULL;
    hr = WsGetHeader( msg, WS_ACTION_HEADER, WS_WSZ_TYPE, WS_READ_REQUIRED_POINTER, NULL, &ptr, sizeof(ptr), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( ptr != NULL, "ptr not set\n" );
    ok( !memcmp( ptr, L"action", sizeof(L"action") ), "wrong data\n" );
    WsFreeMessage( msg );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_NONE, WS_ADDRESSING_VERSION_TRANSPORT, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsInitializeMessage( msg,  WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    ptr = L"action";
    hr = WsSetHeader( msg, WS_ACTION_HEADER, WS_WSZ_TYPE, WS_WRITE_REQUIRED_POINTER, &ptr, sizeof(ptr), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (hr == S_OK) check_output_header( msg, expected2, -1, 0, 0, __LINE__ );

    ptr = NULL;
    hr = WsGetHeader( msg, WS_ACTION_HEADER, WS_WSZ_TYPE, WS_READ_REQUIRED_POINTER, NULL, &ptr, sizeof(ptr), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( ptr != NULL, "ptr not set\n" );
    ok( !memcmp( ptr, L"action", sizeof(L"action") ), "wrong data\n" );
    WsFreeMessage( msg );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_2, WS_ADDRESSING_VERSION_TRANSPORT, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsInitializeMessage( msg,  WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    ptr = L"action";
    hr = WsSetHeader( msg, WS_ACTION_HEADER, WS_WSZ_TYPE, WS_WRITE_REQUIRED_POINTER, &ptr, sizeof(ptr), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (hr == S_OK) check_output_header( msg, expected3, -1, 0, 0, __LINE__ );

    ptr = NULL;
    hr = WsGetHeader( msg, WS_ACTION_HEADER, WS_WSZ_TYPE, WS_READ_REQUIRED_POINTER, NULL, &ptr, sizeof(ptr), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( ptr != NULL, "ptr not set\n" );
    ok( !memcmp( ptr, L"action", sizeof(L"action") ), "wrong data\n" );
    WsFreeMessage( msg );
}

static void test_WsGetCustomHeader(void)
{
    static char expected[] =
        "<Envelope><Header><Custom xmlns=\"ns\">value</Custom></Header><Body/></Envelope>";
    static WS_XML_STRING custom = {6, (BYTE *)"Custom"}, ns = {2, (BYTE *)"ns"};
    WS_ELEMENT_DESCRIPTION desc;
    WS_STRUCT_DESCRIPTION s;
    WS_FIELD_DESCRIPTION f, *fields[1];
    WS_MESSAGE *msg;
    HRESULT hr;
    struct header
    {
        const WCHAR *value;
    } test;

    hr = WsGetCustomHeader( NULL, NULL, 0, 0, 0, NULL, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateMessage( WS_ENVELOPE_VERSION_NONE, WS_ADDRESSING_VERSION_TRANSPORT, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetCustomHeader( msg, NULL, 0, 0, 0, NULL, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    memset( &f, 0, sizeof(f) );
    f.mapping = WS_TEXT_FIELD_MAPPING;
    f.type    = WS_WSZ_TYPE;
    fields[0] = &f;

    memset( &s, 0, sizeof(s) );
    s.size       = sizeof(struct header);
    s.alignment  = TYPE_ALIGNMENT(struct header);
    s.fields     = fields;
    s.fieldCount = 1;

    desc.elementLocalName = &custom;
    desc.elementNs        = &ns;
    desc.type             = WS_STRUCT_TYPE;
    desc.typeDescription  = &s;

    test.value = L"value";
    hr = WsAddCustomHeader( msg, &desc, WS_WRITE_REQUIRED_VALUE, &test, sizeof(test), 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_header( msg, expected, -1, 0, 0, __LINE__ );

    hr = WsGetCustomHeader( msg, &desc, 0, 0, 0, NULL, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsGetCustomHeader( msg, &desc, WS_SINGLETON_HEADER, 1, 0, NULL, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsGetCustomHeader( msg, &desc, WS_SINGLETON_HEADER, 0, WS_READ_REQUIRED_VALUE, NULL, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    memset( &test, 0, sizeof(test) );
    hr = WsGetCustomHeader( msg, &desc, WS_SINGLETON_HEADER, 0, WS_READ_REQUIRED_VALUE, NULL, &test, sizeof(test),
                            NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test.value != NULL, "value not set\n" );
    ok( !memcmp( test.value, L"value", sizeof(L"value") ), "wrong value\n" );
    WsFreeMessage( msg );
}

START_TEST(msg)
{
    test_WsCreateMessage();
    test_WsCreateMessageForChannel();
    test_WsInitializeMessage();
    test_WsAddressMessage();
    test_WsWriteEnvelopeStart();
    test_WsWriteEnvelopeEnd();
    test_WsWriteBody();
    test_WsSetHeader();
    test_WsRemoveHeader();
    test_WsAddMappedHeader();
    test_WsRemoveMappedHeader();
    test_WsAddCustomHeader();
    test_WsRemoveCustomHeader();
    test_WsReadEnvelopeStart();
    test_WsReadEnvelopeEnd();
    test_WsReadBody();
    test_WsResetMessage();
    test_WsGetHeader();
    test_WsGetCustomHeader();
}
