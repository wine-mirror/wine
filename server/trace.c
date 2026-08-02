/*
 * Server request tracing
 *
 * Copyright (C) 1999 Alexandre Julliard
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

#include "config.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wincon.h"
#include "winternl.h"
#include "winuser.h"
#include "winioctl.h"
#include "wine/condrv.h"
#include "ddk/wdm.h"
#include "ddk/ntddser.h"
#define USE_WS_PREFIX
#include "winsock2.h"
#include "ws2tcpip.h"
#include "tcpmib.h"
#include "file.h"
#include "request.h"
#include "security.h"
#include "unicode.h"
#include "request_trace.h"

/* utility functions */

static inline void remove_data( data_size_t size )
{
    cur_data = (const char *)cur_data + size;
    cur_size -= size;
}

static const char *get_status_name( unsigned int status )
{
    int i;
    static char buffer[10];

    if (status)
    {
        for (i = 0; status_names[i].name; i++)
            if (status_names[i].value == status) return status_names[i].name;
    }
    snprintf( buffer, sizeof(buffer), "%x", status );
    return buffer;
}

static void dump_uints( const char *prefix, const unsigned int *ptr, int len )
{
    fprintf( stderr, "%s{", prefix );
    while (len > 0)
    {
        fprintf( stderr, "%08x", *ptr++ );
        if (--len) fputc( ',', stderr );
    }
    fputc( '}', stderr );
}

static void dump_handles( const char *prefix, const obj_handle_t *data, data_size_t size )
{
    data_size_t len = size / sizeof(*data);

    fprintf( stderr,"%s{", prefix );
    while (len > 0)
    {
        fprintf( stderr, "%04x", *data++ );
        if (--len) fputc( ',', stderr );
    }
    fputc( '}', stderr );
}

static void dump_timeout( const char *prefix, const timeout_t *time )
{
    fprintf( stderr, "%s%s", prefix, get_timeout_str(*time) );
}

static void dump_abstime( const char *prefix, const abstime_t *when )
{
    timeout_t timeout = abstime_to_timeout( *when );
    dump_timeout( prefix, &timeout );
}

static void dump_uint64( const char *prefix, const unsigned __int64 *val )
{
    if ((unsigned int)*val != *val)
        fprintf( stderr, "%s%x%08x", prefix, (unsigned int)(*val >> 32), (unsigned int)*val );
    else
        fprintf( stderr, "%s%08x", prefix, (unsigned int)*val );
}

static void dump_uint128( const char *prefix, const unsigned __int64 val[2] )
{
    unsigned __int64 low = val[0], high = val[1];

    if ((unsigned int)high != high)
        fprintf( stderr, "%s%x%08x%08x%08x", prefix, (unsigned int)(high >> 32), (unsigned int)high,
                 (unsigned int)(low >> 32), (unsigned int)low );
    else if (high)
        fprintf( stderr, "%s%x%08x%08x", prefix, (unsigned int)high,
                 (unsigned int)(low >> 32), (unsigned int)low );
    else if ((unsigned int)low != low)
        fprintf( stderr, "%s%x%08x", prefix, (unsigned int)(low >> 32), (unsigned int)low );
    else
        fprintf( stderr, "%s%x", prefix, (unsigned int)low );
}

static void dump_uints64( const char *prefix, const unsigned __int64 *ptr, int len )
{
    fprintf( stderr, "%s{", prefix );
    if (len-- > 0) dump_uint64( "", ptr++ );
    while (len-- > 0) dump_uint64( ",", ptr++ );
    fputc( '}', stderr );
}

static void dump_rectangle( const char *prefix, const struct rectangle *rect )
{
    fprintf( stderr, "%s{%d,%d;%d,%d}", prefix,
             rect->left, rect->top, rect->right, rect->bottom );
}

static void dump_ioctl_code( const char *prefix, const ioctl_code_t *code )
{
    switch(*code)
    {
#define CASE(c) case c: fprintf( stderr, "%s%s", prefix, #c ); break
        CASE(IOCTL_CONDRV_ACTIVATE);
        CASE(IOCTL_CONDRV_BIND_PID);
        CASE(IOCTL_CONDRV_CTRL_EVENT);
        CASE(IOCTL_CONDRV_FILL_OUTPUT);
        CASE(IOCTL_CONDRV_GET_INPUT_INFO);
        CASE(IOCTL_CONDRV_GET_MODE);
        CASE(IOCTL_CONDRV_GET_OUTPUT_INFO);
        CASE(IOCTL_CONDRV_GET_TITLE);
        CASE(IOCTL_CONDRV_PEEK);
        CASE(IOCTL_CONDRV_READ_CONSOLE);
        CASE(IOCTL_CONDRV_READ_INPUT);
        CASE(IOCTL_CONDRV_READ_OUTPUT);
        CASE(IOCTL_CONDRV_SET_MODE);
        CASE(IOCTL_CONDRV_SET_OUTPUT_INFO);
        CASE(IOCTL_CONDRV_SETUP_INPUT);
        CASE(IOCTL_CONDRV_WRITE_CONSOLE);
        CASE(IOCTL_CONDRV_WRITE_INPUT);
        CASE(IOCTL_CONDRV_WRITE_OUTPUT);
        CASE(FSCTL_DISMOUNT_VOLUME);
        CASE(FSCTL_PIPE_DISCONNECT);
        CASE(FSCTL_PIPE_LISTEN);
        CASE(FSCTL_PIPE_PEEK);
        CASE(FSCTL_PIPE_WAIT);
        CASE(IOCTL_SERIAL_GET_TIMEOUTS);
        CASE(IOCTL_SERIAL_GET_WAIT_MASK);
        CASE(IOCTL_SERIAL_SET_TIMEOUTS);
        CASE(IOCTL_SERIAL_SET_WAIT_MASK);
        CASE(WS_SIO_ADDRESS_LIST_CHANGE);
        default: fprintf( stderr, "%s%08x", prefix, *code ); break;
#undef CASE
    }
}

