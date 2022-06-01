/*
 * DInput driver-based testing framework
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

#ifndef __WINE_DINPUT_TEST_H
#define __WINE_DINPUT_TEST_H

#include <stdarg.h>
#include <stddef.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winioctl.h"
#include "winternl.h"

#include "dinput.h"

#include "ddk/wdm.h"
#include "ddk/hidsdi.h"
#include "ddk/hidport.h"

#include "wine/test.h"

#include "driver_hid.h"

#define EXPECT_VIDPID MAKELONG( 0x1209, 0x0001 )
extern const HID_DEVICE_ATTRIBUTES default_attributes;
extern const WCHAR expect_vidpid_str[];
extern const GUID expect_guid_product;
extern const WCHAR expect_path[];
extern const WCHAR expect_path_end[];

extern HANDLE device_added, device_removed;
extern HINSTANCE instance;
extern BOOL localized; /* object names get translated */

BOOL hid_device_start( struct hid_device_desc *desc );
void hid_device_stop( struct hid_device_desc *desc );
BOOL bus_device_start(void);
void bus_device_stop(void);

void cleanup_registry_keys(void);

#define dinput_test_init() dinput_test_init_( __FILE__, __LINE__ )
BOOL dinput_test_init_( const char *file, int line );
void dinput_test_exit(void);

HRESULT dinput_test_create_device( DWORD version, DIDEVICEINSTANCEW *devinst, IDirectInputDevice8W **device );
DWORD WINAPI dinput_test_device_thread( void *stop_event );

#define fill_context( a, b ) fill_context_( __FILE__, __LINE__, a, b )
void fill_context_( const char *file, int line, char *buffer, SIZE_T size );

#define check_member_( file, line, val, exp, fmt, member )                                         \
    ok_(file, line)( (val).member == (exp).member, "got " #member " " fmt "\n", (val).member )
#define check_member( val, exp, fmt, member )                                                      \
    check_member_( __FILE__, __LINE__, val, exp, fmt, member )

#define check_member_guid_( file, line, val, exp, member )                                         \
    ok_(file, line)( IsEqualGUID( &(val).member, &(exp).member ), "got " #member " %s\n",         \
                     debugstr_guid( &(val).member ) )
#define check_member_guid( val, exp, member )                                                      \
    check_member_guid_( __FILE__, __LINE__, val, exp, member )

#define check_member_wstr_( file, line, val, exp, member )                                         \
    ok_(file, line)( !wcscmp( (val).member, (exp).member ), "got " #member " %s\n",                \
                     debugstr_w((val).member) )
#define check_member_wstr( val, exp, member )                                                      \
    check_member_wstr_( __FILE__, __LINE__, val, exp, member )

#define sync_ioctl( a, b, c, d, e, f, g ) sync_ioctl_( __FILE__, __LINE__, a, b, c, d, e, f, g )
BOOL sync_ioctl_( const char *file, int line, HANDLE device, DWORD code, void *in_buf, DWORD in_len,
                  void *out_buf, DWORD *ret_len, DWORD timeout );

#define set_hid_expect( a, b, c ) set_hid_expect_( __FILE__, __LINE__, a, NULL, b, c )
#define bus_set_hid_expect( a, b, c, d ) set_hid_expect_( __FILE__, __LINE__, a, b, c, d )
void set_hid_expect_( const char *file, int line, HANDLE device, struct hid_device_desc *desc,
                      struct hid_expect *expect, DWORD expect_size );

#define wait_hid_expect( a, b ) wait_hid_expect_( __FILE__, __LINE__, a, NULL, b, FALSE, FALSE )
#define wait_hid_pending( a, b ) wait_hid_expect_( __FILE__, __LINE__, a, NULL, b, TRUE, FALSE )
#define bus_wait_hid_expect( a, b, c ) wait_hid_expect_( __FILE__, __LINE__, a, b, c, FALSE, FALSE )
#define bus_wait_hid_pending( a, b, c ) wait_hid_expect_( __FILE__, __LINE__, a, b, c, TRUE, FALSE )
void wait_hid_expect_( const char *file, int line, HANDLE device, struct hid_device_desc *desc,
                       DWORD timeout, BOOL wait_pending, BOOL todo );

#define send_hid_input( a, b, c ) send_hid_input_( __FILE__, __LINE__, a, NULL, b, c )
#define bus_send_hid_input( a, b, c, d ) send_hid_input_( __FILE__, __LINE__, a, b, c, d )
void send_hid_input_( const char *file, int line, HANDLE device, struct hid_device_desc *desc,
                      struct hid_expect *expect, DWORD expect_size );

#endif /* __WINE_DINPUT_TEST_H */
