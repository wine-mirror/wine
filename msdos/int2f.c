/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * DOS interrupt 2fh handler
 *
 *  Cdrom - device driver emulation - Audio features.
 * 	(c) 1998 Petr Tomasek <tomasek@etf.cuni.cz>
 *	(c) 1999 Eric Pouech
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "wine/winbase16.h"
#include "ldt.h"
#include "drive.h"
#include "msdos.h"
#include "miscemu.h"
#include "module.h"
#include "task.h"
#include "dosexe.h"
#include "heap.h"
/* #define DEBUG_INT */
#include "debug.h"
#include "cdrom.h"

/* base WPROCS.DLL ordinal number for VxDs */
#define VXD_BASE 400

static void do_int2f_16( CONTEXT *context );
static void do_mscdex( CONTEXT *context );

/**********************************************************************
 *	    INT_Int2fHandler
 *
 * Handler for int 2fh (multiplex).
 */
void WINAPI INT_Int2fHandler( CONTEXT *context )
{
    TRACE(int,"Subfunction 0x%X\n", AX_reg(context));

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
	case 0x0:  /* Low-level Netware installation check AL=0 not installed.*/
            AL_reg(context) = 0;    
            break;
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
        AX_reg(context) = (GetWinFlags16() & WF_ENHANCED) ?
                                                  LOWORD(GetVersion16()) : 0;
        break;
	
    case 0x0a:  /* Get Windows version and type */
        AX_reg(context) = 0;
        BX_reg(context) = (LOWORD(GetVersion16()) << 8) |
                          (LOWORD(GetVersion16()) >> 8);
        CX_reg(context) = (GetWinFlags16() & WF_ENHANCED) ? 3 : 2;
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
    int 	drive, count;
    char*	p;

    switch(AL_reg(context)) {
    case 0x00: /* Installation check */
	/* Count the number of contiguous CDROM drives
	 */
	for (drive = count = 0; drive < MAX_DOS_DRIVES; drive++) {
	    if (DRIVE_GetType(drive) == TYPE_CDROM) {
		while (DRIVE_GetType(drive + count) == TYPE_CDROM) count++;
		break;
	    }
	}
	TRACE(int, "Installation check: %d cdroms, starting at %d\n", count, drive);
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
	p = CTX_SEG_OFF_TO_LIN(context, ES_reg(context), EBX_reg(context));
	memset( p, 0, MAX_DOS_DRIVES );
	for (drive = 0; drive < MAX_DOS_DRIVES; drive++) {
	    if (DRIVE_GetType(drive) == TYPE_CDROM) *p++ = drive;
	}
	break;
	
    case 0x10: /* direct driver acces */
    {
	static 	WINE_CDAUDIO	wcda;
	BYTE* 	driver_request;
	BYTE* 	io_stru;	
	u_char 	Error = 255; /* No Error */ 
	int	dorealmode = ISV86(context);
	
	driver_request = (dorealmode) ? 
	    DOSMEM_MapRealToLinear(MAKELONG(BX_reg(context),ES_reg(context))) : 
	    PTR_SEG_OFF_TO_LIN(ES_reg(context),BX_reg(context));
	
	if (!driver_request) {
	    /* FIXME - to be deleted ?? */
	    ERR(int,"   ES:BX==0 ! SEGFAULT ?\n");
	    ERR(int," -->BX=0x%04x, ES=0x%04lx, DS=0x%04lx, CX=0x%04x\n",
		BX_reg(context), ES_reg(context), DS_reg(context), CX_reg(context));
	} else {
	    /* FIXME - would be best to open the device at the begining of the wine session .... 
	     */
	    if (wcda.unixdev <= 0) 
		CDAUDIO_Open(&wcda);
	}
	TRACE(int,"CDROM device driver -> command <%d>\n", (unsigned char)driver_request[2]);
	
	/* set status to 0 */
	driver_request[3] = driver_request[4] = 0;
	CDAUDIO_GetCDStatus(&wcda);
	
	switch (driver_request[2]) {
	case 3:
	    io_stru = (dorealmode) ? 
		DOSMEM_MapRealToLinear(MAKELONG(*((WORD*)(driver_request + 14)), *((WORD*)(driver_request + 16)))) :
		    PTR_SEG_OFF_TO_LIN(*((WORD*)(driver_request + 16)), *((WORD*)(driver_request + 18)));
	    
	    TRACE(int," --> IOCTL INPUT <%d>\n", io_stru[0]); 
	    switch (io_stru[0]) {
#if 0
	    case 0: /* Get device Header */
	    {
		static	LPSTR ptr = 0;
		if (ptr == 0)	{
		    ptr = SEGPTR_ALLOC(22);
		    *((DWORD*)(ptr     )) = ~1;		/* Next Device Driver */
		    *((WORD* )(ptr +  4)) = 0xC800;	/* Device attributes  */
		    *((WORD* )(ptr +  6)) = 0x1234;	/* Pointer to device strategy routine: FIXME */
		    *((WORD* )(ptr +  8)) = 0x3142;	/* Pointer to device interrupt routine: FIXME */
		    *((char*) (ptr + 10)) = 'W';  	/* 8-byte character device name field */
		    *((char*) (ptr + 11)) = 'I';
		    *((char*) (ptr + 12)) = 'N';
		    *((char*) (ptr + 13)) = 'E';
		    *((char*) (ptr + 14)) = '_';
		    *((char*) (ptr + 15)) = 'C';
		    *((char*) (ptr + 16)) = 'D';
		    *((char*) (ptr + 17)) = '_';
		    *((WORD*) (ptr + 18)) = 0;		/* Reserved (must be zero) */
		    *((BYTE*) (ptr + 20)) = 0;          /* Drive letter (must be zero) */
		    *((BYTE*) (ptr + 21)) = 1;          /* Number of units supported (one or more) FIXME*/
		}
		((DWORD*)io_stru+1)[0] = SEGPTR_GET(ptr);
	    }
	    break;
#endif
	    
	    case 1: /* location of head */
		if (io_stru[1] == 0) {
		    /* FIXME: what if io_stru+2 is not DWORD aligned ? */
		    ((DWORD*)io_stru+2)[0] = wcda.dwCurFrame;
		    TRACE(int," ----> HEAD LOCATION <%ld>\n", ((DWORD*)io_stru+2)[0]); 
		} else {
		    ERR(int,"CDRom-Driver: Unsupported addressing mode !!\n");
		    Error = 0x0c;
		}
		break;
		
	    case 4: /* Audio channel info */
		io_stru[1] = 0;
		io_stru[2] = 0xff;
		io_stru[3] = 1;
		io_stru[4] = 0xff;
		io_stru[5] = 2;
		io_stru[6] = 0xff;
		io_stru[7] = 3;
		io_stru[8] = 0xff;
		TRACE(int," ----> AUDIO CHANNEL CONTROL\n"); 
		break;
		
	    case 6: /* device status */
		/* FIXME .. does this work properly ?? */
		io_stru[3] = io_stru[4] = 0;
		io_stru[2] = 1;  /* supports audio channels (?? FIXME ??) */
		io_stru[1] = 16; /* data read and plays audio tracks */
		if (wcda.cdaMode == WINE_CDA_OPEN)
		    io_stru[1] |= 1;
		TRACE(int," ----> DEVICE STATUS <0x%08lx>\n", (DWORD)io_stru[1]); 
		break;
		
	    case 8: /* Volume size */
		*((DWORD*)(io_stru+1)) = wcda.dwTotalLen;
		TRACE(int," ----> VOLMUE SIZE <0x%08lx>\n", *((DWORD*)(io_stru+1))); 
		break;
		
	    case 9: /* media changed ? */
		/* answers don't know... -1/1 for yes/no would be better */
		io_stru[0] = 0; /* FIXME? 1? */
		break;
		
	    case 10: /* audio disk info */
		io_stru[1] = wcda.nFirstTrack; /* starting track of the disc */
		io_stru[2] = wcda.nLastTrack; /* ending track */
		((DWORD*)io_stru+3)[0] = wcda.dwFirstOffset;
		TRACE(int," ----> AUDIO DISK INFO <%d-%d/%ld>\n",
		      io_stru[1], io_stru[2], ((DWORD *)io_stru+3)[0]); 
		break;
		
	    case 11: /* audio track info */
		((DWORD*)io_stru+2)[0] = wcda.lpdwTrackPos[io_stru[1]];
		/* starting point if the track */
		io_stru[6] = wcda.lpbTrackFlags[io_stru[1]];
		TRACE(int," ----> AUDIO TRACK INFO <track=%d>[%ld:%d]\n",
		      io_stru[1],((DWORD *)io_stru+2)[0], io_stru[6]); 
		break;
		
	    case 12: /* get Q-Channel / Subchannel (??) info */
		io_stru[ 1] = wcda.lpbTrackFlags[wcda.nCurTrack];
		io_stru[ 2] = wcda.nCurTrack;
		io_stru[ 3] = 0; /* FIXME ?? */
		{
		    DWORD  f = wcda.lpdwTrackPos[wcda.nCurTrack] - wcda.dwCurFrame;
		    io_stru[ 4] = f / CDFRAMES_PERMIN;
		    io_stru[ 5] = (f - CDFRAMES_PERMIN * io_stru[4]) / CDFRAMES_PERSEC;
		    io_stru[ 6] = f - CDFRAMES_PERMIN * io_stru[4] - CDFRAMES_PERSEC * io_stru[5];
		}
		io_stru[ 7] = 0;
		{
		    DWORD  f = wcda.dwCurFrame;
		    io_stru[ 8] = f / CDFRAMES_PERMIN;
		    io_stru[ 9] = (f - CDFRAMES_PERMIN * io_stru[4]) / CDFRAMES_PERSEC;
		    io_stru[10] = f - CDFRAMES_PERMIN * io_stru[4] - CDFRAMES_PERSEC * io_stru[5];
		}
		break;
		
	    case 15: /* Audio status info */
		/* !!!! FIXME FIXME FIXME !! */
		*((WORD*)(io_stru+1))  = (wcda.cdaMode == WINE_CDA_PAUSE);
		*((DWORD*)(io_stru+3)) = wcda.lpdwTrackPos[0];
		*((DWORD*)(io_stru+7)) = wcda.lpdwTrackPos[wcda.nTracks - 1];
		break;
		
	    default:
		FIXME(int," Cdrom-driver: IOCTL INPUT: Unimplemented <%d>!!\n", io_stru[0]); 
		Error=0x0c; 
		break;	
	    }	
	    break;
	    
	case 12:
	    io_stru = (dorealmode) ? 
		DOSMEM_MapRealToLinear(MAKELONG(*((WORD*)(driver_request + 14)), *((WORD*)(driver_request + 16)))) :
		    PTR_SEG_OFF_TO_LIN(*((WORD*)(driver_request + 16)), *((WORD*)(driver_request + 18)));
	    
	    TRACE(int," --> IOCTL OUTPUT <%d>\n",io_stru[0]); 
	    switch (io_stru[0]) {
	    case 0: /* eject */ 
		CDAUDIO_SetDoor(&wcda, 1);
		TRACE(int," ----> EJECT\n"); 
		break;
	    case 2: /* reset drive */
		CDAUDIO_Reset(&wcda);
		TRACE(int," ----> RESET\n"); 
		break;
	    case 3: /* Audio Channel Control */
		FIXME(int, " ----> AUDIO CHANNEL CONTROL (NIY)\n");
		break;
	    case 5: /* close tray */
		CDAUDIO_SetDoor(&wcda, 0);
		TRACE(int," ----> CLOSE TRAY\n"); 
		break;
	    default:
		FIXME(int," Cdrom-driver: IOCTL OUPUT: Unimplemented <%d>!!\n",
		      io_stru[0]); 
		Error=0x0c;
		break;	
	    }	
	    break;
	    
	case 132:  /* FIXME - It didn't function for me... */
	    TRACE(int," --> PLAY AUDIO\n");
	    if (driver_request[13] == 0) {
		TRACE(int,"Mode :<0x%02X> , [%ld-%ld]\n",
		      (unsigned char)driver_request[13],
		      ((DWORD*)driver_request+14)[0],
		      ((DWORD*)driver_request+18)[0]);
		CDAUDIO_Play(&wcda, ((DWORD*)driver_request+14)[0], ((DWORD*)driver_request+14)[0] + ((DWORD*)driver_request+18)[0]);
	    } else {
		ERR(int, "CDRom-Driver: Unsupported address mode !!\n");
		Error=0x0c;
	    }
	    break;
	    
	case 133:
	    if (wcda.cdaMode == WINE_CDA_PLAY) {
		CDAUDIO_Pause(&wcda, 1);
		TRACE(int," --> STOP AUDIO (Paused)\n");
	    } else {
		CDAUDIO_Stop(&wcda);
		TRACE(int," --> STOP AUDIO (Stopped)\n");
	    }
	    break;
	    
	case 136:
	    TRACE(int," --> RESUME AUDIO\n");
	    CDAUDIO_Pause(&wcda, 0);
	    break;
	    
	default:
	    FIXME(int," CDRom-Driver - ioctl uninplemented <%d>\n",driver_request[2]); 
	    Error=0x0c;	
	}
	
	if (Error<255) {
	    driver_request[4] |= 127;
	    driver_request[3] = Error;
	}
	driver_request[4] |= 2 * (wcda.cdaMode = WINE_CDA_PLAY);
	
	/*  close (fdcd); FIXME !! -- cannot use close when ejecting 
	    the cd-rom - close would close it again */ 
    }
    break;
    default:
	FIXME(int, "Unimplemented MSCDEX function 0x%02X.\n", AL_reg(context));
	break;
    }
}
