/*
 * (c) 1993, 1994 Erik Bos
 */

#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>
#include <ctype.h>
#include "dos_fs.h"
#include "drive.h"
#include "file.h"
#include "windows.h"
#include "msdos.h"
#include "registers.h"
#include "ldt.h"
#include "task.h"
#include "options.h"
#include "miscemu.h"
#include "xmalloc.h"
#include "stddebug.h"
#include "debug.h"
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
struct DPB *dpb;
DWORD dpbsegptr;

struct DosHeap {
	BYTE InDosFlag;
        BYTE mediaID;
	BYTE biosdate[8];
        struct DPB dpb;
};
static struct DosHeap *heap;
static WORD DosHeapHandle;

WORD sharing_retries = 3;      /* number of retries at sharing violation */
WORD sharing_pause = 1;        /* pause between retries */

extern char TempDirectory[];

BYTE *GetCurrentDTA(void)
{
    TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
    return (BYTE *)PTR_SEG_TO_LIN( pTask->dta );
}


void ChopOffWhiteSpace(char *string)
{
	int length;

	for (length = strlen(string) ; length ; length--)
		if (string[length] == ' ')
			string[length] = '\0';
}

static void CreateBPB(int drive, BYTE *data)
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
		setword(&data[0x1f], 800);
		data[0x21] = 5;
		setword(&data[0x22], 1);
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
		setword(&data[0x1f], 80);
		data[0x21] = 7;
		setword(&data[0x22], 2);
	}	
}

static int INT21_GetFreeDiskSpace(struct sigcontext_struct *context)
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

static int INT21_GetDriveAllocInfo(struct sigcontext_struct *context)
{
    if (!INT21_GetFreeDiskSpace( context )) return 0;
    heap->mediaID = 0xf0;
    DS_reg(context) = DosHeapHandle;
    BX_reg(context) = (int)&heap->mediaID - (int)heap;
    return 1;
}

static void GetDrivePB(struct sigcontext_struct *context, int drive)
{
        if(!DRIVE_IsValid(drive))
        {
            DOS_ERROR( ER_InvalidDrive, EC_MediaError, SA_Abort, EL_Disk );
            AX_reg(context) = 0x00ff;
        }
        else
        {
                dprintf_int(stddeb, "int21: GetDrivePB not fully implemented.\n");

                /* FIXME: I have no idea what a lot of this information should
                 * say or whether it even really matters since we're not allowing
                 * direct block access.  However, some programs seem to depend on
                 * getting at least _something_ back from here.  The 'next' pointer
                 * does worry me, though.  Should we have a complete table of
                 * separate DPBs per drive?  Probably, but I'm lazy. :-)  -CH
                 */
                dpb->drive_num = dpb->unit_num = drive;    /* The same? */
                dpb->sector_size = 512;
                dpb->high_sector = 1;
                dpb->shift = drive < 2 ? 0 : 6; /* 6 for HD, 0 for floppy */
                dpb->reserved = 0;
                dpb->num_FAT = 1;
                dpb->dir_entries = 2;
                dpb->first_data = 2;
                dpb->high_cluster = 64000;
                dpb->sectors_in_FAT = 1;
                dpb->start_dir = 1;
                dpb->driver_head = 0;
                dpb->media_ID = (drive > 1) ? 0xF8 : 0xF0;
                dpb->access_flag = 0;
                dpb->next = 0;
                dpb->free_search = 0;
                dpb->free_clusters = 0xFFFF;    /* unknown */

                AL_reg(context) = 0x00;
                DS_reg(context) = SELECTOROF(dpbsegptr);
                BX_reg(context) = OFFSETOF(dpbsegptr);
        }
}


static void ioctlGetDeviceInfo(struct sigcontext_struct *context)
{
    dprintf_int (stddeb, "int21: ioctl (%d, GetDeviceInfo)\n", BX_reg(context));
    
    DX_reg(context) = 0x0942;
    /* bits 0-5 are current drive
     * bit 6 - file has NOT been written..FIXME: correct?
     * bit 8 - generate int24 if no diskspace on write/ read past end of file
     * bit 11 - media not removable
     */
    RESET_CFLAG(context);
}

static void ioctlGenericBlkDevReq(struct sigcontext_struct *context)
{
	BYTE *dataptr = PTR_SEG_OFF_TO_LIN(DS_reg(context), DX_reg(context));
	int drive = DOS_GET_DRIVE( BL_reg(context) );

	if (!DRIVE_IsValid(drive))
        {
            DOS_ERROR( ER_FileNotFound, EC_NotFound, SA_Abort, EL_Disk );
            AX_reg(context) = DOS_ExtendedError;
            SET_CFLAG(context);
            return;
	}

	if (CH_reg(context) != 0x08)
        {
            INT_BARF( context, 0x21 );
            return;
	}
	switch (CL_reg(context)) {
		case 0x60: /* get device parameters */
			   /* used by w4wgrp's winfile */
			memset(dataptr, 0, 0x26);
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
			CreateBPB(drive, &dataptr[7]);			
			RESET_CFLAG(context);
			return;
		default:
                        INT_BARF( context, 0x21 );
	}
}

static void GetSystemDate(struct sigcontext_struct *context)
{
	struct tm *now;
	time_t ltime;

	ltime = time(NULL);
	now = localtime(&ltime);

	CX_reg(context) = now->tm_year + 1900;
	DX_reg(context) = ((now->tm_mon + 1) << 8) | now->tm_mday;
	AX_reg(context) = now->tm_wday;
}

static void INT21_GetSystemTime(struct sigcontext_struct *context)
{
	struct tm *now;
	struct timeval tv;
	time_t seconds;

	gettimeofday(&tv,NULL);		/* Note use of gettimeofday(), instead of time() */
	seconds = tv.tv_sec;
	now = localtime(&seconds);
	 
	CX_reg(context) = (now->tm_hour<<8) | now->tm_min;
	DX_reg(context) = (now->tm_sec<<8) | tv.tv_usec/10000;
					/* Note hundredths of seconds */
}

static void CreateFile(struct sigcontext_struct *context)
{
    AX_reg(context) = _lcreat( PTR_SEG_OFF_TO_LIN( DS_reg(context),
                                          DX_reg(context) ), CX_reg(context) );
    if (AX_reg(context) == (WORD)HFILE_ERROR)
    {
	AX_reg(context) = DOS_ExtendedError;
	SET_CFLAG(context);
    }
}


