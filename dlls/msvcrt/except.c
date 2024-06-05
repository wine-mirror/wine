/*
 * msvcrt.dll exception handling
 *
 * Copyright 2000 Jon Griffiths
 * Copyright 2005 Juan Lang
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
 * FIXME: Incomplete support for nested exceptions/try block cleanup.
 */

#include <float.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "msvcrt.h"
#include "excpt.h"
#include "wincon.h"
#include "wine/debug.h"

#include "cppexcept.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

#if _MSVCR_VER>=70 && _MSVCR_VER<=71
static MSVCRT_security_error_handler security_error_handler;
#endif

static __sighandler_t sighandlers[NSIG] = { SIG_DFL };

void dump_function_descr( const cxx_function_descr *descr, uintptr_t base )
{
    unwind_info *unwind_table = rtti_rva( descr->unwind_table, base );
    tryblock_info *tryblock = rtti_rva( descr->tryblock, base );
    ipmap_info *ipmap = rtti_rva( descr->ipmap, base );
    UINT i, j;

    TRACE( "magic %x\n", descr->magic );
    TRACE( "unwind table: %p %d\n", unwind_table, descr->unwind_count );
    for (i = 0; i < descr->unwind_count; i++)
    {
        TRACE("    %d: prev %d func %p\n", i, unwind_table[i].prev,
              unwind_table[i].handler ? rtti_rva( unwind_table[i].handler, base ) : NULL );
    }
    TRACE( "try table: %p %d\n", tryblock, descr->tryblock_count );
    for (i = 0; i < descr->tryblock_count; i++)
    {
        catchblock_info *catchblock = rtti_rva( tryblock[i].catchblock, base );

        TRACE( "    %d: start %d end %d catchlevel %d catch %p %d\n", i,
               tryblock[i].start_level, tryblock[i].end_level,
               tryblock[i].catch_level, catchblock, tryblock[i].catchblock_count);
        for (j = 0; j < tryblock[i].catchblock_count; j++)
        {
            type_info *type_info = catchblock[j].type_info ? rtti_rva( catchblock[j].type_info, base ) : NULL;
            TRACE( "        %d: flags %x offset %d handler %p",
                   j, catchblock[j].flags, catchblock[j].offset,
                   catchblock[j].handler ? rtti_rva(catchblock[j].handler, base) : NULL );
#ifdef _WIN64
            TRACE( " frame %x", catchblock[j].frame );
#endif
            TRACE( " type %p %s\n", type_info, dbgstr_type_info(type_info) );
        }
    }
    TRACE( "ipmap: %p %d\n", ipmap, descr->ipmap_count );
    for (i = 0; i < descr->ipmap_count; i++)
        TRACE( "    %d: ip %x state %d\n", i, ipmap[i].ip, ipmap[i].state );
#ifdef RTTI_USE_RVA
    TRACE( "unwind_help %+d\n", descr->unwind_help );
#endif
    if (descr->magic <= CXX_FRAME_MAGIC_VC6) return;
    TRACE( "expect list: %p\n", rtti_rva( descr->expect_list, base ) );
    if (descr->magic <= CXX_FRAME_MAGIC_VC7) return;
    TRACE( "flags: %08x\n", descr->flags );
}

void *find_catch_handler( void *object, uintptr_t frame, uintptr_t exc_base,
                          const tryblock_info *tryblock,
                          cxx_exception_type *exc_type, uintptr_t image_base )
{
    unsigned int i;
    const catchblock_info *catchblock = rtti_rva( tryblock->catchblock, image_base );
    const cxx_type_info *type;
    const type_info *catch_ti;

    for (i = 0; i < tryblock->catchblock_count; i++)
    {
        if (exc_type)
        {
            catch_ti = catchblock[i].type_info ? rtti_rva( catchblock[i].type_info, image_base ) : NULL;
            type = find_caught_type( exc_type, exc_base, catch_ti, catchblock[i].flags );
            if (!type) continue;

            TRACE( "matched type %p in catchblock %d\n", type, i );

            if (catch_ti && catch_ti->mangled[0] && catchblock[i].offset)
            {
                /* copy the exception to its destination on the stack */
                void **dest = (void **)(frame + catchblock[i].offset);
                copy_exception( object, dest, catchblock[i].flags, type, exc_base );
            }
        }
        else
        {
            /* no CXX_EXCEPTION only proceed with a catch(...) block*/
            if (catchblock[i].type_info) continue;
            TRACE( "found catch(...) block\n" );
        }
        return rtti_rva( catchblock[i].handler, image_base );
    }
    return NULL;
}

