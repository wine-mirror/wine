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
#include "wine/port.h"

#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#ifdef HAVE_SYS_FILE_H
# include <sys/file.h>
#endif
#include <string.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_UTIME_H
# include <utime.h>
#endif
#include <ctype.h>
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "wingdi.h"
#include "winuser.h" /* SW_NORMAL */
#include "wine/winbase16.h"
#include "winerror.h"
#include "drive.h"
#include "file.h"
#include "callback.h"
#include "msdos.h"
#include "miscemu.h"
#include "task.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int21);
#if defined(__svr4__) || defined(_SCO_DS)
/* SVR4 DOESNT do locking the same way must implement properly */
#define LOCK_EX 0
#define LOCK_SH  1
#define LOCK_NB  8
#endif


#define DOS_GET_DRIVE(reg) ((reg) ? (reg) - 1 : DRIVE_GetCurrentDrive())

struct DosHeap {
        BYTE mediaID;
	BYTE biosdate[8];
};
static struct DosHeap *heap;
static WORD DosHeapHandle;

extern char TempDirectory[];

static BOOL INT21_CreateHeap(void)
{
    if (!(DosHeapHandle = GlobalAlloc16(GMEM_FIXED,sizeof(struct DosHeap))))
    {
        WARN("Out of memory\n");
        return FALSE;
    }
    heap = (struct DosHeap *) GlobalLock16(DosHeapHandle);
    strcpy(heap->biosdate, "01/01/80");
    return TRUE;
}

static BYTE *GetCurrentDTA( CONTEXT86 *context )
{
    TDB *pTask = GlobalLock16(GetCurrentTask());

    /* FIXME: This assumes DTA was set correctly! */
    return (BYTE *)CTX_SEG_OFF_TO_LIN( context, SELECTOROF(pTask->dta),
                                                (DWORD)OFFSETOF(pTask->dta) );
}


static void CreateBPB(int drive, BYTE *data, BOOL16 limited)
/* limited == TRUE is used with INT 0x21/0x440d */
{
	if (drive > 1) {
		setword(data, 512);
		data[2] = 2;
		setword(&data[3], 0);
		data[5] = 2;
		setword(&data[6], 240);
		setword(&data[8], 64000);
		data[0x0a] = 0xf8;
		setword(&data[0x0b], 40);
		setword(&data[0x0d], 56);
		setword(&data[0x0f], 2);
		setword(&data[0x11], 0);
		if (!limited) {
		    setword(&data[0x1f], 800);
		    data[0x21] = 5;
		    setword(&data[0x22], 1);
		}
	} else { /* 1.44mb */
		setword(data, 512);
		data[2] = 2;
		setword(&data[3], 0);
		data[5] = 2;
		setword(&data[6], 240);
		setword(&data[8], 2880);
		data[0x0a] = 0xf8;
		setword(&data[0x0b], 6);
		setword(&data[0x0d], 18);
		setword(&data[0x0f], 2);
		setword(&data[0x11], 0);
		if (!limited) {
		    setword(&data[0x1f], 80);
		    data[0x21] = 7;
		    setword(&data[0x22], 2);
		}
	}
}

static int INT21_GetFreeDiskSpace( CONTEXT86 *context )
{
    DWORD cluster_sectors, sector_bytes, free_clusters, total_clusters;
    char root[] = "A:\\";

    *root += DOS_GET_DRIVE( DL_reg(context) );
    if (!GetDiskFreeSpaceA( root, &cluster_sectors, &sector_bytes,
                              &free_clusters, &total_clusters )) return 0;
    SET_AX( context, cluster_sectors );
    SET_BX( context, free_clusters );
    SET_CX( context, sector_bytes );
    SET_DX( context, total_clusters );
    return 1;
}

static int INT21_GetDriveAllocInfo( CONTEXT86 *context )
{
    if (!INT21_GetFreeDiskSpace( context )) return 0;
    if (!heap && !INT21_CreateHeap()) return 0;
    heap->mediaID = 0xf0;
    context->SegDs = DosHeapHandle;
    SET_BX( context, (int)&heap->mediaID - (int)heap );
    return 1;
}

