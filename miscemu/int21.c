#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>

#include "prototypes.h"
#include "regfunc.h"
#include "windows.h"
#include "wine.h"
#include "msdos.h"
#include "options.h"

static char Copyright[] = "copyright Erik Bos, 1993";

WORD ExtendedError, CodePage = 437;
BYTE ErrorClass, Action, ErrorLocus;
BYTE *dta;

extern char TempDirectory[];

static int Error(int e, int class, int el)
{
	ErrorClass = class;
	Action = SA_Ask4Retry;
	ErrorLocus = el;
	ExtendedError = e;

	return e;
}

static void Barf(struct sigcontext_struct *context)
{
	fprintf(stderr, "int21: unknown/not implemented parameters:\n");
	fprintf(stderr, "int21: AX %04x, BX %04x, CX %04x, DX %04x, "
	       "SI %04x, DI %04x, DS %04x, ES %04x\n",
	       AX, BX, CX, DX, SI, DI, DS, ES);
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

	if (!(EDX & 0xffL))
		drive = DOS_GetDefaultDrive();
	else
		drive = (EDX & 0xffL) - 1;
	
	if (!DOS_ValidDrive(drive)) {
		Error(InvalidDrive, EC_MediaError , EL_Disk);
		EAX |= 0xffffL;	
		return;
	}
	
	if (!DOS_GetFreeSpace(drive, &size, &available)) {
		Error(GeneralFailure, EC_MediaError , EL_Disk);
		EAX |= 0xffffL;
		return;
	}

	EAX = (EAX & 0xffff0000L) | 4;	
	ECX = (ECX & 0xffff0000L) | 512;

	EBX = (EBX & 0xffff0000L) | (available / (CX * AX));
	EDX = (EDX & 0xffff0000L) | (size / (CX * AX));
	Error (0,0,0);
}

static void SetDefaultDrive(struct sigcontext_struct *context)
{
	int drive;

	drive = EDX & 0xffL;

	if (!DOS_ValidDrive(drive)) {
		Error (InvalidDrive, EC_MediaError, EL_Disk);
		return;
	} else {
		DOS_SetDefaultDrive(drive);
		EAX = (EAX &0xffffff00L) | MAX_DOS_DRIVES; 
		Error (0,0,0);
	}
}

static void GetDefaultDrive(struct sigcontext_struct *context)
{
	EAX = (EAX & 0xffffff00L) | DOS_GetDefaultDrive();
	Error (0,0,0);
}

static void GetDriveAllocInfo(struct sigcontext_struct *context)
{
	int drive;
	long size, available;
	BYTE mediaID;
	
	drive = EDX & 0xffL;
	
	if (!DOS_ValidDrive(drive)) {
		EAX = (EAX & 0xffff0000L) | 4;
		ECX = (ECX & 0xffff0000L) | 512;
		EDX = (EDX & 0xffff0000L);
		Error (InvalidDrive, EC_MediaError, EL_Disk);
		return;
	}

	if (!DOS_GetFreeSpace(drive, &size, &available)) {
		Error(GeneralFailure, EC_MediaError , EL_Disk);
		EAX |= 0xffffL;
		return;
	}
	
	EAX = (EAX & 0xffff0000L) | 4;	
	ECX = (ECX & 0xffff0000L) | 512;
	EDX = (EDX & 0xffff0000L) | (size / (CX * AX));

	mediaID = 0xf0;

	DS = segment(mediaID);
	EBX = offset(mediaID);	
	Error (0,0,0);
}

static void GetDefDriveAllocInfo(struct sigcontext_struct *context)
{
	EDX = DOS_GetDefaultDrive();
	GetDriveAllocInfo(context);
}

static void GetDrivePB(struct sigcontext_struct *context)
{
	Error (InvalidDrive, EC_MediaError, EL_Disk);
	EAX = (EAX & 0xffff0000L) | 0xffL; 
		/* I'm sorry but I only got networked drives :-) */
}