void OpenExistingFile(struct sigcontext_struct *context)
{
    AX_reg(context) = _lopen( PTR_SEG_OFF_TO_LIN(DS_reg(context),DX_reg(context)),
                              AL_reg(context) );
    if (AX_reg(context) == (WORD)HFILE_ERROR)
    {
        AX_reg(context) = DOS_ExtendedError;
        SET_CFLAG(context);
    }
#if 0
	int handle;
	int mode;
	int lock;
	
	dprintf_int (stddeb, "int21: open (%s, %d) = %d\n",
		DOS_GetUnixFileName(PTR_SEG_OFF_TO_LIN(DS_reg(context),
                                             DX_reg(context))), mode, handle);

        switch (AX_reg(context) & 0x0070)
	{
	  case 0x00:    /* compatability mode */
	  case 0x40:    /* DENYNONE */
            lock = -1;
	    break;

	  case 0x30:    /* DENYREAD */
	    dprintf_int(stddeb,
	      "OpenExistingFile (%s): DENYREAD changed to DENYALL\n",
	      (char *)PTR_SEG_OFF_TO_LIN(DS_reg(context),DX_reg(context)));
	  case 0x10:    /* DENYALL */  
	    lock = LOCK_EX;
	    break;

	  case 0x20:    /* DENYWRITE */
	    lock = LOCK_SH;
	    break;

	  default:
	    lock = -1;
        }

	if (lock != -1)
        {

	  int result,retries=sharing_retries;
	  {
#if defined(__svr4__) || defined(_SCO_DS)
              printf("Should call flock and needs porting to lockf\n");
              result = 0;
              retries = 0;
#else
	    result = flock(handle, lock | LOCK_NB);
#endif
	    if ( retries && (!result) )
	    {
              int i;
              for(i=0;i<32768*((int)sharing_pause);i++)
		  result++;                          /* stop the optimizer */
              for(i=0;i<32768*((int)sharing_pause);i++)
		  result--;
	    }
          }
	  while( (!result) && (!(retries--)) );

	  if(result)  
	  {
	    errno_to_doserr();
	    AX_reg(context) = ExtendedError;
	    close(handle);
	    SET_CFLAG(context);
	    return;
	  }

        }

	Error (0,0,0);
	AX_reg(context) = handle;
	RESET_CFLAG(context);
#endif
}

static void CloseFile(struct sigcontext_struct *context)
{
    if ((AX_reg(context) = _lclose( BX_reg(context) )) != 0)
    {
        AX_reg(context) = DOS_ExtendedError;
        SET_CFLAG(context);
    }
}

void ExtendedOpenCreateFile(struct sigcontext_struct *context)
{
  BYTE action=DL_reg(context);
  dprintf_int(stddeb, "int21: extended open/create: file= %s \n",
	      DOSFS_GetUnixFileName(PTR_SEG_OFF_TO_LIN(DS_reg(context),SI_reg(context)),FALSE));
  /* Shuffle arguments to call OpenExistingFile */
  AL_reg(context) = BL_reg(context);
  DX_reg(context) = SI_reg(context);
  /* BX,CX and DX should be preserved */
  OpenExistingFile(context);
  if ((EFL_reg(context) & 0x0001)==0) 
    { /* It exists */
      dprintf_int(stddeb, "int21: extended open/create %s exists \n",
		  DOSFS_GetUnixFileName(PTR_SEG_OFF_TO_LIN(DS_reg(context),SI_reg(context)),TRUE));
      /* Now decide what do do */
      if ((action & 0x07)== 0)
	{
	  BX_reg(context) = AX_reg(context);
	  CloseFile(context);
	  AX_reg(context) = 0x0050;/*File exists*/
	  CX_reg(context) = 0;
	  SET_CFLAG(context);
	  dprintf_int(stddeb, "int21: extended open/create: failed because file exixts \n");
	  return;
	}
      if ((action & 0x07)== 2) {
	/* Truncate it, but first check if opend for write */
	if ((BL_reg(context) & 0x0007)== 0) {
	  BX_reg(context) = AX_reg(context);
	  CloseFile(context);
	  dprintf_int(stddeb, "int21: extended open/create: failed, trunc on ro file");
	  AX_reg(context) = 0x000C;/*Access code invalid*/
	  CX_reg(context) = 0;
	  SET_CFLAG(context);
	  return;
	}
	/* Shuffle arguments to call CloseFile */
	dprintf_int(stddeb, "int21: extended open/create: Closing before truncate\n");
	BX_reg(context) = AX_reg(context);
	/* BX and DX should be preserved */
	CloseFile(context);
	if (EFL_reg(context) & 0x0001) {
	  dprintf_int(stddeb, "int21: extended open/create: close before trunc failed");
	  AX_reg(context) = 0x0019;/*Seek Error*/
	  CX_reg(context) = 0;
	  SET_CFLAG(context);
	}
	/* Shuffle arguments to call CreateFile */
	dprintf_int(stddeb, "int21: extended open/create: Truncating\n");
	AL_reg(context) = BL_reg(context);
	/* CX is still the same */
	DX_reg(context) = SI_reg(context);
	CreateFile(context);
	if (EFL_reg(context) & 0x0001) { /*no file open, flags set */
	  dprintf_int(stddeb, "int21: extended open/create: truncfailed");
	  return;
	}
	CX_reg(context) = 3;
	return;
      }
      CX_reg(context) = 1;
      return;
    }
  else /* file does not exist */
    {
      RESET_CFLAG(context); /* was set by OpenExistingFile(context) */
     dprintf_int(stddeb, "int21: extended open/create %s dosen't exists \n",
		  DOSFS_GetUnixFileName(PTR_SEG_OFF_TO_LIN(DS_reg(context),SI_reg(context)),FALSE));
      if ((action & 0xF0)== 0) {
	CX_reg(context) = 0;
	SET_CFLAG(context);
	dprintf_int(stddeb, "int21: extended open/create: failed, file dosen't exist\n");
	return;
      }
      /* Shuffle arguments to call CreateFile */
      dprintf_int(stddeb, "int21: extended open/create: Creating\n");
      AL_reg(context) = BL_reg(context);
      /* CX should still be the same */
      DX_reg(context) = SI_reg(context);
      CreateFile(context);
      if (EFL_reg(context) & 0x0001) { /*no file open, flags set */
	dprintf_int(stddeb, "int21: extended open/create: create failed\n");
	return;
      }
      CX_reg(context) = 2;
      return;
    }
}