static void dump_apc_call( const char *prefix, const union apc_call *call )
{
    fprintf( stderr, "%s{", prefix );
    switch(call->type)
    {
    case APC_NONE:
        fprintf( stderr, "APC_NONE" );
        break;
    case APC_USER:
        dump_uint64( "APC_USER,func=", &call->user.func );
        dump_uints64( ",args=", call->user.args, 3 );
        break;
    case APC_ASYNC_IO:
        dump_uint64( "APC_ASYNC_IO,user=", &call->async_io.user );
        dump_uint64( ",sb=", &call->async_io.sb );
        fprintf( stderr, ",status=%s,result=%u", get_status_name(call->async_io.status), call->async_io.result );
        break;
    case APC_VIRTUAL_ALLOC:
        dump_uint64( "APC_VIRTUAL_ALLOC,addr=", &call->virtual_alloc.addr );
        dump_uint64( ",size=", &call->virtual_alloc.size );
        dump_uint64( ",zero_bits=", &call->virtual_alloc.zero_bits );
        fprintf( stderr, ",op_type=%x,prot=%x", call->virtual_alloc.op_type, call->virtual_alloc.prot );
        break;
    case APC_VIRTUAL_ALLOC_EX:
        dump_uint64( "APC_VIRTUAL_ALLOC_EX,addr=", &call->virtual_alloc_ex.addr );
        dump_uint64( ",size=", &call->virtual_alloc_ex.size );
        dump_uint64( ",limit_low=", &call->virtual_alloc_ex.limit_low );
        dump_uint64( ",limit_high=", &call->virtual_alloc_ex.limit_high );
        dump_uint64( ",align=", &call->virtual_alloc_ex.align );
        fprintf( stderr, ",op_type=%x,prot=%x,attributes=%x",
                 call->virtual_alloc_ex.op_type, call->virtual_alloc_ex.prot,
                 call->virtual_alloc_ex.attributes );
        break;
    case APC_VIRTUAL_FREE:
        dump_uint64( "APC_VIRTUAL_FREE,addr=", &call->virtual_free.addr );
        dump_uint64( ",size=", &call->virtual_free.size );
        fprintf( stderr, ",op_type=%x", call->virtual_free.op_type );
        break;
    case APC_VIRTUAL_QUERY:
        dump_uint64( "APC_VIRTUAL_QUERY,addr=", &call->virtual_query.addr );
        break;
    case APC_VIRTUAL_PROTECT:
        dump_uint64( "APC_VIRTUAL_PROTECT,addr=", &call->virtual_protect.addr );
        dump_uint64( ",size=", &call->virtual_protect.size );
        fprintf( stderr, ",prot=%x", call->virtual_protect.prot );
        break;
    case APC_VIRTUAL_FLUSH:
        dump_uint64( "APC_VIRTUAL_FLUSH,addr=", &call->virtual_flush.addr );
        dump_uint64( ",size=", &call->virtual_flush.size );
        break;
    case APC_VIRTUAL_LOCK:
        dump_uint64( "APC_VIRTUAL_LOCK,addr=", &call->virtual_lock.addr );
        dump_uint64( ",size=", &call->virtual_lock.size );
        break;
    case APC_VIRTUAL_UNLOCK:
        dump_uint64( "APC_VIRTUAL_UNLOCK,addr=", &call->virtual_unlock.addr );
        dump_uint64( ",size=", &call->virtual_unlock.size );
        break;
    case APC_MAP_VIEW:
        fprintf( stderr, "APC_MAP_VIEW,handle=%04x", call->map_view.handle );
        dump_uint64( ",addr=", &call->map_view.addr );
        dump_uint64( ",size=", &call->map_view.size );
        dump_uint64( ",offset=", &call->map_view.offset );
        dump_uint64( ",zero_bits=", &call->map_view.zero_bits );
        fprintf( stderr, ",alloc_type=%x,prot=%x", call->map_view.alloc_type, call->map_view.prot );
        break;
    case APC_MAP_VIEW_EX:
        fprintf( stderr, "APC_MAP_VIEW_EX,handle=%04x", call->map_view_ex.handle );
        dump_uint64( ",addr=", &call->map_view_ex.addr );
        dump_uint64( ",size=", &call->map_view_ex.size );
        dump_uint64( ",offset=", &call->map_view_ex.offset );
        dump_uint64( ",limit_low=", &call->map_view_ex.limit_low );
        dump_uint64( ",limit_high=", &call->map_view_ex.limit_high );
        fprintf( stderr, ",alloc_type=%x,prot=%x,machine=%04x",
                 call->map_view_ex.alloc_type, call->map_view_ex.prot, call->map_view_ex.machine );
        break;
    case APC_UNMAP_VIEW:
        dump_uint64( "APC_UNMAP_VIEW,addr=", &call->unmap_view.addr );
        break;
    case APC_CREATE_THREAD:
        dump_uint64( "APC_CREATE_THREAD,func=", &call->create_thread.func );
        dump_uint64( ",arg=", &call->create_thread.arg );
        dump_uint64( ",zero_bits=", &call->create_thread.zero_bits );
        dump_uint64( ",reserve=", &call->create_thread.reserve );
        dump_uint64( ",commit=", &call->create_thread.commit );
        fprintf( stderr, ",flags=%x", call->create_thread.flags );
        break;
    case APC_DUP_HANDLE:
        fprintf( stderr, "APC_DUP_HANDLE,src_handle=%04x,dst_process=%04x,access=%x,attributes=%x,options=%x",
                 call->dup_handle.src_handle, call->dup_handle.dst_process, call->dup_handle.access,
                 call->dup_handle.attributes, call->dup_handle.options );
        break;
    default:
        fprintf( stderr, "type=%u", call->type );
        break;
    }
    fputc( '}', stderr );
}

static void dump_apc_result( const char *prefix, const union apc_result *result )
{
    fprintf( stderr, "%s{", prefix );
    switch(result->type)
    {
    case APC_NONE:
        break;
    case APC_ASYNC_IO:
        fprintf( stderr, "APC_ASYNC_IO,status=%s,total=%u",
                 get_status_name( result->async_io.status ), result->async_io.total );
        break;
    case APC_VIRTUAL_ALLOC:
        fprintf( stderr, "APC_VIRTUAL_ALLOC,status=%s",
                 get_status_name( result->virtual_alloc.status ));
        dump_uint64( ",addr=", &result->virtual_alloc.addr );
        dump_uint64( ",size=", &result->virtual_alloc.size );
        break;
    case APC_VIRTUAL_FREE:
        fprintf( stderr, "APC_VIRTUAL_FREE,status=%s",
                 get_status_name( result->virtual_free.status ));
        dump_uint64( ",addr=", &result->virtual_free.addr );
        dump_uint64( ",size=", &result->virtual_free.size );
        break;
    case APC_VIRTUAL_QUERY:
        fprintf( stderr, "APC_VIRTUAL_QUERY,status=%s",
                 get_status_name( result->virtual_query.status ));
        dump_uint64( ",base=", &result->virtual_query.base );
        dump_uint64( ",alloc_base=", &result->virtual_query.alloc_base );
        dump_uint64( ",size=", &result->virtual_query.size );
        fprintf( stderr, ",state=%x,prot=%x,alloc_prot=%x,alloc_type=%x",
                 result->virtual_query.state, result->virtual_query.prot,
                 result->virtual_query.alloc_prot, result->virtual_query.alloc_type );
        break;
    case APC_VIRTUAL_PROTECT:
        fprintf( stderr, "APC_VIRTUAL_PROTECT,status=%s",
                 get_status_name( result->virtual_protect.status ));
        dump_uint64( ",addr=", &result->virtual_protect.addr );
        dump_uint64( ",size=", &result->virtual_protect.size );
        fprintf( stderr, ",prot=%x", result->virtual_protect.prot );
        break;
    case APC_VIRTUAL_FLUSH:
        fprintf( stderr, "APC_VIRTUAL_FLUSH,status=%s",
                 get_status_name( result->virtual_flush.status ));
        dump_uint64( ",addr=", &result->virtual_flush.addr );
        dump_uint64( ",size=", &result->virtual_flush.size );
        break;
    case APC_VIRTUAL_LOCK:
        fprintf( stderr, "APC_VIRTUAL_LOCK,status=%s",
                 get_status_name( result->virtual_lock.status ));
        dump_uint64( ",addr=", &result->virtual_lock.addr );
        dump_uint64( ",size=", &result->virtual_lock.size );
        break;
    case APC_VIRTUAL_UNLOCK:
        fprintf( stderr, "APC_VIRTUAL_UNLOCK,status=%s",
                 get_status_name( result->virtual_unlock.status ));
        dump_uint64( ",addr=", &result->virtual_unlock.addr );
        dump_uint64( ",size=", &result->virtual_unlock.size );
        break;
    case APC_MAP_VIEW:
        fprintf( stderr, "APC_MAP_VIEW,status=%s",
                 get_status_name( result->map_view.status ));
        dump_uint64( ",addr=", &result->map_view.addr );
        dump_uint64( ",size=", &result->map_view.size );
        break;
    case APC_UNMAP_VIEW:
        fprintf( stderr, "APC_UNMAP_VIEW,status=%s",
                 get_status_name( result->unmap_view.status ) );
        break;
    case APC_CREATE_THREAD:
        fprintf( stderr, "APC_CREATE_THREAD,status=%s,pid=%04x,tid=%04x,handle=%04x",
                 get_status_name( result->create_thread.status ),
                 result->create_thread.pid, result->create_thread.tid, result->create_thread.handle );
        break;
    case APC_DUP_HANDLE:
        fprintf( stderr, "APC_DUP_HANDLE,status=%s,handle=%04x",
                 get_status_name( result->dup_handle.status ), result->dup_handle.handle );
        break;
    default:
        fprintf( stderr, "type=%u", result->type );
        break;
    }
    fputc( '}', stderr );
}

static void dump_async_data( const char *prefix, const struct async_data *data )
{
    fprintf( stderr, "%s{handle=%04x,event=%04x", prefix, data->handle, data->event );
    dump_uint64( ",iosb=", &data->iosb );
    dump_uint64( ",user=", &data->user );
    dump_uint64( ",apc=", &data->apc );
    dump_uint64( ",apc_context=", &data->apc_context );
    fputc( '}', stderr );
}

