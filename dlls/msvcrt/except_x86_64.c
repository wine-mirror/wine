/*
 * msvcrt C++ exception handling
 *
 * Copyright 2011 Alexandre Julliard
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

#ifdef __x86_64__

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "msvcrt.h"
#include "wine/exception.h"
#include "excpt.h"
#include "wine/debug.h"

#include "cppexcept.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

struct _DISPATCHER_CONTEXT;

typedef LONG (WINAPI *PC_LANGUAGE_EXCEPTION_HANDLER)( EXCEPTION_POINTERS *ptrs, ULONG64 frame );
typedef EXCEPTION_DISPOSITION (WINAPI *PEXCEPTION_ROUTINE)( EXCEPTION_RECORD *rec,
                                                            ULONG64 frame,
                                                            CONTEXT *context,
                                                            struct _DISPATCHER_CONTEXT *dispatch );

typedef struct _DISPATCHER_CONTEXT
{
    ULONG64               ControlPc;
    ULONG64               ImageBase;
    PRUNTIME_FUNCTION     FunctionEntry;
    ULONG64               EstablisherFrame;
    ULONG64               TargetIp;
    PCONTEXT              ContextRecord;
    PEXCEPTION_ROUTINE    LanguageHandler;
    PVOID                 HandlerData;
    PUNWIND_HISTORY_TABLE HistoryTable;
    ULONG                 ScopeIndex;
} DISPATCHER_CONTEXT;

typedef struct
{
    int  prev;
    UINT handler;
} unwind_info;

typedef struct
{
    UINT flags;
    UINT type_info;
    int  offset;
    UINT handler;
    UINT frame;
} catchblock_info;
#define TYPE_FLAG_CONST      1
#define TYPE_FLAG_VOLATILE   2
#define TYPE_FLAG_REFERENCE  8

typedef struct
{
    int  start_level;
    int  end_level;
    int  catch_level;
    int  catchblock_count;
    UINT catchblock;
} tryblock_info;

typedef struct
{
    int ip;
    int state;
} ipmap_info;

typedef struct __cxx_function_descr
{
    UINT magic;
    UINT unwind_count;
    UINT unwind_table;
    UINT tryblock_count;
    UINT tryblock;
    UINT ipmap_count;
    UINT ipmap;
    UINT unwind_help;
    UINT expect_list;
    UINT flags;
} cxx_function_descr;

static inline void* rva_to_ptr(UINT rva, ULONG64 base)
{
    return rva ? (void*)(base+rva) : NULL;
}

static inline void dump_type(UINT type_rva, ULONG64 base)
{
    const cxx_type_info *type = rva_to_ptr(type_rva, base);

    TRACE("flags %x type %x %s offsets %d,%d,%d size %d copy ctor %x(%p)\n",
            type->flags, type->type_info, dbgstr_type_info(rva_to_ptr(type->type_info, base)),
            type->offsets.this_offset, type->offsets.vbase_descr, type->offsets.vbase_offset,
            type->size, type->copy_ctor, rva_to_ptr(type->copy_ctor, base));
}

static void dump_exception_type(const cxx_exception_type *type, ULONG64 base)
{
    const cxx_type_info_table *type_info_table = rva_to_ptr(type->type_info_table, base);
    UINT i;

    TRACE("flags %x destr %x(%p) handler %x(%p) type info %x(%p)\n",
            type->flags, type->destructor, rva_to_ptr(type->destructor, base),
            type->custom_handler, rva_to_ptr(type->custom_handler, base),
            type->type_info_table, type_info_table);
    for (i = 0; i < type_info_table->count; i++)
    {
        TRACE("    %d: ", i);
        dump_type(type_info_table->info[i], base);
    }
}

static void dump_function_descr(const cxx_function_descr *descr, ULONG64 image_base)
{
    unwind_info *unwind_table = rva_to_ptr(descr->unwind_table, image_base);
    tryblock_info *tryblock = rva_to_ptr(descr->tryblock, image_base);
    ipmap_info *ipmap = rva_to_ptr(descr->ipmap, image_base);
    UINT i, j;

    TRACE("magic %x\n", descr->magic);
    TRACE("unwind table: %x(%p) %d\n", descr->unwind_table, unwind_table, descr->unwind_count);
    for (i=0; i<descr->unwind_count; i++)
    {
        TRACE("    %d: prev %d func %x(%p)\n", i, unwind_table[i].prev,
                unwind_table[i].handler, rva_to_ptr(unwind_table[i].handler, image_base));
    }
    TRACE("try table: %x(%p) %d\n", descr->tryblock, tryblock, descr->tryblock_count);
    for (i=0; i<descr->tryblock_count; i++)
    {
        catchblock_info *catchblock = rva_to_ptr(tryblock[i].catchblock, image_base);

        TRACE("    %d: start %d end %d catchlevel %d catch %x(%p) %d\n", i,
                tryblock[i].start_level, tryblock[i].end_level,
                tryblock[i].catch_level, tryblock[i].catchblock,
                catchblock, tryblock[i].catchblock_count);
        for (j=0; j<tryblock[i].catchblock_count; j++)
        {
            TRACE("        %d: flags %x offset %d handler %x(%p) frame %x type %x %s\n",
                    j, catchblock[j].flags, catchblock[j].offset, catchblock[j].handler,
                    rva_to_ptr(catchblock[j].handler, image_base), catchblock[j].frame,
                    catchblock[j].type_info,
                    dbgstr_type_info(rva_to_ptr(catchblock[j].type_info, image_base)));
        }
    }
    TRACE("ipmap: %x(%p) %d\n", descr->ipmap, ipmap, descr->ipmap_count);
    for (i=0; i<descr->ipmap_count; i++)
    {
        TRACE("    %d: ip %x state %d\n", i, ipmap[i].ip, ipmap[i].state);
    }
    TRACE("unwind_help %d\n", descr->unwind_help);
    if (descr->magic <= CXX_FRAME_MAGIC_VC6) return;
    TRACE("expect list: %x\n", descr->expect_list);
    if (descr->magic <= CXX_FRAME_MAGIC_VC7) return;
    TRACE("flags: %08x\n", descr->flags);
}

static inline int ip_to_state(ipmap_info *ipmap, UINT count, int ip)
{
    UINT low = 0, high = count-1, med;

    while (low < high) {
        med = low + (high-low)/2;

        if (ipmap[med].ip <= ip && ipmap[med+1].ip > ip)
        {
            low = med;
            break;
        }
        if (ipmap[med].ip < ip) low = med+1;
        else high = med-1;
    }

    TRACE("%x -> %d\n", ip, ipmap[low].state);
    return ipmap[low].state;
}

/* check if the exception type is caught by a given catch block, and return the type that matched */
static const cxx_type_info *find_caught_type(cxx_exception_type *exc_type, ULONG64 exc_base,
                                             const type_info *catch_ti, UINT catch_flags)
{
    const cxx_type_info_table *type_info_table = rva_to_ptr(exc_type->type_info_table, exc_base);
    UINT i;

    for (i = 0; i < type_info_table->count; i++)
    {
        const cxx_type_info *type = rva_to_ptr(type_info_table->info[i], exc_base);
        const type_info *ti = rva_to_ptr(type->type_info, exc_base);

        if (!catch_ti) return type;   /* catch(...) matches any type */
        if (catch_ti != ti)
        {
            if (strcmp( catch_ti->mangled, ti->mangled )) continue;
        }
        /* type is the same, now check the flags */
        if ((exc_type->flags & TYPE_FLAG_CONST) &&
                !(catch_flags & TYPE_FLAG_CONST)) continue;
        if ((exc_type->flags & TYPE_FLAG_VOLATILE) &&
                !(catch_flags & TYPE_FLAG_VOLATILE)) continue;
        return type;  /* it matched */
    }
    return NULL;
}