static int INT21_RenameFile(struct sigcontext_struct *context)
{
    const char *newname, *oldname;
    char *buffer;

    /* FIXME: should not rename over an existing file */
    dprintf_int(stddeb,"int21: renaming %s to %s\n",
                (char *)PTR_SEG_OFF_TO_LIN(DS_reg(context),DX_reg(context)),
                (char *)PTR_SEG_OFF_TO_LIN(ES_reg(context),DI_reg(context)));
	
    oldname = DOSFS_GetUnixFileName( PTR_SEG_OFF_TO_LIN(DS_reg(context),
                                                DX_reg(context)), TRUE );
    if (!oldname) return 0;
    buffer = xstrdup( oldname );
    newname = DOSFS_GetUnixFileName( PTR_SEG_OFF_TO_LIN(ES_reg(context),
                                                   DI_reg(context)), FALSE );
    if (!newname)
    {
        free( buffer );
        return 0;
    }
    
    if (rename( buffer, newname) == -1)
    {
        FILE_SetDosError();
        free( buffer );
        return 0;
    }
    free( buffer );
    return 1;
}


static void INT21_ChangeDir(struct sigcontext_struct *context)
{
    int drive;
    char *dirname = PTR_SEG_OFF_TO_LIN(DS_reg(context),DX_reg(context));

    dprintf_int(stddeb,"int21: changedir %s\n", dirname);
    if (dirname[0] && (dirname[1] == ':'))
    {
        drive = toupper(dirname[0]) - 'A';
        dirname += 2;
    }
    else drive = DRIVE_GetCurrentDrive();
    if (!DRIVE_Chdir( drive, dirname ))
    {
        AX_reg(context) = DOS_ExtendedError;
        SET_CFLAG(context);
    }
}


static int INT21_FindFirst(struct sigcontext_struct *context)
{
    const char *path, *unixPath, *mask;
    char *p;
    FINDFILE_DTA *dta = (FINDFILE_DTA *)GetCurrentDTA();

    path = (const char *)PTR_SEG_OFF_TO_LIN(DS_reg(context), DX_reg(context));
    dta->unixPath = NULL;
    if (!(unixPath = DOSFS_GetUnixFileName( path, FALSE )))
    {
        AX_reg(context) = DOS_ExtendedError;
        SET_CFLAG(context);
        return 0;
    }
    dta->unixPath = xstrdup( unixPath );
    p = strrchr( dta->unixPath, '/' );
    *p = '\0';
    if (!(mask = DOSFS_ToDosFCBFormat( p + 1 )))
    {
        free( dta->unixPath );
        dta->unixPath = NULL;
        DOS_ERROR( ER_FileNotFound, EC_NotFound, SA_Abort, EL_Disk );
        AX_reg(context) = ER_FileNotFound;
        SET_CFLAG(context);
        return 0;
    }
    memcpy( dta->mask, mask, sizeof(dta->mask) );
    dta->drive = (path[0] && (path[1] == ':')) ? toupper(path[0]) - 'A'
                                               : DRIVE_GetCurrentDrive();
    dta->count = 0;
    dta->search_attr = CL_reg(context);
    return 1;
}


static int INT21_FindNext(struct sigcontext_struct *context)
{
    FINDFILE_DTA *dta = (FINDFILE_DTA *)GetCurrentDTA();
    DOS_DIRENT entry;
    int count;

    if (!dta->unixPath) return 0;
    if (!(count = DOSFS_FindNext( dta->unixPath, dta->mask, dta->drive,
                                  dta->search_attr, dta->count, &entry )))
    {
        free( dta->unixPath );
        dta->unixPath = NULL;
        return 0;
    }
    if ((int)dta->count + count > 0xffff)
    {
        fprintf( stderr, "Too many directory entries in %s\n", dta->unixPath );
        free( dta->unixPath );
        dta->unixPath = NULL;
        return 0;
    }
    dta->count += count;
    dta->fileattr = entry.attr;
    dta->filetime = entry.time;
    dta->filedate = entry.date;
    dta->filesize = entry.size;
    strcpy( dta->filename, DOSFS_ToDosDTAFormat( entry.name ) );
    return 1;
}


static int INT21_CreateTempFile(struct sigcontext_struct *context)
{
    static int counter = 0;
    char *name = PTR_SEG_OFF_TO_LIN( DS_reg(context), DX_reg(context) );
    char *p = name + strlen(name);

    for (;;)
    {
        sprintf( p, "wine%04x.%03d", (int)getpid(), counter );
        counter = (counter + 1) % 1000;

        if ((AX_reg(context) = _lcreat_uniq( name, 0 )) != (WORD)HFILE_ERROR)
        {
            dprintf_int( stddeb, "INT21_CreateTempFile: created %s\n", name );
            return 1;
        }
        if (DOS_ExtendedError != ER_FileExists) return 0;
    }
}


static int INT21_GetCurrentDirectory(struct sigcontext_struct *context)
{
    int drive = DOS_GET_DRIVE( DL_reg(context) );
    char *ptr = (char *)PTR_SEG_OFF_TO_LIN( DS_reg(context), SI_reg(context) );

    if (!DRIVE_IsValid(drive))
    {
        DOS_ERROR( ER_InvalidDrive, EC_NotFound, SA_Abort, EL_Disk );
        return 0;
    }

    lstrcpyn32A( ptr, DRIVE_GetDosCwd(drive), 64 );
    if (!ptr[0]) strcpy( ptr, "\\" );
    return 1;
}


static int INT21_GetDiskSerialNumber(struct sigcontext_struct *context)
{
    BYTE *dataptr = PTR_SEG_OFF_TO_LIN(DS_reg(context), DX_reg(context));
    int drive = DOS_GET_DRIVE( BL_reg(context) );
	
    if (!DRIVE_IsValid(drive))
    {
        DOS_ERROR( ER_InvalidDrive, EC_NotFound, SA_Abort, EL_Disk );
        return 0;
    }
    
    *(WORD *)dataptr = 0;
    *(DWORD *)(dataptr + 2) = DRIVE_GetSerialNumber( drive );
    memcpy( dataptr + 6, DRIVE_GetLabel( drive ), 11 );
    strncpy(dataptr + 0x11, "FAT16   ", 8);
    return 1;
}


static int INT21_SetDiskSerialNumber(struct sigcontext_struct *context)
{
    BYTE *dataptr = PTR_SEG_OFF_TO_LIN(DS_reg(context), DX_reg(context));
    int drive = DOS_GET_DRIVE( BL_reg(context) );

    if (!DRIVE_IsValid(drive))
    {
        DOS_ERROR( ER_InvalidDrive, EC_NotFound, SA_Abort, EL_Disk );
        return 0;
    }

    DRIVE_SetSerialNumber( drive, *(DWORD *)(dataptr + 2) );
    return 1;
}


/* microsoft's programmers should be shot for using CP/M style int21
   calls in Windows for Workgroup's winfile.exe */