static void dump_irp_params( const char *prefix, const union irp_params *data )
{
    switch (data->type)
    {
    case IRP_CALL_NONE:
        fprintf( stderr, "%s{NONE}", prefix );
        break;
    case IRP_CALL_CREATE:
        fprintf( stderr, "%s{CREATE,access=%08x,sharing=%08x,options=%08x",
                 prefix, data->create.access, data->create.sharing, data->create.options );
        dump_uint64( ",device=", &data->create.device );
        fprintf( stderr, ",file=%08x}", data->create.file );
        break;
    case IRP_CALL_CLOSE:
        fprintf( stderr, "%s{CLOSE", prefix );
        dump_uint64( ",file=", &data->close.file );
        fputc( '}', stderr );
        break;
    case IRP_CALL_READ:
        fprintf( stderr, "%s{READ,key=%08x,out_size=%u", prefix, data->read.key,
                 data->read.out_size );
        dump_uint64( ",pos=", &data->read.pos );
        dump_uint64( ",file=", &data->read.file );
        fputc( '}', stderr );
        break;
    case IRP_CALL_WRITE:
        fprintf( stderr, "%s{WRITE,key=%08x", prefix, data->write.key );
        dump_uint64( ",pos=", &data->write.pos );
        dump_uint64( ",file=", &data->write.file );
        fputc( '}', stderr );
        break;
    case IRP_CALL_FLUSH:
        fprintf( stderr, "%s{FLUSH", prefix );
        dump_uint64( ",file=", &data->flush.file );
        fputc( '}', stderr );
        break;
    case IRP_CALL_IOCTL:
        fprintf( stderr, "%s{IOCTL", prefix );
        dump_ioctl_code( ",code=", &data->ioctl.code );
        fprintf( stderr, ",out_size=%u", data->ioctl.out_size );
        dump_uint64( ",file=", &data->ioctl.file );
        fputc( '}', stderr );
        break;
    case IRP_CALL_VOLUME:
        fprintf( stderr, "%s{VOLUME,class=%u,out_size=%u", prefix,
                 data->volume.info_class, data->volume.out_size );
        dump_uint64( ",file=", &data->volume.file );
        fputc( '}', stderr );
        break;
    case IRP_CALL_FREE:
        fprintf( stderr, "%s{FREE", prefix );
        dump_uint64( ",obj=", &data->free.obj );
        fputc( '}', stderr );
        break;
    case IRP_CALL_CANCEL:
        fprintf( stderr, "%s{CANCEL", prefix );
        dump_uint64( ",irp=", &data->cancel.irp );
        fputc( '}', stderr );
        break;
    }
}

static void dump_hw_input( const char *prefix, const union hw_input *input )
{
    switch (input->type)
    {
    case INPUT_MOUSE:
        fprintf( stderr, "%s{type=MOUSE,x=%d,y=%d,data=%08x,flags=%08x,time=%u",
                 prefix, input->mouse.x, input->mouse.y, input->mouse.data, input->mouse.flags,
                 input->mouse.time );
        dump_uint64( ",info=", &input->mouse.info );
        fputc( '}', stderr );
        break;
    case INPUT_KEYBOARD:
        fprintf( stderr, "%s{type=KEYBOARD,vkey=%04hx,scan=%04hx,flags=%08x,time=%u",
                 prefix, input->kbd.vkey, input->kbd.scan, input->kbd.flags, input->kbd.time );
        dump_uint64( ",info=", &input->kbd.info );
        fputc( '}', stderr );
        break;
    case INPUT_HARDWARE:
        fprintf( stderr, "%s{type=HARDWARE,msg=%04x", prefix, input->hw.msg );
        dump_uint64( ",wparam=", &input->hw.wparam );
        dump_uint64( ",lparam=", &input->hw.lparam );
        switch (input->hw.msg)
        {
        case WM_INPUT:
            fprintf( stderr, "%s{type=HID,device=%04x,usage=%04x:%04x,count=%u,length=%u}",
                     prefix, input->hw.hid.device, HIWORD(input->hw.hid.usage), LOWORD(input->hw.hid.usage),
                     input->hw.hid.count, input->hw.hid.length );
            break;
        case WM_INPUT_DEVICE_CHANGE:
            fprintf( stderr, "%s{type=HID,device=%04x,usage=%04x:%04x}",
                     prefix, input->hw.hid.device, HIWORD(input->hw.hid.usage), LOWORD(input->hw.hid.usage) );
            break;
        }
        fputc( '}', stderr );
        break;
    default:
        fprintf( stderr, "%s{type=%04x}", prefix, input->type );
        break;
    }
}

static void dump_obj_locator( const char *prefix, const struct obj_locator *locator )
{
    fprintf( stderr, "%s{", prefix );
    dump_uint64( "id=", &locator->id );
    dump_uint64( ",offset=", &locator->offset );
    fprintf( stderr, "}" );
}

static void dump_luid( const char *prefix, const struct luid *luid )
{
    fprintf( stderr, "%s%d.%u", prefix, luid->high_part, luid->low_part );
}

static void dump_generic_map( const char *prefix, const struct generic_map *map )
{
    fprintf( stderr, "%s{r=%08x,w=%08x,x=%08x,a=%08x}",
             prefix, map->read, map->write, map->exec, map->all );
}

static void dump_varargs_ints( const char *prefix, data_size_t size )
{
    const int *data = cur_data;
    data_size_t len = size / sizeof(*data);

    fprintf( stderr,"%s{", prefix );
    while (len > 0)
    {
        fprintf( stderr, "%d", *data++ );
        if (--len) fputc( ',', stderr );
    }
    fputc( '}', stderr );
    remove_data( size );
}

static void dump_varargs_uints( const char *prefix, data_size_t size )
{
    const unsigned int *data = cur_data;

    dump_uints( prefix, data, size / sizeof(*data) );
    remove_data( size );
}

static void dump_varargs_uints64( const char *prefix, data_size_t size )
{
    const unsigned __int64 *data = cur_data;

    dump_uints64( prefix, data, size / sizeof(*data) );
    remove_data( size );
}

static void dump_varargs_ushorts( const char *prefix, data_size_t size )
{
    const unsigned short *data = cur_data;
    data_size_t len = size / sizeof(*data);

    fprintf( stderr, "%s{", prefix );
    while (len > 0)
    {
        fprintf( stderr, "%04x", *data++ );
        if (--len) fputc( ',', stderr );
    }
    fputc( '}', stderr );
    remove_data( size );
}

static void dump_varargs_apc_call( const char *prefix, data_size_t size )
{
    const union apc_call *call = cur_data;

    if (size >= sizeof(*call))
    {
        dump_apc_call( prefix, call );
        size = sizeof(*call);
    }
    else fprintf( stderr, "%s{}", prefix );
    remove_data( size );
}

static void dump_varargs_apc_result( const char *prefix, data_size_t size )
{
    const union apc_result *result = cur_data;

    if (size >= sizeof(*result))
    {
        dump_apc_result( prefix, result );
        size = sizeof(*result);
    }
    remove_data( size );
}

static void dump_varargs_select_op( const char *prefix, data_size_t size )
{
    union select_op data;

    if (!size)
    {
        fprintf( stderr, "%s{}", prefix );
        return;
    }
    memset( &data, 0, sizeof(data) );
    memcpy( &data, cur_data, min( size, sizeof(data) ));

    fprintf( stderr, "%s{", prefix );
    switch (data.op)
    {
    case SELECT_NONE:
        fprintf( stderr, "NONE" );
        break;
    case SELECT_WAIT:
    case SELECT_WAIT_ALL:
        fprintf( stderr, "%s", data.op == SELECT_WAIT ? "WAIT" : "WAIT_ALL" );
        if (size > offsetof( union select_op, wait.handles ))
            dump_handles( ",handles=", data.wait.handles,
                          min( size, sizeof(data.wait) ) - offsetof( union select_op, wait.handles ));
        break;
    case SELECT_SIGNAL_AND_WAIT:
        fprintf( stderr, "SIGNAL_AND_WAIT,signal=%04x,wait=%04x",
                 data.signal_and_wait.signal, data.signal_and_wait.wait );
        break;
    case SELECT_KEYED_EVENT_WAIT:
    case SELECT_KEYED_EVENT_RELEASE:
        fprintf( stderr, "KEYED_EVENT_%s,handle=%04x",
                 data.op == SELECT_KEYED_EVENT_WAIT ? "WAIT" : "RELEASE",
                 data.keyed_event.handle );
        dump_uint64( ",key=", &data.keyed_event.key );
        break;
    default:
        fprintf( stderr, "op=%u", data.op );
        break;
    }
    fputc( '}', stderr );
    remove_data( size );
}

