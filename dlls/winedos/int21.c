/*
 * DOS interrupt 21h handler
 *
 * Copyright 1993, 1994 Erik Bos
 * Copyright 1996 Alexandre Julliard
 * Copyright 1997 Andreas Mohr
 * Copyright 1998 Uwe Bonnes
 * Copyright 1998, 1999 Ove Kaaven
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

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/winbase16.h"
#include "dosexe.h"
#include "miscemu.h"
#include "msdos.h"
#include "file.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int21);

void WINAPI DOSVM_Int21Handler_Ioctl( CONTEXT86 *context )
{
  static const WCHAR emmxxxx0W[] = {'E','M','M','X','X','X','X','0',0};
  const DOS_DEVICE *dev = DOSFS_GetDeviceByHandle(
      DosFileHandleToWin32Handle(BX_reg(context)) );

  if (dev && !strcmpiW( dev->name, emmxxxx0W )) {
    EMS_Ioctl_Handler(context);
    return;
  }

  switch (AL_reg(context))
  {
  case 0x0b: /* SET SHARING RETRY COUNT */
      TRACE("IOCTL - SET SHARING RETRY COUNT pause %d retries %d\n",
           CX_reg(context), DX_reg(context));
      if (!CX_reg(context))
      {
         SET_AX( context, 1 );
         SET_CFLAG(context);
         break;
      }
      DOSMEM_LOL()->sharing_retry_delay = CX_reg(context);
      if (!DX_reg(context))
         DOSMEM_LOL()->sharing_retry_count = DX_reg(context);
      RESET_CFLAG(context);
      break;
  default:
      DOS3Call( context );
  }
}

/***********************************************************************
 *           DOSVM_Int21Handler (WINEDOS16.133)
 *
 * int 21h handler. Most calls are passed directly to DOS3Call.
 */
