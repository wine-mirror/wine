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

#if defined(__x86_64__) && !defined(__arm64ec__)

#include <stdarg.h>
#include <fpieee.h>
#define longjmp ms_longjmp  /* avoid prototype mismatch */
#include <setjmp.h>
#undef longjmp

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

typedef struct
{
    cxx_frame_info frame_info;
    BOOL rethrow;
    EXCEPTION_RECORD *prev_rec;
} cxx_catch_ctx;

typedef struct
{
    ULONG64 dest_frame;
    ULONG64 orig_frame;
    EXCEPTION_RECORD *seh_rec;
    DISPATCHER_CONTEXT *dispatch;
    const cxx_function_descr *descr;
} se_translator_ctx;

static inline int ip_to_state( const cxx_function_descr *descr, uintptr_t ip, uintptr_t base )
{
    const ipmap_info *ipmap = rtti_rva( descr->ipmap, base );
    unsigned int i;
    int ret;

    for (i = 0; i < descr->ipmap_count; i++) if (base + ipmap[i].ip > ip) break;
    ret = i ? ipmap[i - 1].state : -1;
    TRACE( "%Ix -> %d\n", ip, ret );
    return ret;
}

static void cxx_local_unwind(ULONG64 frame, DISPATCHER_CONTEXT *dispatch,
                             const cxx_function_descr *descr, int last_level)
{
    const unwind_info *unwind_table = rtti_rva(descr->unwind_table, dispatch->ImageBase);
    void (__cdecl *handler)(ULONG64 unk, ULONG64 rbp);
    int *unwind_help = (int *)(frame + descr->unwind_help);
    int trylevel;

    if (unwind_help[0] == -2)
    {
        trylevel = ip_to_state( descr, dispatch->ControlPc, dispatch->ImageBase );
    }
    else
    {
        trylevel = unwind_help[0];
    }

    TRACE("current level: %d, last level: %d\n", trylevel, last_level);
    while (trylevel > last_level)
    {
        if (trylevel<0 || trylevel>=descr->unwind_count)
        {
            ERR("invalid trylevel %d\n", trylevel);
            terminate();
        }
        if (unwind_table[trylevel].handler)
        {
            handler = rtti_rva(unwind_table[trylevel].handler, dispatch->ImageBase);
            TRACE("handler: %p\n", handler);
            handler(0, frame);
        }
        trylevel = unwind_table[trylevel].prev;
    }
    unwind_help[0] = trylevel;
}

static LONG CALLBACK cxx_rethrow_filter(PEXCEPTION_POINTERS eptrs, void *c)
{
    EXCEPTION_RECORD *rec = eptrs->ExceptionRecord;
    cxx_catch_ctx *ctx = c;

    if (rec->ExceptionCode != CXX_EXCEPTION)
        return EXCEPTION_CONTINUE_SEARCH;
    if (!rec->ExceptionInformation[1] && !rec->ExceptionInformation[2])
        return EXCEPTION_EXECUTE_HANDLER;
    if (rec->ExceptionInformation[1] == ctx->prev_rec->ExceptionInformation[1])
        ctx->rethrow = TRUE;
    return EXCEPTION_CONTINUE_SEARCH;
}

static void CALLBACK cxx_catch_cleanup(BOOL normal, void *c)
{
    cxx_catch_ctx *ctx = c;
    __CxxUnregisterExceptionObject(&ctx->frame_info, ctx->rethrow);
}

static void* WINAPI call_catch_block(EXCEPTION_RECORD *rec)
{
    ULONG64 frame = rec->ExceptionInformation[1];
    const cxx_function_descr *descr = (void*)rec->ExceptionInformation[2];
    EXCEPTION_RECORD *prev_rec = (void*)rec->ExceptionInformation[4];
    EXCEPTION_RECORD *untrans_rec = (void*)rec->ExceptionInformation[6];
    CONTEXT *context = (void*)rec->ExceptionInformation[7];
    void* (__cdecl *handler)(ULONG64 unk, ULONG64 rbp) = (void*)rec->ExceptionInformation[5];
    int *unwind_help = (int *)(frame + descr->unwind_help);
    EXCEPTION_POINTERS ep = { prev_rec, context };
    cxx_catch_ctx ctx;
    void *ret_addr = NULL;

    TRACE("calling handler %p\n", handler);

    ctx.rethrow = FALSE;
    ctx.prev_rec = prev_rec;
    __CxxRegisterExceptionObject(&ep, &ctx.frame_info);
    msvcrt_get_thread_data()->processing_throw--;
    __TRY
    {
        __TRY
        {
            ret_addr = handler(0, frame);
        }
        __EXCEPT_CTX(cxx_rethrow_filter, &ctx)
        {
            TRACE("detect rethrow: exception code: %lx\n", prev_rec->ExceptionCode);
            ctx.rethrow = TRUE;

            if (untrans_rec)
            {
                __DestructExceptionObject(prev_rec);
                RaiseException(untrans_rec->ExceptionCode, untrans_rec->ExceptionFlags,
                        untrans_rec->NumberParameters, untrans_rec->ExceptionInformation);
            }
            else
            {
                RaiseException(prev_rec->ExceptionCode, prev_rec->ExceptionFlags,
                        prev_rec->NumberParameters, prev_rec->ExceptionInformation);
            }
        }
        __ENDTRY
    }
    __FINALLY_CTX(cxx_catch_cleanup, &ctx)

    unwind_help[0] = -2;
    unwind_help[1] = -1;
    return ret_addr;
}