static void dump_varargs_user_handles( const char *prefix, data_size_t size )
{
    const user_handle_t *data = cur_data;
    data_size_t len = size / sizeof(*data);

    fprintf( stderr,"%s{", prefix );
    while (len > 0)
    {
        fprintf( stderr, "%08x", *data++ );
        if (--len) fputc( ',', stderr );
    }
    fputc( '}', stderr );
    remove_data( size );
}

static void dump_varargs_bytes( const char *prefix, data_size_t size )
{
    const unsigned char *data = cur_data;
    data_size_t len = min( 1024, size );

    fprintf( stderr,"%s{", prefix );
    while (len > 0)
    {
        fprintf( stderr, "%02x", *data++ );
        if (--len) fputc( ',', stderr );
    }
    if (size > 1024) fprintf( stderr, "...(total %u)", size );
    fputc( '}', stderr );
    remove_data( size );
}

static void dump_varargs_string( const char *prefix, data_size_t size )
{
    fprintf( stderr, "%s\"%.*s\"", prefix, (int)size, (const char *)cur_data );
    remove_data( size );
}

static void dump_varargs_unicode_str( const char *prefix, data_size_t size )
{
    fprintf( stderr, "%sL\"", prefix );
    dump_strW( cur_data, size, stderr, "\"\"" );
    fputc( '\"', stderr );
    remove_data( size );
}

static void dump_varargs_unicode_strings( const char *prefix, data_size_t size )
{
    fprintf( stderr, "%s{", prefix );
    while (cur_size >= sizeof(WCHAR))
    {
        const WCHAR *str = cur_data;
        unsigned int len = 0;

        while (len < cur_size / sizeof(WCHAR) && str[len]) len++;
        fputs( "L\"", stderr );
        dump_strW( cur_data, len * sizeof(WCHAR), stderr, "\"\"" );
        fputc( '\"', stderr );
        if (len < cur_size / sizeof(WCHAR)) len++;  /* skip terminating null */
        remove_data( len * sizeof(WCHAR) );
        if (cur_size >= sizeof(WCHAR)) fputc( ',', stderr );
    }
    fputc( '}', stderr );
}

static void dump_varargs_context( const char *prefix, data_size_t size )
{
    const struct context_data *context = cur_data;
    struct context_data ctx;
    unsigned int i;

    if (!size)
    {
        fprintf( stderr, "%s{}", prefix );
        return;
    }
    size = min( size, sizeof(ctx) );
    memset( &ctx, 0, sizeof(ctx) );
    memcpy( &ctx, context, size );

    switch (ctx.machine)
    {
    case IMAGE_FILE_MACHINE_I386:
        fprintf( stderr, "%s{machine=i386", prefix );
        if (ctx.flags & SERVER_CTX_CONTROL)
            fprintf( stderr, ",eip=%08x,esp=%08x,ebp=%08x,eflags=%08x,cs=%04x,ss=%04x",
                     ctx.ctl.i386_regs.eip, ctx.ctl.i386_regs.esp, ctx.ctl.i386_regs.ebp,
                     ctx.ctl.i386_regs.eflags, ctx.ctl.i386_regs.cs, ctx.ctl.i386_regs.ss );
        if (ctx.flags & SERVER_CTX_SEGMENTS)
            fprintf( stderr, ",ds=%04x,es=%04x,fs=%04x,gs=%04x",
                     ctx.seg.i386_regs.ds, ctx.seg.i386_regs.es,
                     ctx.seg.i386_regs.fs, ctx.seg.i386_regs.gs );
        if (ctx.flags & SERVER_CTX_INTEGER)
            fprintf( stderr, ",eax=%08x,ebx=%08x,ecx=%08x,edx=%08x,esi=%08x,edi=%08x",
                     ctx.integer.i386_regs.eax, ctx.integer.i386_regs.ebx, ctx.integer.i386_regs.ecx,
                     ctx.integer.i386_regs.edx, ctx.integer.i386_regs.esi, ctx.integer.i386_regs.edi );
        if (ctx.flags & SERVER_CTX_DEBUG_REGISTERS)
            fprintf( stderr, ",dr0=%08x,dr1=%08x,dr2=%08x,dr3=%08x,dr6=%08x,dr7=%08x",
                     ctx.debug.i386_regs.dr0, ctx.debug.i386_regs.dr1, ctx.debug.i386_regs.dr2,
                     ctx.debug.i386_regs.dr3, ctx.debug.i386_regs.dr6, ctx.debug.i386_regs.dr7 );
        if (ctx.flags & SERVER_CTX_FLOATING_POINT)
        {
            fprintf( stderr, ",fp.ctrl=%08x,fp.status=%08x,fp.tag=%08x,fp.err_off=%08x,fp.err_sel=%08x",
                     ctx.fp.i386_regs.ctrl, ctx.fp.i386_regs.status, ctx.fp.i386_regs.tag,
                     ctx.fp.i386_regs.err_off, ctx.fp.i386_regs.err_sel );
            fprintf( stderr, ",fp.data_off=%08x,fp.data_sel=%08x,fp.cr0npx=%08x",
                     ctx.fp.i386_regs.data_off, ctx.fp.i386_regs.data_sel, ctx.fp.i386_regs.cr0npx );
            for (i = 0; i < 8; i++)
            {
                unsigned __int64 reg[2];
                memset( reg, 0, sizeof(reg) );
                memcpy( reg, &ctx.fp.i386_regs.regs[10 * i], 10 );
                fprintf( stderr, ",fp.reg%u=", i );
                dump_uint128( "", reg );
            }
        }
        if (ctx.flags & SERVER_CTX_EXTENDED_REGISTERS)
            dump_uints( ",extended=", (const unsigned int *)ctx.ext.i386_regs,
                        sizeof(ctx.ext.i386_regs) / sizeof(int) );
        if (ctx.flags & SERVER_CTX_YMM_REGISTERS)
            for (i = 0; i < 16; i++)
            {
                fprintf( stderr, ",ymm%u=", i );
                dump_uint128( "", (const unsigned __int64 *)&ctx.ymm.regs.ymm_high[i] );
            }
        break;
    case IMAGE_FILE_MACHINE_AMD64:
        fprintf( stderr, "%s{machine=x86_64", prefix );
        if (ctx.flags & SERVER_CTX_CONTROL)
        {
            dump_uint64( ",rip=", &ctx.ctl.x86_64_regs.rip );
            dump_uint64( ",rbp=", &ctx.ctl.x86_64_regs.rbp );
            dump_uint64( ",rsp=", &ctx.ctl.x86_64_regs.rsp );
            fprintf( stderr, ",cs=%04x,ss=%04x,flags=%08x",
                     ctx.ctl.x86_64_regs.cs, ctx.ctl.x86_64_regs.ss, ctx.ctl.x86_64_regs.flags );
        }
        if (ctx.flags & SERVER_CTX_INTEGER)
        {
            dump_uint64( ",rax=", &ctx.integer.x86_64_regs.rax );
            dump_uint64( ",rbx=", &ctx.integer.x86_64_regs.rbx );
            dump_uint64( ",rcx=", &ctx.integer.x86_64_regs.rcx );
            dump_uint64( ",rdx=", &ctx.integer.x86_64_regs.rdx );
            dump_uint64( ",rsi=", &ctx.integer.x86_64_regs.rsi );
            dump_uint64( ",rdi=", &ctx.integer.x86_64_regs.rdi );
            dump_uint64( ",r8=",  &ctx.integer.x86_64_regs.r8 );
            dump_uint64( ",r9=",  &ctx.integer.x86_64_regs.r9 );
            dump_uint64( ",r10=", &ctx.integer.x86_64_regs.r10 );
            dump_uint64( ",r11=", &ctx.integer.x86_64_regs.r11 );
            dump_uint64( ",r12=", &ctx.integer.x86_64_regs.r12 );
            dump_uint64( ",r13=", &ctx.integer.x86_64_regs.r13 );
            dump_uint64( ",r14=", &ctx.integer.x86_64_regs.r14 );
            dump_uint64( ",r15=", &ctx.integer.x86_64_regs.r15 );
        }
        if (ctx.flags & SERVER_CTX_SEGMENTS)
            fprintf( stderr, ",ds=%04x,es=%04x,fs=%04x,gs=%04x",
                     ctx.seg.x86_64_regs.ds, ctx.seg.x86_64_regs.es,
                     ctx.seg.x86_64_regs.fs, ctx.seg.x86_64_regs.gs );
        if (ctx.flags & SERVER_CTX_DEBUG_REGISTERS)
        {
            dump_uint64( ",dr0=", &ctx.debug.x86_64_regs.dr0 );
            dump_uint64( ",dr1=", &ctx.debug.x86_64_regs.dr1 );
            dump_uint64( ",dr2=", &ctx.debug.x86_64_regs.dr2 );
            dump_uint64( ",dr3=", &ctx.debug.x86_64_regs.dr3 );
            dump_uint64( ",dr6=", &ctx.debug.x86_64_regs.dr6 );
            dump_uint64( ",dr7=", &ctx.debug.x86_64_regs.dr7 );
        }
        if (ctx.flags & SERVER_CTX_FLOATING_POINT)
            for (i = 0; i < 32; i++)
            {
                fprintf( stderr, ",fp%u=", i );
                dump_uint128( "", (const unsigned __int64 *)&ctx.fp.x86_64_regs.fpregs[i] );
            }
        if (ctx.flags & SERVER_CTX_YMM_REGISTERS)
            for (i = 0; i < 16; i++)
            {
                fprintf( stderr, ",ymm%u=", i );
                dump_uint128( "", (const unsigned __int64 *)&ctx.ymm.regs.ymm_high[i] );
            }
        break;
    case IMAGE_FILE_MACHINE_ARMNT:
        fprintf( stderr, "%s{machine=arm", prefix );
        if (ctx.flags & SERVER_CTX_CONTROL)
            fprintf( stderr, ",sp=%08x,lr=%08x,pc=%08x,cpsr=%08x",
                     ctx.ctl.arm_regs.sp, ctx.ctl.arm_regs.lr,
                     ctx.ctl.arm_regs.pc, ctx.ctl.arm_regs.cpsr );
        if (ctx.flags & SERVER_CTX_INTEGER)
            for (i = 0; i < 13; i++) fprintf( stderr, ",r%u=%08x", i, ctx.integer.arm_regs.r[i] );
        if (ctx.flags & SERVER_CTX_DEBUG_REGISTERS)
        {
            for (i = 0; i < 8; i++)
                fprintf( stderr, ",bcr%u=%08x,bvr%u=%08x",
                         i, ctx.debug.arm_regs.bcr[i], i, ctx.debug.arm_regs.bvr[i] );
            fprintf( stderr, ",wcr0=%08x,wvr0=%08x",
                     ctx.debug.arm_regs.wcr[0], ctx.debug.arm_regs.wvr[0] );
        }
        if (ctx.flags & SERVER_CTX_FLOATING_POINT)
        {
            for (i = 0; i < 32; i++)
            {
                fprintf( stderr, ",d%u=", i );
                dump_uint64( "", &ctx.fp.arm_regs.d[i] );
            }
            fprintf( stderr, ",fpscr=%08x", ctx.fp.arm_regs.fpscr );
        }
        break;
    case IMAGE_FILE_MACHINE_ARM64:
        fprintf( stderr, "%s{machine=arm64", prefix );
        if (ctx.flags & SERVER_CTX_CONTROL)
        {
            dump_uint64( ",sp=", &ctx.ctl.arm64_regs.sp );
            dump_uint64( ",pc=", &ctx.ctl.arm64_regs.pc );
            dump_uint64( ",pstate=", &ctx.ctl.arm64_regs.pstate );
        }
        if (ctx.flags & SERVER_CTX_INTEGER)
        {
            for (i = 0; i < 31; i++)
            {
                fprintf( stderr, ",x%u=", i );
                dump_uint64( "", &ctx.integer.arm64_regs.x[i] );
            }
        }
        if (ctx.flags & SERVER_CTX_DEBUG_REGISTERS)
        {
            for (i = 0; i < 8; i++)
            {
                fprintf( stderr, ",bcr%u=%08x,bvr%u=", i, ctx.debug.arm64_regs.bcr[i], i );
                dump_uint64( "", &ctx.debug.arm64_regs.bvr[i] );
            }
            for (i = 0; i < 2; i++)
            {
                fprintf( stderr, ",wcr%u=%08x,wvr%u=", i, ctx.debug.arm64_regs.wcr[i], i );
                dump_uint64( "", &ctx.debug.arm64_regs.wvr[i] );
            }
        }
        if (ctx.flags & SERVER_CTX_FLOATING_POINT)
        {
            for (i = 0; i < 32; i++)
            {
                fprintf( stderr, ",q%u=", i );
                dump_uint64( "", &ctx.fp.arm64_regs.q[i].high );
                dump_uint64( "", &ctx.fp.arm64_regs.q[i].low );
            }
            fprintf( stderr, ",fpcr=%08x,fpsr=%08x", ctx.fp.arm64_regs.fpcr, ctx.fp.arm64_regs.fpsr );
        }
        break;
    default:
        fprintf( stderr, "%s{machine=%04x", prefix, ctx.machine );
        break;
    }
    if (ctx.flags & SERVER_CTX_EXEC_SPACE)
    {
        const char *space;

        switch (ctx.exec_space.space.space)
        {
        case EXEC_SPACE_USERMODE:  space = "user"; break;
        case EXEC_SPACE_SYSCALL:   space = "syscall"; break;
        case EXEC_SPACE_EXCEPTION: space = "exception"; break;
        default:                   space = "invalid"; break;
        }
        fprintf( stderr, ",exec_space=%s", space );
    }
    fputc( '}', stderr );
    remove_data( size );
}

