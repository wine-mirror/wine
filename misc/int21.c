#include <time.h>
#include <fcntl.h>
#include <errno.h>
#ifndef __STDC__
#include <malloc.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

#include "prototypes.h"
#include "regfunc.h"
#include "windows.h"
#include "wine.h"
#include "int21.h"

static char Copyright[] = "copyright Erik Bos, 1993";

WORD ExtendedError, CodePage = 437;
BYTE ErrorClass, Action, ErrorLocus;

extern char *TempDirectory;

void Error(int e, int class, int el)
{
	ExtendedError = e;
	ErrorClass = class;
	Action = SA_Ask4Retry;
	ErrorLocus = el;
}

void GetFreeDiskSpace(struct sigcontext_struct *context)
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

	EAX = (EAX & 0xffff0000L) | SectorsPerCluster;	
	ECX = (ECX & 0xffff0000L) | SectorSize;

	EBX = (EBX & 0xffff0000L) | (available / (CX * AX));
	EDX = (EDX & 0xffff0000L) | (size / (CX * AX));
	Error (0,0,0);
}

void SetDefaultDrive(struct sigcontext_struct *context)
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

void GetDefaultDrive(struct sigcontext_struct *context)
{
	EAX = (EAX & 0xffffff00L) | DOS_GetDefaultDrive();
	Error (0,0,0);
}

void GetDriveAllocInfo(struct sigcontext_struct *context)
{
	int drive;
	long size, available;
	BYTE mediaID;
	
	drive = EDX & 0xffL;
	
	if (!DOS_ValidDrive(drive)) {
		EAX = (EAX & 0xffff0000L) | SectorsPerCluster;	
		ECX = (ECX & 0xffff0000L) | SectorSize;
		EDX = (EDX & 0xffff0000L);
		Error (InvalidDrive, EC_MediaError, EL_Disk);
		return;
	}

	if (!DOS_GetFreeSpace(drive, &size, &available)) {
		Error(GeneralFailure, EC_MediaError , EL_Disk);
		EAX |= 0xffffL;
		return;
	}
	
	EAX = (EAX & 0xffff0000L) | SectorsPerCluster;	
	ECX = (ECX & 0xffff0000L) | SectorSize;
	EDX = (EDX & 0xffff0000L) | (size / (CX * AX));

	mediaID = 0xf0;

	DS = segment(mediaID);
	EBX = offset(mediaID);	
	Error (0,0,0);
}

void GetDefDriveAllocInfo(struct sigcontext_struct *context)
{
	EDX = DOS_GetDefaultDrive();
	GetDriveAllocInfo(context);
}

void GetDrivePB(struct sigcontext_struct *context)
{
	Error (InvalidDrive, EC_MediaError, EL_Disk);
	EAX = (EAX & 0xffff0000L) | 0xffL; 
		/* I'm sorry but I only got networked drives :-) */
}

void ReadFile(struct sigcontext_struct *context)
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

	ptr = (char *) pointer (DS,DX);

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

