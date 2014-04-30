/*
 * msvcrt C++ exception handling
 *
 * Copyright 2000 Jon Griffiths
 * Copyright 2002 Alexandre Julliard
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
 * NOTES
 * A good reference is the article "How a C++ compiler implements
 * exception handling" by Vishal Kochhar, available on
 * www.thecodeproject.com.
 */

#include "config.h"
#include "wine/port.h"

#ifdef __i386__

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "msvcrt.h"
#include "wine/exception.h"
#include "excpt.h"
#include "wine/debug.h"

#include "cppexcept.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);


/* the exception frame used by CxxFrameHandler */
typedef struct __cxx_exception_frame
{
    EXCEPTION_REGISTRATION_RECORD  frame;    /* the standard exception frame */
    int                            trylevel;
    DWORD                          ebp;
} cxx_exception_frame;

/* info about a single catch {} block */
typedef struct __catchblock_info
{
    UINT             flags;         /* flags (see below) */
    const type_info *type_info;     /* C++ type caught by this block */
    int              offset;        /* stack offset to copy exception object to */
    void           (*handler)(void);/* catch block handler code */
} catchblock_info;
#define TYPE_FLAG_CONST      1
#define TYPE_FLAG_VOLATILE   2
#define TYPE_FLAG_REFERENCE  8

/* info about a single try {} block */
typedef struct __tryblock_info
{
    int                    start_level;      /* start trylevel of that block */
    int                    end_level;        /* end trylevel of that block */
    int                    catch_level;      /* initial trylevel of the catch block */
    int                    catchblock_count; /* count of catch blocks in array */
    const catchblock_info *catchblock;       /* array of catch blocks */
} tryblock_info;

/* info about the unwind handler for a given trylevel */
typedef struct __unwind_info
{
    int    prev;          /* prev trylevel unwind handler, to run after this one */
    void (*handler)(void);/* unwind handler */
} unwind_info;

/* descriptor of all try blocks of a given function */
typedef struct __cxx_function_descr
{
    UINT                 magic;          /* must be CXX_FRAME_MAGIC */
    UINT                 unwind_count;   /* number of unwind handlers */
    const unwind_info   *unwind_table;   /* array of unwind handlers */
    UINT                 tryblock_count; /* number of try blocks */
    const tryblock_info *tryblock;       /* array of try blocks */
    UINT                 ipmap_count;
    const void          *ipmap;
    const void          *expect_list;    /* expected exceptions list when magic >= VC7 */
    UINT                 flags;          /* flags when magic >= VC8 */
} cxx_function_descr;

#define FUNC_DESCR_SYNCHRONOUS  1        /* synchronous exceptions only (built with /EHs) */

typedef struct _SCOPETABLE
{
  int previousTryLevel;
  int (*lpfnFilter)(PEXCEPTION_POINTERS);
  int (*lpfnHandler)(void);
} SCOPETABLE, *PSCOPETABLE;

typedef struct _MSVCRT_EXCEPTION_FRAME
{
  EXCEPTION_REGISTRATION_RECORD *prev;
  void (*handler)(PEXCEPTION_RECORD, EXCEPTION_REGISTRATION_RECORD*,
                  PCONTEXT, PEXCEPTION_RECORD);
  PSCOPETABLE scopetable;
  int trylevel;
  int _ebp;
  PEXCEPTION_POINTERS xpointers;
} MSVCRT_EXCEPTION_FRAME;

typedef struct
{
  int   gs_cookie_offset;
  ULONG gs_cookie_xor;
  int   eh_cookie_offset;
  ULONG eh_cookie_xor;
  SCOPETABLE entries[1];
} SCOPETABLE_V4;

#define TRYLEVEL_END (-1) /* End of trylevel list */

DWORD CDECL cxx_frame_handler( PEXCEPTION_RECORD rec, cxx_exception_frame* frame,
                               PCONTEXT context, EXCEPTION_REGISTRATION_RECORD** dispatch,
                               const cxx_function_descr *descr,
                               EXCEPTION_REGISTRATION_RECORD* nested_frame, int nested_trylevel );

/* call a function with a given ebp */
static inline void *call_ebp_func( void *func, void *ebp )
{
    void *ret;
    int dummy;
    __asm__ __volatile__ ("pushl %%ebx\n\t"
                          "pushl %%ebp\n\t"
                          "movl %4,%%ebp\n\t"
                          "call *%%eax\n\t"
                          "popl %%ebp\n\t"
                          "popl %%ebx"
                          : "=a" (ret), "=S" (dummy), "=D" (dummy)
                          : "0" (func), "1" (ebp) : "ecx", "edx", "memory" );
    return ret;
}

/* call a copy constructor */
static inline void call_copy_ctor( void *func, void *this, void *src, int has_vbase )
{
    TRACE( "calling copy ctor %p object %p src %p\n", func, this, src );
    if (has_vbase)
        /* in that case copy ctor takes an extra bool indicating whether to copy the base class */
        __asm__ __volatile__("pushl $1; pushl %2; call *%0"
                             : : "r" (func), "c" (this), "r" (src) : "eax", "edx", "memory" );
    else
        __asm__ __volatile__("pushl %2; call *%0"
                             : : "r" (func), "c" (this), "r" (src) : "eax", "edx", "memory" );
}