static BOOL ioctlGenericBlkDevReq( CONTEXT86 *context )
{
	BYTE *dataptr = CTX_SEG_OFF_TO_LIN(context, context->SegDs, context->Edx);
	int drive = DOS_GET_DRIVE( BL_reg(context) );

	if (!DRIVE_IsValid(drive))
        {
            SetLastError( ERROR_FILE_NOT_FOUND );
            return TRUE;
	}

	if (CH_reg(context) != 0x08)
        {
            INT_BARF( context, 0x21 );
            return FALSE;
	}

	switch (CL_reg(context))
	{
		case 0x60: /* get device parameters */
			   /* used by w4wgrp's winfile */
			memset(dataptr, 0, 0x20); /* DOS 6.22 uses 0x20 bytes */
			dataptr[0] = 0x04;
			dataptr[6] = 0; /* media type */
			if (drive > 1)
			{
				dataptr[1] = 0x05; /* fixed disk */
				setword(&dataptr[2], 0x01); /* non removable */
				setword(&dataptr[4], 0x300); /* # of cylinders */
			}
			else
			{
				dataptr[1] = 0x07; /* block dev, floppy */
				setword(&dataptr[2], 0x02); /* removable */
				setword(&dataptr[4], 80); /* # of cylinders */
			}
			CreateBPB(drive, &dataptr[7], TRUE);
			RESET_CFLAG(context);
			break;

		case 0x66:/*  get disk serial number */
			{
				char	label[12],fsname[9],path[4];
				DWORD	serial;

				strcpy(path,"x:\\");path[0]=drive+'A';
				GetVolumeInformationA(
					path,label,12,&serial,NULL,NULL,fsname,9
				);
				*(WORD*)dataptr		= 0;
				memcpy(dataptr+2,&serial,4);
				memcpy(dataptr+6,label	,11);
				memcpy(dataptr+17,fsname,8);
			}
			break;

		case 0x6f:
			memset(dataptr+1, '\0', dataptr[0]-1);
			dataptr[1] = dataptr[0];
			dataptr[2] = 0x07; /* protected mode driver; no eject; no notification */
			dataptr[3] = 0xFF; /* no physical drive */
			break;

		case 0x72:
			/* Trail on error implementation */
			SET_AX( context, GetDriveType16(BL_reg(context)) == DRIVE_UNKNOWN ? 0x0f : 0x01 );
			SET_CFLAG(context);	/* Seems to be set all the time */
			break;

		default:
                        INT_BARF( context, 0x21 );
	}
	return FALSE;
}

static void INT21_ParseFileNameIntoFCB( CONTEXT86 *context )
{
    char *filename =
        CTX_SEG_OFF_TO_LIN(context, context->SegDs, context->Esi );
    char *fcb =
        CTX_SEG_OFF_TO_LIN(context, context->SegEs, context->Edi );
    char *s;
    WCHAR *buffer;
    WCHAR fcbW[12];
    INT buffer_len, len;

    SET_AL( context, 0xff ); /* failed */

    TRACE("filename: '%s'\n", filename);

    s = filename;
    len = 0;
    while (*s)
    {
        if ((*s != ' ') && (*s != '\r') && (*s != '\n'))
        {
            s++;
            len++;
        }
        else
            break;
    }

    buffer_len = MultiByteToWideChar(CP_OEMCP, 0, filename, len, NULL, 0);
    buffer = HeapAlloc( GetProcessHeap(), 0, (buffer_len + 1) * sizeof(WCHAR));
    len = MultiByteToWideChar(CP_OEMCP, 0, filename, len, buffer, buffer_len);
    buffer[len] = 0;
    DOSFS_ToDosFCBFormat(buffer, fcbW);
    HeapFree(GetProcessHeap(), 0, buffer);
    WideCharToMultiByte(CP_OEMCP, 0, fcbW, 12, fcb + 1, 12, NULL, NULL);
    *fcb = 0;
    TRACE("FCB: '%s'\n", fcb + 1);

    SET_AL( context, ((strchr(filename, '*')) || (strchr(filename, '$'))) != 0 );

    /* point DS:SI to first unparsed character */
    SET_SI( context, context->Esi + (int)s - (int)filename );
}


/* Many calls translate a drive argument like this:
   drive number (00h = default, 01h = A:, etc)
   */
static const char *INT21_DriveName(int drive)
{
    if (drive > 0) return wine_dbg_sprintf( "%c:", 'A' + drive - 1 );
    return "default";
}

