/*
 * ntoskrnl.exe testing framework
 *
 * Copyright 2015 Sebastian Lackner
 * Copyright 2015 Michael Müller
 * Copyright 2015 Christian Costa
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


/* All custom IOCTLs need to have a function value >= 0x800. */
#define IOCTL_WINETEST_BASIC_IOCTL              CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_MAIN_TEST                CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_LOAD_DRIVER              CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_RESET_CANCEL             CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_TEST_CANCEL              CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_GET_CANCEL_COUNT         CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_DETACH                   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_GET_CREATE_COUNT         CTL_CODE(FILE_DEVICE_UNKNOWN, 0x807, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_GET_CLOSE_COUNT          CTL_CODE(FILE_DEVICE_UNKNOWN, 0x808, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_GET_FSCONTEXT            CTL_CODE(FILE_DEVICE_UNKNOWN, 0x809, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_RETURN_STATUS_BUFFERED   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x80a, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_RETURN_STATUS_DIRECT     CTL_CODE(FILE_DEVICE_UNKNOWN, 0x80a, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_RETURN_STATUS_NEITHER    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x80a, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_COMPLETION               CTL_CODE(FILE_DEVICE_UNKNOWN, 0x80c, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_WINETEST_BUS_MAIN             CTL_CODE(FILE_DEVICE_BUS_EXTENDER, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_BUS_REGISTER_IFACE   CTL_CODE(FILE_DEVICE_BUS_EXTENDER, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_BUS_ENABLE_IFACE     CTL_CODE(FILE_DEVICE_BUS_EXTENDER, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_BUS_DISABLE_IFACE    CTL_CODE(FILE_DEVICE_BUS_EXTENDER, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_BUS_ADD_CHILD        CTL_CODE(FILE_DEVICE_BUS_EXTENDER, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_BUS_REMOVE_CHILD     CTL_CODE(FILE_DEVICE_BUS_EXTENDER, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_WINETEST_CHILD_GET_ID         CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_CHILD_MARK_PENDING   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_CHILD_CHECK_REMOVED  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_WINETEST_CHILD_MAIN           CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

static const char teststr[] = "Wine is not an emulator";

struct test_data
{
    int running_under_wine;
    int winetest_report_success;
    int winetest_debug;
    LONG successes;
    LONG failures;
    LONG todo_successes;
    LONG todo_failures;
    LONG skipped;
};

struct main_test_input
{
    DWORD process_id;
    SIZE_T teststr_offset;
    ULONG64 *modified_value;
};

struct return_status_params
{
    NTSTATUS ret_status;
    NTSTATUS iosb_status;
    BOOL pending;
};

static const GUID control_class = {0xdeadbeef, 0x29ef, 0x4538, {0xa5, 0xfd, 0xb6, 0x95, 0x73, 0xa3, 0x62, 0xc0}};

#define SERVER_LISTEN_PORT 9374
#define CLIENT_LISTEN_PORT 9375

#define WINETEST_DEFINE_DEVPROPS                                                                         \
    WINETEST_DRIVER_DEVPROP( 1, DEVPROP_TYPE_BYTE, {.byte = 0xde}, sizeof( BYTE ) )                      \
    WINETEST_DRIVER_DEVPROP( 2, DEVPROP_TYPE_INT16, {.int16 = 0xbeef}, sizeof( INT16 ) )                 \
    WINETEST_DRIVER_DEVPROP( 3, DEVPROP_TYPE_UINT16, {.uint16 = 0xbeef}, sizeof( UINT16 ) )              \
    WINETEST_DRIVER_DEVPROP( 4, DEVPROP_TYPE_INT32, {.int32 = 0xdeadbeef}, sizeof( INT32 ) )             \
    WINETEST_DRIVER_DEVPROP( 5, DEVPROP_TYPE_UINT32, {.uint32 = 0xdeadbeef}, sizeof( UINT32 ) )          \
    WINETEST_DRIVER_DEVPROP( 6, DEVPROP_TYPE_INT64, {.int64 = 0xdeadbeefdeadbeef}, sizeof( INT64 ) )     \
    WINETEST_DRIVER_DEVPROP( 7, DEVPROP_TYPE_UINT64, {.uint64 = 0xdeadbeefdeadbeef}, sizeof( UINT64 ) )

#define WINETEST_DRIVER_DEVPROP( i, typ, val, size )                                               \
    DEFINE_DEVPROPKEY( DEVPKEY_Winetest_##i, 0xdeadbeef, 0xdead, 0xbeef, 0xde, 0xad, 0xbe, 0xef,   \
                       0xde, 0xad, 0xbe, 0xef, ( i ) );

WINETEST_DEFINE_DEVPROPS;
DEFINE_DEVPROPKEY( DEVPKEY_Winetest_8, 0xdeadbeef, 0xdead, 0xbeef, 0xde, 0xad, 0xbe, 0xef, 0xde,
                   0xad, 0xbe, 0xef, 8 ); /* DEVPROP_TYPE_GUID */

#undef WINETEST_DRIVER_DEVPROP
