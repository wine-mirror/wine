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
#include "windows.h"
#include "msdos.h"
#include "registers.h"
#include "ldt.h"
#include "task.h"
#include "options.h"
#include "miscemu.h"
#include "stddebug.h"
/* #define DEBUG_INT */
#include "debug.h"

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

WORD ExtendedError, CodePage = 437;
BYTE ErrorClass, Action, ErrorLocus;
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

static int Error(int e, int class, int el)
{
	ErrorClass = class;
	Action = SA_Ask4Retry;
	ErrorLocus = el;
	ExtendedError = e;

	return e;
}

void errno_to_doserr(void)
{
	switch (errno) {
		case EAGAIN:
			Error (ShareViolation, EC_Temporary, EL_Unknown);
			break;
		case EBADF:
			Error (InvalidHandle, EC_AppError, EL_Unknown);
			break;
		case ENOSPC:
			Error (DiskFull, EC_MediaError, EL_Disk);
			break;				
		case EACCES:
		case EPERM:
		case EROFS:
			Error (WriteProtected, EC_AccessDenied, EL_Unknown);
			break;
		case EBUSY:
			Error (LockViolation, EC_AccessDenied, EL_Unknown);
			break;		
		case ENOENT:
			Error (FileNotFound, EC_NotFound, EL_Unknown);
			break;				
		case EISDIR:
			Error (CanNotMakeDir, EC_AccessDenied, EL_Unknown);
			break;
		case ENFILE:
		case EMFILE:
			Error (NoMoreFiles, EC_MediaError, EL_Unknown);
			break;
		case EEXIST:
			Error (FileExists, EC_Exists, EL_Disk);
			break;				
		default:
			dprintf_int(stddeb, "int21: unknown errno %d!\n", errno);
			Error (GeneralFailure, EC_SystemFailure, EL_Unknown);
			break;
	}
}

