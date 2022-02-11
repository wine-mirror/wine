/*
 * DPMI 0.9 emulation
 *
 * Copyright 1995 Alexandre Julliard
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/winbase16.h"
#include "wownt32.h"
#include "kernel16_private.h"
#include "dosexe.h"

#include "excpt.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int31);

static void* lastvalloced = NULL;

/**********************************************************************
 *          DPMI_xalloc
 * special virtualalloc, allocates linearly monoton growing memory.
 * (the usual VirtualAlloc does not satisfy that restriction)
 */
static LPVOID DPMI_xalloc( DWORD len ) 
{
    LPVOID  ret;
    LPVOID  oldlastv = lastvalloced;

    if (lastvalloced) 
    {
        int xflag = 0;

        ret = NULL;
        while (!ret) 
        {
            ret = VirtualAlloc( lastvalloced, len,
                                MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE );
            if (!ret)
                lastvalloced = (char *) lastvalloced + 0x10000;

            /* we failed to allocate one in the first round.
             * try non-linear
             */
            if (!xflag && (lastvalloced<oldlastv)) 
            { 
                /* wrapped */
                FIXME( "failed to allocate linearly growing memory (%lu bytes), "
                       "using non-linear growing...\n", len );
                xflag++;
            }

            /* if we even fail to allocate something in the next
             * round, return NULL
             */
            if ((xflag==1) && (lastvalloced >= oldlastv))
                xflag++;

            if ((xflag==2) && (lastvalloced < oldlastv)) {
                FIXME( "failed to allocate any memory of %lu bytes!\n", len );
                return NULL;
            }
        }
    } 
    else
    {
        ret = VirtualAlloc( NULL, len, 
                            MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE );
    }

    lastvalloced = (LPVOID)(((DWORD)ret+len+0xffff)&~0xffff);
    return ret;
}

/**********************************************************************
 *          DPMI_xfree
 */
static void DPMI_xfree( LPVOID ptr ) 
{
    VirtualFree( ptr, 0, MEM_RELEASE );
}

/**********************************************************************
 *          DPMI_xrealloc
 *
 * FIXME: perhaps we could grow this mapped area... 
 */
static LPVOID DPMI_xrealloc( LPVOID ptr, DWORD newsize )
{
    MEMORY_BASIC_INFORMATION        mbi;

    if (ptr)
    {
        LPVOID newptr;

        if (!VirtualQuery(ptr,&mbi,sizeof(mbi)))
        {
            FIXME( "realloc of DPMI_xallocd region %p?\n", ptr );
            return NULL;
        }

        if (mbi.State == MEM_FREE)
        {
            FIXME( "realloc of DPMI_xallocd region %p?\n", ptr );
            return NULL;
        }

        /* We do not shrink allocated memory. most reallocs
         * only do grows anyway
         */
        if (newsize <= mbi.RegionSize)
            return ptr;

        newptr = DPMI_xalloc( newsize );
        if (!newptr)
            return NULL;

        memcpy( newptr, ptr, mbi.RegionSize );
        DPMI_xfree( ptr );

        return newptr;
    }

    return DPMI_xalloc( newsize );
}


/**********************************************************************
 *         DOSVM_Int31Handler
 *
 * Handler for int 31h (DPMI).
 */