static void dump_varargs_contexts( const char *prefix, data_size_t size )
{
    if (!size)
    {
        fprintf( stderr, "%s{}", prefix );
        return;
    }
    fprintf( stderr, "%s{", prefix );
    while (cur_size)
    {
        dump_varargs_context( "", cur_size );
        fputc( cur_size ? ',' : '}', stderr );
    }
}

static void dump_varargs_debug_event( const char *prefix, data_size_t size )
{
    union debug_event_data event;

    if (!size)
    {
        fprintf( stderr, "%s{}", prefix );
        return;
    }
    size = min( size, sizeof(event) );
    memset( &event, 0, sizeof(event) );
    memcpy( &event, cur_data, size );

    switch(event.code)
    {
    case DbgIdle:
        fprintf( stderr, "%s{idle}", prefix );
        break;
    case DbgReplyPending:
        fprintf( stderr, "%s{pending}", prefix );
        break;
    case DbgCreateThreadStateChange:
        fprintf( stderr, "%s{create_thread,thread=%04x", prefix, event.create_thread.handle );
        dump_uint64( ",start=", &event.create_thread.start );
        fputc( '}', stderr );
        break;
    case DbgCreateProcessStateChange:
        fprintf( stderr, "%s{create_process,file=%04x,process=%04x,thread=%04x", prefix,
                 event.create_process.file, event.create_process.process,
                 event.create_process.thread );
        dump_uint64( ",base=", &event.create_process.base );
        fprintf( stderr, ",offset=%d,size=%d",
                 event.create_process.dbg_offset, event.create_process.dbg_size );
        dump_uint64( ",start=", &event.create_process.start );
        fputc( '}', stderr );
        break;
    case DbgExitThreadStateChange:
        fprintf( stderr, "%s{exit_thread,code=%d}", prefix, event.exit.exit_code );
        break;
    case DbgExitProcessStateChange:
        fprintf( stderr, "%s{exit_process,code=%d}", prefix, event.exit.exit_code );
        break;
    case DbgExceptionStateChange:
    case DbgBreakpointStateChange:
    case DbgSingleStepStateChange:
        fprintf( stderr, "%s{%s,first=%d,exc_code=%08x,flags=%08x", prefix,
                 event.code == DbgBreakpointStateChange ? "breakpoint" :
                 event.code == DbgSingleStepStateChange ? "singlestep" : "exception",
                 event.exception.first, event.exception.exc_code, event.exception.flags );
        dump_uint64( ",record=", &event.exception.record );
        dump_uint64( ",address=", &event.exception.address );
        event.exception.nb_params = min( event.exception.nb_params, EXCEPTION_MAXIMUM_PARAMETERS );
        dump_uints64( ",params=", event.exception.params, event.exception.nb_params );
        fputc( '}', stderr );
        break;
    case DbgLoadDllStateChange:
        fprintf( stderr, "%s{load_dll,file=%04x", prefix, event.load_dll.handle );
        dump_uint64( ",base=", &event.load_dll.base );
        fprintf( stderr, ",offset=%d,size=%d",
                 event.load_dll.dbg_offset, event.load_dll.dbg_size );
        dump_uint64( ",name=", &event.load_dll.name );
        fputc( '}', stderr );
        break;
    case DbgUnloadDllStateChange:
        fprintf( stderr, "%s{unload_dll", prefix );
        dump_uint64( ",base=", &event.unload_dll.base );
        fputc( '}', stderr );
        break;
    default:
        fprintf( stderr, "%s{code=??? (%d)}", prefix, event.code );
        break;
    }
    remove_data( size );
}

