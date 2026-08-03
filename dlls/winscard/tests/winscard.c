/*
 * Unit test for winscard functions
 *
 * Copyright 2022 Hans Leidekker for CodeWeavers
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
#include <windows.h>
#include <winscard.h>

#include "wine/test.h"

static void test_SCardEstablishContext(void)
{
    const BYTE cmd[] = {0x00, 0xca, 0x01, 0x86, 0x00};
    SCARDCONTEXT context;
    SCARDHANDLE connect;
    SCARD_READERSTATEA states[2];
    SCARD_IO_REQUEST send_pci = {SCARD_PROTOCOL_T1, 8}, recv_pci = {SCARD_PROTOCOL_T1, 8};
    char *readers, *groups, *ptr;
    WCHAR *names, *ptrW;
    BYTE buf[32], recv_buf[264], *atr, *attr;
    DWORD len, atrlen, state, protocol;
    LONG ret;

    ret = SCardEstablishContext( 0, NULL, NULL, NULL );
    ok( ret == SCARD_E_INVALID_PARAMETER, "got %lx\n", ret );

    ret = SCardEstablishContext( 0, NULL, NULL, &context );
    if (ret == SCARD_E_NO_SERVICE)
    {
        skip( "can't establish context, make sure pcscd is running\n" );
        return;
    }
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );

    ret = SCardIsValidContext( context );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );

    len = 0;
    ret = SCardListReadersA( context, NULL, NULL, &len );
    if (ret == SCARD_E_NO_READERS_AVAILABLE)
    {
        skip( "connect a smart card device to run more tests\n" );
        return;
    }
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );
    ok( len, "got zero length\n" );

    readers = calloc( 1, len );
    ret = SCardListReadersA( context, NULL, readers, NULL );
    ok( ret == SCARD_E_INVALID_PARAMETER, "got %lx\n", ret );

    len -= 1;
    ret = SCardListReadersA( context, NULL, readers, &len );
    ok( ret == SCARD_E_INSUFFICIENT_BUFFER, "got %lx\n", ret );

    len += 1;
    ret = SCardListReadersA( context, NULL, readers, &len );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );
    free( readers );

    readers = NULL;
    len = SCARD_AUTOALLOCATE;
    ret = SCardListReadersA( context, NULL, (char *)&readers, &len );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );
    ok( readers != NULL, "got NULL readers" );
    ok( len != SCARD_AUTOALLOCATE, "got %lu", len );
    if (!*readers)
    {
        skip( "connect a smart card device to run more tests\n" );
        return;
    }
    ptr = readers;
    while (*ptr)
    {
        trace( "found reader: %s\n", wine_dbgstr_a(ptr) );
        ptr += strlen( ptr ) + 1;
    }

    len = 0;
    ret = SCardListReaderGroupsA( context, NULL, &len );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );
    ok( len, "got zero length\n" );

    groups = calloc( 1, len );
    ret = SCardListReaderGroupsA( context, groups, &len );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );
    ptr = groups;
    while (*ptr)
    {
        trace( "found group: %s\n", wine_dbgstr_a(ptr) );
        ptr += strlen( ptr ) + 1;
    }
    free( groups );

    ret = SCardGetStatusChangeW( context, 1000, NULL, 0 );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );

    memset( states, 0, sizeof(states) );
    states[0].szReader = "\\\\?PnP?\\Notification";
    states[1].szReader = readers;
    states[1].cbAtr = sizeof(states[1].rgbAtr) + 1;
    ret = SCardGetStatusChangeA( context, 1000, states, 2 );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );
    ok( states[1].cbAtr <= sizeof(states[1].rgbAtr), "got %lu\n", states[1].cbAtr );

    states[1].dwCurrentState = states[1].dwEventState & ~SCARD_STATE_CHANGED;
    ret = SCardGetStatusChangeA( context, 1000, states, 2 );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );

    ret = SCardConnectA( context, readers, SCARD_SHARE_SHARED, SCARD_PROTOCOL_Tx, &connect, NULL );
    if (ret == SCARD_E_READER_UNAVAILABLE)
    {
        skip( "can't connect to reader %s (in use by other application?)\n", wine_dbgstr_a(readers) );
        SCardFreeMemory( context, readers );
        return;
    }
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );

    connect = 0xdeadbeef;
    protocol = 0xdeadbeef;
    ret = SCardConnectA( context, readers, SCARD_SHARE_SHARED, SCARD_PROTOCOL_Tx, &connect, &protocol );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );
    ok( connect != 0xdeadbeef, "connect not set\n" );
    ok( protocol == SCARD_PROTOCOL_T1, "got %lx\n", protocol );
    SCardFreeMemory( context, readers );

    len = atrlen = 0;
    state = 0xdeadbeef;
    protocol = 0xdeadbeef;
    ret = SCardStatusW( connect, NULL, &len, &state, &protocol, NULL, &atrlen );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );
    ok( len, "got zero length\n" );
    ok( state != 0xdeadbeef, "state not set\n" );
    ok( protocol == SCARD_PROTOCOL_T1, "got %lx\n", protocol );
    ok( atrlen, "got zero length\n" );

    names = calloc( 1, len * sizeof(WCHAR) );
    atr = calloc( 1, atrlen );
    state = protocol = 0xdeadbeef;
    ret = SCardStatusW( connect, names, &len, &state, &protocol, atr, &atrlen );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );
    ok( state != 0xdeadbeef, "state not set\n" );
    ok( protocol == SCARD_PROTOCOL_T1, "got %lx\n", protocol );
    ptrW = names;
    while (*ptrW)
    {
        trace( "found name: %s\n", wine_dbgstr_w(ptrW) );
        ptrW += wcslen( ptrW ) + 1;
    }
    ret = SCardStatusW( connect, names, &len, &state, &protocol, NULL, NULL );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );
    ret = SCardStatusW( connect, NULL, NULL, &state, &protocol, NULL, NULL );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );
    ret = SCardStatusW( connect, NULL, NULL, NULL, NULL, NULL, NULL );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );
    free( names );
    free( atr );

    ret = SCardBeginTransaction( connect );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );

    ret = SCardEndTransaction( connect, 0 );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );

    ret = SCardReconnect( connect, SCARD_SHARE_SHARED, SCARD_PROTOCOL_Tx, SCARD_LEAVE_CARD, NULL );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );

    protocol = 0xdeadbeef;
    ret = SCardReconnect( connect, SCARD_SHARE_SHARED, SCARD_PROTOCOL_Tx, SCARD_LEAVE_CARD, &protocol );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );
    ok( protocol == SCARD_PROTOCOL_T1, "got %lx\n", protocol );

    len = 0;
    ret = SCardGetAttrib( connect, SCARD_ATTR_VENDOR_NAME, NULL, &len );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );
    ok( len, "got zero length\n" );

    attr = calloc( 1, len );
    ret = SCardGetAttrib( connect, SCARD_ATTR_VENDOR_NAME, attr, &len );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );

    ret = SCardGetAttrib( connect, SCARD_ATTR_VENDOR_NAME, attr, NULL );
    ok( ret == SCARD_E_INVALID_PARAMETER, "got %lx\n", ret );
    free( attr );

    ret = SCardBeginTransaction( connect );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );

    send_pci.dwProtocol = recv_pci.dwProtocol = protocol;
    memset( recv_buf, 0, sizeof(recv_buf) );
    ret = SCardTransmit( connect, &send_pci, cmd, sizeof(cmd), &recv_pci, recv_buf, NULL );
    ok( ret == SCARD_E_INVALID_PARAMETER, "got %lx (%lu)\n", ret, ret );

    len = sizeof(recv_buf);
    ret = SCardTransmit( connect, &send_pci, cmd, sizeof(cmd), &recv_pci, recv_buf, &len );
    ok( ret == SCARD_S_SUCCESS, "got %lx (%lu)\n", ret, ret );

    ret = SCardEndTransaction( connect, 0 );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );

    buf[0] = 0x02;
    ret = SCardControl( connect, SCARD_CTL_CODE(1), buf, 1, buf, sizeof(buf), NULL );
    ok( ret == SCARD_E_INVALID_PARAMETER, "got %lx\n", ret );

    len = sizeof(buf);
    ret = SCardControl( connect, SCARD_CTL_CODE(1), buf, 1, buf, sizeof(buf), &len );
    todo_wine ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );
    todo_wine ok( !len, "got %lu\n", len );

    ret = SCardDisconnect( connect, 0 );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );

    ret = SCardDisconnect( 0, 0 );
    ok( ret == ERROR_INVALID_HANDLE, "got %lx\n", ret );

    ret = SCardCancel( context );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );

    ret = SCardCancel( 0 );
    ok( ret == ERROR_INVALID_HANDLE, "got %lx\n", ret );

    ret = SCardIsValidContext( context );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );

    ret = SCardReleaseContext( 0 );
    ok( ret == ERROR_INVALID_HANDLE, "got %lx\n", ret );

    ret = SCardReleaseContext( context );
    ok( ret == SCARD_S_SUCCESS, "got %lx\n", ret );

    ret = SCardIsValidContext( context );
    ok( ret == ERROR_INVALID_HANDLE, "got %lx\n", ret );

    ret = SCardIsValidContext( 0 );
    ok( ret == ERROR_INVALID_HANDLE, "got %lx\n", ret );
}

START_TEST(winscard)
{
    test_SCardEstablishContext();
}
