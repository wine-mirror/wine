/*
 * Temporary RISC-V64 MSVCRT exception stubs.
 * These stubs permit linking but do not implement C++ unwinding.
 */

#if defined(__riscv) && __riscv_xlen == 64

#include <stdarg.h>
#include <fpieee.h>

#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "msvcrt.h"
#include "excpt.h"
#include "wine/debug.h"
#include "cppexcept.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

void *call_catch_handler( EXCEPTION_RECORD *rec )
{
    FIXME( "RISC-V C++ catch handler not implemented, record %p\n", rec );
    return NULL;
}

void *call_unwind_handler( void *handler, ULONG_PTR frame, DISPATCHER_CONTEXT *dispatch )
{
    FIXME( "RISC-V C++ unwind handler not implemented, handler %p frame %Ix dispatch %p\n",
           handler, frame, dispatch );
    return NULL;
}

ULONG_PTR get_exception_pc( DISPATCHER_CONTEXT *dispatch )
{
    return dispatch->ControlPc;
}

int handle_fpieee_flt( __msvcrt_ulong exception_code, EXCEPTION_POINTERS *ep,
                       int (__cdecl *handler)(_FPIEEE_RECORD*) )
{
    FIXME( "(%lx %p %p)\n", exception_code, ep, handler );
    return EXCEPTION_CONTINUE_SEARCH;
}

#endif

