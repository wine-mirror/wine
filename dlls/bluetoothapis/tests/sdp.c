/* Tests for bluetoothapis.dll's SDP API
 *
 * Copyright 2024 Vibhav Pant
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
 *
 */

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>

#include "bthsdpdef.h"
#include "bluetoothapis.h"

#include "wine/test.h"

static const char *debugstr_SDP_ELEMENT_DATA( const SDP_ELEMENT_DATA *data )
{
    return wine_dbg_sprintf( "{%#x %#x {%I64u %I64u}}", data->type, data->specificType,
                             data->data.uint128.LowPart, data->data.uint128.HighPart );
}

/* Returns the exact number of bytes we need to compare to test equality between the 'data' fields of two
 * SDP_ELEMENT_DATA. */
static SIZE_T sdp_type_size( SDP_TYPE type, SDP_SPECIFICTYPE st )
{

    switch (type)
    {
    case SDP_TYPE_NIL:
        return 0;
    case SDP_TYPE_BOOLEAN:
        return sizeof( UCHAR );
    case SDP_TYPE_INT:
    case SDP_TYPE_UINT:
    case SDP_TYPE_UUID:
        return 1 << ((st >> 8) & 0x7);
    default: /* Container/Sequence-like types. */
        return sizeof( BYTE * ) + sizeof( ULONG );
    }
}

static void test_BluetoothSdpGetElementData( BYTE *stream, SIZE_T size, DWORD error,
                                             const SDP_ELEMENT_DATA *sdp_data )
{
    DWORD ret;
    SDP_ELEMENT_DATA result = {0};

    ret = BluetoothSdpGetElementData( stream, size, &result );
    ok( ret == error, "%ld != %ld.\n", error, ret );
    if (ret == error && error == ERROR_SUCCESS)
    {
        ok( result.type == sdp_data->type, "%#x != %#x.\n", sdp_data->type, result.type );
        ok( result.specificType == sdp_data->specificType,
            "%#x != %#x.\n", sdp_data->specificType,
            result.specificType );
        ok( !memcmp( &sdp_data->data, &result.data, sdp_type_size( result.type, result.specificType ) ),
            "%s != %s.\n", debugstr_SDP_ELEMENT_DATA( sdp_data ),
            debugstr_SDP_ELEMENT_DATA( &result ) );
    }
}

static void test_BluetoothSdpGetElementData_invalid( void )
{
    SDP_ELEMENT_DATA data;
    BYTE stream[] = {0};
    DWORD ret;

    ret = BluetoothSdpGetElementData( NULL, 10, &data );
    ok( ret == ERROR_INVALID_PARAMETER, "%d != %ld.\n", ERROR_INVALID_PARAMETER, ret );

    ret = BluetoothSdpGetElementData( stream, 1, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "%d != %ld.\n", ERROR_INVALID_PARAMETER, ret );

    ret = BluetoothSdpGetElementData( stream, 0, &data );
    ok( ret == ERROR_INVALID_PARAMETER, "%d != %ld.\n", ERROR_INVALID_PARAMETER, ret );

    ret = BluetoothSdpGetElementData( NULL, 0, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "%d != %ld.\n", ERROR_INVALID_PARAMETER, ret );
}

static void test_BluetoothSdpGetElementData_nil( void )
{
    struct
    {
        BYTE data_elem;
        DWORD error;
        SDP_ELEMENT_DATA data;
    } test_cases[] = {
        {0x0, ERROR_SUCCESS, {.type = SDP_TYPE_NIL, .specificType = SDP_ST_NONE }},
        {0x1, ERROR_INVALID_PARAMETER},
        {0x3, ERROR_INVALID_PARAMETER},
        {0x4, ERROR_INVALID_PARAMETER},
    };
    SIZE_T i;

    for (i = 0; i < ARRAY_SIZE( test_cases ); i++)
    {
        winetest_push_context( "test_cases nil %d", (int)i );
        test_BluetoothSdpGetElementData( &test_cases[i].data_elem, 1, test_cases[i].error,
                                         &test_cases[i].data );
        winetest_pop_context();
    }
}

