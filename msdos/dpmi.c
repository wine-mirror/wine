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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>

#include "windef.h"
#include "wine/winbase16.h"
#include "miscemu.h"
#include "msdos.h"
#include "task.h"
#include "toolhelp.h"
#include "selectors.h"
#include "callback.h"
#include "wine/debug.h"
#include "stackframe.h"

WINE_DEFAULT_DEBUG_CHANNEL(int31);

#define DOS_GET_DRIVE(reg) ((reg) ? (reg) - 1 : DRIVE_GetCurrentDrive())

void CreateBPB(int drive, BYTE *data, BOOL16 limited);  /* defined in int21.c */

static void* lastvalloced = NULL;


DOSVM_TABLE Dosvm = { NULL, };

static HMODULE DosModule;

/**********************************************************************
 *	    DPMI_LoadDosSystem
 */
BOOL DPMI_LoadDosSystem(void)
{
    if (DosModule) return TRUE;
    DosModule = LoadLibraryA( "winedos.dll" );
    if (!DosModule) {
        ERR("could not load winedos.dll, DOS subsystem unavailable\n");
        return FALSE;
    }
#define GET_ADDR(func)  Dosvm.func = (void *)GetProcAddress(DosModule, #func);

    GET_ADDR(LoadDosExe);
    GET_ADDR(CallRMInt);
    GET_ADDR(CallRMProc);
    GET_ADDR(AllocRMCB);
    GET_ADDR(FreeRMCB);
    GET_ADDR(RawModeSwitch);
    GET_ADDR(SetTimer);
    GET_ADDR(GetTimer);
    GET_ADDR(inport);
    GET_ADDR(outport);
    GET_ADDR(ASPIHandler);
    GET_ADDR(EmulateInterruptPM);
#undef GET_ADDR
    return TRUE;
}

/**********************************************************************
 *	    DPMI_xalloc
 * special virtualalloc, allocates lineary monoton growing memory.
 * (the usual VirtualAlloc does not satisfy that restriction)
 */
