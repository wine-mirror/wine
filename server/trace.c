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
#include "file.h"
#include "request.h"
#include "security.h"
#include "unicode.h"

static const void *cur_data;
static data_size_t cur_size;

static const char *get_status_name( unsigned int status );

/* utility functions */

static inline void remove_data( data_size_t size )
{
    cur_data = (const char *)cur_data + size;
    cur_size -= size;
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

static void dump_rectangle( const char *prefix, const rectangle_t *rect )
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

static void dump_apc_call( const char *prefix, const apc_call_t *call )
{
    fprintf( stderr, "%s{", prefix );
    switch(call->type)
    {
    case APC_NONE:
        fprintf( stderr, "APC_NONE" );
        break;
    case APC_USER:
        dump_uint64( "APC_USER,func=", &call->user.func );
        dump_uint64( ",args={", &call->user.args[0] );
        dump_uint64( ",", &call->user.args[1] );
        dump_uint64( ",", &call->user.args[2] );
        fputc( '}', stderr );
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

static void dump_apc_result( const char *prefix, const apc_result_t *result )
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

static void dump_async_data( const char *prefix, const async_data_t *data )
{
    fprintf( stderr, "%s{handle=%04x,event=%04x", prefix, data->handle, data->event );
    dump_uint64( ",iosb=", &data->iosb );
    dump_uint64( ",user=", &data->user );
    dump_uint64( ",apc=", &data->apc );
    dump_uint64( ",apc_context=", &data->apc_context );
    fputc( '}', stderr );
}

static void dump_irp_params( const char *prefix, const irp_params_t *data )
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

static void dump_hw_input( const char *prefix, const hw_input_t *input )
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

static void dump_luid( const char *prefix, const struct luid *luid )
{
    fprintf( stderr, "%s%d.%u", prefix, luid->high_part, luid->low_part );
}

static void dump_generic_map( const char *prefix, const generic_map_t *map )
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
    data_size_t len = size / sizeof(*data);

    fprintf( stderr,"%s{", prefix );
    while (len > 0)
    {
        dump_uint64( "", data++ );
        if (--len) fputc( ',', stderr );
    }
    fputc( '}', stderr );
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
    const apc_call_t *call = cur_data;

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
    const apc_result_t *result = cur_data;

    if (size >= sizeof(*result))
    {
        dump_apc_result( prefix, result );
        size = sizeof(*result);
    }
    remove_data( size );
}

static void dump_varargs_select_op( const char *prefix, data_size_t size )
{
    select_op_t data;

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
        if (size > offsetof( select_op_t, wait.handles ))
            dump_handles( ",handles=", data.wait.handles,
                          min( size, sizeof(data.wait) ) - offsetof( select_op_t, wait.handles ));
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

static void dump_varargs_context( const char *prefix, data_size_t size )
{
    const context_t *context = cur_data;
    context_t ctx;
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
    debug_event_t event;
    unsigned int i;

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
        fprintf( stderr, ",params={" );
        event.exception.nb_params = min( event.exception.nb_params, EXCEPTION_MAXIMUM_PARAMETERS );
        for (i = 0; i < event.exception.nb_params; i++)
        {
            dump_uint64( "", &event.exception.params[i] );
            if (i < event.exception.nb_params) fputc( ',', stderr );
        }
        fprintf( stderr, "}}" );
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
    startup_info_t info;
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
    pos = dump_inline_unicode_string( "\",runtime=L\"", pos, info.runtime_len, size );
    fprintf( stderr, "\"}" );
    remove_data( size );
}

static void dump_varargs_rectangles( const char *prefix, data_size_t size )
{
    const rectangle_t *rect = cur_data;
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
    const cursor_pos_t *pos = cur_data;
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
    const property_data_t *prop = cur_data;
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
        offset += sd->dacl_len;
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
    pe_image_info_t info;

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

typedef void (*dump_func)( const void *req );

/* Everything below this line is generated automatically by tools/make_requests */
/* ### make_requests begin ### */

static void dump_new_process_request( const struct new_process_request *req )
{
    fprintf( stderr, " token=%04x", req->token );
    fprintf( stderr, ", debug=%04x", req->debug );
    fprintf( stderr, ", parent_process=%04x", req->parent_process );
    fprintf( stderr, ", flags=%08x", req->flags );
    fprintf( stderr, ", socket_fd=%d", req->socket_fd );
    fprintf( stderr, ", access=%08x", req->access );
    fprintf( stderr, ", machine=%04x", req->machine );
    fprintf( stderr, ", info_size=%u", req->info_size );
    fprintf( stderr, ", handles_size=%u", req->handles_size );
    fprintf( stderr, ", jobs_size=%u", req->jobs_size );
    dump_varargs_object_attributes( ", objattr=", cur_size );
    dump_varargs_uints( ", handles=", min(cur_size,req->handles_size) );
    dump_varargs_uints( ", jobs=", min(cur_size,req->jobs_size) );
    dump_varargs_startup_info( ", info=", min(cur_size,req->info_size) );
    dump_varargs_unicode_str( ", env=", cur_size );
}

static void dump_new_process_reply( const struct new_process_reply *req )
{
    fprintf( stderr, " info=%04x", req->info );
    fprintf( stderr, ", pid=%04x", req->pid );
    fprintf( stderr, ", handle=%04x", req->handle );
}

static void dump_get_new_process_info_request( const struct get_new_process_info_request *req )
{
    fprintf( stderr, " info=%04x", req->info );
}

static void dump_get_new_process_info_reply( const struct get_new_process_info_reply *req )
{
    fprintf( stderr, " success=%d", req->success );
    fprintf( stderr, ", exit_code=%d", req->exit_code );
}

static void dump_new_thread_request( const struct new_thread_request *req )
{
    fprintf( stderr, " process=%04x", req->process );
    fprintf( stderr, ", access=%08x", req->access );
    fprintf( stderr, ", flags=%08x", req->flags );
    fprintf( stderr, ", request_fd=%d", req->request_fd );
    dump_varargs_object_attributes( ", objattr=", cur_size );
}

static void dump_new_thread_reply( const struct new_thread_reply *req )
{
    fprintf( stderr, " tid=%04x", req->tid );
    fprintf( stderr, ", handle=%04x", req->handle );
}

static void dump_get_startup_info_request( const struct get_startup_info_request *req )
{
}

static void dump_get_startup_info_reply( const struct get_startup_info_reply *req )
{
    fprintf( stderr, " info_size=%u", req->info_size );
    fprintf( stderr, ", machine=%04x", req->machine );
    dump_varargs_startup_info( ", info=", min(cur_size,req->info_size) );
    dump_varargs_unicode_str( ", env=", cur_size );
}

static void dump_init_process_done_request( const struct init_process_done_request *req )
{
    dump_uint64( " teb=", &req->teb );
    dump_uint64( ", peb=", &req->peb );
    dump_uint64( ", ldt_copy=", &req->ldt_copy );
}

static void dump_init_process_done_reply( const struct init_process_done_reply *req )
{
    fprintf( stderr, " suspend=%d", req->suspend );
}

static void dump_init_first_thread_request( const struct init_first_thread_request *req )
{
    fprintf( stderr, " unix_pid=%d", req->unix_pid );
    fprintf( stderr, ", unix_tid=%d", req->unix_tid );
    fprintf( stderr, ", debug_level=%d", req->debug_level );
    fprintf( stderr, ", reply_fd=%d", req->reply_fd );
    fprintf( stderr, ", wait_fd=%d", req->wait_fd );
}

static void dump_init_first_thread_reply( const struct init_first_thread_reply *req )
{
    fprintf( stderr, " pid=%04x", req->pid );
    fprintf( stderr, ", tid=%04x", req->tid );
    dump_timeout( ", server_start=", &req->server_start );
    fprintf( stderr, ", session_id=%08x", req->session_id );
    fprintf( stderr, ", info_size=%u", req->info_size );
    dump_varargs_ushorts( ", machines=", cur_size );
}

static void dump_init_thread_request( const struct init_thread_request *req )
{
    fprintf( stderr, " unix_tid=%d", req->unix_tid );
    fprintf( stderr, ", reply_fd=%d", req->reply_fd );
    fprintf( stderr, ", wait_fd=%d", req->wait_fd );
    dump_uint64( ", teb=", &req->teb );
    dump_uint64( ", entry=", &req->entry );
}

static void dump_init_thread_reply( const struct init_thread_reply *req )
{
    fprintf( stderr, " suspend=%d", req->suspend );
}

static void dump_terminate_process_request( const struct terminate_process_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", exit_code=%d", req->exit_code );
}

static void dump_terminate_process_reply( const struct terminate_process_reply *req )
{
    fprintf( stderr, " self=%d", req->self );
}

static void dump_terminate_thread_request( const struct terminate_thread_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", exit_code=%d", req->exit_code );
}

static void dump_terminate_thread_reply( const struct terminate_thread_reply *req )
{
    fprintf( stderr, " self=%d", req->self );
}

static void dump_get_process_info_request( const struct get_process_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_process_info_reply( const struct get_process_info_reply *req )
{
    fprintf( stderr, " pid=%04x", req->pid );
    fprintf( stderr, ", ppid=%04x", req->ppid );
    dump_uint64( ", affinity=", &req->affinity );
    dump_uint64( ", peb=", &req->peb );
    dump_timeout( ", start_time=", &req->start_time );
    dump_timeout( ", end_time=", &req->end_time );
    fprintf( stderr, ", session_id=%08x", req->session_id );
    fprintf( stderr, ", exit_code=%d", req->exit_code );
    fprintf( stderr, ", priority=%d", req->priority );
    fprintf( stderr, ", machine=%04x", req->machine );
    dump_varargs_pe_image_info( ", image=", cur_size );
}

static void dump_get_process_debug_info_request( const struct get_process_debug_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_process_debug_info_reply( const struct get_process_debug_info_reply *req )
{
    fprintf( stderr, " debug=%04x", req->debug );
    fprintf( stderr, ", debug_children=%d", req->debug_children );
    dump_varargs_pe_image_info( ", image=", cur_size );
}

static void dump_get_process_image_name_request( const struct get_process_image_name_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", win32=%d", req->win32 );
}

static void dump_get_process_image_name_reply( const struct get_process_image_name_reply *req )
{
    fprintf( stderr, " len=%u", req->len );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_get_process_vm_counters_request( const struct get_process_vm_counters_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_process_vm_counters_reply( const struct get_process_vm_counters_reply *req )
{
    dump_uint64( " peak_virtual_size=", &req->peak_virtual_size );
    dump_uint64( ", virtual_size=", &req->virtual_size );
    dump_uint64( ", peak_working_set_size=", &req->peak_working_set_size );
    dump_uint64( ", working_set_size=", &req->working_set_size );
    dump_uint64( ", pagefile_usage=", &req->pagefile_usage );
    dump_uint64( ", peak_pagefile_usage=", &req->peak_pagefile_usage );
}

static void dump_set_process_info_request( const struct set_process_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", mask=%d", req->mask );
    fprintf( stderr, ", priority=%d", req->priority );
    dump_uint64( ", affinity=", &req->affinity );
}

static void dump_get_thread_info_request( const struct get_thread_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", access=%08x", req->access );
}

static void dump_get_thread_info_reply( const struct get_thread_info_reply *req )
{
    fprintf( stderr, " pid=%04x", req->pid );
    fprintf( stderr, ", tid=%04x", req->tid );
    dump_uint64( ", teb=", &req->teb );
    dump_uint64( ", entry_point=", &req->entry_point );
    dump_uint64( ", affinity=", &req->affinity );
    fprintf( stderr, ", exit_code=%d", req->exit_code );
    fprintf( stderr, ", priority=%d", req->priority );
    fprintf( stderr, ", last=%d", req->last );
    fprintf( stderr, ", suspend_count=%d", req->suspend_count );
    fprintf( stderr, ", flags=%08x", req->flags );
    fprintf( stderr, ", desc_len=%u", req->desc_len );
    dump_varargs_unicode_str( ", desc=", cur_size );
}

static void dump_get_thread_times_request( const struct get_thread_times_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_thread_times_reply( const struct get_thread_times_reply *req )
{
    dump_timeout( " creation_time=", &req->creation_time );
    dump_timeout( ", exit_time=", &req->exit_time );
    fprintf( stderr, ", unix_pid=%d", req->unix_pid );
    fprintf( stderr, ", unix_tid=%d", req->unix_tid );
}

static void dump_set_thread_info_request( const struct set_thread_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", mask=%d", req->mask );
    fprintf( stderr, ", priority=%d", req->priority );
    dump_uint64( ", affinity=", &req->affinity );
    dump_uint64( ", entry_point=", &req->entry_point );
    fprintf( stderr, ", token=%04x", req->token );
    dump_varargs_unicode_str( ", desc=", cur_size );
}

static void dump_suspend_thread_request( const struct suspend_thread_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_suspend_thread_reply( const struct suspend_thread_reply *req )
{
    fprintf( stderr, " count=%d", req->count );
}

static void dump_resume_thread_request( const struct resume_thread_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_resume_thread_reply( const struct resume_thread_reply *req )
{
    fprintf( stderr, " count=%d", req->count );
}

static void dump_queue_apc_request( const struct queue_apc_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    dump_varargs_apc_call( ", call=", cur_size );
}

static void dump_queue_apc_reply( const struct queue_apc_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", self=%d", req->self );
}

static void dump_get_apc_result_request( const struct get_apc_result_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_apc_result_reply( const struct get_apc_result_reply *req )
{
    dump_apc_result( " result=", &req->result );
}

static void dump_close_handle_request( const struct close_handle_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_set_handle_info_request( const struct set_handle_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", flags=%d", req->flags );
    fprintf( stderr, ", mask=%d", req->mask );
}

static void dump_set_handle_info_reply( const struct set_handle_info_reply *req )
{
    fprintf( stderr, " old_flags=%d", req->old_flags );
}

static void dump_dup_handle_request( const struct dup_handle_request *req )
{
    fprintf( stderr, " src_process=%04x", req->src_process );
    fprintf( stderr, ", src_handle=%04x", req->src_handle );
    fprintf( stderr, ", dst_process=%04x", req->dst_process );
    fprintf( stderr, ", access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", options=%08x", req->options );
}

static void dump_dup_handle_reply( const struct dup_handle_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_compare_objects_request( const struct compare_objects_request *req )
{
    fprintf( stderr, " first=%04x", req->first );
    fprintf( stderr, ", second=%04x", req->second );
}

static void dump_make_temporary_request( const struct make_temporary_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_open_process_request( const struct open_process_request *req )
{
    fprintf( stderr, " pid=%04x", req->pid );
    fprintf( stderr, ", access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
}

static void dump_open_process_reply( const struct open_process_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_open_thread_request( const struct open_thread_request *req )
{
    fprintf( stderr, " tid=%04x", req->tid );
    fprintf( stderr, ", access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
}

static void dump_open_thread_reply( const struct open_thread_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_select_request( const struct select_request *req )
{
    fprintf( stderr, " flags=%d", req->flags );
    dump_uint64( ", cookie=", &req->cookie );
    dump_abstime( ", timeout=", &req->timeout );
    fprintf( stderr, ", size=%u", req->size );
    fprintf( stderr, ", prev_apc=%04x", req->prev_apc );
    dump_varargs_apc_result( ", result=", cur_size );
    dump_varargs_select_op( ", data=", min(cur_size,req->size) );
    dump_varargs_contexts( ", contexts=", cur_size );
}

static void dump_select_reply( const struct select_reply *req )
{
    fprintf( stderr, " apc_handle=%04x", req->apc_handle );
    fprintf( stderr, ", signaled=%d", req->signaled );
    dump_varargs_apc_call( ", call=", cur_size );
    dump_varargs_contexts( ", contexts=", cur_size );
}

static void dump_create_event_request( const struct create_event_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", manual_reset=%d", req->manual_reset );
    fprintf( stderr, ", initial_state=%d", req->initial_state );
    dump_varargs_object_attributes( ", objattr=", cur_size );
}

static void dump_create_event_reply( const struct create_event_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_event_op_request( const struct event_op_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", op=%d", req->op );
}

static void dump_event_op_reply( const struct event_op_reply *req )
{
    fprintf( stderr, " state=%d", req->state );
}

static void dump_query_event_request( const struct query_event_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_query_event_reply( const struct query_event_reply *req )
{
    fprintf( stderr, " manual_reset=%d", req->manual_reset );
    fprintf( stderr, ", state=%d", req->state );
}

static void dump_open_event_request( const struct open_event_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", rootdir=%04x", req->rootdir );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_open_event_reply( const struct open_event_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_create_keyed_event_request( const struct create_keyed_event_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    dump_varargs_object_attributes( ", objattr=", cur_size );
}

static void dump_create_keyed_event_reply( const struct create_keyed_event_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_open_keyed_event_request( const struct open_keyed_event_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", rootdir=%04x", req->rootdir );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_open_keyed_event_reply( const struct open_keyed_event_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_create_mutex_request( const struct create_mutex_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", owned=%d", req->owned );
    dump_varargs_object_attributes( ", objattr=", cur_size );
}

static void dump_create_mutex_reply( const struct create_mutex_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_release_mutex_request( const struct release_mutex_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_release_mutex_reply( const struct release_mutex_reply *req )
{
    fprintf( stderr, " prev_count=%08x", req->prev_count );
}

static void dump_open_mutex_request( const struct open_mutex_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", rootdir=%04x", req->rootdir );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_open_mutex_reply( const struct open_mutex_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_query_mutex_request( const struct query_mutex_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_query_mutex_reply( const struct query_mutex_reply *req )
{
    fprintf( stderr, " count=%08x", req->count );
    fprintf( stderr, ", owned=%d", req->owned );
    fprintf( stderr, ", abandoned=%d", req->abandoned );
}

static void dump_create_semaphore_request( const struct create_semaphore_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", initial=%08x", req->initial );
    fprintf( stderr, ", max=%08x", req->max );
    dump_varargs_object_attributes( ", objattr=", cur_size );
}

static void dump_create_semaphore_reply( const struct create_semaphore_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_release_semaphore_request( const struct release_semaphore_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", count=%08x", req->count );
}

static void dump_release_semaphore_reply( const struct release_semaphore_reply *req )
{
    fprintf( stderr, " prev_count=%08x", req->prev_count );
}

static void dump_query_semaphore_request( const struct query_semaphore_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_query_semaphore_reply( const struct query_semaphore_reply *req )
{
    fprintf( stderr, " current=%08x", req->current );
    fprintf( stderr, ", max=%08x", req->max );
}

static void dump_open_semaphore_request( const struct open_semaphore_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", rootdir=%04x", req->rootdir );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_open_semaphore_reply( const struct open_semaphore_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_create_file_request( const struct create_file_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", sharing=%08x", req->sharing );
    fprintf( stderr, ", create=%d", req->create );
    fprintf( stderr, ", options=%08x", req->options );
    fprintf( stderr, ", attrs=%08x", req->attrs );
    dump_varargs_object_attributes( ", objattr=", cur_size );
    dump_varargs_string( ", filename=", cur_size );
}

static void dump_create_file_reply( const struct create_file_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_open_file_object_request( const struct open_file_object_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", rootdir=%04x", req->rootdir );
    fprintf( stderr, ", sharing=%08x", req->sharing );
    fprintf( stderr, ", options=%08x", req->options );
    dump_varargs_unicode_str( ", filename=", cur_size );
}

static void dump_open_file_object_reply( const struct open_file_object_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_alloc_file_handle_request( const struct alloc_file_handle_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", fd=%d", req->fd );
}

static void dump_alloc_file_handle_reply( const struct alloc_file_handle_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_handle_unix_name_request( const struct get_handle_unix_name_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_handle_unix_name_reply( const struct get_handle_unix_name_reply *req )
{
    fprintf( stderr, " name_len=%u", req->name_len );
    dump_varargs_string( ", name=", cur_size );
}

static void dump_get_handle_fd_request( const struct get_handle_fd_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_handle_fd_reply( const struct get_handle_fd_reply *req )
{
    fprintf( stderr, " type=%d", req->type );
    fprintf( stderr, ", cacheable=%d", req->cacheable );
    fprintf( stderr, ", access=%08x", req->access );
    fprintf( stderr, ", options=%08x", req->options );
}

static void dump_get_directory_cache_entry_request( const struct get_directory_cache_entry_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_directory_cache_entry_reply( const struct get_directory_cache_entry_reply *req )
{
    fprintf( stderr, " entry=%d", req->entry );
    dump_varargs_ints( ", free=", cur_size );
}

static void dump_flush_request( const struct flush_request *req )
{
    dump_async_data( " async=", &req->async );
}

static void dump_flush_reply( const struct flush_reply *req )
{
    fprintf( stderr, " event=%04x", req->event );
}

static void dump_get_file_info_request( const struct get_file_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", info_class=%08x", req->info_class );
}

static void dump_get_file_info_reply( const struct get_file_info_reply *req )
{
    dump_varargs_bytes( " data=", cur_size );
}

static void dump_get_volume_info_request( const struct get_volume_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    dump_async_data( ", async=", &req->async );
    fprintf( stderr, ", info_class=%08x", req->info_class );
}

static void dump_get_volume_info_reply( const struct get_volume_info_reply *req )
{
    fprintf( stderr, " wait=%04x", req->wait );
    dump_varargs_bytes( ", data=", cur_size );
}

static void dump_lock_file_request( const struct lock_file_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    dump_uint64( ", offset=", &req->offset );
    dump_uint64( ", count=", &req->count );
    fprintf( stderr, ", shared=%d", req->shared );
    fprintf( stderr, ", wait=%d", req->wait );
}

static void dump_lock_file_reply( const struct lock_file_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", overlapped=%d", req->overlapped );
}

static void dump_unlock_file_request( const struct unlock_file_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    dump_uint64( ", offset=", &req->offset );
    dump_uint64( ", count=", &req->count );
}

static void dump_recv_socket_request( const struct recv_socket_request *req )
{
    fprintf( stderr, " oob=%d", req->oob );
    dump_async_data( ", async=", &req->async );
    fprintf( stderr, ", force_async=%d", req->force_async );
}

static void dump_recv_socket_reply( const struct recv_socket_reply *req )
{
    fprintf( stderr, " wait=%04x", req->wait );
    fprintf( stderr, ", options=%08x", req->options );
    fprintf( stderr, ", nonblocking=%d", req->nonblocking );
}

static void dump_send_socket_request( const struct send_socket_request *req )
{
    dump_async_data( " async=", &req->async );
    fprintf( stderr, ", force_async=%d", req->force_async );
}

static void dump_send_socket_reply( const struct send_socket_reply *req )
{
    fprintf( stderr, " wait=%04x", req->wait );
    fprintf( stderr, ", options=%08x", req->options );
    fprintf( stderr, ", nonblocking=%d", req->nonblocking );
}

static void dump_socket_get_events_request( const struct socket_get_events_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", event=%04x", req->event );
}

static void dump_socket_get_events_reply( const struct socket_get_events_reply *req )
{
    fprintf( stderr, " flags=%08x", req->flags );
    dump_varargs_uints( ", status=", cur_size );
}

static void dump_socket_send_icmp_id_request( const struct socket_send_icmp_id_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", icmp_id=%04x", req->icmp_id );
    fprintf( stderr, ", icmp_seq=%04x", req->icmp_seq );
}

static void dump_socket_get_icmp_id_request( const struct socket_get_icmp_id_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", icmp_seq=%04x", req->icmp_seq );
}

static void dump_socket_get_icmp_id_reply( const struct socket_get_icmp_id_reply *req )
{
    fprintf( stderr, " icmp_id=%04x", req->icmp_id );
}

static void dump_get_next_console_request_request( const struct get_next_console_request_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", signal=%d", req->signal );
    fprintf( stderr, ", read=%d", req->read );
    fprintf( stderr, ", status=%08x", req->status );
    dump_varargs_bytes( ", out_data=", cur_size );
}

static void dump_get_next_console_request_reply( const struct get_next_console_request_reply *req )
{
    fprintf( stderr, " code=%08x", req->code );
    fprintf( stderr, ", output=%08x", req->output );
    fprintf( stderr, ", out_size=%u", req->out_size );
    dump_varargs_bytes( ", in_data=", cur_size );
}

static void dump_read_directory_changes_request( const struct read_directory_changes_request *req )
{
    fprintf( stderr, " filter=%08x", req->filter );
    fprintf( stderr, ", subtree=%d", req->subtree );
    fprintf( stderr, ", want_data=%d", req->want_data );
    dump_async_data( ", async=", &req->async );
}

static void dump_read_change_request( const struct read_change_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_read_change_reply( const struct read_change_reply *req )
{
    dump_varargs_filesystem_event( " events=", cur_size );
}

static void dump_create_mapping_request( const struct create_mapping_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", flags=%08x", req->flags );
    fprintf( stderr, ", file_access=%08x", req->file_access );
    dump_uint64( ", size=", &req->size );
    fprintf( stderr, ", file_handle=%04x", req->file_handle );
    dump_varargs_object_attributes( ", objattr=", cur_size );
}

static void dump_create_mapping_reply( const struct create_mapping_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_open_mapping_request( const struct open_mapping_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", rootdir=%04x", req->rootdir );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_open_mapping_reply( const struct open_mapping_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_mapping_info_request( const struct get_mapping_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", access=%08x", req->access );
}

static void dump_get_mapping_info_reply( const struct get_mapping_info_reply *req )
{
    dump_uint64( " size=", &req->size );
    fprintf( stderr, ", flags=%08x", req->flags );
    fprintf( stderr, ", shared_file=%04x", req->shared_file );
    fprintf( stderr, ", total=%u", req->total );
    dump_varargs_pe_image_info( ", image=", cur_size );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_get_image_map_address_request( const struct get_image_map_address_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_image_map_address_reply( const struct get_image_map_address_reply *req )
{
    dump_uint64( " addr=", &req->addr );
}

static void dump_map_view_request( const struct map_view_request *req )
{
    fprintf( stderr, " mapping=%04x", req->mapping );
    fprintf( stderr, ", access=%08x", req->access );
    dump_uint64( ", base=", &req->base );
    dump_uint64( ", size=", &req->size );
    dump_uint64( ", start=", &req->start );
}

static void dump_map_image_view_request( const struct map_image_view_request *req )
{
    fprintf( stderr, " mapping=%04x", req->mapping );
    dump_uint64( ", base=", &req->base );
    dump_uint64( ", size=", &req->size );
    fprintf( stderr, ", entry=%08x", req->entry );
    fprintf( stderr, ", machine=%04x", req->machine );
}

static void dump_map_builtin_view_request( const struct map_builtin_view_request *req )
{
    dump_varargs_pe_image_info( " image=", cur_size );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_get_image_view_info_request( const struct get_image_view_info_request *req )
{
    fprintf( stderr, " process=%04x", req->process );
    dump_uint64( ", addr=", &req->addr );
}

static void dump_get_image_view_info_reply( const struct get_image_view_info_reply *req )
{
    dump_uint64( " base=", &req->base );
    dump_uint64( ", size=", &req->size );
}

static void dump_unmap_view_request( const struct unmap_view_request *req )
{
    dump_uint64( " base=", &req->base );
}

static void dump_get_mapping_committed_range_request( const struct get_mapping_committed_range_request *req )
{
    dump_uint64( " base=", &req->base );
    dump_uint64( ", offset=", &req->offset );
}

static void dump_get_mapping_committed_range_reply( const struct get_mapping_committed_range_reply *req )
{
    dump_uint64( " size=", &req->size );
    fprintf( stderr, ", committed=%d", req->committed );
}

static void dump_add_mapping_committed_range_request( const struct add_mapping_committed_range_request *req )
{
    dump_uint64( " base=", &req->base );
    dump_uint64( ", offset=", &req->offset );
    dump_uint64( ", size=", &req->size );
}

static void dump_is_same_mapping_request( const struct is_same_mapping_request *req )
{
    dump_uint64( " base1=", &req->base1 );
    dump_uint64( ", base2=", &req->base2 );
}

static void dump_get_mapping_filename_request( const struct get_mapping_filename_request *req )
{
    fprintf( stderr, " process=%04x", req->process );
    dump_uint64( ", addr=", &req->addr );
}

static void dump_get_mapping_filename_reply( const struct get_mapping_filename_reply *req )
{
    fprintf( stderr, " len=%u", req->len );
    dump_varargs_unicode_str( ", filename=", cur_size );
}

static void dump_list_processes_request( const struct list_processes_request *req )
{
}

static void dump_list_processes_reply( const struct list_processes_reply *req )
{
    fprintf( stderr, " info_size=%u", req->info_size );
    fprintf( stderr, ", process_count=%d", req->process_count );
    fprintf( stderr, ", total_thread_count=%d", req->total_thread_count );
    fprintf( stderr, ", total_name_len=%u", req->total_name_len );
    dump_varargs_process_info( ", data=", min(cur_size,req->info_size) );
}

static void dump_create_debug_obj_request( const struct create_debug_obj_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", flags=%08x", req->flags );
    dump_varargs_object_attributes( ", objattr=", cur_size );
}

static void dump_create_debug_obj_reply( const struct create_debug_obj_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_wait_debug_event_request( const struct wait_debug_event_request *req )
{
    fprintf( stderr, " debug=%04x", req->debug );
}

static void dump_wait_debug_event_reply( const struct wait_debug_event_reply *req )
{
    fprintf( stderr, " pid=%04x", req->pid );
    fprintf( stderr, ", tid=%04x", req->tid );
    dump_varargs_debug_event( ", event=", cur_size );
}

static void dump_queue_exception_event_request( const struct queue_exception_event_request *req )
{
    fprintf( stderr, " first=%d", req->first );
    fprintf( stderr, ", code=%08x", req->code );
    fprintf( stderr, ", flags=%08x", req->flags );
    dump_uint64( ", record=", &req->record );
    dump_uint64( ", address=", &req->address );
    fprintf( stderr, ", len=%u", req->len );
    dump_varargs_uints64( ", params=", min(cur_size,req->len) );
}

static void dump_queue_exception_event_reply( const struct queue_exception_event_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_exception_status_request( const struct get_exception_status_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_continue_debug_event_request( const struct continue_debug_event_request *req )
{
    fprintf( stderr, " debug=%04x", req->debug );
    fprintf( stderr, ", pid=%04x", req->pid );
    fprintf( stderr, ", tid=%04x", req->tid );
    fprintf( stderr, ", status=%08x", req->status );
}

static void dump_debug_process_request( const struct debug_process_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", debug=%04x", req->debug );
    fprintf( stderr, ", attach=%d", req->attach );
}

static void dump_set_debug_obj_info_request( const struct set_debug_obj_info_request *req )
{
    fprintf( stderr, " debug=%04x", req->debug );
    fprintf( stderr, ", flags=%08x", req->flags );
}

static void dump_read_process_memory_request( const struct read_process_memory_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    dump_uint64( ", addr=", &req->addr );
}

static void dump_read_process_memory_reply( const struct read_process_memory_reply *req )
{
    dump_varargs_bytes( " data=", cur_size );
}

static void dump_write_process_memory_request( const struct write_process_memory_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    dump_uint64( ", addr=", &req->addr );
    dump_varargs_bytes( ", data=", cur_size );
}

static void dump_create_key_request( const struct create_key_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", options=%08x", req->options );
    dump_varargs_object_attributes( ", objattr=", cur_size );
    dump_varargs_unicode_str( ", class=", cur_size );
}

static void dump_create_key_reply( const struct create_key_reply *req )
{
    fprintf( stderr, " hkey=%04x", req->hkey );
}

static void dump_open_key_request( const struct open_key_request *req )
{
    fprintf( stderr, " parent=%04x", req->parent );
    fprintf( stderr, ", access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_open_key_reply( const struct open_key_reply *req )
{
    fprintf( stderr, " hkey=%04x", req->hkey );
}

static void dump_delete_key_request( const struct delete_key_request *req )
{
    fprintf( stderr, " hkey=%04x", req->hkey );
}

static void dump_flush_key_request( const struct flush_key_request *req )
{
    fprintf( stderr, " hkey=%04x", req->hkey );
}

static void dump_enum_key_request( const struct enum_key_request *req )
{
    fprintf( stderr, " hkey=%04x", req->hkey );
    fprintf( stderr, ", index=%d", req->index );
    fprintf( stderr, ", info_class=%d", req->info_class );
}

static void dump_enum_key_reply( const struct enum_key_reply *req )
{
    fprintf( stderr, " subkeys=%d", req->subkeys );
    fprintf( stderr, ", max_subkey=%d", req->max_subkey );
    fprintf( stderr, ", max_class=%d", req->max_class );
    fprintf( stderr, ", values=%d", req->values );
    fprintf( stderr, ", max_value=%d", req->max_value );
    fprintf( stderr, ", max_data=%d", req->max_data );
    dump_timeout( ", modif=", &req->modif );
    fprintf( stderr, ", total=%u", req->total );
    fprintf( stderr, ", namelen=%u", req->namelen );
    dump_varargs_unicode_str( ", name=", min(cur_size,req->namelen) );
    dump_varargs_unicode_str( ", class=", cur_size );
}

static void dump_set_key_value_request( const struct set_key_value_request *req )
{
    fprintf( stderr, " hkey=%04x", req->hkey );
    fprintf( stderr, ", type=%d", req->type );
    fprintf( stderr, ", namelen=%u", req->namelen );
    dump_varargs_unicode_str( ", name=", min(cur_size,req->namelen) );
    dump_varargs_bytes( ", data=", cur_size );
}

static void dump_get_key_value_request( const struct get_key_value_request *req )
{
    fprintf( stderr, " hkey=%04x", req->hkey );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_get_key_value_reply( const struct get_key_value_reply *req )
{
    fprintf( stderr, " type=%d", req->type );
    fprintf( stderr, ", total=%u", req->total );
    dump_varargs_bytes( ", data=", cur_size );
}

static void dump_enum_key_value_request( const struct enum_key_value_request *req )
{
    fprintf( stderr, " hkey=%04x", req->hkey );
    fprintf( stderr, ", index=%d", req->index );
    fprintf( stderr, ", info_class=%d", req->info_class );
}

static void dump_enum_key_value_reply( const struct enum_key_value_reply *req )
{
    fprintf( stderr, " type=%d", req->type );
    fprintf( stderr, ", total=%u", req->total );
    fprintf( stderr, ", namelen=%u", req->namelen );
    dump_varargs_unicode_str( ", name=", min(cur_size,req->namelen) );
    dump_varargs_bytes( ", data=", cur_size );
}

static void dump_delete_key_value_request( const struct delete_key_value_request *req )
{
    fprintf( stderr, " hkey=%04x", req->hkey );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_load_registry_request( const struct load_registry_request *req )
{
    fprintf( stderr, " file=%04x", req->file );
    dump_varargs_object_attributes( ", objattr=", cur_size );
}

static void dump_unload_registry_request( const struct unload_registry_request *req )
{
    fprintf( stderr, " parent=%04x", req->parent );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_save_registry_request( const struct save_registry_request *req )
{
    fprintf( stderr, " hkey=%04x", req->hkey );
    fprintf( stderr, ", file=%04x", req->file );
}

static void dump_set_registry_notification_request( const struct set_registry_notification_request *req )
{
    fprintf( stderr, " hkey=%04x", req->hkey );
    fprintf( stderr, ", event=%04x", req->event );
    fprintf( stderr, ", subtree=%d", req->subtree );
    fprintf( stderr, ", filter=%08x", req->filter );
}

static void dump_rename_key_request( const struct rename_key_request *req )
{
    fprintf( stderr, " hkey=%04x", req->hkey );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_create_timer_request( const struct create_timer_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", manual=%d", req->manual );
    dump_varargs_object_attributes( ", objattr=", cur_size );
}

static void dump_create_timer_reply( const struct create_timer_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_open_timer_request( const struct open_timer_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", rootdir=%04x", req->rootdir );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_open_timer_reply( const struct open_timer_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_set_timer_request( const struct set_timer_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    dump_timeout( ", expire=", &req->expire );
    dump_uint64( ", callback=", &req->callback );
    dump_uint64( ", arg=", &req->arg );
    fprintf( stderr, ", period=%d", req->period );
}

static void dump_set_timer_reply( const struct set_timer_reply *req )
{
    fprintf( stderr, " signaled=%d", req->signaled );
}

static void dump_cancel_timer_request( const struct cancel_timer_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_cancel_timer_reply( const struct cancel_timer_reply *req )
{
    fprintf( stderr, " signaled=%d", req->signaled );
}

static void dump_get_timer_info_request( const struct get_timer_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_timer_info_reply( const struct get_timer_info_reply *req )
{
    dump_timeout( " when=", &req->when );
    fprintf( stderr, ", signaled=%d", req->signaled );
}

static void dump_get_thread_context_request( const struct get_thread_context_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", context=%04x", req->context );
    fprintf( stderr, ", flags=%08x", req->flags );
    fprintf( stderr, ", native_flags=%08x", req->native_flags );
    fprintf( stderr, ", machine=%04x", req->machine );
}

static void dump_get_thread_context_reply( const struct get_thread_context_reply *req )
{
    fprintf( stderr, " self=%d", req->self );
    fprintf( stderr, ", handle=%04x", req->handle );
    dump_varargs_contexts( ", contexts=", cur_size );
}

static void dump_set_thread_context_request( const struct set_thread_context_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", native_flags=%08x", req->native_flags );
    dump_varargs_contexts( ", contexts=", cur_size );
}

static void dump_set_thread_context_reply( const struct set_thread_context_reply *req )
{
    fprintf( stderr, " self=%d", req->self );
}

static void dump_get_selector_entry_request( const struct get_selector_entry_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", entry=%d", req->entry );
}

static void dump_get_selector_entry_reply( const struct get_selector_entry_reply *req )
{
    fprintf( stderr, " base=%08x", req->base );
    fprintf( stderr, ", limit=%08x", req->limit );
    fprintf( stderr, ", flags=%02x", req->flags );
}

static void dump_add_atom_request( const struct add_atom_request *req )
{
    dump_varargs_unicode_str( " name=", cur_size );
}

static void dump_add_atom_reply( const struct add_atom_reply *req )
{
    fprintf( stderr, " atom=%04x", req->atom );
}

static void dump_delete_atom_request( const struct delete_atom_request *req )
{
    fprintf( stderr, " atom=%04x", req->atom );
}

static void dump_find_atom_request( const struct find_atom_request *req )
{
    dump_varargs_unicode_str( " name=", cur_size );
}

static void dump_find_atom_reply( const struct find_atom_reply *req )
{
    fprintf( stderr, " atom=%04x", req->atom );
}

static void dump_get_atom_information_request( const struct get_atom_information_request *req )
{
    fprintf( stderr, " atom=%04x", req->atom );
}

static void dump_get_atom_information_reply( const struct get_atom_information_reply *req )
{
    fprintf( stderr, " count=%d", req->count );
    fprintf( stderr, ", pinned=%d", req->pinned );
    fprintf( stderr, ", total=%u", req->total );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_get_msg_queue_request( const struct get_msg_queue_request *req )
{
}

static void dump_get_msg_queue_reply( const struct get_msg_queue_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_set_queue_fd_request( const struct set_queue_fd_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_set_queue_mask_request( const struct set_queue_mask_request *req )
{
    fprintf( stderr, " wake_mask=%08x", req->wake_mask );
    fprintf( stderr, ", changed_mask=%08x", req->changed_mask );
    fprintf( stderr, ", skip_wait=%d", req->skip_wait );
}

static void dump_set_queue_mask_reply( const struct set_queue_mask_reply *req )
{
    fprintf( stderr, " wake_bits=%08x", req->wake_bits );
    fprintf( stderr, ", changed_bits=%08x", req->changed_bits );
}

static void dump_get_queue_status_request( const struct get_queue_status_request *req )
{
    fprintf( stderr, " clear_bits=%08x", req->clear_bits );
}

static void dump_get_queue_status_reply( const struct get_queue_status_reply *req )
{
    fprintf( stderr, " wake_bits=%08x", req->wake_bits );
    fprintf( stderr, ", changed_bits=%08x", req->changed_bits );
}

static void dump_get_process_idle_event_request( const struct get_process_idle_event_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_process_idle_event_reply( const struct get_process_idle_event_reply *req )
{
    fprintf( stderr, " event=%04x", req->event );
}

static void dump_send_message_request( const struct send_message_request *req )
{
    fprintf( stderr, " id=%04x", req->id );
    fprintf( stderr, ", type=%d", req->type );
    fprintf( stderr, ", flags=%d", req->flags );
    fprintf( stderr, ", win=%08x", req->win );
    fprintf( stderr, ", msg=%08x", req->msg );
    dump_uint64( ", wparam=", &req->wparam );
    dump_uint64( ", lparam=", &req->lparam );
    dump_timeout( ", timeout=", &req->timeout );
    dump_varargs_message_data( ", data=", cur_size );
}

static void dump_post_quit_message_request( const struct post_quit_message_request *req )
{
    fprintf( stderr, " exit_code=%d", req->exit_code );
}

static void dump_send_hardware_message_request( const struct send_hardware_message_request *req )
{
    fprintf( stderr, " win=%08x", req->win );
    dump_hw_input( ", input=", &req->input );
    fprintf( stderr, ", flags=%08x", req->flags );
    dump_varargs_bytes( ", report=", cur_size );
}

static void dump_send_hardware_message_reply( const struct send_hardware_message_reply *req )
{
    fprintf( stderr, " wait=%d", req->wait );
    fprintf( stderr, ", prev_x=%d", req->prev_x );
    fprintf( stderr, ", prev_y=%d", req->prev_y );
    fprintf( stderr, ", new_x=%d", req->new_x );
    fprintf( stderr, ", new_y=%d", req->new_y );
}

static void dump_get_message_request( const struct get_message_request *req )
{
    fprintf( stderr, " flags=%08x", req->flags );
    fprintf( stderr, ", get_win=%08x", req->get_win );
    fprintf( stderr, ", get_first=%08x", req->get_first );
    fprintf( stderr, ", get_last=%08x", req->get_last );
    fprintf( stderr, ", hw_id=%08x", req->hw_id );
    fprintf( stderr, ", wake_mask=%08x", req->wake_mask );
    fprintf( stderr, ", changed_mask=%08x", req->changed_mask );
}

static void dump_get_message_reply( const struct get_message_reply *req )
{
    fprintf( stderr, " win=%08x", req->win );
    fprintf( stderr, ", msg=%08x", req->msg );
    dump_uint64( ", wparam=", &req->wparam );
    dump_uint64( ", lparam=", &req->lparam );
    fprintf( stderr, ", type=%d", req->type );
    fprintf( stderr, ", x=%d", req->x );
    fprintf( stderr, ", y=%d", req->y );
    fprintf( stderr, ", time=%08x", req->time );
    fprintf( stderr, ", active_hooks=%08x", req->active_hooks );
    fprintf( stderr, ", total=%u", req->total );
    dump_varargs_message_data( ", data=", cur_size );
}

static void dump_reply_message_request( const struct reply_message_request *req )
{
    fprintf( stderr, " remove=%d", req->remove );
    dump_uint64( ", result=", &req->result );
    dump_varargs_bytes( ", data=", cur_size );
}

static void dump_accept_hardware_message_request( const struct accept_hardware_message_request *req )
{
    fprintf( stderr, " hw_id=%08x", req->hw_id );
}

static void dump_get_message_reply_request( const struct get_message_reply_request *req )
{
    fprintf( stderr, " cancel=%d", req->cancel );
}

static void dump_get_message_reply_reply( const struct get_message_reply_reply *req )
{
    dump_uint64( " result=", &req->result );
    dump_varargs_bytes( ", data=", cur_size );
}

static void dump_set_win_timer_request( const struct set_win_timer_request *req )
{
    fprintf( stderr, " win=%08x", req->win );
    fprintf( stderr, ", msg=%08x", req->msg );
    fprintf( stderr, ", rate=%08x", req->rate );
    dump_uint64( ", id=", &req->id );
    dump_uint64( ", lparam=", &req->lparam );
}

static void dump_set_win_timer_reply( const struct set_win_timer_reply *req )
{
    dump_uint64( " id=", &req->id );
}

static void dump_kill_win_timer_request( const struct kill_win_timer_request *req )
{
    fprintf( stderr, " win=%08x", req->win );
    dump_uint64( ", id=", &req->id );
    fprintf( stderr, ", msg=%08x", req->msg );
}

static void dump_is_window_hung_request( const struct is_window_hung_request *req )
{
    fprintf( stderr, " win=%08x", req->win );
}

static void dump_is_window_hung_reply( const struct is_window_hung_reply *req )
{
    fprintf( stderr, " is_hung=%d", req->is_hung );
}

static void dump_get_serial_info_request( const struct get_serial_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", flags=%d", req->flags );
}

static void dump_get_serial_info_reply( const struct get_serial_info_reply *req )
{
    fprintf( stderr, " eventmask=%08x", req->eventmask );
    fprintf( stderr, ", cookie=%08x", req->cookie );
    fprintf( stderr, ", pending_write=%08x", req->pending_write );
}

static void dump_set_serial_info_request( const struct set_serial_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", flags=%d", req->flags );
}

static void dump_cancel_sync_request( const struct cancel_sync_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    dump_uint64( ", iosb=", &req->iosb );
}

static void dump_register_async_request( const struct register_async_request *req )
{
    fprintf( stderr, " type=%d", req->type );
    dump_async_data( ", async=", &req->async );
    fprintf( stderr, ", count=%d", req->count );
}

static void dump_cancel_async_request( const struct cancel_async_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    dump_uint64( ", iosb=", &req->iosb );
    fprintf( stderr, ", only_thread=%d", req->only_thread );
}

static void dump_get_async_result_request( const struct get_async_result_request *req )
{
    dump_uint64( " user_arg=", &req->user_arg );
}

static void dump_get_async_result_reply( const struct get_async_result_reply *req )
{
    dump_varargs_bytes( " out_data=", cur_size );
}

static void dump_set_async_direct_result_request( const struct set_async_direct_result_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    dump_uint64( ", information=", &req->information );
    fprintf( stderr, ", status=%08x", req->status );
    fprintf( stderr, ", mark_pending=%d", req->mark_pending );
}

static void dump_set_async_direct_result_reply( const struct set_async_direct_result_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_read_request( const struct read_request *req )
{
    dump_async_data( " async=", &req->async );
    dump_uint64( ", pos=", &req->pos );
}

static void dump_read_reply( const struct read_reply *req )
{
    fprintf( stderr, " wait=%04x", req->wait );
    fprintf( stderr, ", options=%08x", req->options );
    dump_varargs_bytes( ", data=", cur_size );
}

static void dump_write_request( const struct write_request *req )
{
    dump_async_data( " async=", &req->async );
    dump_uint64( ", pos=", &req->pos );
    dump_varargs_bytes( ", data=", cur_size );
}

static void dump_write_reply( const struct write_reply *req )
{
    fprintf( stderr, " wait=%04x", req->wait );
    fprintf( stderr, ", options=%08x", req->options );
    fprintf( stderr, ", size=%u", req->size );
}

static void dump_ioctl_request( const struct ioctl_request *req )
{
    dump_ioctl_code( " code=", &req->code );
    dump_async_data( ", async=", &req->async );
    dump_varargs_bytes( ", in_data=", cur_size );
}

static void dump_ioctl_reply( const struct ioctl_reply *req )
{
    fprintf( stderr, " wait=%04x", req->wait );
    fprintf( stderr, ", options=%08x", req->options );
    dump_varargs_bytes( ", out_data=", cur_size );
}

static void dump_set_irp_result_request( const struct set_irp_result_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", status=%08x", req->status );
    fprintf( stderr, ", size=%u", req->size );
    dump_varargs_bytes( ", data=", cur_size );
}

static void dump_create_named_pipe_request( const struct create_named_pipe_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", options=%08x", req->options );
    fprintf( stderr, ", sharing=%08x", req->sharing );
    fprintf( stderr, ", disposition=%08x", req->disposition );
    fprintf( stderr, ", maxinstances=%08x", req->maxinstances );
    fprintf( stderr, ", outsize=%08x", req->outsize );
    fprintf( stderr, ", insize=%08x", req->insize );
    dump_timeout( ", timeout=", &req->timeout );
    fprintf( stderr, ", flags=%08x", req->flags );
    dump_varargs_object_attributes( ", objattr=", cur_size );
}

static void dump_create_named_pipe_reply( const struct create_named_pipe_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", created=%d", req->created );
}

static void dump_set_named_pipe_info_request( const struct set_named_pipe_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", flags=%08x", req->flags );
}

static void dump_create_window_request( const struct create_window_request *req )
{
    fprintf( stderr, " parent=%08x", req->parent );
    fprintf( stderr, ", owner=%08x", req->owner );
    fprintf( stderr, ", atom=%04x", req->atom );
    dump_uint64( ", instance=", &req->instance );
    fprintf( stderr, ", dpi=%d", req->dpi );
    fprintf( stderr, ", awareness=%d", req->awareness );
    fprintf( stderr, ", style=%08x", req->style );
    fprintf( stderr, ", ex_style=%08x", req->ex_style );
    dump_varargs_unicode_str( ", class=", cur_size );
}

static void dump_create_window_reply( const struct create_window_reply *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
    fprintf( stderr, ", parent=%08x", req->parent );
    fprintf( stderr, ", owner=%08x", req->owner );
    fprintf( stderr, ", extra=%d", req->extra );
    dump_uint64( ", class_ptr=", &req->class_ptr );
    fprintf( stderr, ", dpi=%d", req->dpi );
    fprintf( stderr, ", awareness=%d", req->awareness );
}

static void dump_destroy_window_request( const struct destroy_window_request *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
}

static void dump_get_desktop_window_request( const struct get_desktop_window_request *req )
{
    fprintf( stderr, " force=%d", req->force );
}

static void dump_get_desktop_window_reply( const struct get_desktop_window_reply *req )
{
    fprintf( stderr, " top_window=%08x", req->top_window );
    fprintf( stderr, ", msg_window=%08x", req->msg_window );
}

static void dump_set_window_owner_request( const struct set_window_owner_request *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
    fprintf( stderr, ", owner=%08x", req->owner );
}

static void dump_set_window_owner_reply( const struct set_window_owner_reply *req )
{
    fprintf( stderr, " full_owner=%08x", req->full_owner );
    fprintf( stderr, ", prev_owner=%08x", req->prev_owner );
}

static void dump_get_window_info_request( const struct get_window_info_request *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
}

static void dump_get_window_info_reply( const struct get_window_info_reply *req )
{
    fprintf( stderr, " full_handle=%08x", req->full_handle );
    fprintf( stderr, ", last_active=%08x", req->last_active );
    fprintf( stderr, ", pid=%04x", req->pid );
    fprintf( stderr, ", tid=%04x", req->tid );
    fprintf( stderr, ", atom=%04x", req->atom );
    fprintf( stderr, ", is_unicode=%d", req->is_unicode );
    fprintf( stderr, ", dpi=%d", req->dpi );
    fprintf( stderr, ", awareness=%d", req->awareness );
}

static void dump_set_window_info_request( const struct set_window_info_request *req )
{
    fprintf( stderr, " flags=%04x", req->flags );
    fprintf( stderr, ", is_unicode=%d", req->is_unicode );
    fprintf( stderr, ", handle=%08x", req->handle );
    fprintf( stderr, ", style=%08x", req->style );
    fprintf( stderr, ", ex_style=%08x", req->ex_style );
    fprintf( stderr, ", extra_size=%u", req->extra_size );
    dump_uint64( ", instance=", &req->instance );
    dump_uint64( ", user_data=", &req->user_data );
    dump_uint64( ", extra_value=", &req->extra_value );
    fprintf( stderr, ", extra_offset=%d", req->extra_offset );
}

static void dump_set_window_info_reply( const struct set_window_info_reply *req )
{
    fprintf( stderr, " old_style=%08x", req->old_style );
    fprintf( stderr, ", old_ex_style=%08x", req->old_ex_style );
    dump_uint64( ", old_instance=", &req->old_instance );
    dump_uint64( ", old_user_data=", &req->old_user_data );
    dump_uint64( ", old_extra_value=", &req->old_extra_value );
    dump_uint64( ", old_id=", &req->old_id );
}

static void dump_set_parent_request( const struct set_parent_request *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
    fprintf( stderr, ", parent=%08x", req->parent );
}

static void dump_set_parent_reply( const struct set_parent_reply *req )
{
    fprintf( stderr, " old_parent=%08x", req->old_parent );
    fprintf( stderr, ", full_parent=%08x", req->full_parent );
    fprintf( stderr, ", dpi=%d", req->dpi );
    fprintf( stderr, ", awareness=%d", req->awareness );
}

static void dump_get_window_parents_request( const struct get_window_parents_request *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
}

static void dump_get_window_parents_reply( const struct get_window_parents_reply *req )
{
    fprintf( stderr, " count=%d", req->count );
    dump_varargs_user_handles( ", parents=", cur_size );
}

static void dump_get_window_children_request( const struct get_window_children_request *req )
{
    fprintf( stderr, " desktop=%04x", req->desktop );
    fprintf( stderr, ", parent=%08x", req->parent );
    fprintf( stderr, ", atom=%04x", req->atom );
    fprintf( stderr, ", tid=%04x", req->tid );
    dump_varargs_unicode_str( ", class=", cur_size );
}

static void dump_get_window_children_reply( const struct get_window_children_reply *req )
{
    fprintf( stderr, " count=%d", req->count );
    dump_varargs_user_handles( ", children=", cur_size );
}

static void dump_get_window_children_from_point_request( const struct get_window_children_from_point_request *req )
{
    fprintf( stderr, " parent=%08x", req->parent );
    fprintf( stderr, ", x=%d", req->x );
    fprintf( stderr, ", y=%d", req->y );
    fprintf( stderr, ", dpi=%d", req->dpi );
}

static void dump_get_window_children_from_point_reply( const struct get_window_children_from_point_reply *req )
{
    fprintf( stderr, " count=%d", req->count );
    dump_varargs_user_handles( ", children=", cur_size );
}

static void dump_get_window_tree_request( const struct get_window_tree_request *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
}

static void dump_get_window_tree_reply( const struct get_window_tree_reply *req )
{
    fprintf( stderr, " parent=%08x", req->parent );
    fprintf( stderr, ", owner=%08x", req->owner );
    fprintf( stderr, ", next_sibling=%08x", req->next_sibling );
    fprintf( stderr, ", prev_sibling=%08x", req->prev_sibling );
    fprintf( stderr, ", first_sibling=%08x", req->first_sibling );
    fprintf( stderr, ", last_sibling=%08x", req->last_sibling );
    fprintf( stderr, ", first_child=%08x", req->first_child );
    fprintf( stderr, ", last_child=%08x", req->last_child );
}

static void dump_set_window_pos_request( const struct set_window_pos_request *req )
{
    fprintf( stderr, " swp_flags=%04x", req->swp_flags );
    fprintf( stderr, ", paint_flags=%04x", req->paint_flags );
    fprintf( stderr, ", handle=%08x", req->handle );
    fprintf( stderr, ", previous=%08x", req->previous );
    dump_rectangle( ", window=", &req->window );
    dump_rectangle( ", client=", &req->client );
    dump_varargs_rectangles( ", valid=", cur_size );
}

static void dump_set_window_pos_reply( const struct set_window_pos_reply *req )
{
    fprintf( stderr, " new_style=%08x", req->new_style );
    fprintf( stderr, ", new_ex_style=%08x", req->new_ex_style );
    fprintf( stderr, ", surface_win=%08x", req->surface_win );
    fprintf( stderr, ", needs_update=%d", req->needs_update );
}

static void dump_get_window_rectangles_request( const struct get_window_rectangles_request *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
    fprintf( stderr, ", relative=%d", req->relative );
    fprintf( stderr, ", dpi=%d", req->dpi );
}

static void dump_get_window_rectangles_reply( const struct get_window_rectangles_reply *req )
{
    dump_rectangle( " window=", &req->window );
    dump_rectangle( ", client=", &req->client );
}

static void dump_get_window_text_request( const struct get_window_text_request *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
}

static void dump_get_window_text_reply( const struct get_window_text_reply *req )
{
    fprintf( stderr, " length=%u", req->length );
    dump_varargs_unicode_str( ", text=", cur_size );
}

static void dump_set_window_text_request( const struct set_window_text_request *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
    dump_varargs_unicode_str( ", text=", cur_size );
}

static void dump_get_windows_offset_request( const struct get_windows_offset_request *req )
{
    fprintf( stderr, " from=%08x", req->from );
    fprintf( stderr, ", to=%08x", req->to );
    fprintf( stderr, ", dpi=%d", req->dpi );
}

static void dump_get_windows_offset_reply( const struct get_windows_offset_reply *req )
{
    fprintf( stderr, " x=%d", req->x );
    fprintf( stderr, ", y=%d", req->y );
    fprintf( stderr, ", mirror=%d", req->mirror );
}

static void dump_get_visible_region_request( const struct get_visible_region_request *req )
{
    fprintf( stderr, " window=%08x", req->window );
    fprintf( stderr, ", flags=%08x", req->flags );
}

static void dump_get_visible_region_reply( const struct get_visible_region_reply *req )
{
    fprintf( stderr, " top_win=%08x", req->top_win );
    dump_rectangle( ", top_rect=", &req->top_rect );
    dump_rectangle( ", win_rect=", &req->win_rect );
    fprintf( stderr, ", paint_flags=%08x", req->paint_flags );
    fprintf( stderr, ", total_size=%u", req->total_size );
    dump_varargs_rectangles( ", region=", cur_size );
}

static void dump_get_surface_region_request( const struct get_surface_region_request *req )
{
    fprintf( stderr, " window=%08x", req->window );
}

static void dump_get_surface_region_reply( const struct get_surface_region_reply *req )
{
    dump_rectangle( " visible_rect=", &req->visible_rect );
    fprintf( stderr, ", total_size=%u", req->total_size );
    dump_varargs_rectangles( ", region=", cur_size );
}

static void dump_get_window_region_request( const struct get_window_region_request *req )
{
    fprintf( stderr, " window=%08x", req->window );
}

static void dump_get_window_region_reply( const struct get_window_region_reply *req )
{
    fprintf( stderr, " total_size=%u", req->total_size );
    dump_varargs_rectangles( ", region=", cur_size );
}

static void dump_set_window_region_request( const struct set_window_region_request *req )
{
    fprintf( stderr, " window=%08x", req->window );
    fprintf( stderr, ", redraw=%d", req->redraw );
    dump_varargs_rectangles( ", region=", cur_size );
}

static void dump_get_update_region_request( const struct get_update_region_request *req )
{
    fprintf( stderr, " window=%08x", req->window );
    fprintf( stderr, ", from_child=%08x", req->from_child );
    fprintf( stderr, ", flags=%08x", req->flags );
}

static void dump_get_update_region_reply( const struct get_update_region_reply *req )
{
    fprintf( stderr, " child=%08x", req->child );
    fprintf( stderr, ", flags=%08x", req->flags );
    fprintf( stderr, ", total_size=%u", req->total_size );
    dump_varargs_rectangles( ", region=", cur_size );
}

static void dump_update_window_zorder_request( const struct update_window_zorder_request *req )
{
    fprintf( stderr, " window=%08x", req->window );
    dump_rectangle( ", rect=", &req->rect );
}

static void dump_redraw_window_request( const struct redraw_window_request *req )
{
    fprintf( stderr, " window=%08x", req->window );
    fprintf( stderr, ", flags=%08x", req->flags );
    dump_varargs_rectangles( ", region=", cur_size );
}

static void dump_set_window_property_request( const struct set_window_property_request *req )
{
    fprintf( stderr, " window=%08x", req->window );
    dump_uint64( ", data=", &req->data );
    fprintf( stderr, ", atom=%04x", req->atom );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_remove_window_property_request( const struct remove_window_property_request *req )
{
    fprintf( stderr, " window=%08x", req->window );
    fprintf( stderr, ", atom=%04x", req->atom );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_remove_window_property_reply( const struct remove_window_property_reply *req )
{
    dump_uint64( " data=", &req->data );
}

static void dump_get_window_property_request( const struct get_window_property_request *req )
{
    fprintf( stderr, " window=%08x", req->window );
    fprintf( stderr, ", atom=%04x", req->atom );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_get_window_property_reply( const struct get_window_property_reply *req )
{
    dump_uint64( " data=", &req->data );
}

static void dump_get_window_properties_request( const struct get_window_properties_request *req )
{
    fprintf( stderr, " window=%08x", req->window );
}

static void dump_get_window_properties_reply( const struct get_window_properties_reply *req )
{
    fprintf( stderr, " total=%d", req->total );
    dump_varargs_properties( ", props=", cur_size );
}

static void dump_create_winstation_request( const struct create_winstation_request *req )
{
    fprintf( stderr, " flags=%08x", req->flags );
    fprintf( stderr, ", access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", rootdir=%04x", req->rootdir );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_create_winstation_reply( const struct create_winstation_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_open_winstation_request( const struct open_winstation_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", rootdir=%04x", req->rootdir );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_open_winstation_reply( const struct open_winstation_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_close_winstation_request( const struct close_winstation_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_process_winstation_request( const struct get_process_winstation_request *req )
{
}

static void dump_get_process_winstation_reply( const struct get_process_winstation_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_set_process_winstation_request( const struct set_process_winstation_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_enum_winstation_request( const struct enum_winstation_request *req )
{
    fprintf( stderr, " index=%08x", req->index );
}

static void dump_enum_winstation_reply( const struct enum_winstation_reply *req )
{
    fprintf( stderr, " next=%08x", req->next );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_create_desktop_request( const struct create_desktop_request *req )
{
    fprintf( stderr, " flags=%08x", req->flags );
    fprintf( stderr, ", access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_create_desktop_reply( const struct create_desktop_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_open_desktop_request( const struct open_desktop_request *req )
{
    fprintf( stderr, " winsta=%04x", req->winsta );
    fprintf( stderr, ", flags=%08x", req->flags );
    fprintf( stderr, ", access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_open_desktop_reply( const struct open_desktop_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_open_input_desktop_request( const struct open_input_desktop_request *req )
{
    fprintf( stderr, " flags=%08x", req->flags );
    fprintf( stderr, ", access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
}

static void dump_open_input_desktop_reply( const struct open_input_desktop_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_close_desktop_request( const struct close_desktop_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_thread_desktop_request( const struct get_thread_desktop_request *req )
{
    fprintf( stderr, " tid=%04x", req->tid );
}

static void dump_get_thread_desktop_reply( const struct get_thread_desktop_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_set_thread_desktop_request( const struct set_thread_desktop_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_enum_desktop_request( const struct enum_desktop_request *req )
{
    fprintf( stderr, " winstation=%04x", req->winstation );
    fprintf( stderr, ", index=%08x", req->index );
}

static void dump_enum_desktop_reply( const struct enum_desktop_reply *req )
{
    fprintf( stderr, " next=%08x", req->next );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_set_user_object_info_request( const struct set_user_object_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", flags=%08x", req->flags );
    fprintf( stderr, ", obj_flags=%08x", req->obj_flags );
}

static void dump_set_user_object_info_reply( const struct set_user_object_info_reply *req )
{
    fprintf( stderr, " is_desktop=%d", req->is_desktop );
    fprintf( stderr, ", old_obj_flags=%08x", req->old_obj_flags );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_register_hotkey_request( const struct register_hotkey_request *req )
{
    fprintf( stderr, " window=%08x", req->window );
    fprintf( stderr, ", id=%d", req->id );
    fprintf( stderr, ", flags=%08x", req->flags );
    fprintf( stderr, ", vkey=%08x", req->vkey );
}

static void dump_register_hotkey_reply( const struct register_hotkey_reply *req )
{
    fprintf( stderr, " replaced=%d", req->replaced );
    fprintf( stderr, ", flags=%08x", req->flags );
    fprintf( stderr, ", vkey=%08x", req->vkey );
}

static void dump_unregister_hotkey_request( const struct unregister_hotkey_request *req )
{
    fprintf( stderr, " window=%08x", req->window );
    fprintf( stderr, ", id=%d", req->id );
}

static void dump_unregister_hotkey_reply( const struct unregister_hotkey_reply *req )
{
    fprintf( stderr, " flags=%08x", req->flags );
    fprintf( stderr, ", vkey=%08x", req->vkey );
}

static void dump_attach_thread_input_request( const struct attach_thread_input_request *req )
{
    fprintf( stderr, " tid_from=%04x", req->tid_from );
    fprintf( stderr, ", tid_to=%04x", req->tid_to );
    fprintf( stderr, ", attach=%d", req->attach );
}

static void dump_get_thread_input_request( const struct get_thread_input_request *req )
{
    fprintf( stderr, " tid=%04x", req->tid );
}

static void dump_get_thread_input_reply( const struct get_thread_input_reply *req )
{
    fprintf( stderr, " focus=%08x", req->focus );
    fprintf( stderr, ", capture=%08x", req->capture );
    fprintf( stderr, ", active=%08x", req->active );
    fprintf( stderr, ", foreground=%08x", req->foreground );
    fprintf( stderr, ", menu_owner=%08x", req->menu_owner );
    fprintf( stderr, ", move_size=%08x", req->move_size );
    fprintf( stderr, ", caret=%08x", req->caret );
    fprintf( stderr, ", cursor=%08x", req->cursor );
    fprintf( stderr, ", show_count=%d", req->show_count );
    dump_rectangle( ", rect=", &req->rect );
}

static void dump_get_last_input_time_request( const struct get_last_input_time_request *req )
{
}

static void dump_get_last_input_time_reply( const struct get_last_input_time_reply *req )
{
    fprintf( stderr, " time=%08x", req->time );
}

static void dump_get_key_state_request( const struct get_key_state_request *req )
{
    fprintf( stderr, " async=%d", req->async );
    fprintf( stderr, ", key=%d", req->key );
}

static void dump_get_key_state_reply( const struct get_key_state_reply *req )
{
    fprintf( stderr, " state=%02x", req->state );
    dump_varargs_bytes( ", keystate=", cur_size );
}

static void dump_set_key_state_request( const struct set_key_state_request *req )
{
    fprintf( stderr, " async=%d", req->async );
    dump_varargs_bytes( ", keystate=", cur_size );
}

static void dump_set_foreground_window_request( const struct set_foreground_window_request *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
}

static void dump_set_foreground_window_reply( const struct set_foreground_window_reply *req )
{
    fprintf( stderr, " previous=%08x", req->previous );
    fprintf( stderr, ", send_msg_old=%d", req->send_msg_old );
    fprintf( stderr, ", send_msg_new=%d", req->send_msg_new );
}

static void dump_set_focus_window_request( const struct set_focus_window_request *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
}

static void dump_set_focus_window_reply( const struct set_focus_window_reply *req )
{
    fprintf( stderr, " previous=%08x", req->previous );
}

static void dump_set_active_window_request( const struct set_active_window_request *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
}

static void dump_set_active_window_reply( const struct set_active_window_reply *req )
{
    fprintf( stderr, " previous=%08x", req->previous );
}

static void dump_set_capture_window_request( const struct set_capture_window_request *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
    fprintf( stderr, ", flags=%08x", req->flags );
}

static void dump_set_capture_window_reply( const struct set_capture_window_reply *req )
{
    fprintf( stderr, " previous=%08x", req->previous );
    fprintf( stderr, ", full_handle=%08x", req->full_handle );
}

static void dump_set_caret_window_request( const struct set_caret_window_request *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
    fprintf( stderr, ", width=%d", req->width );
    fprintf( stderr, ", height=%d", req->height );
}

static void dump_set_caret_window_reply( const struct set_caret_window_reply *req )
{
    fprintf( stderr, " previous=%08x", req->previous );
    dump_rectangle( ", old_rect=", &req->old_rect );
    fprintf( stderr, ", old_hide=%d", req->old_hide );
    fprintf( stderr, ", old_state=%d", req->old_state );
}

static void dump_set_caret_info_request( const struct set_caret_info_request *req )
{
    fprintf( stderr, " flags=%08x", req->flags );
    fprintf( stderr, ", handle=%08x", req->handle );
    fprintf( stderr, ", x=%d", req->x );
    fprintf( stderr, ", y=%d", req->y );
    fprintf( stderr, ", hide=%d", req->hide );
    fprintf( stderr, ", state=%d", req->state );
}

static void dump_set_caret_info_reply( const struct set_caret_info_reply *req )
{
    fprintf( stderr, " full_handle=%08x", req->full_handle );
    dump_rectangle( ", old_rect=", &req->old_rect );
    fprintf( stderr, ", old_hide=%d", req->old_hide );
    fprintf( stderr, ", old_state=%d", req->old_state );
}

static void dump_set_hook_request( const struct set_hook_request *req )
{
    fprintf( stderr, " id=%d", req->id );
    fprintf( stderr, ", pid=%04x", req->pid );
    fprintf( stderr, ", tid=%04x", req->tid );
    fprintf( stderr, ", event_min=%d", req->event_min );
    fprintf( stderr, ", event_max=%d", req->event_max );
    dump_uint64( ", proc=", &req->proc );
    fprintf( stderr, ", flags=%d", req->flags );
    fprintf( stderr, ", unicode=%d", req->unicode );
    dump_varargs_unicode_str( ", module=", cur_size );
}

static void dump_set_hook_reply( const struct set_hook_reply *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
    fprintf( stderr, ", active_hooks=%08x", req->active_hooks );
}

static void dump_remove_hook_request( const struct remove_hook_request *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
    dump_uint64( ", proc=", &req->proc );
    fprintf( stderr, ", id=%d", req->id );
}

static void dump_remove_hook_reply( const struct remove_hook_reply *req )
{
    fprintf( stderr, " active_hooks=%08x", req->active_hooks );
}

static void dump_start_hook_chain_request( const struct start_hook_chain_request *req )
{
    fprintf( stderr, " id=%d", req->id );
    fprintf( stderr, ", event=%d", req->event );
    fprintf( stderr, ", window=%08x", req->window );
    fprintf( stderr, ", object_id=%d", req->object_id );
    fprintf( stderr, ", child_id=%d", req->child_id );
}

static void dump_start_hook_chain_reply( const struct start_hook_chain_reply *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
    fprintf( stderr, ", pid=%04x", req->pid );
    fprintf( stderr, ", tid=%04x", req->tid );
    fprintf( stderr, ", unicode=%d", req->unicode );
    dump_uint64( ", proc=", &req->proc );
    fprintf( stderr, ", active_hooks=%08x", req->active_hooks );
    dump_varargs_unicode_str( ", module=", cur_size );
}

static void dump_finish_hook_chain_request( const struct finish_hook_chain_request *req )
{
    fprintf( stderr, " id=%d", req->id );
}

static void dump_get_hook_info_request( const struct get_hook_info_request *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
    fprintf( stderr, ", get_next=%d", req->get_next );
    fprintf( stderr, ", event=%d", req->event );
    fprintf( stderr, ", window=%08x", req->window );
    fprintf( stderr, ", object_id=%d", req->object_id );
    fprintf( stderr, ", child_id=%d", req->child_id );
}

static void dump_get_hook_info_reply( const struct get_hook_info_reply *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
    fprintf( stderr, ", id=%d", req->id );
    fprintf( stderr, ", pid=%04x", req->pid );
    fprintf( stderr, ", tid=%04x", req->tid );
    dump_uint64( ", proc=", &req->proc );
    fprintf( stderr, ", unicode=%d", req->unicode );
    dump_varargs_unicode_str( ", module=", cur_size );
}

static void dump_create_class_request( const struct create_class_request *req )
{
    fprintf( stderr, " local=%d", req->local );
    fprintf( stderr, ", atom=%04x", req->atom );
    fprintf( stderr, ", style=%08x", req->style );
    dump_uint64( ", instance=", &req->instance );
    fprintf( stderr, ", extra=%d", req->extra );
    fprintf( stderr, ", win_extra=%d", req->win_extra );
    dump_uint64( ", client_ptr=", &req->client_ptr );
    fprintf( stderr, ", name_offset=%u", req->name_offset );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_create_class_reply( const struct create_class_reply *req )
{
    fprintf( stderr, " atom=%04x", req->atom );
}

static void dump_destroy_class_request( const struct destroy_class_request *req )
{
    fprintf( stderr, " atom=%04x", req->atom );
    dump_uint64( ", instance=", &req->instance );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_destroy_class_reply( const struct destroy_class_reply *req )
{
    dump_uint64( " client_ptr=", &req->client_ptr );
}

static void dump_set_class_info_request( const struct set_class_info_request *req )
{
    fprintf( stderr, " window=%08x", req->window );
    fprintf( stderr, ", flags=%08x", req->flags );
    fprintf( stderr, ", atom=%04x", req->atom );
    fprintf( stderr, ", style=%08x", req->style );
    fprintf( stderr, ", win_extra=%d", req->win_extra );
    dump_uint64( ", instance=", &req->instance );
    fprintf( stderr, ", extra_offset=%d", req->extra_offset );
    fprintf( stderr, ", extra_size=%u", req->extra_size );
    dump_uint64( ", extra_value=", &req->extra_value );
}

static void dump_set_class_info_reply( const struct set_class_info_reply *req )
{
    fprintf( stderr, " old_atom=%04x", req->old_atom );
    fprintf( stderr, ", base_atom=%04x", req->base_atom );
    dump_uint64( ", old_instance=", &req->old_instance );
    dump_uint64( ", old_extra_value=", &req->old_extra_value );
    fprintf( stderr, ", old_style=%08x", req->old_style );
    fprintf( stderr, ", old_extra=%d", req->old_extra );
    fprintf( stderr, ", old_win_extra=%d", req->old_win_extra );
}

static void dump_open_clipboard_request( const struct open_clipboard_request *req )
{
    fprintf( stderr, " window=%08x", req->window );
}

static void dump_open_clipboard_reply( const struct open_clipboard_reply *req )
{
    fprintf( stderr, " owner=%08x", req->owner );
}

static void dump_close_clipboard_request( const struct close_clipboard_request *req )
{
}

static void dump_close_clipboard_reply( const struct close_clipboard_reply *req )
{
    fprintf( stderr, " viewer=%08x", req->viewer );
    fprintf( stderr, ", owner=%08x", req->owner );
}

static void dump_empty_clipboard_request( const struct empty_clipboard_request *req )
{
}

static void dump_set_clipboard_data_request( const struct set_clipboard_data_request *req )
{
    fprintf( stderr, " format=%08x", req->format );
    fprintf( stderr, ", lcid=%08x", req->lcid );
    dump_varargs_bytes( ", data=", cur_size );
}

static void dump_set_clipboard_data_reply( const struct set_clipboard_data_reply *req )
{
    fprintf( stderr, " seqno=%08x", req->seqno );
}

static void dump_get_clipboard_data_request( const struct get_clipboard_data_request *req )
{
    fprintf( stderr, " format=%08x", req->format );
    fprintf( stderr, ", render=%d", req->render );
    fprintf( stderr, ", cached=%d", req->cached );
    fprintf( stderr, ", seqno=%08x", req->seqno );
}

static void dump_get_clipboard_data_reply( const struct get_clipboard_data_reply *req )
{
    fprintf( stderr, " from=%08x", req->from );
    fprintf( stderr, ", owner=%08x", req->owner );
    fprintf( stderr, ", seqno=%08x", req->seqno );
    fprintf( stderr, ", total=%u", req->total );
    dump_varargs_bytes( ", data=", cur_size );
}

static void dump_get_clipboard_formats_request( const struct get_clipboard_formats_request *req )
{
    fprintf( stderr, " format=%08x", req->format );
}

static void dump_get_clipboard_formats_reply( const struct get_clipboard_formats_reply *req )
{
    fprintf( stderr, " count=%08x", req->count );
    dump_varargs_uints( ", formats=", cur_size );
}

static void dump_enum_clipboard_formats_request( const struct enum_clipboard_formats_request *req )
{
    fprintf( stderr, " previous=%08x", req->previous );
}

static void dump_enum_clipboard_formats_reply( const struct enum_clipboard_formats_reply *req )
{
    fprintf( stderr, " format=%08x", req->format );
}

static void dump_release_clipboard_request( const struct release_clipboard_request *req )
{
    fprintf( stderr, " owner=%08x", req->owner );
}

static void dump_release_clipboard_reply( const struct release_clipboard_reply *req )
{
    fprintf( stderr, " viewer=%08x", req->viewer );
    fprintf( stderr, ", owner=%08x", req->owner );
}

static void dump_get_clipboard_info_request( const struct get_clipboard_info_request *req )
{
}

static void dump_get_clipboard_info_reply( const struct get_clipboard_info_reply *req )
{
    fprintf( stderr, " window=%08x", req->window );
    fprintf( stderr, ", owner=%08x", req->owner );
    fprintf( stderr, ", viewer=%08x", req->viewer );
    fprintf( stderr, ", seqno=%08x", req->seqno );
}

static void dump_set_clipboard_viewer_request( const struct set_clipboard_viewer_request *req )
{
    fprintf( stderr, " viewer=%08x", req->viewer );
    fprintf( stderr, ", previous=%08x", req->previous );
}

static void dump_set_clipboard_viewer_reply( const struct set_clipboard_viewer_reply *req )
{
    fprintf( stderr, " old_viewer=%08x", req->old_viewer );
    fprintf( stderr, ", owner=%08x", req->owner );
}

static void dump_add_clipboard_listener_request( const struct add_clipboard_listener_request *req )
{
    fprintf( stderr, " window=%08x", req->window );
}

static void dump_remove_clipboard_listener_request( const struct remove_clipboard_listener_request *req )
{
    fprintf( stderr, " window=%08x", req->window );
}

static void dump_create_token_request( const struct create_token_request *req )
{
    dump_luid( " token_id=", &req->token_id );
    fprintf( stderr, ", access=%08x", req->access );
    fprintf( stderr, ", primary=%d", req->primary );
    fprintf( stderr, ", impersonation_level=%d", req->impersonation_level );
    dump_abstime( ", expire=", &req->expire );
    fprintf( stderr, ", group_count=%d", req->group_count );
    fprintf( stderr, ", primary_group=%d", req->primary_group );
    fprintf( stderr, ", priv_count=%d", req->priv_count );
}

static void dump_create_token_reply( const struct create_token_reply *req )
{
    fprintf( stderr, " token=%04x", req->token );
}

static void dump_open_token_request( const struct open_token_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", flags=%08x", req->flags );
}

static void dump_open_token_reply( const struct open_token_reply *req )
{
    fprintf( stderr, " token=%04x", req->token );
}

static void dump_set_global_windows_request( const struct set_global_windows_request *req )
{
    fprintf( stderr, " flags=%08x", req->flags );
    fprintf( stderr, ", shell_window=%08x", req->shell_window );
    fprintf( stderr, ", shell_listview=%08x", req->shell_listview );
    fprintf( stderr, ", progman_window=%08x", req->progman_window );
    fprintf( stderr, ", taskman_window=%08x", req->taskman_window );
}

static void dump_set_global_windows_reply( const struct set_global_windows_reply *req )
{
    fprintf( stderr, " old_shell_window=%08x", req->old_shell_window );
    fprintf( stderr, ", old_shell_listview=%08x", req->old_shell_listview );
    fprintf( stderr, ", old_progman_window=%08x", req->old_progman_window );
    fprintf( stderr, ", old_taskman_window=%08x", req->old_taskman_window );
}

static void dump_adjust_token_privileges_request( const struct adjust_token_privileges_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", disable_all=%d", req->disable_all );
    fprintf( stderr, ", get_modified_state=%d", req->get_modified_state );
    dump_varargs_luid_attr( ", privileges=", cur_size );
}

static void dump_adjust_token_privileges_reply( const struct adjust_token_privileges_reply *req )
{
    fprintf( stderr, " len=%08x", req->len );
    dump_varargs_luid_attr( ", privileges=", cur_size );
}

static void dump_get_token_privileges_request( const struct get_token_privileges_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_token_privileges_reply( const struct get_token_privileges_reply *req )
{
    fprintf( stderr, " len=%08x", req->len );
    dump_varargs_luid_attr( ", privileges=", cur_size );
}

static void dump_check_token_privileges_request( const struct check_token_privileges_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", all_required=%d", req->all_required );
    dump_varargs_luid_attr( ", privileges=", cur_size );
}

static void dump_check_token_privileges_reply( const struct check_token_privileges_reply *req )
{
    fprintf( stderr, " has_privileges=%d", req->has_privileges );
    dump_varargs_luid_attr( ", privileges=", cur_size );
}

static void dump_duplicate_token_request( const struct duplicate_token_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", access=%08x", req->access );
    fprintf( stderr, ", primary=%d", req->primary );
    fprintf( stderr, ", impersonation_level=%d", req->impersonation_level );
    dump_varargs_object_attributes( ", objattr=", cur_size );
}

static void dump_duplicate_token_reply( const struct duplicate_token_reply *req )
{
    fprintf( stderr, " new_handle=%04x", req->new_handle );
}

static void dump_filter_token_request( const struct filter_token_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", flags=%08x", req->flags );
    fprintf( stderr, ", privileges_size=%u", req->privileges_size );
    dump_varargs_luid_attr( ", privileges=", min(cur_size,req->privileges_size) );
    dump_varargs_sid( ", disable_sids=", cur_size );
}

static void dump_filter_token_reply( const struct filter_token_reply *req )
{
    fprintf( stderr, " new_handle=%04x", req->new_handle );
}

static void dump_access_check_request( const struct access_check_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", desired_access=%08x", req->desired_access );
    dump_generic_map( ", mapping=", &req->mapping );
    dump_varargs_security_descriptor( ", sd=", cur_size );
}

static void dump_access_check_reply( const struct access_check_reply *req )
{
    fprintf( stderr, " access_granted=%08x", req->access_granted );
    fprintf( stderr, ", access_status=%08x", req->access_status );
    fprintf( stderr, ", privileges_len=%08x", req->privileges_len );
    dump_varargs_luid_attr( ", privileges=", cur_size );
}

static void dump_get_token_sid_request( const struct get_token_sid_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", which_sid=%08x", req->which_sid );
}

static void dump_get_token_sid_reply( const struct get_token_sid_reply *req )
{
    fprintf( stderr, " sid_len=%u", req->sid_len );
    dump_varargs_sid( ", sid=", cur_size );
}

static void dump_get_token_groups_request( const struct get_token_groups_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", attr_mask=%08x", req->attr_mask );
}

static void dump_get_token_groups_reply( const struct get_token_groups_reply *req )
{
    fprintf( stderr, " attr_len=%u", req->attr_len );
    fprintf( stderr, ", sid_len=%u", req->sid_len );
    dump_varargs_uints( ", attrs=", min(cur_size,req->attr_len) );
    dump_varargs_sids( ", sids=", cur_size );
}

static void dump_get_token_default_dacl_request( const struct get_token_default_dacl_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_token_default_dacl_reply( const struct get_token_default_dacl_reply *req )
{
    fprintf( stderr, " acl_len=%u", req->acl_len );
    dump_varargs_acl( ", acl=", cur_size );
}

static void dump_set_token_default_dacl_request( const struct set_token_default_dacl_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    dump_varargs_acl( ", acl=", cur_size );
}

static void dump_set_security_object_request( const struct set_security_object_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", security_info=%08x", req->security_info );
    dump_varargs_security_descriptor( ", sd=", cur_size );
}

static void dump_get_security_object_request( const struct get_security_object_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", security_info=%08x", req->security_info );
}

static void dump_get_security_object_reply( const struct get_security_object_reply *req )
{
    fprintf( stderr, " sd_len=%08x", req->sd_len );
    dump_varargs_security_descriptor( ", sd=", cur_size );
}

static void dump_get_system_handles_request( const struct get_system_handles_request *req )
{
}

static void dump_get_system_handles_reply( const struct get_system_handles_reply *req )
{
    fprintf( stderr, " count=%08x", req->count );
    dump_varargs_handle_infos( ", data=", cur_size );
}

static void dump_create_mailslot_request( const struct create_mailslot_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    dump_timeout( ", read_timeout=", &req->read_timeout );
    fprintf( stderr, ", max_msgsize=%08x", req->max_msgsize );
    dump_varargs_object_attributes( ", objattr=", cur_size );
}

static void dump_create_mailslot_reply( const struct create_mailslot_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_set_mailslot_info_request( const struct set_mailslot_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    dump_timeout( ", read_timeout=", &req->read_timeout );
    fprintf( stderr, ", flags=%08x", req->flags );
}

static void dump_set_mailslot_info_reply( const struct set_mailslot_info_reply *req )
{
    dump_timeout( " read_timeout=", &req->read_timeout );
    fprintf( stderr, ", max_msgsize=%08x", req->max_msgsize );
}

static void dump_create_directory_request( const struct create_directory_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    dump_varargs_object_attributes( ", objattr=", cur_size );
}

static void dump_create_directory_reply( const struct create_directory_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_open_directory_request( const struct open_directory_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", rootdir=%04x", req->rootdir );
    dump_varargs_unicode_str( ", directory_name=", cur_size );
}

static void dump_open_directory_reply( const struct open_directory_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_directory_entry_request( const struct get_directory_entry_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", index=%08x", req->index );
}

static void dump_get_directory_entry_reply( const struct get_directory_entry_reply *req )
{
    fprintf( stderr, " total_len=%u", req->total_len );
    fprintf( stderr, ", name_len=%u", req->name_len );
    dump_varargs_unicode_str( ", name=", min(cur_size,req->name_len) );
    dump_varargs_unicode_str( ", type=", cur_size );
}

static void dump_create_symlink_request( const struct create_symlink_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    dump_varargs_object_attributes( ", objattr=", cur_size );
    dump_varargs_unicode_str( ", target_name=", cur_size );
}

static void dump_create_symlink_reply( const struct create_symlink_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_open_symlink_request( const struct open_symlink_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", rootdir=%04x", req->rootdir );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_open_symlink_reply( const struct open_symlink_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_query_symlink_request( const struct query_symlink_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_query_symlink_reply( const struct query_symlink_reply *req )
{
    fprintf( stderr, " total=%u", req->total );
    dump_varargs_unicode_str( ", target_name=", cur_size );
}

static void dump_get_object_info_request( const struct get_object_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_object_info_reply( const struct get_object_info_reply *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", ref_count=%08x", req->ref_count );
    fprintf( stderr, ", handle_count=%08x", req->handle_count );
}

static void dump_get_object_name_request( const struct get_object_name_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_object_name_reply( const struct get_object_name_reply *req )
{
    fprintf( stderr, " total=%u", req->total );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_get_object_type_request( const struct get_object_type_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_object_type_reply( const struct get_object_type_reply *req )
{
    dump_varargs_object_type_info( " info=", cur_size );
}

static void dump_get_object_types_request( const struct get_object_types_request *req )
{
}

static void dump_get_object_types_reply( const struct get_object_types_reply *req )
{
    fprintf( stderr, " count=%d", req->count );
    dump_varargs_object_types_info( ", info=", cur_size );
}

static void dump_allocate_locally_unique_id_request( const struct allocate_locally_unique_id_request *req )
{
}

static void dump_allocate_locally_unique_id_reply( const struct allocate_locally_unique_id_reply *req )
{
    dump_luid( " luid=", &req->luid );
}

static void dump_create_device_manager_request( const struct create_device_manager_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
}

static void dump_create_device_manager_reply( const struct create_device_manager_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_create_device_request( const struct create_device_request *req )
{
    fprintf( stderr, " rootdir=%04x", req->rootdir );
    dump_uint64( ", user_ptr=", &req->user_ptr );
    fprintf( stderr, ", manager=%04x", req->manager );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_delete_device_request( const struct delete_device_request *req )
{
    fprintf( stderr, " manager=%04x", req->manager );
    dump_uint64( ", device=", &req->device );
}

static void dump_get_next_device_request_request( const struct get_next_device_request_request *req )
{
    fprintf( stderr, " manager=%04x", req->manager );
    fprintf( stderr, ", prev=%04x", req->prev );
    fprintf( stderr, ", status=%08x", req->status );
    dump_uint64( ", user_ptr=", &req->user_ptr );
    fprintf( stderr, ", pending=%d", req->pending );
    fprintf( stderr, ", iosb_status=%08x", req->iosb_status );
    fprintf( stderr, ", result=%u", req->result );
    dump_varargs_bytes( ", data=", cur_size );
}

static void dump_get_next_device_request_reply( const struct get_next_device_request_reply *req )
{
    dump_irp_params( " params=", &req->params );
    fprintf( stderr, ", next=%04x", req->next );
    fprintf( stderr, ", client_tid=%04x", req->client_tid );
    dump_uint64( ", client_thread=", &req->client_thread );
    fprintf( stderr, ", in_size=%u", req->in_size );
    dump_varargs_bytes( ", next_data=", cur_size );
}

static void dump_get_kernel_object_ptr_request( const struct get_kernel_object_ptr_request *req )
{
    fprintf( stderr, " manager=%04x", req->manager );
    fprintf( stderr, ", handle=%04x", req->handle );
}

static void dump_get_kernel_object_ptr_reply( const struct get_kernel_object_ptr_reply *req )
{
    dump_uint64( " user_ptr=", &req->user_ptr );
}

static void dump_set_kernel_object_ptr_request( const struct set_kernel_object_ptr_request *req )
{
    fprintf( stderr, " manager=%04x", req->manager );
    fprintf( stderr, ", handle=%04x", req->handle );
    dump_uint64( ", user_ptr=", &req->user_ptr );
}

static void dump_grab_kernel_object_request( const struct grab_kernel_object_request *req )
{
    fprintf( stderr, " manager=%04x", req->manager );
    dump_uint64( ", user_ptr=", &req->user_ptr );
}

static void dump_release_kernel_object_request( const struct release_kernel_object_request *req )
{
    fprintf( stderr, " manager=%04x", req->manager );
    dump_uint64( ", user_ptr=", &req->user_ptr );
}

static void dump_get_kernel_object_handle_request( const struct get_kernel_object_handle_request *req )
{
    fprintf( stderr, " manager=%04x", req->manager );
    dump_uint64( ", user_ptr=", &req->user_ptr );
    fprintf( stderr, ", access=%08x", req->access );
}

static void dump_get_kernel_object_handle_reply( const struct get_kernel_object_handle_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_make_process_system_request( const struct make_process_system_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_make_process_system_reply( const struct make_process_system_reply *req )
{
    fprintf( stderr, " event=%04x", req->event );
}

static void dump_get_token_info_request( const struct get_token_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_token_info_reply( const struct get_token_info_reply *req )
{
    dump_luid( " token_id=", &req->token_id );
    dump_luid( ", modified_id=", &req->modified_id );
    fprintf( stderr, ", session_id=%08x", req->session_id );
    fprintf( stderr, ", primary=%d", req->primary );
    fprintf( stderr, ", impersonation_level=%d", req->impersonation_level );
    fprintf( stderr, ", elevation=%d", req->elevation );
    fprintf( stderr, ", group_count=%d", req->group_count );
    fprintf( stderr, ", privilege_count=%d", req->privilege_count );
}

static void dump_create_linked_token_request( const struct create_linked_token_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_create_linked_token_reply( const struct create_linked_token_reply *req )
{
    fprintf( stderr, " linked=%04x", req->linked );
}

static void dump_create_completion_request( const struct create_completion_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", concurrent=%08x", req->concurrent );
    dump_varargs_object_attributes( ", objattr=", cur_size );
}

static void dump_create_completion_reply( const struct create_completion_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_open_completion_request( const struct open_completion_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", rootdir=%04x", req->rootdir );
    dump_varargs_unicode_str( ", filename=", cur_size );
}

static void dump_open_completion_reply( const struct open_completion_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_add_completion_request( const struct add_completion_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    dump_uint64( ", ckey=", &req->ckey );
    dump_uint64( ", cvalue=", &req->cvalue );
    dump_uint64( ", information=", &req->information );
    fprintf( stderr, ", status=%08x", req->status );
}

static void dump_remove_completion_request( const struct remove_completion_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_remove_completion_reply( const struct remove_completion_reply *req )
{
    dump_uint64( " ckey=", &req->ckey );
    dump_uint64( ", cvalue=", &req->cvalue );
    dump_uint64( ", information=", &req->information );
    fprintf( stderr, ", status=%08x", req->status );
}

static void dump_query_completion_request( const struct query_completion_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_query_completion_reply( const struct query_completion_reply *req )
{
    fprintf( stderr, " depth=%08x", req->depth );
}

static void dump_set_completion_info_request( const struct set_completion_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    dump_uint64( ", ckey=", &req->ckey );
    fprintf( stderr, ", chandle=%04x", req->chandle );
}

static void dump_add_fd_completion_request( const struct add_fd_completion_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    dump_uint64( ", cvalue=", &req->cvalue );
    dump_uint64( ", information=", &req->information );
    fprintf( stderr, ", status=%08x", req->status );
    fprintf( stderr, ", async=%d", req->async );
}

static void dump_set_fd_completion_mode_request( const struct set_fd_completion_mode_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", flags=%08x", req->flags );
}

static void dump_set_fd_disp_info_request( const struct set_fd_disp_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", flags=%08x", req->flags );
}

static void dump_set_fd_name_info_request( const struct set_fd_name_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", rootdir=%04x", req->rootdir );
    fprintf( stderr, ", namelen=%u", req->namelen );
    fprintf( stderr, ", link=%d", req->link );
    fprintf( stderr, ", flags=%08x", req->flags );
    dump_varargs_unicode_str( ", name=", min(cur_size,req->namelen) );
    dump_varargs_string( ", filename=", cur_size );
}

static void dump_set_fd_eof_info_request( const struct set_fd_eof_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    dump_uint64( ", eof=", &req->eof );
}

static void dump_get_window_layered_info_request( const struct get_window_layered_info_request *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
}

static void dump_get_window_layered_info_reply( const struct get_window_layered_info_reply *req )
{
    fprintf( stderr, " color_key=%08x", req->color_key );
    fprintf( stderr, ", alpha=%08x", req->alpha );
    fprintf( stderr, ", flags=%08x", req->flags );
}

static void dump_set_window_layered_info_request( const struct set_window_layered_info_request *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
    fprintf( stderr, ", color_key=%08x", req->color_key );
    fprintf( stderr, ", alpha=%08x", req->alpha );
    fprintf( stderr, ", flags=%08x", req->flags );
}

static void dump_alloc_user_handle_request( const struct alloc_user_handle_request *req )
{
}

static void dump_alloc_user_handle_reply( const struct alloc_user_handle_reply *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
}

static void dump_free_user_handle_request( const struct free_user_handle_request *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
}

static void dump_set_cursor_request( const struct set_cursor_request *req )
{
    fprintf( stderr, " flags=%08x", req->flags );
    fprintf( stderr, ", handle=%08x", req->handle );
    fprintf( stderr, ", show_count=%d", req->show_count );
    fprintf( stderr, ", x=%d", req->x );
    fprintf( stderr, ", y=%d", req->y );
    dump_rectangle( ", clip=", &req->clip );
}

static void dump_set_cursor_reply( const struct set_cursor_reply *req )
{
    fprintf( stderr, " prev_handle=%08x", req->prev_handle );
    fprintf( stderr, ", prev_count=%d", req->prev_count );
    fprintf( stderr, ", prev_x=%d", req->prev_x );
    fprintf( stderr, ", prev_y=%d", req->prev_y );
    fprintf( stderr, ", new_x=%d", req->new_x );
    fprintf( stderr, ", new_y=%d", req->new_y );
    dump_rectangle( ", new_clip=", &req->new_clip );
    fprintf( stderr, ", last_change=%08x", req->last_change );
}

static void dump_get_cursor_history_request( const struct get_cursor_history_request *req )
{
}

static void dump_get_cursor_history_reply( const struct get_cursor_history_reply *req )
{
    dump_varargs_cursor_positions( " history=", cur_size );
}

static void dump_get_rawinput_buffer_request( const struct get_rawinput_buffer_request *req )
{
    fprintf( stderr, " header_size=%u", req->header_size );
    fprintf( stderr, ", read_data=%d", req->read_data );
}

static void dump_get_rawinput_buffer_reply( const struct get_rawinput_buffer_reply *req )
{
    fprintf( stderr, " next_size=%u", req->next_size );
    fprintf( stderr, ", count=%08x", req->count );
    dump_varargs_bytes( ", data=", cur_size );
}

static void dump_update_rawinput_devices_request( const struct update_rawinput_devices_request *req )
{
    dump_varargs_rawinput_devices( " devices=", cur_size );
}

static void dump_create_job_request( const struct create_job_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    dump_varargs_object_attributes( ", objattr=", cur_size );
}

static void dump_create_job_reply( const struct create_job_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_open_job_request( const struct open_job_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", rootdir=%04x", req->rootdir );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_open_job_reply( const struct open_job_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_assign_job_request( const struct assign_job_request *req )
{
    fprintf( stderr, " job=%04x", req->job );
    fprintf( stderr, ", process=%04x", req->process );
}

static void dump_process_in_job_request( const struct process_in_job_request *req )
{
    fprintf( stderr, " job=%04x", req->job );
    fprintf( stderr, ", process=%04x", req->process );
}

static void dump_set_job_limits_request( const struct set_job_limits_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", limit_flags=%08x", req->limit_flags );
}

static void dump_set_job_completion_port_request( const struct set_job_completion_port_request *req )
{
    fprintf( stderr, " job=%04x", req->job );
    fprintf( stderr, ", port=%04x", req->port );
    dump_uint64( ", key=", &req->key );
}

static void dump_get_job_info_request( const struct get_job_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_job_info_reply( const struct get_job_info_reply *req )
{
    fprintf( stderr, " total_processes=%d", req->total_processes );
    fprintf( stderr, ", active_processes=%d", req->active_processes );
    dump_varargs_uints( ", pids=", cur_size );
}

static void dump_terminate_job_request( const struct terminate_job_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", status=%d", req->status );
}

static void dump_suspend_process_request( const struct suspend_process_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_resume_process_request( const struct resume_process_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_next_thread_request( const struct get_next_thread_request *req )
{
    fprintf( stderr, " process=%04x", req->process );
    fprintf( stderr, ", last=%04x", req->last );
    fprintf( stderr, ", access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", flags=%08x", req->flags );
}

static void dump_get_next_thread_reply( const struct get_next_thread_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static const dump_func req_dumpers[REQ_NB_REQUESTS] = {
    (dump_func)dump_new_process_request,
    (dump_func)dump_get_new_process_info_request,
    (dump_func)dump_new_thread_request,
    (dump_func)dump_get_startup_info_request,
    (dump_func)dump_init_process_done_request,
    (dump_func)dump_init_first_thread_request,
    (dump_func)dump_init_thread_request,
    (dump_func)dump_terminate_process_request,
    (dump_func)dump_terminate_thread_request,
    (dump_func)dump_get_process_info_request,
    (dump_func)dump_get_process_debug_info_request,
    (dump_func)dump_get_process_image_name_request,
    (dump_func)dump_get_process_vm_counters_request,
    (dump_func)dump_set_process_info_request,
    (dump_func)dump_get_thread_info_request,
    (dump_func)dump_get_thread_times_request,
    (dump_func)dump_set_thread_info_request,
    (dump_func)dump_suspend_thread_request,
    (dump_func)dump_resume_thread_request,
    (dump_func)dump_queue_apc_request,
    (dump_func)dump_get_apc_result_request,
    (dump_func)dump_close_handle_request,
    (dump_func)dump_set_handle_info_request,
    (dump_func)dump_dup_handle_request,
    (dump_func)dump_compare_objects_request,
    (dump_func)dump_make_temporary_request,
    (dump_func)dump_open_process_request,
    (dump_func)dump_open_thread_request,
    (dump_func)dump_select_request,
    (dump_func)dump_create_event_request,
    (dump_func)dump_event_op_request,
    (dump_func)dump_query_event_request,
    (dump_func)dump_open_event_request,
    (dump_func)dump_create_keyed_event_request,
    (dump_func)dump_open_keyed_event_request,
    (dump_func)dump_create_mutex_request,
    (dump_func)dump_release_mutex_request,
    (dump_func)dump_open_mutex_request,
    (dump_func)dump_query_mutex_request,
    (dump_func)dump_create_semaphore_request,
    (dump_func)dump_release_semaphore_request,
    (dump_func)dump_query_semaphore_request,
    (dump_func)dump_open_semaphore_request,
    (dump_func)dump_create_file_request,
    (dump_func)dump_open_file_object_request,
    (dump_func)dump_alloc_file_handle_request,
    (dump_func)dump_get_handle_unix_name_request,
    (dump_func)dump_get_handle_fd_request,
    (dump_func)dump_get_directory_cache_entry_request,
    (dump_func)dump_flush_request,
    (dump_func)dump_get_file_info_request,
    (dump_func)dump_get_volume_info_request,
    (dump_func)dump_lock_file_request,
    (dump_func)dump_unlock_file_request,
    (dump_func)dump_recv_socket_request,
    (dump_func)dump_send_socket_request,
    (dump_func)dump_socket_get_events_request,
    (dump_func)dump_socket_send_icmp_id_request,
    (dump_func)dump_socket_get_icmp_id_request,
    (dump_func)dump_get_next_console_request_request,
    (dump_func)dump_read_directory_changes_request,
    (dump_func)dump_read_change_request,
    (dump_func)dump_create_mapping_request,
    (dump_func)dump_open_mapping_request,
    (dump_func)dump_get_mapping_info_request,
    (dump_func)dump_get_image_map_address_request,
    (dump_func)dump_map_view_request,
    (dump_func)dump_map_image_view_request,
    (dump_func)dump_map_builtin_view_request,
    (dump_func)dump_get_image_view_info_request,
    (dump_func)dump_unmap_view_request,
    (dump_func)dump_get_mapping_committed_range_request,
    (dump_func)dump_add_mapping_committed_range_request,
    (dump_func)dump_is_same_mapping_request,
    (dump_func)dump_get_mapping_filename_request,
    (dump_func)dump_list_processes_request,
    (dump_func)dump_create_debug_obj_request,
    (dump_func)dump_wait_debug_event_request,
    (dump_func)dump_queue_exception_event_request,
    (dump_func)dump_get_exception_status_request,
    (dump_func)dump_continue_debug_event_request,
    (dump_func)dump_debug_process_request,
    (dump_func)dump_set_debug_obj_info_request,
    (dump_func)dump_read_process_memory_request,
    (dump_func)dump_write_process_memory_request,
    (dump_func)dump_create_key_request,
    (dump_func)dump_open_key_request,
    (dump_func)dump_delete_key_request,
    (dump_func)dump_flush_key_request,
    (dump_func)dump_enum_key_request,
    (dump_func)dump_set_key_value_request,
    (dump_func)dump_get_key_value_request,
    (dump_func)dump_enum_key_value_request,
    (dump_func)dump_delete_key_value_request,
    (dump_func)dump_load_registry_request,
    (dump_func)dump_unload_registry_request,
    (dump_func)dump_save_registry_request,
    (dump_func)dump_set_registry_notification_request,
    (dump_func)dump_rename_key_request,
    (dump_func)dump_create_timer_request,
    (dump_func)dump_open_timer_request,
    (dump_func)dump_set_timer_request,
    (dump_func)dump_cancel_timer_request,
    (dump_func)dump_get_timer_info_request,
    (dump_func)dump_get_thread_context_request,
    (dump_func)dump_set_thread_context_request,
    (dump_func)dump_get_selector_entry_request,
    (dump_func)dump_add_atom_request,
    (dump_func)dump_delete_atom_request,
    (dump_func)dump_find_atom_request,
    (dump_func)dump_get_atom_information_request,
    (dump_func)dump_get_msg_queue_request,
    (dump_func)dump_set_queue_fd_request,
    (dump_func)dump_set_queue_mask_request,
    (dump_func)dump_get_queue_status_request,
    (dump_func)dump_get_process_idle_event_request,
    (dump_func)dump_send_message_request,
    (dump_func)dump_post_quit_message_request,
    (dump_func)dump_send_hardware_message_request,
    (dump_func)dump_get_message_request,
    (dump_func)dump_reply_message_request,
    (dump_func)dump_accept_hardware_message_request,
    (dump_func)dump_get_message_reply_request,
    (dump_func)dump_set_win_timer_request,
    (dump_func)dump_kill_win_timer_request,
    (dump_func)dump_is_window_hung_request,
    (dump_func)dump_get_serial_info_request,
    (dump_func)dump_set_serial_info_request,
    (dump_func)dump_cancel_sync_request,
    (dump_func)dump_register_async_request,
    (dump_func)dump_cancel_async_request,
    (dump_func)dump_get_async_result_request,
    (dump_func)dump_set_async_direct_result_request,
    (dump_func)dump_read_request,
    (dump_func)dump_write_request,
    (dump_func)dump_ioctl_request,
    (dump_func)dump_set_irp_result_request,
    (dump_func)dump_create_named_pipe_request,
    (dump_func)dump_set_named_pipe_info_request,
    (dump_func)dump_create_window_request,
    (dump_func)dump_destroy_window_request,
    (dump_func)dump_get_desktop_window_request,
    (dump_func)dump_set_window_owner_request,
    (dump_func)dump_get_window_info_request,
    (dump_func)dump_set_window_info_request,
    (dump_func)dump_set_parent_request,
    (dump_func)dump_get_window_parents_request,
    (dump_func)dump_get_window_children_request,
    (dump_func)dump_get_window_children_from_point_request,
    (dump_func)dump_get_window_tree_request,
    (dump_func)dump_set_window_pos_request,
    (dump_func)dump_get_window_rectangles_request,
    (dump_func)dump_get_window_text_request,
    (dump_func)dump_set_window_text_request,
    (dump_func)dump_get_windows_offset_request,
    (dump_func)dump_get_visible_region_request,
    (dump_func)dump_get_surface_region_request,
    (dump_func)dump_get_window_region_request,
    (dump_func)dump_set_window_region_request,
    (dump_func)dump_get_update_region_request,
    (dump_func)dump_update_window_zorder_request,
    (dump_func)dump_redraw_window_request,
    (dump_func)dump_set_window_property_request,
    (dump_func)dump_remove_window_property_request,
    (dump_func)dump_get_window_property_request,
    (dump_func)dump_get_window_properties_request,
    (dump_func)dump_create_winstation_request,
    (dump_func)dump_open_winstation_request,
    (dump_func)dump_close_winstation_request,
    (dump_func)dump_get_process_winstation_request,
    (dump_func)dump_set_process_winstation_request,
    (dump_func)dump_enum_winstation_request,
    (dump_func)dump_create_desktop_request,
    (dump_func)dump_open_desktop_request,
    (dump_func)dump_open_input_desktop_request,
    (dump_func)dump_close_desktop_request,
    (dump_func)dump_get_thread_desktop_request,
    (dump_func)dump_set_thread_desktop_request,
    (dump_func)dump_enum_desktop_request,
    (dump_func)dump_set_user_object_info_request,
    (dump_func)dump_register_hotkey_request,
    (dump_func)dump_unregister_hotkey_request,
    (dump_func)dump_attach_thread_input_request,
    (dump_func)dump_get_thread_input_request,
    (dump_func)dump_get_last_input_time_request,
    (dump_func)dump_get_key_state_request,
    (dump_func)dump_set_key_state_request,
    (dump_func)dump_set_foreground_window_request,
    (dump_func)dump_set_focus_window_request,
    (dump_func)dump_set_active_window_request,
    (dump_func)dump_set_capture_window_request,
    (dump_func)dump_set_caret_window_request,
    (dump_func)dump_set_caret_info_request,
    (dump_func)dump_set_hook_request,
    (dump_func)dump_remove_hook_request,
    (dump_func)dump_start_hook_chain_request,
    (dump_func)dump_finish_hook_chain_request,
    (dump_func)dump_get_hook_info_request,
    (dump_func)dump_create_class_request,
    (dump_func)dump_destroy_class_request,
    (dump_func)dump_set_class_info_request,
    (dump_func)dump_open_clipboard_request,
    (dump_func)dump_close_clipboard_request,
    (dump_func)dump_empty_clipboard_request,
    (dump_func)dump_set_clipboard_data_request,
    (dump_func)dump_get_clipboard_data_request,
    (dump_func)dump_get_clipboard_formats_request,
    (dump_func)dump_enum_clipboard_formats_request,
    (dump_func)dump_release_clipboard_request,
    (dump_func)dump_get_clipboard_info_request,
    (dump_func)dump_set_clipboard_viewer_request,
    (dump_func)dump_add_clipboard_listener_request,
    (dump_func)dump_remove_clipboard_listener_request,
    (dump_func)dump_create_token_request,
    (dump_func)dump_open_token_request,
    (dump_func)dump_set_global_windows_request,
    (dump_func)dump_adjust_token_privileges_request,
    (dump_func)dump_get_token_privileges_request,
    (dump_func)dump_check_token_privileges_request,
    (dump_func)dump_duplicate_token_request,
    (dump_func)dump_filter_token_request,
    (dump_func)dump_access_check_request,
    (dump_func)dump_get_token_sid_request,
    (dump_func)dump_get_token_groups_request,
    (dump_func)dump_get_token_default_dacl_request,
    (dump_func)dump_set_token_default_dacl_request,
    (dump_func)dump_set_security_object_request,
    (dump_func)dump_get_security_object_request,
    (dump_func)dump_get_system_handles_request,
    (dump_func)dump_create_mailslot_request,
    (dump_func)dump_set_mailslot_info_request,
    (dump_func)dump_create_directory_request,
    (dump_func)dump_open_directory_request,
    (dump_func)dump_get_directory_entry_request,
    (dump_func)dump_create_symlink_request,
    (dump_func)dump_open_symlink_request,
    (dump_func)dump_query_symlink_request,
    (dump_func)dump_get_object_info_request,
    (dump_func)dump_get_object_name_request,
    (dump_func)dump_get_object_type_request,
    (dump_func)dump_get_object_types_request,
    (dump_func)dump_allocate_locally_unique_id_request,
    (dump_func)dump_create_device_manager_request,
    (dump_func)dump_create_device_request,
    (dump_func)dump_delete_device_request,
    (dump_func)dump_get_next_device_request_request,
    (dump_func)dump_get_kernel_object_ptr_request,
    (dump_func)dump_set_kernel_object_ptr_request,
    (dump_func)dump_grab_kernel_object_request,
    (dump_func)dump_release_kernel_object_request,
    (dump_func)dump_get_kernel_object_handle_request,
    (dump_func)dump_make_process_system_request,
    (dump_func)dump_get_token_info_request,
    (dump_func)dump_create_linked_token_request,
    (dump_func)dump_create_completion_request,
    (dump_func)dump_open_completion_request,
    (dump_func)dump_add_completion_request,
    (dump_func)dump_remove_completion_request,
    (dump_func)dump_query_completion_request,
    (dump_func)dump_set_completion_info_request,
    (dump_func)dump_add_fd_completion_request,
    (dump_func)dump_set_fd_completion_mode_request,
    (dump_func)dump_set_fd_disp_info_request,
    (dump_func)dump_set_fd_name_info_request,
    (dump_func)dump_set_fd_eof_info_request,
    (dump_func)dump_get_window_layered_info_request,
    (dump_func)dump_set_window_layered_info_request,
    (dump_func)dump_alloc_user_handle_request,
    (dump_func)dump_free_user_handle_request,
    (dump_func)dump_set_cursor_request,
    (dump_func)dump_get_cursor_history_request,
    (dump_func)dump_get_rawinput_buffer_request,
    (dump_func)dump_update_rawinput_devices_request,
    (dump_func)dump_create_job_request,
    (dump_func)dump_open_job_request,
    (dump_func)dump_assign_job_request,
    (dump_func)dump_process_in_job_request,
    (dump_func)dump_set_job_limits_request,
    (dump_func)dump_set_job_completion_port_request,
    (dump_func)dump_get_job_info_request,
    (dump_func)dump_terminate_job_request,
    (dump_func)dump_suspend_process_request,
    (dump_func)dump_resume_process_request,
    (dump_func)dump_get_next_thread_request,
};

static const dump_func reply_dumpers[REQ_NB_REQUESTS] = {
    (dump_func)dump_new_process_reply,
    (dump_func)dump_get_new_process_info_reply,
    (dump_func)dump_new_thread_reply,
    (dump_func)dump_get_startup_info_reply,
    (dump_func)dump_init_process_done_reply,
    (dump_func)dump_init_first_thread_reply,
    (dump_func)dump_init_thread_reply,
    (dump_func)dump_terminate_process_reply,
    (dump_func)dump_terminate_thread_reply,
    (dump_func)dump_get_process_info_reply,
    (dump_func)dump_get_process_debug_info_reply,
    (dump_func)dump_get_process_image_name_reply,
    (dump_func)dump_get_process_vm_counters_reply,
    NULL,
    (dump_func)dump_get_thread_info_reply,
    (dump_func)dump_get_thread_times_reply,
    NULL,
    (dump_func)dump_suspend_thread_reply,
    (dump_func)dump_resume_thread_reply,
    (dump_func)dump_queue_apc_reply,
    (dump_func)dump_get_apc_result_reply,
    NULL,
    (dump_func)dump_set_handle_info_reply,
    (dump_func)dump_dup_handle_reply,
    NULL,
    NULL,
    (dump_func)dump_open_process_reply,
    (dump_func)dump_open_thread_reply,
    (dump_func)dump_select_reply,
    (dump_func)dump_create_event_reply,
    (dump_func)dump_event_op_reply,
    (dump_func)dump_query_event_reply,
    (dump_func)dump_open_event_reply,
    (dump_func)dump_create_keyed_event_reply,
    (dump_func)dump_open_keyed_event_reply,
    (dump_func)dump_create_mutex_reply,
    (dump_func)dump_release_mutex_reply,
    (dump_func)dump_open_mutex_reply,
    (dump_func)dump_query_mutex_reply,
    (dump_func)dump_create_semaphore_reply,
    (dump_func)dump_release_semaphore_reply,
    (dump_func)dump_query_semaphore_reply,
    (dump_func)dump_open_semaphore_reply,
    (dump_func)dump_create_file_reply,
    (dump_func)dump_open_file_object_reply,
    (dump_func)dump_alloc_file_handle_reply,
    (dump_func)dump_get_handle_unix_name_reply,
    (dump_func)dump_get_handle_fd_reply,
    (dump_func)dump_get_directory_cache_entry_reply,
    (dump_func)dump_flush_reply,
    (dump_func)dump_get_file_info_reply,
    (dump_func)dump_get_volume_info_reply,
    (dump_func)dump_lock_file_reply,
    NULL,
    (dump_func)dump_recv_socket_reply,
    (dump_func)dump_send_socket_reply,
    (dump_func)dump_socket_get_events_reply,
    NULL,
    (dump_func)dump_socket_get_icmp_id_reply,
    (dump_func)dump_get_next_console_request_reply,
    NULL,
    (dump_func)dump_read_change_reply,
    (dump_func)dump_create_mapping_reply,
    (dump_func)dump_open_mapping_reply,
    (dump_func)dump_get_mapping_info_reply,
    (dump_func)dump_get_image_map_address_reply,
    NULL,
    NULL,
    NULL,
    (dump_func)dump_get_image_view_info_reply,
    NULL,
    (dump_func)dump_get_mapping_committed_range_reply,
    NULL,
    NULL,
    (dump_func)dump_get_mapping_filename_reply,
    (dump_func)dump_list_processes_reply,
    (dump_func)dump_create_debug_obj_reply,
    (dump_func)dump_wait_debug_event_reply,
    (dump_func)dump_queue_exception_event_reply,
    NULL,
    NULL,
    NULL,
    NULL,
    (dump_func)dump_read_process_memory_reply,
    NULL,
    (dump_func)dump_create_key_reply,
    (dump_func)dump_open_key_reply,
    NULL,
    NULL,
    (dump_func)dump_enum_key_reply,
    NULL,
    (dump_func)dump_get_key_value_reply,
    (dump_func)dump_enum_key_value_reply,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    (dump_func)dump_create_timer_reply,
    (dump_func)dump_open_timer_reply,
    (dump_func)dump_set_timer_reply,
    (dump_func)dump_cancel_timer_reply,
    (dump_func)dump_get_timer_info_reply,
    (dump_func)dump_get_thread_context_reply,
    (dump_func)dump_set_thread_context_reply,
    (dump_func)dump_get_selector_entry_reply,
    (dump_func)dump_add_atom_reply,
    NULL,
    (dump_func)dump_find_atom_reply,
    (dump_func)dump_get_atom_information_reply,
    (dump_func)dump_get_msg_queue_reply,
    NULL,
    (dump_func)dump_set_queue_mask_reply,
    (dump_func)dump_get_queue_status_reply,
    (dump_func)dump_get_process_idle_event_reply,
    NULL,
    NULL,
    (dump_func)dump_send_hardware_message_reply,
    (dump_func)dump_get_message_reply,
    NULL,
    NULL,
    (dump_func)dump_get_message_reply_reply,
    (dump_func)dump_set_win_timer_reply,
    NULL,
    (dump_func)dump_is_window_hung_reply,
    (dump_func)dump_get_serial_info_reply,
    NULL,
    NULL,
    NULL,
    NULL,
    (dump_func)dump_get_async_result_reply,
    (dump_func)dump_set_async_direct_result_reply,
    (dump_func)dump_read_reply,
    (dump_func)dump_write_reply,
    (dump_func)dump_ioctl_reply,
    NULL,
    (dump_func)dump_create_named_pipe_reply,
    NULL,
    (dump_func)dump_create_window_reply,
    NULL,
    (dump_func)dump_get_desktop_window_reply,
    (dump_func)dump_set_window_owner_reply,
    (dump_func)dump_get_window_info_reply,
    (dump_func)dump_set_window_info_reply,
    (dump_func)dump_set_parent_reply,
    (dump_func)dump_get_window_parents_reply,
    (dump_func)dump_get_window_children_reply,
    (dump_func)dump_get_window_children_from_point_reply,
    (dump_func)dump_get_window_tree_reply,
    (dump_func)dump_set_window_pos_reply,
    (dump_func)dump_get_window_rectangles_reply,
    (dump_func)dump_get_window_text_reply,
    NULL,
    (dump_func)dump_get_windows_offset_reply,
    (dump_func)dump_get_visible_region_reply,
    (dump_func)dump_get_surface_region_reply,
    (dump_func)dump_get_window_region_reply,
    NULL,
    (dump_func)dump_get_update_region_reply,
    NULL,
    NULL,
    NULL,
    (dump_func)dump_remove_window_property_reply,
    (dump_func)dump_get_window_property_reply,
    (dump_func)dump_get_window_properties_reply,
    (dump_func)dump_create_winstation_reply,
    (dump_func)dump_open_winstation_reply,
    NULL,
    (dump_func)dump_get_process_winstation_reply,
    NULL,
    (dump_func)dump_enum_winstation_reply,
    (dump_func)dump_create_desktop_reply,
    (dump_func)dump_open_desktop_reply,
    (dump_func)dump_open_input_desktop_reply,
    NULL,
    (dump_func)dump_get_thread_desktop_reply,
    NULL,
    (dump_func)dump_enum_desktop_reply,
    (dump_func)dump_set_user_object_info_reply,
    (dump_func)dump_register_hotkey_reply,
    (dump_func)dump_unregister_hotkey_reply,
    NULL,
    (dump_func)dump_get_thread_input_reply,
    (dump_func)dump_get_last_input_time_reply,
    (dump_func)dump_get_key_state_reply,
    NULL,
    (dump_func)dump_set_foreground_window_reply,
    (dump_func)dump_set_focus_window_reply,
    (dump_func)dump_set_active_window_reply,
    (dump_func)dump_set_capture_window_reply,
    (dump_func)dump_set_caret_window_reply,
    (dump_func)dump_set_caret_info_reply,
    (dump_func)dump_set_hook_reply,
    (dump_func)dump_remove_hook_reply,
    (dump_func)dump_start_hook_chain_reply,
    NULL,
    (dump_func)dump_get_hook_info_reply,
    (dump_func)dump_create_class_reply,
    (dump_func)dump_destroy_class_reply,
    (dump_func)dump_set_class_info_reply,
    (dump_func)dump_open_clipboard_reply,
    (dump_func)dump_close_clipboard_reply,
    NULL,
    (dump_func)dump_set_clipboard_data_reply,
    (dump_func)dump_get_clipboard_data_reply,
    (dump_func)dump_get_clipboard_formats_reply,
    (dump_func)dump_enum_clipboard_formats_reply,
    (dump_func)dump_release_clipboard_reply,
    (dump_func)dump_get_clipboard_info_reply,
    (dump_func)dump_set_clipboard_viewer_reply,
    NULL,
    NULL,
    (dump_func)dump_create_token_reply,
    (dump_func)dump_open_token_reply,
    (dump_func)dump_set_global_windows_reply,
    (dump_func)dump_adjust_token_privileges_reply,
    (dump_func)dump_get_token_privileges_reply,
    (dump_func)dump_check_token_privileges_reply,
    (dump_func)dump_duplicate_token_reply,
    (dump_func)dump_filter_token_reply,
    (dump_func)dump_access_check_reply,
    (dump_func)dump_get_token_sid_reply,
    (dump_func)dump_get_token_groups_reply,
    (dump_func)dump_get_token_default_dacl_reply,
    NULL,
    NULL,
    (dump_func)dump_get_security_object_reply,
    (dump_func)dump_get_system_handles_reply,
    (dump_func)dump_create_mailslot_reply,
    (dump_func)dump_set_mailslot_info_reply,
    (dump_func)dump_create_directory_reply,
    (dump_func)dump_open_directory_reply,
    (dump_func)dump_get_directory_entry_reply,
    (dump_func)dump_create_symlink_reply,
    (dump_func)dump_open_symlink_reply,
    (dump_func)dump_query_symlink_reply,
    (dump_func)dump_get_object_info_reply,
    (dump_func)dump_get_object_name_reply,
    (dump_func)dump_get_object_type_reply,
    (dump_func)dump_get_object_types_reply,
    (dump_func)dump_allocate_locally_unique_id_reply,
    (dump_func)dump_create_device_manager_reply,
    NULL,
    NULL,
    (dump_func)dump_get_next_device_request_reply,
    (dump_func)dump_get_kernel_object_ptr_reply,
    NULL,
    NULL,
    NULL,
    (dump_func)dump_get_kernel_object_handle_reply,
    (dump_func)dump_make_process_system_reply,
    (dump_func)dump_get_token_info_reply,
    (dump_func)dump_create_linked_token_reply,
    (dump_func)dump_create_completion_reply,
    (dump_func)dump_open_completion_reply,
    NULL,
    (dump_func)dump_remove_completion_reply,
    (dump_func)dump_query_completion_reply,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    (dump_func)dump_get_window_layered_info_reply,
    NULL,
    (dump_func)dump_alloc_user_handle_reply,
    NULL,
    (dump_func)dump_set_cursor_reply,
    (dump_func)dump_get_cursor_history_reply,
    (dump_func)dump_get_rawinput_buffer_reply,
    NULL,
    (dump_func)dump_create_job_reply,
    (dump_func)dump_open_job_reply,
    NULL,
    NULL,
    NULL,
    NULL,
    (dump_func)dump_get_job_info_reply,
    NULL,
    NULL,
    NULL,
    (dump_func)dump_get_next_thread_reply,
};

static const char * const req_names[REQ_NB_REQUESTS] = {
    "new_process",
    "get_new_process_info",
    "new_thread",
    "get_startup_info",
    "init_process_done",
    "init_first_thread",
    "init_thread",
    "terminate_process",
    "terminate_thread",
    "get_process_info",
    "get_process_debug_info",
    "get_process_image_name",
    "get_process_vm_counters",
    "set_process_info",
    "get_thread_info",
    "get_thread_times",
    "set_thread_info",
    "suspend_thread",
    "resume_thread",
    "queue_apc",
    "get_apc_result",
    "close_handle",
    "set_handle_info",
    "dup_handle",
    "compare_objects",
    "make_temporary",
    "open_process",
    "open_thread",
    "select",
    "create_event",
    "event_op",
    "query_event",
    "open_event",
    "create_keyed_event",
    "open_keyed_event",
    "create_mutex",
    "release_mutex",
    "open_mutex",
    "query_mutex",
    "create_semaphore",
    "release_semaphore",
    "query_semaphore",
    "open_semaphore",
    "create_file",
    "open_file_object",
    "alloc_file_handle",
    "get_handle_unix_name",
    "get_handle_fd",
    "get_directory_cache_entry",
    "flush",
    "get_file_info",
    "get_volume_info",
    "lock_file",
    "unlock_file",
    "recv_socket",
    "send_socket",
    "socket_get_events",
    "socket_send_icmp_id",
    "socket_get_icmp_id",
    "get_next_console_request",
    "read_directory_changes",
    "read_change",
    "create_mapping",
    "open_mapping",
    "get_mapping_info",
    "get_image_map_address",
    "map_view",
    "map_image_view",
    "map_builtin_view",
    "get_image_view_info",
    "unmap_view",
    "get_mapping_committed_range",
    "add_mapping_committed_range",
    "is_same_mapping",
    "get_mapping_filename",
    "list_processes",
    "create_debug_obj",
    "wait_debug_event",
    "queue_exception_event",
    "get_exception_status",
    "continue_debug_event",
    "debug_process",
    "set_debug_obj_info",
    "read_process_memory",
    "write_process_memory",
    "create_key",
    "open_key",
    "delete_key",
    "flush_key",
    "enum_key",
    "set_key_value",
    "get_key_value",
    "enum_key_value",
    "delete_key_value",
    "load_registry",
    "unload_registry",
    "save_registry",
    "set_registry_notification",
    "rename_key",
    "create_timer",
    "open_timer",
    "set_timer",
    "cancel_timer",
    "get_timer_info",
    "get_thread_context",
    "set_thread_context",
    "get_selector_entry",
    "add_atom",
    "delete_atom",
    "find_atom",
    "get_atom_information",
    "get_msg_queue",
    "set_queue_fd",
    "set_queue_mask",
    "get_queue_status",
    "get_process_idle_event",
    "send_message",
    "post_quit_message",
    "send_hardware_message",
    "get_message",
    "reply_message",
    "accept_hardware_message",
    "get_message_reply",
    "set_win_timer",
    "kill_win_timer",
    "is_window_hung",
    "get_serial_info",
    "set_serial_info",
    "cancel_sync",
    "register_async",
    "cancel_async",
    "get_async_result",
    "set_async_direct_result",
    "read",
    "write",
    "ioctl",
    "set_irp_result",
    "create_named_pipe",
    "set_named_pipe_info",
    "create_window",
    "destroy_window",
    "get_desktop_window",
    "set_window_owner",
    "get_window_info",
    "set_window_info",
    "set_parent",
    "get_window_parents",
    "get_window_children",
    "get_window_children_from_point",
    "get_window_tree",
    "set_window_pos",
    "get_window_rectangles",
    "get_window_text",
    "set_window_text",
    "get_windows_offset",
    "get_visible_region",
    "get_surface_region",
    "get_window_region",
    "set_window_region",
    "get_update_region",
    "update_window_zorder",
    "redraw_window",
    "set_window_property",
    "remove_window_property",
    "get_window_property",
    "get_window_properties",
    "create_winstation",
    "open_winstation",
    "close_winstation",
    "get_process_winstation",
    "set_process_winstation",
    "enum_winstation",
    "create_desktop",
    "open_desktop",
    "open_input_desktop",
    "close_desktop",
    "get_thread_desktop",
    "set_thread_desktop",
    "enum_desktop",
    "set_user_object_info",
    "register_hotkey",
    "unregister_hotkey",
    "attach_thread_input",
    "get_thread_input",
    "get_last_input_time",
    "get_key_state",
    "set_key_state",
    "set_foreground_window",
    "set_focus_window",
    "set_active_window",
    "set_capture_window",
    "set_caret_window",
    "set_caret_info",
    "set_hook",
    "remove_hook",
    "start_hook_chain",
    "finish_hook_chain",
    "get_hook_info",
    "create_class",
    "destroy_class",
    "set_class_info",
    "open_clipboard",
    "close_clipboard",
    "empty_clipboard",
    "set_clipboard_data",
    "get_clipboard_data",
    "get_clipboard_formats",
    "enum_clipboard_formats",
    "release_clipboard",
    "get_clipboard_info",
    "set_clipboard_viewer",
    "add_clipboard_listener",
    "remove_clipboard_listener",
    "create_token",
    "open_token",
    "set_global_windows",
    "adjust_token_privileges",
    "get_token_privileges",
    "check_token_privileges",
    "duplicate_token",
    "filter_token",
    "access_check",
    "get_token_sid",
    "get_token_groups",
    "get_token_default_dacl",
    "set_token_default_dacl",
    "set_security_object",
    "get_security_object",
    "get_system_handles",
    "create_mailslot",
    "set_mailslot_info",
    "create_directory",
    "open_directory",
    "get_directory_entry",
    "create_symlink",
    "open_symlink",
    "query_symlink",
    "get_object_info",
    "get_object_name",
    "get_object_type",
    "get_object_types",
    "allocate_locally_unique_id",
    "create_device_manager",
    "create_device",
    "delete_device",
    "get_next_device_request",
    "get_kernel_object_ptr",
    "set_kernel_object_ptr",
    "grab_kernel_object",
    "release_kernel_object",
    "get_kernel_object_handle",
    "make_process_system",
    "get_token_info",
    "create_linked_token",
    "create_completion",
    "open_completion",
    "add_completion",
    "remove_completion",
    "query_completion",
    "set_completion_info",
    "add_fd_completion",
    "set_fd_completion_mode",
    "set_fd_disp_info",
    "set_fd_name_info",
    "set_fd_eof_info",
    "get_window_layered_info",
    "set_window_layered_info",
    "alloc_user_handle",
    "free_user_handle",
    "set_cursor",
    "get_cursor_history",
    "get_rawinput_buffer",
    "update_rawinput_devices",
    "create_job",
    "open_job",
    "assign_job",
    "process_in_job",
    "set_job_limits",
    "set_job_completion_port",
    "get_job_info",
    "terminate_job",
    "suspend_process",
    "resume_process",
    "get_next_thread",
};

static const struct
{
    const char  *name;
    unsigned int value;
} status_names[] =
{
    { "ABANDONED_WAIT_0",            STATUS_ABANDONED_WAIT_0 },
    { "ACCESS_DENIED",               STATUS_ACCESS_DENIED },
    { "ACCESS_VIOLATION",            STATUS_ACCESS_VIOLATION },
    { "ADDRESS_ALREADY_ASSOCIATED",  STATUS_ADDRESS_ALREADY_ASSOCIATED },
    { "ALERTED",                     STATUS_ALERTED },
    { "BAD_DEVICE_TYPE",             STATUS_BAD_DEVICE_TYPE },
    { "BAD_IMPERSONATION_LEVEL",     STATUS_BAD_IMPERSONATION_LEVEL },
    { "BUFFER_OVERFLOW",             STATUS_BUFFER_OVERFLOW },
    { "BUFFER_TOO_SMALL",            STATUS_BUFFER_TOO_SMALL },
    { "CANCELLED",                   STATUS_CANCELLED },
    { "CANNOT_DELETE",               STATUS_CANNOT_DELETE },
    { "CANT_OPEN_ANONYMOUS",         STATUS_CANT_OPEN_ANONYMOUS },
    { "CHILD_MUST_BE_VOLATILE",      STATUS_CHILD_MUST_BE_VOLATILE },
    { "CONNECTION_ABORTED",          STATUS_CONNECTION_ABORTED },
    { "CONNECTION_ACTIVE",           STATUS_CONNECTION_ACTIVE },
    { "CONNECTION_REFUSED",          STATUS_CONNECTION_REFUSED },
    { "CONNECTION_RESET",            STATUS_CONNECTION_RESET },
    { "DEBUGGER_INACTIVE",           STATUS_DEBUGGER_INACTIVE },
    { "DEVICE_BUSY",                 STATUS_DEVICE_BUSY },
    { "DEVICE_NOT_READY",            STATUS_DEVICE_NOT_READY },
    { "DIRECTORY_NOT_EMPTY",         STATUS_DIRECTORY_NOT_EMPTY },
    { "DISK_FULL",                   STATUS_DISK_FULL },
    { "ERROR_CLASS_ALREADY_EXISTS",  0xc0010000 | ERROR_CLASS_ALREADY_EXISTS },
    { "ERROR_CLASS_DOES_NOT_EXIST",  0xc0010000 | ERROR_CLASS_DOES_NOT_EXIST },
    { "ERROR_CLASS_HAS_WINDOWS",     0xc0010000 | ERROR_CLASS_HAS_WINDOWS },
    { "ERROR_CLIPBOARD_NOT_OPEN",    0xc0010000 | ERROR_CLIPBOARD_NOT_OPEN },
    { "ERROR_HOTKEY_ALREADY_REGISTERED", 0xc0010000 | ERROR_HOTKEY_ALREADY_REGISTERED },
    { "ERROR_HOTKEY_NOT_REGISTERED", 0xc0010000 | ERROR_HOTKEY_NOT_REGISTERED },
    { "ERROR_INVALID_CURSOR_HANDLE", 0xc0010000 | ERROR_INVALID_CURSOR_HANDLE },
    { "ERROR_INVALID_INDEX",         0xc0010000 | ERROR_INVALID_INDEX },
    { "ERROR_INVALID_WINDOW_HANDLE", 0xc0010000 | ERROR_INVALID_WINDOW_HANDLE },
    { "ERROR_NO_MORE_USER_HANDLES",  0xc0010000 | ERROR_NO_MORE_USER_HANDLES },
    { "ERROR_WINDOW_OF_OTHER_THREAD", 0xc0010000 | ERROR_WINDOW_OF_OTHER_THREAD },
    { "FILE_DELETED",                STATUS_FILE_DELETED },
    { "FILE_INVALID",                STATUS_FILE_INVALID },
    { "FILE_IS_A_DIRECTORY",         STATUS_FILE_IS_A_DIRECTORY },
    { "FILE_LOCK_CONFLICT",          STATUS_FILE_LOCK_CONFLICT },
    { "GENERIC_NOT_MAPPED",          STATUS_GENERIC_NOT_MAPPED },
    { "HANDLES_CLOSED",              STATUS_HANDLES_CLOSED },
    { "HANDLE_NOT_CLOSABLE",         STATUS_HANDLE_NOT_CLOSABLE },
    { "HOST_UNREACHABLE",            STATUS_HOST_UNREACHABLE },
    { "ILLEGAL_FUNCTION",            STATUS_ILLEGAL_FUNCTION },
    { "IMAGE_MACHINE_TYPE_MISMATCH", STATUS_IMAGE_MACHINE_TYPE_MISMATCH },
    { "IMAGE_NOT_AT_BASE",           STATUS_IMAGE_NOT_AT_BASE },
    { "INFO_LENGTH_MISMATCH",        STATUS_INFO_LENGTH_MISMATCH },
    { "INSTANCE_NOT_AVAILABLE",      STATUS_INSTANCE_NOT_AVAILABLE },
    { "INSUFFICIENT_RESOURCES",      STATUS_INSUFFICIENT_RESOURCES },
    { "INVALID_ACL",                 STATUS_INVALID_ACL },
    { "INVALID_ADDRESS",             STATUS_INVALID_ADDRESS },
    { "INVALID_ADDRESS_COMPONENT",   STATUS_INVALID_ADDRESS_COMPONENT },
    { "INVALID_CID",                 STATUS_INVALID_CID },
    { "INVALID_CONNECTION",          STATUS_INVALID_CONNECTION },
    { "INVALID_DEVICE_REQUEST",      STATUS_INVALID_DEVICE_REQUEST },
    { "INVALID_FILE_FOR_SECTION",    STATUS_INVALID_FILE_FOR_SECTION },
    { "INVALID_HANDLE",              STATUS_INVALID_HANDLE },
    { "INVALID_IMAGE_FORMAT",        STATUS_INVALID_IMAGE_FORMAT },
    { "INVALID_IMAGE_NE_FORMAT",     STATUS_INVALID_IMAGE_NE_FORMAT },
    { "INVALID_IMAGE_NOT_MZ",        STATUS_INVALID_IMAGE_NOT_MZ },
    { "INVALID_IMAGE_PROTECT",       STATUS_INVALID_IMAGE_PROTECT },
    { "INVALID_IMAGE_WIN_16",        STATUS_INVALID_IMAGE_WIN_16 },
    { "INVALID_IMAGE_WIN_64",        STATUS_INVALID_IMAGE_WIN_64 },
    { "INVALID_LOCK_SEQUENCE",       STATUS_INVALID_LOCK_SEQUENCE },
    { "INVALID_OWNER",               STATUS_INVALID_OWNER },
    { "INVALID_PARAMETER",           STATUS_INVALID_PARAMETER },
    { "INVALID_PIPE_STATE",          STATUS_INVALID_PIPE_STATE },
    { "INVALID_READ_MODE",           STATUS_INVALID_READ_MODE },
    { "INVALID_SECURITY_DESCR",      STATUS_INVALID_SECURITY_DESCR },
    { "IO_TIMEOUT",                  STATUS_IO_TIMEOUT },
    { "KERNEL_APC",                  STATUS_KERNEL_APC },
    { "KEY_DELETED",                 STATUS_KEY_DELETED },
    { "MAPPED_FILE_SIZE_ZERO",       STATUS_MAPPED_FILE_SIZE_ZERO },
    { "MUTANT_NOT_OWNED",            STATUS_MUTANT_NOT_OWNED },
    { "NAME_TOO_LONG",               STATUS_NAME_TOO_LONG },
    { "NETWORK_BUSY",                STATUS_NETWORK_BUSY },
    { "NETWORK_UNREACHABLE",         STATUS_NETWORK_UNREACHABLE },
    { "NOT_ALL_ASSIGNED",            STATUS_NOT_ALL_ASSIGNED },
    { "NOT_A_DIRECTORY",             STATUS_NOT_A_DIRECTORY },
    { "NOT_FOUND",                   STATUS_NOT_FOUND },
    { "NOT_IMPLEMENTED",             STATUS_NOT_IMPLEMENTED },
    { "NOT_MAPPED_VIEW",             STATUS_NOT_MAPPED_VIEW },
    { "NOT_REGISTRY_FILE",           STATUS_NOT_REGISTRY_FILE },
    { "NOT_SAME_DEVICE",             STATUS_NOT_SAME_DEVICE },
    { "NOT_SAME_OBJECT",             STATUS_NOT_SAME_OBJECT },
    { "NOT_SUPPORTED",               STATUS_NOT_SUPPORTED },
    { "NO_DATA_DETECTED",            STATUS_NO_DATA_DETECTED },
    { "NO_IMPERSONATION_TOKEN",      STATUS_NO_IMPERSONATION_TOKEN },
    { "NO_MEMORY",                   STATUS_NO_MEMORY },
    { "NO_MORE_ENTRIES",             STATUS_NO_MORE_ENTRIES },
    { "NO_SUCH_DEVICE",              STATUS_NO_SUCH_DEVICE },
    { "NO_SUCH_FILE",                STATUS_NO_SUCH_FILE },
    { "NO_TOKEN",                    STATUS_NO_TOKEN },
    { "OBJECT_NAME_COLLISION",       STATUS_OBJECT_NAME_COLLISION },
    { "OBJECT_NAME_EXISTS",          STATUS_OBJECT_NAME_EXISTS },
    { "OBJECT_NAME_INVALID",         STATUS_OBJECT_NAME_INVALID },
    { "OBJECT_NAME_NOT_FOUND",       STATUS_OBJECT_NAME_NOT_FOUND },
    { "OBJECT_PATH_INVALID",         STATUS_OBJECT_PATH_INVALID },
    { "OBJECT_PATH_NOT_FOUND",       STATUS_OBJECT_PATH_NOT_FOUND },
    { "OBJECT_PATH_SYNTAX_BAD",      STATUS_OBJECT_PATH_SYNTAX_BAD },
    { "OBJECT_TYPE_MISMATCH",        STATUS_OBJECT_TYPE_MISMATCH },
    { "PENDING",                     STATUS_PENDING },
    { "PIPE_BROKEN",                 STATUS_PIPE_BROKEN },
    { "PIPE_BUSY",                   STATUS_PIPE_BUSY },
    { "PIPE_CLOSING",                STATUS_PIPE_CLOSING },
    { "PIPE_CONNECTED",              STATUS_PIPE_CONNECTED },
    { "PIPE_DISCONNECTED",           STATUS_PIPE_DISCONNECTED },
    { "PIPE_EMPTY",                  STATUS_PIPE_EMPTY },
    { "PIPE_LISTENING",              STATUS_PIPE_LISTENING },
    { "PIPE_NOT_AVAILABLE",          STATUS_PIPE_NOT_AVAILABLE },
    { "PORT_NOT_SET",                STATUS_PORT_NOT_SET },
    { "PREDEFINED_HANDLE",           STATUS_PREDEFINED_HANDLE },
    { "PRIVILEGE_NOT_HELD",          STATUS_PRIVILEGE_NOT_HELD },
    { "PROCESS_IN_JOB",              STATUS_PROCESS_IN_JOB },
    { "PROCESS_IS_TERMINATING",      STATUS_PROCESS_IS_TERMINATING },
    { "PROCESS_NOT_IN_JOB",          STATUS_PROCESS_NOT_IN_JOB },
    { "REPARSE_POINT_NOT_RESOLVED",  STATUS_REPARSE_POINT_NOT_RESOLVED },
    { "SECTION_TOO_BIG",             STATUS_SECTION_TOO_BIG },
    { "SEMAPHORE_LIMIT_EXCEEDED",    STATUS_SEMAPHORE_LIMIT_EXCEEDED },
    { "SHARING_VIOLATION",           STATUS_SHARING_VIOLATION },
    { "SHUTDOWN_IN_PROGRESS",        STATUS_SHUTDOWN_IN_PROGRESS },
    { "SUSPEND_COUNT_EXCEEDED",      STATUS_SUSPEND_COUNT_EXCEEDED },
    { "THREAD_IS_TERMINATING",       STATUS_THREAD_IS_TERMINATING },
    { "TIMEOUT",                     STATUS_TIMEOUT },
    { "TOO_MANY_OPENED_FILES",       STATUS_TOO_MANY_OPENED_FILES },
    { "UNSUCCESSFUL",                STATUS_UNSUCCESSFUL },
    { "USER_APC",                    STATUS_USER_APC },
    { "USER_MAPPED_FILE",            STATUS_USER_MAPPED_FILE },
    { "VOLUME_DISMOUNTED",           STATUS_VOLUME_DISMOUNTED },
    { "WAS_LOCKED",                  STATUS_WAS_LOCKED },
    { "WSAEACCES",                   0xc0010000 | WSAEACCES },
    { "WSAEADDRINUSE",               0xc0010000 | WSAEADDRINUSE },
    { "WSAEADDRNOTAVAIL",            0xc0010000 | WSAEADDRNOTAVAIL },
    { "WSAEAFNOSUPPORT",             0xc0010000 | WSAEAFNOSUPPORT },
    { "WSAEALREADY",                 0xc0010000 | WSAEALREADY },
    { "WSAEBADF",                    0xc0010000 | WSAEBADF },
    { "WSAECONNABORTED",             0xc0010000 | WSAECONNABORTED },
    { "WSAECONNREFUSED",             0xc0010000 | WSAECONNREFUSED },
    { "WSAECONNRESET",               0xc0010000 | WSAECONNRESET },
    { "WSAEDESTADDRREQ",             0xc0010000 | WSAEDESTADDRREQ },
    { "WSAEDQUOT",                   0xc0010000 | WSAEDQUOT },
    { "WSAEFAULT",                   0xc0010000 | WSAEFAULT },
    { "WSAEHOSTDOWN",                0xc0010000 | WSAEHOSTDOWN },
    { "WSAEHOSTUNREACH",             0xc0010000 | WSAEHOSTUNREACH },
    { "WSAEINTR",                    0xc0010000 | WSAEINTR },
    { "WSAEINVAL",                   0xc0010000 | WSAEINVAL },
    { "WSAEISCONN",                  0xc0010000 | WSAEISCONN },
    { "WSAELOOP",                    0xc0010000 | WSAELOOP },
    { "WSAEMFILE",                   0xc0010000 | WSAEMFILE },
    { "WSAEMSGSIZE",                 0xc0010000 | WSAEMSGSIZE },
    { "WSAENAMETOOLONG",             0xc0010000 | WSAENAMETOOLONG },
    { "WSAENETDOWN",                 0xc0010000 | WSAENETDOWN },
    { "WSAENETRESET",                0xc0010000 | WSAENETRESET },
    { "WSAENETUNREACH",              0xc0010000 | WSAENETUNREACH },
    { "WSAENOBUFS",                  0xc0010000 | WSAENOBUFS },
    { "WSAENOPROTOOPT",              0xc0010000 | WSAENOPROTOOPT },
    { "WSAENOTCONN",                 0xc0010000 | WSAENOTCONN },
    { "WSAENOTEMPTY",                0xc0010000 | WSAENOTEMPTY },
    { "WSAENOTSOCK",                 0xc0010000 | WSAENOTSOCK },
    { "WSAEOPNOTSUPP",               0xc0010000 | WSAEOPNOTSUPP },
    { "WSAEPFNOSUPPORT",             0xc0010000 | WSAEPFNOSUPPORT },
    { "WSAEPROCLIM",                 0xc0010000 | WSAEPROCLIM },
    { "WSAEPROTONOSUPPORT",          0xc0010000 | WSAEPROTONOSUPPORT },
    { "WSAEPROTOTYPE",               0xc0010000 | WSAEPROTOTYPE },
    { "WSAEREMOTE",                  0xc0010000 | WSAEREMOTE },
    { "WSAESHUTDOWN",                0xc0010000 | WSAESHUTDOWN },
    { "WSAESOCKTNOSUPPORT",          0xc0010000 | WSAESOCKTNOSUPPORT },
    { "WSAESTALE",                   0xc0010000 | WSAESTALE },
    { "WSAETIMEDOUT",                0xc0010000 | WSAETIMEDOUT },
    { "WSAETOOMANYREFS",             0xc0010000 | WSAETOOMANYREFS },
    { "WSAEUSERS",                   0xc0010000 | WSAEUSERS },
    { "WSAEWOULDBLOCK",              0xc0010000 | WSAEWOULDBLOCK },
    { NULL, 0 }
};

/* ### make_requests end ### */
/* Everything above this line is generated automatically by tools/make_requests */

static const char *get_status_name( unsigned int status )
{
    int i;
    static char buffer[10];

    if (status)
    {
        for (i = 0; status_names[i].name; i++)
            if (status_names[i].value == status) return status_names[i].name;
    }
    sprintf( buffer, "%x", status );
    return buffer;
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