static inline void copy_exception(void *object, ULONG64 frame,
                                  DISPATCHER_CONTEXT *dispatch,
                                  const catchblock_info *catchblock,
                                  const cxx_type_info *type)
{
    const type_info *catch_ti = rva_to_ptr(catchblock->type_info, dispatch->ImageBase);
    void **dest = rva_to_ptr(catchblock->offset, frame);

    if (!catch_ti || !catch_ti->mangled[0]) return;
    if (!catchblock->offset) return;

    if (catchblock->flags & TYPE_FLAG_REFERENCE)
    {
        *dest = get_this_pointer(&type->offsets, object);
    }
    else if (type->flags & CLASS_IS_SIMPLE_TYPE)
    {
        memmove(dest, object, type->size);
        /* if it is a pointer, adjust it */
        if (type->size == sizeof(void*)) *dest = get_this_pointer(&type->offsets, *dest);
    }
    else  /* copy the object */
    {
        if (type->copy_ctor)
        {
            if (type->flags & CLASS_HAS_VIRTUAL_BASE_CLASS)
            {
                void (__cdecl *copy_ctor)(void*, void*, int) =
                    rva_to_ptr(type->copy_ctor, dispatch->ImageBase);
                copy_ctor(dest, get_this_pointer(&type->offsets, object), 1);
            }
            else
            {
                void (__cdecl *copy_ctor)(void*, void*) =
                    rva_to_ptr(type->copy_ctor, dispatch->ImageBase);
                copy_ctor(dest, get_this_pointer(&type->offsets, object));
            }
        }
        else
            memmove(dest, get_this_pointer(&type->offsets,object), type->size);
    }
}

