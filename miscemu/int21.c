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
#include "prototypes.h"
#include "regfunc.h"
#include "windows.h"
#include "wine.h"
#include "msdos.h"
#include "registers.h"
#include "options.h"
#include "stddebug.h"
/* #define DEBUG_INT */
/* #undef  DEBUG_INT */
#include "debug.h"

WORD ExtendedError, CodePage = 437;
BYTE ErrorClass, Action, ErrorLocus;
BYTE *dta;

struct DosHeap {
	BYTE dta[256];
	BYTE InDosFlag;
	BYTE biosdate[8];
};
static struct DosHeap *heap;

WORD sharing_retries = 3;      /* number of retries at sharing violation */
WORD sharing_pause = 1;        /* pause between retries */

extern char TempDirectory[];
void IntBarf(int i, struct sigcontext_struct *context);

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
			fprintf(stderr, "int21: unknown errno %d!\n", errno);
			Error (GeneralFailure, EC_SystemFailure, EL_Unknown);
			break;
	}
}

/*
static void Barf(struct sigcontext_struct *context)
{
	fprintf(stderr, "int21: unknown/not implemented parameters:\n");
	fprintf(stderr, "int21: AX %04x, BX %04x, CX %04x, DX %04x, "
	       "SI %04x, DI %04x, DS %04x, ES %04x\n",
	       AX, BX, CX, DX, SI, DI, DS, ES);
}
*/

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
	int drive;
	long size, available;
	BYTE mediaID;
	
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
	
	EAX = 4;
	ECX = 512;
	EDX = (size / (CX * AX));

	mediaID = 0xf0;

	DS = segment(mediaID);
	BX = offset(mediaID);	
	Error (0,0,0);
}

static void GetDefDriveAllocInfo(struct sigcontext_struct *context)
{
	DX = DOS_GetDefaultDrive();
	GetDriveAllocInfo(context);
}