BYTE *GetCurrentDTA(void)
{
    TDB *pTask = (TDB *)GlobalLock( GetCurrentTask() );
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

static void GetFreeDiskSpace(struct sigcontext_struct *context)
{
	int drive;
	long size,available;

	if (DL == 0)
		drive = DOS_GetDefaultDrive();
	else
		drive = DL - 1;
	
	if (!DOS_ValidDrive(drive)) {
		Error(InvalidDrive, EC_MediaError , EL_Disk);
		AX = 0xffff;
		return;
	}
	
	if (!DOS_GetFreeSpace(drive, &size, &available)) {
		Error(GeneralFailure, EC_MediaError , EL_Disk);
		AX = 0xffff;
		return;
	}

	AX = 4;	
	CX = 512;

	BX = (available / (CX * AX));
	DX = (size / (CX * AX));
	Error (0,0,0);
}

static void GetDriveAllocInfo(struct sigcontext_struct *context)
{
	long size, available;
	
	if (!DOS_ValidDrive(DL)) {
		AX = 4;
		CX = 512;
		DX = 0;
		Error (InvalidDrive, EC_MediaError, EL_Disk);
		return;
	}

	if (!DOS_GetFreeSpace(DL, &size, &available)) {
		Error(GeneralFailure, EC_MediaError , EL_Disk);
		AX = 0xffff;
		return;
	}
	
	AX = 4;
	CX = 512;
	DX = (size / (CX * AX));

	heap->mediaID = 0xf0;

	DS = DosHeapHandle;
	BX = (int)&heap->mediaID - (int)heap;
	Error (0,0,0);
}

static void GetDefDriveAllocInfo(struct sigcontext_struct *context)
{
	DX = DOS_GetDefaultDrive();
	GetDriveAllocInfo(context);
}

static void GetDrivePB(struct sigcontext_struct *context, int drive)
{
        if(!DOS_ValidDrive(drive))
        {
	        Error (InvalidDrive, EC_MediaError, EL_Disk);
                AX = 0x00ff;
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
                dpb->shift = 0;
                dpb->reserved = 0;
                dpb->num_FAT = 1;
                dpb->dir_entries = 2;
                dpb->first_data = 2;
                dpb->high_cluster = 1023;
                dpb->sectors_in_FAT = 1;
                dpb->start_dir = 1;
                dpb->driver_head = 0;
                dpb->media_ID = (drive > 1) ? 0xF8 : 0xF0;
                dpb->access_flag = 0;
                dpb->next = 0;
                dpb->free_search = 0;
                dpb->free_clusters = 0xFFFF;    /* unknown */

                AL = 0x00;
                DS = SELECTOROF(dpbsegptr);
                BX = OFFSETOF(dpbsegptr);
        }
}

static void ReadFile(struct sigcontext_struct *context)
{
	char *ptr;
	int size;

	/* can't read from stdout / stderr */
	if ((BX == 1) || (BX == 2)) {
		Error (InvalidHandle, EL_Unknown, EC_Unknown);
		AX = InvalidHandle;
		SetCflag;
        	dprintf_int(stddeb,
			"int21: read (%d, void *, 0x%x) = EBADF\n", BX, CX);
		return;
	}

	ptr = PTR_SEG_OFF_TO_LIN (DS,DX);
	if (BX == 0) {
		*ptr = EOF;
		Error (0,0,0);
		AX = 1;
		ResetCflag;
        	dprintf_int(stddeb,
			"int21: read (%d, void *, 0x%x) = EOF\n", BX, CX);
		return;
	} else {
		size = read(BX, ptr, CX);
        	dprintf_int(stddeb,
			"int21: read (%d, void *, 0x%x) = 0x%x\n",
			BX, CX, size);
		if (size == -1) {
			errno_to_doserr();
			AX = ExtendedError;
			SetCflag;
			return;
		}		
		Error (0,0,0);
		AX = size;
		ResetCflag;
	}
}

static void WriteFile(struct sigcontext_struct *context)
{
	char *ptr;
	int x,size;
	
	ptr = PTR_SEG_OFF_TO_LIN (DS,DX);
	
	if (BX == 0) {
		Error (InvalidHandle, EC_Unknown, EL_Unknown);
		EAX = InvalidHandle;
		SetCflag;
		return;
	}

	if (BX < 3) {
		for (x = 0;x != CX;x++) {
			dprintf_int(stddeb, "%c", *ptr++);
		}
		fflush(stddeb);

		Error (0,0,0);
		AX = CX;
		ResetCflag;
	} else {
		size = write(BX, ptr , CX);
		if (size == 0) {
			Error (WriteFault, EC_Unknown, EL_Unknown);
			AX = ExtendedError;
			return;
		}

		if (size == -1) {
			errno_to_doserr();
			AX = ExtendedError;
			SetCflag;
			return;
		}		
		Error (0,0,0);
		AX = size;
		ResetCflag;
	}
}

static void SeekFile(struct sigcontext_struct *context)
{
	off_t status, fileoffset;
	
	switch (AL) {
		case 1: fileoffset = SEEK_CUR;
			break;
		case 2: fileoffset = SEEK_END;
			break;
		default:
		case 0: fileoffset = SEEK_SET;
			break;
		}
        status = lseek(BX, (CX << 16) + DX, fileoffset);

	dprintf_int (stddeb, "int21: seek (%d, 0x%x, %d) = 0x%lx\n",
			BX, (CX << 16) + DX, AL, status);

	if (status == -1) {
		errno_to_doserr();
		AX = ExtendedError;
                SetCflag;
		return;
	}		
	Error (0,0,0);
	AX = LOWORD(status);
	DX = HIWORD(status);
	ResetCflag;
}

static void ioctlGetDeviceInfo(struct sigcontext_struct *context)
{

	dprintf_int (stddeb, "int21: ioctl (%d, GetDeviceInfo)\n", BX);

	switch (BX) {
		case 0:
		case 1:
		case 2:
			DX = 0x80d0 | (1 << (BX != 0));
                        ResetCflag;
			break;

		default:
		{
		    struct stat sbuf;
	    
		    if (fstat(BX, &sbuf) < 0)
		    {
			IntBarf(0x21, context);
			SetCflag;
			return;
		    }
	    
		    /* This isn't the right answer, but should be close enough. */
		    DX = 0x0943;
		}
	}
	ResetCflag;
}

static void ioctlGenericBlkDevReq(struct sigcontext_struct *context)
{
	BYTE *dataptr = PTR_SEG_OFF_TO_LIN(DS, DX);
	int drive;

	if (BL == 0)
		drive = DOS_GetDefaultDrive();
	else
		drive = BL - 1;

	if (!DOS_ValidDrive(drive)) {
                Error( FileNotFound, EC_NotFound, EL_Disk );
                AX = FileNotFound;
		SetCflag;
		return;
	}

	if (CH != 0x08) {
		IntBarf(0x21, context);
		return;
	}
	switch (CL) {
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
			ResetCflag;
			return;
		default:
			IntBarf(0x21, context);
	}
}

static void GetSystemDate(struct sigcontext_struct *context)
{
	struct tm *now;
	time_t ltime;

	ltime = time(NULL);
	now = localtime(&ltime);

	CX = now->tm_year + 1900;
	DX = ((now->tm_mon + 1) << 8) | now->tm_mday;
	AX = now->tm_wday;
}

static void GetSystemTime(struct sigcontext_struct *context)
{
	struct tm *now;
	struct timeval tv;

	gettimeofday(&tv,NULL);		/* Note use of gettimeofday(), instead of time() */
	now = localtime(&tv.tv_sec);
	 
	CX = (now->tm_hour<<8) | now->tm_min;
	DX = (now->tm_sec<<8) | tv.tv_usec/10000;
					/* Note hundredths of seconds */
}

static void GetExtendedErrorInfo(struct sigcontext_struct *context)
{
	AX = ExtendedError;
	BX = (ErrorClass << 8) | Action;
	CH = ErrorLocus;
}

static void CreateFile(struct sigcontext_struct *context)
{
	int handle;

	if ((handle = open(DOS_GetUnixFileName( PTR_SEG_OFF_TO_LIN(DS,DX)), 
           O_CREAT | O_TRUNC | O_RDWR )) == -1) {
		errno_to_doserr();
		AX = ExtendedError;
		SetCflag;
		return;
		}			
	Error (0,0,0);
        AX = handle;
	ResetCflag;
}

void OpenExistingFile(struct sigcontext_struct *context)
{
	int handle;
	int mode;
	int lock;
	
	switch (AX & 0x0007)
	{
	  case 0:
	    mode = O_RDONLY;
	    break;
	    
	  case 1:
	    mode = O_WRONLY;
	    break;

	  default:
	    mode = O_RDWR;
	    break;
	}

	if ((handle = open(DOS_GetUnixFileName(PTR_SEG_OFF_TO_LIN(DS,DX)),
                           mode)) == -1)
        {
            if( Options.allowReadOnly )
                handle = open( DOS_GetUnixFileName(PTR_SEG_OFF_TO_LIN(DS,DX)),
                               O_RDONLY );
            if( handle == -1 )
            {
		dprintf_int (stddeb, "int21: open (%s, %d) = -1\n",
			DOS_GetUnixFileName(PTR_SEG_OFF_TO_LIN(DS,DX)), mode);
                errno_to_doserr();
                AX = ExtendedError;
                SetCflag;
                return;
            }
	}		

	dprintf_int (stddeb, "int21: open (%s, %d) = %d\n",
		DOS_GetUnixFileName(PTR_SEG_OFF_TO_LIN(DS,DX)), mode, handle);

        switch (AX & 0x0070)
	{
	  case 0x00:    /* compatability mode */
	  case 0x40:    /* DENYNONE */
            lock = -1;
	    break;

	  case 0x30:    /* DENYREAD */
	    dprintf_int(stddeb,
	      "OpenExistingFile (%s): DENYREAD changed to DENYALL\n",
	      (char *)PTR_SEG_OFF_TO_LIN(DS,DX));
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
	    result = flock(handle, lock | LOCK_NB);
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
	    AX = ExtendedError;
	    close(handle);
	    SetCflag;
	    return;
	  }

        }

	Error (0,0,0);
	AX = handle;
	ResetCflag;
}

