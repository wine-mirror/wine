/*
 * DOS interrupt 21h handler
 */

#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#ifdef HAVE_SYS_FILE_H
# include <sys/file.h>
#endif
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>
#include <ctype.h>
#include "windows.h"
#include "winerror.h"
#include "drive.h"
#include "file.h"
#include "heap.h"
#include "msdos.h"
#include "ldt.h"
#include "task.h"
#include "options.h"
#include "miscemu.h"
#include "dosexe.h" /* for the MZ_SUPPORTED define */
#include "module.h"
#include "debug.h"
#include "console.h"
#if defined(__svr4__) || defined(_SCO_DS)
/* SVR4 DOESNT do locking the same way must implement properly */
#define LOCK_EX 0
#define LOCK_SH  1
#define LOCK_NB  8
#endif


#define DOS_GET_DRIVE(reg) ((reg) ? (reg) - 1 : DRIVE_GetCurrentDrive())

/* Define the drive parameter block, as used by int21/1F
 * and int21/32.  This table can be accessed through the
 * global 'dpb' pointer, which points into the local dos
 * heap.
 */
struct DPB
{
    BYTE drive_num;         /* 0=A, etc. */
    BYTE unit_num;          /* Drive's unit number (?) */
    WORD sector_size;       /* Sector size in bytes */
    BYTE high_sector;       /* Highest sector in a cluster */
    BYTE shift;             /* Shift count (?) */
    WORD reserved;          /* Number of reserved sectors at start */
    BYTE num_FAT;           /* Number of FATs */
    WORD dir_entries;       /* Number of root dir entries */
    WORD first_data;        /* First data sector */
    WORD high_cluster;      /* Highest cluster number */
    WORD sectors_in_FAT;    /* Number of sectors per FAT */
    WORD start_dir;         /* Starting sector of first dir */
    DWORD driver_head;      /* Address of device driver header (?) */
    BYTE media_ID;          /* Media ID */
    BYTE access_flag;       /* Prev. accessed flag (0=yes,0xFF=no) */
    DWORD next;             /* Pointer to next DPB in list */
    WORD free_search;       /* Free cluster search start */
    WORD free_clusters;     /* Number of free clusters (0xFFFF=unknown) */
};

WORD CodePage = 437;
DWORD dpbsegptr;

struct DosHeap {
	BYTE InDosFlag;
        BYTE mediaID;
	BYTE biosdate[8];
        struct DPB dpb;
        BYTE DummyDBCSLeadTable[6];
};
static struct DosHeap *heap;
static WORD DosHeapHandle;

WORD sharing_retries = 3;      /* number of retries at sharing violation */
WORD sharing_pause = 1;        /* pause between retries */

extern char TempDirectory[];

static BOOL32 INT21_CreateHeap(void)
{
    if (!(DosHeapHandle = GlobalAlloc16(GMEM_FIXED,sizeof(struct DosHeap))))
    {
        WARN(int21, "Out of memory\n");
        return FALSE;
    }
    heap = (struct DosHeap *) GlobalLock16(DosHeapHandle);
    dpbsegptr = PTR_SEG_OFF_TO_SEGPTR(DosHeapHandle,(int)&heap->dpb-(int)heap);
    heap->InDosFlag = 0;
    strcpy(heap->biosdate, "01/01/80");
    memset(heap->DummyDBCSLeadTable, 0, 6);
    return TRUE;
}

static BYTE *GetCurrentDTA( CONTEXT *context )
{
    TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );

    /* FIXME: This assumes DTA was set correctly! */
    return (BYTE *)CTX_SEG_OFF_TO_LIN( context, SELECTOROF(pTask->dta), 
                                                (DWORD)OFFSETOF(pTask->dta) );
}


void CreateBPB(int drive, BYTE *data, BOOL16 limited)
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

static int INT21_GetFreeDiskSpace( CONTEXT *context )
{
    DWORD cluster_sectors, sector_bytes, free_clusters, total_clusters;
    char root[] = "A:\\";

    *root += DOS_GET_DRIVE( DL_reg(context) );
    if (!GetDiskFreeSpace32A( root, &cluster_sectors, &sector_bytes,
                              &free_clusters, &total_clusters )) return 0;
    AX_reg(context) = cluster_sectors;
    BX_reg(context) = free_clusters;
    CX_reg(context) = sector_bytes;
    DX_reg(context) = total_clusters;
    return 1;
}

static int INT21_GetDriveAllocInfo( CONTEXT *context )
{
    if (!INT21_GetFreeDiskSpace( context )) return 0;
    if (!heap && !INT21_CreateHeap()) return 0;
    heap->mediaID = 0xf0;
    DS_reg(context) = DosHeapHandle;
    BX_reg(context) = (int)&heap->mediaID - (int)heap;
    return 1;
}

static void GetDrivePB( CONTEXT *context, int drive )
{
        if(!DRIVE_IsValid(drive))
        {
            SetLastError( ERROR_INVALID_DRIVE );
            AX_reg(context) = 0x00ff;
        }
        else if (heap || INT21_CreateHeap())
        {
                FIXME(int21, "GetDrivePB not fully implemented.\n");

                /* FIXME: I have no idea what a lot of this information should
                 * say or whether it even really matters since we're not allowing
                 * direct block access.  However, some programs seem to depend on
                 * getting at least _something_ back from here.  The 'next' pointer
                 * does worry me, though.  Should we have a complete table of
                 * separate DPBs per drive?  Probably, but I'm lazy. :-)  -CH
                 */
                heap->dpb.drive_num = heap->dpb.unit_num = drive; /*The same?*/
                heap->dpb.sector_size = 512;
                heap->dpb.high_sector = 1;
                heap->dpb.shift = drive < 2 ? 0 : 6; /*6 for HD, 0 for floppy*/
                heap->dpb.reserved = 0;
                heap->dpb.num_FAT = 1;
                heap->dpb.dir_entries = 2;
                heap->dpb.first_data = 2;
                heap->dpb.high_cluster = 64000;
                heap->dpb.sectors_in_FAT = 1;
                heap->dpb.start_dir = 1;
                heap->dpb.driver_head = 0;
                heap->dpb.media_ID = (drive > 1) ? 0xF8 : 0xF0;
                heap->dpb.access_flag = 0;
                heap->dpb.next = 0;
                heap->dpb.free_search = 0;
                heap->dpb.free_clusters = 0xFFFF;    /* unknown */

                AL_reg(context) = 0x00;
                DS_reg(context) = SELECTOROF(dpbsegptr);
                BX_reg(context) = OFFSETOF(dpbsegptr);
        }
}


static void ioctlGetDeviceInfo( CONTEXT *context )
{
    int curr_drive;
    const DOS_DEVICE *dev;

    TRACE(int21, "(%d)\n", BX_reg(context));
    
    RESET_CFLAG(context);

    /* DOS device ? */
    if ((dev = DOSFS_GetDeviceByHandle( FILE_GetHandle32(BX_reg(context)) )))
    {
        DX_reg(context) = dev->flags;
        return;
    }

    /* it seems to be a file */
    curr_drive = DRIVE_GetCurrentDrive();
    DX_reg(context) = 0x0140 + curr_drive + ((curr_drive > 1) ? 0x0800 : 0); 
    /* no floppy */
    /* bits 0-5 are current drive
     * bit 6 - file has NOT been written..FIXME: correct?
     * bit 8 - generate int24 if no diskspace on write/ read past end of file
     * bit 11 - media not removable
     * bit 14 - don't set file date/time on closing
     * bit 15 - file is remote
     */
}

static BOOL32 ioctlGenericBlkDevReq( CONTEXT *context )
{
	BYTE *dataptr = CTX_SEG_OFF_TO_LIN(context, DS_reg(context), EDX_reg(context));
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
		case 0x4a: /* lock logical volume */
			TRACE(int21,"lock logical volume (%d) level %d mode %d\n",drive,BH_reg(context),DX_reg(context));
			break;

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

		case 0x41: /* write logical device track */
		case 0x61: /* read logical device track */
			{
				BYTE drive = BL_reg(context) ?
						BL_reg(context) : DRIVE_GetCurrentDrive(); 
				WORD head   = *(WORD *)dataptr+1;
				WORD cyl    = *(WORD *)dataptr+3;
				WORD sect   = *(WORD *)dataptr+5;
				WORD nrsect = *(WORD *)dataptr+7;
				BYTE *data  =  (BYTE *)dataptr+9;
				int (*raw_func)(BYTE, DWORD, DWORD, BYTE *, BOOL32);

				raw_func = (CL_reg(context) == 0x41) ?
								DRIVE_RawWrite : DRIVE_RawRead;

				if (raw_func(drive, head*cyl*sect, nrsect, data, FALSE))
					RESET_CFLAG(context);
				else
				{
					AX_reg(context) = 0x1e; /* read fault */
					SET_CFLAG(context);
				}
			}
			break;
		case 0x66:/*  get disk serial number */
			{	
				char	label[12],fsname[9],path[4];
				DWORD	serial;

				strcpy(path,"x:\\");path[0]=drive+'A';
				GetVolumeInformation32A(
					path,label,12,&serial,NULL,NULL,fsname,9
				);
				*(WORD*)dataptr		= 0;
				memcpy(dataptr+2,&serial,4);
				memcpy(dataptr+6,label	,11);
				memcpy(dataptr+17,fsname,8);
			}
			break;

		case 0x6a:
			TRACE(int21,"logical volume %d unlocked.\n",drive);
			break;

		case 0x6f:
			memset(dataptr+1, '\0', dataptr[0]-1);
			dataptr[1] = dataptr[0]; 
			dataptr[2] = 0x07; /* protected mode driver; no eject; no notification */
			dataptr[3] = 0xFF; /* no physical drive */
			break;

		default:
                        INT_BARF( context, 0x21 );
	}
	return FALSE;
}

static void INT21_ParseFileNameIntoFCB( CONTEXT *context )
{
    char *filename =
        CTX_SEG_OFF_TO_LIN(context, DS_reg(context), ESI_reg(context) );
    char *fcb =
        CTX_SEG_OFF_TO_LIN(context, ES_reg(context), EDI_reg(context) );
    char *buffer, *s, *d;

    AL_reg(context) = 0xff; /* failed */

    TRACE(int21, "filename: '%s'\n", filename);

    buffer = HeapAlloc( GetProcessHeap(), 0, strlen(filename) + 1);

    s = filename;
    d = buffer;
    while (*s)
    {
        if ((*s != ' ') && (*s != '\r') && (*s != '\n'))
            *d++ = *s++;
        else
            break;
    }

    *d = '\0';
    DOSFS_ToDosFCBFormat(buffer, fcb + 1);
    *fcb = 0;
    TRACE(int21, "FCB: '%s'\n", ((CHAR *)fcb + 1));

    HeapFree( GetProcessHeap(), 0, buffer);

    AL_reg(context) =
        ((strchr(filename, '*')) || (strchr(filename, '$'))) != 0;

    /* point DS:SI to first unparsed character */
    SI_reg(context) += (int)s - (int)filename;
}