void WINAPI DOSVM_Int21Handler( CONTEXT86 *context )
{
    BYTE ascii;

    if (DOSVM_IsWin16()) {
        DOS3Call( context );
        return;
    }

    RESET_CFLAG(context);  /* Not sure if this is a good idea */

    if(AH_reg(context) == 0x0c) /* FLUSH BUFFER AND READ STANDARD INPUT */
    {
        BYTE al = AL_reg(context); /* Input function to execute after flush. */

        /* FIXME: buffers are not flushed */

        /*
         * If AL is not one of 0x01, 0x06, 0x07, 0x08, or 0x0a,
         * the buffer is flushed but no input is attempted.
         */
        if(al != 0x01 && al != 0x06 && al != 0x07 && al != 0x08 && al != 0x0a)
            return;

        SET_AH( context, al );
    }

    switch(AH_reg(context))
    {
    case 0x00: /* TERMINATE PROGRAM */
        TRACE("TERMINATE PROGRAM\n");
        MZ_Exit( context, FALSE, 0 );
        break;

    case 0x01: /* READ CHARACTER FROM STANDARD INPUT, WITH ECHO */
        TRACE("DIRECT CHARACTER INPUT WITH ECHO\n");
        DOSVM_Int16ReadChar(&ascii, NULL, FALSE);
        SET_AL( context, ascii );
        DOSVM_PutChar(AL_reg(context));
        break;

    case 0x02: /* WRITE CHARACTER TO STANDARD OUTPUT */
        TRACE("Write Character to Standard Output\n");
        DOSVM_PutChar(DL_reg(context));
        break;

    case 0x06: /* DIRECT CONSOLE IN/OUTPUT */
        /* FIXME: Use DOSDEV_Peek/Read/Write(DOSDEV_Console(),...) !! */
        if (DL_reg(context) == 0xff) {
            static char scan = 0;
            TRACE("Direct Console Input\n");
            if (scan) {
                /* return pending scancode */
                SET_AL( context, scan );
                RESET_ZFLAG(context);
                scan = 0;
            } else {
                if (DOSVM_Int16ReadChar(&ascii,&scan,TRUE)) {
                    DOSVM_Int16ReadChar(&ascii,&scan,FALSE);
                    /* return ASCII code */
                    SET_AL( context, ascii );
                    RESET_ZFLAG(context);
                    /* return scan code on next call only if ascii==0 */
                    if (ascii) scan = 0;
                } else {
                    /* nothing pending, clear everything */
                    SET_AL( context, 0 );
                    SET_ZFLAG(context);
                    scan = 0; /* just in case */
                }
            }
        } else {
            TRACE("Direct Console Output\n");
            DOSVM_PutChar(DL_reg(context));
        }
        break;

    case 0x07: /* DIRECT CHARACTER INPUT WITHOUT ECHO */
        /* FIXME: Use DOSDEV_Peek/Read(DOSDEV_Console(),...) !! */
        TRACE("DIRECT CHARACTER INPUT WITHOUT ECHO\n");
        DOSVM_Int16ReadChar(&ascii, NULL, FALSE);
        SET_AL( context, ascii );
        break;

    case 0x08: /* CHARACTER INPUT WITHOUT ECHO */
        /* FIXME: Use DOSDEV_Peek/Read(DOSDEV_Console(),...) !! */
        TRACE("CHARACTER INPUT WITHOUT ECHO\n");
        DOSVM_Int16ReadChar(&ascii, NULL, FALSE);
        SET_AL( context, ascii );
        break;

    case 0x0b: /* GET STDIN STATUS */
        {
            BIOSDATA *data = BIOS_DATA;
            if(data->FirstKbdCharPtr == data->NextKbdCharPtr)
                SET_AL( context, 0 );
            else
                SET_AL( context, 0xff );
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
            SET_BX( context, OFFSETOF(addr) );
        }
        break;

    case 0x40: /* WRITE TO FILE OR DEVICE */
        /* Writes to stdout are handled here. */
        if (BX_reg(context) == 1) {
          BYTE *ptr = CTX_SEG_OFF_TO_LIN(context,
                                         context->SegDs,
                                         context->Edx);
          int i;

          for(i=0; i<CX_reg(context); i++)
            DOSVM_PutChar(ptr[i]);
        } else
          DOS3Call( context );
        break;

    case 0x44: /* IOCTL */
        DOSVM_Int21Handler_Ioctl( context );
        break;

    case 0x4b: /* "EXEC" - LOAD AND/OR EXECUTE PROGRAM */
        TRACE("EXEC %s\n", (LPCSTR)CTX_SEG_OFF_TO_LIN(context, context->SegDs, context->Edx ));
        if (!MZ_Exec( context, CTX_SEG_OFF_TO_LIN(context, context->SegDs, context->Edx),
                      AL_reg(context), CTX_SEG_OFF_TO_LIN(context, context->SegEs, context->Ebx) ))
        {
            SET_AX( context, GetLastError() );
            SET_CFLAG(context);
        }
        break;

    case 0x4c: /* "EXIT" - TERMINATE WITH RETURN CODE */
        TRACE("EXIT with return code %d\n",AL_reg(context));
        MZ_Exit( context, FALSE, AL_reg(context) );
        break;

    case 0x4d: /* GET RETURN CODE */
        TRACE("GET RETURN CODE (ERRORLEVEL)\n");
        SET_AX( context, DOSVM_retval );
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
        SET_BX( context, DOSVM_psp );
        break;

    case 0x52: /* "SYSVARS" - GET LIST OF LISTS */
        TRACE("SYSVARS - GET LIST OF LISTS\n");
        {
            context->SegEs = HIWORD(DOS_LOLSeg);
            SET_BX( context, FIELD_OFFSET(DOS_LISTOFLISTS, ptr_first_DPB) );
        }
        break;

    case 0x62: /* GET PSP ADDRESS */
        TRACE("GET CURRENT PSP ADDRESS\n");
        /* FIXME: should we return the original DOS PSP upon */
        /*        Windows startup ? */
        SET_BX( context, DOSVM_psp );
        break;

    default:
        DOS3Call( context );
    }
}