#define SDP_TYPE_DESC_INT8    (0x10)
#define SDP_TYPE_DESC_UINT8   (0x8)
#define SDP_TYPE_DESC_INT16   (0x11)
#define SDP_TYPE_DESC_UINT16  (0x9)
#define SDP_TYPE_DESC_INT32   (0x12)
#define SDP_TYPE_DESC_UINT32  (0xa)
#define SDP_TYPE_DESC_INT64   (0x13)
#define SDP_TYPE_DESC_UINT64  (0xb)
#define SDP_TYPE_DESC_INT128  (0x14)
#define SDP_TYPE_DESC_UINT128 (0xc)

#define SDP_TYPE_DESC_STR8  (0x25)
#define SDP_TYPE_DESC_STR16 (0x26)
#define SDP_TYPE_DESC_STR32 (0x27)

#define SDP_TYPE_DESC_SEQ8  (0x35)
#define SDP_TYPE_DESC_SEQ16 (0x36)
#define SDP_TYPE_DESC_SEQ32 (0x37)

static void test_BluetoothSdpGetElementData_ints( void )
{
    struct
    {
        BYTE data_elem[17];
        SIZE_T size;
        DWORD error;
        SDP_ELEMENT_DATA data;
    } test_cases[] = {
        {
            {SDP_TYPE_DESC_INT8, 0xde},
            2,
            ERROR_SUCCESS,
            {SDP_TYPE_INT, SDP_ST_INT8, {.int8 = 0xde}}
        },
        {
            {SDP_TYPE_DESC_UINT8, 0xde},
            2,
            ERROR_SUCCESS,
            {SDP_TYPE_UINT, SDP_ST_UINT8, {.uint8 = 0xde}},
        },
        {
            {SDP_TYPE_DESC_INT16, 0xde, 0xad},
            3,
            ERROR_SUCCESS,
            {SDP_TYPE_INT, SDP_ST_INT16, {.int16 = 0xdead}},
        },
        {
            {SDP_TYPE_DESC_UINT16, 0xde, 0xad },
            3,
            ERROR_SUCCESS,
            {SDP_TYPE_UINT, SDP_ST_UINT16, {.uint16 = 0xdead}},
        },
        {
            {SDP_TYPE_DESC_INT32, 0xde, 0xad, 0xbe, 0xef},
            5,
            ERROR_SUCCESS,
            {SDP_TYPE_INT, SDP_ST_INT32, {.int32 = 0xdeadbeef}},
        },
        {
            {SDP_TYPE_DESC_UINT32, 0xde, 0xad, 0xbe, 0xef},
            5,
            ERROR_SUCCESS,
            {SDP_TYPE_UINT, SDP_ST_UINT32, {.uint32 = 0xdeadbeef}},
        },
        {
            {SDP_TYPE_DESC_INT64, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef},
            9,
            ERROR_SUCCESS,
            {SDP_TYPE_INT, SDP_ST_INT64, {.int64 = 0xdeadbeefdeadbeef}},
        },
        {
            {SDP_TYPE_DESC_UINT64, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef},
            9,
            ERROR_SUCCESS,
            {SDP_TYPE_UINT, SDP_ST_UINT64, {.uint64 = 0xdeadbeefdeadbeef}},
        },
        {
            {SDP_TYPE_DESC_INT64, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef},
            9,
            ERROR_SUCCESS,
            {SDP_TYPE_INT, SDP_ST_INT64, {.int64 = 0xdeadbeefdeadbeef}},
        },
        {
            {SDP_TYPE_DESC_UINT64, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef},
            9,
            ERROR_SUCCESS,
            {SDP_TYPE_UINT, SDP_ST_UINT64, {.uint64 = 0xdeadbeefdeadbeef}},
        },
        {
            {SDP_TYPE_DESC_INT128, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef },
            17,
            ERROR_SUCCESS,
            {SDP_TYPE_INT, SDP_ST_INT128, {.int128 = {0xdeadbeefdeadbeef, 0xdeadbeefdeadbeef}}},
        },
        {
            {SDP_TYPE_DESC_UINT128, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef},
            17,
            ERROR_SUCCESS,
            {SDP_TYPE_UINT, SDP_ST_UINT128, {.uint128 = {0xdeadbeefdeadbeef, 0xdeadbeefdeadbeef}}},
        }
    };
    SIZE_T i;

    for (i = 0; i < ARRAY_SIZE( test_cases ); i++)
    {
        winetest_push_context( "test_cases int %d", (int)i );
        test_BluetoothSdpGetElementData( test_cases[i].data_elem, test_cases[i].size,
                                         test_cases[i].error, &test_cases[i].data );
        winetest_pop_context();
    }
}