static void cxx_local_unwind(ULONG64 frame, DISPATCHER_CONTEXT *dispatch,
                             const cxx_function_descr *descr, int last_level)
{
    const unwind_info *unwind_table = rva_to_ptr(descr->unwind_table, dispatch->ImageBase);
    void (__cdecl *handler)(ULONG64 unk, ULONG64 rbp);
    int *unwind_help = rva_to_ptr(descr->unwind_help, frame);
    int trylevel;

    if (unwind_help[0] == -2)
    {
        trylevel = ip_to_state(rva_to_ptr(descr->ipmap, dispatch->ImageBase),
                descr->ipmap_count, dispatch->ControlPc-dispatch->ImageBase);
    }
    else
    {
        trylevel = unwind_help[0];
    }

    TRACE("current level: %d, last level: %d\n", trylevel, last_level);
    while (trylevel != last_level)
    {
        if (trylevel<0 || trylevel>=descr->unwind_count)
        {
            ERR("invalid trylevel %d\n", trylevel);
            MSVCRT_terminate();
        }
        handler = rva_to_ptr(unwind_table[trylevel].handler, dispatch->ImageBase);
        if (handler)
        {
            TRACE("handler: %p\n", handler);
            handler(0, frame);
        }
        trylevel = unwind_table[trylevel].prev;
    }
    unwind_help[0] = last_level;
}

static inline void* WINAPI call_catch_block(EXCEPTION_RECORD *rec)
{
    ULONG64 frame = rec->ExceptionInformation[1];
    const cxx_function_descr *descr = (void*)rec->ExceptionInformation[2];
    EXCEPTION_RECORD *prev_rec = (void*)rec->ExceptionInformation[4];
    void* (__cdecl *handler)(ULONG64 unk, ULONG64 rbp) = (void*)rec->ExceptionInformation[5];
    int *unwind_help = rva_to_ptr(descr->unwind_help, frame);
    cxx_frame_info frame_info;
    void *ret_addr;

    TRACE("calling handler %p\n", handler);

    /* FIXME:  native does local_unwind here in case of exception rethrow */
    __CxxRegisterExceptionObject(&prev_rec, &frame_info);
    ret_addr = handler(0, frame);
    __CxxUnregisterExceptionObject(&frame_info, FALSE);
    unwind_help[0] = -2;
    unwind_help[1] = -1;
    return ret_addr;
}