static void ReadFile(struct sigcontext_struct *context)
{
	char *ptr;
	int size;

	/* can't read from stdout / stderr */

	if ((BX == 1) || (BX == 2)) {
		Error (InvalidHandle, EL_Unknown, EC_Unknown);
		EAX = (EAX & 0xffff0000L) | InvalidHandle;
		SetCflag;
		return;
	}

	ptr = pointer (DS,DX);

	if (BX == 0) {
		*ptr = EOF;
		Error (0,0,0);
		EAX = (EAX & 0xffff0000L) | 1;
		ResetCflag;
		return;
	} else {
		size = read(BX, ptr, CX);
		if (size == 0) {
			Error (ReadFault, EC_Unknown, EL_Unknown);
			EAX = (EAX & 0xffffff00L) | ExtendedError;
			return;
		}

		if (size == -1) {
			switch (errno) {
				case EAGAIN:
					Error (ShareViolation, EC_Temporary, EL_Unknown);
					break;
				case EBADF:
					Error (InvalidHandle, EC_AppError, EL_Unknown);
					break;
				default:
					Error (GeneralFailure, EC_SystemFailure, EL_Unknown);
					break;
			}
			EAX = (EAX & 0xffffff00L) | ExtendedError;
			SetCflag;
			return;
		}		
		Error (0,0,0);
		EAX = (EAX & 0xffff0000L) | size;
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
			fprintf(stderr, "%c", *ptr++);
		}
		fflush(stderr);

		Error (0,0,0);
		EAX = (EAX & 0xffffff00L) | CX;
		ResetCflag;
	} else {
		size = write(BX, ptr , CX);
		if (size == 0) {
			Error (WriteFault, EC_Unknown, EL_Unknown);
			EAX = (EAX & 0xffffff00L) | ExtendedError;
			return;
		}

		if (size == -1) {
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
				default:
					Error (GeneralFailure, EC_SystemFailure, EL_Unknown);
					break;
			}
			EAX = (EAX & 0xffffff00L) | ExtendedError;
			SetCflag;
			return;
		}		
		Error (0,0,0);
		EAX = (EAX & 0xffff0000L) | size;
		ResetCflag;
	}
}

static void UnlinkFile(struct sigcontext_struct *context)
{
	if (unlink( GetUnixFileName( pointer(DS,DX)) ) == -1) {
		switch (errno) {
			case EACCES:
			case EPERM:
			case EROFS:
				Error (WriteProtected, EC_AccessDenied, EL_Unknown);
				break;
			case EBUSY:
				Error (LockViolation, EC_AccessDenied, EL_Unknown);
				break;		
			case EAGAIN:
				Error (ShareViolation, EC_Temporary, EL_Unknown);
				break;
			case ENOENT:
				Error (FileNotFound, EC_NotFound, EL_Unknown);
				break;				
			default:
				Error (GeneralFailure, EC_SystemFailure, EL_Unknown);
				break;
		}
		EAX = (EAX & 0xffffff00L) | ExtendedError;			SetCflag;
		return;
	}		
	Error (0,0,0);
	ResetCflag;
}

static void SeekFile(struct sigcontext_struct *context)
{
	int status, fileoffset;
	
	switch (EAX & 0xffL) {
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
		switch (errno) {
			case EBADF:
				Error (InvalidHandle, EC_AppError, EL_Unknown);
				break;
			case EINVAL:
				Error (DataInvalid, EC_AppError, EL_Unknown);
				break;
			default:
				Error (GeneralFailure, EC_SystemFailure, EL_Unknown);
				break;
		}
		EAX = (EAX & 0xffffff00L) | ExtendedError;			SetCflag;
		return;
	}		
	Error (0,0,0);
	ResetCflag;
}

static void GetFileAttributes(struct sigcontext_struct *context)
{
	EAX &= 0xfffff00L;
	ResetCflag;
}

static void SetFileAttributes(struct sigcontext_struct *context)
{
	ResetCflag;
}

static void ioctlGetDeviceInfo(struct sigcontext_struct *context)
{
	WORD handle = EBX & 0xffff;

	switch (handle) {
		case 0:
		case 1:
		case 2:
			EDX = (EDX & 0xffff0000) | 0x80d3;
			break;

		default:
			Barf(context);
			EDX = (EDX & 0xffff0000) | 0x50;
	}
	ResetCflag;
}

static void ioctlGenericBlkDevReq(struct sigcontext_struct *context)
{
	BYTE *dataptr = pointer(DS, DX);
	int drive;

	if (!(EBX & 0xffL))
		drive = DOS_GetDefaultDrive();
	else
		drive = (EBX & 0xffL) - 1;

	if ((ECX & 0xff00L) != 0x0800) {
		Barf(context);
		return;
	}
	switch (ECX & 0xffL) {
		case 0x60: /* get device parameters */
			   /* used by w4wgrp's winfile */
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
			EAX = (EAX & 0xfffff00L);
			ResetCflag;
			return;
		default:
			Barf(context);
	}
}