/* dump a unicode string contained in a buffer; helper for dump_varargs_startup_info */
static data_size_t dump_inline_unicode_string( const char *prefix, data_size_t pos, data_size_t len, data_size_t total_size )
{
    fputs( prefix, stderr );
    if (pos >= total_size) return pos;
    if (len > total_size - pos) len = total_size - pos;
    dump_strW( (const WCHAR *)cur_data + pos/sizeof(WCHAR), len, stderr, "\"\"" );
    return pos + (len / sizeof(WCHAR)) * sizeof(WCHAR);
}

static void dump_varargs_startup_info( const char *prefix, data_size_t size )
{
    struct startup_info_data info;
    data_size_t pos = sizeof(info);

    memset( &info, 0, sizeof(info) );
    memcpy( &info, cur_data, min( size, sizeof(info) ));

    fprintf( stderr,
             "%s{debug_flags=%x,console_flags=%x,console=%04x,hstdin=%04x,hstdout=%04x,hstderr=%04x,"
             "x=%u,y=%u,xsize=%u,ysize=%u,xchars=%u,ychars=%u,attribute=%02x,flags=%x,show=%u,"
             "process_group_id=%u",
             prefix, info.debug_flags, info.console_flags, info.console,
             info.hstdin, info.hstdout, info.hstderr, info.x, info.y, info.xsize, info.ysize,
             info.xchars, info.ychars, info.attribute, info.flags, info.show, info.process_group_id );
    pos = dump_inline_unicode_string( ",curdir=L\"", pos, info.curdir_len, size );
    pos = dump_inline_unicode_string( "\",dllpath=L\"", pos, info.dllpath_len, size );
    pos = dump_inline_unicode_string( "\",imagepath=L\"", pos, info.imagepath_len, size );
    pos = dump_inline_unicode_string( "\",cmdline=L\"", pos, info.cmdline_len, size );
    pos = dump_inline_unicode_string( "\",title=L\"", pos, info.title_len, size );
    pos = dump_inline_unicode_string( "\",desktop=L\"", pos, info.desktop_len, size );
    pos = dump_inline_unicode_string( "\",shellinfo=L\"", pos, info.shellinfo_len, size );
    dump_inline_unicode_string( "\",runtime=L\"", pos, info.runtime_len, size );
    fprintf( stderr, "\"}" );
    remove_data( size );
}

static void dump_varargs_rectangles( const char *prefix, data_size_t size )
{
    const struct rectangle *rect = cur_data;
    data_size_t len = size / sizeof(*rect);

    fprintf( stderr,"%s{", prefix );
    while (len > 0)
    {
        dump_rectangle( "", rect++ );
        if (--len) fputc( ',', stderr );
    }
    fputc( '}', stderr );
    remove_data( size );
}

static void dump_varargs_cursor_positions( const char *prefix, data_size_t size )
{
    const struct cursor_pos *pos = cur_data;
    data_size_t len = size / sizeof(*pos);

    fprintf( stderr, "%s{", prefix );
    while (len > 0)
    {
        fprintf( stderr, "{x=%d,y=%d,time=%u", pos->x, pos->y, pos->time );
        dump_uint64( ",info=", &pos->info );
        fputc( '}', stderr );
        pos++;
        if (--len) fputc( ',', stderr );
    }
    fputc( '}', stderr );
    remove_data( size );
}

static void dump_varargs_message_data( const char *prefix, data_size_t size )
{
    /* FIXME: dump the structured data */
    dump_varargs_bytes( prefix, size );
}

static void dump_varargs_properties( const char *prefix, data_size_t size )
{
    const struct property_data *prop = cur_data;
    data_size_t len = size / sizeof(*prop);

    fprintf( stderr,"%s{", prefix );
    while (len > 0)
    {
        fprintf( stderr, "{atom=%04x,str=%d", prop->atom, prop->string );
        dump_uint64( ",data=", &prop->data );
        fputc( '}', stderr );
        prop++;
        if (--len) fputc( ',', stderr );
    }
    fputc( '}', stderr );
    remove_data( size );
}

static void dump_varargs_luid_attr( const char *prefix, data_size_t size )
{
    const struct luid_attr *lat = cur_data;
    data_size_t len = size / sizeof(*lat);

    fprintf( stderr,"%s{", prefix );
    while (len > 0)
    {
        fprintf( stderr, "{luid=%08x%08x,attrs=%x}", lat->luid.high_part, lat->luid.low_part, lat->attrs );
        lat++;
        if (--len) fputc( ',', stderr );
    }
    fputc( '}', stderr );
    remove_data( size );
}

static void dump_inline_sid( const char *prefix, const struct sid *sid, data_size_t size )
{
    DWORD i;

    fprintf( stderr,"%s", prefix );
    if (sid_valid_size( sid, size ))
    {
        fprintf( stderr, "S-%u-%u", sid->revision,
                 ((unsigned int)sid->id_auth[2] << 24) |
                 ((unsigned int)sid->id_auth[3] << 16) |
                 ((unsigned int)sid->id_auth[4] << 8) |
                 ((unsigned int)sid->id_auth[5]) );
        for (i = 0; i < sid->sub_count; i++) fprintf( stderr, "-%u", sid->sub_auth[i] );
    }
    else fprintf( stderr, "<invalid>" );
}

static void dump_varargs_sid( const char *prefix, data_size_t size )
{
    const struct sid *sid = cur_data;
    if (size) dump_inline_sid( prefix, sid, size );
    remove_data( size );
}

static void dump_varargs_sids( const char *prefix, data_size_t size )
{
    const struct sid *sid = cur_data;
    data_size_t len = size;

    fprintf( stderr,"%s{", prefix );
    while (len > 0)
    {
        if (!sid_valid_size( sid, len ))
        {
            fprintf( stderr, "bad len %u", len);
            break;
        }
        dump_inline_sid( "", sid, size );
        len -= sid_len( sid );
        sid = (const struct sid *)((const char *)sid + sid_len( sid ));
        if (len) fputc( ',', stderr );
    }
    fputc( '}', stderr );
    remove_data( size );
}

static void dump_inline_acl( const char *prefix, const struct acl *acl, data_size_t size )
{
    const struct ace *ace;
    ULONG i;

    fprintf( stderr,"%s{", prefix );
    if (size)
    {
        if (size < sizeof(*acl))
        {
            fprintf( stderr, "<invalid acl>}" );
            return;
        }
        size -= sizeof(*acl);
        for (i = 0, ace = ace_first( acl ); i < acl->count; i++, ace = ace_next( ace ))
        {
            const struct sid *sid = (const struct sid *)(ace + 1);
            data_size_t sid_size;

            if (size < sizeof(*ace) || size < ace->size) break;
            size -= ace->size;
            sid_size = ace->size - sizeof(*ace);
            if (i != 0) fputc( ',', stderr );
            fprintf( stderr, "{type=" );
            switch (ace->type)
            {
            case ACCESS_DENIED_ACE_TYPE:          fprintf( stderr, "ACCESS_DENIED" ); break;
            case ACCESS_ALLOWED_ACE_TYPE:         fprintf( stderr, "ACCESS_ALLOWED" ); break;
            case SYSTEM_AUDIT_ACE_TYPE:           fprintf( stderr, "SYSTEM_AUDIT" ); break;
            case SYSTEM_ALARM_ACE_TYPE:           fprintf( stderr, "SYSTEM_ALARM" ); break;
            case SYSTEM_MANDATORY_LABEL_ACE_TYPE: fprintf( stderr, "SYSTEM_MANDATORY_LABEL" ); break;
            default:
                fprintf( stderr, "%02x", ace->type );
                sid = NULL;
                break;
            }
            fprintf( stderr, ",flags=%x,mask=%x", ace->flags, ace->mask );
            if (sid) dump_inline_sid( ",sid=", sid, sid_size );
            fputc( '}', stderr );
        }
    }
    fputc( '}', stderr );
}

