/*
 * DOS interrupt 2fh handler
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ldt.h"
#include "drive.h"
#include "msdos.h"
#include "miscemu.h"
#include "module.h"
#include "task.h"
#include "dosexe.h"
/* #define DEBUG_INT */
#include "debug.h"

  /* base WPROCS.DLL ordinal number for VxDs */
#define VXD_BASE 400

static void do_int2f_16( CONTEXT *context );

/**********************************************************************
 *	    INT_Int2fHandler
 *
 * Handler for int 2fh (multiplex).
 */
void WINAPI INT_Int2fHandler( CONTEXT *context )
{
    TRACE(int,"Subfunction 0x%X\n", AH_reg(context));

    switch(AH_reg(context))
    {
    case 0x10:
        AL_reg(context) = 0xff; /* share is installed */
        break;

    case 0x11:  /* Network Redirector / IFSFUNC */
        switch (AL_reg(context))
        {
        case 0x00:  /* Install check */
            /* not installed */
            break;
        case 0x80:  /* Enhanced services - Install check */
            /* not installed */
            break;
        default:
	    INT_BARF( context, 0x2f );
            break;
        }
        break;

    case 0x12:
        switch (AL_reg(context))
        {
        case 0x2e: /* get or set DOS error table address */
            switch (DL_reg(context))
            {
            /* Four tables: even commands are 'get', odd are 'set' */
            /* DOS 5.0+ ignores "set" commands */
            case 0x01:
            case 0x03:
            case 0x05:
            case 0x07:
            case 0x09:
                break; 
            /* Instead of having a message table in DOS-space, */
            /* we can use a special case for MS-DOS to force   */
            /* the secondary interface.			       */
            case 0x00:
            case 0x02:
            case 0x04:
            case 0x06: 
                ES_reg(context) = 0x0001;
                DI_reg(context) = 0x0000;
                break;
            case 0x08:
                FIXME(int, "No real-mode handler for errors yet! (bye!)");
                break;
            default:
                INT_BARF(context, 0x2f);
            }
            break;
        default:
           INT_BARF(context, 0x2f);
        }  
        break;
   
    case 0x15: /* mscdex */
        do_mscdex(context);
        break;

    case 0x16:
        do_int2f_16( context );
        break;

    case 0x1a: /* ANSI.SYS / AVATAR.SYS Install Check */
        /* Not supported yet, do nothing */
        break;

    case 0x43:
#if 1
	switch (AL_reg(context))
	{
	case 0x00:   /* XMS v2+ installation check */
	    WARN(int,"XMS is not fully implemented\n");
	    AL_reg(context) = 0x80;
	    break;
	case 0x10:   /* XMS v2+ get driver address */
	{
            TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
            NE_MODULE *pModule = pTask ? NE_GetPtr( pTask->hModule ) : NULL;
            GlobalUnlock16( GetCurrentTask() );
#ifdef MZ_SUPPORTED
            if (pModule && pModule->lpDosTask)
                ES_reg(context) = pModule->lpDosTask->xms_seg;
            else
#endif
                ES_reg(context) = 0;
            BX_reg(context) = 0;
            break;
	}
	default:
	    INT_BARF( context, 0x2f );
	}
#else
    	FIXME(int,"check for XMS (not supported)\n");
	AL_reg(context) = 0x42; /* != 0x80 */
#endif
    	break;

    case 0x45:
       switch (AL_reg(context)) 
       {
       case 0x00:
       case 0x01:
       case 0x02:
       case 0x03:
       case 0x04:
       case 0x05:
       case 0x06:
       case 0x07:
       case 0x08:
           /* Microsoft Profiler - not installed */
           break;
       default:
            INT_BARF( context, 0x2f );
       }
       break;

    case 0x4a:
        switch(AL_reg(context))
        {
	case 0x10:  /* smartdrv */
	    break;  /* not installed */
        case 0x11:  /* dblspace */
            break;  /* not installed */
        case 0x12:  /* realtime compression interface */
            break;  /* not installed */
        case 0x32:  /* patch IO.SYS (???) */
            break;  /* we have no IO.SYS, so we can't patch it :-/ */
        default:
            INT_BARF( context, 0x2f );
        }
        break;
    case 0x56:  /* INTERLNK */
	switch(AL_reg(context))
	{
	case 0x01:  /* check if redirected drive */
	    AL_reg(context) = 0; /* not redirected */
	    break;
	default:
	    INT_BARF( context, 0x2f );
	}
	break;
    case 0x7a:  /* NOVELL NetWare */
        switch (AL_reg(context))
        {
        case 0x20:  /* Get VLM Call Address */
            /* return nothing -> NetWare not installed */
            break;
        default:
	    INT_BARF( context, 0x2f );
            break;
        }
        break;
    case 0xb7:  /* append */
        AL_reg(context) = 0; /* not installed */
        break;
    case 0xb8:  /* network */
        switch (AL_reg(context))
        {
        case 0x00:  /* Install check */
            /* not installed */
            break;
        default:
	    INT_BARF( context, 0x2f );
            break;
        }
        break;
    case 0xbd:  /* some Novell network install check ??? */
        AX_reg(context) = 0xa5a5; /* pretend to have Novell IPX installed */
	break;
    case 0xbf:  /* REDIRIFS.EXE */
        switch (AL_reg(context))
        {
        case 0x00:  /* Install check */
            /* not installed */
            break;
        default:
	    INT_BARF( context, 0x2f );
            break;
        }
        break;
    case 0xd7:  /* Banyan Vines */
        switch (AL_reg(context))
        {
        case 0x01:  /* Install check - Get Int Number */
            /* not installed */
            break;
        default:
	    INT_BARF( context, 0x2f );
            break;
        }
        break;
    case 0xfa:  /* Watcom debugger check, returns 0x666 if installed */
        break;
    default:
        INT_BARF( context, 0x2f );
        break;
    }
}