static void DupeFileHandle(struct sigcontext_struct *context)
{
	EAX = (EAX & 0xffff0000L) | dup(BX);
	ResetCflag;
}

static void GetSystemDate(struct sigcontext_struct *context)
{
	struct tm *now;
	time_t ltime;

	ltime = time(NULL);
	now = localtime(&ltime);

	ECX = (ECX & 0xffff0000L) | (now->tm_year + 1900);
	EDX = (EDX & 0xffff0000L) | ((now->tm_mon + 1) << 8) | now->tm_mday;
	EAX = (EAX & 0xffff0000L) | now->tm_wday;
}

static void GetSystemTime(struct sigcontext_struct *context)
{
	struct tm *now;
	time_t ltime;

	ltime = time(NULL);
	now = localtime(&ltime);
	 
	ECX = (ECX & 0xffffff00L) | (now->tm_hour << 8) | now->tm_min;
	EDX = (EDX & 0xffffff00L) | now->tm_sec << 8;
}

static void GetExtendedErrorInfo(struct sigcontext_struct *context)
{
	EAX = (EAX & 0xffffff00L) | ExtendedError;
	EBX = (EBX & 0xffff0000L) | (0x100 * ErrorClass) | Action;
	ECX = (ECX & 0xffff00ffL) | (0x100 * ErrorLocus);
}

static void GetInDosFlag(struct sigcontext_struct *context)
{
	const BYTE InDosFlag = 0;
	
	ES  = (ES & 0xffff0000L) | segment(InDosFlag);
	EBX = (EBX & 0xffff0000L) | offset(InDosFlag);
}

static void CreateFile(struct sigcontext_struct *context)
{
	int handle;

	if ((handle = open(GetUnixFileName( pointer(DS,DX)), O_CREAT | O_TRUNC)) == -1) {
		switch (errno) {
			case EACCES:
			case EPERM:
			case EROFS:
				Error (WriteProtected, EC_AccessDenied, EL_Unknown);
				break;
			case EISDIR:
				Error (CanNotMakeDir, EC_AccessDenied, EL_Unknown);
				break;
			case ENFILE:
			case EMFILE:
				Error (NoMoreFiles, EC_MediaError, EL_Unknown);
			case EEXIST:
				Error (FileExists, EC_Exists, EL_Disk);
				break;				
			case ENOSPC:
				Error (DiskFull, EC_MediaError, EL_Disk);
				break;				
			default:
				Error (GeneralFailure, EC_SystemFailure, EL_Unknown);
				break;
			}			
		EAX = (EAX & 0xffffff00L) | ExtendedError;
		SetCflag;
		return;
		}			
	Error (0,0,0);
	EBX = (EBX & 0xffff0000L) | handle;
	EAX = (EAX & 0xffffff00L) | NoError;
	ResetCflag;
}

static void OpenExistingFile(struct sigcontext_struct *context)
{
	int handle;

	fprintf(stderr,"OpenExistingFile (%s)\n", pointer(DS,DX));
	
	if ((handle = open(GetUnixFileName(pointer(DS,DX)), O_RDWR)) == -1) {
		switch (errno) {
			case EACCES:
			case EPERM:
			case EROFS:
				Error (WriteProtected, EC_AccessDenied, EL_Unknown);
				break;
			case EISDIR:
				Error (CanNotMakeDir, EC_AccessDenied, EL_Unknown);
				break;
			case ENFILE:
			case EMFILE:
				Error (NoMoreFiles, EC_MediaError, EL_Unknown);
			case EEXIST:
				Error (FileExists, EC_Exists, EL_Disk);
				break;				
			case ENOSPC:
				Error (DiskFull, EC_MediaError, EL_Disk);
				break;				
			case ENOENT:
				Error (FileNotFound, EC_MediaError, EL_Disk);
				break;
			default:
				Error (GeneralFailure, EC_SystemFailure, EL_Unknown);
				break;
		}
		EAX = (EAX & 0xffffff00L) | ExtendedError;
		SetCflag;
		return;
	}		
	Error (0,0,0);
	EBX = (EBX & 0xffff0000L) | handle;
	EAX = (EAX & 0xffffff00L) | NoError;
	ResetCflag;
}