static int INT21_FindFirstFCB( struct sigcontext_struct *context )
{
    BYTE *fcb = (BYTE *)PTR_SEG_OFF_TO_LIN(DS_reg(context), DX_reg(context));
    FINDFILE_FCB *pFCB;
    BYTE attr;
    char buffer[] = "A:.";
    const char *unixPath;

    if (*fcb == 0xff)
    {
        attr = fcb[6];
        pFCB = (FINDFILE_FCB *)(fcb + 7);
    }
    else
    {
        attr = 0;
        pFCB = (FINDFILE_FCB *)fcb;
    }

    buffer[0] += DOS_GET_DRIVE( pFCB->drive );
    pFCB->unixPath = NULL;
    if (!(unixPath = DOSFS_GetUnixFileName( buffer, TRUE ))) return 0;
    pFCB->unixPath = xstrdup( unixPath );
    pFCB->count = 0;
    return 1;
}


static int INT21_FindNextFCB( struct sigcontext_struct *context )
{
    BYTE *fcb = (BYTE *)PTR_SEG_OFF_TO_LIN(DS_reg(context), DX_reg(context));
    FINDFILE_FCB *pFCB;
    DOS_DIRENTRY_LAYOUT *pResult = (DOS_DIRENTRY_LAYOUT *)GetCurrentDTA();
    DOS_DIRENT entry;
    BYTE attr;
    int count;

    if (*fcb == 0xff)
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
    if (!(count = DOSFS_FindNext( pFCB->unixPath, pFCB->filename,
                                  DOS_GET_DRIVE( pFCB->drive ), attr,
                                  pFCB->count, &entry )))
    {
        free( pFCB->unixPath );
        pFCB->unixPath = NULL;
        return 0;
    }
    pFCB->count += count;

    memcpy( pResult->filename, entry.name, sizeof(pResult->filename) );
    pResult->fileattr = entry.attr;
    memset( pResult->reserved, 0, sizeof(pResult->reserved) );
    pResult->filetime = entry.time;
    pResult->filedate = entry.date;
    pResult->cluster  = 0;  /* what else? */
    pResult->filesize = entry.size;
    return 1;
}


static void DeleteFileFCB(struct sigcontext_struct *context)
{
    fprintf( stderr, "DeleteFileFCB: not implemented yet\n" );
#if 0
	BYTE *fcb = PTR_SEG_OFF_TO_LIN(DS_reg(context), DX_reg(context));
	struct dosdirent *dp;
	char temp[256], *ptr;
        int drive = DOS_GET_DRIVE( *fcb );

	DumpFCB( fcb );

        temp[0] = '\\';
	strcpy(temp+1, DRIVE_GetDosCwd(drive));
	strcat(temp, "\\");
	strncat(temp, fcb + 1, 8);
	ChopOffWhiteSpace(temp);
	strncat(temp, fcb + 9, 3);
	ChopOffWhiteSpace(temp);

	if ((dp = DOS_opendir(temp)) == NULL) {
		Error(InvalidDrive, EC_MediaError , EL_Disk);
		AX_reg(context) = 0xff;
		return;
	}

        temp[0] = '\\';
	strcpy(temp+1, DRIVE_GetDosCwd(drive) );
	strcat(temp, "\\");
	
	ptr = temp + strlen(temp);
	
	while (DOS_readdir(dp) != NULL)
	{
		strcpy(ptr, dp->filename);
		dprintf_int(stddeb, "int21: delete file %s\n", temp);
		/* unlink(DOS_GetUnixFileName(temp)); */
	}
	DOS_closedir(dp);
	AX_reg(context) = 0;
#endif
}

static void RenameFileFCB(struct sigcontext_struct *context)
{
    fprintf( stderr, "RenameFileFCB: not implemented yet\n" );
#if 0
	BYTE *fcb = PTR_SEG_OFF_TO_LIN(DS_reg(context), DX_reg(context));
	struct dosdirent *dp;
	char temp[256], oldname[256], newname[256], *oldnameptr, *newnameptr;
        int drive = DOS_GET_DRIVE( *fcb );

	DumpFCB( fcb );

        temp[0] = '\\';
	strcpy(temp+1, DRIVE_GetDosCwd(drive) );
	strcat(temp, "\\");
	strncat(temp, fcb + 1, 8);
	ChopOffWhiteSpace(temp);
	strncat(temp, fcb + 9, 3);
	ChopOffWhiteSpace(temp);

	if ((dp = DOS_opendir(temp)) == NULL) {
		Error(InvalidDrive, EC_MediaError , EL_Disk);
		AX_reg(context) = 0xff;
		return;
	}

        oldname[0] = '\\';
	strcpy(oldname+1, DRIVE_GetDosCwd(drive) );
	strcat(oldname, "\\");
        strcpy( newname, oldname );
	oldnameptr = oldname + strlen(oldname);
	newnameptr = newname + strlen(newname);
	
	while (DOS_readdir(dp) != NULL)
	{
		strcpy(oldnameptr, dp->filename);
		strcpy(newnameptr, fcb + 1);
		dprintf_int(stddeb, "int21: renamefile %s -> %s\n",
			oldname, newname);
	}
	DOS_closedir(dp);
	AX_reg(context) = 0;
#endif
}



static void fLock (struct sigcontext_struct * context)
{
#if 0
    struct flock f;
    int result,retries=sharing_retries;

    f.l_start = MAKELONG(DX_reg(context),CX_reg(context));
    f.l_len   = MAKELONG(DI_reg(context),SI_reg(context));
    f.l_whence = 0;
    f.l_pid = 0;

    switch ( AX_reg(context) & 0xff )
    {
        case 0x00: /* LOCK */
	  f.l_type = F_WRLCK;
          break;

	case 0x01: /* UNLOCK */
          f.l_type = F_UNLCK;
	  break;

	default:
	  AX_reg(context) = 0x0001;
	  SET_CFLAG(context);
	  return;
     }
 
     {
          result = fcntl(BX_reg(context),F_SETLK,&f); 
          if ( retries && (!result) )
          {
              int i;
              for(i=0;i<32768*((int)sharing_pause);i++)
                  result++;                          /* stop the optimizer */
              for(i=0;i<32768*((int)sharing_pause);i++)
                  result--;
          }
      }
      while( (!result) && (!(retries--)) );

      if(result)  
      {
          FILE_SetDosError();
          AX_reg(context) = DOS_ExtendedError;
          SET_CFLAG(context);
          return;
      }
#endif
} 