/**********************************************************************
 *	    do_int2f_16
 */
static void do_int2f_16( CONTEXT *context )
{
    DWORD addr;

    switch(AL_reg(context))
    {
    case 0x00:  /* Windows enhanced mode installation check */
        AX_reg(context) = (GetWinFlags() & WF_ENHANCED) ?
                                                  LOWORD(GetVersion16()) : 0;
        break;
	
    case 0x0a:  /* Get Windows version and type */
        AX_reg(context) = 0;
        BX_reg(context) = (LOWORD(GetVersion16()) << 8) |
                          (LOWORD(GetVersion16()) >> 8);
        CX_reg(context) = (GetWinFlags() & WF_ENHANCED) ? 3 : 2;
        break;

    case 0x0b:  /* Identify Windows-aware TSRs */
        /* we don't have any pre-Windows TSRs */
        break;

    case 0x11:  /* Get Shell Parameters - (SHELL= in CONFIG.SYS) */
        /* We can mock this up. But not today... */ 
        FIXME(int, "Get Shell Parameters\n");       
        break;

    case 0x80:  /* Release time-slice */
	AL_reg(context) = 0;
	/* FIXME: We need to do something that lets some other process run
	   here.  */
	sleep(0);
        break;

    case 0x81: /* Begin critical section.  */
        /* FIXME? */
        break;

    case 0x82: /* End critical section.  */
        /* FIXME? */
        break;

    case 0x83:  /* Return Current Virtual Machine ID */
        /* Virtual Machines are usually created/destroyed when Windows runs
         * DOS programs. Since we never do, we are always in the System VM.
         * According to Ralf Brown's Interrupt List, never return 0. But it
         * seems to work okay (returning 0), just to be sure we return 1.
         */
	BX_reg(context) = 1; /* VM 1 is probably the System VM */
	break;

    case 0x84:  /* Get device API entry point */
        addr = (DWORD)NE_GetEntryPoint( GetModuleHandle16("WPROCS"),
                                        VXD_BASE + BX_reg(context) );
        if (!addr)  /* not supported */
        {
	    ERR(int,"Accessing unknown VxD %04x - Expect a failure now.\n",
                     BX_reg(context) );
        }
	ES_reg(context) = SELECTOROF(addr);
	DI_reg(context) = OFFSETOF(addr);
	break;

    case 0x86:  /* DPMI detect mode */
        AX_reg(context) = 0;  /* Running under DPMI */
        break;

    case 0x87: /* DPMI installation check */
#if 1   /* DPMI still breaks pkunzip */
        if (ISV86(context)) break; /* so bail out for now if in v86 mode */
#endif
        {
            TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
            NE_MODULE *pModule = pTask ? NE_GetPtr( pTask->hModule ) : NULL;
	    SYSTEM_INFO si;

            GlobalUnlock16( GetCurrentTask() );
	    GetSystemInfo(&si);
	    AX_reg(context) = 0x0000; /* DPMI Installed */
            BX_reg(context) = 0x0001; /* 32bits available */
            CL_reg(context) = si.wProcessorLevel;
            DX_reg(context) = 0x005a; /* DPMI major/minor 0.90 */
            SI_reg(context) = 0;      /* # of para. of DOS extended private data */
#ifdef MZ_SUPPORTED                   /* ES:DI is DPMI switch entry point */
            if (pModule && pModule->lpDosTask)
                ES_reg(context) = pModule->lpDosTask->dpmi_seg;
            else
#endif
                ES_reg(context) = 0;
            DI_reg(context) = 0;
            break;
        }
    case 0x8a:  /* DPMI get vendor-specific API entry point. */
	/* The 1.0 specs say this should work with all 0.9 hosts.  */
	break;

    default:
        INT_BARF( context, 0x2f );
    }
}

void do_mscdex( CONTEXT *context )
{
    int drive, count;
    char *p;

    switch(AL_reg(context))
    {
        case 0x00: /* Installation check */
            /* Count the number of contiguous CDROM drives
             */
            for (drive = count = 0; drive < MAX_DOS_DRIVES; drive++)
            {
                if (DRIVE_GetType(drive) == TYPE_CDROM)
                {
                    while (DRIVE_GetType(drive + count) == TYPE_CDROM) count++;
                    break;
                }
            }

            BX_reg(context) = count;
            CX_reg(context) = (drive < MAX_DOS_DRIVES) ? drive : 0;
            break;

        case 0x0B: /* drive check */
            AX_reg(context) = (DRIVE_GetType(CX_reg(context)) == TYPE_CDROM);
            BX_reg(context) = 0xADAD;
            break;

        case 0x0C: /* get version */
            BX_reg(context) = 0x020a;
            break;

        case 0x0D: /* get drive letters */
	    p = CTX_SEG_OFF_TO_LIN(context, ES_reg(context), BX_reg(context));
            memset( p, 0, MAX_DOS_DRIVES );
            for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
            {
                if (DRIVE_GetType(drive) == TYPE_CDROM) *p++ = drive;
            }
            break;

#ifdef linux
        /* FIXME: why a new linux-only CDROM drive access, for crying out loud?
         * There are pretty complete routines in multimedia/mcicda.c already! */
	case 0x10: /* direct driver acces */
            FIXME(cdaudio,"mscdex should use multimedia/mcicda.c");
	    do_mscdex_dd(context,ISV86(context));
	    break;

#endif

        default:
            FIXME(int, "Unimplemented MSCDEX function 0x%02X.\n", AL_reg(context));
            break;
    }
}