void WINAPI DOSVM_Int31Handler( CONTEXT *context )
{
    RESET_CFLAG(context);
    switch(AX_reg(context))
    {
    case 0x0000:  /* Allocate LDT descriptors */
        TRACE( "allocate LDT descriptors (%d)\n", CX_reg(context) );
        {
            WORD sel =  AllocSelectorArray16( CX_reg(context) );
            if(!sel) 
            {
               TRACE( "failed\n" );
               SET_AX( context, 0x8011 ); /* descriptor unavailable */
               SET_CFLAG( context );
            } 
            else 
            { 
                TRACE( "success, array starts at 0x%04x\n", sel );
                SET_AX( context, sel );      
            }
        }
        break;

    case 0x0001:  /* Free LDT descriptor */
        TRACE( "free LDT descriptor (0x%04x)\n", BX_reg(context) );
        if (FreeSelector16( BX_reg(context) ))
        {
            SET_AX( context, 0x8022 );  /* invalid selector */
            SET_CFLAG( context );
        }
        else
        {
            /* If a segment register contains the selector being freed, */
            /* set it to zero. */
            if (!((context->SegDs^BX_reg(context)) & ~3)) context->SegDs = 0;
            if (!((context->SegEs^BX_reg(context)) & ~3)) context->SegEs = 0;
            if (!((context->SegFs^BX_reg(context)) & ~3)) context->SegFs = 0;
            if (!((context->SegGs^BX_reg(context)) & ~3)) context->SegGs = 0;
        }
        break;

    case 0x0002:  /* Real mode segment to descriptor */
        TRACE( "real mode segment to descriptor (0x%04x)\n", BX_reg(context) );
        {
            WORD entryPoint = 0;  /* KERNEL entry point for descriptor */
            switch(BX_reg(context))
            {
            case 0x0000: entryPoint = 183; break;  /* __0000H */
            case 0x0040: entryPoint = 193; break;  /* __0040H */
            case 0xa000: entryPoint = 174; break;  /* __A000H */
            case 0xb000: entryPoint = 181; break;  /* __B000H */
            case 0xb800: entryPoint = 182; break;  /* __B800H */
            case 0xc000: entryPoint = 195; break;  /* __C000H */
            case 0xd000: entryPoint = 179; break;  /* __D000H */
            case 0xe000: entryPoint = 190; break;  /* __E000H */
            case 0xf000: entryPoint = 194; break;  /* __F000H */
            default:
                FIXME("Real mode segment (%x) to descriptor: no longer supported\n",
                      BX_reg(context));
                SET_CFLAG( context );
                break;
            }
            if (entryPoint)
            {
                FARPROC16 proc = GetProcAddress16( GetModuleHandle16( "KERNEL" ),
                                                   (LPCSTR)(ULONG_PTR)entryPoint );
                SET_AX( context, LOWORD(proc) );
            }
        }
        break;

    case 0x0003:  /* Get next selector increment */
        TRACE("get selector increment (__AHINCR)\n");
        context->Eax = __AHINCR;
        break;

    case 0x0004:  /* Lock selector (not supported) */
        FIXME("lock selector not supported\n");
        context->Eax = 0;  /* FIXME: is this a correct return value? */
        break;

    case 0x0005:  /* Unlock selector (not supported) */
        FIXME("unlock selector not supported\n");
        context->Eax = 0;  /* FIXME: is this a correct return value? */
        break;

    case 0x0006:  /* Get selector base address */
        TRACE( "get selector base address (0x%04x)\n", BX_reg(context) );
        if (!ldt_is_valid( BX_reg(context) ))
        {
            context->Eax = 0x8022;  /* invalid selector */
            SET_CFLAG(context);
        }
        else
        {
            void *base = ldt_get_base( BX_reg(context) );
            SET_CX( context, HIWORD(base) );
            SET_DX( context, LOWORD(base) );
        }
        break;

    case 0x0007:  /* Set selector base address */
        {
            DWORD base = MAKELONG( DX_reg(context), CX_reg(context) );
            WORD  sel = BX_reg(context);
            TRACE( "set selector base address (0x%04x,0x%08lx)\n", sel, base );

            /* check if Win16 app wants to access lower 64K of DOS memory */
            if (base < 0x10000) DOSMEM_MapDosLayout();

            SetSelectorBase( sel, base );
        }
        break;

    case 0x0008:  /* Set selector limit */
        {
            DWORD limit = MAKELONG( DX_reg(context), CX_reg(context) );
            TRACE( "set selector limit (0x%04x,0x%08lx)\n",
                   BX_reg(context), limit );
            SetSelectorLimit16( BX_reg(context), limit );
        }
        break;

    case 0x0009:  /* Set selector access rights */
        TRACE( "set selector access rights(0x%04x,0x%04x)\n",
               BX_reg(context), CX_reg(context) );
        SelectorAccessRights16( BX_reg(context), 1, CX_reg(context) );
        break;

    case 0x000a:  /* Allocate selector alias */
        TRACE( "allocate selector alias (0x%04x)\n", BX_reg(context) );
        SET_AX( context, AllocCStoDSAlias16( BX_reg(context) ) );
        if (!AX_reg(context))
        {
            SET_AX( context, 0x8011 );  /* descriptor unavailable */
            SET_CFLAG(context);
        }
        break;

    case 0x000b:  /* Get descriptor */
        TRACE( "get descriptor (0x%04x)\n", BX_reg(context) );
        {
            LDT_ENTRY *entry = CTX_SEG_OFF_TO_LIN( context, context->SegEs,
                                                   context->Edi );
            ldt_get_entry( BX_reg(context), entry );
        }
        break;

    case 0x000c:  /* Set descriptor */
        TRACE( "set descriptor (0x%04x)\n", BX_reg(context) );
        {
            LDT_ENTRY *entry = CTX_SEG_OFF_TO_LIN( context, context->SegEs,
                                                   context->Edi );
            if (!ldt_is_system( BX_reg(context) )) ldt_set_entry( BX_reg(context), *entry );
        }
        break;

    case 0x000d:  /* Allocate specific LDT descriptor */
        FIXME( "allocate descriptor (0x%04x), stub!\n", BX_reg(context) );
        SET_AX( context, 0x8011 ); /* descriptor unavailable */
        SET_CFLAG( context );
        break;

    case 0x000e:  /* Get Multiple Descriptors (1.0) */
        FIXME( "get multiple descriptors - unimplemented\n" );
        break;

    case 0x000f:  /* Set Multiple Descriptors (1.0) */
        FIXME( "set multiple descriptors - unimplemented\n" );
        break;

    case 0x0100:  /* Allocate DOS memory block */
        TRACE( "allocate DOS memory block (0x%x paragraphs)\n", BX_reg(context) );
        {
            DWORD dw = GlobalDOSAlloc16( (DWORD)BX_reg(context) << 4 );
            if (dw) {
                SET_AX( context, HIWORD(dw) );
                SET_DX( context, LOWORD(dw) );
            } else {
                SET_AX( context, 0x0008 ); /* insufficient memory */
                SET_BX( context, DOSMEM_Available() >> 4 );
                SET_CFLAG(context);
            }
            break;
        }

    case 0x0101:  /* Free DOS memory block */
        TRACE( "free DOS memory block (0x%04x)\n", DX_reg(context) );
        {
            WORD error = GlobalDOSFree16( DX_reg(context) );
            if (error) {
                SET_AX( context, 0x0009 ); /* memory block address invalid */
                SET_CFLAG( context );
            }
        }
        break;

    case 0x0102: /* Resize DOS Memory Block */
        FIXME( "resize DOS memory block (0x%04x, 0x%x paragraphs) - unimplemented\n", 
               DX_reg(context), BX_reg(context) );
        break;

    case 0x0200: /* get real mode interrupt vector */
        TRACE( "get realmode interrupt vector (0x%02x) - not supported\n",
               BL_reg(context) );
        SET_CX( context, 0 );
        SET_DX( context, 0 );
        break;

    case 0x0201: /* set real mode interrupt vector */
        TRACE( "set realmode interrupt vector (0x%02x, 0x%04x:0x%04x) - not supported\n",
               BL_reg(context), CX_reg(context), DX_reg(context) );
        break;

    case 0x0202:  /* Get Processor Exception Handler Vector */
        FIXME( "Get Processor Exception Handler Vector (0x%02x)\n",
               BL_reg(context) );
        SET_CX( context, 0 );
        SET_DX( context, 0 );
        break;

    case 0x0203:  /* Set Processor Exception Handler Vector */
         FIXME( "Set Processor Exception Handler Vector (0x%02x)\n",
                BL_reg(context) );
         break;

    case 0x0204:  /* Get protected mode interrupt vector */
        TRACE("get protected mode interrupt handler (0x%02x)\n",
              BL_reg(context));
        {
            FARPROC16 handler = DOSVM_GetPMHandler16( BL_reg(context) );
            SET_CX( context, SELECTOROF(handler) );
            SET_DX( context, OFFSETOF(handler) );
        }
        break;

    case 0x0205:  /* Set protected mode interrupt vector */
        TRACE("set protected mode interrupt handler (0x%02x,0x%04x:0x%08lx)\n",
              BL_reg(context), CX_reg(context), context->Edx);
        {
            FARPROC16 handler;
            handler = (FARPROC16)MAKESEGPTR( CX_reg(context), DX_reg(context)); 
            DOSVM_SetPMHandler16( BL_reg(context), handler );
        }
        break;

    case 0x0300:  /* Simulate real mode interrupt */
        TRACE( "Simulate real mode interrupt %02x - not supported\n", BL_reg(context));
        break;

    case 0x0301:  /* Call real mode procedure with far return */
        TRACE( "Call real mode procedure with far return - not supported\n" );
        break;

    case 0x0302:  /* Call real mode procedure with interrupt return */
        TRACE( "Call real mode procedure with interrupt return - not supported\n" );
        break;

    case 0x0303:  /* Allocate Real Mode Callback Address */
        TRACE( "Allocate real mode callback address - not supported\n" );
        break;

    case 0x0304:  /* Free Real Mode Callback Address */
        TRACE( "Free real mode callback address - not supported\n" );
        break;

    case 0x0305:  /* Get State Save/Restore Addresses */
        TRACE("get state save/restore addresses - no longer supported\n");
        /* we probably won't need this kind of state saving */
        SET_AX( context, 0 );
        /* real mode */
        SET_BX( context, 0 );
        SET_CX( context, 0 );
        /* protected mode */
        SET_SI( context, 0 );
        context->Edi = 0;
        break;

    case 0x0306:  /* Get Raw Mode Switch Addresses */
        TRACE("get raw mode switch addresses - no longer supported\n");
        /* real mode */
        SET_BX( context, 0 );
        SET_CX( context, 0 );
        /* protected mode */
        SET_SI( context, 0 );
        context->Edi = 0;
        break;

    case 0x0400:  /* Get DPMI version */
        TRACE("get DPMI version\n");
        {
            SYSTEM_INFO si;

            GetSystemInfo(&si);
            SET_AX( context, 0x005a );  /* DPMI version 0.90 */
            SET_BX( context, 0x0005 );  /* Flags: 32-bit, virtual memory */
            SET_CL( context, si.wProcessorLevel );
            SET_DX( context, 0x0870 );  /* Interrupt controller base */
        }
        break;

    case 0x0401:  /* Get DPMI Capabilities (1.0) */
        FIXME( "get dpmi capabilities - unimplemented\n");
        break;

    case 0x0500:  /* Get free memory information */
        TRACE("get free memory information\n");
        {
            MEMORYSTATUS status;
            SYSTEM_BASIC_INFORMATION sbi;

            /* the layout is just the same as MEMMANINFO, but without
             * the dwSize entry.
             */
            struct
            {
                DWORD dwLargestFreeBlock;
                DWORD dwMaxPagesAvailable;
                DWORD dwMaxPagesLockable;
                DWORD dwTotalLinearSpace;
                DWORD dwTotalUnlockedPages;
                DWORD dwFreePages;
                DWORD dwTotalPages;
                DWORD dwFreeLinearSpace;
                DWORD dwSwapFilePages;
                WORD  wPageSize;
            } *info = CTX_SEG_OFF_TO_LIN( context, context->SegEs, context->Edi );

            GlobalMemoryStatus( &status );
            NtQuerySystemInformation( SystemBasicInformation, &sbi, sizeof(sbi), NULL );

            info->wPageSize            = sbi.PageSize;
            info->dwLargestFreeBlock   = status.dwAvailVirtual;
            info->dwMaxPagesAvailable  = info->dwLargestFreeBlock / info->wPageSize;
            info->dwMaxPagesLockable   = info->dwMaxPagesAvailable;
            info->dwTotalLinearSpace   = status.dwTotalVirtual / info->wPageSize;
            info->dwTotalUnlockedPages = info->dwTotalLinearSpace;
            info->dwFreePages          = info->dwMaxPagesAvailable;
            info->dwTotalPages         = info->dwTotalLinearSpace;
            info->dwFreeLinearSpace    = info->dwMaxPagesAvailable;
            info->dwSwapFilePages      = status.dwTotalPageFile / info->wPageSize;
            break;
        }

    case 0x0501:  /* Allocate memory block */
        {
            DWORD size = MAKELONG( CX_reg(context), BX_reg(context) );
            BYTE *ptr;

            TRACE( "allocate memory block (%lu bytes)\n", size );

            ptr = DPMI_xalloc( size );
            if (!ptr)
            {
                SET_AX( context, 0x8012 );  /* linear memory not available */
                SET_CFLAG(context);
            } 
            else 
            {
                SET_BX( context, HIWORD(ptr) );
                SET_CX( context, LOWORD(ptr) );
                SET_SI( context, HIWORD(ptr) );
                SET_DI( context, LOWORD(ptr) );
            }
            break;
        }

    case 0x0502:  /* Free memory block */
        {
            DWORD handle = MAKELONG( DI_reg(context), SI_reg(context) );
            TRACE( "free memory block (0x%08lx)\n", handle );
            DPMI_xfree( (void *)handle );
        }
        break;

    case 0x0503:  /* Resize memory block */
        {
            DWORD size = MAKELONG( CX_reg(context), BX_reg(context) );
            DWORD handle = MAKELONG( DI_reg(context), SI_reg(context) );
            BYTE *ptr;

            TRACE( "resize memory block (0x%08lx, %lu bytes)\n", handle, size );

            ptr = DPMI_xrealloc( (void *)handle, size );
            if (!ptr)
            {
                SET_AX( context, 0x8012 );  /* linear memory not available */
                SET_CFLAG(context);
            } else {
                SET_BX( context, HIWORD(ptr) );
                SET_CX( context, LOWORD(ptr) );
                SET_SI( context, HIWORD(ptr) );
                SET_DI( context, LOWORD(ptr) );
            }
        }
        break;

    case 0x0507:  /* Set page attributes (1.0) */
        FIXME( "set page attributes - unimplemented\n" );
        break;  /* Just ignore it */

    case 0x0600:  /* Lock linear region */
        TRACE( "lock linear region - ignored (no paging)\n" );
        break;

    case 0x0601:  /* Unlock linear region */
        TRACE( "unlock linear region - ignored (no paging)\n" );
        break;

    case 0x0602:  /* Mark real mode region as pageable */
        TRACE( "mark real mode region as pageable - ignored (no paging)\n" );
        break;

    case 0x0603:  /* Relock real mode region */
        TRACE( "relock real mode region - ignored (no paging)\n" );
        break;

    case 0x0604:  /* Get page size */
    {
        SYSTEM_BASIC_INFORMATION info;
        TRACE("get pagesize\n");
        NtQuerySystemInformation( SystemBasicInformation, &info, sizeof(info), NULL );
        SET_BX( context, HIWORD(info.PageSize) );
        SET_CX( context, LOWORD(info.PageSize) );
        break;
    }
    case 0x0700: /* Mark pages as paging candidates */
        TRACE( "mark pages as paging candidates - ignored (no paging)\n" );
        break;

    case 0x0701: /* Discard pages */
        TRACE( "discard pages - ignored (no paging)\n" );
        break;

    case 0x0702:  /* Mark page as demand-paging candidate */
        TRACE( "mark page as demand-paging candidate - ignored (no paging)\n" );
        break;

    case 0x0703:  /* Discard page contents */
        TRACE( "discard page contents - ignored (no paging)\n" );
        break;

    case 0x0800:  /* Physical address mapping */
        FIXME( "physical address mapping (0x%08lx) - unimplemented\n",
               MAKELONG(CX_reg(context),BX_reg(context)) );
        break;

    case 0x0900:  /* Get and Disable Virtual Interrupt State */
        TRACE( "Get and Disable Virtual Interrupt State - not supported\n" );
        break;

    case 0x0901:  /* Get and Enable Virtual Interrupt State */
        TRACE( "Get and Enable Virtual Interrupt State - not supported\n" );
        break;

    case 0x0902:  /* Get Virtual Interrupt State */
        TRACE( "Get Virtual Interrupt State - not supported\n" );
        break;

    case 0x0e00:  /* Get Coprocessor Status (1.0) */
        /*
         * Return status in AX bits:
         * B0    - MPv (MP bit in the virtual MSW/CR0)
         *         0 = numeric coprocessor is disabled for this client
         *         1 = numeric coprocessor is enabled for this client
         * B1    - EMv (EM bit in the virtual MSW/CR0)
         *         0 = client is not emulating coprocessor instructions
         *         1 = client is emulating coprocessor instructions
         * B2    - MPr (MP bit from the actual MSW/CR0)
         *         0 = numeric coprocessor is not present
         *         1 = numeric coprocessor is present
         * B3    - EMr (EM bit from the actual MSW/CR0)
         *         0 = host is not emulating coprocessor instructions
         *         1 = host is emulating coprocessor instructions
         * B4-B7 - coprocessor type
         *         00H = no coprocessor
         *         02H = 80287
         *         03H = 80387
         *         04H = 80486 with numeric coprocessor
         *         05H-0FH = reserved for future numeric processors
         */
        TRACE( "Get Coprocessor Status\n" );
        SET_AX( context, 69 ); /* 486, coprocessor present and enabled */ 
        break;

    case 0x0e01: /* Set Coprocessor Emulation (1.0) */
        /*
         * See function 0x0e00.
         * BX bit B0 is new value for MPv.
         * BX bit B1 is new value for EMv.
         */
        if (BX_reg(context) != 1)
            FIXME( "Set Coprocessor Emulation to %d - unimplemented\n", 
                   BX_reg(context) );
        else
            TRACE( "Set Coprocessor Emulation - ignored\n" );
        break;

    default:
        INT_BARF( context, 0x31 );
        SET_AX( context, 0x8001 );  /* unsupported function */
        SET_CFLAG(context);
        break;
    }  
}