static void CloseFile(struct sigcontext_struct *context)
{
	dprintf_int (stddeb, "int21: close (%d)\n", BX);

	if (close(BX) == -1) {
		errno_to_doserr();
		AX = ExtendedError;
		SetCflag;
		return;
	}
	Error (0,0,0);
	AX = NoError;
	ResetCflag;
}

static void RenameFile(struct sigcontext_struct *context)
{
	char *newname, *oldname;

        /* FIXME: should not rename over an existing file */
	dprintf_int(stddeb,"int21: renaming %s to %s\n",
			(char *)PTR_SEG_OFF_TO_LIN(DS,DX), (char *)PTR_SEG_OFF_TO_LIN(ES,DI) );
	
	oldname = DOS_GetUnixFileName( PTR_SEG_OFF_TO_LIN(DS,DX) );
	newname = DOS_GetUnixFileName( PTR_SEG_OFF_TO_LIN(ES,DI) );

	rename( oldname, newname);
	ResetCflag;
}


static void MakeDir(struct sigcontext_struct *context)
{
	char *dirname;

	dprintf_int(stddeb,"int21: makedir %s\n", (char *)PTR_SEG_OFF_TO_LIN(DS,DX) );
	
	if ((dirname = DOS_GetUnixFileName( PTR_SEG_OFF_TO_LIN(DS,DX) ))== NULL) {
            Error( CanNotMakeDir, EC_AccessDenied, EL_Disk );
            AX = CanNotMakeDir;
            SetCflag;
            return;
	}

	if (mkdir(dirname,0) == -1) {
            Error( CanNotMakeDir, EC_AccessDenied, EL_Disk );
            AX = CanNotMakeDir;
            SetCflag;
            return;
	}
	ResetCflag;
}

static void ChangeDir(struct sigcontext_struct *context)
{
	int drive;
	char *dirname = PTR_SEG_OFF_TO_LIN(DS,DX);
	drive = DOS_GetDefaultDrive();
	dprintf_int(stddeb,"int21: changedir %s\n", dirname);
	if (dirname != NULL && dirname[1] == ':') {
		drive = toupper(dirname[0]) - 'A';
		dirname += 2;
		}
	if (!DOS_ChangeDir(drive, dirname))
	{
		SetCflag;
		AX=0x03;
	}
}

static void RemoveDir(struct sigcontext_struct *context)
{
	char *dirname;

	dprintf_int(stddeb,"int21: removedir %s\n", (char *)PTR_SEG_OFF_TO_LIN(DS,DX) );

	if ((dirname = DOS_GetUnixFileName( PTR_SEG_OFF_TO_LIN(DS,DX) ))== NULL) {
            Error( PathNotFound, EC_NotFound, EL_Disk );
            AX = PathNotFound;
            SetCflag;
            return;
	}

/*
	if (strcmp(unixname,DosDrives[drive].CurrentDirectory)) {
		AL = CanNotRemoveCwd;
		SetCflag;
	}
*/	
	if (rmdir(dirname) == -1) {
            Error( AccessDenied, EC_AccessDenied, EL_Disk );
            AX = AccessDenied;
            SetCflag;
            return;
	} 
	ResetCflag;
}

static void FindNext(struct sigcontext_struct *context)
{
	struct dosdirent *dp;
        struct tm *t;
	BYTE *dta = GetCurrentDTA();

        memcpy(&dp, dta+0x11, sizeof(dp));

	do {
		if ((dp = DOS_readdir(dp)) == NULL) {
			Error(NoMoreFiles, EC_MediaError , EL_Disk);
			AX = NoMoreFiles;
			SetCflag;
			return;
		}
	} /* while (*(dta + 0x0c) != dp->attribute);*/
        while ( ( dp->search_attribute & dp->attribute) != dp->attribute);
	
  	*(dta + 0x15) = dp->attribute;
        setword(&dta[0x0d], dp->entnum);

        t = localtime(&(dp->filetime));
	setword(&dta[0x16], (t->tm_hour << 11) + (t->tm_min << 5) +
                (t->tm_sec / 2)); /* time */
	setword(&dta[0x18], ((t->tm_year - 80) << 9) + (t->tm_mon << 5) +
                (t->tm_mday)); /* date */
	setdword(&dta[0x1a], dp->filesize);
	strncpy(dta + 0x1e, dp->filename, 13);

	AX = 0;
	ResetCflag;

        dprintf_int(stddeb, "int21: FindNext -- (%s) index=%d size=%ld\n", dp->filename, dp->entnum, dp->filesize);
	return;
}