static int INT21_GetFileAttribute (struct sigcontext_struct * context)
{
    const char *unixName;

    unixName = DOSFS_GetUnixFileName( PTR_SEG_OFF_TO_LIN(DS_reg(context),DX_reg(context)), TRUE );
    if (!unixName) return 0;
    if (!FILE_Stat( unixName, &CL_reg(context), NULL, NULL, NULL )) return 0;
    CH_reg(context) = 0;
    dprintf_int( stddeb, "INT21_GetFileAttributes(%s) = 0x%x\n",
                 unixName, CX_reg(context) );
    return 1;
}


extern void LOCAL_PrintHeap (WORD ds);

/***********************************************************************
 *           DOS3Call  (KERNEL.102)
 */
void DOS3Call( struct sigcontext_struct context )
{
    dprintf_int( stddeb, "int21: AX=%04x BX=%04x CX=%04x DX=%04x "
                 "SI=%04x DI=%04x DS=%04x ES=%04x EFL=%08lx\n",
                 AX_reg(&context), BX_reg(&context), CX_reg(&context),
                 DX_reg(&context), SI_reg(&context), DI_reg(&context),
                 DS_reg(&context), ES_reg(&context), EFL_reg(&context));

    if (AH_reg(&context) == 0x59)  /* Get extended error info */
    {
        AX_reg(&context) = DOS_ExtendedError;
        BH_reg(&context) = DOS_ErrorClass;
        BL_reg(&context) = DOS_ErrorAction;
        CH_reg(&context) = DOS_ErrorLocus;
        return;
    }

    DOS_ERROR( 0, 0, 0, 0 );
    RESET_CFLAG(&context);  /* Not sure if this is a good idea */

    switch(AH_reg(&context)) 
    {
    case 0x00: /* TERMINATE PROGRAM */
        TASK_KillCurrentTask( 0 );
        break;

    case 0x01: /* READ CHARACTER FROM STANDARD INPUT, WITH ECHO */
    case 0x02: /* WRITE CHARACTER TO STANDARD OUTPUT */
    case 0x03: /* READ CHARACTER FROM STDAUX  */
    case 0x04: /* WRITE CHARACTER TO STDAUX */
    case 0x05: /* WRITE CHARACTER TO PRINTER */
    case 0x06: /* DIRECT CONSOLE IN/OUTPUT */
    case 0x07: /* DIRECT CHARACTER INPUT, WITHOUT ECHO */
    case 0x08: /* CHARACTER INPUT WITHOUT ECHO */
    case 0x09: /* WRITE STRING TO STANDARD OUTPUT */
    case 0x0a: /* BUFFERED INPUT */
    case 0x0b: /* GET STDIN STATUS */
    case 0x0c: /* FLUSH BUFFER AND READ STANDARD INPUT */
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
    case 0x29: /* PARSE FILENAME INTO FCB */
    case 0x2e: /* SET VERIFY FLAG */
    case 0x37: /* "SWITCHAR" - GET SWITCH CHARACTER
                  "SWITCHAR" - SET SWITCH CHARACTER
                  "AVAILDEV" - SPECIFY \DEV\ PREFIX USE */
    case 0x54: /* GET VERIFY FLAG */
        INT_BARF( &context, 0x21 );
        break;

    case 0x18: /* NULL FUNCTIONS FOR CP/M COMPATIBILITY */
    case 0x1d:
    case 0x1e:
    case 0x20:
    case 0x6b: /* NULL FUNCTION */
        AL_reg(&context) = 0;
        break;
	
    case 0x5c: /* "FLOCK" - RECORD LOCKING */
        fLock(&context);
        break;

    case 0x0d: /* DISK BUFFER FLUSH */
        RESET_CFLAG(&context); /* dos 6+ only */
        break;

    case 0x0e: /* SELECT DEFAULT DRIVE */
        DRIVE_SetCurrentDrive( DL_reg(&context) );
        AL_reg(&context) = MAX_DOS_DRIVES;
        break;

    case 0x11: /* FIND FIRST MATCHING FILE USING FCB */
        if (!INT21_FindFirstFCB(&context))
        {
            AL_reg(&context) = 0xff;
            break;
        }
        /* else fall through */

    case 0x12: /* FIND NEXT MATCHING FILE USING FCB */
        AL_reg(&context) = INT21_FindNextFCB(&context) ? 0x00 : 0xff;
        break;

    case 0x13: /* DELETE FILE USING FCB */
        DeleteFileFCB(&context);
        break;
            
    case 0x17: /* RENAME FILE USING FCB */
        RenameFileFCB(&context);
        break;

    case 0x19: /* GET CURRENT DEFAULT DRIVE */
        AL_reg(&context) = DRIVE_GetCurrentDrive();
        break;

    case 0x1a: /* SET DISK TRANSFER AREA ADDRESS */
        {
            TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
            pTask->dta = PTR_SEG_OFF_TO_SEGPTR(DS_reg(&context),DX_reg(&context));
            dprintf_int(stddeb, "int21: Set DTA: %08lx\n", pTask->dta);
        }
        break;

    case 0x1b: /* GET ALLOCATION INFORMATION FOR DEFAULT DRIVE */
        DL_reg(&context) = 0;
        if (!INT21_GetDriveAllocInfo(&context)) AX_reg(&context) = 0xffff;
        break;
	
    case 0x1c: /* GET ALLOCATION INFORMATION FOR SPECIFIC DRIVE */
        if (!INT21_GetDriveAllocInfo(&context)) AX_reg(&context) = 0xffff;
        break;

    case 0x1f: /* GET DRIVE PARAMETER BLOCK FOR DEFAULT DRIVE */
        GetDrivePB(&context, DRIVE_GetCurrentDrive());
        break;
		
    case 0x25: /* SET INTERRUPT VECTOR */
        INT_SetHandler( AL_reg(&context),
                   PTR_SEG_OFF_TO_SEGPTR( DS_reg(&context), DX_reg(&context)));
        break;

    case 0x2a: /* GET SYSTEM DATE */
        GetSystemDate(&context);
        break;

    case 0x2b: /* SET SYSTEM DATE */
        fprintf( stdnimp, "SetSystemDate(%02d/%02d/%04d): not allowed\n",
                 DL_reg(&context), DH_reg(&context), CX_reg(&context) );
        AL_reg(&context) = 0;  /* Let's pretend we succeeded */
        break;

    case 0x2c: /* GET SYSTEM TIME */
        INT21_GetSystemTime(&context);
        break;

    case 0x2d: /* SET SYSTEM TIME */
        fprintf( stdnimp, "SetSystemTime(%02d:%02d:%02d.%02d): not allowed\n",
                 CH_reg(&context), CL_reg(&context),
                 DH_reg(&context), DL_reg(&context) );
        AL_reg(&context) = 0;  /* Let's pretend we succeeded */
        break;

    case 0x2f: /* GET DISK TRANSFER AREA ADDRESS */
        {
            TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
            ES_reg(&context) = SELECTOROF( pTask->dta );
            BX_reg(&context) = OFFSETOF( pTask->dta );
        }
        break;
            
    case 0x30: /* GET DOS VERSION */
        AX_reg(&context) = DOSVERSION;
        BX_reg(&context) = 0x0012;     /* 0x123456 is Wine's serial # */
        CX_reg(&context) = 0x3456;
        break;

    case 0x31: /* TERMINATE AND STAY RESIDENT */
        INT_BARF( &context, 0x21 );
        break;

    case 0x32: /* GET DOS DRIVE PARAMETER BLOCK FOR SPECIFIC DRIVE */
        GetDrivePB(&context, DOS_GET_DRIVE( DL_reg(&context) ) );
        break;

    case 0x33: /* MULTIPLEXED */
        switch (AL_reg(&context))
        {
	      case 0x00: /* GET CURRENT EXTENDED BREAK STATE */
                DL_reg(&context) = 0;
		break;

	      case 0x01: /* SET EXTENDED BREAK STATE */
		break;		
		
	      case 0x02: /* GET AND SET EXTENDED CONTROL-BREAK CHECKING STATE*/
		DL_reg(&context) = 0;
		break;

	      case 0x05: /* GET BOOT DRIVE */
		DL_reg(&context) = 2;
		/* c: is Wine's bootdrive */
		break;
				
	      case 0x06: /* GET TRUE VERSION NUMBER */
		BX_reg(&context) = DOSVERSION;
		DX_reg(&context) = 0x00;
		break;

	      default:
                INT_BARF( &context, 0x21 );
		break;			
        }
        break;	
	    
    case 0x34: /* GET ADDRESS OF INDOS FLAG */
        ES_reg(&context) = DosHeapHandle;
        BX_reg(&context) = (int)&heap->InDosFlag - (int)heap;
        break;

    case 0x35: /* GET INTERRUPT VECTOR */
        {
            SEGPTR addr = INT_GetHandler( AL_reg(&context) );
            ES_reg(&context) = SELECTOROF(addr);
            BX_reg(&context) = OFFSETOF(addr);
        }
        break;

    case 0x36: /* GET FREE DISK SPACE */
        if (!INT21_GetFreeDiskSpace(&context)) AX_reg(&context) = 0xffff;
        break;

    case 0x38: /* GET COUNTRY-SPECIFIC INFORMATION */
        AX_reg(&context) = 0x02; /* no country support available */
        SET_CFLAG(&context);
        break;

    case 0x39: /* "MKDIR" - CREATE SUBDIRECTORY */
        if (!CreateDirectory32A( PTR_SEG_OFF_TO_LIN( DS_reg(&context),
                                                     DX_reg(&context) ), NULL))
        {
            AX_reg(&context) = DOS_ExtendedError;
            SET_CFLAG(&context);
        }
        break;
	
    case 0x3a: /* "RMDIR" - REMOVE SUBDIRECTORY */
        if (!FILE_RemoveDir( PTR_SEG_OFF_TO_LIN( DS_reg(&context),
                                                 DX_reg(&context) )))
        {
            AX_reg(&context) = DOS_ExtendedError;
            SET_CFLAG(&context);
        }
        break;

    case 0x3b: /* "CHDIR" - SET CURRENT DIRECTORY */
        INT21_ChangeDir(&context);
        break;
	
    case 0x3c: /* "CREAT" - CREATE OR TRUNCATE FILE */
        AX_reg(&context) = _lcreat( PTR_SEG_OFF_TO_LIN( DS_reg(&context),
                                    DX_reg(&context) ), CX_reg(&context) );
        if (AX_reg(&context) == (WORD)HFILE_ERROR)
        {
            AX_reg(&context) = DOS_ExtendedError;
            SET_CFLAG(&context);
        }
        break;

    case 0x3d: /* "OPEN" - OPEN EXISTING FILE */
        OpenExistingFile(&context);
        break;

    case 0x3e: /* "CLOSE" - CLOSE FILE */
        if ((AX_reg(&context) = _lclose( BX_reg(&context) )) != 0)
        {
            AX_reg(&context) = DOS_ExtendedError;
            SET_CFLAG(&context);
        }
        break;

    case 0x3f: /* "READ" - READ FROM FILE OR DEVICE */
        {
            LONG result = _hread( BX_reg(&context),
                                  PTR_SEG_OFF_TO_SEGPTR( DS_reg(&context),
                                                         DX_reg(&context) ),
                                  CX_reg(&context) );
            if (result == -1)
            {
                AX_reg(&context) = DOS_ExtendedError;
                SET_CFLAG(&context);
            }
            else AX_reg(&context) = (WORD)result;
        }
        break;

    case 0x40: /* "WRITE" - WRITE TO FILE OR DEVICE */
        {
            LONG result = _hwrite( BX_reg(&context),
                                   PTR_SEG_OFF_TO_LIN( DS_reg(&context),
                                                       DX_reg(&context) ),
                                   CX_reg(&context) );
            if (result == -1)
            {
                AX_reg(&context) = DOS_ExtendedError;
                SET_CFLAG(&context);
            }
            else AX_reg(&context) = (WORD)result;
        }
        break;

    case 0x41: /* "UNLINK" - DELETE FILE */
        if (!FILE_Unlink( PTR_SEG_OFF_TO_LIN( DS_reg(&context),
                                              DX_reg(&context) )))
        {
            AX_reg(&context) = DOS_ExtendedError;
            SET_CFLAG(&context);
        }		
        break;

    case 0x42: /* "LSEEK" - SET CURRENT FILE POSITION */
        {
            LONG status = _llseek( BX_reg(&context),
                                   MAKELONG(DX_reg(&context),CX_reg(&context)),
                                   AL_reg(&context) );
            if (status == HFILE_ERROR)
            {
                AX_reg(&context) = DOS_ExtendedError;
                SET_CFLAG(&context);
                break;
            }
            AX_reg(&context) = LOWORD(status);
            DX_reg(&context) = HIWORD(status);
        }
        break;

    case 0x43: /* FILE ATTRIBUTES */
        switch (AL_reg(&context))
        {
        case 0x00:
            if (!INT21_GetFileAttribute(&context))
            {
                AX_reg(&context) = DOS_ExtendedError;
                SET_CFLAG(&context);
            }
            break;
        case 0x01:
            RESET_CFLAG(&context);
            break;
        }
        break;
	
    case 0x44: /* IOCTL */
        switch (AL_reg(&context))
        {
        case 0x00:
            ioctlGetDeviceInfo(&context);
            break;

        case 0x01:
            break;

        case 0x08:   /* Check if drive is removable. */
            switch(GetDriveType( DOS_GET_DRIVE( BL_reg(&context) )))
            {
            case DRIVE_CANNOTDETERMINE:
                DOS_ERROR( ER_InvalidDrive, EC_NotFound, SA_Abort, EL_Disk );
                AX_reg(&context) = ER_InvalidDrive;
                SET_CFLAG(&context);
                break;
            case DRIVE_REMOVABLE:
                AX_reg(&context) = 0;      /* removable */
                break;
            default:
                AX_reg(&context) = 1;   /* not removable */
                break;
            }
            break;

        case 0x09:   /* CHECK IF BLOCK DEVICE REMOTE */
            switch(GetDriveType( DOS_GET_DRIVE( BL_reg(&context) )))
            {
            case DRIVE_CANNOTDETERMINE:
                DOS_ERROR( ER_InvalidDrive, EC_NotFound, SA_Abort, EL_Disk );
                AX_reg(&context) = ER_InvalidDrive;
                SET_CFLAG(&context);
                break;
            case DRIVE_REMOTE:
                DX_reg(&context) = (1<<9) | (1<<12);  /* remote */
                break;
            default:
                DX_reg(&context) = 0;  /* FIXME: use driver attr here */
                break;
            }
            break;

        case 0x0a: /* check if handle (BX) is remote */
            /* returns DX, bit 15 set if remote, bit 14 set if date/time
             * not set on close
             */
            DX_reg(&context) = 0;
            break;

        case 0x0b:   /* SET SHARING RETRY COUNT */
            if (!CX_reg(&context))
            { 
                AX_reg(&context) = 1;
                SET_CFLAG(&context);
                break;
            }
            sharing_pause = CX_reg(&context);
            if (!DX_reg(&context))
                sharing_retries = DX_reg(&context);
            RESET_CFLAG(&context);
            break;

        case 0x0d:
            ioctlGenericBlkDevReq(&context);
            break;

        case 0x0F:   /* Set logical drive mapping */
            /* FIXME: Not implemented at the moment, always returns error
             */
            INT_BARF( &context, 0x21 );
            AX_reg(&context) = 0x0001; /* invalid function */
            SET_CFLAG(&context);
            break;
                
        default:
            INT_BARF( &context, 0x21 );
            break;
        }
        break;

    case 0x45: /* "DUP" - DUPLICATE FILE HANDLE */
        if ((AX_reg(&context) = FILE_Dup(BX_reg(&context))) == (WORD)HFILE_ERROR)
        {
            AX_reg(&context) = DOS_ExtendedError;
            SET_CFLAG(&context);
        }
        break;

    case 0x46: /* "DUP2", "FORCEDUP" - FORCE DUPLICATE FILE HANDLE */
        if (FILE_Dup2( BX_reg(&context), CX_reg(&context) ) == HFILE_ERROR)
        {
            AX_reg(&context) = DOS_ExtendedError;
            SET_CFLAG(&context);
        }
        break;

    case 0x47: /* "CWD" - GET CURRENT DIRECTORY */
        if (!INT21_GetCurrentDirectory(&context))
        {
            AX_reg(&context) = DOS_ExtendedError;
            SET_CFLAG(&context);
        }
        else AX_reg(&context) = 0x0100; 
        /* intlist: many Microsoft products for Windows rely on this */
        break;
	
    case 0x48: /* ALLOCATE MEMORY */
    case 0x49: /* FREE MEMORY */
    case 0x4a: /* RESIZE MEMORY BLOCK */
        INT_BARF( &context, 0x21 );
        break;
	
    case 0x4b: /* "EXEC" - LOAD AND/OR EXECUTE PROGRAM */
        AX_reg(&context) = WinExec( PTR_SEG_OFF_TO_LIN( DS_reg(&context),
                                                        DX_reg(&context) ),
                                    SW_NORMAL );
        if (AX_reg(&context) < 32) SET_CFLAG(&context);
        break;		
	
    case 0x4c: /* "EXIT" - TERMINATE WITH RETURN CODE */
        TASK_KillCurrentTask( AL_reg(&context) );
        break;

    case 0x4d: /* GET RETURN CODE */
        AX_reg(&context) = 0; /* normal exit */
        break;

    case 0x4e: /* "FINDFIRST" - FIND FIRST MATCHING FILE */
        if (!INT21_FindFirst(&context)) break;
        /* fall through */

    case 0x4f: /* "FINDNEXT" - FIND NEXT MATCHING FILE */
        if (!INT21_FindNext(&context))
        {
            DOS_ERROR( ER_NoMoreFiles, EC_MediaError, SA_Abort, EL_Disk );
            AX_reg(&context) = ER_NoMoreFiles;
            SET_CFLAG(&context);
        }
        break;

    case 0x51: /* GET PSP ADDRESS */
    case 0x62: /* GET PSP ADDRESS */
        /* FIXME: should we return the original DOS PSP upon */
        /*        Windows startup ? */
        BX_reg(&context) = GetCurrentPDB();
        break;

    case 0x52: /* "SYSVARS" - GET LIST OF LISTS */
        ES_reg(&context) = 0x0;
        BX_reg(&context) = 0x0;
        INT_BARF( &context, 0x21 );
        break;

    case 0x56: /* "RENAME" - RENAME FILE */
        if (!INT21_RenameFile(&context))
        {
            AX_reg(&context) = DOS_ExtendedError;
            SET_CFLAG(&context);
        }
        break;

    case 0x57: /* FILE DATE AND TIME */
        switch (AL_reg(&context))
        {
        case 0x00:  /* Get */
            if (!FILE_GetDateTime( BX_reg(&context), &DX_reg(&context),
                                   &CX_reg(&context), TRUE ))
            {
                AX_reg(&context) = DOS_ExtendedError;
                SET_CFLAG(&context);
            }
            break;

        case 0x01:  /* Set */
            if (!FILE_SetDateTime( BX_reg(&context), DX_reg(&context),
                                   CX_reg(&context) ))
            {
                AX_reg(&context) = DOS_ExtendedError;
                SET_CFLAG(&context);
            }
            break;
        }
        break;

    case 0x58: /* GET OR SET MEMORY/UMB ALLOCATION STRATEGY */
        switch (AL_reg(&context))
        {
        case 0x00:
            AX_reg(&context) = 1;
            break;
        case 0x02:
            AX_reg(&context) = 0;
            break;
        case 0x01:
        case 0x03:
            break;
        }
        RESET_CFLAG(&context);
        break;

    case 0x5a: /* CREATE TEMPORARY FILE */
        if (!INT21_CreateTempFile(&context))
        {
            AX_reg(&context) = DOS_ExtendedError;
            SET_CFLAG(&context);
        }
        break;

    case 0x5b: /* CREATE NEW FILE */
        if ((AX_reg(&context) = _lcreat_uniq( PTR_SEG_OFF_TO_LIN(DS_reg(&context),DX_reg(&context)), 0 )) == (WORD)HFILE_ERROR)
        {
            AX_reg(&context) = DOS_ExtendedError;
            SET_CFLAG(&context);
        }
        break;

    case 0x5d: /* NETWORK */
    case 0x5e:
        /* network software not installed */
        DOS_ERROR( ER_NoNetwork, EC_NotFound, SA_Abort, EL_Network );
        AX_reg(&context) = DOS_ExtendedError;
        SET_CFLAG(&context);
        break;

    case 0x5f: /* NETWORK */
        switch (AL_reg(&context))
        {
        case 0x07: /* ENABLE DRIVE */
            if (!DRIVE_Enable( DL_reg(&context) ))
            {
                DOS_ERROR( ER_InvalidDrive, EC_MediaError, SA_Abort, EL_Disk );
                AX_reg(&context) = DOS_ExtendedError;
                SET_CFLAG(&context);
            }
            break;

        case 0x08: /* DISABLE DRIVE */
            if (!DRIVE_Disable( DL_reg(&context) ))
            {
                DOS_ERROR( ER_InvalidDrive, EC_MediaError, SA_Abort, EL_Disk );
                AX_reg(&context) = DOS_ExtendedError;
                SET_CFLAG(&context);
            } 
            break;

        default:
            /* network software not installed */
            DOS_ERROR( ER_NoNetwork, EC_NotFound, SA_Abort, EL_Network );
            AX_reg(&context) = DOS_ExtendedError;
            SET_CFLAG(&context);
            break;
        }
        break;

    case 0x60: /* "TRUENAME" - CANONICALIZE FILENAME OR PATH */
        {
            const char *truename = DOSFS_GetDosTrueName( PTR_SEG_OFF_TO_LIN(DS_reg(&context),SI_reg(&context) ), FALSE );
            if (!truename)
            {
                AX_reg(&context) = DOS_ExtendedError;
                SET_CFLAG(&context);
            }
            else
            {
                lstrcpyn32A( PTR_SEG_OFF_TO_LIN( ES_reg(&context),
                                                 DI_reg(&context) ),
                             truename, 128 );
                AX_reg(&context) = 0;
            }
        }
        break;

    case 0x61: /* UNUSED */
    case 0x63: /* UNUSED */
    case 0x64: /* OS/2 DOS BOX */
    case 0x65: /* GET EXTENDED COUNTRY INFORMATION */
        INT_BARF( &context, 0x21 );
        SET_CFLAG(&context);
        break;
	
    case 0x66: /* GLOBAL CODE PAGE TABLE */
        switch (AL_reg(&context))
        {
        case 0x01:
            DX_reg(&context) = BX_reg(&context) = CodePage;
            RESET_CFLAG(&context);
            break;			
        case 0x02: 
            CodePage = BX_reg(&context);
            RESET_CFLAG(&context);
            break;
        }
        break;

    case 0x67: /* SET HANDLE COUNT */
        SetHandleCount( BX_reg(&context) );
        if (DOS_ExtendedError)
        {
            AX_reg(&context) = DOS_ExtendedError;
            SET_CFLAG(&context);
        }
        break;

    case 0x68: /* "FFLUSH" - COMMIT FILE */
    case 0x6a: /* COMMIT FILE */
        if (!FlushFileBuffers( BX_reg(&context) ))
        {
            AX_reg(&context) = DOS_ExtendedError;
            SET_CFLAG(&context);
        }
        break;		
	
    case 0x69: /* DISK SERIAL NUMBER */
        switch (AL_reg(&context))
        {
        case 0x00:
            if (!INT21_GetDiskSerialNumber(&context))
            {
                AX_reg(&context) = DOS_ExtendedError;
                SET_CFLAG(&context);
            }
            else AX_reg(&context) = 0;
            break;
        case 0x01:
            if (!INT21_SetDiskSerialNumber(&context))
            {
                AX_reg(&context) = DOS_ExtendedError;
                SET_CFLAG(&context);
            }
            else AX_reg(&context) = 1;
            break;
        }
        break;
    
    case 0x6C: /* Extended Open/Create*/
        ExtendedOpenCreateFile(&context);
        break;
	
    case 0x70: /* MS-DOS 7 (Windows95) - ??? (country-specific?)*/
    case 0x71: /* MS-DOS 7 (Chicago) - LONG FILENAME FUNCTIONS */
    case 0x72: /* MS-DOS 7 (Windows95) - ??? */
    case 0x73: /* MS-DOS 7 (Windows95) - DRIVE LOCKING ??? */
        dprintf_int(stddeb,"int21: windows95 function AX %04x\n",
                    AX_reg(&context));
        dprintf_int(stddeb, "        returning unimplemented\n");
        SET_CFLAG(&context);
        AL_reg(&context) = 0;
        break;

    case 0xdc: /* CONNECTION SERVICES - GET CONNECTION NUMBER */
    case 0xea: /* NOVELL NETWARE - RETURN SHELL VERSION */
        break;

    default:
        INT_BARF( &context, 0x21 );
        break;
    }
    dprintf_int( stddeb, "ret21: AX=%04x BX=%04x CX=%04x DX=%04x "
                 "SI=%04x DI=%04x DS=%04x ES=%04x EFL=%08lx\n",
                 AX_reg(&context), BX_reg(&context), CX_reg(&context),
                 DX_reg(&context), SI_reg(&context), DI_reg(&context),
                 DS_reg(&context), ES_reg(&context), EFL_reg(&context));
}


BOOL INT21_Init(void)
{
    if (!(DosHeapHandle = GlobalAlloc16(GMEM_FIXED,sizeof(struct DosHeap))))
    {
        fprintf( stderr, "INT21_Init: Out of memory\n");
        return FALSE;
    }
    heap = (struct DosHeap *) GlobalLock16(DosHeapHandle);

    dpb = &heap->dpb;
    dpbsegptr = PTR_SEG_OFF_TO_SEGPTR(DosHeapHandle,(int)&heap->dpb-(int)heap);
    heap->InDosFlag = 0;
    strcpy(heap->biosdate, "01/01/80");
    return TRUE;
}