static HFILE16 _lcreat16_uniq( LPCSTR path, INT attr )
{
    /* Mask off all flags not explicitly allowed by the doc */
    attr &= FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
    return Win32HandleToDosFileHandle( CreateFileA( path, GENERIC_READ | GENERIC_WRITE,
                                             FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                             CREATE_NEW, attr, 0 ));
}


static int INT21_FindFirst( CONTEXT86 *context )
{
    char *p;
    const char *path;
    DOS_FULL_NAME full_name;
    FINDFILE_DTA *dta = (FINDFILE_DTA *)GetCurrentDTA(context);
    WCHAR pathW[MAX_PATH];
    WCHAR maskW[12];

    path = (const char *)CTX_SEG_OFF_TO_LIN(context, context->SegDs, context->Edx);
    MultiByteToWideChar(CP_OEMCP, 0, path, -1, pathW, MAX_PATH);

    dta->unixPath = NULL;
    if (!DOSFS_GetFullName( pathW, FALSE, &full_name ))
    {
        SET_AX( context, GetLastError() );
        SET_CFLAG(context);
        return 0;
    }
    dta->unixPath = HeapAlloc( GetProcessHeap(), 0, strlen(full_name.long_name)+1 );
    strcpy( dta->unixPath, full_name.long_name );
    p = strrchr( dta->unixPath, '/' );
    *p = '\0';

    MultiByteToWideChar(CP_OEMCP, 0, p + 1, -1, pathW, MAX_PATH);

    /* Note: terminating NULL in dta->mask overwrites dta->search_attr
     *       (doesn't matter as it is set below anyway)
     */
    if (!DOSFS_ToDosFCBFormat( pathW, maskW ))
    {
        HeapFree( GetProcessHeap(), 0, dta->unixPath );
        dta->unixPath = NULL;
        SetLastError( ERROR_FILE_NOT_FOUND );
        SET_AX( context, ERROR_FILE_NOT_FOUND );
        SET_CFLAG(context);
        return 0;
    }
    WideCharToMultiByte(CP_OEMCP, 0, maskW, 12, dta->mask, sizeof(dta->mask), NULL, NULL);
    dta->drive = (path[0] && (path[1] == ':')) ? toupper(path[0]) - 'A'
                                               : DRIVE_GetCurrentDrive();
    dta->count = 0;
    dta->search_attr = CL_reg(context);
    return 1;
}


static int INT21_FindNext( CONTEXT86 *context )
{
    FINDFILE_DTA *dta = (FINDFILE_DTA *)GetCurrentDTA(context);
    WIN32_FIND_DATAA entry;
    int count;

    if (!dta->unixPath) return 0;
    if (!(count = DOSFS_FindNext( dta->unixPath, dta->mask, NULL, dta->drive,
                                  dta->search_attr, dta->count, &entry )))
    {
        HeapFree( GetProcessHeap(), 0, dta->unixPath );
        dta->unixPath = NULL;
        return 0;
    }
    if ((int)dta->count + count > 0xffff)
    {
        WARN("Too many directory entries in %s\n", dta->unixPath );
        HeapFree( GetProcessHeap(), 0, dta->unixPath );
        dta->unixPath = NULL;
        return 0;
    }
    dta->count += count;
    dta->fileattr = entry.dwFileAttributes;
    dta->filesize = entry.nFileSizeLow;
    FileTimeToDosDateTime( &entry.ftLastWriteTime,
                           &dta->filedate, &dta->filetime );
    strcpy( dta->filename, entry.cAlternateFileName );
    if (!memchr(dta->mask,'?',11)) {
	/* wildcardless search, release resources in case no findnext will
	 * be issued, and as a workaround in case file creation messes up
	 * findnext, as sometimes happens with pkunzip */
        HeapFree( GetProcessHeap(), 0, dta->unixPath );
        dta->unixPath = NULL;
    }
    return 1;
}