static void FindFirst(struct sigcontext_struct *context)
{
	BYTE drive, *path = PTR_SEG_OFF_TO_LIN(DS, DX);
	struct dosdirent *dp;

	BYTE *dta = GetCurrentDTA();

        dprintf_int(stddeb, "int21: FindFirst path = %s\n", path);

	if ((*path)&&(path[1] == ':')) {
		drive = (islower(*path) ? toupper(*path) : *path) - 'A';

		if (!DOS_ValidDrive(drive)) {
			Error(InvalidDrive, EC_MediaError , EL_Disk);
			AX = InvalidDrive;
			SetCflag;
			return;
		}
	} else
		drive = DOS_GetDefaultDrive();

	*dta = drive;
	memset(dta + 1 , '?', 11);
	*(dta + 0x0c) = ECX & (FA_LABEL | FA_DIREC);

	if (ECX & FA_LABEL) {
		/* return volume label */

		if (DOS_GetVolumeLabel(drive) != NULL) 
			strncpy(dta + 0x1e, DOS_GetVolumeLabel(drive), 8);

		AX = 0;
		ResetCflag;
		return;
	}

	if ((dp = DOS_opendir(path)) == NULL) {
		Error(PathNotFound, EC_MediaError, EL_Disk);
		AX = FileNotFound;
		SetCflag;
		return;
	}

	dp->search_attribute = ECX & (FA_LABEL | FA_DIREC);
	memcpy(dta + 0x11, &dp, sizeof(dp));
	FindNext(context);
}

static void GetFileDateTime(struct sigcontext_struct *context)
{
	struct stat filestat;
	struct tm *now;

        fstat( BX, &filestat );	
	now = localtime (&filestat.st_mtime);
	
	CX = ((now->tm_hour * 0x2000) + (now->tm_min * 0x20) + now->tm_sec/2);
	DX = (((now->tm_year + 1900 - 1980) * 0x200) +
              (now->tm_mon * 0x20) + now->tm_mday);

	ResetCflag;
}

static void SetFileDateTime(struct sigcontext_struct *context)
{
	char *filename;
	struct utimbuf filetime;
	
        /* FIXME: Argument isn't the name of the file in DS:DX,
           but the file handle in BX */
	filename = DOS_GetUnixFileName( PTR_SEG_OFF_TO_LIN(DS,DX) );

	filetime.actime = 0L;
	filetime.modtime = filetime.actime;

	utime(filename, &filetime);
	ResetCflag;
}

static void CreateTempFile(struct sigcontext_struct *context)
{
	char temp[256];
	int handle;
	
	sprintf(temp,"%s\\win%d.tmp",TempDirectory,(int) getpid());

	dprintf_int(stddeb,"CreateTempFile %s\n",temp);

	handle = open(DOS_GetUnixFileName(temp), O_CREAT | O_TRUNC | O_RDWR);

	if (handle == -1) {
            Error( WriteProtected, EC_AccessDenied, EL_Disk );
            AX = WriteProtected;
            SetCflag;
            return;
	}

	strcpy(PTR_SEG_OFF_TO_LIN(DS,DX), temp);
	
	AX = handle;
	ResetCflag;
}

static void CreateNewFile(struct sigcontext_struct *context)
{
	int handle;
	
	if ((handle = open(DOS_GetUnixFileName( PTR_SEG_OFF_TO_LIN(DS,DX) ), O_CREAT | O_EXCL | O_RDWR)) == -1) {
            Error( WriteProtected, EC_AccessDenied, EL_Disk );
            AX = WriteProtected;
            SetCflag;
            return;
	}

	AX = handle;
	ResetCflag;
}

static void GetCurrentDirectory(struct sigcontext_struct *context)
{
	int drive;

	if (DL == 0)
		drive = DOS_GetDefaultDrive();
	else
		drive = DL - 1;

	if (!DOS_ValidDrive(drive)) {
            Error( InvalidDrive, EC_NotFound, EL_Disk );
            AX = InvalidDrive;
            SetCflag;
            return;
	}

	strcpy(PTR_SEG_OFF_TO_LIN(DS,SI), DOS_GetCurrentDir(drive) );
	ResetCflag;
}

static void GetDiskSerialNumber(struct sigcontext_struct *context)
{
	int drive;
	BYTE *dataptr = PTR_SEG_OFF_TO_LIN(DS, DX);
	DWORD serialnumber;
	
	if (BL == 0)
		drive = DOS_GetDefaultDrive();
	else
		drive = BL - 1;

	if (!DOS_ValidDrive(drive)) {
            Error( InvalidDrive, EC_NotFound, EL_Disk );
            AX = InvalidDrive;
            SetCflag;
            return;
	}

	DOS_GetSerialNumber(drive, &serialnumber);

	setword(dataptr, 0);
	setdword(&dataptr[2], serialnumber);
	strncpy(dataptr + 6, DOS_GetVolumeLabel(drive), 8);
	strncpy(dataptr + 0x11, "FAT16   ", 8);
	
	AX = 0;
	ResetCflag;
}

static void SetDiskSerialNumber(struct sigcontext_struct *context)
{
	int drive;
	BYTE *dataptr = PTR_SEG_OFF_TO_LIN(DS, DX);
	DWORD serialnumber;

	if (BL == 0)
		drive = DOS_GetDefaultDrive();
	else
		drive = BL - 1;

	if (!DOS_ValidDrive(drive)) {
            Error( InvalidDrive, EC_NotFound, EL_Disk );
            AX = InvalidDrive;
            SetCflag;
            return;
	}

	serialnumber = dataptr[1] + (dataptr[2] << 8) + (dataptr[3] << 16) + 
			(dataptr[4] << 24);

	DOS_SetSerialNumber(drive, serialnumber);
	AX = 1;
	ResetCflag;
}

static void DumpFCB(BYTE *fcb)
{
	int x, y;

	fcb -= 7;

	for (y = 0; y !=2 ; y++) {
		for (x = 0; x!=15;x++)
			dprintf_int(stddeb, "%02x ", *fcb++);
		dprintf_int(stddeb,"\n");
	}
}

