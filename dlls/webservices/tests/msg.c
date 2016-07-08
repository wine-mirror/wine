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
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateMessage( 0, 0, NULL, 0, &msg, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateMessage( WS_ADDRESSING_VERSION_0_9, 0, NULL, 0, &msg, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateMessage( 0, WS_ENVELOPE_VERSION_SOAP_1_1, NULL, 0, &msg, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    env_version = WS_ENVELOPE_VERSION_SOAP_1_1;
    prop.id = WS_MESSAGE_PROPERTY_ENVELOPE_VERSION;
    prop.value = &env_version;
    prop.valueSize = sizeof(env_version);
    hr = WsCreateMessage( WS_ENVELOPE_VERSION_SOAP_1_1, WS_ADDRESSING_VERSION_0_9, &prop,
                          1, &msg, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateMessage( WS_ADDRESSING_VERSION_0_9, WS_ENVELOPE_VERSION_SOAP_1_1, NULL,
                          0, &msg, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    state = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( state == WS_MESSAGE_STATE_EMPTY, "got %u\n", state );

    env_version = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_ENVELOPE_VERSION, &env_version,
                               sizeof(env_version), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( env_version == WS_ENVELOPE_VERSION_SOAP_1_1, "got %u\n", env_version );

    addr_version = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_ADDRESSING_VERSION, &addr_version,
                               sizeof(addr_version), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( addr_version == WS_ADDRESSING_VERSION_0_9, "got %u\n", addr_version );

    state = WS_MESSAGE_STATE_EMPTY;
    hr = WsSetMessageProperty( msg, WS_MESSAGE_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );

    env_version = WS_ENVELOPE_VERSION_SOAP_1_1;
    hr = WsSetMessageProperty( msg, WS_MESSAGE_PROPERTY_ENVELOPE_VERSION, &env_version,
                               sizeof(env_version), NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );

    addr_version = WS_ADDRESSING_VERSION_0_9;
    hr = WsSetMessageProperty( msg, WS_MESSAGE_PROPERTY_ADDRESSING_VERSION, &addr_version,
                               sizeof(addr_version), NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );
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
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateMessageForChannel( NULL, NULL, 0, &msg, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateChannel( WS_CHANNEL_TYPE_REQUEST, WS_HTTP_CHANNEL_BINDING, NULL, 0, NULL,
                          &channel, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    state = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( state == WS_MESSAGE_STATE_EMPTY, "got %u\n", state );

    env_version = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_ENVELOPE_VERSION, &env_version,
                               sizeof(env_version), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( env_version == WS_ENVELOPE_VERSION_SOAP_1_2, "got %u\n", env_version );

    addr_version = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_ADDRESSING_VERSION, &addr_version,
                               sizeof(addr_version), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( addr_version == WS_ADDRESSING_VERSION_1_0, "got %u\n", addr_version );

    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_IS_ADDRESSED, &addressed, sizeof(addressed),
                               NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );

    WsFreeChannel( channel );
    WsFreeMessage( msg );

    env_version = WS_ENVELOPE_VERSION_SOAP_1_1;
    prop.id = WS_CHANNEL_PROPERTY_ENVELOPE_VERSION;
    prop.value = &env_version;
    prop.valueSize = sizeof(env_version);
    hr = WsCreateChannel( WS_CHANNEL_TYPE_REQUEST, WS_HTTP_CHANNEL_BINDING, &prop, 1, NULL,
                          &channel, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    env_version = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_ENVELOPE_VERSION, &env_version,
                               sizeof(env_version), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( env_version == WS_ENVELOPE_VERSION_SOAP_1_1, "got %u\n", env_version );

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

    return;
    hr = WsInitializeMessage( NULL, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateMessage( WS_ADDRESSING_VERSION_0_9, WS_ENVELOPE_VERSION_SOAP_1_1, NULL,
                          0, &msg, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsInitializeMessage( msg, 0xdeadbeef, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );

    state = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( state == WS_MESSAGE_STATE_INITIALIZED, "got %u\n", state );

    addressed = -1;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_IS_ADDRESSED, &addressed, sizeof(addressed),
                               NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !addressed, "unexpected value %d\n", addressed );

    state = WS_MESSAGE_STATE_EMPTY;
    hr = WsSetMessageProperty( msg, WS_MESSAGE_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    env_version = WS_ENVELOPE_VERSION_SOAP_1_1;
    hr = WsSetMessageProperty( msg, WS_MESSAGE_PROPERTY_ENVELOPE_VERSION, &env_version,
                               sizeof(env_version), NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    addr_version = WS_ADDRESSING_VERSION_0_9;
    hr = WsSetMessageProperty( msg, WS_MESSAGE_PROPERTY_ADDRESSING_VERSION, &addr_version,
                               sizeof(addr_version), NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );
    WsFreeMessage( msg );
}

static void test_WsAddressMessage(void)
{
    static WCHAR localhost[] = {'h','t','t','p',':','/','/','l','o','c','a','l','h','o','s','t','/',0};
    HRESULT hr;
    WS_MESSAGE *msg;
    WS_ENDPOINT_ADDRESS endpoint;
    BOOL addressed;

    hr = WsAddressMessage( NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateMessage( WS_ADDRESSING_VERSION_0_9, WS_ENVELOPE_VERSION_SOAP_1_1, NULL,
                          0, &msg, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_IS_ADDRESSED, &addressed, sizeof(addressed),
                               NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );

    hr = WsAddressMessage( msg, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );

    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    addressed = -1;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_IS_ADDRESSED, &addressed, sizeof(addressed),
                               NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !addressed, "unexpected value %d\n", addressed );

    hr = WsAddressMessage( msg, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    addressed = -1;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_IS_ADDRESSED, &addressed, sizeof(addressed),
                               NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( addressed == TRUE, "unexpected value %d\n", addressed );
    WsFreeMessage( msg );

    hr = WsCreateMessage( WS_ADDRESSING_VERSION_0_9, WS_ENVELOPE_VERSION_SOAP_1_1, NULL,
                          0, &msg, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    memset( &endpoint, 0, sizeof(endpoint) );
    endpoint.url.chars  = localhost;
    endpoint.url.length = sizeof(localhost)/sizeof(localhost[0]);
    hr = WsAddressMessage( msg, &endpoint, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    addressed = -1;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_IS_ADDRESSED, &addressed, sizeof(addressed),
                               NULL );
    ok( hr == S_OK, "got %08x\n", hr );
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
    ok( hr == S_OK, "%u: got %08x\n", line, hr );
    ok( bytes.length == len, "%u: got %u expected %u\n", line, bytes.length, len );
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
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_HEADER_BUFFER, &buf, sizeof(buf), NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteXmlBuffer( writer, buf, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

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
    HRESULT hr;
    WS_MESSAGE *msg;
    WS_XML_WRITER *writer;
    WS_MESSAGE_STATE state;

    hr = WsWriteEnvelopeStart( NULL, NULL, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteEnvelopeStart( NULL, writer, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateMessage( WS_ADDRESSING_VERSION_0_9, WS_ENVELOPE_VERSION_SOAP_1_1, NULL, 0, &msg,
                          NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteEnvelopeStart( msg, writer, NULL, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );

    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteEnvelopeStart( msg, writer, NULL, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, expected, -1, strstr(expected, "urn:uuid:") - expected, 46, __LINE__ );
    check_output_header( msg, expected2, -1, strstr(expected2, "urn:uuid:") - expected2, 46, __LINE__ );

    state = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( state == WS_MESSAGE_STATE_WRITING, "got %u\n", state );

    WsFreeMessage( msg );
    WsFreeWriter( writer );
}

START_TEST(msg)
{
    test_WsCreateMessage();
    test_WsCreateMessageForChannel();
    test_WsInitializeMessage();
    test_WsAddressMessage();
    test_WsWriteEnvelopeStart();
}
