/*
 * DOS Virtual Machine
 *
 * Copyright 1998 Ove Kåven
 * Copyright 2002 Jukka Heinonen
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
 * Note: This code hasn't been completely cleaned up yet.
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <sys/types.h>

#include "wine/winbase16.h"
#include "wine/exception.h"
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wingdi.h"
#include "winuser.h"
#include "wownt32.h"
#include "winnt.h"
#include "wincon.h"

#include "dosexe.h"
#include "wine/debug.h"
#include "excpt.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

WORD DOSVM_psp = 0;
WORD DOSVM_retval = 0;

/*
 * Wine DOS memory layout above 640k:
 *
 *   a0000 - affff : VGA graphics         (vga.c)
 *   b0000 - bffff : Monochrome text      (unused)
 *   b8000 - bffff : VGA text             (vga.c)
 *   c0000 - cffff : EMS frame            (int67.c)
 *   d0000 - effff : Free memory for UMBs (himem.c)
 *   f0000 - fffff : BIOS stuff           (msdos/dosmem.c)
 *  100000 -10ffff : High memory area     (unused)
 */

/*
 * Table of real mode segments and protected mode selectors
 * for code stubs and other miscellaneous storage.
 */
struct DPMI_segments *DOSVM_dpmi_segments = NULL;

/*
 * First and last address available for upper memory blocks.
 */
#define DOSVM_UMB_BOTTOM 0xd0000
#define DOSVM_UMB_TOP    0xeffff

/*
 * First free address for upper memory blocks.
 */
static DWORD DOSVM_umb_free = DOSVM_UMB_BOTTOM;


/**********************************************************************
 *          DOSVM_Exit
 */
void DOSVM_Exit( WORD retval )
{
    DWORD count;

    ReleaseThunkLock( &count );
    ExitThread( retval );
}

/***********************************************************************
 *           DOSVM_AllocUMB
 *
 * Allocate upper memory block (UMB) from upper memory.
 * Returned pointer is aligned to 16-byte (paragraph) boundary.
 *
 * This routine is only for allocating static storage for
 * Wine internal uses. Allocated memory can be accessed from
 * real mode, memory is taken from area already mapped and reserved
 * by Wine and the allocation has very little memory and speed
 * overhead. Use of this routine also preserves precious DOS
 * conventional memory.
 */
static LPVOID DOSVM_AllocUMB( DWORD size )
{
  LPVOID ptr = (LPVOID)DOSVM_umb_free;

  size = ((size + 15) >> 4) << 4;

  if(DOSVM_umb_free + size - 1 > DOSVM_UMB_TOP) {
    ERR("Out of upper memory area.\n");
    return 0;
  }

  DOSVM_umb_free += size;
  return ptr;
}


/**********************************************************************
 *          alloc_selector
 *
 * Allocate a selector corresponding to a real mode address.
 * size must be < 64k.
 */
static WORD alloc_selector( void *base, DWORD size, unsigned char flags )
{
    WORD sel = wine_ldt_alloc_entries( 1 );

    if (sel)
    {
        LDT_ENTRY entry;
        wine_ldt_set_base( &entry, base );
        wine_ldt_set_limit( &entry, size - 1 );
        wine_ldt_set_flags( &entry, flags );
        wine_ldt_set_entry( sel, &entry );
    }
    return sel;
}


/***********************************************************************
 *           DOSVM_AllocCodeUMB
 *
 * Allocate upper memory block for storing code stubs.
 * Initializes real mode segment and 16-bit protected mode selector
 * for the allocated code block.
 *
 * FIXME: should allocate a single PM selector for the whole UMB range.
 */
static LPVOID DOSVM_AllocCodeUMB( DWORD size, WORD *selector )
{
    LPVOID ptr = DOSVM_AllocUMB( size );
    *selector = alloc_selector( ptr, size, WINE_LDT_FLAGS_CODE );
    return ptr;
}


/***********************************************************************
 *           DOSVM_AllocDataUMB
 *
 * Allocate upper memory block for storing data.
 * Initializes real mode segment and 16-bit protected mode selector
 * for the allocated data block.
 */
LPVOID DOSVM_AllocDataUMB( DWORD size, WORD *selector )
{
    LPVOID ptr = DOSVM_AllocUMB( size );
    *selector = alloc_selector( ptr, size, WINE_LDT_FLAGS_DATA );
    return ptr;
}


/***********************************************************************
 *           DOSVM_InitSegments
 *
 * Initializes DOSVM_dpmi_segments. Allocates required memory and
 * sets up segments and selectors for accessing the memory.
 */
void DOSVM_InitSegments(void)
{
    DWORD old_prot;
    LPSTR ptr;
    int   i;

    static const char relay[]=
    {
        0xca, 0x04, 0x00, /* 16-bit far return and pop 4 bytes (relay void* arg) */
        0xcd, 0x31,       /* int 31 */
        0xfb, 0x66, 0xcb  /* sti and 32-bit far return */
    };

    /*
     * Allocate pointer array.
     */
    DOSVM_dpmi_segments = DOSVM_AllocUMB( sizeof(struct DPMI_segments) );

    /*
     * PM / offset N*5: Interrupt N in 16-bit protected mode.
     */
    ptr = DOSVM_AllocCodeUMB( 5 * 256, &DOSVM_dpmi_segments->int16_sel );
    for(i=0; i<256; i++) {
        /*
         * Each 16-bit interrupt handler is 5 bytes:
         * 0xCD,<i>       = int <i> (interrupt)
         * 0xCA,0x02,0x00 = ret 2   (16-bit far return and pop 2 bytes / eflags)
         */
        ptr[i * 5 + 0] = 0xCD;
        ptr[i * 5 + 1] = i;
        ptr[i * 5 + 2] = 0xCA;
        ptr[i * 5 + 3] = 0x02;
        ptr[i * 5 + 4] = 0x00;
    }

    /*
     * PM / offset 0: Stub where __wine_call_from_16_regs returns.
     * PM / offset 3: Stub which swaps back to 32-bit application code/stack.
     * PM / offset 5: Stub which enables interrupts
     */
    ptr = DOSVM_AllocCodeUMB( sizeof(relay), &DOSVM_dpmi_segments->relay_code_sel);
    memcpy( ptr, relay, sizeof(relay) );

    /*
     * Space for 16-bit stack used by relay code.
     */
    ptr = DOSVM_AllocDataUMB( DOSVM_RELAY_DATA_SIZE, &DOSVM_dpmi_segments->relay_data_sel);
    memset( ptr, 0, DOSVM_RELAY_DATA_SIZE );

    /*
     * As we store code in UMB we should make sure it is executable
     */
    VirtualProtect((void *)DOSVM_UMB_BOTTOM, DOSVM_UMB_TOP - DOSVM_UMB_BOTTOM, PAGE_EXECUTE_READWRITE, &old_prot);
}