static BOOL INT21_CreateTempFile( CONTEXT86 *context )
{
    static int counter = 0;
    char *name = CTX_SEG_OFF_TO_LIN(context,  context->SegDs, context->Edx );
    char *p = name + strlen(name);

    /* despite what Ralf Brown says, some programs seem to call without
     * ending backslash (DOS accepts that, so we accept it too) */
    if ((p == name) || (p[-1] != '\\')) *p++ = '\\';

    for (;;)
    {
        sprintf( p, "wine%04x.%03d", (int)getpid(), counter );
        counter = (counter + 1) % 1000;

        SET_AX( context, _lcreat16_uniq( name, 0 ) );
        if (AX_reg(context) != HFILE_ERROR16)
        {
            TRACE("created %s\n", name );
            return TRUE;
        }
        if (GetLastError() != ERROR_FILE_EXISTS) return FALSE;
    }
}


static int INT21_GetDiskSerialNumber( CONTEXT86 *context )
{
    BYTE *dataptr = CTX_SEG_OFF_TO_LIN(context, context->SegDs, context->Edx);
    int drive = DOS_GET_DRIVE( BL_reg(context) );

    if (!DRIVE_IsValid(drive))
    {
        SetLastError( ERROR_INVALID_DRIVE );
        return 0;
    }

    *(WORD *)dataptr = 0;
    *(DWORD *)(dataptr + 2) = DRIVE_GetSerialNumber( drive );
    memcpy( dataptr + 6, DRIVE_GetLabel( drive ), 11 );
    strncpy(dataptr + 0x11, "FAT16   ", 8);
    return 1;
}


static int INT21_SetDiskSerialNumber( CONTEXT86 *context )
{
    BYTE *dataptr = CTX_SEG_OFF_TO_LIN(context, context->SegDs, context->Edx);
    int drive = DOS_GET_DRIVE( BL_reg(context) );

    if (!DRIVE_IsValid(drive))
    {
        SetLastError( ERROR_INVALID_DRIVE );
        return 0;
    }

    DRIVE_SetSerialNumber( drive, *(DWORD *)(dataptr + 2) );
    return 1;
}


/* microsoft's programmers should be shot for using CP/M style int21
   calls in Windows for Workgroup's winfile.exe */

static int INT21_FindFirstFCB( CONTEXT86 *context )
{
    BYTE *fcb = (BYTE *)CTX_SEG_OFF_TO_LIN(context, context->SegDs, context->Edx);
    FINDFILE_FCB *pFCB;
    LPCSTR root, cwd;
    int drive;

    if (*fcb == 0xff) pFCB = (FINDFILE_FCB *)(fcb + 7);
    else pFCB = (FINDFILE_FCB *)fcb;
    drive = DOS_GET_DRIVE( pFCB->drive );
    if (!DRIVE_IsValid( drive )) return 0;
    root = DRIVE_GetRoot( drive );
    cwd  = DRIVE_GetUnixCwd( drive );
    pFCB->unixPath = HeapAlloc( GetProcessHeap(), 0,
                                strlen(root)+strlen(cwd)+2 );
    if (!pFCB->unixPath) return 0;
    strcpy( pFCB->unixPath, root );
    strcat( pFCB->unixPath, "/" );
    strcat( pFCB->unixPath, cwd );
    pFCB->count = 0;
    return 1;
}