static void dump_varargs_acl( const char *prefix, data_size_t size )
{
    const struct acl *acl = cur_data;
    dump_inline_acl( prefix, acl, size );
    remove_data( size );
}

static void dump_inline_security_descriptor( const char *prefix, const struct security_descriptor *sd, data_size_t size )
{
    fprintf( stderr,"%s{", prefix );
    if (size >= sizeof(struct security_descriptor))
    {
        size_t offset = sizeof(struct security_descriptor);
        fprintf( stderr, "control=%08x", sd->control );
        if ((sd->owner_len > offsetof(struct sid, sub_auth[255])) || (offset + sd->owner_len > size))
            return;
        if (sd->owner_len)
            dump_inline_sid( ",owner=", (const struct sid *)((const char *)sd + offset), sd->owner_len );
        else
            fprintf( stderr, ",owner=<not present>" );
        offset += sd->owner_len;
        if ((sd->group_len > offsetof(struct sid, sub_auth[255])) || (offset + sd->group_len > size))
            return;
        if (sd->group_len)
            dump_inline_sid( ",group=", (const struct sid *)((const char *)sd + offset), sd->group_len );
        else
            fprintf( stderr, ",group=<not present>" );
        offset += sd->group_len;
        if ((sd->sacl_len >= MAX_ACL_LEN) || (offset + sd->sacl_len > size))
            return;
        dump_inline_acl( ",sacl=", (const struct acl *)((const char *)sd + offset), sd->sacl_len );
        offset += sd->sacl_len;
        if ((sd->dacl_len >= MAX_ACL_LEN) || (offset + sd->dacl_len > size))
            return;
        dump_inline_acl( ",dacl=", (const struct acl *)((const char *)sd + offset), sd->dacl_len );
    }
    fputc( '}', stderr );
}

static void dump_varargs_security_descriptor( const char *prefix, data_size_t size )
{
    const struct security_descriptor *sd = cur_data;
    dump_inline_security_descriptor( prefix, sd, size );
    remove_data( size );
}

static void dump_varargs_process_info( const char *prefix, data_size_t size )
{
    data_size_t pos = 0;
    unsigned int i;

    fprintf( stderr,"%s{", prefix );

    while (size - pos >= sizeof(struct process_info))
    {
        const struct process_info *process;
        pos = (pos + 7) & ~7;
        process = (const struct process_info *)((const char *)cur_data + pos);
        if (size - pos < sizeof(*process)) break;
        if (pos) fputc( ',', stderr );
        dump_timeout( "{start_time=", &process->start_time );
        fprintf( stderr, ",thread_count=%u,priority=%d,pid=%04x,parent_pid=%04x,session_id=%08x,handle_count=%u,unix_pid=%d,",
                 process->thread_count, process->priority, process->pid,
                 process->parent_pid, process->session_id, process->handle_count, process->unix_pid );
        pos += sizeof(*process);

        pos = dump_inline_unicode_string( "name=L\"", pos, process->name_len, size );

        pos = (pos + 7) & ~7;
        fprintf( stderr, "\",threads={" );
        for (i = 0; i < process->thread_count; i++)
        {
            const struct thread_info *thread = (const struct thread_info *)((const char *)cur_data + pos);
            if (size - pos < sizeof(*thread)) break;
            if (i) fputc( ',', stderr );
            dump_timeout( "{start_time=", &thread->start_time );
            fprintf( stderr, ",tid=%04x,base_priority=%d,current_priority=%d,unix_tid=%d}",
                     thread->tid, thread->base_priority, thread->current_priority, thread->unix_tid );
            pos += sizeof(*thread);
        }
        fprintf( stderr, "}}" );
    }
    fputc( '}', stderr );
    remove_data( size );
}

static void dump_varargs_object_attributes( const char *prefix, data_size_t size )
{
    const struct object_attributes *objattr = cur_data;

    fprintf( stderr,"%s{", prefix );
    if (size)
    {
        const WCHAR *str;

        if (size < sizeof(*objattr) ||
            (size - sizeof(*objattr) < objattr->sd_len) ||
            (size - sizeof(*objattr) - objattr->sd_len < objattr->name_len))
        {
            fprintf( stderr, "***invalid***}" );
            remove_data( size );
            return;
        }

        fprintf( stderr, "rootdir=%04x,attributes=%08x", objattr->rootdir, objattr->attributes );
        dump_inline_security_descriptor( ",sd=", (const struct security_descriptor *)(objattr + 1), objattr->sd_len );
        str = (const WCHAR *)objattr + (sizeof(*objattr) + objattr->sd_len) / sizeof(WCHAR);
        fprintf( stderr, ",name=L\"" );
        dump_strW( str, objattr->name_len, stderr, "\"\"" );
        fputc( '\"', stderr );
        remove_data( (sizeof(*objattr) + (objattr->sd_len & ~1) + (objattr->name_len & ~1) + 3) & ~3 );
    }
    fputc( '}', stderr );
}

static void dump_varargs_object_type_info( const char *prefix, data_size_t size )
{
    const struct object_type_info *info = cur_data;

    fprintf( stderr,"%s{", prefix );
    if (size)
    {
        if (size < sizeof(*info) || (size - sizeof(*info) < info->name_len))
        {
            fprintf( stderr, "***invalid***}" );
            remove_data( size );
            return;
        }

        fprintf( stderr, "index=%u,obj_count=%u,handle_count=%u,obj_max=%u,handle_max=%u,valid=%08x",
                 info->index,info->obj_count, info->handle_count, info->obj_max, info->handle_max,
                 info->valid_access );
        dump_generic_map( ",access=", &info->mapping );
        fprintf( stderr, ",name=L\"" );
        dump_strW( (const WCHAR *)(info + 1), info->name_len, stderr, "\"\"" );
        fputc( '\"', stderr );
        remove_data( min( size, sizeof(*info) + ((info->name_len + 2) & ~3 )));
    }
    fputc( '}', stderr );
}

static void dump_varargs_object_types_info( const char *prefix, data_size_t size )
{
    fprintf( stderr,"%s{", prefix );
    while (cur_size) dump_varargs_object_type_info( ",", cur_size );
    fputc( '}', stderr );
}

static void dump_varargs_filesystem_event( const char *prefix, data_size_t size )
{
    static const char * const actions[] = {
        NULL,
        "ADDED",
        "REMOVED",
        "MODIFIED",
        "RENAMED_OLD_NAME",
        "RENAMED_NEW_NAME",
        "ADDED_STREAM",
        "REMOVED_STREAM",
        "MODIFIED_STREAM"
    };

    fprintf( stderr,"%s{", prefix );
    while (size)
    {
        const struct filesystem_event *event = cur_data;
        data_size_t len = (offsetof( struct filesystem_event, name[event->len] ) + sizeof(int)-1)
                           / sizeof(int) * sizeof(int);
        if (size < len) break;
        if (event->action < ARRAY_SIZE( actions ) && actions[event->action])
            fprintf( stderr, "{action=%s", actions[event->action] );
        else
            fprintf( stderr, "{action=%u", event->action );
        fprintf( stderr, ",name=\"%.*s\"}", event->len, event->name );
        size -= len;
        remove_data( len );
        if (size)fputc( ',', stderr );
    }
    fputc( '}', stderr );
}

static void dump_varargs_pe_image_info( const char *prefix, data_size_t size )
{
    struct pe_image_info info;

    if (!size)
    {
        fprintf( stderr, "%s{}", prefix );
        return;
    }
    memset( &info, 0, sizeof(info) );
    memcpy( &info, cur_data, min( size, sizeof(info) ));

    fprintf( stderr, "%s{", prefix );
    dump_uint64( "base=", &info.base );
    dump_uint64( ",stack_size=", &info.stack_size );
    dump_uint64( ",stack_commit=", &info.stack_commit );
    fprintf( stderr, ",entry_point=%08x,map_size=%08x,zerobits=%08x,subsystem=%08x,subsystem_minor=%04x,subsystem_major=%04x"
             ",osversion_major=%04x,osversion_minor=%04x,image_charact=%04x,dll_charact=%04x,machine=%04x"
             ",contains_code=%u,image_flags=%02x"
             ",loader_flags=%08x,header_size=%08x,file_size=%08x,checksum=%08x}",
             info.entry_point, info.map_size, info.zerobits, info.subsystem, info.subsystem_minor,
             info.subsystem_major, info.osversion_major, info.osversion_minor, info.image_charact,
             info.dll_charact, info.machine, info.contains_code, info.image_flags, info.loader_flags,
             info.header_size, info.file_size, info.checksum );
    remove_data( min( size, sizeof(info) ));
}