static BOOL WINAPI msvcrt_console_handler(DWORD ctrlType)
{
    BOOL ret = FALSE;

    switch (ctrlType)
    {
    case CTRL_C_EVENT:
        if (sighandlers[SIGINT])
        {
            if (sighandlers[SIGINT] != SIG_IGN)
                sighandlers[SIGINT](SIGINT);
            ret = TRUE;
        }
        break;
    }
    return ret;
}

/*********************************************************************
 *              __pxcptinfoptrs (MSVCRT.@)
 */
void** CDECL __pxcptinfoptrs(void)
{
    return (void**)&msvcrt_get_thread_data()->xcptinfo;
}

typedef void (CDECL *float_handler)(int, int);

/* The exception codes are actually NTSTATUS values */
static const struct
{
    NTSTATUS status;
    int signal;
} float_exception_map[] = {
 { EXCEPTION_FLT_DENORMAL_OPERAND, _FPE_DENORMAL },
 { EXCEPTION_FLT_DIVIDE_BY_ZERO, _FPE_ZERODIVIDE },
 { EXCEPTION_FLT_INEXACT_RESULT, _FPE_INEXACT },
 { EXCEPTION_FLT_INVALID_OPERATION, _FPE_INVALID },
 { EXCEPTION_FLT_OVERFLOW, _FPE_OVERFLOW },
 { EXCEPTION_FLT_STACK_CHECK, _FPE_STACKOVERFLOW },
 { EXCEPTION_FLT_UNDERFLOW, _FPE_UNDERFLOW },
};

static LONG msvcrt_exception_filter(struct _EXCEPTION_POINTERS *except)
{
    LONG ret = EXCEPTION_CONTINUE_SEARCH;
    __sighandler_t handler;

    if (!except || !except->ExceptionRecord)
        return EXCEPTION_CONTINUE_SEARCH;

    switch (except->ExceptionRecord->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        if ((handler = sighandlers[SIGSEGV]) != SIG_DFL)
        {
            if (handler != SIG_IGN)
            {
                EXCEPTION_POINTERS **ep = (EXCEPTION_POINTERS**)__pxcptinfoptrs(), *old_ep;

                old_ep = *ep;
                *ep = except;
                sighandlers[SIGSEGV] = SIG_DFL;
                handler(SIGSEGV);
                *ep = old_ep;
            }
            ret = EXCEPTION_CONTINUE_EXECUTION;
        }
        break;
    /* According to msdn,
     * the FPE signal handler takes as a second argument the type of
     * floating point exception.
     */
    case EXCEPTION_FLT_DENORMAL_OPERAND:
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_FLT_INEXACT_RESULT:
    case EXCEPTION_FLT_INVALID_OPERATION:
    case EXCEPTION_FLT_OVERFLOW:
    case EXCEPTION_FLT_STACK_CHECK:
    case EXCEPTION_FLT_UNDERFLOW:
        if ((handler = sighandlers[SIGFPE]) != SIG_DFL)
        {
            if (handler != SIG_IGN)
            {
                EXCEPTION_POINTERS **ep = (EXCEPTION_POINTERS**)__pxcptinfoptrs(), *old_ep;
                unsigned int i;
                int float_signal = _FPE_INVALID;

                sighandlers[SIGFPE] = SIG_DFL;
                for (i = 0; i < ARRAY_SIZE(float_exception_map); i++)
                {
                    if (float_exception_map[i].status ==
                        except->ExceptionRecord->ExceptionCode)
                    {
                        float_signal = float_exception_map[i].signal;
                        break;
                    }
                }

                old_ep = *ep;
                *ep = except;
                ((float_handler)handler)(SIGFPE, float_signal);
                *ep = old_ep;
            }
            ret = EXCEPTION_CONTINUE_EXECUTION;
        }
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
    case EXCEPTION_PRIV_INSTRUCTION:
        if ((handler = sighandlers[SIGILL]) != SIG_DFL)
        {
            if (handler != SIG_IGN)
            {
                EXCEPTION_POINTERS **ep = (EXCEPTION_POINTERS**)__pxcptinfoptrs(), *old_ep;

                old_ep = *ep;
                *ep = except;
                sighandlers[SIGILL] = SIG_DFL;
                handler(SIGILL);
                *ep = old_ep;
            }
            ret = EXCEPTION_CONTINUE_EXECUTION;
        }
        break;
    }
    return ret;
}