/* microsoft's programmers should be shot for using CP/M style int21
   calls in Windows for Workgroup's winfile.exe */

static void FindFirstFCB(struct sigcontext_struct *context)
{
	BYTE *fcb = PTR_SEG_OFF_TO_LIN(DS, DX);
	struct fcb *standard_fcb;
	struct fcb *output_fcb;
	int drive;
	char path[12];

	BYTE *dta = GetCurrentDTA();

	DumpFCB( fcb );
	
	if ((*fcb) == 0xff)
	  {
	    standard_fcb = (struct fcb *)(fcb + 7);
	    output_fcb = (struct fcb *)(dta + 7);
	    *dta = 0xff;
	  }
	else
	  {
	    standard_fcb = (struct fcb *)fcb;
	    output_fcb = (struct fcb *)dta;
	  }

	if (standard_fcb->drive)
	  {
	    drive = standard_fcb->drive - 1;
	    if (!DOS_ValidDrive(drive))
	      {
		Error (InvalidDrive, EC_MediaError, EL_Disk);
		AX = 0xff;
		return;
	      }
	  }
	else
	  drive = DOS_GetDefaultDrive();

	output_fcb->drive = drive;

	if (*(fcb) == 0xff)
	  {
	    if (*(fcb+6) & FA_LABEL)      /* return volume label */
	      {
		*(dta+6) = FA_LABEL;
		memset(&output_fcb->name, ' ', 11);
		if (DOS_GetVolumeLabel(drive) != NULL) 
		  {
		    strncpy(output_fcb->name, DOS_GetVolumeLabel(drive), 11);
		    AX = 0x00;
		    return;
		  }
	      }
	  }

	strncpy(output_fcb->name, standard_fcb->name, 11);
	if (*fcb == 0xff)
	  *(dta+6) = ( *(fcb+6) & (!FA_DIREC));

	sprintf(path,"%c:*.*",drive+'A');
	if ((output_fcb->directory = DOS_opendir(path))==NULL)
	  {
	    Error (PathNotFound, EC_MediaError, EL_Disk);
	    AX = 0xff;
	    return;
	  }
	
}


static void DeleteFileFCB(struct sigcontext_struct *context)
{
	BYTE *fcb = PTR_SEG_OFF_TO_LIN(DS, DX);
	int drive;
	struct dosdirent *dp;
	char temp[256], *ptr;

	DumpFCB( fcb );

	if (*fcb)
		drive = *fcb - 1;
	else
		drive = DOS_GetDefaultDrive();

	strcpy(temp, DOS_GetCurrentDir(drive));
	strcat(temp, "\\");
	strncat(temp, fcb + 1, 8);
	ChopOffWhiteSpace(temp);
	strncat(temp, fcb + 9, 3);
	ChopOffWhiteSpace(temp);

	if ((dp = DOS_opendir(temp)) == NULL) {
		Error(InvalidDrive, EC_MediaError , EL_Disk);
		AX = 0xff;
		return;
	}

	strcpy(temp, DOS_GetCurrentDir(drive) );
	strcat(temp, "\\");
	
	ptr = temp + strlen(temp);
	
	while (DOS_readdir(dp) != NULL)
	{
		strcpy(ptr, dp->filename);
		dprintf_int(stddeb, "int21: delete file %s\n", temp);
		/* unlink(DOS_GetUnixFileName(temp)); */
	}
	DOS_closedir(dp);
	AX = 0;
}

static void RenameFileFCB(struct sigcontext_struct *context)
{
	BYTE *fcb = PTR_SEG_OFF_TO_LIN(DS, DX);
	int drive;
	struct dosdirent *dp;
	char temp[256], oldname[256], newname[256], *oldnameptr, *newnameptr;

	DumpFCB( fcb );

	if (*fcb)
		drive = *fcb - 1;
	else
		drive = DOS_GetDefaultDrive();

	strcpy(temp, DOS_GetCurrentDir(drive));
	strcat(temp, "\\");
	strncat(temp, fcb + 1, 8);
	ChopOffWhiteSpace(temp);
	strncat(temp, fcb + 9, 3);
	ChopOffWhiteSpace(temp);

	if ((dp = DOS_opendir(temp)) == NULL) {
		Error(InvalidDrive, EC_MediaError , EL_Disk);
		AX = 0xff;
		return;
	}

	strcpy(oldname, DOS_GetCurrentDir(drive) );
	strcat(oldname, "\\");
	oldnameptr = oldname + strlen(oldname);

	strcpy(newname, DOS_GetCurrentDir(drive) );
	strcat(newname, "\\");
	newnameptr = newname + strlen(newname);
	
	while (DOS_readdir(dp) != NULL)
	{
		strcpy(oldnameptr, dp->filename);
		strcpy(newnameptr, fcb + 1);
		dprintf_int(stddeb, "int21: renamefile %s -> %s\n",
			oldname, newname);
	}
	DOS_closedir(dp);
	AX = 0;
}



static void fLock (struct sigcontext_struct * context)
{
    struct flock f;
    int result,retries=sharing_retries;

    f.l_start = MAKELONG(DX,CX);
    f.l_len   = MAKELONG(DI,SI);
    f.l_whence = 0;
    f.l_pid = 0;

    switch ( AX & 0xff )
    {
        case 0x00: /* LOCK */
	  f.l_type = F_WRLCK;
          break;

	case 0x01: /* UNLOCK */
          f.l_type = F_UNLCK;
	  break;

	default:
	  AX = 0x0001;
	  SetCflag;
	  return;
     }
 
     {
          result = fcntl(BX,F_SETLK,&f); 
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
         AX = ExtendedError;
         SetCflag;
         return;
      }

       Error (0,0,0);
       ResetCflag;
} 