static inline void find_catch_block(EXCEPTION_RECORD *rec, ULONG64 frame,
                                    DISPATCHER_CONTEXT *dispatch,
                                    const cxx_function_descr *descr,
                                    cxx_exception_type *info)
{
    ULONG64 exc_base = (rec->NumberParameters == 4 ? rec->ExceptionInformation[3] : 0);
    int trylevel = ip_to_state(rva_to_ptr(descr->ipmap, dispatch->ImageBase),
            descr->ipmap_count, dispatch->ControlPc-dispatch->ImageBase);
    EXCEPTION_RECORD catch_record;
    CONTEXT context;
    UINT i, j;
    ULONG64 orig_frame = frame, throw_base;
    DWORD throw_func_off;
    void *throw_func;
    INT *unwind_help;

    /* update orig_frame if it's a nested exception */
    throw_func_off = RtlLookupFunctionEntry(dispatch->ControlPc, &throw_base, NULL)->BeginAddress;
    throw_func = rva_to_ptr(throw_func_off, throw_base);
    TRACE("reconstructed handler pointer: %p\n", throw_func);
    for (i=descr->tryblock_count; i>0; i--)
    {
        const tryblock_info *tryblock = rva_to_ptr(descr->tryblock, dispatch->ImageBase);
        tryblock = &tryblock[i-1];

        if (trylevel>tryblock->end_level && trylevel<=tryblock->catch_level)
        {
            for (j=0; j<tryblock->catchblock_count; j++)
            {
                /* TODO: is it possible to have the same handler for multiple catch blocks? */
                const catchblock_info *catchblock = rva_to_ptr(tryblock->catchblock, dispatch->ImageBase);
                catchblock = &catchblock[j];

                if (rva_to_ptr(catchblock->handler, dispatch->ImageBase) == throw_func)
                {
                    TRACE("nested exception detected\n");
                    orig_frame = *(ULONG64*)rva_to_ptr(catchblock->frame, frame);
                    TRACE("setting orig_frame to %lx\n", orig_frame);
                }
            }
        }
    }

    unwind_help = rva_to_ptr(descr->unwind_help, orig_frame);
    unwind_help[0] = trylevel;
    TRACE("current trylevel: %d, last catch block: %d\n", trylevel, unwind_help[1]);

    for (i=0; i<descr->tryblock_count; i++)
    {
        const tryblock_info *tryblock = rva_to_ptr(descr->tryblock, dispatch->ImageBase);
        tryblock = &tryblock[i];

        if (unwind_help[1] != -1)
        {
            if (unwind_help[1] < tryblock->start_level) continue;
            if (unwind_help[1] > tryblock->end_level) continue;
        }

        if (trylevel < tryblock->start_level) continue;
        if (trylevel > tryblock->end_level) continue;

        /* got a try block */
        for (j=0; j<tryblock->catchblock_count; j++)
        {
            const catchblock_info *catchblock = rva_to_ptr(tryblock->catchblock, dispatch->ImageBase);
            catchblock = &catchblock[j];

            if (info)
            {
                const cxx_type_info *type = find_caught_type(info, exc_base,
                        rva_to_ptr(catchblock->type_info, dispatch->ImageBase),
                        catchblock->flags);
                if (!type) continue;

                TRACE("matched type %p in tryblock %d catchblock %d\n", type, i, j);

                /* copy the exception to its destination on the stack */
                copy_exception((void*)rec->ExceptionInformation[1],
                        orig_frame, dispatch, catchblock, type);
            }
            else
            {
                /* no CXX_EXCEPTION only proceed with a catch(...) block*/
                if (catchblock->type_info)
                    continue;
                TRACE("found catch(...) block\n");
            }

            unwind_help[1] = tryblock->end_level+1;

            /* unwind stack and call catch */
            memset(&catch_record, 0, sizeof(catch_record));
            catch_record.ExceptionCode = STATUS_UNWIND_CONSOLIDATE;
            catch_record.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
            catch_record.NumberParameters = 6;
            catch_record.ExceptionInformation[0] = (ULONG_PTR)call_catch_block;
            catch_record.ExceptionInformation[1] = orig_frame;
            catch_record.ExceptionInformation[2] = (ULONG_PTR)descr;
            catch_record.ExceptionInformation[3] = tryblock->start_level;
            catch_record.ExceptionInformation[4] = (ULONG_PTR)rec;
            catch_record.ExceptionInformation[5] =
                (ULONG_PTR)rva_to_ptr(catchblock->handler, dispatch->ImageBase);
            RtlUnwindEx((void*)orig_frame, 0, &catch_record, NULL, &context, NULL);
        }
    }

    TRACE("no matching catch block found\n");
}