void msvcrt_init_signals(void)
{
    SetConsoleCtrlHandler(msvcrt_console_handler, TRUE);
}

void msvcrt_free_signals(void)
{
    SetConsoleCtrlHandler(msvcrt_console_handler, FALSE);
}

/*********************************************************************
 *		signal (MSVCRT.@)
 * Some signals may never be generated except through an explicit call to
 * raise.
 */
__sighandler_t CDECL signal(int sig, __sighandler_t func)
{
    __sighandler_t ret = SIG_ERR;

    TRACE("(%d, %p)\n", sig, func);

    if (func == SIG_ERR) return SIG_ERR;

    switch (sig)
    {
    /* Cases handled internally.  Note SIGTERM is never generated by Windows,
     * so we effectively mask it.
     */
    case SIGABRT:
    case SIGFPE:
    case SIGILL:
    case SIGSEGV:
    case SIGINT:
    case SIGTERM:
    case SIGBREAK:
        ret = sighandlers[sig];
        sighandlers[sig] = func;
        break;
    default:
        ret = SIG_ERR;
    }
    return ret;
}

/*********************************************************************
 *		raise (MSVCRT.@)
 */
int CDECL raise(int sig)
{
    __sighandler_t handler;

    TRACE("(%d)\n", sig);

    switch (sig)
    {
    case SIGFPE:
    case SIGILL:
    case SIGSEGV:
        handler = sighandlers[sig];
        if (handler == SIG_DFL) _exit(3);
        if (handler != SIG_IGN)
        {
            EXCEPTION_POINTERS **ep = (EXCEPTION_POINTERS**)__pxcptinfoptrs(), *old_ep;

            sighandlers[sig] = SIG_DFL;

            old_ep = *ep;
            *ep = NULL;
            if (sig == SIGFPE)
                ((float_handler)handler)(sig, _FPE_EXPLICITGEN);
            else
                handler(sig);
            *ep = old_ep;
        }
        break;
    case SIGABRT:
    case SIGINT:
    case SIGTERM:
    case SIGBREAK:
        handler = sighandlers[sig];
        if (handler == SIG_DFL) _exit(3);
        if (handler != SIG_IGN)
        {
            sighandlers[sig] = SIG_DFL;
            handler(sig);
        }
        break;
    default:
        return -1;
    }
    return 0;
}

/*********************************************************************
 *		_XcptFilter (MSVCRT.@)
 */
int CDECL _XcptFilter(NTSTATUS ex, PEXCEPTION_POINTERS ptr)
{
    TRACE("(%08lx,%p)\n", ex, ptr);
    /* I assume ptr->ExceptionRecord->ExceptionCode is the same as ex */
    return msvcrt_exception_filter(ptr);
}

/*********************************************************************
 *		__CppXcptFilter (MSVCRT.@)
 */
int CDECL __CppXcptFilter(NTSTATUS ex, PEXCEPTION_POINTERS ptr)
{
    /* only filter c++ exceptions */
    if (ex != CXX_EXCEPTION) return EXCEPTION_CONTINUE_SEARCH;
    return _XcptFilter(ex, ptr);
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
    if (is_cxx_exception( rec ) && rec->ExceptionInformation[2])
    {
        ptrs->ExceptionRecord = msvcrt_get_thread_data()->exc_record;
        return TRUE;
    }
    return (msvcrt_get_thread_data()->exc_record == rec);
}

/*********************************************************************
 *		__CxxExceptionFilter (MSVCRT.@)
 */