static void CloseFile(struct sigcontext_struct *context)
{
	if (close(BX) == -1) {
		switch (errno) {
			case EBADF:
				Error (InvalidHandle, EC_AppError, EL_Unknown);
				break;
			default:
				Error (GeneralFailure, EC_SystemFailure, EL_Unknown);
				break;
		}
		EAX = (EAX & 0xffffff00L) | ExtendedError;
		SetCflag;
		return;
	}		
	Error (0,0,0);
	EAX = (EAX & 0xffffff00L) | NoError;
	ResetCflag;
}

static void RenameFile(struct sigcontext_struct *context)
{
	char *newname, *oldname;

	fprintf(stderr,"int21: renaming %s to %s\n", 
			pointer(DS,DX), pointer(ES,DI) );
	
	oldname = GetUnixFileName( pointer(DS,DX) );
	newname = GetUnixFileName( pointer(ES,DI) );

	rename( oldname, newname);
	ResetCflag;
}

static void GetTrueFileName(struct sigcontext_struct *context)
{
	fprintf(stderr,"int21: GetTrueFileName of %s\n", pointer(DS,SI) );
	
	strncpy(pointer(ES,DI), pointer(DS,SI), strlen(pointer(DS,SI)) & 0x7f);
	ResetCflag;
}

static void MakeDir(struct sigcontext_struct *context)
{
	char *dirname;

	fprintf(stderr,"int21: makedir %s\n", pointer(DS,DX) );
	
	if ((dirname = GetUnixFileName( pointer(DS,DX) ))== NULL) {
		EAX = (EAX & 0xffffff00L) | CanNotMakeDir;
		SetCflag;
		return;
	}

	if (mkdir(dirname,0) == -1) {
		EAX = (EAX & 0xffffff00L) | CanNotMakeDir;
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
	fprintf(stderr,"int21: changedir %s\n", dirname);
	if (dirname != NULL && dirname[1] == ':') {
		drive = toupper(dirname[0]) - 'A';
		dirname += 2;
		}
	DOS_ChangeDir(drive, dirname);
}

static void RemoveDir(struct sigcontext_struct *context)
{
	char *dirname;

	fprintf(stderr,"int21: removedir %s\n", pointer(DS,DX) );

	if ((dirname = GetUnixFileName( pointer(DS,DX) ))== NULL) {
		EAX = (EAX & 0xffffff00L) | CanNotMakeDir;
		SetCflag;
		return;
	}

/*
	if (strcmp(unixname,DosDrives[drive].CurrentDirectory)) {
		EAX = (EAX & 0xffffff00L) | CanNotRemoveCwd;
		SetCflag;
	}
*/	
	if (rmdir(dirname) == -1) {
		EAX = (EAX & 0xffffff00L) | CanNotMakeDir; 
		SetCflag;
	} 
	ResetCflag;
}

static void AllocateMemory(struct sigcontext_struct *context)
{
	char *ptr;
	
	fprintf(stderr,"int21: malloc %d bytes\n", BX * 0x10 );
	
/*	if ((ptr = (void *) memalign((size_t) (BX * 0x10), 0x10)) == NULL) {
		EAX = (EAX & 0xffffff00L) | OutOfMemory;
		EBX = (EBX & 0xffffff00L);
		SetCflag;
	}
	fprintf(stderr,"int21: malloc (ptr = %d)\n", ptr );

	EAX = (EAX & 0xffff0000L) | segment(ptr);
*/
	Barf(context);
	SetCflag;
}

static void FreeMemory(struct sigcontext_struct *context)
{
	fprintf(stderr,"int21: freemem (ptr = %d)\n", ES * 0x10 );
/*
	free((void *)(ES * 0x10));
*/	Barf(context);
	ResetCflag;
}

static void ResizeMemoryBlock(struct sigcontext_struct *context)
{
	char *ptr;

	fprintf(stderr,"int21: realloc (ptr = %d)\n", ES * 0x10 );
	
/*	if ((ptr = (void *) realloc((void *)(ES * 0x10), (size_t) BX * 0x10)) == NULL) {
		EAX = (EAX & 0xffffff00L) | OutOfMemory;
		EBX = (EBX & 0xffffff00L);
		SetCflag;
	}
	EBX = (EBX & 0xffff0000L) | segment(ptr);
*/	Barf(context);
	SetCflag;
}

static void ExecProgram(struct sigcontext_struct *context)
{
	execl("wine", GetUnixFileName( pointer(DS,DX)) );
}

static void GetReturnCode(struct sigcontext_struct *context)
{
	EAX = (EAX & 0xffffff00L) | NoError; /* normal exit */
}

static void FindNext(struct sigcontext_struct *context)
{
	struct dosdirent *dp;
	
	dp = (struct dosdirent *)(dta + 0x0d);

	do {
		if ((dp = DOS_readdir(dp)) == NULL) {
			Error(NoMoreFiles, EC_MediaError , EL_Disk);
			EAX = (EAX & 0xffffff00L) | NoMoreFiles;
			SetCflag;
			return;
		}
	} while (*(dta + 0x0c) != dp->attribute);
				
	setword(&dta[0x16], 0x1234); /* time */
	setword(&dta[0x18], 0x1234); /* date */
	setdword(&dta[0x1a], dp->filesize);
	strncpy(dta + 0x1e, dp->filename, 13);

	EAX = (EAX & 0xffffff00L);
	ResetCflag;
	return;
}

static void FindFirst(struct sigcontext_struct *context)
{
	BYTE drive, *path = pointer(DS, DX);
	struct dosdirent *dp;

	if (path[1] == ':') {
		drive = (islower(*path) ? toupper(*path) : *path) - 'A';

		if (!DOS_ValidDrive(drive)) {
			Error(InvalidDrive, EC_MediaError , EL_Disk);
			EAX = (EAX & 0xffffff00L) | InvalidDrive;
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

		EAX = (EAX & 0xffffff00L);
		ResetCflag;
		return;
	}

	if ((dp = DOS_opendir(path)) == NULL) {
		Error(PathNotFound, EC_MediaError, EL_Disk);
		EAX = (EAX & 0xffffff00L) | FileNotFound;
		SetCflag;
		return;
	}
/*	strncpy(dta + 1, AsciizToFCB(dp->filemask), 11); */

/*	*((BYTE *) ((void*)dta + 0x0d)) = (BYTE *) dp; */

	*((struct dosdirent *)(dta + 0x0d))
	 =
	  *dp;
	FindNext(context);
}

static void GetSysVars(struct sigcontext_struct *context)
{
	/* return a null pointer, to encourage anyone who tries to
	   use the pointer */

	ES = 0x0;
	EBX = (EBX & 0xffff0000L);
	Barf(context);
}

static void GetFileDateTime(struct sigcontext_struct *context)
{
	char *filename;
	struct stat filestat;
	struct tm *now;

	if ((filename = GetUnixFileName( pointer(DS,DX) ))== NULL) {
		EAX = (EAX & 0xffffff00L) | FileNotFound;
		SetCflag;
		return;
	}
	stat(filename, &filestat);
	 	
	now = localtime (&filestat.st_mtime);
	
	ECX = (ECX & 0xffff0000L) | ((now->tm_hour * 0x2000) + (now->tm_min * 0x20) + now->tm_sec/2);
	EDX = (EDX & 0xffff0000L) | ((now->tm_year * 0x200) + (now->tm_mon * 0x20) + now->tm_mday);

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

	fprintf(stderr,"CreateTempFile %s\n",temp);

	handle = open(GetUnixFileName(temp), O_CREAT | O_TRUNC | O_RDWR);

	if (handle == -1) {
		EAX = (EAX & 0xffffff00L) | WriteProtected;
		SetCflag;
		return;
	}

	strcpy(pointer(DS,DX), temp);
	
	EAX = (EAX & 0xffff0000L) | handle;
	ResetCflag;
}

static void CreateNewFile(struct sigcontext_struct *context)
{
	int handle;
	
	if ((handle = open(GetUnixFileName( pointer(DS,DX) ), O_CREAT | O_TRUNC | O_RDWR)) == -1) {
		EAX = (EAX & 0xffffff00L) | WriteProtected;
		SetCflag;
		return;
	}

	EAX = (EAX & 0xffff0000L) | handle;
	ResetCflag;
}

static void FileLock(struct sigcontext_struct *context)
{
	fprintf(stderr, "int21: flock()\n");
}

static void GetExtendedCountryInfo(struct sigcontext_struct *context)
{
	ResetCflag;
}

static void GetCurrentDirectory(struct sigcontext_struct *context)
{
	int drive;

	if ((EDX & 0xffL) == 0)
		drive = DOS_GetDefaultDrive();
	else
		drive = (EDX & 0xffL)-1;

	if (!DOS_ValidDrive(drive)) {
		EAX = (EAX & 0xffffff00L) | InvalidDrive;
		SetCflag;
		return;
	}

	strcpy(pointer(DS,SI), DOS_GetCurrentDir(drive) );
	ResetCflag;
}

static void GetCurrentPSP(struct sigcontext_struct *context)
{
	Barf(context);
}

static void GetDiskSerialNumber(struct sigcontext_struct *context)
{
	int drive;
	BYTE *dataptr = pointer(DS, DX);
	DWORD serialnumber;
	
	if ((EBX & 0xffL) == 0)
		drive = DOS_GetDefaultDrive();
	else
		drive = (EBX & 0xffL) - 1;

	if (!DOS_ValidDrive(drive)) {
		EAX = (EAX & 0xffffff00L) |InvalidDrive;
		SetCflag;
		return;
	}

	DOS_GetSerialNumber(drive, &serialnumber);

	setword(dataptr, 0);
	setdword(&dataptr[2], serialnumber);
	strncpy(dataptr + 6, DOS_GetVolumeLabel(drive), 8);
	strncpy(dataptr + 0x11, "FAT16   ", 8);
	
	EAX = (EAX & 0xffffff00L);
	ResetCflag;
}

static void SetDiskSerialNumber(struct sigcontext_struct *context)
{
	int drive;
	BYTE *dataptr = pointer(DS, DX);
	DWORD serialnumber;

	if ((EBX & 0xffL) == 0)
		drive = DOS_GetDefaultDrive();
	else
		drive = (EBX & 0xffL) - 1;

	if (!DOS_ValidDrive(drive)) {
		EAX = (EAX & 0xffffff00L) | InvalidDrive;
		SetCflag;
		return;
	}

	serialnumber = dataptr[1] + (dataptr[2] << 8) + (dataptr[3] << 16) + 
			(dataptr[4] << 24);

	DOS_SetSerialNumber(drive, serialnumber);
	EAX = (EAX & 0xffffff00L) | 1L;
	ResetCflag;
}

static void DumpFCB(BYTE *fcb)
{
	int x, y;

	fcb -= 7;

	for (y = 0; y !=2 ; y++) {
		for (x = 0; x!=15;x++)
			fprintf(stderr, "%02x ", *fcb++);
		fprintf(stderr,"\n");
	}
}

/* microsoft's programmers should be shot for using CP/M style int21
   calls in Windows for Workgroup's winfile.exe */

static void FindFirstFCB(struct sigcontext_struct *context)
{
	BYTE *fcb = pointer(DS, DX);
	int drive;

	DumpFCB( fcb );

	if (*fcb)
		drive = *fcb - 1;
	else
		drive = DOS_GetDefaultDrive();

	if (*(fcb - 7) == 0xff) {
		if (*(fcb - 1) == FA_DIREC) {
			/* return volume label */

			memset(dta, ' ', 11);
			if (DOS_GetVolumeLabel(drive) != NULL) 
				strncpy(dta, DOS_GetVolumeLabel(drive), 8);
			*(dta + 0x0b) = FA_DIREC;

			EAX = (EAX & 0xffffff00L);
			return;
		}
	}
	Barf(context);
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
		EAX = (EAX & 0xffffff00L) | 0xffL;
		return;
	}

	strcpy(temp, DOS_GetCurrentDir(drive) );
	strcat(temp, "\\");
	
	ptr = temp + strlen(temp);
	
	while (DOS_readdir(dp) != NULL)
	{
		strcpy(ptr, dp->filename);
		fprintf(stderr, "int21: delete file %s\n", temp);
		/* unlink(GetUnixFileName(temp)); */
	}
	DOS_closedir(dp);
	EAX = (EAX & 0xffffff00L);
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
		EAX = (EAX & 0xffffff00L) | 0xffL;
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
		fprintf(stderr, "int21: renamefile %s -> %s\n", oldname, newname);
	}
	DOS_closedir(dp);
	EAX = (EAX & 0xffffff00L);
}