static DWORD cxx_frame_handler(EXCEPTION_RECORD *rec, ULONG64 frame,
                               CONTEXT *context, DISPATCHER_CONTEXT *dispatch,
                               const cxx_function_descr *descr)
{
    cxx_exception_type *exc_type;

    if (descr->magic<CXX_FRAME_MAGIC_VC6 || descr->magic>CXX_FRAME_MAGIC_VC8)
    {
        FIXME("unhandled frame magic %x\n", descr->magic);
        return ExceptionContinueSearch;
    }

    if (rec->ExceptionFlags & (EH_UNWINDING|EH_EXIT_UNWIND))
    {
        if (rec->ExceptionCode==STATUS_UNWIND_CONSOLIDATE && rec->NumberParameters==6 &&
                rec->ExceptionInformation[0]==(ULONG_PTR)call_catch_block)
        {
            ULONG64 orig_frame = rec->ExceptionInformation[1];
            const cxx_function_descr *descr = (void*)rec->ExceptionInformation[2];
            int end_level = rec->ExceptionInformation[3];
            frame_info *cur;

            cxx_local_unwind(orig_frame, dispatch, descr, end_level);

            /* FIXME: we should only unregister frames registered by call_catch_block here */
            for (cur = msvcrt_get_thread_data()->frame_info_head; cur; cur = cur->next)
            {
                if ((ULONG64)cur > frame)
                    __CxxUnregisterExceptionObject((cxx_frame_info*)cur, FALSE);
            }
            return ExceptionContinueSearch;
        }

        cxx_local_unwind(frame, dispatch, descr, -1);
        return ExceptionContinueSearch;
    }
    if (!descr->tryblock_count) return ExceptionContinueSearch;

    if (rec->ExceptionCode == CXX_EXCEPTION &&
            rec->ExceptionInformation[1] == 0 && rec->ExceptionInformation[2] == 0)
    {
        *rec = *msvcrt_get_thread_data()->exc_record;
        rec->ExceptionFlags &= ~EH_UNWINDING;
        if (TRACE_ON(seh)) {
            TRACE("detect rethrow: exception code: %x\n", rec->ExceptionCode);
            if (rec->ExceptionCode == CXX_EXCEPTION)
                TRACE("re-propage: obj: %lx, type: %lx\n",
                        rec->ExceptionInformation[1], rec->ExceptionInformation[2]);
        }
    }

    if (rec->ExceptionCode == CXX_EXCEPTION)
    {
        exc_type = (cxx_exception_type *)rec->ExceptionInformation[2];

        if (TRACE_ON(seh))
        {
            TRACE("handling C++ exception rec %p frame %lx descr %p\n", rec, frame,  descr);
            dump_exception_type(exc_type, rec->ExceptionInformation[3]);
            dump_function_descr(descr, dispatch->ImageBase);
        }
    }
    else
    {
        thread_data_t *data = msvcrt_get_thread_data();

        exc_type = NULL;
        TRACE("handling C exception code %x rec %p frame %lx descr %p\n",
                rec->ExceptionCode, rec, frame, descr);

        if (data->se_translator) {
            EXCEPTION_POINTERS except_ptrs;

            except_ptrs.ExceptionRecord = rec;
            except_ptrs.ContextRecord = context;
            data->se_translator(rec->ExceptionCode, &except_ptrs);
        }
    }

    find_catch_block(rec, frame, dispatch, descr, exc_type);
    return ExceptionContinueSearch;
}

/*********************************************************************
 *		__CxxExceptionFilter (MSVCRT.@)
 */
int CDECL __CxxExceptionFilter( PEXCEPTION_POINTERS ptrs,
                                const type_info *ti, int flags, void **copy )
{
    FIXME( "%p %p %x %p: not implemented\n", ptrs, ti, flags, copy );
    return EXCEPTION_CONTINUE_SEARCH;
}

/*********************************************************************
 *		__CxxFrameHandler (MSVCRT.@)
 */