int CDECL __CxxExceptionFilter( EXCEPTION_POINTERS *ptrs, const type_info *ti, UINT flags, void **copy)
{
    const cxx_type_info *type;
    EXCEPTION_RECORD *rec;
    uintptr_t exc_base;

    TRACE( "%p %p %x %p\n", ptrs, ti, flags, copy );

    if (!ptrs) return EXCEPTION_CONTINUE_SEARCH;

    /* handle catch(...) */
    if (!ti) return EXCEPTION_EXECUTE_HANDLER;

    rec = ptrs->ExceptionRecord;
    if (!is_cxx_exception( rec )) return EXCEPTION_CONTINUE_SEARCH;

    if (rec->ExceptionInformation[1] == 0 && rec->ExceptionInformation[2] == 0)
    {
        rec = msvcrt_get_thread_data()->exc_record;
        if (!rec) return EXCEPTION_CONTINUE_SEARCH;
    }

    exc_base = rec->ExceptionInformation[3];
    type = find_caught_type( (cxx_exception_type *)rec->ExceptionInformation[2], exc_base, ti, flags );
    if (!type) return EXCEPTION_CONTINUE_SEARCH;

    if (copy) copy_exception( (void *)rec->ExceptionInformation[1], copy, flags, type, exc_base );

    return EXCEPTION_EXECUTE_HANDLER;
}

/*********************************************************************
 *		__CxxQueryExceptionSize (MSVCRT.@)
 */
unsigned int CDECL __CxxQueryExceptionSize(void)
{
    return sizeof(cxx_exception_type);
}

/*********************************************************************
 *		_abnormal_termination (MSVCRT.@)
 */
int CDECL __intrinsic_abnormal_termination(void)
{
  FIXME("(void)stub\n");
  return 0;
}

/******************************************************************
 *		__uncaught_exception (MSVCRT.@)
 */
BOOL CDECL __uncaught_exception(void)
{
    return msvcrt_get_thread_data()->processing_throw != 0;
}

/*********************************************************************
 *              _fpieee_flt (MSVCRT.@)
 */
int __cdecl _fpieee_flt( __msvcrt_ulong code, EXCEPTION_POINTERS *ep,
                         int (__cdecl *handler)(_FPIEEE_RECORD*) )
{
    switch (code)
    {
    case STATUS_FLOAT_DIVIDE_BY_ZERO:
    case STATUS_FLOAT_INEXACT_RESULT:
    case STATUS_FLOAT_INVALID_OPERATION:
    case STATUS_FLOAT_OVERFLOW:
    case STATUS_FLOAT_UNDERFLOW:
        return handle_fpieee_flt( code, ep, handler );
    default:
        return EXCEPTION_CONTINUE_SEARCH;
    }
}

#if _MSVCR_VER>=70 && _MSVCR_VER<=71

/*********************************************************************
 *		_set_security_error_handler (MSVCR70.@)
 */
MSVCRT_security_error_handler CDECL _set_security_error_handler(
    MSVCRT_security_error_handler handler )
{
    MSVCRT_security_error_handler old = security_error_handler;

    TRACE("(%p)\n", handler);

    security_error_handler = handler;
    return old;
}

/*********************************************************************
 *		__security_error_handler (MSVCR70.@)
 */
void CDECL __security_error_handler(int code, void *data)
{
    if(security_error_handler)
        security_error_handler(code, data);
    else
        FIXME("(%d, %p) stub\n", code, data);

    _exit(3);
}

#endif /* _MSVCR_VER>=70 && _MSVCR_VER<=71 */

#if _MSVCR_VER>=110
/*********************************************************************
 *  __crtSetUnhandledExceptionFilter (MSVCR110.@)
 */
LPTOP_LEVEL_EXCEPTION_FILTER CDECL __crtSetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER filter)
{
    return SetUnhandledExceptionFilter(filter);
}
#endif

/*********************************************************************
 * _CreateFrameInfo (MSVCR80.@)
 */
frame_info* CDECL _CreateFrameInfo(frame_info *fi, void *obj)
{
    thread_data_t *data = msvcrt_get_thread_data();

    TRACE("(%p, %p)\n", fi, obj);

    fi->next = data->frame_info_head;
    data->frame_info_head = fi;
    fi->object = obj;
    return fi;
}

/*********************************************************************
 * _FindAndUnlinkFrame (MSVCR80.@)
 */
void CDECL _FindAndUnlinkFrame(frame_info *fi)
{
    thread_data_t *data = msvcrt_get_thread_data();
    frame_info *cur = data->frame_info_head;

    TRACE("(%p)\n", fi);

    if (cur == fi)
    {
        data->frame_info_head = cur->next;
        return;
    }

    for (; cur->next; cur = cur->next)
    {
        if (cur->next == fi)
        {
            cur->next = fi->next;
            return;
        }
    }

    ERR("frame not found, native crashes in this case\n");
}