/* call the destructor of the exception object */
static inline void call_dtor( void *func, void *object )
{
    __asm__ __volatile__("call *%0" : : "r" (func), "c" (object) : "eax", "edx", "memory" );
}

/* continue execution to the specified address after exception is caught */
static inline void DECLSPEC_NORETURN continue_after_catch( cxx_exception_frame* frame, void *addr )
{
    __asm__ __volatile__("movl -4(%0),%%esp; leal 12(%0),%%ebp; jmp *%1"
                         : : "r" (frame), "a" (addr) );
    for (;;) ; /* unreached */
}

static inline void call_finally_block( void *code_block, void *base_ptr )
{
    __asm__ __volatile__ ("movl %1,%%ebp; call *%%eax"
                          : : "a" (code_block), "g" (base_ptr));
}

static inline int call_filter( int (*func)(PEXCEPTION_POINTERS), void *arg, void *ebp )
{
    int ret;
    __asm__ __volatile__ ("pushl %%ebp; pushl %3; movl %2,%%ebp; call *%%eax; popl %%ebp; popl %%ebp"
                          : "=a" (ret)
                          : "0" (func), "r" (ebp), "r" (arg)
                          : "ecx", "edx", "memory" );
    return ret;
}

static inline int call_unwind_func( int (*func)(void), void *ebp )
{
    int ret;
    __asm__ __volatile__ ("pushl %%ebp\n\t"
                          "pushl %%ebx\n\t"
                          "pushl %%esi\n\t"
                          "pushl %%edi\n\t"
                          "movl %2,%%ebp\n\t"
                          "call *%0\n\t"
                          "popl %%edi\n\t"
                          "popl %%esi\n\t"
                          "popl %%ebx\n\t"
                          "popl %%ebp"
                          : "=a" (ret)
                          : "0" (func), "r" (ebp)
                          : "ecx", "edx", "memory" );
    return ret;
}

static inline void dump_type( const cxx_type_info *type )
{
    TRACE( "flags %x type %p %s offsets %d,%d,%d size %d copy ctor %p\n",
             type->flags, type->type_info, dbgstr_type_info(type->type_info),
             type->offsets.this_offset, type->offsets.vbase_descr, type->offsets.vbase_offset,
             type->size, type->copy_ctor );
}

static void dump_exception_type( const cxx_exception_type *type )
{
    UINT i;

    TRACE( "flags %x destr %p handler %p type info %p\n",
             type->flags, type->destructor, type->custom_handler, type->type_info_table );
    for (i = 0; i < type->type_info_table->count; i++)
    {
        TRACE( "    %d: ", i );
        dump_type( type->type_info_table->info[i] );
    }
}

static void dump_function_descr( const cxx_function_descr *descr )
{
    UINT i;
    int j;

    TRACE( "magic %x\n", descr->magic );
    TRACE( "unwind table: %p %d\n", descr->unwind_table, descr->unwind_count );
    for (i = 0; i < descr->unwind_count; i++)
    {
        TRACE( "    %d: prev %d func %p\n", i,
                 descr->unwind_table[i].prev, descr->unwind_table[i].handler );
    }
    TRACE( "try table: %p %d\n", descr->tryblock, descr->tryblock_count );
    for (i = 0; i < descr->tryblock_count; i++)
    {
        TRACE( "    %d: start %d end %d catchlevel %d catch %p %d\n", i,
                 descr->tryblock[i].start_level, descr->tryblock[i].end_level,
                 descr->tryblock[i].catch_level, descr->tryblock[i].catchblock,
                 descr->tryblock[i].catchblock_count );
        for (j = 0; j < descr->tryblock[i].catchblock_count; j++)
        {
            const catchblock_info *ptr = &descr->tryblock[i].catchblock[j];
            TRACE( "        %d: flags %x offset %d handler %p type %p %s\n",
                     j, ptr->flags, ptr->offset, ptr->handler,
                     ptr->type_info, dbgstr_type_info( ptr->type_info ) );
        }
    }
    if (descr->magic <= CXX_FRAME_MAGIC_VC6) return;
    TRACE( "expect list: %p\n", descr->expect_list );
    if (descr->magic <= CXX_FRAME_MAGIC_VC7) return;
    TRACE( "flags: %08x\n", descr->flags );
}

/* check if the exception type is caught by a given catch block, and return the type that matched */
static const cxx_type_info *find_caught_type( cxx_exception_type *exc_type,
                                              const catchblock_info *catchblock )
{
    UINT i;

    for (i = 0; i < exc_type->type_info_table->count; i++)
    {
        const cxx_type_info *type = exc_type->type_info_table->info[i];

        if (!catchblock->type_info) return type;   /* catch(...) matches any type */
        if (catchblock->type_info != type->type_info)
        {
            if (strcmp( catchblock->type_info->mangled, type->type_info->mangled )) continue;
        }
        /* type is the same, now check the flags */
        if ((exc_type->flags & TYPE_FLAG_CONST) &&
            !(catchblock->flags & TYPE_FLAG_CONST)) continue;
        if ((exc_type->flags & TYPE_FLAG_VOLATILE) &&
            !(catchblock->flags & TYPE_FLAG_VOLATILE)) continue;
        return type;  /* it matched */
    }
    return NULL;
}