static inline BOOL cxx_is_consolidate(const EXCEPTION_RECORD *rec)
{
    return rec->ExceptionCode==STATUS_UNWIND_CONSOLIDATE && rec->NumberParameters==8 &&
           rec->ExceptionInformation[0]==(ULONG_PTR)call_catch_block;
}

static inline void find_catch_block(EXCEPTION_RECORD *rec, CONTEXT *context,
                                    EXCEPTION_RECORD *untrans_rec,
                                    ULONG64 frame, DISPATCHER_CONTEXT *dispatch,
                                    const cxx_function_descr *descr,
                                    cxx_exception_type *info, ULONG64 orig_frame)
{
    ULONG64 exc_base = (rec->NumberParameters == 4 ? rec->ExceptionInformation[3] : 0);
    void *handler, *object = (void *)rec->ExceptionInformation[1];
    int trylevel = ip_to_state( descr, dispatch->ControlPc, dispatch->ImageBase );
    thread_data_t *data = msvcrt_get_thread_data();
    const tryblock_info *in_catch;
    EXCEPTION_RECORD catch_record;
    CONTEXT ctx;
    UINT i;
    INT *unwind_help;

    data->processing_throw++;
    for (i=descr->tryblock_count; i>0; i--)
    {
        in_catch = rtti_rva(descr->tryblock, dispatch->ImageBase);
        in_catch = &in_catch[i-1];

        if (trylevel>in_catch->end_level && trylevel<=in_catch->catch_level)
            break;
    }
    if (!i)
        in_catch = NULL;

    unwind_help = (int *)(orig_frame + descr->unwind_help);
    if (trylevel > unwind_help[1])
        unwind_help[0] = unwind_help[1] = trylevel;
    else
        trylevel = unwind_help[1];
    TRACE("current trylevel: %d\n", trylevel);

    for (i=0; i<descr->tryblock_count; i++)
    {
        const tryblock_info *tryblock = rtti_rva(descr->tryblock, dispatch->ImageBase);
        tryblock = &tryblock[i];

        if (trylevel < tryblock->start_level) continue;
        if (trylevel > tryblock->end_level) continue;

        if (in_catch)
        {
            if(tryblock->start_level <= in_catch->end_level) continue;
            if(tryblock->end_level > in_catch->catch_level) continue;
        }

        handler = find_catch_handler( object, orig_frame, exc_base, tryblock, info, dispatch->ImageBase );
        if (!handler) continue;

        /* unwind stack and call catch */
        memset(&catch_record, 0, sizeof(catch_record));
        catch_record.ExceptionCode = STATUS_UNWIND_CONSOLIDATE;
        catch_record.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
        catch_record.NumberParameters = 8;
        catch_record.ExceptionInformation[0] = (ULONG_PTR)call_catch_block;
        catch_record.ExceptionInformation[1] = orig_frame;
        catch_record.ExceptionInformation[2] = (ULONG_PTR)descr;
        catch_record.ExceptionInformation[3] = tryblock->start_level;
        catch_record.ExceptionInformation[4] = (ULONG_PTR)rec;
        catch_record.ExceptionInformation[5] = (ULONG_PTR)handler;
        catch_record.ExceptionInformation[6] = (ULONG_PTR)untrans_rec;
        catch_record.ExceptionInformation[7] = (ULONG_PTR)context;
        RtlUnwindEx((void*)frame, (void*)dispatch->ControlPc, &catch_record, NULL, &ctx, NULL);
    }

    TRACE("no matching catch block found\n");
    data->processing_throw--;
}

