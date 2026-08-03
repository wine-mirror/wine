/*
 * BIOS interrupt 13h handler
 *
 * Copyright 1997 Andreas Mohr
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

#include <stdlib.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#include <fcntl.h>
#ifdef linux
#ifdef HAVE_LINUX_COMPILER_H
#include <linux/compiler.h>
#endif
# include <linux/fd.h>
#endif

#include "dosexe.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);


/*
 * Status of last int13 operation.
 */
static BYTE INT13_last_status;


/**********************************************************************
 *         INT13_SetStatus
 *
 * Write status to AH register and set carry flag on error (AH != 0).
 *
 * Despite what Ralf Brown says, at least functions 0x06 and 0x07 
 * seem to set carry, too.
 */
static void INT13_SetStatus( CONTEXT *context, BYTE status )
{
    INT13_last_status = status;

    SET_AH( context, status );

    if (status)
        SET_CFLAG( context );
    else
        RESET_CFLAG( context );        
}


/**********************************************************************
 *	    INT13_ReadFloppyParams
 *
 * Read floppy disk parameters.
 */
static void INT13_ReadFloppyParams( CONTEXT *context )
{
#ifdef linux
    static const BYTE floppy_params[2][13] =
    {
        { 0xaf, 0x02, 0x25, 0x02, 0x12, 0x1b, 0xff, 0x6c, 0xf6, 0x0f, 0x08 },
        { 0xaf, 0x02, 0x25, 0x02, 0x12, 0x1b, 0xff, 0x6c, 0xf6, 0x0f, 0x08 }
    };

    static const DWORD drive_type_info[7]={
        0x0000, /* none */
        0x2709, /* 360 K */
        0x4f0f, /* 1.2 M */
        0x4f09, /* 720 K */
        0x4f12, /* 1.44 M */
        0x4f24, /* 2.88 M */
        0x4f24  /* 2.88 M */
    };

    unsigned int i;
    unsigned int nr_of_drives = 0;
    BYTE drive_nr = DL_reg( context );
    int floppy_fd;
    int r;
    struct floppy_drive_params floppy_parm;
    WCHAR root[] = {'A',':','\\',0}, drive_root[] = {'\\','\\','.','\\','A',':',0};
    HANDLE h;

    TRACE("in  [ EDX=%08x ]\n", context->Edx );

    SET_AL( context, 0 );
    SET_BX( context, 0 );
    SET_CX( context, 0 );
    SET_DH( context, 0 );

    for (i = 0; i < MAX_DOS_DRIVES; i++, root[0]++)
        if (GetDriveTypeW(root) == DRIVE_REMOVABLE) nr_of_drives++;
    SET_DL( context, nr_of_drives );

    if (drive_nr > 1) { 
        /* invalid drive ? */
        INT13_SetStatus( context, 0x07 ); /* drive parameter activity failed */
        return;
    }

    drive_root[4] = 'A' + drive_nr;
    h = CreateFileW(drive_root, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (h == INVALID_HANDLE_VALUE ||
        wine_server_handle_to_fd(h, FILE_READ_DATA, &floppy_fd, NULL))
    {
        WARN("Can't determine floppy geometry !\n");
        INT13_SetStatus( context, 0x07 ); /* drive parameter activity failed */
        return;
    }
    r = ioctl(floppy_fd, FDGETDRVPRM, &floppy_parm);
    wine_server_release_fd( h, floppy_fd );
    CloseHandle(h);

    if(r<0)
    {
        INT13_SetStatus( context, 0x07 ); /* drive parameter activity failed */
        return;
    }

    SET_BL( context, floppy_parm.cmos );

    /*
     * CH = low eight bits of max cyl
     * CL = max sec nr (bits 5-0),
     *      hi two bits of max cyl (bits 7-6)
     * DH = max head nr 
     */
    if(BL_reg( context ) && BL_reg( context ) < 7)
    {
        SET_DH( context, 0x01 );
        SET_CX( context, drive_type_info[BL_reg( context )] );
    }

    context->Edi = (DWORD)floppy_params[drive_nr];

    if(!context->Edi)
    {
        ERR("Get floppy params failed for drive %d\n", drive_nr);
        INT13_SetStatus( context, 0x07 ); /* drive parameter activity failed */
        return;
    }

    TRACE("out [ EAX=%08x EBX=%08x ECX=%08x EDX=%08x EDI=%08x ]\n",
          context->Eax, context->Ebx, context->Ecx, context->Edx, context->Edi);

    INT13_SetStatus( context, 0x00 ); /* success */

    /* FIXME: Word exits quietly if we return with no error. Why? */
    FIXME("Returned ERROR!\n");
    SET_CFLAG( context );

#else
    INT13_SetStatus( context, 0x01 ); /* invalid function */
#endif
}


/**********************************************************************
 *         DOSVM_Int13Handler
 *
 * Handler for int 13h (disk I/O).
 */
void WINAPI DOSVM_Int13Handler( CONTEXT *context )
{
    TRACE( "AH=%02x\n", AH_reg( context ) );

    switch( AH_reg( context ) )
    {
    case 0x00: /* RESET DISK SYSTEM */
        INT13_SetStatus( context, 0x00 ); /* success */
        break;

    case 0x01: /* STATUS OF DISK SYSTEM */
        INT13_SetStatus( context, INT13_last_status );
        break;

    case 0x02: /* READ SECTORS INTO MEMORY */
        SET_AL( context, 0 ); /* number of sectors transferred */
        INT13_SetStatus( context, 0x00 ); /* success */
        break;

    case 0x03: /* WRITE SECTORS FROM MEMORY */
        SET_AL( context, 0 ); /* number of sectors transferred */
        INT13_SetStatus( context, 0x00 ); /* success */
        break;

    case 0x04: /* VERIFY DISK SECTOR(S) */
        SET_AL( context, 0 ); /* number of sectors verified */
        INT13_SetStatus( context, 0x00 ); /* success */
        break;

    case 0x05: /* FORMAT TRACK */
    case 0x06: /* FORMAT TRACK AND SET BAD SECTOR FLAGS */
    case 0x07: /* FORMAT DRIVE STARTING AT GIVEN TRACK  */
        INT13_SetStatus( context, 0x0c ); /* unsupported track or invalid media */
        break;

    case 0x08: /* GET DRIVE PARAMETERS  */
        if (DL_reg( context ) & 0x80) 
        {
            /* hard disk ? */
            INT13_SetStatus( context, 0x07 ); /* drive parameter activity failed */
        }
        else
        { 
            /* floppy disk */
            INT13_ReadFloppyParams( context );
        }
        break;

    case 0x09: /* INITIALIZE CONTROLLER WITH DRIVE PARAMETERS */
    case 0x0a: /* FIXED DISK - READ LONG */
    case 0x0b: /* FIXED DISK - WRITE LONG */
    case 0x0c: /* SEEK TO CYLINDER */
    case 0x0d: /* ALTERNATE RESET HARD DISK */
        INT13_SetStatus( context, 0x00 ); /* success */
        break;

    case 0x0e: /* READ SECTOR BUFFER */
    case 0x0f: /* WRITE SECTOR BUFFER */
        INT13_SetStatus( context, 0x01 ); /* invalid function */
        break;

    case 0x10: /* CHECK IF DRIVE READY */
    case 0x11: /* RECALIBRATE DRIVE */
        INT13_SetStatus( context, 0x00 ); /* success */
        break;

    case 0x12: /* CONTROLLER RAM DIAGNOSTIC */
    case 0x13: /* DRIVE DIAGNOSTIC */
        INT13_SetStatus( context, 0x01 ); /* invalid function */
        break;

    case 0x14: /* CONTROLLER INTERNAL DIAGNOSTIC */
        INT13_SetStatus( context, 0x00 ); /* success */
        break;

    case 0x15: /* GET DISK TYPE */
        if (DL_reg( context ) & 0x80) 
        {
            /* hard disk ? */
            INT13_SetStatus( context, 0x00 ); /* success */
            /* type is fixed disk, overwrites status */
            SET_AH( context, 0x03 );
        }
        else
        { 
            /* floppy disk */
            INT13_SetStatus( context, 0x00 ); /* success */
            /* type is floppy with change detection, overwrites status */
            SET_AH( context, 0x02 );
        }
        break;

    case 0x16: /* FLOPPY - CHANGE OF DISK STATUS */
        INT13_SetStatus( context, 0x00 ); /* success */
        break;

    case 0x17: /* SET DISK TYPE FOR FORMAT */
        if (DL_reg( context ) < 4)
            INT13_SetStatus( context, 0x00 ); /* successful completion */
        else
            INT13_SetStatus( context, 0x01 ); /* error */
        break;

    case 0x18: /* SET MEDIA TYPE FOR FORMAT */
        if (DL_reg( context ) < 4)
            INT13_SetStatus( context, 0x00 ); /* success */
        else
            INT13_SetStatus( context, 0x01 ); /* error */
        break;

    case 0x19: /* FIXED DISK - PARK HEADS */
        INT13_SetStatus( context, 0x00 ); /* success */
        break;

    default:
        INT_BARF( context, 0x13 );
        INT13_SetStatus( context, 0x01 ); /* invalid function */
    } 
}