/* copy the exception object where the catch block wants it */
static void copy_exception( void *object, cxx_exception_frame *frame,
                            const catchblock_info *catchblock, const cxx_type_info *type )
{
    void **dest_ptr;

    if (!catchblock->type_info || !catchblock->type_info->mangled[0]) return;
    if (!catchblock->offset) return;
    dest_ptr = (void **)((char *)&frame->ebp + catchblock->offset);

    if (catchblock->flags & TYPE_FLAG_REFERENCE)
    {
        *dest_ptr = get_this_pointer( &type->offsets, object );
    }
    else if (type->flags & CLASS_IS_SIMPLE_TYPE)
    {
        memmove( dest_ptr, object, type->size );
        /* if it is a pointer, adjust it */
        if (type->size == sizeof(void *)) *dest_ptr = get_this_pointer( &type->offsets, *dest_ptr );
    }
    else  /* copy the object */
    {
        if (type->copy_ctor)
            call_copy_ctor( type->copy_ctor, dest_ptr, get_this_pointer(&type->offsets,object),
                            (type->flags & CLASS_HAS_VIRTUAL_BASE_CLASS) );
        else
            memmove( dest_ptr, get_this_pointer(&type->offsets,object), type->size );
    }
}

/* unwind the local function up to a given trylevel */
static void cxx_local_unwind( cxx_exception_frame* frame, const cxx_function_descr *descr, int last_level)
{
    void (*handler)(void);
    int trylevel = frame->trylevel;

    while (trylevel != last_level)
    {
        if (trylevel < 0 || trylevel >= descr->unwind_count)
        {
            ERR( "invalid trylevel %d\n", trylevel );
            MSVCRT_terminate();
        }
        handler = descr->unwind_table[trylevel].handler;
        if (handler)
        {
            TRACE( "calling unwind handler %p trylevel %d last %d ebp %p\n",
                   handler, trylevel, last_level, &frame->ebp );
            call_ebp_func( handler, &frame->ebp );
        }
        trylevel = descr->unwind_table[trylevel].prev;
    }
    frame->trylevel = last_level;
}

/* exception frame for nested exceptions in catch block */
struct catch_func_nested_frame
{
    EXCEPTION_REGISTRATION_RECORD frame;     /* standard exception frame */
    EXCEPTION_RECORD             *prev_rec;  /* previous record to restore in thread data */
    cxx_exception_frame          *cxx_frame; /* frame of parent exception */
    const cxx_function_descr     *descr;     /* descriptor of parent exception */
    int                           trylevel;  /* current try level */
    EXCEPTION_RECORD             *rec;       /* rec associated with frame */
};

/* handler for exceptions happening while calling a catch function */
static DWORD catch_function_nested_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                            CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    struct catch_func_nested_frame *nested_frame = (struct catch_func_nested_frame *)frame;

    if (rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
    {
        msvcrt_get_thread_data()->exc_record = nested_frame->prev_rec;
        return ExceptionContinueSearch;
    }

    TRACE( "got nested exception in catch function\n" );

    if(rec->ExceptionCode == CXX_EXCEPTION)
    {
        PEXCEPTION_RECORD prev_rec = nested_frame->rec;
        if((rec->ExceptionInformation[1] == 0 && rec->ExceptionInformation[2] == 0) ||
                (prev_rec->ExceptionCode == CXX_EXCEPTION &&
                 rec->ExceptionInformation[1] == prev_rec->ExceptionInformation[1] &&
                 rec->ExceptionInformation[2] == prev_rec->ExceptionInformation[2]))
        {
            /* exception was rethrown */
            *rec = *prev_rec;
            rec->ExceptionFlags &= ~EH_UNWINDING;
            if(TRACE_ON(seh)) {
                TRACE("detect rethrow: exception code: %x\n", rec->ExceptionCode);
                if(rec->ExceptionCode == CXX_EXCEPTION)
                    TRACE("re-propage: obj: %lx, type: %lx\n",
                            rec->ExceptionInformation[1], rec->ExceptionInformation[2]);
            }
        }
        else if (nested_frame->prev_rec && nested_frame->prev_rec->ExceptionCode == CXX_EXCEPTION &&
                nested_frame->prev_rec->ExceptionInformation[1] == prev_rec->ExceptionInformation[1] &&
                nested_frame->prev_rec->ExceptionInformation[2] == prev_rec->ExceptionInformation[2])
        {
            TRACE("detect threw new exception in catch block - not owning old(obj: %lx type: %lx)\n",
                    prev_rec->ExceptionInformation[1], prev_rec->ExceptionInformation[2]);
        }
        else if (prev_rec->ExceptionCode == CXX_EXCEPTION) {
            /* new exception in exception handler, destroy old */
            void *object = (void*)prev_rec->ExceptionInformation[1];
            cxx_exception_type *info = (cxx_exception_type*) prev_rec->ExceptionInformation[2];
            TRACE("detect threw new exception in catch block - destroy old(obj: %p type: %p)\n",
                    object, info);
            if(info && info->destructor)
                call_dtor( info->destructor, object );
        }
        else
        {
            TRACE("detect threw new exception in catch block\n");
        }
    }

    return cxx_frame_handler( rec, nested_frame->cxx_frame, context,
                              NULL, nested_frame->descr, &nested_frame->frame,
                              nested_frame->trylevel );
}