static LONG CALLBACK se_translation_filter(EXCEPTION_POINTERS *ep, void *c)
{
    se_translator_ctx *ctx = (se_translator_ctx *)c;
    EXCEPTION_RECORD *rec = ep->ExceptionRecord;
    cxx_exception_type *exc_type;

    if (rec->ExceptionCode != CXX_EXCEPTION)
    {
        TRACE("non-c++ exception thrown in SEH handler: %lx\n", rec->ExceptionCode);
        terminate();
    }

    exc_type = (cxx_exception_type *)rec->ExceptionInformation[2];
    find_catch_block(rec, ep->ContextRecord, ctx->seh_rec, ctx->dest_frame, ctx->dispatch,
                     ctx->descr, exc_type, ctx->orig_frame);

    __DestructExceptionObject(rec);
    return ExceptionContinueSearch;
}

static void check_noexcept( PEXCEPTION_RECORD rec,
        const cxx_function_descr *descr, BOOL nested )
{
    if (!nested && rec->ExceptionCode == CXX_EXCEPTION &&
            descr->magic >= CXX_FRAME_MAGIC_VC8 &&
            (descr->flags & FUNC_DESCR_NOEXCEPT))
    {
        ERR("noexcept function propagating exception\n");
        terminate();
    }
}

static DWORD cxx_frame_handler(EXCEPTION_RECORD *rec, ULONG64 frame,
                               CONTEXT *context, DISPATCHER_CONTEXT *dispatch,
                               const cxx_function_descr *descr)
{
    int trylevel = ip_to_state( descr, dispatch->ControlPc, dispatch->ImageBase );
    cxx_exception_type *exc_type;
    ULONG64 orig_frame = frame;
    ULONG64 throw_base;
    DWORD throw_func_off;
    void *throw_func;
    UINT i, j;
    int unwindlevel = -1;

    if (descr->magic<CXX_FRAME_MAGIC_VC6 || descr->magic>CXX_FRAME_MAGIC_VC8)
    {
        FIXME("unhandled frame magic %x\n", descr->magic);
        return ExceptionContinueSearch;
    }

    if (descr->magic >= CXX_FRAME_MAGIC_VC8 &&
        (descr->flags & FUNC_DESCR_SYNCHRONOUS) &&
        (rec->ExceptionCode != CXX_EXCEPTION &&
        !cxx_is_consolidate(rec) &&
        rec->ExceptionCode != STATUS_LONGJUMP))
        return ExceptionContinueSearch;  /* handle only c++ exceptions */

    /* update orig_frame if it's a nested exception */
    throw_func_off = RtlLookupFunctionEntry(dispatch->ControlPc, &throw_base, NULL)->BeginAddress;
    throw_func = rtti_rva(throw_func_off, throw_base);
    TRACE("reconstructed handler pointer: %p\n", throw_func);
    for (i=descr->tryblock_count; i>0; i--)
    {
        const tryblock_info *tryblock = rtti_rva(descr->tryblock, dispatch->ImageBase);
        tryblock = &tryblock[i-1];

        if (trylevel>tryblock->end_level && trylevel<=tryblock->catch_level)
        {
            for (j=0; j<tryblock->catchblock_count; j++)
            {
                const catchblock_info *catchblock = rtti_rva(tryblock->catchblock, dispatch->ImageBase);
                catchblock = &catchblock[j];

                if (rtti_rva(catchblock->handler, dispatch->ImageBase) == throw_func)
                {
                    TRACE("nested exception detected\n");
                    unwindlevel = tryblock->end_level;
                    orig_frame = *(ULONG64*)(frame + catchblock->frame);
                    TRACE("setting orig_frame to %I64x\n", orig_frame);
                }
            }
        }
    }

    if (rec->ExceptionFlags & (EXCEPTION_UNWINDING|EXCEPTION_EXIT_UNWIND))
    {
        if (rec->ExceptionFlags & EXCEPTION_TARGET_UNWIND)
            cxx_local_unwind(orig_frame, dispatch, descr,
                cxx_is_consolidate(rec) ? rec->ExceptionInformation[3] : trylevel);
        else
            cxx_local_unwind(orig_frame, dispatch, descr, unwindlevel);
        return ExceptionContinueSearch;
    }
    if (!descr->tryblock_count)
    {
        check_noexcept(rec, descr, orig_frame != frame);
        return ExceptionContinueSearch;
    }

    if (rec->ExceptionCode == CXX_EXCEPTION &&
        (!rec->ExceptionInformation[1] && !rec->ExceptionInformation[2]))
    {
        TRACE("rethrow detected.\n");
        *rec = *msvcrt_get_thread_data()->exc_record;
    }
    if (rec->ExceptionCode == CXX_EXCEPTION)
    {
        exc_type = (cxx_exception_type *)rec->ExceptionInformation[2];

        if (TRACE_ON(seh))
        {
            TRACE("handling C++ exception rec %p frame %I64x descr %p\n", rec, frame,  descr);
            TRACE_EXCEPTION_TYPE(exc_type, rec->ExceptionInformation[3]);
            dump_function_descr(descr, dispatch->ImageBase);
        }
    }
    else
    {
        thread_data_t *data = msvcrt_get_thread_data();

        exc_type = NULL;
        TRACE("handling C exception code %lx rec %p frame %I64x descr %p\n",
                rec->ExceptionCode, rec, frame, descr);

        if (data->se_translator) {
            EXCEPTION_POINTERS except_ptrs;
            se_translator_ctx ctx;

            ctx.dest_frame = frame;
            ctx.orig_frame = orig_frame;
            ctx.seh_rec    = rec;
            ctx.dispatch   = dispatch;
            ctx.descr      = descr;
            __TRY
            {
                except_ptrs.ExceptionRecord = rec;
                except_ptrs.ContextRecord = context;
                data->se_translator(rec->ExceptionCode, &except_ptrs);
            }
            __EXCEPT_CTX(se_translation_filter, &ctx)
            {
            }
            __ENDTRY
        }
    }

    find_catch_block(rec, context, NULL, frame, dispatch, descr, exc_type, orig_frame);
    check_noexcept(rec, descr, orig_frame != frame);
    return ExceptionContinueSearch;
}