static void test_BluetoothSdpGetElementData_str( void )
{
    struct {
        BYTE stream[11];
        SIZE_T size;
        DWORD error;
        SDP_ELEMENT_DATA data;
        const char *string;
    } test_cases[] = {
        {
            {SDP_TYPE_DESC_STR8, 0x06, 'f', 'o', 'o', 'b', 'a', 'r'},
            8,
            ERROR_SUCCESS,
            {SDP_TYPE_STRING, SDP_ST_NONE, {.string = {&test_cases[0].stream[2], 6}}},
            "foobar",
        },
        {
            {SDP_TYPE_DESC_STR16, 0x00, 0x06, 'f', 'o', 'o', 'b', 'a', 'r'},
            9,
            ERROR_SUCCESS,
            {SDP_TYPE_STRING, SDP_ST_NONE, {.string = {&test_cases[1].stream[3], 6}}},
            "foobar",
        },
        {
            {SDP_TYPE_DESC_STR32, 0x00, 0x00, 0x00, 0x06, 'f', 'o', 'o', 'b', 'a', 'r'},
            11,
            ERROR_SUCCESS,
            {SDP_TYPE_STRING, SDP_ST_NONE, {.string = {&test_cases[2].stream[5], 6}}},
            "foobar",
        }
    };
    SIZE_T i;

    for (i = 0; i < ARRAY_SIZE( test_cases ); i++)
    {
        winetest_push_context( "test_cases str %d", (int)i );
        test_BluetoothSdpGetElementData( test_cases[i].stream, test_cases[i].size,
                                         test_cases[i].error, &test_cases[i].data );
        if (test_cases[i].error == ERROR_SUCCESS)
        {
            SDP_ELEMENT_DATA result = {0};
            if (!BluetoothSdpGetElementData( test_cases[i].stream, test_cases[i].size, &result ))
            {
                ok( strlen( test_cases[i].string ) == result.data.string.length, "%s != %s.\n",
                    debugstr_a( test_cases[i].string ),
                    debugstr_an( (const char *)result.data.string.value,
                                 result.data.string.length ) );
                ok( !memcmp( result.data.string.value, test_cases[i].string,
                             result.data.string.length ),
                    "%s != %s.\n", debugstr_a( test_cases[i].string ),
                    debugstr_an( (const char *)result.data.string.value,
                                 result.data.string.length ) );
            }
        }
        winetest_pop_context();
    }
}