/* find and call the appropriate catch block for an exception */
/* returns the address to continue execution to after the catch block was called */
static inline void call_catch_block( PEXCEPTION_RECORD rec, cxx_exception_frame *frame,
                                     const cxx_function_descr *descr, int nested_trylevel,
                                     cxx_exception_type *info )
{
    UINT i;
    int j;
    void *addr, *object = (void *)rec->ExceptionInformation[1];
    struct catch_func_nested_frame nested_frame;
    int trylevel = frame->trylevel;
    thread_data_t *thread_data = msvcrt_get_thread_data();
    DWORD save_esp = ((DWORD*)frame)[-1];

    for (i = 0; i < descr->tryblock_count; i++)
    {
        const tryblock_info *tryblock = &descr->tryblock[i];

        if (trylevel < tryblock->start_level) continue;
        if (trylevel > tryblock->end_level) continue;

        /* got a try block */
        for (j = 0; j < tryblock->catchblock_count; j++)
        {
            const catchblock_info *catchblock = &tryblock->catchblock[j];
            if(info)
            {
                const cxx_type_info *type = find_caught_type( info, catchblock );
                if (!type) continue;

                TRACE( "matched type %p in tryblock %d catchblock %d\n", type, i, j );

                /* copy the exception to its destination on the stack */
                copy_exception( object, frame, catchblock, type );
            }
            else
            {
                /* no CXX_EXCEPTION only proceed with a catch(...) block*/
                if(catchblock->type_info)
                    continue;
                TRACE("found catch(...) block\n");
            }

            /* unwind the stack */
            RtlUnwind( frame, 0, rec, 0 );
            cxx_local_unwind( frame, descr, tryblock->start_level );
            frame->trylevel = tryblock->end_level + 1;

            /* call the catch block */
            TRACE( "calling catch block %p addr %p ebp %p\n",
                   catchblock, catchblock->handler, &frame->ebp );

            /* setup an exception block for nested exceptions */

            nested_frame.frame.Handler = catch_function_nested_handler;
            nested_frame.prev_rec  = thread_data->exc_record;
            nested_frame.cxx_frame = frame;
            nested_frame.descr     = descr;
            nested_frame.trylevel  = nested_trylevel + 1;
            nested_frame.rec = rec;

            __wine_push_frame( &nested_frame.frame );
            thread_data->exc_record = rec;
            addr = call_ebp_func( catchblock->handler, &frame->ebp );
            thread_data->exc_record = nested_frame.prev_rec;
            __wine_pop_frame( &nested_frame.frame );

            ((DWORD*)frame)[-1] = save_esp;
            if (info && info->destructor) call_dtor( info->destructor, object );
            TRACE( "done, continuing at %p\n", addr );

            continue_after_catch( frame, addr );
        }
    }
}


/*********************************************************************
 *		cxx_frame_handler
 *
 * Implementation of __CxxFrameHandler.
 */
DWORD CDECL cxx_frame_handler( PEXCEPTION_RECORD rec, cxx_exception_frame* frame,
                               PCONTEXT context, EXCEPTION_REGISTRATION_RECORD** dispatch,
                               const cxx_function_descr *descr,
                               EXCEPTION_REGISTRATION_RECORD* nested_frame,
                               int nested_trylevel )
{
    cxx_exception_type *exc_type;

    if (descr->magic < CXX_FRAME_MAGIC_VC6 || descr->magic > CXX_FRAME_MAGIC_VC8)
    {
        ERR( "invalid frame magic %x\n", descr->magic );
        return ExceptionContinueSearch;
    }
    if (descr->magic >= CXX_FRAME_MAGIC_VC8 &&
        (descr->flags & FUNC_DESCR_SYNCHRONOUS) &&
        (rec->ExceptionCode != CXX_EXCEPTION))
        return ExceptionContinueSearch;  /* handle only c++ exceptions */

    if (rec->ExceptionFlags & (EH_UNWINDING|EH_EXIT_UNWIND))
    {
        if (descr->unwind_count && !nested_trylevel) cxx_local_unwind( frame, descr, -1 );
        return ExceptionContinueSearch;
    }
    if (!descr->tryblock_count) return ExceptionContinueSearch;

    if(rec->ExceptionCode == CXX_EXCEPTION &&
            rec->ExceptionInformation[1] == 0 && rec->ExceptionInformation[2] == 0)
    {
        *rec = *msvcrt_get_thread_data()->exc_record;
        rec->ExceptionFlags &= ~EH_UNWINDING;
        if(TRACE_ON(seh)) {
            TRACE("detect rethrow: exception code: %x\n", rec->ExceptionCode);
            if(rec->ExceptionCode == CXX_EXCEPTION)
                TRACE("re-propage: obj: %lx, type: %lx\n",
                        rec->ExceptionInformation[1], rec->ExceptionInformation[2]);
        }
    }

    if(rec->ExceptionCode == CXX_EXCEPTION)
    {
        exc_type = (cxx_exception_type *)rec->ExceptionInformation[2];

        if (rec->ExceptionInformation[0] > CXX_FRAME_MAGIC_VC8 &&
                exc_type->custom_handler)
        {
            return exc_type->custom_handler( rec, frame, context, dispatch,
                                         descr, nested_trylevel, nested_frame, 0 );
        }

        if (TRACE_ON(seh))
        {
            TRACE("handling C++ exception rec %p frame %p trylevel %d descr %p nested_frame %p\n",
                  rec, frame, frame->trylevel, descr, nested_frame );
            dump_exception_type( exc_type );
            dump_function_descr( descr );
        }
    }
    else
    {
        thread_data_t *data = msvcrt_get_thread_data();

        exc_type = NULL;
        TRACE("handling C exception code %x  rec %p frame %p trylevel %d descr %p nested_frame %p\n",
              rec->ExceptionCode,  rec, frame, frame->trylevel, descr, nested_frame );

        if (data->se_translator) {
            EXCEPTION_POINTERS except_ptrs;

            except_ptrs.ExceptionRecord = rec;
            except_ptrs.ContextRecord = context;
            data->se_translator(rec->ExceptionCode, &except_ptrs);
        }
    }

    call_catch_block( rec, frame, descr, frame->trylevel, exc_type );
    return ExceptionContinueSearch;
}


