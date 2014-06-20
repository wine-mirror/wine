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
#include "wine/port.h"

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
#include "file.h"
#include "request.h"
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

static void dump_uints( const int *ptr, int len )
{
    fputc( '{', stderr );
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

static void dump_uint64( const char *prefix, const unsigned __int64 *val )
{
    if ((unsigned int)*val != *val)
        fprintf( stderr, "%s%x%08x", prefix, (unsigned int)(*val >> 32), (unsigned int)*val );
    else
        fprintf( stderr, "%s%08x", prefix, (unsigned int)*val );
}

static void dump_rectangle( const char *prefix, const rectangle_t *rect )
{
    fprintf( stderr, "%s{%d,%d;%d,%d}", prefix,
             rect->left, rect->top, rect->right, rect->bottom );
}

static void dump_char_info( const char *prefix, const char_info_t *info )
{
    fprintf( stderr, "%s{'", prefix );
    dump_strW( &info->ch, 1, stderr, "\'\'" );
    fprintf( stderr, "',%04x}", info->attr );
}

static void dump_ioctl_code( const char *prefix, const ioctl_code_t *code )
{
    switch(*code)
    {
#define CASE(c) case c: fprintf( stderr, "%s%s", prefix, #c ); break
        CASE(FSCTL_DISMOUNT_VOLUME);
        CASE(FSCTL_PIPE_DISCONNECT);
        CASE(FSCTL_PIPE_LISTEN);
        CASE(FSCTL_PIPE_WAIT);
        default: fprintf( stderr, "%s%08x", prefix, *code ); break;
#undef CASE
    }
}

static void dump_cpu_type( const char *prefix, const cpu_type_t *code )
{
    switch (*code)
    {
#define CASE(c) case CPU_##c: fprintf( stderr, "%s%s", prefix, #c ); break
        CASE(x86);
        CASE(x86_64);
        CASE(POWERPC);
        default: fprintf( stderr, "%s%u", prefix, *code ); break;
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
    case APC_TIMER:
        dump_timeout( "APC_TIMER,time=", &call->timer.time );
        dump_uint64( ",arg=", &call->timer.arg );
        break;
    case APC_ASYNC_IO:
        dump_uint64( "APC_ASYNC_IO,func=", &call->async_io.func );
        dump_uint64( ",user=", &call->async_io.user );
        dump_uint64( ",sb=", &call->async_io.sb );
        fprintf( stderr, ",status=%s", get_status_name(call->async_io.status) );
        break;
    case APC_VIRTUAL_ALLOC:
        dump_uint64( "APC_VIRTUAL_ALLOC,addr==", &call->virtual_alloc.addr );
        dump_uint64( ",size=", &call->virtual_alloc.size );
        fprintf( stderr, ",zero_bits=%u,op_type=%x,prot=%x",
                 call->virtual_alloc.zero_bits, call->virtual_alloc.op_type,
                 call->virtual_alloc.prot );
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
        fprintf( stderr, ",zero_bits=%u,alloc_type=%x,prot=%x",
                 call->map_view.zero_bits, call->map_view.alloc_type, call->map_view.prot );
        break;
    case APC_UNMAP_VIEW:
        dump_uint64( "APC_UNMAP_VIEW,addr=", &call->unmap_view.addr );
        break;
    case APC_CREATE_THREAD:
        dump_uint64( "APC_CREATE_THREAD,func=", &call->create_thread.func );
        dump_uint64( ",arg=", &call->create_thread.arg );
        dump_uint64( ",reserve=", &call->create_thread.reserve );
        dump_uint64( ",commit=", &call->create_thread.commit );
        fprintf( stderr, ",suspend=%u", call->create_thread.suspend );
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
        dump_uint64( ",apc=", &result->async_io.apc );
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
        fprintf( stderr, "APC_CREATE_THREAD,status=%s,tid=%04x,handle=%04x",
                 get_status_name( result->create_thread.status ),
                 result->create_thread.tid, result->create_thread.handle );
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
    dump_uint64( ",callback=", &data->callback );
    dump_uint64( ",iosb=", &data->iosb );
    dump_uint64( ",arg=", &data->arg );
    dump_uint64( ",cvalue=", &data->cvalue );
    fputc( '}', stderr );
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
        dump_uint64( ",lparam=", &input->hw.lparam );
        fputc( '}', stderr );
        break;
    default:
        fprintf( stderr, "%s{type=%04x}", prefix, input->type );
        break;
    }
}

static void dump_luid( const char *prefix, const luid_t *luid )
{
    fprintf( stderr, "%s%d.%u", prefix, luid->high_part, luid->low_part );
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
    dump_strW( cur_data, size / sizeof(WCHAR), stderr, "\"\"" );
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

    fprintf( stderr,"%s{", prefix );
    dump_cpu_type( "cpu=", &ctx.cpu );
    switch (ctx.cpu)
    {
    case CPU_x86:
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
                fprintf( stderr, ",fp.reg%u=%Lg", i, *(long double *)&ctx.fp.i386_regs.regs[10*i] );
        }
        if (ctx.flags & SERVER_CTX_EXTENDED_REGISTERS)
        {
            fprintf( stderr, ",extended=" );
            dump_uints( (const int *)ctx.ext.i386_regs, sizeof(ctx.ext.i386_regs) / sizeof(int) );
        }
        break;
    case CPU_x86_64:
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
        {
            for (i = 0; i < 32; i++)
                fprintf( stderr, ",fp%u=%08x%08x%08x%08x", i,
                         (unsigned int)(ctx.fp.x86_64_regs.fpregs[i].high >> 32),
                         (unsigned int)ctx.fp.x86_64_regs.fpregs[i].high,
                         (unsigned int)(ctx.fp.x86_64_regs.fpregs[i].low >> 32),
                         (unsigned int)ctx.fp.x86_64_regs.fpregs[i].low );
        }
        break;
    case CPU_POWERPC:
        if (ctx.flags & SERVER_CTX_CONTROL)
            fprintf( stderr, ",iar=%08x,msr=%08x,ctr=%08x,lr=%08x,dar=%08x,dsisr=%08x,trap=%08x",
                     ctx.ctl.powerpc_regs.iar, ctx.ctl.powerpc_regs.msr, ctx.ctl.powerpc_regs.ctr,
                     ctx.ctl.powerpc_regs.lr, ctx.ctl.powerpc_regs.dar, ctx.ctl.powerpc_regs.dsisr,
                     ctx.ctl.powerpc_regs.trap );
        if (ctx.flags & SERVER_CTX_INTEGER)
        {
            for (i = 0; i < 32; i++) fprintf( stderr, ",gpr%u=%08x", i, ctx.integer.powerpc_regs.gpr[i] );
            fprintf( stderr, ",cr=%08x,xer=%08x",
                     ctx.integer.powerpc_regs.cr, ctx.integer.powerpc_regs.xer );
        }
        if (ctx.flags & SERVER_CTX_DEBUG_REGISTERS)
            for (i = 0; i < 8; i++) fprintf( stderr, ",dr%u=%08x", i, ctx.debug.powerpc_regs.dr[i] );
        if (ctx.flags & SERVER_CTX_FLOATING_POINT)
        {
            for (i = 0; i < 32; i++) fprintf( stderr, ",fpr%u=%g", i, ctx.fp.powerpc_regs.fpr[i] );
            fprintf( stderr, ",fpscr=%g", ctx.fp.powerpc_regs.fpscr );
        }
        break;
    case CPU_ARM:
        if (ctx.flags & SERVER_CTX_CONTROL)
            fprintf( stderr, ",sp=%08x,lr=%08x,pc=%08x,cpsr=%08x",
                     ctx.ctl.arm_regs.sp, ctx.ctl.arm_regs.lr,
                     ctx.ctl.arm_regs.pc, ctx.ctl.arm_regs.cpsr );
        if (ctx.flags & SERVER_CTX_INTEGER)
            for (i = 0; i < 13; i++) fprintf( stderr, ",r%u=%08x", i, ctx.integer.arm_regs.r[i] );
        break;
    case CPU_ARM64:
        if (ctx.flags & SERVER_CTX_CONTROL)
        {
            dump_uint64( ",sp=", &ctx.ctl.arm64_regs.sp );
            dump_uint64( ",pc=", &ctx.ctl.arm64_regs.pc );
            dump_uint64( ",pstate=", &ctx.ctl.arm64_regs.pstate );
        }
        if (ctx.flags & SERVER_CTX_INTEGER)
        {
            dump_uint64( ",x0=",  &ctx.integer.arm64_regs.x[0] );
            dump_uint64( ",x1=",  &ctx.integer.arm64_regs.x[1] );
            dump_uint64( ",x2=",  &ctx.integer.arm64_regs.x[2] );
            dump_uint64( ",x3=",  &ctx.integer.arm64_regs.x[3] );
            dump_uint64( ",x4=",  &ctx.integer.arm64_regs.x[4] );
            dump_uint64( ",x5=",  &ctx.integer.arm64_regs.x[5] );
            dump_uint64( ",x6=",  &ctx.integer.arm64_regs.x[6] );
            dump_uint64( ",x7=",  &ctx.integer.arm64_regs.x[7] );
            dump_uint64( ",x8=",  &ctx.integer.arm64_regs.x[8] );
            dump_uint64( ",x9=",  &ctx.integer.arm64_regs.x[9] );
            dump_uint64( ",x10=", &ctx.integer.arm64_regs.x[10] );
            dump_uint64( ",x11=", &ctx.integer.arm64_regs.x[11] );
            dump_uint64( ",x12=", &ctx.integer.arm64_regs.x[12] );
            dump_uint64( ",x13=", &ctx.integer.arm64_regs.x[13] );
            dump_uint64( ",x14=", &ctx.integer.arm64_regs.x[14] );
            dump_uint64( ",x15=", &ctx.integer.arm64_regs.x[15] );
            dump_uint64( ",x16=", &ctx.integer.arm64_regs.x[16] );
            dump_uint64( ",x17=", &ctx.integer.arm64_regs.x[17] );
            dump_uint64( ",x18=", &ctx.integer.arm64_regs.x[18] );
            dump_uint64( ",x19=", &ctx.integer.arm64_regs.x[19] );
            dump_uint64( ",x20=", &ctx.integer.arm64_regs.x[20] );
            dump_uint64( ",x21=", &ctx.integer.arm64_regs.x[21] );
            dump_uint64( ",x22=", &ctx.integer.arm64_regs.x[22] );
            dump_uint64( ",x23=", &ctx.integer.arm64_regs.x[23] );
            dump_uint64( ",x24=", &ctx.integer.arm64_regs.x[24] );
            dump_uint64( ",x25=", &ctx.integer.arm64_regs.x[25] );
            dump_uint64( ",x26=", &ctx.integer.arm64_regs.x[26] );
            dump_uint64( ",x27=", &ctx.integer.arm64_regs.x[27] );
            dump_uint64( ",x28=", &ctx.integer.arm64_regs.x[28] );
            dump_uint64( ",x29=", &ctx.integer.arm64_regs.x[29] );
            dump_uint64( ",x30=", &ctx.integer.arm64_regs.x[30] );
        }
        break;
    }
    fputc( '}', stderr );
    remove_data( size );
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
    case EXCEPTION_DEBUG_EVENT:
        fprintf( stderr, "%s{exception,first=%d,exc_code=%08x,flags=%08x", prefix,
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
    case CREATE_THREAD_DEBUG_EVENT:
        fprintf( stderr, "%s{create_thread,thread=%04x", prefix, event.create_thread.handle );
        dump_uint64( ",teb=", &event.create_thread.teb );
        dump_uint64( ",start=", &event.create_thread.start );
        fputc( '}', stderr );
        break;
    case CREATE_PROCESS_DEBUG_EVENT:
        fprintf( stderr, "%s{create_process,file=%04x,process=%04x,thread=%04x", prefix,
                 event.create_process.file, event.create_process.process,
                 event.create_process.thread );
        dump_uint64( ",base=", &event.create_process.base );
        fprintf( stderr, ",offset=%d,size=%d",
                 event.create_process.dbg_offset, event.create_process.dbg_size );
        dump_uint64( ",teb=", &event.create_process.teb );
        dump_uint64( ",start=", &event.create_process.start );
        dump_uint64( ",name=", &event.create_process.name );
        fprintf( stderr, ",unicode=%d}", event.create_process.unicode );
        break;
    case EXIT_THREAD_DEBUG_EVENT:
        fprintf( stderr, "%s{exit_thread,code=%d}", prefix, event.exit.exit_code );
        break;
    case EXIT_PROCESS_DEBUG_EVENT:
        fprintf( stderr, "%s{exit_process,code=%d}", prefix, event.exit.exit_code );
        break;
    case LOAD_DLL_DEBUG_EVENT:
        fprintf( stderr, "%s{load_dll,file=%04x", prefix, event.load_dll.handle );
        dump_uint64( ",base=", &event.load_dll.base );
        fprintf( stderr, ",offset=%d,size=%d",
                 event.load_dll.dbg_offset, event.load_dll.dbg_size );
        dump_uint64( ",name=", &event.load_dll.name );
        fprintf( stderr, ",unicode=%d}", event.load_dll.unicode );
        break;
    case UNLOAD_DLL_DEBUG_EVENT:
        fprintf( stderr, "%s{unload_dll", prefix );
        dump_uint64( ",base=", &event.unload_dll.base );
        fputc( '}', stderr );
        break;
    case RIP_EVENT:
        fprintf( stderr, "%s{rip,err=%d,type=%d}", prefix,
                 event.rip_info.error, event.rip_info.type );
        break;
    case 0:  /* zero is the code returned on timeouts */
        fprintf( stderr, "%s{}", prefix );
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
    len /= sizeof(WCHAR);
    dump_strW( (const WCHAR *)cur_data + pos/sizeof(WCHAR), len, stderr, "\"\"" );
    return pos + len * sizeof(WCHAR);
}

static void dump_varargs_startup_info( const char *prefix, data_size_t size )
{
    startup_info_t info;
    data_size_t pos = sizeof(info);

    memset( &info, 0, sizeof(info) );
    memcpy( &info, cur_data, min( size, sizeof(info) ));

    fprintf( stderr,
             "%s{debug_flags=%x,console_flags=%x,console=%04x,hstdin=%04x,hstdout=%04x,hstderr=%04x,"
             "x=%u,y=%u,xsize=%u,ysize=%u,xchars=%u,ychars=%u,attribute=%02x,flags=%x,show=%u",
             prefix, info.debug_flags, info.console_flags, info.console,
             info.hstdin, info.hstdout, info.hstderr, info.x, info.y, info.xsize, info.ysize,
             info.xchars, info.ychars, info.attribute, info.flags, info.show );
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

static void dump_varargs_input_records( const char *prefix, data_size_t size )
{
    const INPUT_RECORD *rec = cur_data;
    data_size_t len = size / sizeof(*rec);

    fprintf( stderr,"%s{", prefix );
    while (len > 0)
    {
        fprintf( stderr, "{%04x,...}", rec->EventType );
        rec++;
        if (--len) fputc( ',', stderr );
    }
    fputc( '}', stderr );
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

static void dump_varargs_LUID_AND_ATTRIBUTES( const char *prefix, data_size_t size )
{
    const LUID_AND_ATTRIBUTES *lat = cur_data;
    data_size_t len = size / sizeof(*lat);

    fprintf( stderr,"%s{", prefix );
    while (len > 0)
    {
        fprintf( stderr, "{luid=%08x%08x,attr=%x}",
                 lat->Luid.HighPart, lat->Luid.LowPart, lat->Attributes );
        lat++;
        if (--len) fputc( ',', stderr );
    }
    fputc( '}', stderr );
    remove_data( size );
}

static void dump_inline_sid( const char *prefix, const SID *sid, data_size_t size )
{
    DWORD i;

    /* security check */
    if ((FIELD_OFFSET(SID, SubAuthority[0]) > size) ||
        (FIELD_OFFSET(SID, SubAuthority[sid->SubAuthorityCount]) > size))
    {
        fprintf( stderr, "<invalid sid>" );
        return;
    }

    fprintf( stderr,"%s{", prefix );
    fprintf( stderr, "S-%u-%u", sid->Revision, MAKELONG(
        MAKEWORD( sid->IdentifierAuthority.Value[5],
                  sid->IdentifierAuthority.Value[4] ),
        MAKEWORD( sid->IdentifierAuthority.Value[3],
                  sid->IdentifierAuthority.Value[2] ) ) );
    for (i = 0; i < sid->SubAuthorityCount; i++)
        fprintf( stderr, "-%u", sid->SubAuthority[i] );
    fputc( '}', stderr );
}

static void dump_varargs_SID( const char *prefix, data_size_t size )
{
    const SID *sid = cur_data;
    dump_inline_sid( prefix, sid, size );
    remove_data( size );
}

static void dump_inline_acl( const char *prefix, const ACL *acl, data_size_t size )
{
    const ACE_HEADER *ace;
    ULONG i;

    fprintf( stderr,"%s{", prefix );
    if (size)
    {
        if (size < sizeof(ACL))
        {
            fprintf( stderr, "<invalid acl>}" );
            return;
        }
        size -= sizeof(ACL);
        ace = (const ACE_HEADER *)(acl + 1);
        for (i = 0; i < acl->AceCount; i++)
        {
            const SID *sid = NULL;
            data_size_t sid_size = 0;

            if (size < sizeof(ACE_HEADER) || size < ace->AceSize) break;
            size -= ace->AceSize;
            if (i != 0) fputc( ',', stderr );
            fprintf( stderr, "{AceType=" );
            switch (ace->AceType)
            {
            case ACCESS_DENIED_ACE_TYPE:
                sid = (const SID *)&((const ACCESS_DENIED_ACE *)ace)->SidStart;
                sid_size = ace->AceSize - FIELD_OFFSET(ACCESS_DENIED_ACE, SidStart);
                fprintf( stderr, "ACCESS_DENIED_ACE_TYPE,Mask=%x",
                         ((const ACCESS_DENIED_ACE *)ace)->Mask );
                break;
            case ACCESS_ALLOWED_ACE_TYPE:
                sid = (const SID *)&((const ACCESS_ALLOWED_ACE *)ace)->SidStart;
                sid_size = ace->AceSize - FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart);
                fprintf( stderr, "ACCESS_ALLOWED_ACE_TYPE,Mask=%x",
                         ((const ACCESS_ALLOWED_ACE *)ace)->Mask );
                break;
            case SYSTEM_AUDIT_ACE_TYPE:
                sid = (const SID *)&((const SYSTEM_AUDIT_ACE *)ace)->SidStart;
                sid_size = ace->AceSize - FIELD_OFFSET(SYSTEM_AUDIT_ACE, SidStart);
                fprintf( stderr, "SYSTEM_AUDIT_ACE_TYPE,Mask=%x",
                         ((const SYSTEM_AUDIT_ACE *)ace)->Mask );
                break;
            case SYSTEM_ALARM_ACE_TYPE:
                sid = (const SID *)&((const SYSTEM_ALARM_ACE *)ace)->SidStart;
                sid_size = ace->AceSize - FIELD_OFFSET(SYSTEM_ALARM_ACE, SidStart);
                fprintf( stderr, "SYSTEM_ALARM_ACE_TYPE,Mask=%x",
                         ((const SYSTEM_ALARM_ACE *)ace)->Mask );
                break;
            default:
                fprintf( stderr, "unknown<%d>", ace->AceType );
                break;
            }
            fprintf( stderr, ",AceFlags=%x", ace->AceFlags );
            if (sid)
                dump_inline_sid( ",Sid=", sid, sid_size );
            ace = (const ACE_HEADER *)((const char *)ace + ace->AceSize);
            fputc( '}', stderr );
        }
    }
    fputc( '}', stderr );
}

static void dump_varargs_ACL( const char *prefix, data_size_t size )
{
    const ACL *acl = cur_data;
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
        if ((sd->owner_len > FIELD_OFFSET(SID, SubAuthority[255])) || (offset + sd->owner_len > size))
            return;
        if (sd->owner_len)
            dump_inline_sid( ",owner=", (const SID *)((const char *)sd + offset), sd->owner_len );
        else
            fprintf( stderr, ",owner=<not present>" );
        offset += sd->owner_len;
        if ((sd->group_len > FIELD_OFFSET(SID, SubAuthority[255])) || (offset + sd->group_len > size))
            return;
        if (sd->group_len)
            dump_inline_sid( ",group=", (const SID *)((const char *)sd + offset), sd->group_len );
        else
            fprintf( stderr, ",group=<not present>" );
        offset += sd->group_len;
        if ((sd->sacl_len >= MAX_ACL_LEN) || (offset + sd->sacl_len > size))
            return;
        dump_inline_acl( ",sacl=", (const ACL *)((const char *)sd + offset), sd->sacl_len );
        offset += sd->sacl_len;
        if ((sd->dacl_len >= MAX_ACL_LEN) || (offset + sd->dacl_len > size))
            return;
        dump_inline_acl( ",dacl=", (const ACL *)((const char *)sd + offset), sd->dacl_len );
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

static void dump_varargs_token_groups( const char *prefix, data_size_t size )
{
    const struct token_groups *tg = cur_data;

    fprintf( stderr,"%s{", prefix );
    if (size >= sizeof(struct token_groups))
    {
        size_t offset = sizeof(*tg);
        fprintf( stderr, "count=%08x,", tg->count );
        if (tg->count * sizeof(unsigned int) <= size)
        {
            unsigned int i;
            const unsigned int *attr = (const unsigned int *)(tg + 1);

            offset += tg->count * sizeof(unsigned int);

            fputc( '[', stderr );
            for (i = 0; i < tg->count; i++)
            {
                const SID *sid = (const SID *)((const char *)cur_data + offset);
                if (i != 0)
                    fputc( ',', stderr );
                fputc( '{', stderr );
                fprintf( stderr, "attributes=%08x", attr[i] );
                dump_inline_sid( ",sid=", sid, size - offset );
                if ((offset + FIELD_OFFSET(SID, SubAuthority[0]) > size) ||
                    (offset + FIELD_OFFSET(SID, SubAuthority[sid->SubAuthorityCount]) > size))
                    break;
                offset += FIELD_OFFSET(SID, SubAuthority[sid->SubAuthorityCount]);
                fputc( '}', stderr );
            }
            fputc( ']', stderr );
        }
    }
    fputc( '}', stderr );
}

static void dump_varargs_object_attributes( const char *prefix, data_size_t size )
{
    const struct object_attributes *objattr = cur_data;

    fprintf( stderr,"%s{", prefix );
    if (size >= sizeof(struct object_attributes))
    {
        const WCHAR *str;
        fprintf( stderr, "rootdir=%04x", objattr->rootdir );
        if (objattr->sd_len > size - sizeof(*objattr) ||
            objattr->name_len > size - sizeof(*objattr) - objattr->sd_len)
            return;
        dump_inline_security_descriptor( ",sd=", (const struct security_descriptor *)(objattr + 1), objattr->sd_len );
        str = (const WCHAR *)objattr + (sizeof(*objattr) + objattr->sd_len) / sizeof(WCHAR);
        fprintf( stderr, ",name=L\"" );
        dump_strW( str, objattr->name_len / sizeof(WCHAR), stderr, "\"\"" );
        fputc( '\"', stderr );
        remove_data( ((sizeof(*objattr) + objattr->sd_len) / sizeof(WCHAR)) * sizeof(WCHAR) +
                     objattr->name_len );
    }
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
        if (event->action < sizeof(actions)/sizeof(actions[0]) && actions[event->action])
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

static void dump_varargs_rawinput_devices(const char *prefix, data_size_t size )
{
    const struct rawinput_device *device;

    fprintf( stderr, "%s{", prefix );
    while (size >= sizeof(*device))
    {
        device = cur_data;
        fprintf( stderr, "{usage_page=%04x,usage=%04x,flags=%08x,target=%08x}",
                 device->usage_page, device->usage, device->flags, device->target );
        size -= sizeof(*device);
        remove_data( sizeof(*device) );
        if (size) fputc( ',', stderr );
    }
    fputc( '}', stderr );
}

typedef void (*dump_func)( const void *req );

/* Everything below this line is generated automatically by tools/make_requests */
/* ### make_requests begin ### */

static void dump_new_process_request( const struct new_process_request *req )
{
    fprintf( stderr, " inherit_all=%d", req->inherit_all );
    fprintf( stderr, ", create_flags=%08x", req->create_flags );
    fprintf( stderr, ", socket_fd=%d", req->socket_fd );
    fprintf( stderr, ", exe_file=%04x", req->exe_file );
    fprintf( stderr, ", process_access=%08x", req->process_access );
    fprintf( stderr, ", process_attr=%08x", req->process_attr );
    fprintf( stderr, ", thread_access=%08x", req->thread_access );
    fprintf( stderr, ", thread_attr=%08x", req->thread_attr );
    dump_cpu_type( ", cpu=", &req->cpu );
    fprintf( stderr, ", info_size=%u", req->info_size );
    dump_varargs_startup_info( ", info=", min(cur_size,req->info_size) );
    dump_varargs_unicode_str( ", env=", cur_size );
}

static void dump_new_process_reply( const struct new_process_reply *req )
{
    fprintf( stderr, " info=%04x", req->info );
    fprintf( stderr, ", pid=%04x", req->pid );
    fprintf( stderr, ", phandle=%04x", req->phandle );
    fprintf( stderr, ", tid=%04x", req->tid );
    fprintf( stderr, ", thandle=%04x", req->thandle );
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
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", suspend=%d", req->suspend );
    fprintf( stderr, ", request_fd=%d", req->request_fd );
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
    fprintf( stderr, " exe_file=%04x", req->exe_file );
    fprintf( stderr, ", info_size=%u", req->info_size );
    dump_varargs_startup_info( ", info=", min(cur_size,req->info_size) );
    dump_varargs_unicode_str( ", env=", cur_size );
}

static void dump_init_process_done_request( const struct init_process_done_request *req )
{
    fprintf( stderr, " gui=%d", req->gui );
    dump_uint64( ", module=", &req->module );
    dump_uint64( ", ldt_copy=", &req->ldt_copy );
    dump_uint64( ", entry=", &req->entry );
}

static void dump_init_thread_request( const struct init_thread_request *req )
{
    fprintf( stderr, " unix_pid=%d", req->unix_pid );
    fprintf( stderr, ", unix_tid=%d", req->unix_tid );
    fprintf( stderr, ", debug_level=%d", req->debug_level );
    dump_uint64( ", teb=", &req->teb );
    dump_uint64( ", entry=", &req->entry );
    fprintf( stderr, ", reply_fd=%d", req->reply_fd );
    fprintf( stderr, ", wait_fd=%d", req->wait_fd );
    dump_cpu_type( ", cpu=", &req->cpu );
}

static void dump_init_thread_reply( const struct init_thread_reply *req )
{
    fprintf( stderr, " pid=%04x", req->pid );
    fprintf( stderr, ", tid=%04x", req->tid );
    dump_timeout( ", server_start=", &req->server_start );
    fprintf( stderr, ", info_size=%u", req->info_size );
    fprintf( stderr, ", version=%d", req->version );
    fprintf( stderr, ", all_cpus=%08x", req->all_cpus );
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
    fprintf( stderr, ", last=%d", req->last );
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
    fprintf( stderr, ", exit_code=%d", req->exit_code );
    fprintf( stderr, ", priority=%d", req->priority );
    dump_cpu_type( ", cpu=", &req->cpu );
    fprintf( stderr, ", debugger_present=%d", req->debugger_present );
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
    fprintf( stderr, ", tid_in=%04x", req->tid_in );
}

static void dump_get_thread_info_reply( const struct get_thread_info_reply *req )
{
    fprintf( stderr, " pid=%04x", req->pid );
    fprintf( stderr, ", tid=%04x", req->tid );
    dump_uint64( ", teb=", &req->teb );
    dump_uint64( ", affinity=", &req->affinity );
    dump_timeout( ", creation_time=", &req->creation_time );
    dump_timeout( ", exit_time=", &req->exit_time );
    fprintf( stderr, ", exit_code=%d", req->exit_code );
    fprintf( stderr, ", priority=%d", req->priority );
    fprintf( stderr, ", last=%d", req->last );
}

static void dump_set_thread_info_request( const struct set_thread_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", mask=%d", req->mask );
    fprintf( stderr, ", priority=%d", req->priority );
    dump_uint64( ", affinity=", &req->affinity );
    fprintf( stderr, ", token=%04x", req->token );
}

static void dump_get_dll_info_request( const struct get_dll_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    dump_uint64( ", base_address=", &req->base_address );
}

static void dump_get_dll_info_reply( const struct get_dll_info_reply *req )
{
    dump_uint64( " entry_point=", &req->entry_point );
    fprintf( stderr, ", size=%u", req->size );
    fprintf( stderr, ", filename_len=%u", req->filename_len );
    dump_varargs_unicode_str( ", filename=", cur_size );
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

static void dump_load_dll_request( const struct load_dll_request *req )
{
    fprintf( stderr, " mapping=%04x", req->mapping );
    dump_uint64( ", base=", &req->base );
    dump_uint64( ", name=", &req->name );
    fprintf( stderr, ", size=%u", req->size );
    fprintf( stderr, ", dbg_offset=%d", req->dbg_offset );
    fprintf( stderr, ", dbg_size=%d", req->dbg_size );
    dump_varargs_unicode_str( ", filename=", cur_size );
}

static void dump_unload_dll_request( const struct unload_dll_request *req )
{
    dump_uint64( " base=", &req->base );
}

static void dump_queue_apc_request( const struct queue_apc_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    dump_apc_call( ", call=", &req->call );
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
    fprintf( stderr, ", self=%d", req->self );
    fprintf( stderr, ", closed=%d", req->closed );
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
    dump_timeout( ", timeout=", &req->timeout );
    fprintf( stderr, ", prev_apc=%04x", req->prev_apc );
    dump_varargs_apc_result( ", result=", cur_size );
    dump_varargs_select_op( ", data=", cur_size );
}

static void dump_select_reply( const struct select_reply *req )
{
    dump_timeout( " timeout=", &req->timeout );
    dump_apc_call( ", call=", &req->call );
    fprintf( stderr, ", apc_handle=%04x", req->apc_handle );
}

static void dump_create_event_request( const struct create_event_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
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
    fprintf( stderr, ", attributes=%08x", req->attributes );
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
    fprintf( stderr, ", attributes=%08x", req->attributes );
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

static void dump_create_semaphore_request( const struct create_semaphore_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
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
    fprintf( stderr, ", attributes=%08x", req->attributes );
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

static void dump_flush_file_request( const struct flush_file_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_flush_file_reply( const struct flush_file_reply *req )
{
    fprintf( stderr, " event=%04x", req->event );
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

static void dump_create_socket_request( const struct create_socket_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", family=%d", req->family );
    fprintf( stderr, ", type=%d", req->type );
    fprintf( stderr, ", protocol=%d", req->protocol );
    fprintf( stderr, ", flags=%08x", req->flags );
}

static void dump_create_socket_reply( const struct create_socket_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_accept_socket_request( const struct accept_socket_request *req )
{
    fprintf( stderr, " lhandle=%04x", req->lhandle );
    fprintf( stderr, ", access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
}

static void dump_accept_socket_reply( const struct accept_socket_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_accept_into_socket_request( const struct accept_into_socket_request *req )
{
    fprintf( stderr, " lhandle=%04x", req->lhandle );
    fprintf( stderr, ", ahandle=%04x", req->ahandle );
}

static void dump_set_socket_event_request( const struct set_socket_event_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", mask=%08x", req->mask );
    fprintf( stderr, ", event=%04x", req->event );
    fprintf( stderr, ", window=%08x", req->window );
    fprintf( stderr, ", msg=%08x", req->msg );
}

static void dump_get_socket_event_request( const struct get_socket_event_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", service=%d", req->service );
    fprintf( stderr, ", c_event=%04x", req->c_event );
}

static void dump_get_socket_event_reply( const struct get_socket_event_reply *req )
{
    fprintf( stderr, " mask=%08x", req->mask );
    fprintf( stderr, ", pmask=%08x", req->pmask );
    fprintf( stderr, ", state=%08x", req->state );
    dump_varargs_ints( ", errors=", cur_size );
}

static void dump_get_socket_info_request( const struct get_socket_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_socket_info_reply( const struct get_socket_info_reply *req )
{
    fprintf( stderr, " family=%d", req->family );
    fprintf( stderr, ", type=%d", req->type );
    fprintf( stderr, ", protocol=%d", req->protocol );
}

static void dump_enable_socket_event_request( const struct enable_socket_event_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", mask=%08x", req->mask );
    fprintf( stderr, ", sstate=%08x", req->sstate );
    fprintf( stderr, ", cstate=%08x", req->cstate );
}

static void dump_set_socket_deferred_request( const struct set_socket_deferred_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", deferred=%04x", req->deferred );
}

static void dump_alloc_console_request( const struct alloc_console_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", pid=%04x", req->pid );
    fprintf( stderr, ", input_fd=%d", req->input_fd );
}

static void dump_alloc_console_reply( const struct alloc_console_reply *req )
{
    fprintf( stderr, " handle_in=%04x", req->handle_in );
    fprintf( stderr, ", event=%04x", req->event );
}

static void dump_free_console_request( const struct free_console_request *req )
{
}

static void dump_get_console_renderer_events_request( const struct get_console_renderer_events_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_console_renderer_events_reply( const struct get_console_renderer_events_reply *req )
{
    dump_varargs_bytes( " data=", cur_size );
}

static void dump_open_console_request( const struct open_console_request *req )
{
    fprintf( stderr, " from=%04x", req->from );
    fprintf( stderr, ", access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", share=%d", req->share );
}

static void dump_open_console_reply( const struct open_console_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_console_wait_event_request( const struct get_console_wait_event_request *req )
{
}

static void dump_get_console_wait_event_reply( const struct get_console_wait_event_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_console_mode_request( const struct get_console_mode_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_console_mode_reply( const struct get_console_mode_reply *req )
{
    fprintf( stderr, " mode=%d", req->mode );
}

static void dump_set_console_mode_request( const struct set_console_mode_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", mode=%d", req->mode );
}

static void dump_set_console_input_info_request( const struct set_console_input_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", mask=%d", req->mask );
    fprintf( stderr, ", active_sb=%04x", req->active_sb );
    fprintf( stderr, ", history_mode=%d", req->history_mode );
    fprintf( stderr, ", history_size=%d", req->history_size );
    fprintf( stderr, ", edition_mode=%d", req->edition_mode );
    fprintf( stderr, ", input_cp=%d", req->input_cp );
    fprintf( stderr, ", output_cp=%d", req->output_cp );
    fprintf( stderr, ", win=%08x", req->win );
    dump_varargs_unicode_str( ", title=", cur_size );
}

static void dump_get_console_input_info_request( const struct get_console_input_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_console_input_info_reply( const struct get_console_input_info_reply *req )
{
    fprintf( stderr, " history_mode=%d", req->history_mode );
    fprintf( stderr, ", history_size=%d", req->history_size );
    fprintf( stderr, ", history_index=%d", req->history_index );
    fprintf( stderr, ", edition_mode=%d", req->edition_mode );
    fprintf( stderr, ", input_cp=%d", req->input_cp );
    fprintf( stderr, ", output_cp=%d", req->output_cp );
    fprintf( stderr, ", win=%08x", req->win );
    dump_varargs_unicode_str( ", title=", cur_size );
}

static void dump_append_console_input_history_request( const struct append_console_input_history_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    dump_varargs_unicode_str( ", line=", cur_size );
}

static void dump_get_console_input_history_request( const struct get_console_input_history_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", index=%d", req->index );
}

static void dump_get_console_input_history_reply( const struct get_console_input_history_reply *req )
{
    fprintf( stderr, " total=%d", req->total );
    dump_varargs_unicode_str( ", line=", cur_size );
}

static void dump_create_console_output_request( const struct create_console_output_request *req )
{
    fprintf( stderr, " handle_in=%04x", req->handle_in );
    fprintf( stderr, ", access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", share=%08x", req->share );
    fprintf( stderr, ", fd=%d", req->fd );
}

static void dump_create_console_output_reply( const struct create_console_output_reply *req )
{
    fprintf( stderr, " handle_out=%04x", req->handle_out );
}

static void dump_set_console_output_info_request( const struct set_console_output_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", mask=%d", req->mask );
    fprintf( stderr, ", cursor_size=%d", req->cursor_size );
    fprintf( stderr, ", cursor_visible=%d", req->cursor_visible );
    fprintf( stderr, ", cursor_x=%d", req->cursor_x );
    fprintf( stderr, ", cursor_y=%d", req->cursor_y );
    fprintf( stderr, ", width=%d", req->width );
    fprintf( stderr, ", height=%d", req->height );
    fprintf( stderr, ", attr=%d", req->attr );
    fprintf( stderr, ", win_left=%d", req->win_left );
    fprintf( stderr, ", win_top=%d", req->win_top );
    fprintf( stderr, ", win_right=%d", req->win_right );
    fprintf( stderr, ", win_bottom=%d", req->win_bottom );
    fprintf( stderr, ", max_width=%d", req->max_width );
    fprintf( stderr, ", max_height=%d", req->max_height );
}

static void dump_get_console_output_info_request( const struct get_console_output_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_console_output_info_reply( const struct get_console_output_info_reply *req )
{
    fprintf( stderr, " cursor_size=%d", req->cursor_size );
    fprintf( stderr, ", cursor_visible=%d", req->cursor_visible );
    fprintf( stderr, ", cursor_x=%d", req->cursor_x );
    fprintf( stderr, ", cursor_y=%d", req->cursor_y );
    fprintf( stderr, ", width=%d", req->width );
    fprintf( stderr, ", height=%d", req->height );
    fprintf( stderr, ", attr=%d", req->attr );
    fprintf( stderr, ", win_left=%d", req->win_left );
    fprintf( stderr, ", win_top=%d", req->win_top );
    fprintf( stderr, ", win_right=%d", req->win_right );
    fprintf( stderr, ", win_bottom=%d", req->win_bottom );
    fprintf( stderr, ", max_width=%d", req->max_width );
    fprintf( stderr, ", max_height=%d", req->max_height );
}

static void dump_write_console_input_request( const struct write_console_input_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    dump_varargs_input_records( ", rec=", cur_size );
}

static void dump_write_console_input_reply( const struct write_console_input_reply *req )
{
    fprintf( stderr, " written=%d", req->written );
}

static void dump_read_console_input_request( const struct read_console_input_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", flush=%d", req->flush );
}

static void dump_read_console_input_reply( const struct read_console_input_reply *req )
{
    fprintf( stderr, " read=%d", req->read );
    dump_varargs_input_records( ", rec=", cur_size );
}

static void dump_write_console_output_request( const struct write_console_output_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", x=%d", req->x );
    fprintf( stderr, ", y=%d", req->y );
    fprintf( stderr, ", mode=%d", req->mode );
    fprintf( stderr, ", wrap=%d", req->wrap );
    dump_varargs_bytes( ", data=", cur_size );
}

static void dump_write_console_output_reply( const struct write_console_output_reply *req )
{
    fprintf( stderr, " written=%d", req->written );
    fprintf( stderr, ", width=%d", req->width );
    fprintf( stderr, ", height=%d", req->height );
}

static void dump_fill_console_output_request( const struct fill_console_output_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", x=%d", req->x );
    fprintf( stderr, ", y=%d", req->y );
    fprintf( stderr, ", mode=%d", req->mode );
    fprintf( stderr, ", count=%d", req->count );
    fprintf( stderr, ", wrap=%d", req->wrap );
    dump_char_info( ", data=", &req->data );
}

static void dump_fill_console_output_reply( const struct fill_console_output_reply *req )
{
    fprintf( stderr, " written=%d", req->written );
}

static void dump_read_console_output_request( const struct read_console_output_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", x=%d", req->x );
    fprintf( stderr, ", y=%d", req->y );
    fprintf( stderr, ", mode=%d", req->mode );
    fprintf( stderr, ", wrap=%d", req->wrap );
}

static void dump_read_console_output_reply( const struct read_console_output_reply *req )
{
    fprintf( stderr, " width=%d", req->width );
    fprintf( stderr, ", height=%d", req->height );
    dump_varargs_bytes( ", data=", cur_size );
}

static void dump_move_console_output_request( const struct move_console_output_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", x_src=%d", req->x_src );
    fprintf( stderr, ", y_src=%d", req->y_src );
    fprintf( stderr, ", x_dst=%d", req->x_dst );
    fprintf( stderr, ", y_dst=%d", req->y_dst );
    fprintf( stderr, ", w=%d", req->w );
    fprintf( stderr, ", h=%d", req->h );
}

static void dump_send_console_signal_request( const struct send_console_signal_request *req )
{
    fprintf( stderr, " signal=%d", req->signal );
    fprintf( stderr, ", group_id=%04x", req->group_id );
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
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", protect=%08x", req->protect );
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
    fprintf( stderr, ", protect=%d", req->protect );
    fprintf( stderr, ", header_size=%d", req->header_size );
    dump_uint64( ", base=", &req->base );
    fprintf( stderr, ", mapping=%04x", req->mapping );
    fprintf( stderr, ", shared_file=%04x", req->shared_file );
}

static void dump_get_mapping_committed_range_request( const struct get_mapping_committed_range_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    dump_uint64( ", offset=", &req->offset );
}

static void dump_get_mapping_committed_range_reply( const struct get_mapping_committed_range_reply *req )
{
    dump_uint64( " size=", &req->size );
    fprintf( stderr, ", committed=%d", req->committed );
}

static void dump_add_mapping_committed_range_request( const struct add_mapping_committed_range_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    dump_uint64( ", offset=", &req->offset );
    dump_uint64( ", size=", &req->size );
}

static void dump_create_snapshot_request( const struct create_snapshot_request *req )
{
    fprintf( stderr, " attributes=%08x", req->attributes );
    fprintf( stderr, ", flags=%08x", req->flags );
}

static void dump_create_snapshot_reply( const struct create_snapshot_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_next_process_request( const struct next_process_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", reset=%d", req->reset );
}

static void dump_next_process_reply( const struct next_process_reply *req )
{
    fprintf( stderr, " count=%d", req->count );
    fprintf( stderr, ", pid=%04x", req->pid );
    fprintf( stderr, ", ppid=%04x", req->ppid );
    fprintf( stderr, ", threads=%d", req->threads );
    fprintf( stderr, ", priority=%d", req->priority );
    fprintf( stderr, ", handles=%d", req->handles );
    fprintf( stderr, ", unix_pid=%d", req->unix_pid );
    dump_varargs_unicode_str( ", filename=", cur_size );
}

static void dump_next_thread_request( const struct next_thread_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", reset=%d", req->reset );
}

static void dump_next_thread_reply( const struct next_thread_reply *req )
{
    fprintf( stderr, " count=%d", req->count );
    fprintf( stderr, ", pid=%04x", req->pid );
    fprintf( stderr, ", tid=%04x", req->tid );
    fprintf( stderr, ", base_pri=%d", req->base_pri );
    fprintf( stderr, ", delta_pri=%d", req->delta_pri );
}

static void dump_wait_debug_event_request( const struct wait_debug_event_request *req )
{
    fprintf( stderr, " get_handle=%d", req->get_handle );
}

static void dump_wait_debug_event_reply( const struct wait_debug_event_reply *req )
{
    fprintf( stderr, " pid=%04x", req->pid );
    fprintf( stderr, ", tid=%04x", req->tid );
    fprintf( stderr, ", wait=%04x", req->wait );
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
    dump_varargs_context( ", context=", cur_size );
}

static void dump_queue_exception_event_reply( const struct queue_exception_event_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_exception_status_request( const struct get_exception_status_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_exception_status_reply( const struct get_exception_status_reply *req )
{
    dump_varargs_context( " context=", cur_size );
}

static void dump_continue_debug_event_request( const struct continue_debug_event_request *req )
{
    fprintf( stderr, " pid=%04x", req->pid );
    fprintf( stderr, ", tid=%04x", req->tid );
    fprintf( stderr, ", status=%d", req->status );
}

static void dump_debug_process_request( const struct debug_process_request *req )
{
    fprintf( stderr, " pid=%04x", req->pid );
    fprintf( stderr, ", attach=%d", req->attach );
}

static void dump_debug_break_request( const struct debug_break_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_debug_break_reply( const struct debug_break_reply *req )
{
    fprintf( stderr, " self=%d", req->self );
}

static void dump_set_debugger_kill_on_exit_request( const struct set_debugger_kill_on_exit_request *req )
{
    fprintf( stderr, " kill_on_exit=%d", req->kill_on_exit );
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
    fprintf( stderr, " parent=%04x", req->parent );
    fprintf( stderr, ", access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", options=%08x", req->options );
    fprintf( stderr, ", namelen=%u", req->namelen );
    dump_varargs_unicode_str( ", name=", min(cur_size,req->namelen) );
    dump_varargs_unicode_str( ", class=", cur_size );
}

static void dump_create_key_reply( const struct create_key_reply *req )
{
    fprintf( stderr, " hkey=%04x", req->hkey );
    fprintf( stderr, ", created=%d", req->created );
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
    fprintf( stderr, " hkey=%04x", req->hkey );
    fprintf( stderr, ", file=%04x", req->file );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_unload_registry_request( const struct unload_registry_request *req )
{
    fprintf( stderr, " hkey=%04x", req->hkey );
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

static void dump_create_timer_request( const struct create_timer_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", rootdir=%04x", req->rootdir );
    fprintf( stderr, ", manual=%d", req->manual );
    dump_varargs_unicode_str( ", name=", cur_size );
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
    fprintf( stderr, ", flags=%08x", req->flags );
    fprintf( stderr, ", suspend=%d", req->suspend );
}

static void dump_get_thread_context_reply( const struct get_thread_context_reply *req )
{
    fprintf( stderr, " self=%d", req->self );
    dump_varargs_context( ", context=", cur_size );
}

static void dump_set_thread_context_request( const struct set_thread_context_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", suspend=%d", req->suspend );
    dump_varargs_context( ", context=", cur_size );
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
    fprintf( stderr, " table=%04x", req->table );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_add_atom_reply( const struct add_atom_reply *req )
{
    fprintf( stderr, " atom=%04x", req->atom );
}

static void dump_delete_atom_request( const struct delete_atom_request *req )
{
    fprintf( stderr, " table=%04x", req->table );
    fprintf( stderr, ", atom=%04x", req->atom );
}

static void dump_find_atom_request( const struct find_atom_request *req )
{
    fprintf( stderr, " table=%04x", req->table );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_find_atom_reply( const struct find_atom_reply *req )
{
    fprintf( stderr, " atom=%04x", req->atom );
}

static void dump_get_atom_information_request( const struct get_atom_information_request *req )
{
    fprintf( stderr, " table=%04x", req->table );
    fprintf( stderr, ", atom=%04x", req->atom );
}

static void dump_get_atom_information_reply( const struct get_atom_information_reply *req )
{
    fprintf( stderr, " count=%d", req->count );
    fprintf( stderr, ", pinned=%d", req->pinned );
    fprintf( stderr, ", total=%u", req->total );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_set_atom_information_request( const struct set_atom_information_request *req )
{
    fprintf( stderr, " table=%04x", req->table );
    fprintf( stderr, ", atom=%04x", req->atom );
    fprintf( stderr, ", pinned=%d", req->pinned );
}

static void dump_empty_atom_table_request( const struct empty_atom_table_request *req )
{
    fprintf( stderr, " table=%04x", req->table );
    fprintf( stderr, ", if_pinned=%d", req->if_pinned );
}

static void dump_init_atom_table_request( const struct init_atom_table_request *req )
{
    fprintf( stderr, " entries=%d", req->entries );
}

static void dump_init_atom_table_reply( const struct init_atom_table_reply *req )
{
    fprintf( stderr, " table=%04x", req->table );
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
    fprintf( stderr, " clear=%d", req->clear );
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
}

static void dump_send_hardware_message_reply( const struct send_hardware_message_reply *req )
{
    fprintf( stderr, " wait=%d", req->wait );
    fprintf( stderr, ", prev_x=%d", req->prev_x );
    fprintf( stderr, ", prev_y=%d", req->prev_y );
    fprintf( stderr, ", new_x=%d", req->new_x );
    fprintf( stderr, ", new_y=%d", req->new_y );
    dump_varargs_bytes( ", keystate=", cur_size );
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
    fprintf( stderr, ", remove=%d", req->remove );
    fprintf( stderr, ", new_win=%08x", req->new_win );
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
    fprintf( stderr, " readinterval=%08x", req->readinterval );
    fprintf( stderr, ", readconst=%08x", req->readconst );
    fprintf( stderr, ", readmult=%08x", req->readmult );
    fprintf( stderr, ", writeconst=%08x", req->writeconst );
    fprintf( stderr, ", writemult=%08x", req->writemult );
    fprintf( stderr, ", eventmask=%08x", req->eventmask );
    fprintf( stderr, ", cookie=%08x", req->cookie );
    fprintf( stderr, ", pending_write=%08x", req->pending_write );
}

static void dump_set_serial_info_request( const struct set_serial_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", flags=%d", req->flags );
    fprintf( stderr, ", readinterval=%08x", req->readinterval );
    fprintf( stderr, ", readconst=%08x", req->readconst );
    fprintf( stderr, ", readmult=%08x", req->readmult );
    fprintf( stderr, ", writeconst=%08x", req->writeconst );
    fprintf( stderr, ", writemult=%08x", req->writemult );
    fprintf( stderr, ", eventmask=%08x", req->eventmask );
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

static void dump_ioctl_request( const struct ioctl_request *req )
{
    dump_ioctl_code( " code=", &req->code );
    dump_async_data( ", async=", &req->async );
    fprintf( stderr, ", blocking=%d", req->blocking );
    dump_varargs_bytes( ", in_data=", cur_size );
}

static void dump_ioctl_reply( const struct ioctl_reply *req )
{
    fprintf( stderr, " wait=%04x", req->wait );
    fprintf( stderr, ", options=%08x", req->options );
    dump_varargs_bytes( ", out_data=", cur_size );
}

static void dump_get_ioctl_result_request( const struct get_ioctl_result_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    dump_uint64( ", user_arg=", &req->user_arg );
}

static void dump_get_ioctl_result_reply( const struct get_ioctl_result_reply *req )
{
    dump_varargs_bytes( " out_data=", cur_size );
}

static void dump_create_named_pipe_request( const struct create_named_pipe_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", rootdir=%04x", req->rootdir );
    fprintf( stderr, ", options=%08x", req->options );
    fprintf( stderr, ", sharing=%08x", req->sharing );
    fprintf( stderr, ", maxinstances=%08x", req->maxinstances );
    fprintf( stderr, ", outsize=%08x", req->outsize );
    fprintf( stderr, ", insize=%08x", req->insize );
    dump_timeout( ", timeout=", &req->timeout );
    fprintf( stderr, ", flags=%08x", req->flags );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_create_named_pipe_reply( const struct create_named_pipe_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_named_pipe_info_request( const struct get_named_pipe_info_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_named_pipe_info_reply( const struct get_named_pipe_info_reply *req )
{
    fprintf( stderr, " flags=%08x", req->flags );
    fprintf( stderr, ", sharing=%08x", req->sharing );
    fprintf( stderr, ", maxinstances=%08x", req->maxinstances );
    fprintf( stderr, ", instances=%08x", req->instances );
    fprintf( stderr, ", outsize=%08x", req->outsize );
    fprintf( stderr, ", insize=%08x", req->insize );
}

static void dump_create_window_request( const struct create_window_request *req )
{
    fprintf( stderr, " parent=%08x", req->parent );
    fprintf( stderr, ", owner=%08x", req->owner );
    fprintf( stderr, ", atom=%04x", req->atom );
    dump_uint64( ", instance=", &req->instance );
    dump_varargs_unicode_str( ", class=", cur_size );
}

static void dump_create_window_reply( const struct create_window_reply *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
    fprintf( stderr, ", parent=%08x", req->parent );
    fprintf( stderr, ", owner=%08x", req->owner );
    fprintf( stderr, ", extra=%d", req->extra );
    dump_uint64( ", class_ptr=", &req->class_ptr );
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
}

static void dump_set_window_info_request( const struct set_window_info_request *req )
{
    fprintf( stderr, " flags=%04x", req->flags );
    fprintf( stderr, ", is_unicode=%d", req->is_unicode );
    fprintf( stderr, ", handle=%08x", req->handle );
    fprintf( stderr, ", style=%08x", req->style );
    fprintf( stderr, ", ex_style=%08x", req->ex_style );
    fprintf( stderr, ", id=%08x", req->id );
    dump_uint64( ", instance=", &req->instance );
    dump_uint64( ", user_data=", &req->user_data );
    fprintf( stderr, ", extra_offset=%d", req->extra_offset );
    fprintf( stderr, ", extra_size=%u", req->extra_size );
    dump_uint64( ", extra_value=", &req->extra_value );
}

static void dump_set_window_info_reply( const struct set_window_info_reply *req )
{
    fprintf( stderr, " old_style=%08x", req->old_style );
    fprintf( stderr, ", old_ex_style=%08x", req->old_ex_style );
    dump_uint64( ", old_instance=", &req->old_instance );
    dump_uint64( ", old_user_data=", &req->old_user_data );
    dump_uint64( ", old_extra_value=", &req->old_extra_value );
    fprintf( stderr, ", old_id=%08x", req->old_id );
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
}

static void dump_get_window_rectangles_request( const struct get_window_rectangles_request *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
    fprintf( stderr, ", relative=%d", req->relative );
}

static void dump_get_window_rectangles_reply( const struct get_window_rectangles_reply *req )
{
    dump_rectangle( " window=", &req->window );
    dump_rectangle( ", visible=", &req->visible );
    dump_rectangle( ", client=", &req->client );
}

static void dump_get_window_text_request( const struct get_window_text_request *req )
{
    fprintf( stderr, " handle=%08x", req->handle );
}

static void dump_get_window_text_reply( const struct get_window_text_reply *req )
{
    dump_varargs_unicode_str( " text=", cur_size );
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
    fprintf( stderr, " tid=%04x", req->tid );
    fprintf( stderr, ", key=%d", req->key );
}

static void dump_get_key_state_reply( const struct get_key_state_reply *req )
{
    fprintf( stderr, " state=%02x", req->state );
    dump_varargs_bytes( ", keystate=", cur_size );
}

static void dump_set_key_state_request( const struct set_key_state_request *req )
{
    fprintf( stderr, " tid=%04x", req->tid );
    fprintf( stderr, ", async=%d", req->async );
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
    fprintf( stderr, ", old_style=%08x", req->old_style );
    fprintf( stderr, ", old_extra=%d", req->old_extra );
    fprintf( stderr, ", old_win_extra=%d", req->old_win_extra );
    dump_uint64( ", old_instance=", &req->old_instance );
    dump_uint64( ", old_extra_value=", &req->old_extra_value );
}

static void dump_set_clipboard_info_request( const struct set_clipboard_info_request *req )
{
    fprintf( stderr, " flags=%08x", req->flags );
    fprintf( stderr, ", clipboard=%08x", req->clipboard );
    fprintf( stderr, ", owner=%08x", req->owner );
    fprintf( stderr, ", viewer=%08x", req->viewer );
    fprintf( stderr, ", seqno=%08x", req->seqno );
}

static void dump_set_clipboard_info_reply( const struct set_clipboard_info_reply *req )
{
    fprintf( stderr, " flags=%08x", req->flags );
    fprintf( stderr, ", old_clipboard=%08x", req->old_clipboard );
    fprintf( stderr, ", old_owner=%08x", req->old_owner );
    fprintf( stderr, ", old_viewer=%08x", req->old_viewer );
    fprintf( stderr, ", seqno=%08x", req->seqno );
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
    dump_varargs_LUID_AND_ATTRIBUTES( ", privileges=", cur_size );
}

static void dump_adjust_token_privileges_reply( const struct adjust_token_privileges_reply *req )
{
    fprintf( stderr, " len=%08x", req->len );
    dump_varargs_LUID_AND_ATTRIBUTES( ", privileges=", cur_size );
}

static void dump_get_token_privileges_request( const struct get_token_privileges_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_token_privileges_reply( const struct get_token_privileges_reply *req )
{
    fprintf( stderr, " len=%08x", req->len );
    dump_varargs_LUID_AND_ATTRIBUTES( ", privileges=", cur_size );
}

static void dump_check_token_privileges_request( const struct check_token_privileges_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", all_required=%d", req->all_required );
    dump_varargs_LUID_AND_ATTRIBUTES( ", privileges=", cur_size );
}

static void dump_check_token_privileges_reply( const struct check_token_privileges_reply *req )
{
    fprintf( stderr, " has_privileges=%d", req->has_privileges );
    dump_varargs_LUID_AND_ATTRIBUTES( ", privileges=", cur_size );
}

static void dump_duplicate_token_request( const struct duplicate_token_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", primary=%d", req->primary );
    fprintf( stderr, ", impersonation_level=%d", req->impersonation_level );
}

static void dump_duplicate_token_reply( const struct duplicate_token_reply *req )
{
    fprintf( stderr, " new_handle=%04x", req->new_handle );
}

static void dump_access_check_request( const struct access_check_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", desired_access=%08x", req->desired_access );
    fprintf( stderr, ", mapping_read=%08x", req->mapping_read );
    fprintf( stderr, ", mapping_write=%08x", req->mapping_write );
    fprintf( stderr, ", mapping_execute=%08x", req->mapping_execute );
    fprintf( stderr, ", mapping_all=%08x", req->mapping_all );
    dump_varargs_security_descriptor( ", sd=", cur_size );
}

static void dump_access_check_reply( const struct access_check_reply *req )
{
    fprintf( stderr, " access_granted=%08x", req->access_granted );
    fprintf( stderr, ", access_status=%08x", req->access_status );
    fprintf( stderr, ", privileges_len=%08x", req->privileges_len );
    dump_varargs_LUID_AND_ATTRIBUTES( ", privileges=", cur_size );
}

static void dump_get_token_sid_request( const struct get_token_sid_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    fprintf( stderr, ", which_sid=%08x", req->which_sid );
}

static void dump_get_token_sid_reply( const struct get_token_sid_reply *req )
{
    fprintf( stderr, " sid_len=%u", req->sid_len );
    dump_varargs_SID( ", sid=", cur_size );
}

static void dump_get_token_groups_request( const struct get_token_groups_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_token_groups_reply( const struct get_token_groups_reply *req )
{
    fprintf( stderr, " user_len=%u", req->user_len );
    dump_varargs_token_groups( ", user=", cur_size );
}

static void dump_get_token_default_dacl_request( const struct get_token_default_dacl_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_token_default_dacl_reply( const struct get_token_default_dacl_reply *req )
{
    fprintf( stderr, " acl_len=%u", req->acl_len );
    dump_varargs_ACL( ", acl=", cur_size );
}

static void dump_set_token_default_dacl_request( const struct set_token_default_dacl_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
    dump_varargs_ACL( ", acl=", cur_size );
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

static void dump_create_mailslot_request( const struct create_mailslot_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", rootdir=%04x", req->rootdir );
    dump_timeout( ", read_timeout=", &req->read_timeout );
    fprintf( stderr, ", max_msgsize=%08x", req->max_msgsize );
    dump_varargs_unicode_str( ", name=", cur_size );
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
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", rootdir=%04x", req->rootdir );
    dump_varargs_unicode_str( ", directory_name=", cur_size );
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
    fprintf( stderr, " name_len=%u", req->name_len );
    dump_varargs_unicode_str( ", name=", min(cur_size,req->name_len) );
    dump_varargs_unicode_str( ", type=", cur_size );
}

static void dump_create_symlink_request( const struct create_symlink_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", rootdir=%04x", req->rootdir );
    fprintf( stderr, ", name_len=%u", req->name_len );
    dump_varargs_unicode_str( ", name=", min(cur_size,req->name_len) );
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
    fprintf( stderr, ", total=%u", req->total );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_unlink_object_request( const struct unlink_object_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_token_impersonation_level_request( const struct get_token_impersonation_level_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_token_impersonation_level_reply( const struct get_token_impersonation_level_reply *req )
{
    fprintf( stderr, " impersonation_level=%d", req->impersonation_level );
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
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", rootdir=%04x", req->rootdir );
    dump_uint64( ", user_ptr=", &req->user_ptr );
    fprintf( stderr, ", manager=%04x", req->manager );
    dump_varargs_unicode_str( ", name=", cur_size );
}

static void dump_create_device_reply( const struct create_device_reply *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_delete_device_request( const struct delete_device_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_next_device_request_request( const struct get_next_device_request_request *req )
{
    fprintf( stderr, " manager=%04x", req->manager );
    fprintf( stderr, ", prev=%04x", req->prev );
    fprintf( stderr, ", status=%08x", req->status );
    dump_varargs_bytes( ", prev_data=", cur_size );
}

static void dump_get_next_device_request_reply( const struct get_next_device_request_reply *req )
{
    fprintf( stderr, " next=%04x", req->next );
    dump_ioctl_code( ", code=", &req->code );
    dump_uint64( ", user_ptr=", &req->user_ptr );
    fprintf( stderr, ", client_pid=%04x", req->client_pid );
    fprintf( stderr, ", client_tid=%04x", req->client_tid );
    fprintf( stderr, ", in_size=%u", req->in_size );
    fprintf( stderr, ", out_size=%u", req->out_size );
    dump_varargs_bytes( ", next_data=", cur_size );
}

static void dump_make_process_system_request( const struct make_process_system_request *req )
{
}

static void dump_make_process_system_reply( const struct make_process_system_reply *req )
{
    fprintf( stderr, " event=%04x", req->event );
}

static void dump_get_token_statistics_request( const struct get_token_statistics_request *req )
{
    fprintf( stderr, " handle=%04x", req->handle );
}

static void dump_get_token_statistics_reply( const struct get_token_statistics_reply *req )
{
    dump_luid( " token_id=", &req->token_id );
    dump_luid( ", modified_id=", &req->modified_id );
    fprintf( stderr, ", primary=%d", req->primary );
    fprintf( stderr, ", impersonation_level=%d", req->impersonation_level );
    fprintf( stderr, ", group_count=%d", req->group_count );
    fprintf( stderr, ", privilege_count=%d", req->privilege_count );
}

static void dump_create_completion_request( const struct create_completion_request *req )
{
    fprintf( stderr, " access=%08x", req->access );
    fprintf( stderr, ", attributes=%08x", req->attributes );
    fprintf( stderr, ", concurrent=%08x", req->concurrent );
    fprintf( stderr, ", rootdir=%04x", req->rootdir );
    dump_varargs_string( ", filename=", cur_size );
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
    dump_varargs_string( ", filename=", cur_size );
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
    fprintf( stderr, ", clip_msg=%08x", req->clip_msg );
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

static void dump_update_rawinput_devices_request( const struct update_rawinput_devices_request *req )
{
    dump_varargs_rawinput_devices( " devices=", cur_size );
}

static void dump_get_suspend_context_request( const struct get_suspend_context_request *req )
{
}

static void dump_get_suspend_context_reply( const struct get_suspend_context_reply *req )
{
    dump_varargs_context( " context=", cur_size );
}

static void dump_set_suspend_context_request( const struct set_suspend_context_request *req )
{
    dump_varargs_context( " context=", cur_size );
}

static const dump_func req_dumpers[REQ_NB_REQUESTS] = {
    (dump_func)dump_new_process_request,
    (dump_func)dump_get_new_process_info_request,
    (dump_func)dump_new_thread_request,
    (dump_func)dump_get_startup_info_request,
    (dump_func)dump_init_process_done_request,
    (dump_func)dump_init_thread_request,
    (dump_func)dump_terminate_process_request,
    (dump_func)dump_terminate_thread_request,
    (dump_func)dump_get_process_info_request,
    (dump_func)dump_set_process_info_request,
    (dump_func)dump_get_thread_info_request,
    (dump_func)dump_set_thread_info_request,
    (dump_func)dump_get_dll_info_request,
    (dump_func)dump_suspend_thread_request,
    (dump_func)dump_resume_thread_request,
    (dump_func)dump_load_dll_request,
    (dump_func)dump_unload_dll_request,
    (dump_func)dump_queue_apc_request,
    (dump_func)dump_get_apc_result_request,
    (dump_func)dump_close_handle_request,
    (dump_func)dump_set_handle_info_request,
    (dump_func)dump_dup_handle_request,
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
    (dump_func)dump_create_semaphore_request,
    (dump_func)dump_release_semaphore_request,
    (dump_func)dump_query_semaphore_request,
    (dump_func)dump_open_semaphore_request,
    (dump_func)dump_create_file_request,
    (dump_func)dump_open_file_object_request,
    (dump_func)dump_alloc_file_handle_request,
    (dump_func)dump_get_handle_unix_name_request,
    (dump_func)dump_get_handle_fd_request,
    (dump_func)dump_flush_file_request,
    (dump_func)dump_lock_file_request,
    (dump_func)dump_unlock_file_request,
    (dump_func)dump_create_socket_request,
    (dump_func)dump_accept_socket_request,
    (dump_func)dump_accept_into_socket_request,
    (dump_func)dump_set_socket_event_request,
    (dump_func)dump_get_socket_event_request,
    (dump_func)dump_get_socket_info_request,
    (dump_func)dump_enable_socket_event_request,
    (dump_func)dump_set_socket_deferred_request,
    (dump_func)dump_alloc_console_request,
    (dump_func)dump_free_console_request,
    (dump_func)dump_get_console_renderer_events_request,
    (dump_func)dump_open_console_request,
    (dump_func)dump_get_console_wait_event_request,
    (dump_func)dump_get_console_mode_request,
    (dump_func)dump_set_console_mode_request,
    (dump_func)dump_set_console_input_info_request,
    (dump_func)dump_get_console_input_info_request,
    (dump_func)dump_append_console_input_history_request,
    (dump_func)dump_get_console_input_history_request,
    (dump_func)dump_create_console_output_request,
    (dump_func)dump_set_console_output_info_request,
    (dump_func)dump_get_console_output_info_request,
    (dump_func)dump_write_console_input_request,
    (dump_func)dump_read_console_input_request,
    (dump_func)dump_write_console_output_request,
    (dump_func)dump_fill_console_output_request,
    (dump_func)dump_read_console_output_request,
    (dump_func)dump_move_console_output_request,
    (dump_func)dump_send_console_signal_request,
    (dump_func)dump_read_directory_changes_request,
    (dump_func)dump_read_change_request,
    (dump_func)dump_create_mapping_request,
    (dump_func)dump_open_mapping_request,
    (dump_func)dump_get_mapping_info_request,
    (dump_func)dump_get_mapping_committed_range_request,
    (dump_func)dump_add_mapping_committed_range_request,
    (dump_func)dump_create_snapshot_request,
    (dump_func)dump_next_process_request,
    (dump_func)dump_next_thread_request,
    (dump_func)dump_wait_debug_event_request,
    (dump_func)dump_queue_exception_event_request,
    (dump_func)dump_get_exception_status_request,
    (dump_func)dump_continue_debug_event_request,
    (dump_func)dump_debug_process_request,
    (dump_func)dump_debug_break_request,
    (dump_func)dump_set_debugger_kill_on_exit_request,
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
    (dump_func)dump_set_atom_information_request,
    (dump_func)dump_empty_atom_table_request,
    (dump_func)dump_init_atom_table_request,
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
    (dump_func)dump_register_async_request,
    (dump_func)dump_cancel_async_request,
    (dump_func)dump_ioctl_request,
    (dump_func)dump_get_ioctl_result_request,
    (dump_func)dump_create_named_pipe_request,
    (dump_func)dump_get_named_pipe_info_request,
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
    (dump_func)dump_set_clipboard_info_request,
    (dump_func)dump_open_token_request,
    (dump_func)dump_set_global_windows_request,
    (dump_func)dump_adjust_token_privileges_request,
    (dump_func)dump_get_token_privileges_request,
    (dump_func)dump_check_token_privileges_request,
    (dump_func)dump_duplicate_token_request,
    (dump_func)dump_access_check_request,
    (dump_func)dump_get_token_sid_request,
    (dump_func)dump_get_token_groups_request,
    (dump_func)dump_get_token_default_dacl_request,
    (dump_func)dump_set_token_default_dacl_request,
    (dump_func)dump_set_security_object_request,
    (dump_func)dump_get_security_object_request,
    (dump_func)dump_create_mailslot_request,
    (dump_func)dump_set_mailslot_info_request,
    (dump_func)dump_create_directory_request,
    (dump_func)dump_open_directory_request,
    (dump_func)dump_get_directory_entry_request,
    (dump_func)dump_create_symlink_request,
    (dump_func)dump_open_symlink_request,
    (dump_func)dump_query_symlink_request,
    (dump_func)dump_get_object_info_request,
    (dump_func)dump_unlink_object_request,
    (dump_func)dump_get_token_impersonation_level_request,
    (dump_func)dump_allocate_locally_unique_id_request,
    (dump_func)dump_create_device_manager_request,
    (dump_func)dump_create_device_request,
    (dump_func)dump_delete_device_request,
    (dump_func)dump_get_next_device_request_request,
    (dump_func)dump_make_process_system_request,
    (dump_func)dump_get_token_statistics_request,
    (dump_func)dump_create_completion_request,
    (dump_func)dump_open_completion_request,
    (dump_func)dump_add_completion_request,
    (dump_func)dump_remove_completion_request,
    (dump_func)dump_query_completion_request,
    (dump_func)dump_set_completion_info_request,
    (dump_func)dump_add_fd_completion_request,
    (dump_func)dump_get_window_layered_info_request,
    (dump_func)dump_set_window_layered_info_request,
    (dump_func)dump_alloc_user_handle_request,
    (dump_func)dump_free_user_handle_request,
    (dump_func)dump_set_cursor_request,
    (dump_func)dump_update_rawinput_devices_request,
    (dump_func)dump_get_suspend_context_request,
    (dump_func)dump_set_suspend_context_request,
};

static const dump_func reply_dumpers[REQ_NB_REQUESTS] = {
    (dump_func)dump_new_process_reply,
    (dump_func)dump_get_new_process_info_reply,
    (dump_func)dump_new_thread_reply,
    (dump_func)dump_get_startup_info_reply,
    NULL,
    (dump_func)dump_init_thread_reply,
    (dump_func)dump_terminate_process_reply,
    (dump_func)dump_terminate_thread_reply,
    (dump_func)dump_get_process_info_reply,
    NULL,
    (dump_func)dump_get_thread_info_reply,
    NULL,
    (dump_func)dump_get_dll_info_reply,
    (dump_func)dump_suspend_thread_reply,
    (dump_func)dump_resume_thread_reply,
    NULL,
    NULL,
    (dump_func)dump_queue_apc_reply,
    (dump_func)dump_get_apc_result_reply,
    NULL,
    (dump_func)dump_set_handle_info_reply,
    (dump_func)dump_dup_handle_reply,
    (dump_func)dump_open_process_reply,
    (dump_func)dump_open_thread_reply,
    (dump_func)dump_select_reply,
    (dump_func)dump_create_event_reply,
    NULL,
    (dump_func)dump_query_event_reply,
    (dump_func)dump_open_event_reply,
    (dump_func)dump_create_keyed_event_reply,
    (dump_func)dump_open_keyed_event_reply,
    (dump_func)dump_create_mutex_reply,
    (dump_func)dump_release_mutex_reply,
    (dump_func)dump_open_mutex_reply,
    (dump_func)dump_create_semaphore_reply,
    (dump_func)dump_release_semaphore_reply,
    (dump_func)dump_query_semaphore_reply,
    (dump_func)dump_open_semaphore_reply,
    (dump_func)dump_create_file_reply,
    (dump_func)dump_open_file_object_reply,
    (dump_func)dump_alloc_file_handle_reply,
    (dump_func)dump_get_handle_unix_name_reply,
    (dump_func)dump_get_handle_fd_reply,
    (dump_func)dump_flush_file_reply,
    (dump_func)dump_lock_file_reply,
    NULL,
    (dump_func)dump_create_socket_reply,
    (dump_func)dump_accept_socket_reply,
    NULL,
    NULL,
    (dump_func)dump_get_socket_event_reply,
    (dump_func)dump_get_socket_info_reply,
    NULL,
    NULL,
    (dump_func)dump_alloc_console_reply,
    NULL,
    (dump_func)dump_get_console_renderer_events_reply,
    (dump_func)dump_open_console_reply,
    (dump_func)dump_get_console_wait_event_reply,
    (dump_func)dump_get_console_mode_reply,
    NULL,
    NULL,
    (dump_func)dump_get_console_input_info_reply,
    NULL,
    (dump_func)dump_get_console_input_history_reply,
    (dump_func)dump_create_console_output_reply,
    NULL,
    (dump_func)dump_get_console_output_info_reply,
    (dump_func)dump_write_console_input_reply,
    (dump_func)dump_read_console_input_reply,
    (dump_func)dump_write_console_output_reply,
    (dump_func)dump_fill_console_output_reply,
    (dump_func)dump_read_console_output_reply,
    NULL,
    NULL,
    NULL,
    (dump_func)dump_read_change_reply,
    (dump_func)dump_create_mapping_reply,
    (dump_func)dump_open_mapping_reply,
    (dump_func)dump_get_mapping_info_reply,
    (dump_func)dump_get_mapping_committed_range_reply,
    NULL,
    (dump_func)dump_create_snapshot_reply,
    (dump_func)dump_next_process_reply,
    (dump_func)dump_next_thread_reply,
    (dump_func)dump_wait_debug_event_reply,
    (dump_func)dump_queue_exception_event_reply,
    (dump_func)dump_get_exception_status_reply,
    NULL,
    NULL,
    (dump_func)dump_debug_break_reply,
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
    NULL,
    NULL,
    (dump_func)dump_init_atom_table_reply,
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
    (dump_func)dump_ioctl_reply,
    (dump_func)dump_get_ioctl_result_reply,
    (dump_func)dump_create_named_pipe_reply,
    (dump_func)dump_get_named_pipe_info_reply,
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
    (dump_func)dump_set_clipboard_info_reply,
    (dump_func)dump_open_token_reply,
    (dump_func)dump_set_global_windows_reply,
    (dump_func)dump_adjust_token_privileges_reply,
    (dump_func)dump_get_token_privileges_reply,
    (dump_func)dump_check_token_privileges_reply,
    (dump_func)dump_duplicate_token_reply,
    (dump_func)dump_access_check_reply,
    (dump_func)dump_get_token_sid_reply,
    (dump_func)dump_get_token_groups_reply,
    (dump_func)dump_get_token_default_dacl_reply,
    NULL,
    NULL,
    (dump_func)dump_get_security_object_reply,
    (dump_func)dump_create_mailslot_reply,
    (dump_func)dump_set_mailslot_info_reply,
    (dump_func)dump_create_directory_reply,
    (dump_func)dump_open_directory_reply,
    (dump_func)dump_get_directory_entry_reply,
    (dump_func)dump_create_symlink_reply,
    (dump_func)dump_open_symlink_reply,
    (dump_func)dump_query_symlink_reply,
    (dump_func)dump_get_object_info_reply,
    NULL,
    (dump_func)dump_get_token_impersonation_level_reply,
    (dump_func)dump_allocate_locally_unique_id_reply,
    (dump_func)dump_create_device_manager_reply,
    (dump_func)dump_create_device_reply,
    NULL,
    (dump_func)dump_get_next_device_request_reply,
    (dump_func)dump_make_process_system_reply,
    (dump_func)dump_get_token_statistics_reply,
    (dump_func)dump_create_completion_reply,
    (dump_func)dump_open_completion_reply,
    NULL,
    (dump_func)dump_remove_completion_reply,
    (dump_func)dump_query_completion_reply,
    NULL,
    NULL,
    (dump_func)dump_get_window_layered_info_reply,
    NULL,
    (dump_func)dump_alloc_user_handle_reply,
    NULL,
    (dump_func)dump_set_cursor_reply,
    NULL,
    (dump_func)dump_get_suspend_context_reply,
    NULL,
};

static const char * const req_names[REQ_NB_REQUESTS] = {
    "new_process",
    "get_new_process_info",
    "new_thread",
    "get_startup_info",
    "init_process_done",
    "init_thread",
    "terminate_process",
    "terminate_thread",
    "get_process_info",
    "set_process_info",
    "get_thread_info",
    "set_thread_info",
    "get_dll_info",
    "suspend_thread",
    "resume_thread",
    "load_dll",
    "unload_dll",
    "queue_apc",
    "get_apc_result",
    "close_handle",
    "set_handle_info",
    "dup_handle",
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
    "create_semaphore",
    "release_semaphore",
    "query_semaphore",
    "open_semaphore",
    "create_file",
    "open_file_object",
    "alloc_file_handle",
    "get_handle_unix_name",
    "get_handle_fd",
    "flush_file",
    "lock_file",
    "unlock_file",
    "create_socket",
    "accept_socket",
    "accept_into_socket",
    "set_socket_event",
    "get_socket_event",
    "get_socket_info",
    "enable_socket_event",
    "set_socket_deferred",
    "alloc_console",
    "free_console",
    "get_console_renderer_events",
    "open_console",
    "get_console_wait_event",
    "get_console_mode",
    "set_console_mode",
    "set_console_input_info",
    "get_console_input_info",
    "append_console_input_history",
    "get_console_input_history",
    "create_console_output",
    "set_console_output_info",
    "get_console_output_info",
    "write_console_input",
    "read_console_input",
    "write_console_output",
    "fill_console_output",
    "read_console_output",
    "move_console_output",
    "send_console_signal",
    "read_directory_changes",
    "read_change",
    "create_mapping",
    "open_mapping",
    "get_mapping_info",
    "get_mapping_committed_range",
    "add_mapping_committed_range",
    "create_snapshot",
    "next_process",
    "next_thread",
    "wait_debug_event",
    "queue_exception_event",
    "get_exception_status",
    "continue_debug_event",
    "debug_process",
    "debug_break",
    "set_debugger_kill_on_exit",
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
    "set_atom_information",
    "empty_atom_table",
    "init_atom_table",
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
    "register_async",
    "cancel_async",
    "ioctl",
    "get_ioctl_result",
    "create_named_pipe",
    "get_named_pipe_info",
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
    "set_clipboard_info",
    "open_token",
    "set_global_windows",
    "adjust_token_privileges",
    "get_token_privileges",
    "check_token_privileges",
    "duplicate_token",
    "access_check",
    "get_token_sid",
    "get_token_groups",
    "get_token_default_dacl",
    "set_token_default_dacl",
    "set_security_object",
    "get_security_object",
    "create_mailslot",
    "set_mailslot_info",
    "create_directory",
    "open_directory",
    "get_directory_entry",
    "create_symlink",
    "open_symlink",
    "query_symlink",
    "get_object_info",
    "unlink_object",
    "get_token_impersonation_level",
    "allocate_locally_unique_id",
    "create_device_manager",
    "create_device",
    "delete_device",
    "get_next_device_request",
    "make_process_system",
    "get_token_statistics",
    "create_completion",
    "open_completion",
    "add_completion",
    "remove_completion",
    "query_completion",
    "set_completion_info",
    "add_fd_completion",
    "get_window_layered_info",
    "set_window_layered_info",
    "alloc_user_handle",
    "free_user_handle",
    "set_cursor",
    "update_rawinput_devices",
    "get_suspend_context",
    "set_suspend_context",
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
    { "ALIAS_EXISTS",                STATUS_ALIAS_EXISTS },
    { "BAD_DEVICE_TYPE",             STATUS_BAD_DEVICE_TYPE },
    { "BAD_IMPERSONATION_LEVEL",     STATUS_BAD_IMPERSONATION_LEVEL },
    { "BREAKPOINT",                  STATUS_BREAKPOINT },
    { "BUFFER_OVERFLOW",             STATUS_BUFFER_OVERFLOW },
    { "BUFFER_TOO_SMALL",            STATUS_BUFFER_TOO_SMALL },
    { "CANCELLED",                   STATUS_CANCELLED },
    { "CANNOT_DELETE",               STATUS_CANNOT_DELETE },
    { "CANT_OPEN_ANONYMOUS",         STATUS_CANT_OPEN_ANONYMOUS },
    { "CANT_WAIT",                   STATUS_CANT_WAIT },
    { "CHILD_MUST_BE_VOLATILE",      STATUS_CHILD_MUST_BE_VOLATILE },
    { "CONNECTION_ABORTED",          STATUS_CONNECTION_ABORTED },
    { "CONNECTION_DISCONNECTED",     STATUS_CONNECTION_DISCONNECTED },
    { "CONNECTION_REFUSED",          STATUS_CONNECTION_REFUSED },
    { "CONNECTION_RESET",            STATUS_CONNECTION_RESET },
    { "DEBUGGER_INACTIVE",           STATUS_DEBUGGER_INACTIVE },
    { "DEVICE_BUSY",                 STATUS_DEVICE_BUSY },
    { "DIRECTORY_NOT_EMPTY",         STATUS_DIRECTORY_NOT_EMPTY },
    { "DISK_FULL",                   STATUS_DISK_FULL },
    { "DLL_NOT_FOUND",               STATUS_DLL_NOT_FOUND },
    { "ERROR_CLASS_ALREADY_EXISTS",  0xc0010000 | ERROR_CLASS_ALREADY_EXISTS },
    { "ERROR_CLASS_DOES_NOT_EXIST",  0xc0010000 | ERROR_CLASS_DOES_NOT_EXIST },
    { "ERROR_CLASS_HAS_WINDOWS",     0xc0010000 | ERROR_CLASS_HAS_WINDOWS },
    { "ERROR_CLIPBOARD_NOT_OPEN",    0xc0010000 | ERROR_CLIPBOARD_NOT_OPEN },
    { "ERROR_HOTKEY_ALREADY_REGISTERED", 0xc0010000 | ERROR_HOTKEY_ALREADY_REGISTERED },
    { "ERROR_HOTKEY_NOT_REGISTERED", 0xc0010000 | ERROR_HOTKEY_NOT_REGISTERED },
    { "ERROR_INVALID_CURSOR_HANDLE", 0xc0010000 | ERROR_INVALID_CURSOR_HANDLE },
    { "ERROR_INVALID_INDEX",         0xc0010000 | ERROR_INVALID_INDEX },
    { "ERROR_INVALID_WINDOW_HANDLE", 0xc0010000 | ERROR_INVALID_WINDOW_HANDLE },
    { "ERROR_WINDOW_OF_OTHER_THREAD", 0xc0010000 | ERROR_WINDOW_OF_OTHER_THREAD },
    { "FILE_DELETED",                STATUS_FILE_DELETED },
    { "FILE_IS_A_DIRECTORY",         STATUS_FILE_IS_A_DIRECTORY },
    { "FILE_LOCK_CONFLICT",          STATUS_FILE_LOCK_CONFLICT },
    { "GENERIC_NOT_MAPPED",          STATUS_GENERIC_NOT_MAPPED },
    { "HANDLES_CLOSED",              STATUS_HANDLES_CLOSED },
    { "HANDLE_NOT_CLOSABLE",         STATUS_HANDLE_NOT_CLOSABLE },
    { "HOST_UNREACHABLE",            STATUS_HOST_UNREACHABLE },
    { "ILLEGAL_FUNCTION",            STATUS_ILLEGAL_FUNCTION },
    { "INSTANCE_NOT_AVAILABLE",      STATUS_INSTANCE_NOT_AVAILABLE },
    { "INSUFFICIENT_RESOURCES",      STATUS_INSUFFICIENT_RESOURCES },
    { "INVALID_CID",                 STATUS_INVALID_CID },
    { "INVALID_DEVICE_REQUEST",      STATUS_INVALID_DEVICE_REQUEST },
    { "INVALID_FILE_FOR_SECTION",    STATUS_INVALID_FILE_FOR_SECTION },
    { "INVALID_HANDLE",              STATUS_INVALID_HANDLE },
    { "INVALID_IMAGE_FORMAT",        STATUS_INVALID_IMAGE_FORMAT },
    { "INVALID_IMAGE_NE_FORMAT",     STATUS_INVALID_IMAGE_NE_FORMAT },
    { "INVALID_IMAGE_NOT_MZ",        STATUS_INVALID_IMAGE_NOT_MZ },
    { "INVALID_IMAGE_PROTECT",       STATUS_INVALID_IMAGE_PROTECT },
    { "INVALID_IMAGE_WIN_64",        STATUS_INVALID_IMAGE_WIN_64 },
    { "INVALID_PARAMETER",           STATUS_INVALID_PARAMETER },
    { "INVALID_SECURITY_DESCR",      STATUS_INVALID_SECURITY_DESCR },
    { "IO_TIMEOUT",                  STATUS_IO_TIMEOUT },
    { "KEY_DELETED",                 STATUS_KEY_DELETED },
    { "MAPPED_FILE_SIZE_ZERO",       STATUS_MAPPED_FILE_SIZE_ZERO },
    { "MORE_PROCESSING_REQUIRED",    STATUS_MORE_PROCESSING_REQUIRED },
    { "MUTANT_NOT_OWNED",            STATUS_MUTANT_NOT_OWNED },
    { "NAME_TOO_LONG",               STATUS_NAME_TOO_LONG },
    { "NETWORK_BUSY",                STATUS_NETWORK_BUSY },
    { "NETWORK_UNREACHABLE",         STATUS_NETWORK_UNREACHABLE },
    { "NOTIFY_ENUM_DIR",             STATUS_NOTIFY_ENUM_DIR },
    { "NOT_ALL_ASSIGNED",            STATUS_NOT_ALL_ASSIGNED },
    { "NOT_A_DIRECTORY",             STATUS_NOT_A_DIRECTORY },
    { "NOT_FOUND",                   STATUS_NOT_FOUND },
    { "NOT_IMPLEMENTED",             STATUS_NOT_IMPLEMENTED },
    { "NOT_REGISTRY_FILE",           STATUS_NOT_REGISTRY_FILE },
    { "NOT_SUPPORTED",               STATUS_NOT_SUPPORTED },
    { "NO_DATA_DETECTED",            STATUS_NO_DATA_DETECTED },
    { "NO_IMPERSONATION_TOKEN",      STATUS_NO_IMPERSONATION_TOKEN },
    { "NO_MEMORY",                   STATUS_NO_MEMORY },
    { "NO_MORE_ENTRIES",             STATUS_NO_MORE_ENTRIES },
    { "NO_MORE_FILES",               STATUS_NO_MORE_FILES },
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
    { "PIPE_BUSY",                   STATUS_PIPE_BUSY },
    { "PIPE_CONNECTED",              STATUS_PIPE_CONNECTED },
    { "PIPE_DISCONNECTED",           STATUS_PIPE_DISCONNECTED },
    { "PIPE_LISTENING",              STATUS_PIPE_LISTENING },
    { "PIPE_NOT_AVAILABLE",          STATUS_PIPE_NOT_AVAILABLE },
    { "PRIVILEGE_NOT_HELD",          STATUS_PRIVILEGE_NOT_HELD },
    { "PROCESS_IS_TERMINATING",      STATUS_PROCESS_IS_TERMINATING },
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