static void test_BluetoothSdpGetContainerElementData( void )
{
    struct {
        BYTE stream[11];
        SIZE_T size;
        DWORD error;
        SDP_ELEMENT_DATA data;
        SDP_ELEMENT_DATA sequence[6];
        SIZE_T container_size;
    } test_cases[] = {
        {
            {SDP_TYPE_DESC_SEQ8, 0x06, SDP_TYPE_DESC_UINT8, 0xde, SDP_TYPE_DESC_UINT8, 0xad, SDP_TYPE_DESC_UINT8, 0xbe},
            8,
            ERROR_SUCCESS,
            {SDP_TYPE_SEQUENCE, SDP_ST_NONE, {.sequence = {&test_cases[0].stream[0], 8}}},
            {
                {SDP_TYPE_UINT, SDP_ST_UINT8, {.uint8 = 0xde}},
                {SDP_TYPE_UINT, SDP_ST_UINT8, {.uint8 = 0xad}},
                {SDP_TYPE_UINT, SDP_ST_UINT8, {.uint8 = 0xbe}}
            },
            3
        },
        {
            {SDP_TYPE_DESC_SEQ16, 0x00, 0x06, SDP_TYPE_DESC_UINT8, 0xde, SDP_TYPE_DESC_UINT8, 0xad, SDP_TYPE_DESC_UINT8, 0xbe},
            9,
            ERROR_SUCCESS,
            {SDP_TYPE_SEQUENCE, SDP_ST_NONE, {.sequence = {&test_cases[1].stream[0], 9}}},
            {
                {SDP_TYPE_UINT, SDP_ST_UINT8, {.uint8 = 0xde}},
                {SDP_TYPE_UINT, SDP_ST_UINT8, {.uint8 = 0xad}},
                {SDP_TYPE_UINT, SDP_ST_UINT8, {.uint8 = 0xbe}}
            },
            3
        },
        {
            {SDP_TYPE_DESC_SEQ32, 0x00, 0x00, 0x00, 0x06, SDP_TYPE_DESC_UINT8, 0xde, SDP_TYPE_DESC_UINT8, 0xad, SDP_TYPE_DESC_UINT8, 0xbe},
            11,
            ERROR_SUCCESS,
            {SDP_TYPE_SEQUENCE, SDP_ST_NONE, {.sequence = {&test_cases[2].stream[0], 11}}},
            {
                {SDP_TYPE_UINT, SDP_ST_UINT8, {.uint8 = 0xde}},
                {SDP_TYPE_UINT, SDP_ST_UINT8, {.uint8 = 0xad}},
                {SDP_TYPE_UINT, SDP_ST_UINT8, {.uint8 = 0xbe}}
            },
            3
        },
        {
            {SDP_TYPE_DESC_SEQ8, SDP_TYPE_DESC_UINT8, 0xde, SDP_TYPE_DESC_UINT8, 0xad, SDP_TYPE_DESC_UINT8, 0xbe},
            1,
            ERROR_INVALID_PARAMETER,
        },
    };
    SIZE_T i;

    for (i = 0; i < ARRAY_SIZE( test_cases ); i++)
    {
        SIZE_T n = 0;
        HBLUETOOTH_CONTAINER_ELEMENT handle = NULL;
        DWORD ret;

        winetest_push_context( "test_cases seq-like %d", (int)i );
        test_BluetoothSdpGetElementData( test_cases[i].stream, test_cases[i].size,
                                         test_cases[i].error, &test_cases[i].data );
        if (test_cases[i].error != ERROR_SUCCESS)
        {
            winetest_pop_context();
            continue;
        }

        while (n < test_cases[i].container_size)
        {
            SDP_ELEMENT_DATA container_elem = {0};

            winetest_push_context( "test_cases[%d].sequence[%d]", (int)i, (int)n );
            ret = BluetoothSdpGetContainerElementData( test_cases[i].data.data.sequence.value,
                                                       test_cases[i].data.data.sequence.length,
                                                       &handle, &container_elem );
            if (ret == ERROR_NO_MORE_ITEMS)
            {
                ok( n == test_cases[i].container_size, "%d != %d.\n",
                    (int)test_cases[i].container_size, (int)n );
                winetest_pop_context();
                break;
            }
            ok( ret == ERROR_SUCCESS, "BluetoothSdpGetContainerElementData failed: %ld.\n", ret );
            if (ret == ERROR_SUCCESS)
            {
                ok( !memcmp( &test_cases[i].sequence[n], &container_elem,
                             sizeof( container_elem ) ),
                    "%s != %s.\n", debugstr_SDP_ELEMENT_DATA( &test_cases[i].sequence[n] ),
                    debugstr_SDP_ELEMENT_DATA( &container_elem ) );
            }
            n++;
            winetest_pop_context();
        }
        winetest_pop_context();
    }
}

struct attr_callback_data
{
    const ULONG *attrs_id;
    const SDP_ELEMENT_DATA *attrs;
    const SIZE_T attrs_n;
    SIZE_T i;
};

static BYTE sdp_record_bytes[] = {
    0x35, 0x48, 0x09, 0x00, 0x00, 0x0a, 0x00, 0x01, 0x00, 0x00, 0x09, 0x00, 0x01, 0x35, 0x03,
    0x19, 0x12, 0x00, 0x09, 0x00, 0x05, 0x35, 0x03, 0x19, 0x10, 0x02, 0x09, 0x00, 0x09, 0x35,
    0x08, 0x35, 0x06, 0x19, 0x12, 0x00, 0x09, 0x01, 0x03, 0x09, 0x02, 0x00, 0x09, 0x01, 0x03,
    0x09, 0x02, 0x01, 0x09, 0x1d, 0x6b, 0x09, 0x02, 0x02, 0x09, 0x02, 0x46, 0x09, 0x02, 0x03,
    0x09, 0x05, 0x4d, 0x09, 0x02, 0x04, 0x28, 0x01, 0x09, 0x02, 0x05, 0x09, 0x00, 0x02 };