EXCEPTION_DISPOSITION CDECL __CxxFrameHandler( EXCEPTION_RECORD *rec, ULONG64 frame,
                                               CONTEXT *context, DISPATCHER_CONTEXT *dispatch )
{
    TRACE( "%p %lx %p %p\n", rec, frame, context, dispatch );
    return cxx_frame_handler( rec, frame, context, dispatch,
            rva_to_ptr(*(UINT*)dispatch->HandlerData, dispatch->ImageBase) );
}


/*********************************************************************
 *		__CppXcptFilter (MSVCRT.@)
 */
int CDECL __CppXcptFilter(NTSTATUS ex, PEXCEPTION_POINTERS ptr)
{
    /* only filter c++ exceptions */
    if (ex != CXX_EXCEPTION) return EXCEPTION_CONTINUE_SEARCH;
    return _XcptFilter( ex, ptr );
}


/*********************************************************************
 *		__CxxDetectRethrow (MSVCRT.@)
 */
BOOL CDECL __CxxDetectRethrow(PEXCEPTION_POINTERS ptrs)
{
    PEXCEPTION_RECORD rec;

    if (!ptrs)
        return FALSE;

    rec = ptrs->ExceptionRecord;

    if (rec->ExceptionCode == CXX_EXCEPTION &&
        rec->NumberParameters == 4 &&
        rec->ExceptionInformation[0] == CXX_FRAME_MAGIC_VC6 &&
        rec->ExceptionInformation[2])
    {
        ptrs->ExceptionRecord = msvcrt_get_thread_data()->exc_record;
        return TRUE;
    }
    return (msvcrt_get_thread_data()->exc_record == rec);
}


/*********************************************************************
 *		__CxxQueryExceptionSize (MSVCRT.@)
 */
unsigned int CDECL __CxxQueryExceptionSize(void)
{
    return sizeof(cxx_exception_type);
}


/*******************************************************************
 *		_setjmp (MSVCRT.@)
 */
__ASM_GLOBAL_FUNC( MSVCRT__setjmp,
                   "xorq %rdx,%rdx\n\t"  /* frame */
                   "jmp " __ASM_NAME("MSVCRT__setjmpex") );

/*******************************************************************
 *		_setjmpex (MSVCRT.@)
 */
__ASM_GLOBAL_FUNC( MSVCRT__setjmpex,
                   "movq %rdx,(%rcx)\n\t"          /* jmp_buf->Frame */
                   "movq %rbx,0x8(%rcx)\n\t"       /* jmp_buf->Rbx */
                   "leaq 0x8(%rsp),%rax\n\t"
                   "movq %rax,0x10(%rcx)\n\t"      /* jmp_buf->Rsp */
                   "movq %rbp,0x18(%rcx)\n\t"      /* jmp_buf->Rbp */
                   "movq %rsi,0x20(%rcx)\n\t"      /* jmp_buf->Rsi */
                   "movq %rdi,0x28(%rcx)\n\t"      /* jmp_buf->Rdi */
                   "movq %r12,0x30(%rcx)\n\t"      /* jmp_buf->R12 */
                   "movq %r13,0x38(%rcx)\n\t"      /* jmp_buf->R13 */
                   "movq %r14,0x40(%rcx)\n\t"      /* jmp_buf->R14 */
                   "movq %r15,0x48(%rcx)\n\t"      /* jmp_buf->R15 */
                   "movq (%rsp),%rax\n\t"
                   "movq %rax,0x50(%rcx)\n\t"      /* jmp_buf->Rip */
                   "movdqa %xmm6,0x60(%rcx)\n\t"   /* jmp_buf->Xmm6 */
                   "movdqa %xmm7,0x70(%rcx)\n\t"   /* jmp_buf->Xmm7 */
                   "movdqa %xmm8,0x80(%rcx)\n\t"   /* jmp_buf->Xmm8 */
                   "movdqa %xmm9,0x90(%rcx)\n\t"   /* jmp_buf->Xmm9 */
                   "movdqa %xmm10,0xa0(%rcx)\n\t"  /* jmp_buf->Xmm10 */
                   "movdqa %xmm11,0xb0(%rcx)\n\t"  /* jmp_buf->Xmm11 */
                   "movdqa %xmm12,0xc0(%rcx)\n\t"  /* jmp_buf->Xmm12 */
                   "movdqa %xmm13,0xd0(%rcx)\n\t"  /* jmp_buf->Xmm13 */
                   "movdqa %xmm14,0xe0(%rcx)\n\t"  /* jmp_buf->Xmm14 */
                   "movdqa %xmm15,0xf0(%rcx)\n\t"  /* jmp_buf->Xmm15 */
                   "xorq %rax,%rax\n\t"
                   "retq" );