/************************************************************************/

int do_int21(struct sigcontext_struct * context)
{
    int ah;

    if (Options.relay_debug)
    {
	printf("int21: AX %04x, BX %04x, CX %04x, DX %04x, "
	       "SI %04x, DI %04x, DS %04x, ES %04x\n",
	       AX, BX, CX, DX, SI, DI, DS, ES);
    }

    ah = (EAX >> 8) & 0xffL;

    if (ah == 0x59) 
    {
	GetExtendedErrorInfo(context);
	return 1;
    } 
    else 
    {
	Error (0,0,0);
	switch(ah) 
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
	    Barf(context);
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
	  case 0x61: /* UNUSED */
	  case 0x6b: /* NULL FUNCTION */
            Barf(context);
	    EAX &= 0xff00;
	    break;
	
	  case 0x0d: /* DISK BUFFER FLUSH */
            ResetCflag; /* dos 6+ only */
            break;

	  case 0x0e: /* SELECT DEFAULT DRIVE */
	    SetDefaultDrive(context);
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
	    GetDefaultDrive(context);
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
 		fprintf(stderr, "int21: set interrupt vector %2x (%04x:%04x)\n", AX & 0xff, DS, DX);
            break;

	  case 0x2a: /* GET SYSTEM DATE */
	    GetSystemDate(context);
            break;

	  case 0x2c: /* GET SYSTEM TIME */
	    GetSystemTime(context);
	    break;

	  case 0x2f: /* GET DISK TRANSFER AREA ADDRESS */
            ES = segment(dta);
            EBX = (EBX & 0xffff0000L) | offset(dta);
            break;
            
	  case 0x30: /* GET DOS VERSION */
	    EAX = (EAX & 0xffff0000L) | DOSVERSION;
	    EBX = (EBX & 0xffff0000L) | 0x0012;     /* 0x123456 is Wine's serial # */
	    ECX = (ECX & 0xffff0000L) | 0x3456;
	    break;

	  case 0x31: /* TERMINATE AND STAY RESIDENT */
            Barf(context);
	    break;

	  case 0x32: /* GET DOS DRIVE PARAMETER BLOCK FOR SPECIFIC DRIVE */
	    GetDrivePB(context);
	    break;

	  case 0x33: /* MULTIPLEXED */
	    switch (EAX & 0xff) {
	      case 0x00: /* GET CURRENT EXTENDED BREAK STATE */
		if (!(EAX & 0xffL)) 
		    EDX &= 0xff00L;
		break;

	      case 0x01: /* SET EXTENDED BREAK STATE */
		break;		
		
	      case 0x02: /* GET AND SET EXTENDED CONTROL-BREAK CHECKING STATE*/
		EDX &= 0xff00L;
		break;

	      case 0x05: /* GET BOOT DRIVE */
		EDX = (EDX & 0xff00L) | 2;
		/* c: is Wine's bootdrive */
		break;
				
	      case 0x06: /* GET TRUE VERSION NUMBER */
		EBX = DOSVERSION;
		EDX = 0x00;
		break;

	      default:
		Barf(context);
		break;			
	    }
	    break;	
	    
	  case 0x34: /* GET ADDRESS OF INDOS FLAG */
	    GetInDosFlag(context);
	    break;

	  case 0x35: /* GET INTERRUPT VECTOR */
	    /* Return a NULL segment selector - this will bomb, 
	    		if anyone ever tries to use it */
	    fprintf(stderr, "int21: get interrupt vector %2x\n", AX & 0xff);
	    ES = 0;
	    EBX = 0;
	    break;

	  case 0x36: /* GET FREE DISK SPACE */
	    GetFreeDiskSpace(context);
	    break;

	  case 0x38: /* GET COUNTRY-SPECIFIC INFORMATION */
	    EAX &= 0xff00;
	    EAX |= 0x02; /* no country support available */
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
	    UnlinkFile(context);
	    break;
	
	  case 0x42: /* "LSEEK" - SET CURRENT FILE POSITION */
	    SeekFile(context);
	    break;
	    
	  case 0x43: /* FILE ATTRIBUTES */
	    switch (EAX & 0xffL) 
	    {
	      case 0x00:
		GetFileAttributes(context);
		break;
	      case 0x01:
		SetFileAttributes(context);
		break;
	    }
	    break;
		
	  case 0x44: /* IOCTL */
	    switch (EAX & 0xff) 
	    {
              case 0x00:
                ioctlGetDeviceInfo(context);
		break;
		                
              case 0x0d:
                ioctlGenericBlkDevReq(context);
                break;
                
	      default:
                Barf(context);
		break;
	    }
	    break;


	  case 0x45: /* "DUP" - DUPLICATE FILE HANDLE */
	  case 0x46: /* "DUP2", "FORCEDUP" - FORCE DUPLICATE FILE HANDLE */
	    DupeFileHandle(context);
	    break;

	  case 0x47: /* "CWD" - GET CURRENT DIRECTORY */	
	    GetCurrentDirectory(context);
	    EAX = (EAX & 0xffff0000L) | 0x0100; 
	    /* intlist: many Microsoft products for Windows rely on this */
	    break;
	
	  case 0x48: /* ALLOCATE MEMORY */
	    AllocateMemory(context);
	    break;
	
	  case 0x49: /* FREE MEMORY */
	    FreeMemory(context);
	    break;
	
	  case 0x4a: /* RESIZE MEMORY BLOCK */
	    ResizeMemoryBlock(context);
	    break;
	
	  case 0x4b: /* "EXEC" - LOAD AND/OR EXECUTE PROGRAM */
	    ExecProgram(context);
	    break;		
	
	  case 0x4c: /* "EXIT" - TERMINATE WITH RETURN CODE */
	    exit(EAX & 0xffL);
	    break;
		
	  case 0x4d: /* GET RETURN CODE */
	    GetReturnCode(context);
	    break;

	  case 0x4e: /* "FINDFIRST" - FIND FIRST MATCHING FILE */
	    FindFirst(context);
	    break;

	  case 0x4f: /* "FINDNEXT" - FIND NEXT MATCHING FILE */
	    FindNext(context);
	    break;
			
	  case 0x52: /* "SYSVARS" - GET LIST OF LISTS */
	    GetSysVars(context);
	    break;
		
	  case 0x56: /* "RENAME" - RENAME FILE */
	    RenameFile(context);
	    break;
	
	  case 0x57: /* FILE DATE AND TIME */
	    switch (EAX & 0xffL) 
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
	    switch (EAX & 0xffL) 
	    {
	      case 0x00:
		EAX = (EAX & 0xffffff00L) | 0x01L;
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
	
	  case 0x5c: /* "FLOCK" - RECORD LOCKING */
	    FileLock(context);
	    break;	

	  case 0x5d: /* NETWORK */
	  case 0x5e:
	    /* network software not installed */
	    EAX = (EAX & 0xfffff00L) | NoNetwork;
	    SetCflag;
	    break;
	
	  case 0x5f: /* NETWORK */
	    switch (EAX & 0xffL) 
	    {
	      case 0x07: /* ENABLE DRIVE */
		if (!DOS_EnableDrive(EDX & 0xffL)) 
		{
		    Error(InvalidDrive, EC_MediaError , EL_Disk);
		    EAX = (EAX & 0xfffff00L) | InvalidDrive;
		    SetCflag;
		    break;
		} 
		else 
		{
		    ResetCflag;
		    break;
		}
	      case 0x08: /* DISABLE DRIVE */
		if (!DOS_DisableDrive(EDX & 0xffL)) 
		{
		    Error(InvalidDrive, EC_MediaError , EL_Disk);
		    EAX = (EAX & 0xfffff00L) | InvalidDrive;
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
		EAX = (EAX & 0xfffff00L) | NoNetwork; 
		SetCflag;
		break;
	    }
	    break;

	  case 0x60: /* "TRUENAME" - CANONICALIZE FILENAME OR PATH */
	    GetTrueFileName(context);
	    break;

	  case 0x62: /* GET CURRENT PSP ADDRESS */
	    GetCurrentPSP(context);
	    break;
	
	  case 0x65: /* GET EXTENDED COUNTRY INFORMATION */
	    GetExtendedCountryInfo(context);
	    break;
	
	  case 0x66: /* GLOBAL CODE PAGE TABLE */
	    switch (EAX & 0xffL) 
	    {
	      case 0x01:
		EBX = CodePage;
		EDX = BX;
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
	    switch (EAX & 0xffL) 
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


	  default:
            Barf(context);
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