/*********************************************************************
 *		__CxxFrameHandler (MSVCRT.@)
 */
extern DWORD CDECL __CxxFrameHandler( PEXCEPTION_RECORD rec, EXCEPTION_REGISTRATION_RECORD* frame,
                                      PCONTEXT context, EXCEPTION_REGISTRATION_RECORD** dispatch );
__ASM_GLOBAL_FUNC( __CxxFrameHandler,
                   "pushl $0\n\t"        /* nested_trylevel */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "pushl $0\n\t"        /* nested_frame */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "pushl %eax\n\t"      /* descr */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "pushl 28(%esp)\n\t"  /* dispatch */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "pushl 28(%esp)\n\t"  /* context */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "pushl 28(%esp)\n\t"  /* frame */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "pushl 28(%esp)\n\t"  /* rec */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "call " __ASM_NAME("cxx_frame_handler") "\n\t"
                   "add $28,%esp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -28\n\t")
                   "ret" )


/*********************************************************************
 *		__CxxLongjmpUnwind (MSVCRT.@)
 *
 * Callback meant to be used as UnwindFunc for setjmp/longjmp.
 */
void __stdcall __CxxLongjmpUnwind( const struct MSVCRT___JUMP_BUFFER *buf )
{
    cxx_exception_frame *frame = (cxx_exception_frame *)buf->Registration;
    const cxx_function_descr *descr = (const cxx_function_descr *)buf->UnwindData[0];

    TRACE( "unwinding frame %p descr %p trylevel %ld\n", frame, descr, buf->TryLevel );
    cxx_local_unwind( frame, descr, buf->TryLevel );
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
      rec->NumberParameters == 3 &&
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


/*********************************************************************
 *		_EH_prolog (MSVCRT.@)
 */

/* Provided for VC++ binary compatibility only */
__ASM_GLOBAL_FUNC(_EH_prolog,
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")  /* skip ret addr */
                  "pushl $-1\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  "pushl %eax\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  "pushl %fs:0\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  "movl  %esp, %fs:0\n\t"
                  "movl  12(%esp), %eax\n\t"
                  "movl  %ebp, 12(%esp)\n\t"
                  "leal  12(%esp), %ebp\n\t"
                  "pushl %eax\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  "ret")

static const SCOPETABLE_V4 *get_scopetable_v4( MSVCRT_EXCEPTION_FRAME *frame, ULONG_PTR cookie )
{
    return (const SCOPETABLE_V4 *)((ULONG_PTR)frame->scopetable ^ cookie);
}

static DWORD MSVCRT_nested_handler(PEXCEPTION_RECORD rec,
                                   EXCEPTION_REGISTRATION_RECORD* frame,
                                   PCONTEXT context,
                                   EXCEPTION_REGISTRATION_RECORD** dispatch)
{
  if (!(rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND)))
    return ExceptionContinueSearch;
  *dispatch = frame;
  return ExceptionCollidedUnwind;
}

static void msvcrt_local_unwind2(MSVCRT_EXCEPTION_FRAME* frame, int trylevel, void *ebp)
{
  EXCEPTION_REGISTRATION_RECORD reg;

  TRACE("(%p,%d,%d)\n",frame, frame->trylevel, trylevel);

  /* Register a handler in case of a nested exception */
  reg.Handler = MSVCRT_nested_handler;
  reg.Prev = NtCurrentTeb()->Tib.ExceptionList;
  __wine_push_frame(&reg);

  while (frame->trylevel != TRYLEVEL_END && frame->trylevel != trylevel)
  {
      int level = frame->trylevel;
      frame->trylevel = frame->scopetable[level].previousTryLevel;
      if (!frame->scopetable[level].lpfnFilter)
      {
          TRACE( "__try block cleanup level %d handler %p ebp %p\n",
                 level, frame->scopetable[level].lpfnHandler, ebp );
          call_unwind_func( frame->scopetable[level].lpfnHandler, ebp );
      }
  }
  __wine_pop_frame(&reg);
  TRACE("unwound OK\n");
}

static void msvcrt_local_unwind4( ULONG *cookie, MSVCRT_EXCEPTION_FRAME* frame, int trylevel, void *ebp )
{
    EXCEPTION_REGISTRATION_RECORD reg;
    const SCOPETABLE_V4 *scopetable = get_scopetable_v4( frame, *cookie );

    TRACE("(%p,%d,%d)\n",frame, frame->trylevel, trylevel);

    /* Register a handler in case of a nested exception */
    reg.Handler = MSVCRT_nested_handler;
    reg.Prev = NtCurrentTeb()->Tib.ExceptionList;
    __wine_push_frame(&reg);

    while (frame->trylevel != -2 && frame->trylevel != trylevel)
    {
        int level = frame->trylevel;
        frame->trylevel = scopetable->entries[level].previousTryLevel;
        if (!scopetable->entries[level].lpfnFilter)
        {
            TRACE( "__try block cleanup level %d handler %p ebp %p\n",
                   level, scopetable->entries[level].lpfnHandler, ebp );
            call_unwind_func( scopetable->entries[level].lpfnHandler, ebp );
        }
    }
    __wine_pop_frame(&reg);
    TRACE("unwound OK\n");
}

/*******************************************************************
 *		_local_unwind2 (MSVCRT.@)
 */
void CDECL _local_unwind2(MSVCRT_EXCEPTION_FRAME* frame, int trylevel)
{
    msvcrt_local_unwind2( frame, trylevel, &frame->_ebp );
}

/*******************************************************************
 *		_local_unwind4 (MSVCRT.@)
 */
void CDECL _local_unwind4( ULONG *cookie, MSVCRT_EXCEPTION_FRAME* frame, int trylevel )
{
    msvcrt_local_unwind4( cookie, frame, trylevel, &frame->_ebp );
}

/*******************************************************************
 *		_global_unwind2 (MSVCRT.@)
 */
void CDECL _global_unwind2(EXCEPTION_REGISTRATION_RECORD* frame)
{
    TRACE("(%p)\n",frame);
    RtlUnwind( frame, 0, 0, 0 );
}

/*********************************************************************
 *		_except_handler2 (MSVCRT.@)
 */
int CDECL _except_handler2(PEXCEPTION_RECORD rec,
                           EXCEPTION_REGISTRATION_RECORD* frame,
                           PCONTEXT context,
                           EXCEPTION_REGISTRATION_RECORD** dispatcher)
{
  FIXME("exception %x flags=%x at %p handler=%p %p %p stub\n",
        rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
        frame->Handler, context, dispatcher);
  return ExceptionContinueSearch;
}

/*********************************************************************
 *		_except_handler3 (MSVCRT.@)
 */
int CDECL _except_handler3(PEXCEPTION_RECORD rec,
                           MSVCRT_EXCEPTION_FRAME* frame,
                           PCONTEXT context, void* dispatcher)
{
  int retval, trylevel;
  EXCEPTION_POINTERS exceptPtrs;
  PSCOPETABLE pScopeTable;

  TRACE("exception %x flags=%x at %p handler=%p %p %p semi-stub\n",
        rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
        frame->handler, context, dispatcher);

  __asm__ __volatile__ ("cld");

  if (rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
  {
    /* Unwinding the current frame */
    msvcrt_local_unwind2(frame, TRYLEVEL_END, &frame->_ebp);
    TRACE("unwound current frame, returning ExceptionContinueSearch\n");
    return ExceptionContinueSearch;
  }
  else
  {
    /* Hunting for handler */
    exceptPtrs.ExceptionRecord = rec;
    exceptPtrs.ContextRecord = context;
    *((DWORD *)frame-1) = (DWORD)&exceptPtrs;
    trylevel = frame->trylevel;
    pScopeTable = frame->scopetable;

    while (trylevel != TRYLEVEL_END)
    {
      TRACE( "level %d prev %d filter %p\n", trylevel, pScopeTable[trylevel].previousTryLevel,
             pScopeTable[trylevel].lpfnFilter );
      if (pScopeTable[trylevel].lpfnFilter)
      {
        retval = call_filter( pScopeTable[trylevel].lpfnFilter, &exceptPtrs, &frame->_ebp );

        TRACE("filter returned %s\n", retval == EXCEPTION_CONTINUE_EXECUTION ?
              "CONTINUE_EXECUTION" : retval == EXCEPTION_EXECUTE_HANDLER ?
              "EXECUTE_HANDLER" : "CONTINUE_SEARCH");

        if (retval == EXCEPTION_CONTINUE_EXECUTION)
          return ExceptionContinueExecution;

        if (retval == EXCEPTION_EXECUTE_HANDLER)
        {
          /* Unwind all higher frames, this one will handle the exception */
          _global_unwind2((EXCEPTION_REGISTRATION_RECORD*)frame);
          msvcrt_local_unwind2(frame, trylevel, &frame->_ebp);

          /* Set our trylevel to the enclosing block, and call the __finally
           * code, which won't return
           */
          frame->trylevel = pScopeTable[trylevel].previousTryLevel;
          TRACE("__finally block %p\n",pScopeTable[trylevel].lpfnHandler);
          call_finally_block(pScopeTable[trylevel].lpfnHandler, &frame->_ebp);
          ERR("Returned from __finally block - expect crash!\n");
       }
      }
      trylevel = pScopeTable[trylevel].previousTryLevel;
    }
  }
  TRACE("reached TRYLEVEL_END, returning ExceptionContinueSearch\n");
  return ExceptionContinueSearch;
}

/*********************************************************************
 *		_except_handler4_common (MSVCRT.@)
 */
int CDECL _except_handler4_common( ULONG *cookie, void (*check_cookie)(void),
                                   EXCEPTION_RECORD *rec, MSVCRT_EXCEPTION_FRAME *frame,
                                   CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    int retval, trylevel;
    EXCEPTION_POINTERS exceptPtrs;
    const SCOPETABLE_V4 *scope_table = get_scopetable_v4( frame, *cookie );

    TRACE( "exception %x flags=%x at %p handler=%p %p %p cookie=%x scope table=%p cookies=%d/%x,%d/%x\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
           frame->handler, context, dispatcher, *cookie, scope_table,
           scope_table->gs_cookie_offset, scope_table->gs_cookie_xor,
           scope_table->eh_cookie_offset, scope_table->eh_cookie_xor );

    /* FIXME: no cookie validation yet */

    if (rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
    {
        /* Unwinding the current frame */
        msvcrt_local_unwind4( cookie, frame, -2, &frame->_ebp );
        TRACE("unwound current frame, returning ExceptionContinueSearch\n");
        return ExceptionContinueSearch;
    }
    else
    {
        /* Hunting for handler */
        exceptPtrs.ExceptionRecord = rec;
        exceptPtrs.ContextRecord = context;
        *((DWORD *)frame-1) = (DWORD)&exceptPtrs;
        trylevel = frame->trylevel;

        while (trylevel != -2)
        {
            TRACE( "level %d prev %d filter %p\n", trylevel,
                   scope_table->entries[trylevel].previousTryLevel,
                   scope_table->entries[trylevel].lpfnFilter );
            if (scope_table->entries[trylevel].lpfnFilter)
            {
                retval = call_filter( scope_table->entries[trylevel].lpfnFilter, &exceptPtrs, &frame->_ebp );

                TRACE("filter returned %s\n", retval == EXCEPTION_CONTINUE_EXECUTION ?
                      "CONTINUE_EXECUTION" : retval == EXCEPTION_EXECUTE_HANDLER ?
                      "EXECUTE_HANDLER" : "CONTINUE_SEARCH");

                if (retval == EXCEPTION_CONTINUE_EXECUTION)
                    return ExceptionContinueExecution;

                if (retval == EXCEPTION_EXECUTE_HANDLER)
                {
                    /* Unwind all higher frames, this one will handle the exception */
                    _global_unwind2((EXCEPTION_REGISTRATION_RECORD*)frame);
                    msvcrt_local_unwind4( cookie, frame, trylevel, &frame->_ebp );

                    /* Set our trylevel to the enclosing block, and call the __finally
                     * code, which won't return
                     */
                    frame->trylevel = scope_table->entries[trylevel].previousTryLevel;
                    TRACE("__finally block %p\n",scope_table->entries[trylevel].lpfnHandler);
                    call_finally_block(scope_table->entries[trylevel].lpfnHandler, &frame->_ebp);
                    ERR("Returned from __finally block - expect crash!\n");
                }
            }
            trylevel = scope_table->entries[trylevel].previousTryLevel;
        }
    }
    TRACE("reached -2, returning ExceptionContinueSearch\n");
    return ExceptionContinueSearch;
}


/*
 * setjmp/longjmp implementation
 */

#define MSVCRT_JMP_MAGIC 0x56433230 /* ID value for new jump structure */
typedef void (__stdcall *MSVCRT_unwind_function)(const struct MSVCRT___JUMP_BUFFER *);

/* define an entrypoint for setjmp/setjmp3 that stores the registers in the jmp buf */
/* and then jumps to the C backend function */
#define DEFINE_SETJMP_ENTRYPOINT(name) \
    __ASM_GLOBAL_FUNC( name, \
                       "movl 4(%esp),%ecx\n\t"   /* jmp_buf */      \
                       "movl %ebp,0(%ecx)\n\t"   /* jmp_buf.Ebp */  \
                       "movl %ebx,4(%ecx)\n\t"   /* jmp_buf.Ebx */  \
                       "movl %edi,8(%ecx)\n\t"   /* jmp_buf.Edi */  \
                       "movl %esi,12(%ecx)\n\t"  /* jmp_buf.Esi */  \
                       "movl %esp,16(%ecx)\n\t"  /* jmp_buf.Esp */  \
                       "movl 0(%esp),%eax\n\t"                      \
                       "movl %eax,20(%ecx)\n\t"  /* jmp_buf.Eip */  \
                       "jmp " __ASM_NAME("__regs_") # name )

/* restore the registers from the jmp buf upon longjmp */
extern void DECLSPEC_NORETURN longjmp_set_regs( struct MSVCRT___JUMP_BUFFER *jmp, int retval );
__ASM_GLOBAL_FUNC( longjmp_set_regs,
                   "movl 4(%esp),%ecx\n\t"   /* jmp_buf */
                   "movl 8(%esp),%eax\n\t"   /* retval */
                   "movl 0(%ecx),%ebp\n\t"   /* jmp_buf.Ebp */
                   "movl 4(%ecx),%ebx\n\t"   /* jmp_buf.Ebx */
                   "movl 8(%ecx),%edi\n\t"   /* jmp_buf.Edi */
                   "movl 12(%ecx),%esi\n\t"  /* jmp_buf.Esi */
                   "movl 16(%ecx),%esp\n\t"  /* jmp_buf.Esp */
                   "addl $4,%esp\n\t"        /* get rid of return address */
                   "jmp *20(%ecx)\n\t"       /* jmp_buf.Eip */ )

/*
 * The signatures of the setjmp/longjmp functions do not match that
 * declared in the setjmp header so they don't follow the regular naming
 * convention to avoid conflicts.
 */

/*******************************************************************
 *		_setjmp (MSVCRT.@)
 */
DEFINE_SETJMP_ENTRYPOINT(MSVCRT__setjmp)
int CDECL __regs_MSVCRT__setjmp(struct MSVCRT___JUMP_BUFFER *jmp)
{
    jmp->Registration = (unsigned long)NtCurrentTeb()->Tib.ExceptionList;
    if (jmp->Registration == ~0UL)
        jmp->TryLevel = TRYLEVEL_END;
    else
        jmp->TryLevel = ((MSVCRT_EXCEPTION_FRAME*)jmp->Registration)->trylevel;

    TRACE("buf=%p ebx=%08lx esi=%08lx edi=%08lx ebp=%08lx esp=%08lx eip=%08lx frame=%08lx\n",
          jmp, jmp->Ebx, jmp->Esi, jmp->Edi, jmp->Ebp, jmp->Esp, jmp->Eip, jmp->Registration );
    return 0;
}

/*******************************************************************
 *		_setjmp3 (MSVCRT.@)
 */
DEFINE_SETJMP_ENTRYPOINT( MSVCRT__setjmp3 )
int CDECL __regs_MSVCRT__setjmp3(struct MSVCRT___JUMP_BUFFER *jmp, int nb_args, ...)
{
    jmp->Cookie = MSVCRT_JMP_MAGIC;
    jmp->UnwindFunc = 0;
    jmp->Registration = (unsigned long)NtCurrentTeb()->Tib.ExceptionList;
    if (jmp->Registration == ~0UL)
    {
        jmp->TryLevel = TRYLEVEL_END;
    }
    else
    {
        int i;
        va_list args;

        va_start( args, nb_args );
        if (nb_args > 0) jmp->UnwindFunc = va_arg( args, unsigned long );
        if (nb_args > 1) jmp->TryLevel = va_arg( args, unsigned long );
        else jmp->TryLevel = ((MSVCRT_EXCEPTION_FRAME*)jmp->Registration)->trylevel;
        for (i = 0; i < 6 && i < nb_args - 2; i++)
            jmp->UnwindData[i] = va_arg( args, unsigned long );
        va_end( args );
    }

    TRACE("buf=%p ebx=%08lx esi=%08lx edi=%08lx ebp=%08lx esp=%08lx eip=%08lx frame=%08lx\n",
          jmp, jmp->Ebx, jmp->Esi, jmp->Edi, jmp->Ebp, jmp->Esp, jmp->Eip, jmp->Registration );
    return 0;
}

/*********************************************************************
 *		longjmp (MSVCRT.@)
 */
void CDECL MSVCRT_longjmp(struct MSVCRT___JUMP_BUFFER *jmp, int retval)
{
    unsigned long cur_frame = 0;

    TRACE("buf=%p ebx=%08lx esi=%08lx edi=%08lx ebp=%08lx esp=%08lx eip=%08lx frame=%08lx retval=%08x\n",
          jmp, jmp->Ebx, jmp->Esi, jmp->Edi, jmp->Ebp, jmp->Esp, jmp->Eip, jmp->Registration, retval );

    cur_frame=(unsigned long)NtCurrentTeb()->Tib.ExceptionList;
    TRACE("cur_frame=%lx\n",cur_frame);

    if (cur_frame != jmp->Registration)
        _global_unwind2((EXCEPTION_REGISTRATION_RECORD*)jmp->Registration);

    if (jmp->Registration)
    {
        if (IsBadReadPtr(&jmp->Cookie, sizeof(long)) || jmp->Cookie != MSVCRT_JMP_MAGIC)
        {
            msvcrt_local_unwind2((MSVCRT_EXCEPTION_FRAME*)jmp->Registration,
                                 jmp->TryLevel, (void *)jmp->Ebp);
        }
        else if(jmp->UnwindFunc)
        {
            MSVCRT_unwind_function unwind_func;

            unwind_func=(MSVCRT_unwind_function)jmp->UnwindFunc;
            unwind_func(jmp);
        }
    }

    if (!retval)
        retval = 1;

    longjmp_set_regs( jmp, retval );
}

/*********************************************************************
 *		_seh_longjmp_unwind (MSVCRT.@)
 */
void __stdcall _seh_longjmp_unwind(struct MSVCRT___JUMP_BUFFER *jmp)
{
    msvcrt_local_unwind2( (MSVCRT_EXCEPTION_FRAME *)jmp->Registration, jmp->TryLevel, (void *)jmp->Ebp );
}

/*********************************************************************
 *		_seh_longjmp_unwind4 (MSVCRT.@)
 */
void __stdcall _seh_longjmp_unwind4(struct MSVCRT___JUMP_BUFFER *jmp)
{
    msvcrt_local_unwind4( (void *)jmp->Cookie, (MSVCRT_EXCEPTION_FRAME *)jmp->Registration,
                          jmp->TryLevel, (void *)jmp->Ebp );
}

#endif  /* __i386__ */