/*********************************************************************
 *              _IsExceptionObjectToBeDestroyed (MSVCR80.@)
 */
BOOL __cdecl _IsExceptionObjectToBeDestroyed(const void *obj)
{
    frame_info *cur;

    TRACE( "%p\n", obj );

    for (cur = msvcrt_get_thread_data()->frame_info_head; cur; cur = cur->next)
    {
        if (cur->object == obj)
            return FALSE;
    }

    return TRUE;
}

/*********************************************************************
 * __DestructExceptionObject (MSVCRT.@)
 */
void CDECL __DestructExceptionObject(EXCEPTION_RECORD *rec)
{
    cxx_exception_type *info = (cxx_exception_type*) rec->ExceptionInformation[2];
    void *object = (void*)rec->ExceptionInformation[1];

    TRACE("(%p)\n", rec);

    if (!is_cxx_exception( rec )) return;

    if (!info || !info->destructor)
        return;

    call_dtor( rtti_rva( info->destructor, rec->ExceptionInformation[3] ), object );
}

/*********************************************************************
 *  __CxxRegisterExceptionObject (MSVCRT.@)
 */
BOOL CDECL __CxxRegisterExceptionObject(EXCEPTION_POINTERS *ep, cxx_frame_info *frame_info)
{
    thread_data_t *data = msvcrt_get_thread_data();

    TRACE("(%p, %p)\n", ep, frame_info);

    if (!ep || !ep->ExceptionRecord)
    {
        frame_info->rec = (void*)-1;
        frame_info->context = (void*)-1;
        return TRUE;
    }

    frame_info->rec = data->exc_record;
    frame_info->context = data->ctx_record;
    data->exc_record = ep->ExceptionRecord;
    data->ctx_record = ep->ContextRecord;
    _CreateFrameInfo(&frame_info->frame_info, (void*)ep->ExceptionRecord->ExceptionInformation[1]);
    return TRUE;
}

/*********************************************************************
 *  __CxxUnregisterExceptionObject (MSVCRT.@)
 */
void CDECL __CxxUnregisterExceptionObject(cxx_frame_info *frame_info, BOOL in_use)
{
    thread_data_t *data = msvcrt_get_thread_data();

    TRACE("(%p)\n", frame_info);

    if(frame_info->rec == (void*)-1)
        return;

    _FindAndUnlinkFrame(&frame_info->frame_info);
    if(data->exc_record->ExceptionCode == CXX_EXCEPTION && !in_use
            && _IsExceptionObjectToBeDestroyed((void*)data->exc_record->ExceptionInformation[1]))
        __DestructExceptionObject(data->exc_record);
    data->exc_record = frame_info->rec;
    data->ctx_record = frame_info->context;
}

struct __std_exception_data {
    char *what;
    char dofree;
};

#if _MSVCR_VER>=140

/*********************************************************************
 *  __std_exception_copy (UCRTBASE.@)
 */
void CDECL __std_exception_copy(const struct __std_exception_data *src,
                                       struct __std_exception_data *dst)
{
    TRACE("(%p %p)\n", src, dst);

    if(src->dofree && src->what) {
        dst->what   = _strdup(src->what);
        dst->dofree = 1;
    } else {
        dst->what   = src->what;
        dst->dofree = 0;
    }
}

/*********************************************************************
 *  __std_exception_destroy (UCRTBASE.@)
 */
void CDECL __std_exception_destroy(struct __std_exception_data *data)
{
    TRACE("(%p)\n", data);

    if(data->dofree)
        free(data->what);
    data->what   = NULL;
    data->dofree = 0;
}

/*********************************************************************
 *  __current_exception (UCRTBASE.@)
 */
void** CDECL __current_exception(void)
{
    TRACE("()\n");
    return (void**)&msvcrt_get_thread_data()->exc_record;
}

/*********************************************************************
 *  __current_exception_context (UCRTBASE.@)
 */
void** CDECL __current_exception_context(void)
{
    TRACE("()\n");
    return (void**)&msvcrt_get_thread_data()->ctx_record;
}

/*********************************************************************
 *  __processing_throw (UCRTBASE.@)
 */
int* CDECL __processing_throw(void)
{
    TRACE("()\n");
    return &msvcrt_get_thread_data()->processing_throw;
}

#endif /* _MSVCR_VER>=140 */