static void GetFileAttribute (struct sigcontext_struct * context)
{
  char *filename = PTR_SEG_OFF_TO_LIN (DS,DX);
  struct stat s;
  int res;

  res = stat(DOS_GetUnixFileName(filename), &s);
  if (res==-1)
  {
    errno_to_doserr();
    AX = ExtendedError;
    SetCflag;
    return;
  }
  
  CX = 0;
  if (S_ISDIR(s.st_mode))
    CX |= 0x10;
  if ((S_IWRITE & s.st_mode) != S_IWRITE)
    CX |= 0x01;

  dprintf_int (stddeb, "int21: GetFileAttributes (%s) = 0x%x\n",
		filename, CX );

  ResetCflag;
  Error (0,0,0); 
}


extern void LOCAL_PrintHeap (WORD ds);

/************************************************************************/

int do_int21(struct sigcontext_struct * context)
{
    int drive;

    dprintf_int(stddeb,"int21: AX %04x, BX %04x, CX %04x, DX %04x, "
           "SI %04x, DI %04x, DS %04x, ES %04x\n",
           AX, BX, CX, DX, SI, DI, DS, ES);

    if (AH == 0x59) 
    {
	GetExtendedErrorInfo(context);
	return 1;
    } 
    else 
    {
	Error (0,0,0);
	switch(AH) 
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
	  case 0x12: /* FIND NEXT MATCHING FILE USING FCB */
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
	    IntBarf(0x21, context);
	    break;

	  case 0x2b: /* SET SYSTEM DATE */
	  case 0x2d: /* SET SYSTEM TIME */
	  case 0x37: /* "SWITCHAR" - GET SWITCH CHARACTER
			"SWITCHAR" - SET SWITCH CHARACTER
			"AVAILDEV" - SPECIFY \DEV\ PREFIX USE */
	  case 0x54: /* GET VERIFY FLAG */
            IntBarf(0x21, context);
	    break;

	  case 0x18: /* NULL FUNCTIONS FOR CP/M COMPATIBILITY */
	  case 0x1d:
	  case 0x1e:
	  case 0x20:
	  case 0x6b: /* NULL FUNCTION */
            AL = 0;
	    break;
	
	  case 0x5c: /* "FLOCK" - RECORD LOCKING */
	    fLock(context);
	    break;

	  case 0x0d: /* DISK BUFFER FLUSH */
            ResetCflag; /* dos 6+ only */
            break;

	  case 0x0e: /* SELECT DEFAULT DRIVE */
		if (!DOS_ValidDrive(DL)) {
			Error (InvalidDrive, EC_MediaError, EL_Disk);
			AX = MAX_DOS_DRIVES; 
			break;
		} else {
			DOS_SetDefaultDrive(DL);
			AX = MAX_DOS_DRIVES; 
			Error(0,0,0);
		}
		break;

	  case 0x11: /* FIND FIRST MATCHING FILE USING FCB */
            FindFirstFCB(context);
            break;

	  case 0x13: /* DELETE FILE USING FCB */
            DeleteFileFCB(context);
            break;
            
	  case 0x17: /* RENAME FILE USING FCB */
            RenameFileFCB(context);
            break;

	  case 0x19: /* GET CURRENT DEFAULT DRIVE */
		AL = DOS_GetDefaultDrive();
		Error (0,0,0);
	    break;

	  case 0x1a: /* SET DISK TRANSFER AREA ADDRESS */
            {
                TDB *pTask = (TDB *)GlobalLock( GetCurrentTask() );
                pTask->dta = MAKELONG( DX, DS );
                dprintf_int(stddeb, "int21: Set DTA: %08lx\n", pTask->dta);
            }
            break;

	  case 0x1b: /* GET ALLOCATION INFORMATION FOR DEFAULT DRIVE */
	    GetDefDriveAllocInfo(context);
	    break;
	
	  case 0x1c: /* GET ALLOCATION INFORMATION FOR SPECIFIC DRIVE */
	    GetDriveAllocInfo(context);
	    break;

	  case 0x1f: /* GET DRIVE PARAMETER BLOCK FOR DEFAULT DRIVE */
	    GetDrivePB(context, DOS_GetDefaultDrive());
	    break;
		
	  case 0x25: /* SET INTERRUPT VECTOR */
            INT_SetHandler( AL, MAKELONG( DX, DS ) );
            break;

	  case 0x2a: /* GET SYSTEM DATE */
	    GetSystemDate(context);
            break;

	  case 0x2c: /* GET SYSTEM TIME */
	    GetSystemTime(context);
	    break;

	  case 0x2f: /* GET DISK TRANSFER AREA ADDRESS */
            {
                TDB *pTask = (TDB *)GlobalLock( GetCurrentTask() );
                ES = SELECTOROF( pTask->dta );
                BX = OFFSETOF( pTask->dta );
            }
            break;
            
	  case 0x30: /* GET DOS VERSION */
	    AX = DOSVERSION;
	    BX = 0x0012;     /* 0x123456 is Wine's serial # */
	    CX = 0x3456;
	    break;

	  case 0x31: /* TERMINATE AND STAY RESIDENT */
            IntBarf(0x21, context);
	    break;

	  case 0x32: /* GET DOS DRIVE PARAMETER BLOCK FOR SPECIFIC DRIVE */
	    GetDrivePB(context, (DL == 0) ? (DOS_GetDefaultDrive()) : (DL-1));
	    break;

	  case 0x33: /* MULTIPLEXED */
	    switch (AL) {
	      case 0x00: /* GET CURRENT EXTENDED BREAK STATE */
                DL = 0;
		break;

	      case 0x01: /* SET EXTENDED BREAK STATE */
		break;		
		
	      case 0x02: /* GET AND SET EXTENDED CONTROL-BREAK CHECKING STATE*/
		DL = 0;
		break;

	      case 0x05: /* GET BOOT DRIVE */
		DL = 2;
		/* c: is Wine's bootdrive */
		break;
				
	      case 0x06: /* GET TRUE VERSION NUMBER */
		BX = DOSVERSION;
		DX = 0x00;
		break;

	      default:
		IntBarf(0x21, context);
		break;			
	    }
	    break;	
	    
	  case 0x34: /* GET ADDRESS OF INDOS FLAG */
		ES = DosHeapHandle;
		BX = (int)&heap->InDosFlag - (int)heap;
	    break;

	  case 0x35: /* GET INTERRUPT VECTOR */
            {
                SEGPTR addr = INT_GetHandler( AL );
                ES = SELECTOROF(addr);
                BX = OFFSETOF(addr);
            }
	    break;

	  case 0x36: /* GET FREE DISK SPACE */
	    GetFreeDiskSpace(context);
	    break;

	  case 0x38: /* GET COUNTRY-SPECIFIC INFORMATION */
	    AX = 0x02; /* no country support available */
	    SetCflag;
	    break;
		
	  case 0x39: /* "MKDIR" - CREATE SUBDIRECTORY */
	    MakeDir(context);
	    break;
	
	  case 0x3a: /* "RMDIR" - REMOVE SUBDIRECTORY */
	    RemoveDir(context);
	    break;
	
	  case 0x3b: /* "CHDIR" - SET CURRENT DIRECTORY */
	    ChangeDir(context);
	    break;
	
	  case 0x3c: /* "CREAT" - CREATE OR TRUNCATE FILE */
	    CreateFile(context);
	    break;

	  case 0x3d: /* "OPEN" - OPEN EXISTING FILE */
	    OpenExistingFile(context);
	    break;
	
	  case 0x3e: /* "CLOSE" - CLOSE FILE */
	    CloseFile(context);
	    break;
	
	  case 0x3f: /* "READ" - READ FROM FILE OR DEVICE */
	    ReadFile(context);
	    break;
	
	  case 0x40: /* "WRITE" - WRITE TO FILE OR DEVICE */
	    WriteFile(context);
	    break;
	
	  case 0x41: /* "UNLINK" - DELETE FILE */
		if (unlink( DOS_GetUnixFileName( PTR_SEG_OFF_TO_LIN(DS,DX)) ) == -1) {
			errno_to_doserr();
			AX = ExtendedError;
			SetCflag;
			break;
		}		
		Error(0,0,0);
		ResetCflag;
		break;
	
	  case 0x42: /* "LSEEK" - SET CURRENT FILE POSITION */
	    SeekFile(context);
	    break;
	    
	  case 0x43: /* FILE ATTRIBUTES */
	    switch (AL) 
	    {
	      case 0x00:
                        GetFileAttribute(context);
		break;
	      case 0x01:
			ResetCflag;
		break;
	    }
	    break;
		
	  case 0x44: /* IOCTL */
	    switch (AL) 
	    {
              case 0x00:
                ioctlGetDeviceInfo(context);
		break;

              case 0x08:   /* Check if drive is removable. */
                drive = BL ? (BL - 1) : DOS_GetDefaultDrive();
                if(!DOS_ValidDrive(drive))
                {
                    Error( InvalidDrive, EC_NotFound, EL_Disk );
                    AX = InvalidDrive;
                    SetCflag;
                }
                else
                {
                    if (drive > 1)
                        AX = 1;   /* not removable */
                    else
                        AX = 0;      /* removable */
                    ResetCflag;
                }
                break;
		   
	      case 0x09:   /* CHECK IF BLOCK DEVICE REMOTE */
                drive = BL ? (BL - 1) : DOS_GetDefaultDrive();
                if(!DOS_ValidDrive(drive))
                {
                    Error( InvalidDrive, EC_NotFound, EL_Disk );
                    AX = InvalidDrive;
                    SetCflag;
                }
                else
                {
		    DX = (1<<9) | (1<<12) | (1<<15);
		    ResetCflag;
                }
		break;

	      case 0x0b:   /* SET SHARING RETRY COUNT */
		if (!CX)
		{ 
		  AX = 1;
		  SetCflag;
		  break;
		}
		sharing_pause = CX;
		if (!DX)
		  sharing_retries = DX;
		ResetCflag;
		break;

              case 0x0d:
                ioctlGenericBlkDevReq(context);
                break;

              case 0x0F:   /* Set logical drive mapping */
                /* FIXME: Not implemented at the moment, always returns error
                 */
                AX = 0x0001; /* invalid function */
                SetCflag;
                break;
                
	      default:
                IntBarf(0x21, context);
		break;
	    }
	    break;

	  case 0x45: /* "DUP" - DUPLICATE FILE HANDLE */
            if ((AX = dup(BX)) == 0xffff)
            {
                errno_to_doserr();
                AX = ExtendedError;
                SetCflag;
            }
            else ResetCflag;
	    break;

	  case 0x46: /* "DUP2", "FORCEDUP" - FORCE DUPLICATE FILE HANDLE */
            if (dup2( BX, CX ) == -1)
            {
                errno_to_doserr();
                AX = ExtendedError;
                SetCflag;
            }
            else ResetCflag;
            break;

	  case 0x47: /* "CWD" - GET CURRENT DIRECTORY */
	    GetCurrentDirectory(context);
	    AX = 0x0100; 
	    /* intlist: many Microsoft products for Windows rely on this */
	    break;
	
	  case 0x48: /* ALLOCATE MEMORY */
	  case 0x49: /* FREE MEMORY */
	  case 0x4a: /* RESIZE MEMORY BLOCK */
	    IntBarf(0x21, context);
	    break;
	
	  case 0x4b: /* "EXEC" - LOAD AND/OR EXECUTE PROGRAM */
            WinExec( PTR_SEG_OFF_TO_LIN(DS,DX), SW_NORMAL );
	    break;		
	
	  case 0x4c: /* "EXIT" - TERMINATE WITH RETURN CODE */
            TASK_KillCurrentTask( AL );
	    break;

	  case 0x4d: /* GET RETURN CODE */
	    AX = NoError; /* normal exit */
	    break;

	  case 0x4e: /* "FINDFIRST" - FIND FIRST MATCHING FILE */
	    FindFirst(context);
	    break;

	  case 0x4f: /* "FINDNEXT" - FIND NEXT MATCHING FILE */
	    FindNext(context);
	    break;
			
	  case 0x52: /* "SYSVARS" - GET LIST OF LISTS */
		ES = 0x0;
		BX = 0x0;
		IntBarf(0x21, context);
	    break;
		
	  case 0x56: /* "RENAME" - RENAME FILE */
	    RenameFile(context);
	    break;
	
	  case 0x57: /* FILE DATE AND TIME */
	    switch (AL) 
	    {
	      case 0x00:
		GetFileDateTime(context);
		break;
	      case 0x01:
		SetFileDateTime(context);
		break;
	    }
	    break;

	  case 0x58: /* GET OR SET MEMORY/UMB ALLOCATION STRATEGY */
	    switch (AL) 
	    {
	      case 0x00:
		AX = 1;
		break;
	      case 0x02:
		AX = 0;
		break;
	      case 0x01:
	      case 0x03:
		break;
	    }
	    ResetCflag;
	    break;
	
	  case 0x5a: /* CREATE TEMPORARY FILE */
	    CreateTempFile(context);
	    break;
	
	  case 0x5b: /* CREATE NEW FILE */
	    CreateNewFile(context);
	    break;
	
	  case 0x5d: /* NETWORK */
	  case 0x5e:
	    /* network software not installed */
	    AX = NoNetwork;
	    SetCflag;
	    break;
	
	  case 0x5f: /* NETWORK */
	    switch (AL) 
	    {
	      case 0x07: /* ENABLE DRIVE */
		if (!DOS_EnableDrive(DL)) 
		{
		    Error(InvalidDrive, EC_MediaError , EL_Disk);
		    AX = InvalidDrive;
		    SetCflag;
		    break;
		}
		else 
		{
		    ResetCflag;
		    break;
		}
	      case 0x08: /* DISABLE DRIVE */
		if (!DOS_DisableDrive(DL)) 
		{
		    Error(InvalidDrive, EC_MediaError , EL_Disk);
		    AX = InvalidDrive;
		    SetCflag;
		    break;
		} 
		else 
		{
		    ResetCflag;
		    break;
		}
	      default:
		/* network software not installed */
		AX = NoNetwork; 
		SetCflag;
		break;
	    }
	    break;

	  case 0x60: /* "TRUENAME" - CANONICALIZE FILENAME OR PATH */
		strncpy(PTR_SEG_OFF_TO_LIN(ES,DI), PTR_SEG_OFF_TO_LIN(DS,SI), strlen(PTR_SEG_OFF_TO_LIN(DS,SI)) & 0x7f);
		ResetCflag;
	    break;

	  case 0x61: /* UNUSED */
	  case 0x62: /* GET CURRENT PSP ADDRESS */
	  case 0x63: /* UNUSED */
	  case 0x64: /* OS/2 DOS BOX */
	  case 0x65: /* GET EXTENDED COUNTRY INFORMATION */
		IntBarf(0x21, context);
	    break;
	
	  case 0x66: /* GLOBAL CODE PAGE TABLE */
	    switch (AL) {
	      case 0x01:
		BX = CodePage;
		DX = BX;
		ResetCflag;
		break;			
	      case 0x02: 
		CodePage = BX;
		ResetCflag;
		break;
	    }
	    break;

	  case 0x67: /* SET HANDLE COUNT */			
	    ResetCflag;
	    break;

	  case 0x68: /* "FFLUSH" - COMMIT FILE */
	  case 0x6a: /* COMMIT FILE */
            fsync( BX );
	    ResetCflag;
	    break;		
	
	  case 0x69: /* DISK SERIAL NUMBER */
	    switch (AL) 
	    {
	      case 0x00:
		GetDiskSerialNumber(context);
		break;			
	      case 0x01: 
		SetDiskSerialNumber(context);
		break;
	    }
	    break;

	  case 0xea: /* NOVELL NETWARE - RETURN SHELL VERSION */
	    break;

	  default:
            IntBarf(0x21, context);
	    return 1;
	}
    }
    dprintf_int(stddeb,"ret21: AX %04x, BX %04x, CX %04x, DX %04x, "
           "SI %04x, DI %04x, DS %04x, ES %04x EFL %08lx\n",
           AX, BX, CX, DX, SI, DI, DS, ES, EFL);

    return 1;
}


/***********************************************************************
 *           DOS3Call  (KERNEL.102)
 */
void DOS3Call( struct sigcontext_struct context )
{
    do_int21( &context );
}

void INT21_Init(void)
{
    if ((DosHeapHandle = GlobalAlloc(GMEM_FIXED,sizeof(struct DosHeap))) == 0)
    {
        fprintf( stderr, "INT21_Init: Out of memory\n");
        exit(1);
    }
    heap = (struct DosHeap *) GlobalLock(DosHeapHandle);

    dpb = &heap->dpb;
    dpbsegptr = MAKELONG( (int)&heap->dpb - (int)heap, DosHeapHandle );
    heap->InDosFlag = 0;
    strcpy(heap->biosdate, "01/01/80");
}
