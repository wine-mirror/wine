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
    return wine_dbg_sprintf( "{%#x %#x {%llu %llu}}", data->type, data->specificType,
                             data->data.uint128.LowPart, data->data.uint128.HighPart );
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
        ok( !memcmp( &sdp_data->data, &result.data, sizeof( result.data ) ),
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
    static struct
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

#define SDP_SIZE_DESC_1_BYTE 0
#define SDP_SIZE_DESC_2_BYTES 1
#define SDP_SIZE_DESC_4_BYTES 2
#define SDP_SIZE_DESC_8_BYTES 3
#define SDP_SIZE_DESC_16_BYTES 4
#define SDP_SIZE_DESC_NEXT_UINT8 5
#define SDP_SIZE_DESC_NEXT_UINT16 6
#define SDP_SIZE_DESC_NEXT_UINT32 7

#define SDP_DATA_ELEM_TYPE_DESC(t,s) ((t) << 3 | SDP_SIZE_DESC_##s)

#define SDP_DEF_TYPE(n, t, s) const static BYTE SDP_TYPE_DESC_##n = SDP_DATA_ELEM_TYPE_DESC(SDP_TYPE_##t, s)
#define SDP_DEF_INTEGRAL( w, s )                                                                   \
    SDP_DEF_TYPE( INT##w, INT, s );                                                                \
    SDP_DEF_TYPE( UINT##w, UINT, s);

SDP_DEF_INTEGRAL( 8, 1_BYTE );
SDP_DEF_INTEGRAL( 16, 2_BYTES );
SDP_DEF_INTEGRAL( 32, 4_BYTES );
SDP_DEF_INTEGRAL( 64, 8_BYTES );
SDP_DEF_INTEGRAL( 128, 16_BYTES );

SDP_DEF_TYPE( STR8, STRING, NEXT_UINT8 );
SDP_DEF_TYPE( STR16, STRING, NEXT_UINT16 );
SDP_DEF_TYPE( STR32, STRING, NEXT_UINT32 );

SDP_DEF_TYPE( SEQ8, SEQUENCE, NEXT_UINT8 );
SDP_DEF_TYPE( SEQ16, SEQUENCE, NEXT_UINT16 );
SDP_DEF_TYPE( SEQ32, SEQUENCE, NEXT_UINT32 );

static void test_BluetoothSdpGetElementData_ints( void )
{
    static struct
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
    static struct {
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
    static struct {
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
                todo_wine ok( n == test_cases[i].container_size, "%d != %d.\n",
                              (int)test_cases[i].container_size, (int)n );
                winetest_pop_context();
                break;
            }
            todo_wine ok( ret == ERROR_SUCCESS,
                          "BluetoothSdpGetContainerElementData failed: %ld.\n", ret );
            if (ret == ERROR_SUCCESS)
            {
                todo_wine ok( !memcmp( &test_cases[i].sequence[n], &container_elem,
                                       sizeof( container_elem ) ),
                              "%s != %s.\n",
                              debugstr_SDP_ELEMENT_DATA( &test_cases[i].sequence[n] ),
                              debugstr_SDP_ELEMENT_DATA( &container_elem ) );
            }
            n++;
            winetest_pop_context();
        }
        winetest_pop_context();
    }
}

START_TEST( sdp )
{
    test_BluetoothSdpGetElementData_nil();
    test_BluetoothSdpGetElementData_ints();
    test_BluetoothSdpGetElementData_invalid();
    test_BluetoothSdpGetElementData_str();
    test_BluetoothSdpGetContainerElementData();
}