static void INT21_GetSystemDate( CONTEXT *context )
{
    SYSTEMTIME systime;
    GetLocalTime( &systime );
    CX_reg(context) = systime.wYear;
    DX_reg(context) = (systime.wMonth << 8) | systime.wDay;
    AX_reg(context) = systime.wDayOfWeek;
}

static void INT21_GetSystemTime( CONTEXT *context )
{
    SYSTEMTIME systime;
    GetLocalTime( &systime );
    CX_reg(context) = (systime.wHour << 8) | systime.wMinute;
    DX_reg(context) = (systime.wSecond << 8) | (systime.wMilliseconds / 10);
}

/* Many calls translate a drive argument like this:
   drive number (00h = default, 01h = A:, etc)
   */  
static char drivestring[]="default";

char *INT21_DriveName(int drive)
{

    if(drive >0)
      {
	drivestring[0]= (unsigned char)drive + '@';
	drivestring[1]=':';
	drivestring[2]=0;
      }
    return drivestring;
}
static BOOL32 INT21_CreateFile( CONTEXT *context )
{
    AX_reg(context) = _lcreat16( CTX_SEG_OFF_TO_LIN(context, DS_reg(context),
                                          EDX_reg(context) ), CX_reg(context) );
    return (AX_reg(context) == (WORD)HFILE_ERROR16);
}


static void OpenExistingFile( CONTEXT *context )
{
    AX_reg(context) = _lopen16( CTX_SEG_OFF_TO_LIN(context, DS_reg(context),EDX_reg(context)),
                               AL_reg(context) );
    if (AX_reg(context) == (WORD)HFILE_ERROR16)
    {
        AX_reg(context) = GetLastError();
        SET_CFLAG(context);
    }
}

static BOOL32 INT21_ExtendedOpenCreateFile(CONTEXT *context )
{
  BOOL32 bExtendedError = FALSE;
  BYTE action = DL_reg(context);

  /* Shuffle arguments to call OpenExistingFile */
  AL_reg(context) = BL_reg(context);
  DX_reg(context) = SI_reg(context);
  /* BX,CX and DX should be preserved */
  OpenExistingFile(context);

  if ((EFL_reg(context) & 0x0001) == 0) /* File exists */
  {
      UINT16	uReturnCX = 0;

      /* Now decide what do do */

      if ((action & 0x07) == 0)
      {
	  _lclose16( AX_reg(context) );
	  AX_reg(context) = 0x0050;	/*File exists*/
	  SET_CFLAG(context);
	  WARN(int21, "extended open/create: failed because file exists \n");
      }
      else if ((action & 0x07) == 2) 
      {
	/* Truncate it, but first check if opened for write */
	if ((BL_reg(context) & 0x0007)== 0) 
	{
            _lclose16( AX_reg(context) );
            WARN(int21, "extended open/create: failed, trunc on ro file\n");
            AX_reg(context) = 0x000C;	/*Access code invalid*/
            SET_CFLAG(context);
	}
	else
	{
		TRACE(int21, "extended open/create: Closing before truncate\n");
                if (_lclose16( AX_reg(context) ))
		{
		   WARN(int21, "extended open/create: close before trunc failed\n");
		   AX_reg(context) = 0x0019;	/*Seek Error*/
		   CX_reg(context) = 0;
		   SET_CFLAG(context);
		}
		/* Shuffle arguments to call CreateFile */

		TRACE(int21, "extended open/create: Truncating\n");
		AL_reg(context) = BL_reg(context);
		/* CX is still the same */
		DX_reg(context) = SI_reg(context);
		bExtendedError = INT21_CreateFile(context);

		if (EFL_reg(context) & 0x0001) 	/*no file open, flags set */
		{
		    WARN(int21, "extended open/create: trunc failed\n");
		    return bExtendedError;
		}
		uReturnCX = 0x3;
	}
      } 
      else uReturnCX = 0x1;

      CX_reg(context) = uReturnCX;
  }
  else /* file does not exist */
  {
      RESET_CFLAG(context); /* was set by OpenExistingFile(context) */
      if ((action & 0xF0)== 0)
      {
	CX_reg(context) = 0;
	SET_CFLAG(context);
	WARN(int21, "extended open/create: failed, file dosen't exist\n");
      }
      else
      {
        /* Shuffle arguments to call CreateFile */
        TRACE(int21, "extended open/create: Creating\n");
        AL_reg(context) = BL_reg(context);
        /* CX should still be the same */
        DX_reg(context) = SI_reg(context);
        bExtendedError = INT21_CreateFile(context);
        if (EFL_reg(context) & 0x0001)  /*no file open, flags set */
	{
  	    WARN(int21, "extended open/create: create failed\n");
	    return bExtendedError;
        }
        CX_reg(context) = 2;
      }
  }

  return bExtendedError;
}


static BOOL32 INT21_ChangeDir( CONTEXT *context )
{
    int drive;
    char *dirname = CTX_SEG_OFF_TO_LIN(context, DS_reg(context),EDX_reg(context));

    TRACE(int21,"changedir %s\n", dirname);
    if (dirname[0] && (dirname[1] == ':'))
    {
        drive = toupper(dirname[0]) - 'A';
        dirname += 2;
    }
    else drive = DRIVE_GetCurrentDrive();
    return DRIVE_Chdir( drive, dirname );
}


static int INT21_FindFirst( CONTEXT *context )
{
    char *p;
    const char *path;
    DOS_FULL_NAME full_name;
    FINDFILE_DTA *dta = (FINDFILE_DTA *)GetCurrentDTA(context);

    path = (const char *)CTX_SEG_OFF_TO_LIN(context, DS_reg(context), EDX_reg(context));
    dta->unixPath = NULL;
    if (!DOSFS_GetFullName( path, FALSE, &full_name ))
    {
        AX_reg(context) = GetLastError();
        SET_CFLAG(context);
        return 0;
    }
    dta->unixPath = HEAP_strdupA( GetProcessHeap(), 0, full_name.long_name );
    p = strrchr( dta->unixPath, '/' );
    *p = '\0';

    /* Note: terminating NULL in dta->mask overwrites dta->search_attr
     *       (doesn't matter as it is set below anyway)
     */
    if (!DOSFS_ToDosFCBFormat( p + 1, dta->mask ))
    {
        HeapFree( GetProcessHeap(), 0, dta->unixPath );
        dta->unixPath = NULL;
        SetLastError( ERROR_FILE_NOT_FOUND );
        AX_reg(context) = ERROR_FILE_NOT_FOUND;
        SET_CFLAG(context);
        return 0;
    }
    dta->drive = (path[0] && (path[1] == ':')) ? toupper(path[0]) - 'A'
                                               : DRIVE_GetCurrentDrive();
    dta->count = 0;
    dta->search_attr = CL_reg(context);
    return 1;
}