static LPVOID
DPMI_xalloc(int len) {
	LPVOID	ret;
	LPVOID	oldlastv = lastvalloced;

	if (lastvalloced) {
		int	xflag = 0;
		ret = NULL;
		while (!ret) {
			ret=VirtualAlloc(lastvalloced,len,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
			if (!ret)
				lastvalloced = (char *) lastvalloced + 0x10000;
			/* we failed to allocate one in the first round.
			 * try non-linear
			 */
			if (!xflag && (lastvalloced<oldlastv)) { /* wrapped */
				FIXME("failed to allocate lineary growing memory (%d bytes), using non-linear growing...\n",len);
				xflag++;
			}
			/* if we even fail to allocate something in the next
			 * round, return NULL
			 */
			if ((xflag==1) && (lastvalloced >= oldlastv))
				xflag++;
			if ((xflag==2) && (lastvalloced < oldlastv)) {
				FIXME("failed to allocate any memory of %d bytes!\n",len);
				return NULL;
			}
		}
	} else
		 ret=VirtualAlloc(NULL,len,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
	lastvalloced = (LPVOID)(((DWORD)ret+len+0xffff)&~0xffff);
	return ret;
}

static void
DPMI_xfree(LPVOID ptr) {
	VirtualFree(ptr,0,MEM_RELEASE);
}

/* FIXME: perhaps we could grow this mapped area... */
static LPVOID
DPMI_xrealloc(LPVOID ptr,DWORD newsize) {
	MEMORY_BASIC_INFORMATION	mbi;
	LPVOID				newptr;

	newptr = DPMI_xalloc(newsize);
	if (ptr) {
		if (!VirtualQuery(ptr,&mbi,sizeof(mbi))) {
			FIXME("realloc of DPMI_xallocd region %p?\n",ptr);
			return NULL;
		}
		if (mbi.State == MEM_FREE) {
			FIXME("realloc of DPMI_xallocd region %p?\n",ptr);
			return NULL;
		}
		/* We do not shrink allocated memory. most reallocs
		 * only do grows anyway
		 */
		if (newsize<=mbi.RegionSize)
			return ptr;
		memcpy(newptr,ptr,mbi.RegionSize);
		DPMI_xfree(ptr);
	}
	return newptr;
}

/**********************************************************************
 *	    CallRMInt
 */
static void CallRMInt( CONTEXT86 *context )
{
    if (!Dosvm.CallRMInt && !DPMI_LoadDosSystem())
    {
        ERR("could not setup real-mode calls\n");
        return;
    }
    Dosvm.CallRMInt( context );
}


static void CallRMProc( CONTEXT86 *context, int iret )
{
    if (!Dosvm.CallRMProc && !DPMI_LoadDosSystem())
    {
        ERR("could not setup real-mode calls\n");
        return;
    }
    Dosvm.CallRMProc( context, iret );
}

static void AllocRMCB( CONTEXT86 *context )
{
    if (!Dosvm.AllocRMCB && !DPMI_LoadDosSystem())
    {
        ERR("could not setup real-mode calls\n");
        return;
    }
    Dosvm.AllocRMCB( context );
}

static void FreeRMCB( CONTEXT86 *context )
{
    if (!Dosvm.FreeRMCB)  /* cannot have allocated one if dosvm not loaded */
    {
        SET_LOWORD( context->Eax, 0x8024 ); /* invalid callback address */
        SET_CFLAG( context );
    }
    else Dosvm.FreeRMCB( context );
}

static void RawModeSwitch( CONTEXT86 *context )
{
    if (!Dosvm.RawModeSwitch)
    {
      ERR("could not setup real-mode calls\n");
      return;
    }
    else
    {
      /*
       * FIXME: This routine will not work if it is called
       *        from 32 bit DPMI program and the program returns
       *        to protected mode while ESP or EIP is over 0xffff.
       * FIXME: This routine will not work if it is not called
       *        using 16-bit-to-Wine callback glue function.
       */
      STACK16FRAME frame = *CURRENT_STACK16;

      Dosvm.RawModeSwitch( context );

      /*
       * After this function returns to relay code, protected mode
       * 16 bit stack will contain STACK16FRAME and single WORD
       * (EFlags, see next comment).
       */
      NtCurrentTeb()->cur_stack =
        MAKESEGPTR( context->SegSs,
                    context->Esp - sizeof(STACK16FRAME) - sizeof(WORD) );

      /*
       * After relay code returns to glue function, protected
       * mode 16 bit stack will contain interrupt return record:
       * IP, CS and EFlags. Since EFlags is ignored, it won't
       * need to be initialized.
       */
      context->Esp -= 3 * sizeof(WORD);

      /*
       * Restore stack frame so that relay code won't be confused.
       * It should be noted that relay code overwrites IP and CS
       * in STACK16FRAME with values taken from current CONTEXT86.
       * These values are what is returned to glue function
       * (see previous comment).
       */
      *CURRENT_STACK16 = frame;
    }
}

/**********************************************************************
 *	    INT_Int31Handler (WPROCS.149)
 *
 * Handler for int 31h (DPMI).
 */

void WINAPI INT_Int31Handler( CONTEXT86 *context )
{
    /*
     * Note: For Win32s processes, the whole linear address space is
     *       shifted by 0x10000 relative to the OS linear address space.
     *       See the comment in msdos/vxd.c.
     */
    DWORD dw;
    BYTE *ptr;

    if (context->SegCs == DOSMEM_dpmi_segments.dpmi_sel) {
        RawModeSwitch( context );
        return;
    }

    RESET_CFLAG(context);
    switch(AX_reg(context))
    {
    case 0x0000:  /* Allocate LDT descriptors */
    	TRACE("allocate LDT descriptors (%d)\n",CX_reg(context));
        if (!(context->Eax = AllocSelectorArray16( CX_reg(context) )))
        {
    	    TRACE("failed\n");
            context->Eax = 0x8011;  /* descriptor unavailable */
            SET_CFLAG(context);
        }
	TRACE("success, array starts at 0x%04x\n",AX_reg(context));
        break;

    case 0x0001:  /* Free LDT descriptor */
    	TRACE("free LDT descriptor (0x%04x)\n",BX_reg(context));
        if (FreeSelector16( BX_reg(context) ))
        {
            context->Eax = 0x8022;  /* invalid selector */
            SET_CFLAG(context);
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
    	TRACE("real mode segment to descriptor (0x%04x)\n",BX_reg(context));
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
	    	context->Eax = DOSMEM_AllocSelector(BX_reg(context));
                break;
            }
            if (entryPoint)
                context->Eax = LOWORD(GetProcAddress16( GetModuleHandle16( "KERNEL" ),
                                                        (LPCSTR)(ULONG_PTR)entryPoint ));
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
    	TRACE("get selector base address (0x%04x)\n",BX_reg(context));
        if (!(dw = GetSelectorBase( BX_reg(context) )))
        {
            context->Eax = 0x8022;  /* invalid selector */
            SET_CFLAG(context);
        }
        else
        {
            SET_CX( context, HIWORD(W32S_WINE2APP(dw)) );
            SET_DX( context, LOWORD(W32S_WINE2APP(dw)) );
        }
        break;

    case 0x0007:  /* Set selector base address */
    	TRACE("set selector base address (0x%04x,0x%08lx)\n",
                     BX_reg(context),
                     W32S_APP2WINE(MAKELONG(DX_reg(context),CX_reg(context))));
        dw = W32S_APP2WINE(MAKELONG(DX_reg(context), CX_reg(context)));
        if (dw < 0x10000)
            /* app wants to access lower 64K of DOS memory, load DOS subsystem */
            DPMI_LoadDosSystem();
        SetSelectorBase(BX_reg(context), dw);
        break;

    case 0x0008:  /* Set selector limit */
    	TRACE("set selector limit (0x%04x,0x%08lx)\n",BX_reg(context),MAKELONG(DX_reg(context),CX_reg(context)));
        dw = MAKELONG( DX_reg(context), CX_reg(context) );
        SetSelectorLimit16( BX_reg(context), dw );
        break;

    case 0x0009:  /* Set selector access rights */
    	TRACE("set selector access rights(0x%04x,0x%04x)\n",BX_reg(context),CX_reg(context));
        SelectorAccessRights16( BX_reg(context), 1, CX_reg(context) );
        break;

    case 0x000a:  /* Allocate selector alias */
    	TRACE("allocate selector alias (0x%04x)\n",BX_reg(context));
        if (!SET_AX( context, AllocCStoDSAlias16( BX_reg(context) )))
        {
            SET_AX( context, 0x8011 );  /* descriptor unavailable */
            SET_CFLAG(context);
        }
        break;

    case 0x000b:  /* Get descriptor */
    	TRACE("get descriptor (0x%04x)\n",BX_reg(context));
        {
            LDT_ENTRY entry;
            wine_ldt_get_entry( LOWORD(context->Ebx), &entry);
            wine_ldt_set_base( &entry, (void*)W32S_WINE2APP(wine_ldt_get_base(&entry)) );
            /* FIXME: should use ES:EDI for 32-bit clients */
            *(LDT_ENTRY *)MapSL( MAKESEGPTR( context->SegEs, LOWORD(context->Edi) )) = entry;
        }
        break;

    case 0x000c:  /* Set descriptor */
    	TRACE("set descriptor (0x%04x)\n",BX_reg(context));
        {
            LDT_ENTRY entry = *(LDT_ENTRY *)MapSL(MAKESEGPTR( context->SegEs, LOWORD(context->Edi)));
            wine_ldt_set_base( &entry, (void*)W32S_APP2WINE(wine_ldt_get_base(&entry)) );
            wine_ldt_set_entry( LOWORD(context->Ebx), &entry );
        }
        break;

    case 0x000d:  /* Allocate specific LDT descriptor */
    	FIXME("allocate descriptor (0x%04x), stub!\n",BX_reg(context));
        SET_AX( context, 0x8011 ); /* descriptor unavailable */
        SET_CFLAG(context);
        break;
    case 0x0100:  /* Allocate DOS memory block */
        TRACE("allocate DOS memory block (0x%x paragraphs)\n",BX_reg(context));
        dw = GlobalDOSAlloc16((DWORD)BX_reg(context)<<4);
        if (dw) {
            SET_AX( context, HIWORD(dw) );
            SET_DX( context, LOWORD(dw) );
        } else {
            SET_AX( context, 0x0008 ); /* insufficient memory */
            SET_BX( context, DOSMEM_Available()>>4 );
            SET_CFLAG(context);
        }
        break;
    case 0x0101:  /* Free DOS memory block */
        TRACE("free DOS memory block (0x%04x)\n",DX_reg(context));
        dw = GlobalDOSFree16(DX_reg(context));
        if (!dw) {
            SET_AX( context, 0x0009 ); /* memory block address invalid */
            SET_CFLAG(context);
        }
        break;
    case 0x0200: /* get real mode interrupt vector */
	FIXME("get realmode interupt vector(0x%02x) unimplemented.\n",
	      BL_reg(context));
    	SET_CFLAG(context);
        break;
    case 0x0201: /* set real mode interrupt vector */
	FIXME("set realmode interrupt vector(0x%02x,0x%04x:0x%04x) unimplemented\n", BL_reg(context),CX_reg(context),DX_reg(context));
    	SET_CFLAG(context);
        break;
    case 0x0204:  /* Get protected mode interrupt vector */
    	TRACE("get protected mode interrupt handler (0x%02x), stub!\n",BL_reg(context));
	dw = (DWORD)INT_GetPMHandler( BL_reg(context) );
	SET_CX( context, HIWORD(dw) );
	SET_DX( context, LOWORD(dw) );
	break;

    case 0x0205:  /* Set protected mode interrupt vector */
    	TRACE("set protected mode interrupt handler (0x%02x,%p), stub!\n",
            BL_reg(context),MapSL(MAKESEGPTR(CX_reg(context),DX_reg(context))));
	INT_SetPMHandler( BL_reg(context),
                          (FARPROC16)MAKESEGPTR( CX_reg(context), DX_reg(context) ));
	break;

    case 0x0300:  /* Simulate real mode interrupt */
        CallRMInt( context );
        break;

    case 0x0301:  /* Call real mode procedure with far return */
	CallRMProc( context, FALSE );
	break;

    case 0x0302:  /* Call real mode procedure with interrupt return */
	CallRMProc( context, TRUE );
        break;

    case 0x0303:  /* Allocate Real Mode Callback Address */
	AllocRMCB( context );
	break;

    case 0x0304:  /* Free Real Mode Callback Address */
	FreeRMCB( context );
	break;

    case 0x0305:  /* Get State Save/Restore Addresses */
        TRACE("get state save/restore addresses\n");
        /* we probably won't need this kind of state saving */
        SET_AX( context, 0 );
	/* real mode: just point to the lret */
	SET_BX( context, DOSMEM_dpmi_segments.wrap_seg );
	context->Ecx = 2;
	/* protected mode: don't have any handler yet... */
	FIXME("no protected-mode dummy state save/restore handler yet\n");
	SET_SI( context, 0 );
	context->Edi = 0;
        break;

    case 0x0306:  /* Get Raw Mode Switch Addresses */
        TRACE("get raw mode switch addresses\n");
        /* real mode, point to standard DPMI return wrapper */
        SET_BX( context, DOSMEM_dpmi_segments.wrap_seg );
        context->Ecx = 0;
        /* protected mode, point to DPMI call wrapper */
        SET_SI( context, DOSMEM_dpmi_segments.dpmi_sel );
        context->Edi = 8; /* offset of the INT 0x31 call */
	break;
    case 0x0400:  /* Get DPMI version */
        TRACE("get DPMI version\n");
    	{
	    SYSTEM_INFO si;

	    GetSystemInfo(&si);
	    SET_AX( context, 0x005a );  /* DPMI version 0.90 */
	    SET_BX( context, 0x0005 );  /* Flags: 32-bit, virtual memory */
	    SET_CL( context, si.wProcessorLevel );
	    SET_DX( context, 0x0102 );  /* Master/slave interrupt controller base*/
	    break;
	}
    case 0x0500:  /* Get free memory information */
        TRACE("get free memory information\n");
        {
            MEMMANINFO mmi;

            mmi.dwSize = sizeof(mmi);
            MemManInfo16(&mmi);
            ptr = MapSL(MAKESEGPTR(context->SegEs,DI_reg(context)));
            /* the layout is just the same as MEMMANINFO, but without
             * the dwSize entry.
             */
            memcpy(ptr,((char*)&mmi)+4,sizeof(mmi)-4);
            break;
        }
    case 0x0501:  /* Allocate memory block */
        TRACE("allocate memory block (%ld)\n",MAKELONG(CX_reg(context),BX_reg(context)));
	if (!(ptr = (BYTE *)DPMI_xalloc(MAKELONG(CX_reg(context), BX_reg(context)))))
        {
            SET_AX( context, 0x8012 );  /* linear memory not available */
            SET_CFLAG(context);
        } else {
            SET_BX( context, HIWORD(W32S_WINE2APP(ptr)) );
            SET_CX( context, LOWORD(W32S_WINE2APP(ptr)) );
            SET_SI( context, HIWORD(W32S_WINE2APP(ptr)) );
            SET_DI( context, LOWORD(W32S_WINE2APP(ptr)) );
        }
        break;

    case 0x0502:  /* Free memory block */
        TRACE("free memory block (0x%08lx)\n",
                     W32S_APP2WINE(MAKELONG(DI_reg(context),SI_reg(context))));
	DPMI_xfree( (void *)W32S_APP2WINE(MAKELONG(DI_reg(context), SI_reg(context))) );
        break;

    case 0x0503:  /* Resize memory block */
        TRACE("resize memory block (0x%08lx,%ld)\n",
                     W32S_APP2WINE(MAKELONG(DI_reg(context),SI_reg(context))),
                     MAKELONG(CX_reg(context),BX_reg(context)));
        if (!(ptr = (BYTE *)DPMI_xrealloc(
                (void *)W32S_APP2WINE(MAKELONG(DI_reg(context),SI_reg(context))),
                MAKELONG(CX_reg(context),BX_reg(context)))))
        {
            SET_AX( context, 0x8012 );  /* linear memory not available */
            SET_CFLAG(context);
        } else {
            SET_BX( context, HIWORD(W32S_WINE2APP(ptr)) );
            SET_CX( context, LOWORD(W32S_WINE2APP(ptr)) );
            SET_SI( context, HIWORD(W32S_WINE2APP(ptr)) );
            SET_DI( context, LOWORD(W32S_WINE2APP(ptr)) );
        }
        break;

    case 0x0507:  /* Modify page attributes */
        FIXME("modify page attributes unimplemented\n");
        break;  /* Just ignore it */

    case 0x0600:  /* Lock linear region */
        FIXME("lock linear region unimplemented\n");
        break;  /* Just ignore it */

    case 0x0601:  /* Unlock linear region */
        FIXME("unlock linear region unimplemented\n");
        break;  /* Just ignore it */

    case 0x0602:  /* Unlock real-mode region */
        FIXME("unlock realmode region unimplemented\n");
        break;  /* Just ignore it */

    case 0x0603:  /* Lock real-mode region */
        FIXME("lock realmode region unimplemented\n");
        break;  /* Just ignore it */

    case 0x0604:  /* Get page size */
        TRACE("get pagesize\n");
        SET_BX( context, 0 );
        SET_CX( context, getpagesize() );
	break;

    case 0x0702:  /* Mark page as demand-paging candidate */
        FIXME("mark page as demand-paging candidate\n");
        break;  /* Just ignore it */

    case 0x0703:  /* Discard page contents */
        FIXME("discard page contents\n");
        break;  /* Just ignore it */

     case 0x0800:  /* Physical address mapping */
        FIXME("map real to linear (0x%08lx)\n",MAKELONG(CX_reg(context),BX_reg(context)));
         if(!(ptr=DOSMEM_MapRealToLinear(MAKELONG(CX_reg(context),BX_reg(context)))))
         {
             SET_AX( context, 0x8021 );
             SET_CFLAG(context);
         }
         else
         {
             SET_BX( context, HIWORD(W32S_WINE2APP(ptr)) );
             SET_CX( context, LOWORD(W32S_WINE2APP(ptr)) );
             RESET_CFLAG(context);
         }
         break;

    default:
        INT_BARF( context, 0x31 );
        SET_AX( context, 0x8001 );  /* unsupported function */
        SET_CFLAG(context);
        break;
    }

}