static void GetDrivePB(struct sigcontext_struct *context)
{
	Error (InvalidDrive, EC_MediaError, EL_Disk);
	AX = 0x00ff;
		/* I'm sorry but I only got networked drives :-) */
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
		return;
	}

	ptr = pointer (DS,DX);
	if (BX == 0) {
		*ptr = EOF;
		Error (0,0,0);
		AX = 1;
		ResetCflag;
		return;
	} else {
		size = read(BX, ptr, CX);
		if (size == -1) {
			errno_to_doserr();
			AL = ExtendedError;
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
	
	ptr = pointer (DS,DX);
	
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
		AL = CX;
		ResetCflag;
	} else {
		size = write(BX, ptr , CX);
		if (size == 0) {
			Error (WriteFault, EC_Unknown, EL_Unknown);
			AL = ExtendedError;
			return;
		}

		if (size == -1) {
			errno_to_doserr();
			AL = ExtendedError;
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
	status = lseek(BX, (CX * 0x100) + DX, fileoffset);
	if (status == -1) {
		errno_to_doserr();
		AL = ExtendedError;			SetCflag;
		return;
	}		
	Error (0,0,0);
	AX = (status & 0xffff);
	DX = ((status >> 16) & 0xffff);

	ResetCflag;
}

static void ioctlGetDeviceInfo(struct sigcontext_struct *context)
{
	switch (BX) {
		case 0:
		case 1:
		case 2:
			DX = 0x80d3;
			break;

		default:
		{
		    struct stat sbuf;
	    
		    if (fstat(BX, &sbuf) < 0)
		    {
			IntBarf(0x21, context);
			DX = 0x50;
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
	BYTE *dataptr = pointer(DS, DX);
	int drive;

	if (BL == 0)
		drive = DOS_GetDefaultDrive();
	else
		drive = BL - 1;

	if (!DOS_ValidDrive(drive)) {
		AX = 0x02;				
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
	AL = ExtendedError;
	BX = (ErrorClass << 8) | Action;
	CH = ErrorLocus << 8;
}

static void CreateFile(struct sigcontext_struct *context)
{
	int handle;

	if ((handle = open(GetUnixFileName( pointer(DS,DX)), 
           O_CREAT | O_TRUNC | O_RDWR )) == -1) {
		errno_to_doserr();
		AL = ExtendedError;
		SetCflag;
		return;
		}			
	Error (0,0,0);
	EAX = (EAX & 0xffff0000) | handle;
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

	if ((handle = open(GetUnixFileName(pointer(DS,DX)), mode)) == -1) {
		errno_to_doserr();
		AL = ExtendedError;
		SetCflag;
		return;
	}		

        switch (AX & 0x0070)
	{
	  case 0x00:    /* compatability mode */
	  case 0x40:    /* DENYNONE */
            lock = -1;
	    break;

	  case 0x30:    /* DENYREAD */
	    dprintf_int(stdnimp,
	      "OpenExistingFile (%s): DENYREAD changed to DENYALL\n",
	      pointer(DS,DX));
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
	    EAX = (EAX & 0xffffff00) | ExtendedError;
	    close(handle);
	    SetCflag;
	    return;
	  }

        }

	Error (0,0,0);
	EAX = (EAX & 0xffff0000) | handle;
	ResetCflag;
}

static void CloseFile(struct sigcontext_struct *context)
{
	if (close(BX) == -1) {
		errno_to_doserr();
		AL = ExtendedError;
		SetCflag;
		return;
	}
	Error (0,0,0);
	AL = NoError;
	ResetCflag;
}

static void RenameFile(struct sigcontext_struct *context)
{
	char *newname, *oldname;

	dprintf_int(stddeb,"int21: renaming %s to %s\n",
			pointer(DS,DX), pointer(ES,DI) );
	
	oldname = GetUnixFileName( pointer(DS,DX) );
	newname = GetUnixFileName( pointer(ES,DI) );

	rename( oldname, newname);
	ResetCflag;
}


static void MakeDir(struct sigcontext_struct *context)
{
	char *dirname;

	dprintf_int(stddeb,"int21: makedir %s\n", pointer(DS,DX) );
	
	if ((dirname = GetUnixFileName( pointer(DS,DX) ))== NULL) {
		AL = CanNotMakeDir;
		SetCflag;
		return;
	}

	if (mkdir(dirname,0) == -1) {
		AL = CanNotMakeDir;
		SetCflag;
		return;
	}
	ResetCflag;
}

static void ChangeDir(struct sigcontext_struct *context)
{
	int drive;
	char *dirname = pointer(DS,DX);
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

	dprintf_int(stddeb,"int21: removedir %s\n", pointer(DS,DX) );

	if ((dirname = GetUnixFileName( pointer(DS,DX) ))== NULL) {
		AL = CanNotMakeDir;
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
		AL = CanNotMakeDir; 
		SetCflag;
	} 
	ResetCflag;
}

static void ExecProgram(struct sigcontext_struct *context)
{
	execl("wine", GetUnixFileName( pointer(DS,DX)) );
}

static void FindNext(struct sigcontext_struct *context)
{
	struct dosdirent *dp;
	
        memcpy(&dp, dta+0x0d, sizeof(dp));

	do {
		if ((dp = DOS_readdir(dp)) == NULL) {
			Error(NoMoreFiles, EC_MediaError , EL_Disk);
			AL = NoMoreFiles;
			SetCflag;
			return;
		}
	} /* while (*(dta + 0x0c) != dp->attribute);*/
       while ( ( dp->search_attribute & dp->attribute) != dp->attribute);
	
  	*(dta + 0x15) = dp->attribute;
	setword(&dta[0x16], 0x1234); /* time */
	setword(&dta[0x18], 0x1234); /* date */
	setdword(&dta[0x1a], dp->filesize);
	strncpy(dta + 0x1e, dp->filename, 13);

	AL = 0;
	ResetCflag;
	return;
}

static void FindFirst(struct sigcontext_struct *context)
{
	BYTE drive, *path = pointer(DS, DX);
	struct dosdirent *dp;

	if ((*path)&&(path[1] == ':')) {
		drive = (islower(*path) ? toupper(*path) : *path) - 'A';

		if (!DOS_ValidDrive(drive)) {
			Error(InvalidDrive, EC_MediaError , EL_Disk);
			AL = InvalidDrive;
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

		AL = 0;
		ResetCflag;
		return;
	}

	if ((dp = DOS_opendir(path)) == NULL) {
		Error(PathNotFound, EC_MediaError, EL_Disk);
		AL = FileNotFound;
		SetCflag;
		return;
	}

	dp->search_attribute = ECX & (FA_LABEL | FA_DIREC);
	memcpy(dta + 0x0d, &dp, sizeof(dp));
	FindNext(context);
}

static void GetFileDateTime(struct sigcontext_struct *context)
{
	char *filename;
	struct stat filestat;
	struct tm *now;

	if ((filename = GetUnixFileName( pointer(DS,DX) ))== NULL) {
		AL = FileNotFound;
		SetCflag;
		return;
	}
	stat(filename, &filestat);
	 	
	now = localtime (&filestat.st_mtime);
	
	CX = ((now->tm_hour * 0x2000) + (now->tm_min * 0x20) + now->tm_sec/2);
	DX = ((now->tm_year * 0x200) + (now->tm_mon * 0x20) + now->tm_mday);

	ResetCflag;
}

static void SetFileDateTime(struct sigcontext_struct *context)
{
	char *filename;
	struct utimbuf filetime;
	
	filename = GetUnixFileName( pointer(DS,DX) );

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

	handle = open(GetUnixFileName(temp), O_CREAT | O_TRUNC | O_RDWR);

	if (handle == -1) {
		AL = WriteProtected;
		SetCflag;
		return;
	}

	strcpy(pointer(DS,DX), temp);
	
	AX = handle;
	ResetCflag;
}

static void CreateNewFile(struct sigcontext_struct *context)
{
	int handle;
	
	if ((handle = open(GetUnixFileName( pointer(DS,DX) ), O_CREAT | O_EXCL | O_RDWR)) == -1) {
		AL = WriteProtected;
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
		AL = InvalidDrive;
		SetCflag;
		return;
	}

	strcpy(pointer(DS,SI), DOS_GetCurrentDir(drive) );
	ResetCflag;
}

static void GetDiskSerialNumber(struct sigcontext_struct *context)
{
	int drive;
	BYTE *dataptr = pointer(DS, DX);
	DWORD serialnumber;
	
	if (BL == 0)
		drive = DOS_GetDefaultDrive();
	else
		drive = BL - 1;

	if (!DOS_ValidDrive(drive)) {
		AL =InvalidDrive;
		SetCflag;
		return;
	}

	DOS_GetSerialNumber(drive, &serialnumber);

	setword(dataptr, 0);
	setdword(&dataptr[2], serialnumber);
	strncpy(dataptr + 6, DOS_GetVolumeLabel(drive), 8);
	strncpy(dataptr + 0x11, "FAT16   ", 8);
	
	AL = 0;
	ResetCflag;
}

static void SetDiskSerialNumber(struct sigcontext_struct *context)
{
	int drive;
	BYTE *dataptr = pointer(DS, DX);
	DWORD serialnumber;

	if (BL == 0)
		drive = DOS_GetDefaultDrive();
	else
		drive = BL - 1;

	if (!DOS_ValidDrive(drive)) {
		AL = InvalidDrive;
		SetCflag;
		return;
	}

	serialnumber = dataptr[1] + (dataptr[2] << 8) + (dataptr[3] << 16) + 
			(dataptr[4] << 24);

	DOS_SetSerialNumber(drive, serialnumber);
	AL = 1L;
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
	BYTE *fcb = pointer(DS, DX);
	struct fcb *standard_fcb;
	struct fcb *output_fcb;
	int drive;
	char path[12];

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
		AL = 0xff;
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
		    AL = 0x00;
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
	    AL = 0xff;
	    return;
	  }
	
}


static void DeleteFileFCB(struct sigcontext_struct *context)
{
	BYTE *fcb = pointer(DS, DX);
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
		AL = 0xffL;
		return;
	}

	strcpy(temp, DOS_GetCurrentDir(drive) );
	strcat(temp, "\\");
	
	ptr = temp + strlen(temp);
	
	while (DOS_readdir(dp) != NULL)
	{
		strcpy(ptr, dp->filename);
		dprintf_int(stddeb, "int21: delete file %s\n", temp);
		/* unlink(GetUnixFileName(temp)); */
	}
	DOS_closedir(dp);
	AL = 0;
}

static void RenameFileFCB(struct sigcontext_struct *context)
{
	BYTE *fcb = pointer(DS, DX);
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
		AL = 0xffL;
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
	AL = 0;
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
	  EAX = (EAX & 0xffff0000) | 0x0001;
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
         EAX = (EAX & 0xffffff00) | ExtendedError;
         SetCflag;
         return;
      }

       Error (0,0,0);
       ResetCflag;
} 


static void GetFileAttribute (struct sigcontext_struct * context)
{
  char *filename = pointer (DS,DX);
  struct stat s;
  int res,cx; 

  res = stat(GetUnixFileName(filename), &s);
  if (res==-1)
  {
    errno_to_doserr();
    EAX = (EAX & 0xffffff00) | ExtendedError;
    SetCflag;
    return;
  }
  
  cx = 0;
  if (S_ISDIR(s.st_mode))
    cx|=0x10;
  if ((S_IWRITE & s.st_mode) != S_IWRITE)
    cx|=0x01;

  ECX = (ECX & 0xffff0000) | cx;
  ResetCflag;
  Error (0,0,0); 
}



/************************************************************************/

int do_int21(struct sigcontext_struct * context)
{
    if (Options.relay_debug)
    {
	fprintf(stddeb,"int21: AX %04x, BX %04x, CX %04x, DX %04x, "
	       "SI %04x, DI %04x, DS %04x, ES %04x\n",
	       AX, BX, CX, DX, SI, DI, DS, ES);
    }

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
	    exit(0);

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

	  case 0x18: /* NULL FUNCTIONS FOR CP/M COMPATIBILITY */
	  case 0x1d:
	  case 0x1e:
	  case 0x20:
	  case 0x2b: /* SET SYSTEM DATE */
	  case 0x2d: /* SET SYSTEM TIME */
	  case 0x37: /* "SWITCHAR" - GET SWITCH CHARACTER
			"SWITCHAR" - SET SWITCH CHARACTER
			"AVAILDEV" - SPECIFY \DEV\ PREFIX USE */
	  case 0x54: /* GET VERIFY FLAG */
	  case 0x6b: /* NULL FUNCTION */
            IntBarf(0x21, context);
	    EAX &= 0xff00;
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
			return;
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
            dta = pointer(DS, DX);
            break;

	  case 0x1b: /* GET ALLOCATION INFORMATION FOR DEFAULT DRIVE */
	    GetDefDriveAllocInfo(context);
	    break;
	
	  case 0x1c: /* GET ALLOCATION INFORMATION FOR SPECIFIC DRIVE */
	    GetDriveAllocInfo(context);
	    break;

	  case 0x1f: /* GET DRIVE PARAMETER BLOCK FOR DEFAULT DRIVE */
	    GetDrivePB(context);
	    break;
		
	  case 0x25: /* SET INTERRUPT VECTOR */
	    /* Ignore any attempt to set a segment vector */
 		dprintf_int(stdnimp,
			"int21: set interrupt vector %2x (%04x:%04x)\n",
			AL, DS, DX);
            break;

	  case 0x2a: /* GET SYSTEM DATE */
	    GetSystemDate(context);
            break;

	  case 0x2c: /* GET SYSTEM TIME */
	    GetSystemTime(context);
	    break;

	  case 0x2f: /* GET DISK TRANSFER AREA ADDRESS */
            ES = segment(dta);
            BX = offset(dta);
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
	    GetDrivePB(context);
	    break;

	  case 0x33: /* MULTIPLEXED */
	    switch (AL) {
	      case 0x00: /* GET CURRENT EXTENDED BREAK STATE */
		if (!(AL)) 
		    EDX &= 0xff00L;
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
		EBX = DOSVERSION;
		EDX = 0x00;
		break;

	      default:
		IntBarf(0x21, context);
		break;			
	    }
	    break;	
	    
	  case 0x34: /* GET ADDRESS OF INDOS FLAG */
		ES = segment(heap->InDosFlag);
		BX = offset(heap->InDosFlag);
	    break;

	  case 0x35: /* GET INTERRUPT VECTOR */
	    /* Return a NULL segment selector - this will bomb, 
	    		if anyone ever tries to use it */
	    dprintf_int(stdnimp, "int21: get interrupt vector %2x\n",
		AX & 0xff);
	    ES = 0;
	    BX = 0;
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
		if (unlink( GetUnixFileName( pointer(DS,DX)) ) == -1) {
			errno_to_doserr();
			AL = ExtendedError;
			SetCflag;
			return;
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
		   
	      case 0x09:   /* CHECK IF BLOCK DEVICE REMOTE */
		EDX = (EDX & 0xffff0000) | (1<<9) | (1<<12) | (1<<15);
		ResetCflag;
		break;

	      case 0x0b:   /* SET SHARING RETRY COUNT */
		if (!CX)
		{ 
		  EAX = (EAX & 0xffff0000) | 0x0001;
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
                
	      default:
                IntBarf(0x21, context);
		break;
	    }
	    break;

	  case 0x45: /* "DUP" - DUPLICATE FILE HANDLE */
	  case 0x46: /* "DUP2", "FORCEDUP" - FORCE DUPLICATE FILE HANDLE */
		AX = dup(BX);
		ResetCflag;
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
	    ExecProgram(context);
	    break;		
	
	  case 0x4c: /* "EXIT" - TERMINATE WITH RETURN CODE */
	    exit(AL);
	    break;

	  case 0x4d: /* GET RETURN CODE */
	    AL = NoError; /* normal exit */
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
		AL = 0x01L;
		break;
	      case 0x02:
		EAX &= 0xff00L;
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
	    AL = NoNetwork;
	    SetCflag;
	    break;
	
	  case 0x5f: /* NETWORK */
	    switch (AL) 
	    {
	      case 0x07: /* ENABLE DRIVE */
		if (!DOS_EnableDrive(DL)) 
		{
		    Error(InvalidDrive, EC_MediaError , EL_Disk);
		    AL = InvalidDrive;
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
		    AL = InvalidDrive;
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
		AL = NoNetwork; 
		SetCflag;
		break;
	    }
	    break;

	  case 0x60: /* "TRUENAME" - CANONICALIZE FILENAME OR PATH */
		strncpy(pointer(ES,DI), pointer(DS,SI), strlen(pointer(DS,SI)) & 0x7f);
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

	  case 0x6a: /* COMMIT FILE */
	    ResetCflag;
	    break;		

	  case 0xea: /* NOVELL NETWARE - RETURN SHELL VERSION */
	    break;

	  default:
            IntBarf(0x21, context);
	    return 1;
	}
    }
    return 1;
}

/**********************************************************************
 *			DOS3Call
 */
void DOS3Call()
{
    do_int21((struct sigcontext_struct *) _CONTEXT);
    ReturnFromRegisterFunc();
}

void INT21_Init(void)
{
	int handle;
	MDESC *DosHeapDesc;

	if ((handle = GlobalAlloc(GMEM_FIXED,sizeof(struct DosHeap))) == 0)
		myerror("out of memory");

	heap = (struct DosHeap *) GlobalLock(handle);
	HEAP_Init(&DosHeapDesc, heap, sizeof(struct DosHeap));

	dta = heap->dta;
	heap->InDosFlag = 0;
	strcpy(heap->biosdate, "01/01/80");
}