static BOOL WINAPI enum_attr_callback( ULONG attr_id, BYTE *stream, ULONG stream_size, void *param )
{
    struct attr_callback_data *params = param;
    winetest_push_context( "attributes %d", (int)params->i );
    if (params->i < params->attrs_n)
    {
        SDP_ELEMENT_DATA data = {0}, data2 = {0};
        DWORD result;

        ok( attr_id == params->attrs_id[params->i], "Expected attribute id %lu, got %lu.\n",
            params->attrs_id[params->i], attr_id );
        result = BluetoothSdpGetElementData( stream, stream_size, &data );
        ok( result == ERROR_SUCCESS, "BluetoothSdpGetElementData failed: %ld.\n", result );
        ok( !memcmp( &params->attrs[params->i], &data, sizeof( data ) ), "Expected %s, got %s.\n",
            debugstr_SDP_ELEMENT_DATA( &params->attrs[params->i] ),
            debugstr_SDP_ELEMENT_DATA( &data ) );

        result = BluetoothSdpGetAttributeValue( sdp_record_bytes, ARRAY_SIZE( sdp_record_bytes ),
                                               params->attrs_id[params->i], &data2 );
        ok( result == ERROR_SUCCESS, "BluetoothSdpGetAttributeValue failed: %ld.\n", result );
        ok( !memcmp( &params->attrs[params->i], &data2, sizeof( data2 ) ), "Expected %s, got %s.\n",
            debugstr_SDP_ELEMENT_DATA( &params->attrs[params->i] ),
            debugstr_SDP_ELEMENT_DATA( &data2 ) );

        params->i++;
    }
    winetest_pop_context();
    return TRUE;
}

static void test_BluetoothSdpEnumAttributes( void )
{
    SDP_ELEMENT_DATA attributes[] = {
        {SDP_TYPE_UINT, SDP_ST_UINT32, {.uint32 = 0x10000}},
        {SDP_TYPE_SEQUENCE, SDP_ST_NONE, {.sequence = {&sdp_record_bytes[13], 5}}},
        {SDP_TYPE_SEQUENCE, SDP_ST_NONE, {.sequence = {&sdp_record_bytes[21], 5}}},
        {SDP_TYPE_SEQUENCE, SDP_ST_NONE, {.sequence = {&sdp_record_bytes[29], 10}}},
        {SDP_TYPE_UINT, SDP_ST_UINT16, {.uint16 = 0x0103}},
        {SDP_TYPE_UINT, SDP_ST_UINT16, {.uint16 = 0x1d6b}},
        {SDP_TYPE_UINT, SDP_ST_UINT16, {.uint16 = 0x0246}},
        {SDP_TYPE_UINT, SDP_ST_UINT16, {.uint16 = 0x054d}},
        {SDP_TYPE_BOOLEAN, SDP_ST_NONE, {.booleanVal = 1}},
        {SDP_TYPE_UINT, SDP_ST_UINT16, {.uint16 = 0x02}},
    };
    const ULONG attrs_id[] = {0x0, 0x1, 0x5, 0x9, 0x200, 0x201, 0x202, 0x203, 0x204, 0x205};
    struct attr_callback_data data = {attrs_id, attributes, ARRAY_SIZE( attributes ), 0};
    SDP_ELEMENT_DATA elem_data = {0};
    DWORD result;

    BOOL ret;

    SetLastError( 0xdeadbeef );
    ret = BluetoothSdpEnumAttributes( sdp_record_bytes, ARRAY_SIZE( sdp_record_bytes ), enum_attr_callback,
                                      &data );
    ok( ret, "BluetoothSdpEnumAttributes failed with %ld.\n", GetLastError() );
    ok( data.i == data.attrs_n, "%d != %d\n", (int)data.i, (int)data.attrs_n );

    result = BluetoothSdpGetAttributeValue( sdp_record_bytes, ARRAY_SIZE( sdp_record_bytes ), 0xff,
                                            &elem_data );
    ok( result == ERROR_FILE_NOT_FOUND, "%d != %ld.\n", ERROR_FILE_NOT_FOUND, result );
}

START_TEST( sdp )
{
    test_BluetoothSdpGetElementData_nil();
    test_BluetoothSdpGetElementData_ints();
    test_BluetoothSdpGetElementData_invalid();
    test_BluetoothSdpGetElementData_str();
    test_BluetoothSdpGetContainerElementData();
    test_BluetoothSdpEnumAttributes();
}