extern void DECLSPEC_NORETURN CDECL longjmp_set_regs( struct MSVCRT___JUMP_BUFFER *jmp, int retval );
__ASM_GLOBAL_FUNC( longjmp_set_regs,
                   "movq %rdx,%rax\n\t"            /* retval */
                   "movq 0x8(%rcx),%rbx\n\t"       /* jmp_buf->Rbx */
                   "movq 0x18(%rcx),%rbp\n\t"      /* jmp_buf->Rbp */
                   "movq 0x20(%rcx),%rsi\n\t"      /* jmp_buf->Rsi */
                   "movq 0x28(%rcx),%rdi\n\t"      /* jmp_buf->Rdi */
                   "movq 0x30(%rcx),%r12\n\t"      /* jmp_buf->R12 */
                   "movq 0x38(%rcx),%r13\n\t"      /* jmp_buf->R13 */
                   "movq 0x40(%rcx),%r14\n\t"      /* jmp_buf->R14 */
                   "movq 0x48(%rcx),%r15\n\t"      /* jmp_buf->R15 */
                   "movdqa 0x60(%rcx),%xmm6\n\t"   /* jmp_buf->Xmm6 */
                   "movdqa 0x70(%rcx),%xmm7\n\t"   /* jmp_buf->Xmm7 */
                   "movdqa 0x80(%rcx),%xmm8\n\t"   /* jmp_buf->Xmm8 */
                   "movdqa 0x90(%rcx),%xmm9\n\t"   /* jmp_buf->Xmm9 */
                   "movdqa 0xa0(%rcx),%xmm10\n\t"  /* jmp_buf->Xmm10 */
                   "movdqa 0xb0(%rcx),%xmm11\n\t"  /* jmp_buf->Xmm11 */
                   "movdqa 0xc0(%rcx),%xmm12\n\t"  /* jmp_buf->Xmm12 */
                   "movdqa 0xd0(%rcx),%xmm13\n\t"  /* jmp_buf->Xmm13 */
                   "movdqa 0xe0(%rcx),%xmm14\n\t"  /* jmp_buf->Xmm14 */
                   "movdqa 0xf0(%rcx),%xmm15\n\t"  /* jmp_buf->Xmm15 */
                   "movq 0x50(%rcx),%rdx\n\t"      /* jmp_buf->Rip */
                   "movq 0x10(%rcx),%rsp\n\t"      /* jmp_buf->Rsp */
                   "jmp *%rdx" );

/*******************************************************************
 *		longjmp (MSVCRT.@)
 */
void __cdecl MSVCRT_longjmp( struct MSVCRT___JUMP_BUFFER *jmp, int retval )
{
    EXCEPTION_RECORD rec;

    if (!retval) retval = 1;
    if (jmp->Frame)
    {
        rec.ExceptionCode = STATUS_LONGJUMP;
        rec.ExceptionFlags = 0;
        rec.ExceptionRecord = NULL;
        rec.ExceptionAddress = NULL;
        rec.NumberParameters = 1;
        rec.ExceptionInformation[0] = (DWORD_PTR)jmp;
        RtlUnwind( (void *)jmp->Frame, (void *)jmp->Rip, &rec, IntToPtr(retval) );
    }
    longjmp_set_regs( jmp, retval );
}

/*******************************************************************
 *		_local_unwind (MSVCRT.@)
 */
void __cdecl _local_unwind( void *frame, void *target )
{
    RtlUnwind( frame, target, NULL, 0 );
}

#endif  /* __x86_64__ */