static void dump_varargs_rawinput_devices(const char *prefix, data_size_t size )
{
    const struct rawinput_device *device;

    fprintf( stderr, "%s{", prefix );
    while (size >= sizeof(*device))
    {
        device = cur_data;
        fprintf( stderr, "{usage=%08x,flags=%08x,target=%08x}",
                 device->usage, device->flags, device->target );
        size -= sizeof(*device);
        remove_data( sizeof(*device) );
        if (size) fputc( ',', stderr );
    }
    fputc( '}', stderr );
}

static void dump_varargs_handle_infos( const char *prefix, data_size_t size )
{
    const struct handle_info *handle;

    fprintf( stderr, "%s{", prefix );
    while (size >= sizeof(*handle))
    {
        handle = cur_data;
        fprintf( stderr, "{owner=%04x,handle=%04x,access=%08x,attributes=%08x,type=%u}",
                 handle->owner, handle->handle, handle->access, handle->attributes, handle->type );
        size -= sizeof(*handle);
        remove_data( sizeof(*handle) );
        if (size) fputc( ',', stderr );
    }
    fputc( '}', stderr );
}

static void dump_varargs_tcp_connections( const char *prefix, data_size_t size )
{
    static const char * const state_names[] = {
        NULL,
        "CLOSED",
        "LISTEN",
        "SYN_SENT",
        "SYN_RCVD",
        "ESTAB",
        "FIN_WAIT1",
        "FIN_WAIT2",
        "CLOSE_WAIT",
        "CLOSING",
        "LAST_ACK",
        "TIME_WAIT",
        "DELETE_TCB"
    };
    const union tcp_connection *conn;

    fprintf( stderr, "%s{", prefix );
    while (size >= sizeof(*conn))
    {
        conn = cur_data;

        if (conn->common.family == WS_AF_INET)
        {
            char local_addr_str[INET_ADDRSTRLEN] = { 0 };
            char remote_addr_str[INET_ADDRSTRLEN] = { 0 };
            inet_ntop( AF_INET, (struct in_addr *)&conn->ipv4.local_addr, local_addr_str, INET_ADDRSTRLEN );
            inet_ntop( AF_INET, (struct in_addr *)&conn->ipv4.remote_addr, remote_addr_str, INET_ADDRSTRLEN );
            fprintf( stderr, "{family=AF_INET,owner=%04x,state=%s,local=%s:%d,remote=%s:%d}",
                     conn->ipv4.owner, state_names[conn->ipv4.state],
                     local_addr_str, conn->ipv4.local_port,
                     remote_addr_str, conn->ipv4.remote_port );
        }
        else
        {
            char local_addr_str[INET6_ADDRSTRLEN];
            char remote_addr_str[INET6_ADDRSTRLEN];
            inet_ntop( AF_INET6, (struct in6_addr *)&conn->ipv6.local_addr, local_addr_str, INET6_ADDRSTRLEN );
            inet_ntop( AF_INET6, (struct in6_addr *)&conn->ipv6.remote_addr, remote_addr_str, INET6_ADDRSTRLEN );
            fprintf( stderr, "{family=AF_INET6,owner=%04x,state=%s,local=[%s%%%d]:%d,remote=[%s%%%d]:%d}",
                     conn->ipv6.owner, state_names[conn->ipv6.state],
                     local_addr_str, conn->ipv6.local_scope_id, conn->ipv6.local_port,
                     remote_addr_str, conn->ipv6.remote_scope_id, conn->ipv6.remote_port );
        }

        size -= sizeof(*conn);
        remove_data( sizeof(*conn) );
        if (size) fputc( ',', stderr );
    }
    fputc( '}', stderr );
}

static void dump_varargs_udp_endpoints( const char *prefix, data_size_t size )
{
    const union udp_endpoint *endpt;

    fprintf( stderr, "%s{", prefix );
    while (size >= sizeof(*endpt))
    {
        endpt = cur_data;

        if (endpt->common.family == WS_AF_INET)
        {
            char addr_str[INET_ADDRSTRLEN] = { 0 };
            inet_ntop( AF_INET, (struct in_addr *)&endpt->ipv4.addr, addr_str, INET_ADDRSTRLEN );
            fprintf( stderr, "{family=AF_INET,owner=%04x,addr=%s:%d}",
                     endpt->ipv4.owner, addr_str, endpt->ipv4.port );
        }
        else
        {
            char addr_str[INET6_ADDRSTRLEN];
            inet_ntop( AF_INET6, (struct in6_addr *)&endpt->ipv6.addr, addr_str, INET6_ADDRSTRLEN );
            fprintf( stderr, "{family=AF_INET6,owner=%04x,addr=[%s%%%d]:%d}",
                     endpt->ipv6.owner, addr_str, endpt->ipv6.scope_id, endpt->ipv6.port );
        }

        size -= sizeof(*endpt);
        remove_data( sizeof(*endpt) );
        if (size) fputc( ',', stderr );
    }
    fputc( '}', stderr );
}

static void dump_varargs_directory_entries( const char *prefix, data_size_t size )
{
    fprintf( stderr, "%s{", prefix );
    while (size)
    {
        const struct directory_entry *entry = cur_data;
        data_size_t entry_size;
        const char *next;

        if (size < sizeof(*entry) ||
            (size - sizeof(*entry) < entry->name_len) ||
            (size - sizeof(*entry) - entry->name_len < entry->type_len))
        {
            fprintf( stderr, "***invalid***}" );
            remove_data( size );
            return;
        }

        next = (const char *)(entry + 1);
        fprintf( stderr, "{name=L\"" );
        dump_strW( (const WCHAR *)next, entry->name_len, stderr, "\"\"" );
        next += entry->name_len;
        fprintf( stderr, "\",type=L\"" );
        dump_strW( (const WCHAR *)next, entry->type_len, stderr, "\"\"" );
        fprintf( stderr, "\"}" );

        entry_size = min( size, (sizeof(*entry) + entry->name_len + entry->type_len + 3) & ~3 );
        size -= entry_size;
        remove_data( entry_size );
        if (size) fputc( ',', stderr );
    }
    fputc( '}', stderr );
}

static void dump_varargs_monitor_infos( const char *prefix, data_size_t size )
{
    const struct monitor_info *monitor = cur_data;
    data_size_t len = size / sizeof(*monitor);

    fprintf( stderr,"%s{", prefix );
    while (len > 0)
    {
        dump_rectangle( "{raw:", &monitor->virt );
        dump_rectangle( ",virt:", &monitor->virt );
        fprintf( stderr, ",flags:%#x,dpi:%u", monitor->flags, monitor->dpi );
        fputc( '}', stderr );
        if (--len) fputc( ',', stderr );
    }
    fputc( '}', stderr );
    remove_data( size );
}

void trace_request(void)
{
    enum request req = current->req.request_header.req;
    if (req < REQ_NB_REQUESTS)
    {
        fprintf( stderr, "%04x: %s(", current->id, req_names[req] );
        if (req_dumpers[req])
        {
            cur_data = get_req_data();
            cur_size = get_req_data_size();
            req_dumpers[req]( &current->req );
        }
        fprintf( stderr, " )\n" );
    }
    else fprintf( stderr, "%04x: %d(?)\n", current->id, req );
}

void trace_reply( enum request req, const union generic_reply *reply )
{
    if (req < REQ_NB_REQUESTS)
    {
        fprintf( stderr, "%04x: %s() = %s",
                 current->id, req_names[req], get_status_name(current->error) );
        if (reply_dumpers[req])
        {
            fprintf( stderr, " {" );
            cur_data = current->reply_data;
            cur_size = reply->reply_header.reply_size;
            reply_dumpers[req]( reply );
            fprintf( stderr, " }" );
        }
        fputc( '\n', stderr );
    }
    else fprintf( stderr, "%04x: %d() = %s\n",
                  current->id, req, get_status_name(current->error) );
}