/*********************************************************************
 *		__CxxFrameHandler (MSVCRT.@)
 */
EXCEPTION_DISPOSITION CDECL __CxxFrameHandler( EXCEPTION_RECORD *rec, ULONG64 frame,
                                               CONTEXT *context, DISPATCHER_CONTEXT *dispatch )
{
    TRACE( "%p %I64x %p %p\n", rec, frame, context, dispatch );
    return cxx_frame_handler( rec, frame, context, dispatch,
                              rtti_rva(*(UINT *)dispatch->HandlerData, dispatch->ImageBase) );
}


/*******************************************************************
 *		longjmp (MSVCRT.@)
 */
#ifndef __WINE_PE_BUILD
void __cdecl longjmp( _JUMP_BUFFER *jmp, int retval )
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
    __wine_longjmp( (__wine_jmp_buf *)jmp, retval );
}
#endif

/*******************************************************************
 *		_local_unwind (MSVCRT.@)
 */
void __cdecl _local_unwind( void *frame, void *target )
{
    RtlUnwind( frame, target, NULL, 0 );
}

/*********************************************************************
 *              handle_fpieee_flt
 */
int handle_fpieee_flt( __msvcrt_ulong exception_code, EXCEPTION_POINTERS *ep,
                       int (__cdecl *handler)(_FPIEEE_RECORD*) )
{
    FIXME("(%lx %p %p) opcode: %#I64x\n", exception_code, ep, handler,
            *(ULONG64*)ep->ContextRecord->Rip);
    return EXCEPTION_CONTINUE_SEARCH;
}

#if _MSVCR_VER>=110 && _MSVCR_VER<=120
/*********************************************************************
 *  __crtCapturePreviousContext (MSVCR110.@)
 */
void __cdecl get_prev_context(CONTEXT *ctx, DWORD64 rip)
{
    ULONG64 frame, image_base;
    RUNTIME_FUNCTION *rf;
    void *data;

    TRACE("(%p)\n", ctx);

    rf = RtlLookupFunctionEntry(ctx->Rip, &image_base, NULL);
    if(!rf) {
        FIXME("RtlLookupFunctionEntry failed\n");
        return;
    }

    RtlVirtualUnwind(UNW_FLAG_NHANDLER, image_base, ctx->Rip,
            rf, ctx, &data, &frame, NULL);
}

__ASM_GLOBAL_FUNC( __crtCapturePreviousContext,
                   "movq %rcx,8(%rsp)\n\t"
                   "call " __ASM_NAME("RtlCaptureContext") "\n\t"
                   "movq 8(%rsp),%rcx\n\t"     /* context */
                   "leaq 8(%rsp),%rax\n\t"
                   "movq %rax,0x98(%rcx)\n\t"  /* context->Rsp */
                   "movq (%rsp),%rax\n\t"
                   "movq %rax,0xf8(%rcx)\n\t"  /* context->Rip */
                   "jmp " __ASM_NAME("get_prev_context") )
#endif

#endif  /* __x86_64__ */