static int INT21_FindNextFCB( CONTEXT86 *context )
{
    BYTE *fcb = (BYTE *)CTX_SEG_OFF_TO_LIN(context, context->SegDs, context->Edx);
    FINDFILE_FCB *pFCB;
    DOS_DIRENTRY_LAYOUT *pResult = (DOS_DIRENTRY_LAYOUT *)GetCurrentDTA(context);
    WIN32_FIND_DATAA entry;
    BYTE attr;
    int count;

    if (*fcb == 0xff) /* extended FCB ? */
    {
        attr = fcb[6];
        pFCB = (FINDFILE_FCB *)(fcb + 7);
    }
    else
    {
        attr = 0;
        pFCB = (FINDFILE_FCB *)fcb;
    }

    if (!pFCB->unixPath) return 0;
    if (!(count = DOSFS_FindNext( pFCB->unixPath, pFCB->filename, NULL,
                                  DOS_GET_DRIVE( pFCB->drive ), attr,
                                  pFCB->count, &entry )))
    {
        HeapFree( GetProcessHeap(), 0, pFCB->unixPath );
        pFCB->unixPath = NULL;
        return 0;
    }
    pFCB->count += count;

    if (*fcb == 0xff) { /* place extended FCB header before pResult if called with extended FCB */
	*(BYTE *)pResult = 0xff;
	(BYTE *)pResult +=6; /* leave reserved field behind */
	*(BYTE *)pResult = entry.dwFileAttributes;
	((BYTE *)pResult)++;
    }
    *(BYTE *)pResult = DOS_GET_DRIVE( pFCB->drive ); /* DOS_DIRENTRY_LAYOUT after current drive number */
    ((BYTE *)pResult)++;
    pResult->fileattr = entry.dwFileAttributes;
    pResult->cluster  = 0;  /* what else? */
    pResult->filesize = entry.nFileSizeLow;
    memset( pResult->reserved, 0, sizeof(pResult->reserved) );
    FileTimeToDosDateTime( &entry.ftLastWriteTime,
                           &pResult->filedate, &pResult->filetime );

    /* Convert file name to FCB format */

    memset( pResult->filename, ' ', sizeof(pResult->filename) );
    if (!strcmp( entry.cAlternateFileName, "." )) pResult->filename[0] = '.';
    else if (!strcmp( entry.cAlternateFileName, ".." ))
        pResult->filename[0] = pResult->filename[1] = '.';
    else
    {
        char *p = strrchr( entry.cAlternateFileName, '.' );
        if (p && p[1] && (p != entry.cAlternateFileName))
        {
            memcpy( pResult->filename, entry.cAlternateFileName,
                    min( (p - entry.cAlternateFileName), 8 ) );
            memcpy( pResult->filename + 8, p + 1, min( strlen(p), 3 ) );
        }
        else
            memcpy( pResult->filename, entry.cAlternateFileName,
                    min( strlen(entry.cAlternateFileName), 8 ) );
    }
    return 1;
}


static BOOL
INT21_networkfunc (CONTEXT86 *context)
{
     switch (AL_reg(context)) {
     case 0x00: /* Get machine name. */
     {
	  char *dst = CTX_SEG_OFF_TO_LIN (context,context->SegDs,context->Edx);
	  TRACE("getting machine name to %p\n", dst);
	  if (gethostname (dst, 15))
	  {
	       WARN("failed!\n");
	       SetLastError( ER_NoNetwork );
	       return TRUE;
	  } else {
	       int len = strlen (dst);
	       while (len < 15)
		    dst[len++] = ' ';
	       dst[15] = 0;
	       SET_CH( context, 1 ); /* Valid */
	       SET_CL( context, 1 ); /* NETbios number??? */
	       TRACE("returning %s\n", debugstr_an (dst, 16));
	       return FALSE;
	  }
     }

     default:
	  SetLastError( ER_NoNetwork );
	  return TRUE;
     }
}


/***********************************************************************
 *           INT_Int21Handler
 */
