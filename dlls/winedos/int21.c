/*
 * DOS interrupt 21h handler
 */

#include "config.h"

#include "windef.h"
#include "winbase.h"
#include "ntddk.h"
#include "wine/winbase16.h"
#include "dosexe.h"
#include "miscemu.h"
#include "msdos.h"
#include "console.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(int21);


/***********************************************************************
 *           DOSVM_Int21Handler
 *
 * int 21h real-mode handler. Most calls are passed directly to DOS3Call.
 */
void WINAPI DOSVM_Int21Handler( CONTEXT86 *context )
{
    RESET_CFLAG(context);  /* Not sure if this is a good idea */

    switch(AH_reg(context))
    {
    case 0x00: /* TERMINATE PROGRAM */
        TRACE("TERMINATE PROGRAM\n");
        MZ_Exit( context, FALSE, 0 );
        break;

    case 0x01: /* READ CHARACTER FROM STANDARD INPUT, WITH ECHO */
        TRACE("DIRECT CHARACTER INPUT WITH ECHO\n");
	AL_reg(context) = CONSOLE_GetCharacter();
	/* FIXME: no echo */
	break;

    case 0x02: /* WRITE CHARACTER TO STANDARD OUTPUT */
        TRACE("Write Character to Standard Output\n");
    	CONSOLE_Write(DL_reg(context), 0, 0, 0);
        break;

    case 0x06: /* DIRECT CONSOLE IN/OUTPUT */
        /* FIXME: Use DOSDEV_Peek/Read/Write(DOSDEV_Console(),...) !! */
        if (DL_reg(context) == 0xff) {
            static char scan = 0;
            TRACE("Direct Console Input\n");
            if (scan) {
                /* return pending scancode */
                AL_reg(context) = scan;
                RESET_ZFLAG(context);
                scan = 0;
            } else {
                char ascii;
                if (DOSVM_Int16ReadChar(&ascii,&scan,TRUE)) {
                    DOSVM_Int16ReadChar(&ascii,&scan,FALSE);
                    /* return ASCII code */
                    AL_reg(context) = ascii;
                    RESET_ZFLAG(context);
                    /* return scan code on next call only if ascii==0 */
                    if (ascii) scan = 0;
                } else {
                    /* nothing pending, clear everything */
                    AL_reg(context) = 0;
                    SET_ZFLAG(context);
                    scan = 0; /* just in case */
                }
            }
        } else {
            TRACE("Direct Console Output\n");
            CONSOLE_Write(DL_reg(context), 0, 0, 0);
        }
        break;

    case 0x07: /* DIRECT CHARACTER INPUT WITHOUT ECHO */
        /* FIXME: Use DOSDEV_Peek/Read(DOSDEV_Console(),...) !! */
        TRACE("DIRECT CHARACTER INPUT WITHOUT ECHO\n");
        DOSVM_Int16ReadChar(&AL_reg(context), NULL, FALSE);
        break;

    case 0x08: /* CHARACTER INPUT WITHOUT ECHO */
        /* FIXME: Use DOSDEV_Peek/Read(DOSDEV_Console(),...) !! */
        TRACE("CHARACTER INPUT WITHOUT ECHO\n");
        DOSVM_Int16ReadChar(&AL_reg(context), NULL, FALSE);
        break;

    case 0x0b: /* GET STDIN STATUS */
        {
            char x1,x2;

            if (CONSOLE_CheckForKeystroke(&x1,&x2))
                AL_reg(context) = 0xff;
            else
                AL_reg(context) = 0;
        }
        break;

    case 0x25: /* SET INTERRUPT VECTOR */
        DOSVM_SetRMHandler( AL_reg(context),
                            (FARPROC16)MAKESEGPTR( context->SegDs, DX_reg(context)));
        break;

    case 0x35: /* GET INTERRUPT VECTOR */
        TRACE("GET INTERRUPT VECTOR 0x%02x\n",AL_reg(context));
        {
            FARPROC16 addr = DOSVM_GetRMHandler( AL_reg(context) );
            context->SegEs = SELECTOROF(addr);
            BX_reg(context) = OFFSETOF(addr);
        }
        break;

    case 0x44: /* IOCTL */
        switch (AL_reg(context))
        {
        case 0x0b:   /* SET SHARING RETRY COUNT */
            TRACE("IOCTL - SET SHARING RETRY COUNT pause %d retries %d\n",
                  CX_reg(context), DX_reg(context));
            if (!CX_reg(context))
            {
                AX_reg(context) = 1;
                SET_CFLAG(context);
                break;
            }
            DOSMEM_LOL()->sharing_retry_delay = CX_reg(context);
            if (!DX_reg(context))
                DOSMEM_LOL()->sharing_retry_count = DX_reg(context);
            RESET_CFLAG(context);
            break;
        }
        break;

    case 0x4b: /* "EXEC" - LOAD AND/OR EXECUTE PROGRAM */
        TRACE("EXEC %s\n", (LPCSTR)CTX_SEG_OFF_TO_LIN(context, context->SegDs, context->Edx ));
        if (!MZ_Exec( context, CTX_SEG_OFF_TO_LIN(context, context->SegDs, context->Edx),
                      AL_reg(context), CTX_SEG_OFF_TO_LIN(context, context->SegEs, context->Ebx) ))
        {
            AX_reg(context) = GetLastError();
            SET_CFLAG(context);
        }
        break;

    case 0x4c: /* "EXIT" - TERMINATE WITH RETURN CODE */
        TRACE("EXIT with return code %d\n",AL_reg(context));
        MZ_Exit( context, FALSE, AL_reg(context) );
        break;

    case 0x4d: /* GET RETURN CODE */
        TRACE("GET RETURN CODE (ERRORLEVEL)\n");
        AX_reg(context) = DOSVM_retval;
        DOSVM_retval = 0;
        break;

    case 0x50: /* SET CURRENT PROCESS ID (SET PSP ADDRESS) */
        TRACE("SET CURRENT PROCESS ID (SET PSP ADDRESS)\n");
        DOSVM_psp = BX_reg(context);
        break;

    case 0x51: /* GET PSP ADDRESS */
        TRACE("GET CURRENT PROCESS ID (GET PSP ADDRESS)\n");
        /* FIXME: should we return the original DOS PSP upon */
        /*        Windows startup ? */
        BX_reg(context) = DOSVM_psp;
        break;

    case 0x52: /* "SYSVARS" - GET LIST OF LISTS */
        TRACE("SYSVARS - GET LIST OF LISTS\n");
        {
            context->SegEs = HIWORD(DOS_LOLSeg);
            BX_reg(context) = FIELD_OFFSET(DOS_LISTOFLISTS, ptr_first_DPB);
        }
        break;

    case 0x62: /* GET PSP ADDRESS */
        TRACE("GET CURRENT PSP ADDRESS\n");
        /* FIXME: should we return the original DOS PSP upon */
        /*        Windows startup ? */
        BX_reg(context) = DOSVM_psp;
        break;

    default:
        DOS3Call( context );
    }
}