void WriteFile(struct sigcontext_struct *context)
{
	char *ptr;
	int x,size;
	
	ptr = (char *) pointer (DS,DX);
	
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

void UnlinkFile(struct sigcontext_struct *context)
{
	if (unlink( GetUnixFileName((char *) pointer(DS,DX)) ) == -1) {
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

void SeekFile(struct sigcontext_struct *context)
{
	int handle, status, fileoffset;
	
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

void GetFileAttributes(struct sigcontext_struct *context)
{
	EAX &= 0xfffff00L;
	ResetCflag;
}

void SetFileAttributes(struct sigcontext_struct *context)
{
	ResetCflag;
}

void DosIOCTL(struct sigcontext_struct *context)
{
	fprintf(stderr,"int21: IOCTL\n");
	EAX = (EAX & 0xfffff00L) | UnknownUnit;
	SetCflag;
}

void DupeFileHandle(struct sigcontext_struct *context)
{
	EAX = (EAX & 0xffff0000L) | dup(BX);
	ResetCflag;
}

void GetSystemDate(struct sigcontext_struct *context)
{
	struct tm *now;
	time_t ltime;

	ltime = time(NULL);
	now = localtime(&ltime);

	ECX = (ECX & 0xffff0000L) | now->tm_year + 1900;
	EDX = (EDX & 0xffff0000L) | ((now->tm_mon + 1) << 8) | now->tm_mday;
	EAX = (EAX & 0xffff0000L) | now->tm_wday;
}

void GetSystemTime(struct sigcontext_struct *context)
{
	struct tm *now;
	time_t ltime;

	ltime = time(NULL);
	now = localtime(&ltime);
	 
	ECX = (ECX & 0xffffff00L) | (now->tm_hour << 8) | now->tm_min;
	EDX = (EDX & 0xffffff00L) | now->tm_sec << 8;
}

void GetExtendedErrorInfo(struct sigcontext_struct *context)
{
	EAX = (EAX & 0xffffff00L) | ExtendedError;
	EBX = (EBX & 0xffff0000L) | (0x100 * ErrorClass) | Action;
	ECX = (ECX & 0xffff00ffL) | (0x100 * ErrorLocus);
}

void GetInDosFlag(struct sigcontext_struct *context)
{
	const BYTE InDosFlag = 0;
	
	ES  = (ES & 0xffff0000L) | segment(InDosFlag);
	EBX = (EBX & 0xffff0000L) | offset(InDosFlag);
}

void CreateFile(struct sigcontext_struct *context)
{
	int handle;

	if ((handle = open(GetUnixFileName((char *) pointer(DS,DX)), O_CREAT | O_TRUNC)) == -1) {
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

void OpenExistingFile(struct sigcontext_struct *context)
{
	int handle;

	fprintf(stderr,"OpenExistingFile (%s)\n",(char *) pointer(DS,DX));
	
	if ((handle = open(GetUnixFileName((char*) pointer(DS,DX)), O_RDWR)) == -1) {
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

void CloseFile(struct sigcontext_struct *context)
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

void RenameFile(struct sigcontext_struct *context)
{
	char *newname, *oldname;

	fprintf(stderr,"int21: renaming %s to %s\n",(char *) pointer(DS,DX), pointer(ES,DI) );
	
	oldname = GetUnixFileName( (char *) pointer(DS,DX) );
	newname = GetUnixFileName( (char *) pointer(ES,DI) );

	rename( oldname, newname);
	ResetCflag;
}

void GetTrueFileName(struct sigcontext_struct *context)
{
	fprintf(stderr,"int21: GetTrueFileName of %s\n",(char *) pointer(DS,SI));
	
	strncpy((char *) pointer(ES,DI), (char *) pointer(DS,SI), strlen((char *) pointer(DS,SI)) & 0x7f);
	ResetCflag;
}

void MakeDir(struct sigcontext_struct *context)
{
	char *dirname;

	fprintf(stderr,"int21: makedir %s\n",(char *) pointer(DS,DX) );
	
	if ((dirname = GetUnixFileName( (char *) pointer(DS,DX) ))== NULL) {
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

void ChangeDir(struct sigcontext_struct *context)
{
	fprintf(stderr,"int21: changedir %s\n",(char *) pointer(DS,DX) );

	DOS_ChangeDir(DOS_GetDefaultDrive(), (char *) pointer (DS,DX));
}

void RemoveDir(struct sigcontext_struct *context)
{
	char *dirname;

	fprintf(stderr,"int21: removedir %s\n",(char *) pointer(DS,DX) );

	if ((dirname = GetUnixFileName( (char *) pointer(DS,DX) ))== NULL) {
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

void AllocateMemory(struct sigcontext_struct *context)
{
	char *ptr;
	
	fprintf(stderr,"int21: malloc %d bytes\n", BX * 0x10 );
	
	if ((ptr = (void *) memalign((size_t) (BX * 0x10), 0x10)) == NULL) {
		EAX = (EAX & 0xffffff00L) | OutOfMemory;
		EBX = (EBX & 0xffffff00L); /* out of memory */
		SetCflag;
	}
	fprintf(stderr,"int21: malloc (ptr = %d)\n", ptr );

	EAX = (EAX & 0xffff0000L) | segment((unsigned long) ptr);
	ResetCflag;
}

void FreeMemory(struct sigcontext_struct *context)
{
	fprintf(stderr,"int21: freemem (ptr = %d)\n", ES * 0x10 );

	free((void *)(ES * 0x10));
	ResetCflag;
}

void ResizeMemoryBlock(struct sigcontext_struct *context)
{
	char *ptr;

	fprintf(stderr,"int21: realloc (ptr = %d)\n", ES * 0x10 );
	
	if ((ptr = (void *) realloc((void *)(ES * 0x10), (size_t) BX * 0x10)) == NULL) {
		EAX = (EAX & 0xffffff00L) | OutOfMemory;
		EBX = (EBX & 0xffffff00L); /* out of memory */
		SetCflag;
	}
	EBX = (EBX & 0xffff0000L) | segment((unsigned long) ptr);
	ResetCflag;
}

void ExecProgram(struct sigcontext_struct *context)
{
	execl("wine", GetUnixFileName((char *) pointer(DS,DX)) );
}

void GetReturnCode(struct sigcontext_struct *context)
{
	EAX = (EAX & 0xffffff00L) | NoError; /* normal exit */
}

void FindFirst(struct sigcontext_struct *context)
{

}

void FindNext(struct sigcontext_struct *context)
{

}

void GetSysVars(struct sigcontext_struct *context)
{
	/* return a null pointer, to encourage anyone who tries to
	   use the pointer */

	ES = 0x0;
	EBX = (EBX & 0xffff0000L);
}

void GetFileDateTime(struct sigcontext_struct *context)
{
	char *filename;
	struct stat filestat;
	struct tm *now;

	if ((filename = GetUnixFileName( (char *) pointer(DS,DX) ))== NULL) {
		EAX = (EAX & 0xffffff00L) | FileNotFound;
		SetCflag;
		return;
	}
	stat(filename, &filestat);
	 	
	now = localtime (&filestat.st_mtime);
	
	ECX = (ECX & 0xffff0000L) | (now->tm_hour * 0x2000) + (now->tm_min * 0x20) + now->tm_sec/2;
	EDX = (EDX & 0xffff0000L) | (now->tm_year * 0x200) + (now->tm_mon * 0x20) + now->tm_mday;

	ResetCflag;
}

void SetFileDateTime(struct sigcontext_struct *context)
{
	char *filename;
	struct utimbuf filetime;
	
	filename = GetUnixFileName((char *) pointer(DS,DX));

	filetime.actime = 0L;
	filetime.modtime = filetime.actime;

	utime(filename, &filetime);
	ResetCflag;
}

void CreateTempFile(struct sigcontext_struct *context)
{
	char *filename, temp[256];
	int drive, handle;
	
	sprintf(temp,"%s\\win%d.tmp",TempDirectory,(int) getpid());

	fprintf(stderr,"CreateTempFile %s\n",temp);

	handle = open(GetUnixFileName(temp), O_CREAT | O_TRUNC | O_RDWR);

	if (handle == -1) {
		EAX = (EAX & 0xffffff00L) | WriteProtected;
		SetCflag;
		return;
	}

	strcpy((char *) pointer(DS,DX), temp);
	
	EAX = (EAX & 0xffff0000L) | handle;
	ResetCflag;
}

void CreateNewFile(struct sigcontext_struct *context)
{
	int handle;
	
	if ((handle = open(GetUnixFileName((char *) pointer(DS,DX)), O_CREAT | O_TRUNC | O_RDWR)) == -1) {
		EAX = (EAX & 0xffffff00L) | WriteProtected;
		SetCflag;
		return;
	}

	EAX = (EAX & 0xffff0000L) | handle;
	ResetCflag;
}

void FileLock(struct sigcontext_struct *context)
{

}

void GetExtendedCountryInfo(struct sigcontext_struct *context)
{
	ResetCflag;
}

void GetCurrentDirectory(struct sigcontext_struct *context)
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

	DOS_GetCurrentDir(drive, (char *) pointer(DS,SI) );
	ResetCflag;
}

void GetCurrentPSP(struct sigcontext_struct *context)
{

}

void GetDiskSerialNumber(struct sigcontext_struct *context)
{
	int drive;
	unsigned long serialnumber;
	struct diskinfo *ptr;
	
	if ((EBX & 0xffL) == 0)
		drive = DOS_GetDefaultDrive();
	else
		drive = (EBX & 0xffL) - 1;

	if (!DOS_ValidDrive(drive)) {
		EAX = (EAX & 0xffffff00L) |InvalidDrive;
		SetCflag;
		return;
	}

	DOS_GetSerialNumber(drive,&serialnumber);

	ptr = (struct diskinfo *) pointer(DS,SI);

	ptr->infolevel = 0;
	ptr->serialnumber = serialnumber;
	strcpy(ptr->label,"NO NAME    ");
	strcpy(ptr->fstype,"FAT16   ");
	
	EAX = (EAX & 0xffffff00L) | NoError;
	ResetCflag;
}

void SetDiskSerialNumber(struct sigcontext_struct *context)
{
	EAX = (EAX & 0xffffff00L) | 1L;
	ResetCflag;
}

/************************************************************************/

int do_int21(struct sigcontext_struct * context){
	int ah;

	fprintf(stderr,"int21: doing AX=%04x BX=%04x CX=%04x DX=%04x\n",
		EAX & 0xffffL,EBX & 0xffffL,ECX & 0xffffL,EDX & 0xffffL);

	ah = (EAX >> 8) & 0xffL;

	if (ah == 0x59) {
		GetExtendedErrorInfo(context);
		return 1;
	} else {
	
	Error (0,0,0);
	
	switch(ah) {

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
	case 0x0d: /* DISK BUFFER FLUSH */
		break;

	/* no FCB support for CP/M hackers */
	
	case 0x0f: /* OPEN FILE USING FCB */
	case 0x10: /* CLOSE FILE USING FCB */
	case 0x11: /* FIND FIRST MATCHING FILE USING FCB */
	case 0x12: /* FIND NEXT MATCHING FILE USING FCB */
	case 0x13: /* DELETE FILE USING FCB */
	case 0x14: /* SEQUENTIAL READ FROM FCB FILE */		
	case 0x15: /* SEQUENTIAL WRITE TO FCB FILE */
	case 0x16: /* CREATE OR TRUNCATE FILE USING FCB */
	case 0x17: /* RENAME FILE USING FCB */
	case 0x1a: /* SET DISK TRANSFER AREA ADDRESS */
	case 0x21: /* READ RANDOM RECORD FROM FCB FILE */
	case 0x22: /* WRITE RANDOM RECORD TO FCB FILE */
	case 0x23: /* GET FILE SIZE FOR FCB */
	case 0x24: /* SET RANDOM RECORD NUMBER FOR FCB */
	case 0x27: /* RANDOM BLOCK READ FROM FCB FILE */
	case 0x28: /* RANDOM BLOCK WRITE TO FCB FILE */
	case 0x29: /* PARSE FILENAME INTO FCB */
	case 0x2f: /* GET DISK TRANSFER AREA ADDRESS */

	case 0x2e: /* SET VERIFY FLAG */
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
	        EAX &= 0xff00;
		break;
	
	case 0x67: /* SET HANDLE COUNT */			
		ResetCflag;
		break;
	
	case 0x0e: /* SELECT DEFAULT DRIVE */
		SetDefaultDrive(context);
		break;

	case 0x19: /* GET CURRENT DEFAULT DRIVE */
		GetDefaultDrive(context);
		break;
		
	case 0x1b: /* GET ALLOCATION INFORMATION FOR DEFAULT DRIVE */
		GetDefDriveAllocInfo(context);
		break;
	
	case 0x1c: /* GET ALLOCATION INFORMATION FOR SPECIFIC DRIVE */
		GetDriveAllocInfo(context);
		break;

	case 0x1f: /* GET DRIVE PARAMETER BLOCK FOR DEFAULT DRIVE */
	case 0x32: /* GET DOS DRIVE PARAMETER BLOCK FOR SPECIFIC DRIVE */
		GetDrivePB(context);
		break;
		
	case 0x25: /* SET INTERRUPT VECTOR */
		/* Ignore any attempt to set a segment vector */
		return 1;

	case 0x26: /* CREATE NEW PROGRAM SEGMENT PREFIX */
		break;

	case 0x2a: /* GET SYSTEM DATE */
		GetSystemDate(context);
		break;

	case 0x2c: /* GET SYSTEM TIME */
		GetSystemTime(context);
                break;

	case 0x30: /* GET DOS VERSION */
		fprintf(stderr,"int21: GetDosVersion\n");
	        EAX = DosVersion; /* Hey folks, this is DOS V3.3! */
		EBX = 0x0012;     /* 0x123456 is Wine's serial # */
		ECX = 0x3456;
		break;

	case 0x31: /* TERMINATE AND STAY RESIDENT */
		break;

	case 0x33: /* MULTIPLEXED */
		switch (EAX & 0xff) {
			case 0x00: /* GET CURRENT EXTENDED BREAK STATE */
				if (!(EAX & 0xffL)) 
					EDX &= 0xff00L;
				break;

			case 0x01: /* SET EXTENDED BREAK STATE */
				break;		
		
			case 0x02: /* GET AND SET EXTENDED CONTROL-BREAK CHECKING STATE */
				EDX &= 0xff00L;
				break;

			case 0x05: /* GET BOOT DRIVE */
				EDX = (EDX & 0xff00L) | 2;
					/* c: is Wine's bootdrive */
				break;
				
			case 0x06: /* GET TRUE VERSION NUMBER */
				EBX = DosVersion;
				EDX = 0x00;
				break;
			default:
				break;			
		}
		break;	

	case 0x34: /* GET ADDRESS OF INDOS FLAG */
		GetInDosFlag(context);
		break;

	case 0x35: /* GET INTERRUPT VECTOR */
 		   /* Return a NULL segment selector - this will bomb, 
 		              if anyone ever tries to use it */
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
		switch (EAX & 0xff) {
			case 0x00:
				GetFileAttributes(context);
				break;
			case 0x01:
				SetFileAttributes(context);
				break;
		}
		break;
		
	case 0x44: /* IOCTL */
		DosIOCTL(context);
		break;

	case 0x45: /* "DUP" - DUPLICATE FILE HANDLE */
	case 0x46: /* "DUP2", "FORCEDUP" - FORCE DUPLICATE FILE HANDLE */
		DupeFileHandle(context);
		break;
	
	case 0x47: /* "CWD" - GET CURRENT DIRECTORY */	
		GetCurrentDirectory(context);
		EAX = (EAX & 0xffff0000L) | 0x0100; 
		/* many Microsoft products for Windows rely on this */
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
		fprintf(stderr,"int21: DosExit\n");
		exit(EAX & 0xff);
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
		switch (EAX & 0xff) {
			case 0x00:
				GetFileDateTime(context);
				break;
			case 0x01:
				SetFileDateTime(context);
				break;
		}
		break;

	case 0x58: /* GET OR SET MEMORY/UMB ALLOCATION STRATEGY */
		switch (EAX & 0xff) {
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
	
	case 0x59: /* GET EXTENDED ERROR INFO */
		GetExtendedErrorInfo(context);
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
		EAX = (EAX & 0xfffff00L) | NoNetwork; /* network software not installed */
		SetCflag;
		break;
	
	case 0x5f: /* NETWORK */
		switch (EAX & 0xffL) {
			case 0x07: /* ENABLE DRIVE */
				if (!DOS_EnableDrive(EDX & 0xffL)) {
					Error(InvalidDrive, EC_MediaError , EL_Disk);
					EAX = (EAX & 0xfffff00L) | InvalidDrive;
					SetCflag;
					break;
				} else {
					ResetCflag;
					break;
				}
			case 0x08: /* DISABLE DRIVE */
				if (!DOS_DisableDrive(EDX & 0xffL)) {
					Error(InvalidDrive, EC_MediaError , EL_Disk);
					EAX = (EAX & 0xfffff00L) | InvalidDrive;
					SetCflag;
					break;
				} else {
					ResetCflag;
					break;
				}
			default:
				EAX = (EAX & 0xfffff00L) | NoNetwork; /* network software not installed */
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
		switch (EAX & 0xffL) {
			case 0x01:
				EBX = CodePage;
				EDX = EBX;
				ResetCflag;
				break;			
			case 0x02: 
				CodePage = EBX;
				ResetCflag;
				break;
		}
		break;

	case 0x68: /* "FFLUSH" - COMMIT FILE */
		ResetCflag;
		break;		
	
	case 0x69: /* DISK SERIAL NUMBER */
		switch (EAX & 0xff) {
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
		fprintf(stderr,"Unable to handle int 0x21 %x\n", context->sc_eax);
		return 1;
	};
	}
	return 1;
}

/********************************************************************/

static void
GetTimeDate(int time_flag)
{
    struct tm *now;
    time_t ltime;
    
    ltime = time(NULL);
    now = localtime(&ltime);
    if (time_flag)
    {
	_CX = (now->tm_hour << 8) | now->tm_min;
	_DX = now->tm_sec << 8;
    }
    else
    {
	_CX = now->tm_year + 1900;
	_DX = ((now->tm_mon + 1) << 8) | now->tm_mday;
	_AX &= 0xff00;
	_AX |= now->tm_wday;
    }
#ifdef DEBUG_DOS
    printf("GetTimeDate: AX = %04x, CX = %04x, DX = %04x\n", _AX, _CX, _DX);
#endif
    
    ReturnFromRegisterFunc();
    /* Function does not return */
}

/**********************************************************************
 *					KERNEL_DOS3Call
 */
int KERNEL_DOS3Call()
{
    switch ((_AX >> 8) & 0xff)
    {
      case 0x30:
	_AX = 0x0303;
	ReturnFromRegisterFunc();
	/* Function does not return */
	
      case 0x25:
      case 0x35:
	return 0;

      case 0x2a:
	GetTimeDate(0);
	/* Function does not return */
	
      case 0x2c:
	GetTimeDate(1);
	/* Function does not return */

      case 0x4c:
	exit(_AX & 0xff);

      default:
	fprintf(stderr, "DOS: AX %04x, BX %04x, CX %04x, DX %04x\n",
		_AX, _BX, _CX, _DX);
	fprintf(stderr, "     SP %04x, BP %04x, SI %04x, DI %04x\n",
		_SP, _BP, _SI, _DI);
	fprintf(stderr, "     DS %04x, ES %04x\n",
		_DS, _ES);
    }
    
    return 0;
}