void WINAPI INT_Int21Handler( CONTEXT86 *context )
{
    BOOL	bSetDOSExtendedError = FALSE;

    switch(AH_reg(context))
    {
    case 0x11: /* FIND FIRST MATCHING FILE USING FCB */
	TRACE("FIND FIRST MATCHING FILE USING FCB %p\n",
	      CTX_SEG_OFF_TO_LIN(context, context->SegDs, context->Edx));
        if (!INT21_FindFirstFCB(context))
        {
            SET_AL( context, 0xff );
            break;
        }
        /* else fall through */

    case 0x12: /* FIND NEXT MATCHING FILE USING FCB */
        SET_AL( context, INT21_FindNextFCB(context) ? 0x00 : 0xff );
        break;

    case 0x1b: /* GET ALLOCATION INFORMATION FOR DEFAULT DRIVE */
        SET_DL( context, 0 );
        if (!INT21_GetDriveAllocInfo(context)) SET_AX( context, 0xffff );
        break;

    case 0x1c: /* GET ALLOCATION INFORMATION FOR SPECIFIC DRIVE */
        if (!INT21_GetDriveAllocInfo(context)) SET_AX( context, 0xffff );
        break;

    case 0x29: /* PARSE FILENAME INTO FCB */
        INT21_ParseFileNameIntoFCB(context);
        break;

    case 0x36: /* GET FREE DISK SPACE */
	TRACE("GET FREE DISK SPACE FOR DRIVE %s\n",
	      INT21_DriveName( DL_reg(context)));
        if (!INT21_GetFreeDiskSpace(context)) SET_AX( context, 0xffff );
        break;

    case 0x44: /* IOCTL */
        switch (AL_reg(context))
        {
        case 0x0d:
            TRACE("IOCTL - GENERIC BLOCK DEVICE REQUEST %s\n",
		  INT21_DriveName( BL_reg(context)));
            bSetDOSExtendedError = ioctlGenericBlkDevReq(context);
            break;

        case 0x0F:   /* Set logical drive mapping */
	    {
	    int drive;
            TRACE("IOCTL - SET LOGICAL DRIVE MAP for drive %s\n",
		  INT21_DriveName( BL_reg(context)));
	    drive = DOS_GET_DRIVE ( BL_reg(context) );
	    if ( ! DRIVE_SetLogicalMapping ( drive, drive+1 ) )
	    {
		SET_CFLAG(context);
		SET_AX( context, 0x000F );  /* invalid drive */
	    }
            break;
	    }
        }
        break;

    case 0x4e: /* "FINDFIRST" - FIND FIRST MATCHING FILE */
        TRACE("FINDFIRST mask 0x%04x spec %s\n",CX_reg(context),
	      (LPCSTR)CTX_SEG_OFF_TO_LIN(context,  context->SegDs, context->Edx));
        if (!INT21_FindFirst(context)) break;
        /* fall through */

    case 0x4f: /* "FINDNEXT" - FIND NEXT MATCHING FILE */
        TRACE("FINDNEXT\n");
        if (!INT21_FindNext(context))
        {
            SetLastError( ERROR_NO_MORE_FILES );
            SET_AX( context, ERROR_NO_MORE_FILES );
            SET_CFLAG(context);
        }
        else SET_AX( context, 0 );  /* OK */
        break;

    case 0x5a: /* CREATE TEMPORARY FILE */
        TRACE("CREATE TEMPORARY FILE\n");
        bSetDOSExtendedError = !INT21_CreateTempFile(context);
        break;

    case 0x5e:
	bSetDOSExtendedError = INT21_networkfunc (context);
        break;

    case 0x5f: /* NETWORK */
        switch (AL_reg(context))
        {
        case 0x07: /* ENABLE DRIVE */
            TRACE("ENABLE DRIVE %c:\n",(DL_reg(context)+'A'));
            if (!DRIVE_Enable( DL_reg(context) ))
            {
                SetLastError( ERROR_INVALID_DRIVE );
		bSetDOSExtendedError = TRUE;
            }
            break;

        case 0x08: /* DISABLE DRIVE */
            TRACE("DISABLE DRIVE %c:\n",(DL_reg(context)+'A'));
            if (!DRIVE_Disable( DL_reg(context) ))
            {
                SetLastError( ERROR_INVALID_DRIVE );
		bSetDOSExtendedError = TRUE;
            }
            break;

        default:
            /* network software not installed */
            TRACE("NETWORK function AX=%04x not implemented\n",AX_reg(context));
            SetLastError( ER_NoNetwork );
	    bSetDOSExtendedError = TRUE;
            break;
        }
        break;

    case 0x60: /* "TRUENAME" - CANONICALIZE FILENAME OR PATH */
        TRACE("TRUENAME %s\n",
	      (LPCSTR)CTX_SEG_OFF_TO_LIN(context, context->SegDs,context->Esi));
        {
            if (!GetFullPathNameA( CTX_SEG_OFF_TO_LIN(context, context->SegDs,
                                                        context->Esi), 128,
                                     CTX_SEG_OFF_TO_LIN(context, context->SegEs,
                                                        context->Edi),NULL))
		bSetDOSExtendedError = TRUE;
            else SET_AX( context, 0 );
        }
        break;

    case 0x69: /* DISK SERIAL NUMBER */
        switch (AL_reg(context))
        {
        case 0x00:
	    TRACE("GET DISK SERIAL NUMBER for drive %s\n",
		  INT21_DriveName(BL_reg(context)));
            if (!INT21_GetDiskSerialNumber(context)) bSetDOSExtendedError = TRUE;
            else SET_AX( context, 0 );
            break;

        case 0x01:
	    TRACE("SET DISK SERIAL NUMBER for drive %s\n",
		  INT21_DriveName(BL_reg(context)));
            if (!INT21_SetDiskSerialNumber(context)) bSetDOSExtendedError = TRUE;
            else SET_AX( context, 1 );
            break;
        }
        break;

    case 0x71: /* MS-DOS 7 (Windows95) - LONG FILENAME FUNCTIONS */
        switch(AL_reg(context))
        {
        case 0x4e:  /* Find first file */
	    TRACE(" LONG FILENAME - FIND FIRST MATCHING FILE for %s\n",
		  (LPCSTR)CTX_SEG_OFF_TO_LIN(context, context->SegDs,context->Edx));
            /* FIXME: use attributes in CX */
            SET_AX( context, FindFirstFile16(
                        CTX_SEG_OFF_TO_LIN(context, context->SegDs,context->Edx),
                        (WIN32_FIND_DATAA *)CTX_SEG_OFF_TO_LIN(context, context->SegEs,
                                                               context->Edi)));
            if (AX_reg(context) == INVALID_HANDLE_VALUE16)
		bSetDOSExtendedError = TRUE;
            break;
        case 0x4f:  /* Find next file */
	    TRACE("LONG FILENAME - FIND NEXT MATCHING FILE for handle %d\n",
		  BX_reg(context));
            if (!FindNextFile16( BX_reg(context),
                    (WIN32_FIND_DATAA *)CTX_SEG_OFF_TO_LIN(context, context->SegEs,
                                                             context->Edi)))
		bSetDOSExtendedError = TRUE;
            break;
	case 0xa0:
	    {
		LPCSTR driveroot = (LPCSTR)CTX_SEG_OFF_TO_LIN(context,  context->SegDs,context->Edx);
		LPSTR buffer = (LPSTR)CTX_SEG_OFF_TO_LIN(context,  context->SegEs,context->Edi);
                DWORD filename_len, flags;

		TRACE("LONG FILENAME - GET VOLUME INFORMATION for drive having root dir '%s'.\n", driveroot);
		SET_AX( context, 0 );
                if (!GetVolumeInformationA( driveroot, NULL, 0, NULL, &filename_len,
                                            &flags, buffer, 8 ))
                {
                    INT_BARF( context, 0x21 );
                    SET_CFLAG(context);
                    break;
                }
		SET_BX( context, flags | 0x4000 ); /* support for LFN functions */
		SET_CX( context, filename_len );
		SET_DX( context, MAX_PATH ); /* FIXME: which len if DRIVE_SHORT_NAMES ? */
	    }
	    break;
        case 0xa1:  /* Find close */
	    TRACE("LONG FILENAME - FINDCLOSE for handle %d\n",
		  BX_reg(context));
            bSetDOSExtendedError = (!FindClose16( BX_reg(context) ));
            break;
        case 0x60:
	  switch(CL_reg(context))
	  {
	    case 0x01:  /* Get short filename or path */
	      if (!GetShortPathNameA
		  ( CTX_SEG_OFF_TO_LIN(context, context->SegDs,
				       context->Esi),
		    CTX_SEG_OFF_TO_LIN(context, context->SegEs,
				       context->Edi), 67))
		bSetDOSExtendedError = TRUE;
	      else SET_AX( context, 0 );
	      break;
	    case 0x02:  /* Get canonical long filename or path */
	      if (!GetFullPathNameA
		  ( CTX_SEG_OFF_TO_LIN(context, context->SegDs,
				       context->Esi), 128,
		    CTX_SEG_OFF_TO_LIN(context, context->SegEs,
				       context->Edi),NULL))
		bSetDOSExtendedError = TRUE;
	      else SET_AX( context, 0 );
	      break;
	    default:
	      FIXME("Unimplemented long file name function:\n");
	      INT_BARF( context, 0x21 );
	      SET_CFLAG(context);
	      SET_AL( context, 0 );
	      break;
	  }
	  break;

        default:
            FIXME("Unimplemented long file name function:\n");
            INT_BARF( context, 0x21 );
            SET_CFLAG(context);
            SET_AL( context, 0 );
            break;
        }
        break;

    default:
        INT_BARF( context, 0x21 );
        break;

    } /* END OF SWITCH */

    if( bSetDOSExtendedError )		/* set general error condition */
    {
	SET_AX( context, GetLastError() );
	SET_CFLAG(context);
    }
}