static int INT21_FindNext( CONTEXT *context )
{
    FINDFILE_DTA *dta = (FINDFILE_DTA *)GetCurrentDTA(context);
    WIN32_FIND_DATA32A entry;
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
        WARN(int21, "Too many directory entries in %s\n", dta->unixPath );
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


static BOOL32 INT21_CreateTempFile( CONTEXT *context )
{
    static int counter = 0;
    char *name = CTX_SEG_OFF_TO_LIN(context,  DS_reg(context), EDX_reg(context) );
    char *p = name + strlen(name);

    /* despite what Ralf Brown says, some programs seem to call without 
     * ending backslash (DOS accepts that, so we accept it too) */
    if ((p == name) || (p[-1] != '\\')) *p++ = '\\';

    for (;;)
    {
        sprintf( p, "wine%04x.%03d", (int)getpid(), counter );
        counter = (counter + 1) % 1000;

        if ((AX_reg(context) = _lcreat16_uniq( name, 0 )) != (WORD)HFILE_ERROR16)
        {
            TRACE(int21, "created %s\n", name );
            return TRUE;
        }
        if (GetLastError() != ERROR_FILE_EXISTS) return FALSE;
    }
}


static BOOL32 INT21_GetCurrentDirectory( CONTEXT *context ) 
{
    int drive = DOS_GET_DRIVE( DL_reg(context) );
    char *ptr = (char *)CTX_SEG_OFF_TO_LIN(context,  DS_reg(context), ESI_reg(context) );

    if (!DRIVE_IsValid(drive))
    {
        SetLastError( ERROR_INVALID_DRIVE );
        return FALSE;
    }
    lstrcpyn32A( ptr, DRIVE_GetDosCwd(drive), 64 );
    AX_reg(context) = 0x0100;			     /* success return code */
    return TRUE;
}


static void INT21_GetDBCSLeadTable( CONTEXT *context )
{
    if (heap || INT21_CreateHeap())
    { /* return an empty table just as DOS 4.0+ does */
	DS_reg(context) = DosHeapHandle;
	SI_reg(context) = (int)&heap->DummyDBCSLeadTable - (int)heap;
    }
    else
    {
        AX_reg(context) = 0x1; /* error */
        SET_CFLAG(context);
    }
}


static int INT21_GetDiskSerialNumber( CONTEXT *context )
{
    BYTE *dataptr = CTX_SEG_OFF_TO_LIN(context, DS_reg(context), EDX_reg(context));
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


static int INT21_SetDiskSerialNumber( CONTEXT *context )
{
    BYTE *dataptr = CTX_SEG_OFF_TO_LIN(context, DS_reg(context), EDX_reg(context));
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

static int INT21_FindFirstFCB( CONTEXT *context )
{
    BYTE *fcb = (BYTE *)CTX_SEG_OFF_TO_LIN(context, DS_reg(context), EDX_reg(context));
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


static int INT21_FindNextFCB( CONTEXT *context )
{
    BYTE *fcb = (BYTE *)CTX_SEG_OFF_TO_LIN(context, DS_reg(context), EDX_reg(context));
    FINDFILE_FCB *pFCB;
    DOS_DIRENTRY_LAYOUT *pResult = (DOS_DIRENTRY_LAYOUT *)GetCurrentDTA(context);
    WIN32_FIND_DATA32A entry;
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
                    MIN( (p - entry.cAlternateFileName), 8 ) );
            memcpy( pResult->filename + 8, p + 1, MIN( strlen(p), 3 ) );
        }
        else
            memcpy( pResult->filename, entry.cAlternateFileName,
                    MIN( strlen(entry.cAlternateFileName), 8 ) );
    }
    return 1;
}


static void DeleteFileFCB( CONTEXT *context )
{
    FIXME(int21, "(%p): stub\n", context);
}

static void RenameFileFCB( CONTEXT *context )
{
    FIXME(int21, "(%p): stub\n", context);
}



static void fLock( CONTEXT * context )
{

    switch ( AX_reg(context) & 0xff )
    {
        case 0x00: /* LOCK */
	  TRACE(int21,"lock handle %d offset %ld length %ld\n",
		BX_reg(context),
		MAKELONG(DX_reg(context),CX_reg(context)),
		MAKELONG(DI_reg(context),SI_reg(context))) ;
          if (!LockFile(FILE_GetHandle32(BX_reg(context)),
                        MAKELONG(DX_reg(context),CX_reg(context)), 0,
                        MAKELONG(DI_reg(context),SI_reg(context)), 0)) {
	    AX_reg(context) = GetLastError();
	    SET_CFLAG(context);
	  }
          break;

	case 0x01: /* UNLOCK */
	  TRACE(int21,"unlock handle %d offset %ld length %ld\n",
		BX_reg(context),
		MAKELONG(DX_reg(context),CX_reg(context)),
		MAKELONG(DI_reg(context),SI_reg(context))) ;
          if (!UnlockFile(FILE_GetHandle32(BX_reg(context)),
                          MAKELONG(DX_reg(context),CX_reg(context)), 0,
                          MAKELONG(DI_reg(context),SI_reg(context)), 0)) {
	    AX_reg(context) = GetLastError();
	    SET_CFLAG(context);
	  }
	  return;
	default:
	  AX_reg(context) = 0x0001;
	  SET_CFLAG(context);
	  return;
     }
} 

static BOOL32
INT21_networkfunc (CONTEXT *context)
{
     switch (AL_reg(context)) {
     case 0x00: /* Get machine name. */
     {
	  char *dst = CTX_SEG_OFF_TO_LIN (context,DS_reg(context),EDX_reg(context));
	  TRACE(int21, "getting machine name to %p\n", dst);
	  if (gethostname (dst, 15))
	  {
	       WARN(int21,"failed!\n");
	       SetLastError( ER_NoNetwork );
	       return TRUE;
	  } else {
	       int len = strlen (dst);
	       while (len < 15)
		    dst[len++] = ' ';
	       dst[15] = 0;
	       CH_reg(context) = 1; /* Valid */
	       CL_reg(context) = 1; /* NETbios number??? */
	       TRACE(int21, "returning %s\n", debugstr_an (dst, 16));
	       return FALSE;
	  }
     }

     default:
	  SetLastError( ER_NoNetwork );
	  return TRUE;
     }
}

static void INT21_SetCurrentPSP(WORD psp)
{
#ifdef MZ_SUPPORTED
    TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
    NE_MODULE *pModule = pTask ? NE_GetPtr( pTask->hModule ) : NULL;
    
    GlobalUnlock16( GetCurrentTask() );
    if (pModule->lpDosTask)
        pModule->lpDosTask->psp_seg = psp;
    else
#endif
        ERR(int21, "Cannot change PSP for non-DOS task!\n");
}
    
static WORD INT21_GetCurrentPSP()
{
#ifdef MZ_SUPPORTED
    TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
    NE_MODULE *pModule = pTask ? NE_GetPtr( pTask->hModule ) : NULL;
        
    GlobalUnlock16( GetCurrentTask() );
    if (pModule->lpDosTask)
        return pModule->lpDosTask->psp_seg;
    else
#endif
        return GetCurrentPDB();
}


SEGPTR INT21_GetListOfLists()
{
	static DOS_LISTOFLISTS *LOL;
	static SEGPTR seg_LOL;

/*
Output of DOS 6.22:

0133:0020                    6A 13-33 01 CC 00 33 01 59 00         j.3...3.Y.
0133:0030  70 00 00 00 72 02 00 02-6D 00 33 01 00 00 2E 05   p...r...m.3.....
0133:0040  00 00 FC 04 00 00 03 08-92 21 11 E0 04 80 C6 0D   .........!......
0133:0050  CC 0D 4E 55 4C 20 20 20-20 20 00 00 00 00 00 00   ..NUL     ......
0133:0060  00 4B BA C1 06 14 00 00-00 03 01 00 04 70 CE FF   .K...........p..
0133:0070  FF 00 00 00 00 00 00 00-00 01 00 00 0D 05 00 00   ................
0133:0080  00 FF FF 00 00 00 00 FE-00 00 F8 03 FF 9F 70 02   ..............p.
0133:0090  D0 44 C8 FD D4 44 C8 FD-D4 44 C8 FD D0 44 C8 FD   .D...D...D...D..
0133:00A0  D0 44 C8 FD D0 44                                 .D...D
*/
	if (!LOL) {
		LOL = SEGPTR_ALLOC(sizeof(DOS_LISTOFLISTS));

		LOL->CX_Int21_5e01		= 0x0;
		LOL->LRU_count_FCB_cache	= 0x0;
		LOL->LRU_count_FCB_open		= 0x0;
		LOL->OEM_func_handler		= -1; /* not available */
		LOL->INT21_offset		= 0x0;
		LOL->sharing_retry_count	= sharing_retries; /* default value: 3 */
		LOL->sharing_retry_delay	= sharing_pause; /* default value: 1 */
		LOL->ptr_disk_buf		= 0x0;
		LOL->offs_unread_CON		= 0x0;
		LOL->seg_first_MCB		= 0x0;
		LOL->ptr_first_DPB		= 0x0;
		LOL->ptr_first_SysFileTable	= 0x0;
		LOL->ptr_clock_dev_hdr		= 0x0;
		LOL->ptr_CON_dev_hdr		= 0x0;
		LOL->max_byte_per_sec		= 512;
		LOL->ptr_disk_buf_info		= 0x0;
		LOL->ptr_array_CDS		= 0x0;
		LOL->ptr_sys_FCB		= 0x0;
		LOL->nr_protect_FCB		= 0x0;
		LOL->nr_block_dev		= 0x0;
		LOL->nr_avail_drive_letters	= 26; /* A - Z */
		LOL->nr_drives_JOINed		= 0x0;
		LOL->ptr_spec_prg_names		= 0x0;
		LOL->ptr_SETVER_prg_list	= 0x0; /* no SETVER list */
		LOL->DOS_HIGH_A20_func_offs	= 0x0;
		LOL->PSP_last_exec		= 0x0;
		LOL->BUFFERS_val		= 99; /* maximum: 99 */
		LOL->BUFFERS_nr_lookahead	= 8; /* maximum: 8 */
		LOL->boot_drive			= 3; /* C: */
		LOL->flag_DWORD_moves		= 0x01; /* i386+ */
		LOL->size_extended_mem		= 0xf000; /* very high value */
	}	
	if (!seg_LOL) seg_LOL = SEGPTR_GET(LOL);
	return seg_LOL+(WORD)&((DOS_LISTOFLISTS*)0)->ptr_first_DPB;
}


/***********************************************************************
 *           INT21_GetExtendedError
 */
static void INT21_GetExtendedError( CONTEXT *context )
{
    BYTE class, action, locus;
    WORD error = GetLastError();

    switch(error)
    {
    case ERROR_SUCCESS:
        class = action = locus = 0;
        break;
    case ERROR_DIR_NOT_EMPTY:
        class  = EC_Exists;
        action = SA_Ignore;
        locus  = EL_Disk;
        break;
    case ERROR_ACCESS_DENIED:
        class  = EC_AccessDenied;
        action = SA_Abort;
        locus  = EL_Disk;
        break;
    case ERROR_CANNOT_MAKE:
        class  = EC_AccessDenied;
        action = SA_Abort;
        locus  = EL_Unknown;
        break;
    case ERROR_HANDLE_DISK_FULL:
        class  = EC_MediaError;
        action = SA_Abort;
        locus  = EL_Disk;
        break;
    case ERROR_FILE_EXISTS:
        class  = EC_Exists;
        action = SA_Abort;
        locus  = EL_Disk;
        break;
    case ERROR_FILE_NOT_FOUND:
        class  = EC_NotFound;
        action = SA_Abort;
        locus  = EL_Disk;
        break;
    case ER_GeneralFailure:
        class  = EC_SystemFailure;
        action = SA_Abort;
        locus  = EL_Unknown;
        break;
    case ERROR_INVALID_DRIVE:
        class  = EC_MediaError;
        action = SA_Abort;
        locus  = EL_Disk;
        break;
    case ERROR_INVALID_HANDLE:
        class  = EC_ProgramError;
        action = SA_Abort;
        locus  = EL_Disk;
        break;
    case ERROR_LOCK_VIOLATION:
        class  = EC_AccessDenied;
        action = SA_Abort;
        locus  = EL_Disk;
        break;
    case ERROR_NO_MORE_FILES:
        class  = EC_MediaError;
        action = SA_Abort;
        locus  = EL_Disk;
        break;
    case ER_NoNetwork:
        class  = EC_NotFound;
        action = SA_Abort;
        locus  = EL_Network;
        break;
    case ERROR_NOT_ENOUGH_MEMORY:
        class  = EC_OutOfResource;
        action = SA_Abort;
        locus  = EL_Memory;
        break;
    case ERROR_PATH_NOT_FOUND:
        class  = EC_NotFound;
        action = SA_Abort;
        locus  = EL_Disk;
        break;
    case ERROR_SEEK:
        class  = EC_NotFound;
        action = SA_Ignore;
        locus  = EL_Disk;
        break;
    case ERROR_SHARING_VIOLATION:
        class  = EC_Temporary;
        action = SA_Retry;
        locus  = EL_Disk;
        break;
    case ERROR_TOO_MANY_OPEN_FILES:
        class  = EC_ProgramError;
        action = SA_Abort;
        locus  = EL_Disk;
        break;
    default:
        FIXME( int21, "Unknown error %d\n", error );
        class  = EC_SystemFailure;
        action = SA_Abort;
        locus  = EL_Unknown;
        break;
    }
    TRACE( int21, "GET EXTENDED ERROR code 0x%02x class 0x%02x action 0x%02x locus %02x\n",
           error, class, action, locus );
    AX_reg(context) = error;
    BH_reg(context) = class;
    BL_reg(context) = action;
    CH_reg(context) = locus;
}

/***********************************************************************
 *           DOS3Call  (KERNEL.102)
 */
void WINAPI DOS3Call( CONTEXT *context )
{
    BOOL32	bSetDOSExtendedError = FALSE;


    TRACE(int21, "AX=%04x BX=%04x CX=%04x DX=%04x "
	  "SI=%04x DI=%04x DS=%04x ES=%04x EFL=%08lx\n",
	  AX_reg(context), BX_reg(context), CX_reg(context), DX_reg(context),
	  SI_reg(context), DI_reg(context),
	  (WORD)DS_reg(context), (WORD)ES_reg(context),
	  EFL_reg(context) );


    if (AH_reg(context) == 0x59)  /* Get extended error info */
    {
        INT21_GetExtendedError( context );
        return;
    }

    if (AH_reg(context) == 0x0C)  /* Flush buffer and read standard input */
    {
	TRACE(int21, "FLUSH BUFFER AND READ STANDARD INPUT\n");
	/* no flush here yet */
	AH_reg(context) = AL_reg(context);
    }

    if (AH_reg(context)>=0x2f) {
        /* extended error is used by (at least) functions 0x2f to 0x62 */
        SetLastError(0);
    }
    RESET_CFLAG(context);  /* Not sure if this is a good idea */

    switch(AH_reg(context)) 
    {
    case 0x03: /* READ CHARACTER FROM STDAUX  */
    case 0x04: /* WRITE CHARACTER TO STDAUX */
    case 0x05: /* WRITE CHARACTER TO PRINTER */
    case 0x0f: /* OPEN FILE USING FCB */
    case 0x10: /* CLOSE FILE USING FCB */
    case 0x14: /* SEQUENTIAL READ FROM FCB FILE */		
    case 0x15: /* SEQUENTIAL WRITE TO FCB FILE */
    case 0x16: /* CREATE OR TRUNCATE FILE USING FCB */
    case 0x21: /* READ RANDOM RECORD FROM FCB FILE */
    case 0x22: /* WRITE RANDOM RECORD TO FCB FILE */
    case 0x23: /* GET FILE SIZE FOR FCB */
    case 0x24: /* SET RANDOM RECORD NUMBER FOR FCB */
    case 0x26: /* CREATE NEW PROGRAM SEGMENT PREFIX */
    case 0x27: /* RANDOM BLOCK READ FROM FCB FILE */
    case 0x28: /* RANDOM BLOCK WRITE TO FCB FILE */
    case 0x54: /* GET VERIFY FLAG */
        INT_BARF( context, 0x21 );
        break;

    case 0x00: /* TERMINATE PROGRAM */
        TRACE(int21,"TERMINATE PROGRAM\n");
        ExitProcess( 0 );
        break;

    case 0x01: /* READ CHARACTER FROM STANDARD INPUT, WITH ECHO */
        TRACE(int21,"DIRECT CHARACTER INPUT WITH ECHO\n");
	AL_reg(context) = CONSOLE_GetCharacter();
	/* FIXME: no echo */
	break;

    case 0x02: /* WRITE CHARACTER TO STANDARD OUTPUT */
        TRACE(int21, "Write Character to Standard Output\n");
    	CONSOLE_Write(DL_reg(context), 0, 0, 0);
        break;

    case 0x06: /* DIRECT CONSOLE IN/OUTPUT */
        TRACE(int21, "Direct Console Input/Output\n");
        if (DL_reg(context) == 0xff) {
            FIXME(int21,"Direct Console Input should not block\n");
            AL_reg(context) = CONSOLE_GetCharacter();
            FL_reg(context) &= ~0x40; /* clear ZF */
        } else
    	    CONSOLE_Write(DL_reg(context), 0, 0, 0);
        break;

    case 0x07: /* DIRECT CHARACTER INPUT WITHOUT ECHO */
        TRACE(int21,"DIRECT CHARACTER INPUT WITHOUT ECHO\n");
	AL_reg(context) = CONSOLE_GetCharacter();
        break;

    case 0x08: /* CHARACTER INPUT WITHOUT ECHO */
        TRACE(int21,"CHARACTER INPUT WITHOUT ECHO\n");
	AL_reg(context) = CONSOLE_GetCharacter();
        break;

    case 0x09: /* WRITE STRING TO STANDARD OUTPUT */
        TRACE(int21,"WRITE '$'-terminated string from %04lX:%04X to stdout\n",
	      DS_reg(context),DX_reg(context) );
        {
            LPSTR data = CTX_SEG_OFF_TO_LIN(context,DS_reg(context),EDX_reg(context));
            LPSTR p = data;
            /* do NOT use strchr() to calculate the string length,
            as '\0' is valid string content, too !
            Maybe we should check for non-'$' strings, but DOS doesn't. */
            while (*p != '$') p++;
            _hwrite16( 1, data, (int)p - (int)data);
            AL_reg(context) = '$'; /* yes, '$' (0x24) gets returned in AL */
        }
        break;

    case 0x0a: /* BUFFERED INPUT */
      {
	char *buffer = ((char *)CTX_SEG_OFF_TO_LIN(context,  DS_reg(context), 
						   EDX_reg(context) ));
	int res;
	
	TRACE(int21,"BUFFERED INPUT (size=%d)\n",buffer[0]);
	if (buffer[1])
	  TRACE(int21,"Handle old chars in buffer!\n");
	res=_lread16( 0, buffer+2,buffer[0]);
	buffer[1]=res;
	if(buffer[res+1] == '\n')
	  buffer[res+1] = '\r';
	break;
      }

    case 0x0b: {/* GET STDIN STATUS */
    		char x1,x2;

		if (CONSOLE_CheckForKeystroke(&x1,&x2))
		    AL_reg(context) = 0xff; 
		else
		    AL_reg(context) = 0; 
		break;
	}
    case 0x2e: /* SET VERIFY FLAG */
        TRACE(int21,"SET VERIFY FLAG ignored\n");
    	/* we cannot change the behaviour anyway, so just ignore it */
    	break;

    case 0x18: /* NULL FUNCTIONS FOR CP/M COMPATIBILITY */
    case 0x1d:
    case 0x1e:
    case 0x20:
    case 0x6b: /* NULL FUNCTION */
        AL_reg(context) = 0;
        break;
	
    case 0x5c: /* "FLOCK" - RECORD LOCKING */
        fLock(context);
        break;

    case 0x0d: /* DISK BUFFER FLUSH */
	TRACE(int21,"DISK BUFFER FLUSH ignored\n");
        RESET_CFLAG(context); /* dos 6+ only */
        break;

    case 0x0e: /* SELECT DEFAULT DRIVE */
	TRACE(int21,"SELECT DEFAULT DRIVE %d\n", DL_reg(context));
        DRIVE_SetCurrentDrive( DL_reg(context) );
        AL_reg(context) = MAX_DOS_DRIVES;
        break;

    case 0x11: /* FIND FIRST MATCHING FILE USING FCB */
	TRACE(int21,"FIND FIRST MATCHING FILE USING FCB %p\n", 
	      CTX_SEG_OFF_TO_LIN(context, DS_reg(context), EDX_reg(context)));
        if (!INT21_FindFirstFCB(context))
        {
            AL_reg(context) = 0xff;
            break;
        }
        /* else fall through */

    case 0x12: /* FIND NEXT MATCHING FILE USING FCB */
        AL_reg(context) = INT21_FindNextFCB(context) ? 0x00 : 0xff;
        break;

    case 0x13: /* DELETE FILE USING FCB */
        DeleteFileFCB(context);
        break;
            
    case 0x17: /* RENAME FILE USING FCB */
        RenameFileFCB(context);
        break;

    case 0x19: /* GET CURRENT DEFAULT DRIVE */
        AL_reg(context) = DRIVE_GetCurrentDrive();
        break;

    case 0x1a: /* SET DISK TRANSFER AREA ADDRESS */
        {
            TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
            pTask->dta = PTR_SEG_OFF_TO_SEGPTR(DS_reg(context),DX_reg(context));
            TRACE(int21, "Set DTA: %08lx\n", pTask->dta);
        }
        break;

    case 0x1b: /* GET ALLOCATION INFORMATION FOR DEFAULT DRIVE */
        DL_reg(context) = 0;
        if (!INT21_GetDriveAllocInfo(context)) AX_reg(context) = 0xffff;
        break;
	
    case 0x1c: /* GET ALLOCATION INFORMATION FOR SPECIFIC DRIVE */
        if (!INT21_GetDriveAllocInfo(context)) AX_reg(context) = 0xffff;
        break;

    case 0x1f: /* GET DRIVE PARAMETER BLOCK FOR DEFAULT DRIVE */
        GetDrivePB(context, DRIVE_GetCurrentDrive());
        break;
		
    case 0x25: /* SET INTERRUPT VECTOR */
        INT_CtxSetHandler( context, AL_reg(context),
                        (FARPROC16)PTR_SEG_OFF_TO_SEGPTR( DS_reg(context),
                                                          DX_reg(context)));
        break;

    case 0x29: /* PARSE FILENAME INTO FCB */
        INT21_ParseFileNameIntoFCB(context);
        break;

    case 0x2a: /* GET SYSTEM DATE */
        INT21_GetSystemDate(context);
        break;

    case 0x2b: /* SET SYSTEM DATE */
        FIXME(int21, "SetSystemDate(%02d/%02d/%04d): not allowed\n",
	      DL_reg(context), DH_reg(context), CX_reg(context) );
        AL_reg(context) = 0;  /* Let's pretend we succeeded */
        break;

    case 0x2c: /* GET SYSTEM TIME */
        INT21_GetSystemTime(context);
        break;

    case 0x2d: /* SET SYSTEM TIME */
        FIXME(int21, "SetSystemTime(%02d:%02d:%02d.%02d): not allowed\n",
	      CH_reg(context), CL_reg(context),
	      DH_reg(context), DL_reg(context) );
        AL_reg(context) = 0;  /* Let's pretend we succeeded */
        break;

    case 0x2f: /* GET DISK TRANSFER AREA ADDRESS */
        TRACE(int21,"GET DISK TRANSFER AREA ADDRESS\n");
        {
            TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
            ES_reg(context) = SELECTOROF( pTask->dta );
            BX_reg(context) = OFFSETOF( pTask->dta );
        }
        break;
            
    case 0x30: /* GET DOS VERSION */
        TRACE(int21,"GET DOS VERSION %s requested\n",
	      (AL_reg(context) == 0x00)?"OEM number":"version flag");
        AX_reg(context) = (HIWORD(GetVersion16()) >> 8) |
                          (HIWORD(GetVersion16()) << 8);
#if 0
        AH_reg(context) = 0x7;
        AL_reg(context) = 0xA;
#endif

        BX_reg(context) = 0x00FF;     /* 0x123456 is Wine's serial # */
        CX_reg(context) = 0x0000;
        break;

    case 0x31: /* TERMINATE AND STAY RESIDENT */
        FIXME(int21,"TERMINATE AND STAY RESIDENT stub\n");
        break;

    case 0x32: /* GET DOS DRIVE PARAMETER BLOCK FOR SPECIFIC DRIVE */
        TRACE(int21,"GET DOS DRIVE PARAMETER BLOCK FOR DRIVE %s\n",
	      INT21_DriveName( DL_reg(context)));
        GetDrivePB(context, DOS_GET_DRIVE( DL_reg(context) ) );
        break;

    case 0x33: /* MULTIPLEXED */
        switch (AL_reg(context))
        {
	      case 0x00: /* GET CURRENT EXTENDED BREAK STATE */
		TRACE(int21,"GET CURRENT EXTENDED BREAK STATE\n");
                DL_reg(context) = DOSCONF_config.brk_flag;
		break;

	      case 0x01: /* SET EXTENDED BREAK STATE */
		TRACE(int21,"SET CURRENT EXTENDED BREAK STATE\n");
		DOSCONF_config.brk_flag = (DL_reg(context) > 0);
		break;		
		
	      case 0x02: /* GET AND SET EXTENDED CONTROL-BREAK CHECKING STATE*/
		TRACE(int21,"GET AND SET EXTENDED CONTROL-BREAK CHECKING STATE\n");
		/* ugly coding in order to stay reentrant */
		if (DL_reg(context))
		{
		    DL_reg(context) = DOSCONF_config.brk_flag;
		    DOSCONF_config.brk_flag = 1;
		}
		else
		{
		    DL_reg(context) = DOSCONF_config.brk_flag;
		    DOSCONF_config.brk_flag = 0;
		}
		break;

	      case 0x05: /* GET BOOT DRIVE */
		TRACE(int21,"GET BOOT DRIVE\n");
		DL_reg(context) = 3;
		/* c: is Wine's bootdrive (a: is 1)*/
		break;
				
	      case 0x06: /* GET TRUE VERSION NUMBER */
		TRACE(int21,"GET TRUE VERSION NUMBER\n");
		BX_reg(context) = (HIWORD(GetVersion16() >> 8)) |
                                  (HIWORD(GetVersion16() << 8));
		DX_reg(context) = 0x00;
		break;

	      default:
                INT_BARF( context, 0x21 );
		break;			
        }
        break;	
	    
    case 0x34: /* GET ADDRESS OF INDOS FLAG */
        TRACE(int21,"GET ADDRESS OF INDOS FLAG\n");
        if (!heap) INT21_CreateHeap();
        ES_reg(context) = DosHeapHandle;
        BX_reg(context) = (int)&heap->InDosFlag - (int)heap;
        break;

    case 0x35: /* GET INTERRUPT VECTOR */
        TRACE(int21,"GET INTERRUPT VECTOR 0x%02x\n",AL_reg(context));
        {
            FARPROC16 addr = INT_CtxGetHandler( context, AL_reg(context) );
            ES_reg(context) = SELECTOROF(addr);
            BX_reg(context) = OFFSETOF(addr);
        }
        break;

    case 0x36: /* GET FREE DISK SPACE */
	TRACE(int21,"GET FREE DISK SPACE FOR DRIVE %s\n",
	      INT21_DriveName( DL_reg(context)));
        if (!INT21_GetFreeDiskSpace(context)) AX_reg(context) = 0xffff;
        break;

    case 0x37: 
      {
	unsigned char switchchar='/';
	switch (AL_reg(context))
	{
	case 0x00: /* "SWITCHAR" - GET SWITCH CHARACTER */
	  TRACE(int21,"SWITCHAR - GET SWITCH CHARACTER\n");
	  AL_reg(context) = 0x00; /* success*/
	  DL_reg(context) = switchchar;
	  break;
	case 0x01: /*"SWITCHAR" - SET SWITCH CHARACTER*/
	  TRACE(int21,"SWITCHAR - SET SWITCH CHARACTER\n");
	  switchchar = DL_reg(context);
	  AL_reg(context) = 0x00; /* success*/
	  break;
	default: /*"AVAILDEV" - SPECIFY \DEV\ PREFIX USE*/
	  INT_BARF( context, 0x21 );
	  break;
	}
	break;
      }

    case 0x38: /* GET COUNTRY-SPECIFIC INFORMATION */
	TRACE(int21,"GET COUNTRY-SPECIFIC INFORMATION for country 0x%02x\n",
	      AL_reg(context));
        AX_reg(context) = 0x02; /* no country support available */
        SET_CFLAG(context);
        break;

    case 0x39: /* "MKDIR" - CREATE SUBDIRECTORY */
        TRACE(int21,"MKDIR %s\n",
	      (LPCSTR)CTX_SEG_OFF_TO_LIN(context,  DS_reg(context), EDX_reg(context)));
        bSetDOSExtendedError = (!CreateDirectory16( CTX_SEG_OFF_TO_LIN(context,  DS_reg(context),
                                                           EDX_reg(context) ), NULL));
        break;
	
    case 0x3a: /* "RMDIR" - REMOVE SUBDIRECTORY */
        TRACE(int21,"RMDIR %s\n",
	      (LPCSTR)CTX_SEG_OFF_TO_LIN(context,  DS_reg(context), EDX_reg(context)));
        bSetDOSExtendedError = (!RemoveDirectory16( CTX_SEG_OFF_TO_LIN(context,  DS_reg(context),
                                                                 EDX_reg(context) )));
        break;

    case 0x3b: /* "CHDIR" - SET CURRENT DIRECTORY */
        TRACE(int21,"CHDIR %s\n",
	      (LPCSTR)CTX_SEG_OFF_TO_LIN(context,  DS_reg(context), EDX_reg(context)));
        bSetDOSExtendedError = !INT21_ChangeDir(context);
        break;
	
    case 0x3c: /* "CREAT" - CREATE OR TRUNCATE FILE */
        TRACE(int21,"CREAT flag 0x%02x %s\n",CX_reg(context),
	      (LPCSTR)CTX_SEG_OFF_TO_LIN(context,  DS_reg(context), EDX_reg(context)));
        AX_reg(context) = _lcreat16( CTX_SEG_OFF_TO_LIN(context,  DS_reg(context),
                                                        EDX_reg(context) ), CX_reg(context) );
        bSetDOSExtendedError = (AX_reg(context) == (WORD)HFILE_ERROR16);
        break;

    case 0x3d: /* "OPEN" - OPEN EXISTING FILE */
        TRACE(int21,"OPEN mode 0x%02x %s\n",AL_reg(context),
	      (LPCSTR)CTX_SEG_OFF_TO_LIN(context,  DS_reg(context), EDX_reg(context)));
        OpenExistingFile(context);
        break;

    case 0x3e: /* "CLOSE" - CLOSE FILE */
        TRACE(int21,"CLOSE handle %d\n",BX_reg(context));
        bSetDOSExtendedError = ((AX_reg(context) = _lclose16( BX_reg(context) )) != 0);
        break;

    case 0x3f: /* "READ" - READ FROM FILE OR DEVICE */
        TRACE(int21,"READ from %d to %04lX:%04X for %d byte\n",BX_reg(context),
	      DS_reg(context),DX_reg(context),CX_reg(context) );
        {
            LONG result;
            if (ISV86(context))
                result = _hread16( BX_reg(context),
                                   CTX_SEG_OFF_TO_LIN(context, DS_reg(context),
                                                               EDX_reg(context) ),
                                   CX_reg(context) );
            else
                result = WIN16_hread( BX_reg(context),
                                      PTR_SEG_OFF_TO_SEGPTR( DS_reg(context),
                                                             EDX_reg(context) ),
                                      CX_reg(context) );
            if (result == -1) bSetDOSExtendedError = TRUE;
            else AX_reg(context) = (WORD)result;
        }
        break;

    case 0x40: /* "WRITE" - WRITE TO FILE OR DEVICE */
        TRACE(int21,"WRITE from %04lX:%04X to handle %d for %d byte\n",
	      DS_reg(context),DX_reg(context),BX_reg(context),CX_reg(context) );
        {
            LONG result = _hwrite16( BX_reg(context),
                                     CTX_SEG_OFF_TO_LIN(context,  DS_reg(context),
                                                         EDX_reg(context) ),
                                     CX_reg(context) );
            if (result == -1) bSetDOSExtendedError = TRUE;
            else AX_reg(context) = (WORD)result;
        }
        break;

    case 0x41: /* "UNLINK" - DELETE FILE */
        TRACE(int21,"UNLINK%s\n",
	      (LPCSTR)CTX_SEG_OFF_TO_LIN(context,  DS_reg(context), EDX_reg(context)));
        bSetDOSExtendedError = (!DeleteFile32A( CTX_SEG_OFF_TO_LIN(context,  DS_reg(context),
                                                             EDX_reg(context) )));
        break;

    case 0x42: /* "LSEEK" - SET CURRENT FILE POSITION */
        TRACE(int21,"LSEEK handle %d offset %ld from %s\n",
	      BX_reg(context), MAKELONG(DX_reg(context),CX_reg(context)),
	      (AL_reg(context)==0)?"start of file":(AL_reg(context)==1)?
	      "current file position":"end of file");
        {
            LONG status = _llseek16( BX_reg(context),
                                     MAKELONG(DX_reg(context),CX_reg(context)),
                                     AL_reg(context) );
            if (status == -1) bSetDOSExtendedError = TRUE;
	    else
	    {
            	AX_reg(context) = LOWORD(status);
           	DX_reg(context) = HIWORD(status);
	    }
        }
        break;

    case 0x43: /* FILE ATTRIBUTES */
        switch (AL_reg(context))
        {
        case 0x00:
            TRACE(int21,"GET FILE ATTRIBUTES for %s\n", 
		  (LPCSTR)CTX_SEG_OFF_TO_LIN(context,  DS_reg(context), EDX_reg(context)));
            AX_reg(context) = (WORD)GetFileAttributes32A(
                                          CTX_SEG_OFF_TO_LIN(context, DS_reg(context),
                                                             EDX_reg(context)));
            if (AX_reg(context) == 0xffff) bSetDOSExtendedError = TRUE;
            else CX_reg(context) = AX_reg(context);
            break;

        case 0x01:
            TRACE(int21,"SET FILE ATTRIBUTES 0x%02x for %s\n", CX_reg(context),
		  (LPCSTR)CTX_SEG_OFF_TO_LIN(context,  DS_reg(context), EDX_reg(context)));
            bSetDOSExtendedError = 
		(!SetFileAttributes32A( CTX_SEG_OFF_TO_LIN(context, DS_reg(context), 
							   EDX_reg(context)),
                                       			   CX_reg(context) ));
            break;
        case 0x02:
            TRACE(int21,"GET COMPRESSED FILE SIZE for %s stub\n",
		  (LPCSTR)CTX_SEG_OFF_TO_LIN(context,  DS_reg(context), EDX_reg(context)));
        }
        break;
	
    case 0x44: /* IOCTL */
        switch (AL_reg(context))
        {
        case 0x00:
            ioctlGetDeviceInfo(context);
            break;

        case 0x01:
            break;
        case 0x02:{
            const DOS_DEVICE *dev;
            if ((dev = DOSFS_GetDeviceByHandle( FILE_GetHandle32(BX_reg(context)) )) &&
                !strcasecmp( dev->name, "SCSIMGR$" ))
            {
                ASPI_DOS_HandleInt(context);
            }
            break;
       }
	case 0x05:{	/* IOCTL - WRITE TO BLOCK DEVICE CONTROL CHANNEL */
	    /*BYTE *dataptr = CTX_SEG_OFF_TO_LIN(context, DS_reg(context),EDX_reg(context));*/
	    int	drive = DOS_GET_DRIVE(BL_reg(context));

	    FIXME(int21,"program tried to write to block device control channel of drive %d:\n",drive);
	    /* for (i=0;i<CX_reg(context);i++)
	    	fprintf(stdnimp,"%02x ",dataptr[i]);
	    fprintf(stdnimp,"\n");*/
	    AX_reg(context)=CX_reg(context);
	    break;
	}
        case 0x08:   /* Check if drive is removable. */
            TRACE(int21,"IOCTL - CHECK IF BLOCK DEVICE REMOVABLE for drive %s\n",
		 INT21_DriveName( BL_reg(context)));
            switch(GetDriveType16( DOS_GET_DRIVE( BL_reg(context) )))
            {
            case DRIVE_CANNOTDETERMINE:
                SetLastError( ERROR_INVALID_DRIVE );
                AX_reg(context) = ERROR_INVALID_DRIVE;
                SET_CFLAG(context);
                break;
            case DRIVE_REMOVABLE:
                AX_reg(context) = 0;      /* removable */
                break;
            default:
                AX_reg(context) = 1;   /* not removable */
                break;
            }
            break;

        case 0x09:   /* CHECK IF BLOCK DEVICE REMOTE */
            TRACE(int21,"IOCTL - CHECK IF BLOCK DEVICE REMOTE for drive %s\n",
		 INT21_DriveName( BL_reg(context))); 
            switch(GetDriveType16( DOS_GET_DRIVE( BL_reg(context) )))
            {
            case DRIVE_CANNOTDETERMINE:
                SetLastError( ERROR_INVALID_DRIVE );
                AX_reg(context) = ERROR_INVALID_DRIVE;
                SET_CFLAG(context);
                break;
            case DRIVE_REMOTE:
                DX_reg(context) = (1<<9) | (1<<12);  /* remote */
                break;
            default:
                DX_reg(context) = 0;  /* FIXME: use driver attr here */
                break;
            }
            break;

        case 0x0a: /* check if handle (BX) is remote */
            TRACE(int21,"IOCTL - CHECK IF HANDLE %d IS REMOTE\n",BX_reg(context));
            /* returns DX, bit 15 set if remote, bit 14 set if date/time
             * not set on close
             */
            DX_reg(context) = 0;
            break;

        case 0x0b:   /* SET SHARING RETRY COUNT */
            TRACE(int21,"IOCTL - SET SHARING RETRY COUNT pause %d retries %d\n",
		 CX_reg(context), DX_reg(context));
            if (!CX_reg(context))
            { 
                AX_reg(context) = 1;
                SET_CFLAG(context);
                break;
            }
            sharing_pause = CX_reg(context);
            if (!DX_reg(context))
                sharing_retries = DX_reg(context);
            RESET_CFLAG(context);
            break;

        case 0x0d:
            TRACE(int21,"IOCTL - GENERIC BLOCK DEVICE REQUEST %s\n",
		  INT21_DriveName( BL_reg(context)));
            bSetDOSExtendedError = ioctlGenericBlkDevReq(context);
            break;

	case 0x0e: /* get logical drive mapping */
            TRACE(int21,"IOCTL - GET LOGICAL DRIVE MAP for drive %s\n",
		  INT21_DriveName( BL_reg(context)));
	    AL_reg(context) = 0; /* drive has no mapping - FIXME: may be wrong*/
	    break;

        case 0x0F:   /* Set logical drive mapping */
	    {
	    int drive;
            TRACE(int21,"IOCTL - SET LOGICAL DRIVE MAP for drive %s\n",
		  INT21_DriveName( BL_reg(context))); 
	    drive = DOS_GET_DRIVE ( BL_reg(context) );
	    if ( ! DRIVE_SetLogicalMapping ( drive, drive+1 ) )
	    {
		SET_CFLAG(context);
		AX_reg(context) = 0x000F;  /* invalid drive */
	    }
            break;
	    }

        case 0xe0:  /* Sun PC-NFS API */
            /* not installed */
            break;
                
        default:
            INT_BARF( context, 0x21 );
            break;
        }
        break;

    case 0x45: /* "DUP" - DUPLICATE FILE HANDLE */
        {
            HANDLE32 handle;
            TRACE(int21,"DUP - DUPLICATE FILE HANDLE %d\n",BX_reg(context));
            if ((bSetDOSExtendedError = !DuplicateHandle( GetCurrentProcess(),
                                                          FILE_GetHandle32(BX_reg(context)),
                                                          GetCurrentProcess(), &handle,
                                                          0, TRUE, DUPLICATE_SAME_ACCESS )))
                AX_reg(context) = HFILE_ERROR16;
            else
                AX_reg(context) = FILE_AllocDosHandle(handle);
            break;
        }

    case 0x46: /* "DUP2", "FORCEDUP" - FORCE DUPLICATE FILE HANDLE */
        TRACE(int21,"FORCEDUP - FORCE DUPLICATE FILE HANDLE %d to %d\n",
	      BX_reg(context),CX_reg(context));
        bSetDOSExtendedError = (FILE_Dup2( BX_reg(context), CX_reg(context) ) == HFILE_ERROR16);
        break;

    case 0x47: /* "CWD" - GET CURRENT DIRECTORY */
        TRACE(int21,"CWD - GET CURRENT DIRECTORY for drive %s\n",
	      INT21_DriveName( DL_reg(context)));
        bSetDOSExtendedError = !INT21_GetCurrentDirectory(context);
        break;

    case 0x48: /* ALLOCATE MEMORY */
        TRACE(int21,"ALLOCATE MEMORY for %d paragraphs\n", BX_reg(context));
        {
            LPVOID *mem; 
	    if (ISV86(context))
	      {
		mem= DOSMEM_GetBlock(0,(DWORD)BX_reg(context)<<4,NULL);
            if (mem)
                AX_reg(context) = DOSMEM_MapLinearToDos(mem)>>4;
	      }
	    else
	      {
		mem = (LPVOID)GlobalDOSAlloc(BX_reg(context)<<4);
		if (mem)
		  AX_reg(context) = (DWORD)mem&0xffff;
	      }
	    if (!mem)
	      {
                SET_CFLAG(context);
                AX_reg(context) = 0x0008; /* insufficient memory */
                BX_reg(context) = DOSMEM_Available(0)>>4;
            }
        }
        break;

    case 0x49: /* FREE MEMORY */
        TRACE(int21,"FREE MEMORY segment %04lX\n", ES_reg(context));
        {
	  BOOL32 ret;
	  if (ISV86(context))
	    ret= DOSMEM_FreeBlock(0,DOSMEM_MapDosToLinear(ES_reg(context)<<4));
	  else
	    {
	      ret = !GlobalDOSFree(ES_reg(context));
	      /* If we don't reset ES_reg, we will fail in the relay code */
	      ES_reg(context)=ret;
	    }
	  if (!ret)
	    {
	      TRACE(int21,"FREE MEMORY failed\n");
            SET_CFLAG(context);
            AX_reg(context) = 0x0009; /* memory block address invalid */
        }
	}
        break;

    case 0x4a: /* RESIZE MEMORY BLOCK */
        TRACE(int21,"RESIZE MEMORY segment %04lX to %d paragraphs\n", ES_reg(context), BX_reg(context));
	if (!ISV86(context))
	  FIXME(int21,"RESIZE MEMORY probably insufficent implementation. Expect crash soon\n");
	{
	    LPVOID *mem = DOSMEM_ResizeBlock(0,DOSMEM_MapDosToLinear(ES_reg(context)<<4),
					       BX_reg(context)<<4,NULL);
	    if (mem)
		AX_reg(context) = DOSMEM_MapLinearToDos(mem)>>4;
	    else {
		SET_CFLAG(context);
		AX_reg(context) = 0x0008; /* insufficient memory */
		BX_reg(context) = DOSMEM_Available(0)>>4; /* not quite right */
	    }
	}
        break;

    case 0x4b: /* "EXEC" - LOAD AND/OR EXECUTE PROGRAM */
        TRACE(int21,"EXEC %s\n",
	      (LPCSTR)CTX_SEG_OFF_TO_LIN(context,  DS_reg(context),EDX_reg(context) ));
        AX_reg(context) = WinExec16( CTX_SEG_OFF_TO_LIN(context,  DS_reg(context),
                                                         EDX_reg(context) ),
                                     SW_NORMAL );
        if (AX_reg(context) < 32) SET_CFLAG(context);
        break;		
	
    case 0x4c: /* "EXIT" - TERMINATE WITH RETURN CODE */
        TRACE(int21,"EXIT with return code %d\n",AL_reg(context));
        ExitProcess( AL_reg(context) );
        break;

    case 0x4d: /* GET RETURN CODE */
        TRACE(int21,"GET RETURN CODE (ERRORLEVEL)\n");
        AX_reg(context) = 0; /* normal exit */
        break;

    case 0x4e: /* "FINDFIRST" - FIND FIRST MATCHING FILE */
        TRACE(int21,"FINDFIRST mask 0x%04x spec %s\n",CX_reg(context),
	      (LPCSTR)CTX_SEG_OFF_TO_LIN(context,  DS_reg(context), EDX_reg(context)));
        if (!INT21_FindFirst(context)) break;
        /* fall through */

    case 0x4f: /* "FINDNEXT" - FIND NEXT MATCHING FILE */
        TRACE(int21,"FINDNEXT\n");
        if (!INT21_FindNext(context))
        {
            SetLastError( ERROR_NO_MORE_FILES );
            AX_reg(context) = ERROR_NO_MORE_FILES;
            SET_CFLAG(context);
        }
        else AX_reg(context) = 0;  /* OK */
        break;
    case 0x50: /* SET CURRENT PROCESS ID (SET PSP ADDRESS) */
        TRACE(int21, "SET CURRENT PROCESS ID (SET PSP ADDRESS)\n");
        INT21_SetCurrentPSP(BX_reg(context));
        break;
    case 0x51: /* GET PSP ADDRESS */
        TRACE(int21,"GET CURRENT PROCESS ID (GET PSP ADDRESS)\n");
        /* FIXME: should we return the original DOS PSP upon */
        /*        Windows startup ? */
        BX_reg(context) = INT21_GetCurrentPSP();
        break;
    case 0x62: /* GET PSP ADDRESS */
        TRACE(int21,"GET CURRENT PSP ADDRESS\n");
        /* FIXME: should we return the original DOS PSP upon */
        /*        Windows startup ? */
        BX_reg(context) = INT21_GetCurrentPSP();
        break;

    case 0x52: /* "SYSVARS" - GET LIST OF LISTS */
        TRACE(int21,"SYSVARS - GET LIST OF LISTS\n");
        {
                SEGPTR lol;
                lol = INT21_GetListOfLists();
                ES_reg(context) = HIWORD(lol);
                BX_reg(context) = LOWORD(lol);
        }
        break;

    case 0x56: /* "RENAME" - RENAME FILE */
        TRACE(int21,"RENAME %s to %s\n",
	      (LPCSTR)CTX_SEG_OFF_TO_LIN(context, DS_reg(context),EDX_reg(context)),
	      (LPCSTR)CTX_SEG_OFF_TO_LIN(context, ES_reg(context),EDI_reg(context)));
        bSetDOSExtendedError = 
		(!MoveFile32A( CTX_SEG_OFF_TO_LIN(context, DS_reg(context),EDX_reg(context)),
			       CTX_SEG_OFF_TO_LIN(context, ES_reg(context),EDI_reg(context))));
        break;

    case 0x57: /* FILE DATE AND TIME */
        switch (AL_reg(context))
        {
        case 0x00:  /* Get */
            {
                FILETIME filetime;
                TRACE(int21,"GET FILE DATE AND TIME for handle %d\n",
		      BX_reg(context));
                if (!GetFileTime( FILE_GetHandle32(BX_reg(context)), NULL, NULL, &filetime ))
		     bSetDOSExtendedError = TRUE;
                else FileTimeToDosDateTime( &filetime, &DX_reg(context),
                                            &CX_reg(context) );
            }
            break;

        case 0x01:  /* Set */
            {
                FILETIME filetime;
                TRACE(int21,"SET FILE DATE AND TIME for handle %d\n",
		      BX_reg(context));
                DosDateTimeToFileTime( DX_reg(context), CX_reg(context),
                                       &filetime );
                bSetDOSExtendedError = 
			(!SetFileTime( FILE_GetHandle32(BX_reg(context)),
                                      NULL, NULL, &filetime ));
            }
            break;
        }
        break;

    case 0x58: /* GET OR SET MEMORY/UMB ALLOCATION STRATEGY */
	TRACE(int21,"GET OR SET MEMORY/UMB ALLOCATION STRATEGY subfunction %d\n",
	      AL_reg(context));
        switch (AL_reg(context))
        {
        case 0x00:
            AX_reg(context) = 1;
            break;
        case 0x02:
            AX_reg(context) = 0;
            break;
        case 0x01:
        case 0x03:
            break;
        }
        RESET_CFLAG(context);
        break;

    case 0x5a: /* CREATE TEMPORARY FILE */
        TRACE(int21,"CREATE TEMPORARY FILE\n");
        bSetDOSExtendedError = !INT21_CreateTempFile(context);
        break;

    case 0x5b: /* CREATE NEW FILE */
        TRACE(int21,"CREATE NEW FILE 0x%02x for %s\n", CX_reg(context),
	      (LPCSTR)CTX_SEG_OFF_TO_LIN(context,  DS_reg(context), EDX_reg(context)));
        bSetDOSExtendedError = ((AX_reg(context) = 
               _lcreat16_uniq( CTX_SEG_OFF_TO_LIN(context, DS_reg(context),EDX_reg(context)), 0 ))
								== (WORD)HFILE_ERROR16);
        break;

    case 0x5d: /* NETWORK */
        FIXME(int21,"Function 0x%04x not implemented.\n", AX_reg (context));
	/* Fix the following while you're at it.  */
        SetLastError( ER_NoNetwork );
	bSetDOSExtendedError = TRUE;
        break;

    case 0x5e:
	bSetDOSExtendedError = INT21_networkfunc (context);
        break;

    case 0x5f: /* NETWORK */
        switch (AL_reg(context))
        {
        case 0x07: /* ENABLE DRIVE */
            TRACE(int21,"ENABLE DRIVE %c:\n",(DL_reg(context)+'A'));
            if (!DRIVE_Enable( DL_reg(context) ))
            {
                SetLastError( ERROR_INVALID_DRIVE );
		bSetDOSExtendedError = TRUE;
            }
            break;

        case 0x08: /* DISABLE DRIVE */
            TRACE(int21,"DISABLE DRIVE %c:\n",(DL_reg(context)+'A'));
            if (!DRIVE_Disable( DL_reg(context) ))
            {
                SetLastError( ERROR_INVALID_DRIVE );
		bSetDOSExtendedError = TRUE;
            } 
            break;

        default:
            /* network software not installed */
            TRACE(int21,"NETWORK function AX=%04x not implemented\n",AX_reg(context));
            SetLastError( ER_NoNetwork );
	    bSetDOSExtendedError = TRUE;
            break;
        }
        break;

    case 0x60: /* "TRUENAME" - CANONICALIZE FILENAME OR PATH */
        TRACE(int21,"TRUENAME %s\n",
	      (LPCSTR)CTX_SEG_OFF_TO_LIN(context, DS_reg(context),ESI_reg(context)));
        {
            if (!GetFullPathName32A( CTX_SEG_OFF_TO_LIN(context, DS_reg(context),
                                                        ESI_reg(context)), 128,
                                     CTX_SEG_OFF_TO_LIN(context, ES_reg(context),
                                                        EDI_reg(context)),NULL))
		bSetDOSExtendedError = TRUE;
            else AX_reg(context) = 0;
        }
        break;

    case 0x61: /* UNUSED */
    case 0x63: /* misc. language support */
        switch (AL_reg(context)) {
        case 0x00: /* GET DOUBLE BYTE CHARACTER SET LEAD-BYTE TABLE */
	    INT21_GetDBCSLeadTable(context);
            break;
        }
        break;
    case 0x64: /* OS/2 DOS BOX */
        INT_BARF( context, 0x21 );
        SET_CFLAG(context);
    	break;

    case 0x65:{/* GET EXTENDED COUNTRY INFORMATION */
    	extern WORD WINE_LanguageId;
	BYTE    *dataptr=CTX_SEG_OFF_TO_LIN(context, ES_reg(context),EDI_reg(context));
	TRACE(int21,"GET EXTENDED COUNTRY INFORMATION code page %d country %d\n",
	      BX_reg(context), DX_reg(context));
    	switch (AL_reg(context)) {
	case 0x01:
	    TRACE(int21,"\tget general internationalization info\n");
	    dataptr[0] = 0x1;
	    *(WORD*)(dataptr+1) = 41;
	    *(WORD*)(dataptr+3) = WINE_LanguageId;
	    *(WORD*)(dataptr+5) = CodePage;
	    *(DWORD*)(dataptr+0x19) = 0; /* FIXME: ptr to case map routine */
	    break;
	case 0x06:
	    TRACE(int21,"\tget pointer to collating sequence table\n");
	    dataptr[0] = 0x06;
	    *(DWORD*)(dataptr+1) = MAKELONG(DOSMEM_CollateTable & 0xFFFF,DOSMEM_AllocSelector(DOSMEM_CollateTable>>16));
	    CX_reg(context)         = 258;/*FIXME: size of table?*/
	    break;
	default:
	    TRACE(int21,"\tunimplemented function %d\n",AL_reg(context));
            INT_BARF( context, 0x21 );
            SET_CFLAG(context);
    	    break;
	}
    	break;
    }
    case 0x66: /* GLOBAL CODE PAGE TABLE */
        switch (AL_reg(context))
        {
        case 0x01:
	    TRACE(int21,"GET GLOBAL CODE PAGE TABLE\n");
            DX_reg(context) = BX_reg(context) = CodePage;
            RESET_CFLAG(context);
            break;			
        case 0x02: 
	    TRACE(int21,"SET GLOBAL CODE PAGE TABLE active page %d system page %d\n",
		  BX_reg(context),DX_reg(context));
            CodePage = BX_reg(context);
            RESET_CFLAG(context);
            break;
        }
        break;

    case 0x67: /* SET HANDLE COUNT */
        TRACE(int21,"SET HANDLE COUNT to %d\n",BX_reg(context) );
        SetHandleCount16( BX_reg(context) );
        if (GetLastError()) bSetDOSExtendedError = TRUE;
        break;

    case 0x68: /* "FFLUSH" - COMMIT FILE */
    case 0x6a: /* COMMIT FILE */
        TRACE(int21,"FFLUSH/COMMIT handle %d\n",BX_reg(context));
        bSetDOSExtendedError = (!FlushFileBuffers( FILE_GetHandle32(BX_reg(context)) ));
        break;		
	
    case 0x69: /* DISK SERIAL NUMBER */
        switch (AL_reg(context))
        {
        case 0x00:
	    TRACE(int21,"GET DISK SERIAL NUMBER for drive %s\n",
		  INT21_DriveName(BL_reg(context)));
            if (!INT21_GetDiskSerialNumber(context)) bSetDOSExtendedError = TRUE;
            else AX_reg(context) = 0;
            break;

        case 0x01:
	    TRACE(int21,"SET DISK SERIAL NUMBER for drive %s\n",
		  INT21_DriveName(BL_reg(context)));
            if (!INT21_SetDiskSerialNumber(context)) bSetDOSExtendedError = TRUE;
            else AX_reg(context) = 1;
            break;
        }
        break;
    
    case 0x6C: /* Extended Open/Create*/
        TRACE(int21,"EXTENDED OPEN/CREATE %s\n",
	      (LPCSTR)CTX_SEG_OFF_TO_LIN(context,  DS_reg(context), EDI_reg(context)));
        bSetDOSExtendedError = INT21_ExtendedOpenCreateFile(context);
        break;
	
    case 0x71: /* MS-DOS 7 (Windows95) - LONG FILENAME FUNCTIONS */
	if ((GetVersion32()&0xC0000004)!=0xC0000004) {
	    /* not supported on anything but Win95 */
	    TRACE(int21,"LONG FILENAME functions supported only by win95\n");
	    SET_CFLAG(context);
	    AL_reg(context) = 0;
	} else
        switch(AL_reg(context))
        {
        case 0x39:  /* Create directory */
	    TRACE(int21,"LONG FILENAME - MAKE DIRECTORY %s\n",
		  (LPCSTR)CTX_SEG_OFF_TO_LIN(context,  DS_reg(context),EDX_reg(context)));
            bSetDOSExtendedError = (!CreateDirectory32A( 
					CTX_SEG_OFF_TO_LIN(context,  DS_reg(context),
                                                  EDX_reg(context) ), NULL));
            break;
        case 0x3a:  /* Remove directory */
	    TRACE(int21,"LONG FILENAME - REMOVE DIRECTORY %s\n",
		  (LPCSTR)CTX_SEG_OFF_TO_LIN(context,  DS_reg(context),EDX_reg(context)));
            bSetDOSExtendedError = (!RemoveDirectory32A( 
					CTX_SEG_OFF_TO_LIN(context,  DS_reg(context),
                                                        EDX_reg(context) )));
            break;
        case 0x43:  /* Get/Set file attributes */
	  TRACE(int21,"LONG FILENAME -EXTENDED GET/SET FILE ATTRIBUTES %s\n",
		(LPCSTR)CTX_SEG_OFF_TO_LIN(context,  DS_reg(context),EDX_reg(context)));
        switch (BL_reg(context))
        {
        case 0x00: /* Get file attributes */
            TRACE(int21,"\tretrieve attributes\n");
            CX_reg(context) = (WORD)GetFileAttributes32A(
                                          CTX_SEG_OFF_TO_LIN(context, DS_reg(context),
                                                             EDX_reg(context)));
            if (CX_reg(context) == 0xffff) bSetDOSExtendedError = TRUE;
            break;
        case 0x01:
            TRACE(int21,"\tset attributes 0x%04x\n",CX_reg(context));
            bSetDOSExtendedError = (!SetFileAttributes32A( 
				  	CTX_SEG_OFF_TO_LIN(context, DS_reg(context),
                                                           EDX_reg(context)),
                                        CX_reg(context)  ) );
            break;
	default:
	  FIXME(int21, "Unimplemented long file name function:\n");
	  INT_BARF( context, 0x21 );
	  SET_CFLAG(context);
	  AL_reg(context) = 0;
	  break;
	}
	break;
        case 0x47:  /* Get current directory */
	    TRACE(int21," LONG FILENAME - GET CURRENT DIRECTORY for drive %s\n",
		  INT21_DriveName(DL_reg(context)));
	    bSetDOSExtendedError = !INT21_GetCurrentDirectory(context);
            break;

        case 0x4e:  /* Find first file */
	    TRACE(int21," LONG FILENAME - FIND FIRST MATCHING FILE for %s\n",
		  (LPCSTR)CTX_SEG_OFF_TO_LIN(context, DS_reg(context),EDX_reg(context)));
            /* FIXME: use attributes in CX */
            if ((AX_reg(context) = FindFirstFile16(
                   CTX_SEG_OFF_TO_LIN(context, DS_reg(context),EDX_reg(context)),
                   (WIN32_FIND_DATA32A *)CTX_SEG_OFF_TO_LIN(context, ES_reg(context),
                                                            EDI_reg(context))))
		== INVALID_HANDLE_VALUE16)
		bSetDOSExtendedError = TRUE;
            break;
        case 0x4f:  /* Find next file */
	    TRACE(int21,"LONG FILENAME - FIND NEXT MATCHING FILE for handle %d\n",
		  BX_reg(context));
            if (!FindNextFile16( BX_reg(context),
                    (WIN32_FIND_DATA32A *)CTX_SEG_OFF_TO_LIN(context, ES_reg(context),
                                                             EDI_reg(context))))
		bSetDOSExtendedError = TRUE;
            break;
        case 0xa1:  /* Find close */
	    TRACE(int21,"LONG FILENAME - FINDCLOSE for handle %d\n",
		  BX_reg(context));
            bSetDOSExtendedError = (!FindClose16( BX_reg(context) ));
            break;
	case 0xa0:
	    TRACE(int21,"LONG FILENAME - GET VOLUME INFORMATION for drive %s stub\n",
		  (LPCSTR)CTX_SEG_OFF_TO_LIN(context,  DS_reg(context),EDX_reg(context)));
	    break;
        case 0x60:  
	  switch(CL_reg(context))
	  {
	    case 0x01:  /*Get short filename or path */
	      if (!GetShortPathName32A
		  ( CTX_SEG_OFF_TO_LIN(context, DS_reg(context),
				       ESI_reg(context)),
		    CTX_SEG_OFF_TO_LIN(context, ES_reg(context),
				       EDI_reg(context)), 67))
		bSetDOSExtendedError = TRUE;
	      else AX_reg(context) = 0;
	      break;
	    case 0x02:  /*Get canonical long filename or path */
	      if (!GetFullPathName32A
		  ( CTX_SEG_OFF_TO_LIN(context, DS_reg(context),
				       ESI_reg(context)), 128,
		    CTX_SEG_OFF_TO_LIN(context, ES_reg(context),
				       EDI_reg(context)),NULL))
		bSetDOSExtendedError = TRUE;
	      else AX_reg(context) = 0;
	      break;
	    default:
	      FIXME(int21, "Unimplemented long file name function:\n");
	      INT_BARF( context, 0x21 );
	      SET_CFLAG(context);
	      AL_reg(context) = 0;
	      break;
	  }
	    break;
        case 0x6c:  /* Create or open file */
            TRACE(int21,"LONG FILENAME - CREATE OR OPEN FILE %s\n",
		 (LPCSTR)CTX_SEG_OFF_TO_LIN(context,  DS_reg(context), ESI_reg(context))); 
	  /* translate Dos 7 action to Dos 6 action */
	    bSetDOSExtendedError = INT21_ExtendedOpenCreateFile(context);
	    break;

        case 0x3b:  /* Change directory */
            TRACE(int21,"LONG FILENAME - CHANGE DIRECTORY %s\n",
		 (LPCSTR)CTX_SEG_OFF_TO_LIN(context,  DS_reg(context), EDX_reg(context))); 
	    if (!SetCurrentDirectory32A(CTX_SEG_OFF_TO_LIN(context, 
	    					DS_reg(context),
				        	EDX_reg(context)
					))
	    ) {
		SET_CFLAG(context);
		AL_reg(context) = GetLastError();
	    }
	    break;
        case 0x41:  /* Delete file */
            TRACE(int21,"LONG FILENAME - DELETE FILE %s\n",
		 (LPCSTR)CTX_SEG_OFF_TO_LIN(context,  DS_reg(context), EDX_reg(context))); 
	    if (!DeleteFile32A(CTX_SEG_OFF_TO_LIN(context, 
	    				DS_reg(context),
					EDX_reg(context))
	    )) {
		SET_CFLAG(context);
		AL_reg(context) = GetLastError();
	    }
	    break;
        case 0x56:  /* Move (rename) file */
            TRACE(int21,"LONG FILENAME - RENAME FILE %s to %s stub\n",
		  (LPCSTR)CTX_SEG_OFF_TO_LIN(context,  DS_reg(context), EDX_reg(context)),
		  (LPCSTR)CTX_SEG_OFF_TO_LIN(context,  ES_reg(context), EDI_reg(context))); 
        default:
            FIXME(int21, "Unimplemented long file name function:\n");
            INT_BARF( context, 0x21 );
            SET_CFLAG(context);
            AL_reg(context) = 0;
            break;
        }
        break;

    case 0x70: /* MS-DOS 7 (Windows95) - ??? (country-specific?)*/
    case 0x72: /* MS-DOS 7 (Windows95) - ??? */
    case 0x73: /* MS-DOS 7 (Windows95) - DRIVE LOCKING ??? */
        TRACE(int21,"windows95 function AX %04x\n",
                    AX_reg(context));
        WARN(int21, "        returning unimplemented\n");
        SET_CFLAG(context);
        AL_reg(context) = 0;
        break;

    case 0xdc: /* CONNECTION SERVICES - GET CONNECTION NUMBER */
    case 0xea: /* NOVELL NETWARE - RETURN SHELL VERSION */
        break;

    default:
        INT_BARF( context, 0x21 );
        break;

    } /* END OF SWITCH */

    if( bSetDOSExtendedError )		/* set general error condition */
    {   
	AX_reg(context) = GetLastError();
	SET_CFLAG(context);
    }

    if ((EFL_reg(context) & 0x0001))
        TRACE(int21, "failed, error 0x%04lx\n", GetLastError() );
 
    TRACE(int21, "returning: AX=%04x BX=%04x CX=%04x DX=%04x "
                 "SI=%04x DI=%04x DS=%04x ES=%04x EFL=%08lx\n",
                 AX_reg(context), BX_reg(context), CX_reg(context),
                 DX_reg(context), SI_reg(context), DI_reg(context),
                 (WORD)DS_reg(context), (WORD)ES_reg(context),
                 EFL_reg(context));
}

FARPROC16 WINAPI GetSetKernelDOSProc(FARPROC16 DosProc)
{
	FIXME(int21, "(DosProc=0x%08x): stub\n", (UINT32)DosProc);
	return NULL;
}

