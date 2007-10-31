/*
 * hal.dll implementation
 *
 * Copyright (C) 2007 Chris Wulff
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

#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "excpt.h"
#include "ddk/wdm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntoskrnl);

#ifdef __i386__
#define DEFINE_FASTCALL1_ENTRYPOINT( name ) \
    __ASM_GLOBAL_FUNC( name, \
                       "popl %eax\n\t" \
                       "pushl %ecx\n\t" \
                       "pushl %eax\n\t" \
                       "jmp " __ASM_NAME("__regs_") #name )
#define DEFINE_FASTCALL2_ENTRYPOINT( name ) \
    __ASM_GLOBAL_FUNC( name, \
                       "popl %eax\n\t" \
                       "pushl %edx\n\t" \
                       "pushl %ecx\n\t" \
                       "pushl %eax\n\t" \
                       "jmp " __ASM_NAME("__regs_") #name )
#endif


#ifdef DEFINE_FASTCALL1_ENTRYPOINT
DEFINE_FASTCALL1_ENTRYPOINT( KfAcquireSpinLock )
KIRQL WINAPI __regs_KfAcquireSpinLock(PKSPIN_LOCK SpinLock)
#else
KIRQL WINAPI KfAcquireSpinLock(PKSPIN_LOCK SpinLock)
#endif
{
    FIXME( "(%p) stub!\n", SpinLock );

    return (KIRQL)0;
}


#ifdef DEFINE_FASTCALL1_ENTRYPOINT
DEFINE_FASTCALL1_ENTRYPOINT( KfLowerIrql )
VOID WINAPI __regs_KfLowerIrql(KIRQL NewIrql)
#else
VOID WINAPI KfLowerIrql(KIRQL NewIrql)
#endif
{
    FIXME( "(%u) stub!\n", NewIrql );
}


#ifdef DEFINE_FASTCALL1_ENTRYPOINT
DEFINE_FASTCALL1_ENTRYPOINT( KfRaiseIrql )
KIRQL WINAPI __regs_KfRaiseIrql(KIRQL NewIrql)
#else
KIRQL WINAPI KfRaiseIrql(KIRQL NewIrql)
#endif
{
    FIXME( "(%u) stub!\n", NewIrql );

    return (KIRQL)0;
}


#ifdef DEFINE_FASTCALL2_ENTRYPOINT
DEFINE_FASTCALL2_ENTRYPOINT( KfReleaseSpinLock )
VOID WINAPI __regs_KfReleaseSpinLock(PKSPIN_LOCK SpinLock, KIRQL NewIrql)
#else
VOID WINAPI KfReleaseSpinLock(PKSPIN_LOCK SpinLock, KIRQL NewIrql)
#endif
{
    FIXME( "(%p %u) stub!\n", SpinLock, NewIrql );
}
