#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#ifdef linux
#include <sys/vfs.h>
#endif
#ifdef __NetBSD__
#include <sys/mount.h>
#endif
#include <time.h>
#include <unistd.h>
#include <utime.h>

#include "windows.h"
#include "wine.h"
#include "int21.h"
#include "files.h"

#define MAX_DRIVES 26

static char Copyright[] = "int21.c, copyright Erik Bos, 1993";

extern struct DosDriveStruct DosDrives[];
extern int CurrentDrive;
extern void ParseDOSFileName();

int ValidDrive(int);

WORD ExtendedError, CodePage = 437;
BYTE ErrorClass, Action, ErrorLocus;

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
	struct statfs info;
	long size,avail;

	if (!(DX & 0xff))
		drive = CurrentDrive;
	else
		drive = (DX & 0xff) - 1;
	
	if (!ValidDrive(drive)) {
		Error(InvalidDrive, EC_MediaError , EL_Disk);
		AX = 0xffff;	
		return;
	}
	
	{
		if (statfs(DosDrives[drive].RootDirectory, &info) < 0) {
			fprintf(stderr,"cannot do statfs(%s)\n",DosDrives[drive].RootDirectory);
			Error(GeneralFailure, EC_MediaError , EL_Disk);
			AX = 0xffff;
			return;
		}

		size = info.f_bsize * info.f_blocks / 1024;
		avail = info.f_bavail * info.f_bsize / 1024;
	
		#ifdef DOSDEBUG
		fprintf(stderr,"statfs: size: %8d avail: %8d\n",size,avail);
		#endif

		AX = SectorsPerCluster;	
		CX = SectorSize;
		
		BX = (avail / (CX * AX));
		DX = (size / (CX * AX));
		Error (0,0,0);
	}
}

void SetDefaultDrive(struct sigcontext_struct *context)
{
	if ((DX & 0xff) < MAX_DRIVES) {
		CurrentDrive = DX & 0xff;
		AX &= 0xff00;
		AX |= MAX_DRIVES; /* # of valid drive letters */
		Error (0,0,0);
	} else
		Error (InvalidDrive, EC_MediaError, EL_Disk);
}

void GetDefaultDrive(struct sigcontext_struct *context)
{
	AX &= 0xff00;
	AX |= CurrentDrive;
	Error (0,0,0);
}

void GetDriveAllocInfo(struct sigcontext_struct *context)
{
	int drive;
	long size;
	BYTE mediaID;
	struct statfs info;
	
	drive = DX & 0xff;
	
	if (!ValidDrive(drive)) {
		AX = SectorsPerCluster;
		CX = SectorSize;
		DX = 0;
		Error (InvalidDrive, EC_MediaError, EL_Disk);
		return;
	}

	{
		if (statfs(DosDrives[drive].RootDirectory, &info) < 0) {
			fprintf(stderr,"cannot do statfs(%s)\n",DosDrives[drive].RootDirectory);
			Error(GeneralFailure, EC_MediaError , EL_Disk);
			AX = 0xffff;
			return;
		}

		size = info.f_bsize * info.f_blocks / 1024;
	
		#ifdef DOSDEBUG
		fprintf(stderr,"statfs: size: %8d\n",size);
		#endif

		AX = SectorsPerCluster;	
		CX = SectorSize;
		DX = (size / (CX * AX));

		mediaID = 0xf0;

		DS = segment(mediaID);
		BX = offset(mediaID);	
		Error (0,0,0);
	}
}

void GetDefDriveAllocInfo(struct sigcontext_struct *context)
{
	DX = CurrentDrive;
	GetDriveAllocInfo(context);
}

void GetDrivePB(struct sigcontext_struct *context)
{
	Error (InvalidDrive, EC_MediaError, EL_Disk);
	AX = 0xff; /* I'm sorry but I only got networked drives :-) */
}

void ReadFile(struct sigcontext_struct *context)
{
	char *ptr;
	int size;

	/* can't read from stdout / stderr */

	if (((BX & 0xffff) == 1) ||((BX & 0xffff) == 2)) {
		Error (InvalidHandle, EL_Unknown, EC_Unknown);
		AX = InvalidHandle;
		SetCflag;
		return;
	}

	ptr = (char *) pointer (DS,DX);

	if ((BX & 0xffff) == 0) {
		*ptr = EOF;
		Error (0,0,0);
		AX = 1;
		ResetCflag;
		return;
	} else {
		size = read(BX, ptr, CX);
		if (size == 0) {
			Error (ReadFault, EC_Unknown, EL_Unknown);
			AX = ExtendedError;
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
			AX = ExtendedError;
			SetCflag;
			return;
		}		
		Error (0,0,0);
		AX = size;
		ResetCflag;
	}
}

void WriteFile(struct sigcontext_struct *context)
{
	char *ptr;
	int x,size;
	
	ptr = (char *) pointer (DS,DX);
	
	if ((BX & 0xffff) == 0) {
		Error (InvalidHandle, EC_Unknown, EL_Unknown);
		AX = InvalidHandle;
		SetCflag;
		return;
	}

	if ((BX & 0xffff) < 3) {
		for (x = 0;x != CX;x++) {
			fprintf(stderr, "%c", *ptr++);
		}
		fflush(stderr);

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
			AX = ExtendedError;
			SetCflag;
			return;
		}		
		Error (0,0,0);
		AX = size;
		ResetCflag;
	}
}

void UnlinkFile(struct sigcontext_struct *context)
{
	char UnixFileName[256];
	int drive, status;
	
	ParseDOSFileName(UnixFileName, (char *) pointer(DS,DX), &drive);

	{
		status = unlink((char *) pointer(DS,DX));
		if (status == -1) {
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
			AX = ExtendedError;
			SetCflag;
			return;
		}		
		Error (0,0,0);
		ResetCflag;
	}
}

void SeekFile(struct sigcontext_struct *context)
{
	char UnixFileName[256];
	int drive, handle, status, fileoffset;
	
	
	ParseDOSFileName(UnixFileName, (char *) pointer(DS,DX), &drive);
	
	{
		switch (AX & 0xff) {
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
			AX = ExtendedError;
			SetCflag;
			return;
		}		
		Error (0,0,0);
		ResetCflag;
	}
}

void GetFileAttributes(struct sigcontext_struct *context)
{
	char UnixFileName[256];
	int drive,handle;
	
	ParseDOSFileName(UnixFileName, (char *) pointer(DS,DX), &drive);

	{
		CX = 0x0;
		ResetCflag;
	}
}

void SetFileAttributes(struct sigcontext_struct *context)
{
	ResetCflag;
}

void DosIOCTL(struct sigcontext_struct *context)
{
	AX = UnknownUnit;
	SetCflag;
}

void DupeFileHandle(struct sigcontext_struct *context)
{
	AX = dup(BX);
	ResetCflag;
}

void GetSystemDate(struct sigcontext_struct *context)
{
	struct tm *now;
	time_t ltime;

	ltime = time(NULL);
	now = localtime(&ltime);

	CX = now->tm_year + 1900;
	DX = ((now->tm_mon + 1) << 8) | now->tm_mday;
	AX &= 0xff00;
	AX |= now->tm_wday;
}

void GetSystemTime(struct sigcontext_struct *context)
{
	struct tm *now;
	time_t ltime;

	ltime = time(NULL);
	now = localtime(&ltime);
	 
	CX = (now->tm_hour << 8) | now->tm_min;
	DX = now->tm_sec << 8;
}

void GetExtendedErrorInfo(struct sigcontext_struct *context)
{
	AX = ExtendedError;
	BX = (0x100 * ErrorClass) | Action;
	CX &= 0x00ff;
	CX |= (0x100 * ErrorLocus);
}

void GetInDosFlag(struct sigcontext_struct *context)
{
	const BYTE InDosFlag = 0;
	
	ES = segment(InDosFlag);
	BX = offset(InDosFlag);
}

void CreateFile(struct sigcontext_struct *context)
{
	char UnixFileName[256];
	int drive,handle;
	
	ParseDOSFileName(UnixFileName, (char *) pointer(DS,DX), &drive);

	{
		handle = open(UnixFileName, O_CREAT | O_TRUNC);

		if (handle == -1) {
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
			AX = ExtendedError;
			SetCflag;
			return;
		}		
		Error (0,0,0);
		BX = handle;
		AX = NoError;
		ResetCflag;
	}
}

void OpenExistingFile(struct sigcontext_struct *context)
{
	char UnixFileName[256];
	int drive, handle;
	
	ParseDOSFileName(UnixFileName, (char *) pointer(DS,DX), &drive);
	
	{
		handle = open(UnixFileName, O_RDWR);

		if (handle == -1) {
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
			AX = ExtendedError;
			SetCflag;
			return;
		}		
		Error (0,0,0);
		BX = handle;
		AX = NoError;
		ResetCflag;
	}
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
		AX = ExtendedError;
		SetCflag;
		return;
	}		
	Error (0,0,0);
	AX = NoError;
	ResetCflag;
}

void RenameFile(struct sigcontext_struct *context)
{
	rename((char *) pointer(DS,DX), (char *) pointer(ES,DI));
	ResetCflag;
}

void GetTrueFileName(struct sigcontext_struct *context)
{ 
	strncpy((char *) pointer(ES,DI), (char *) pointer(DS,SI), strlen((char *) pointer(DS,SI)) & 0x7f);
	ResetCflag;
}

void MakeDir(struct sigcontext_struct *context)
{
	int drive;
	char *dirname;
	char unixname[256];
	
	dirname = (char *) pointer(DS,DX);

	ParseDOSFileName(unixname,dirname,&drive);

	{
		if (mkdir(unixname,0) == -1) {
			AX = CanNotMakeDir;
			SetCflag;
		}
		ResetCflag;
	}
}

void ChangeDir(struct sigcontext_struct *context)
{
	int drive;
	char *dirname;
	char unixname[256];
	
	dirname = (char *) pointer(DS,DX);

	ParseDOSFileName(unixname,dirname,&drive);

	{
		strcpy(unixname,DosDrives[drive].CurrentDirectory);
		ResetCflag;
	}
}

void RemoveDir(struct sigcontext_struct *context)
{
	int drive;
	char *dirname;
	char unixname[256];
	
	dirname = (char *) pointer(DS,DX);

	ParseDOSFileName(unixname,dirname,&drive);

	{
		if (strcmp(unixname,DosDrives[drive].CurrentDirectory)) {
			AX = CanNotRemoveCwd;
			SetCflag;
		}
	
		#ifdef DOSDEBUG
		fprintf(stderr,"rmdir %s\n",unixname);
		#endif
		
		if (rmdir(unixname) == -1) {
			AX = CanNotMakeDir; /* HUH ?*/
			SetCflag;
		} 
		ResetCflag;
	}
}

void AllocateMemory(struct sigcontext_struct *context)
{
	char *ptr;
	
	if ((ptr = (void *) memalign((size_t) (BX * 0x10), 0x10)) == NULL) {
		AX = OutOfMemory;
		BX = 0x0; /* out of memory */
		SetCflag;
	}
	AX = segment((unsigned long) ptr);
	ResetCflag;
}

void FreeMemory(struct sigcontext_struct *context)
{
	free((void *)(ES * 0x10));
	ResetCflag;
}

void ResizeMemoryBlock(struct sigcontext_struct *context)
{
	char *ptr;
	
	if ((ptr = (void *) realloc((void *)(ES * 0x10), (size_t) BX * 0x10)) == NULL) {
		AX = OutOfMemory;
		BX = 0x0; /* out of memory */
		SetCflag;
	}
	BX = segment((unsigned long) ptr);
	ResetCflag;
}

void ExecProgram(struct sigcontext_struct *context)
{
	execl("wine",(char *) pointer(DS,DX));
}

void GetReturnCode(struct sigcontext_struct *context)
{
	AX = NoError; /* normal exit */
}

void FindFirst(struct sigcontext_struct *context)
{

}

void FindNext(struct sigcontext_struct *context)
{

}

void GetSysVars(struct sigcontext_struct *context)
{
	ES = 0x0;
	BX = 0x0;
}

void GetFileDateTime(struct sigcontext_struct *context)
{
	int drive;
	char *dirname;
	char unixname[256];
	struct stat filestat;
	struct tm *now;

	ParseDOSFileName(unixname, dirname, &drive);

	{
		stat(unixname, &filestat);
	 	
		now = localtime (&filestat.st_mtime);
	
		CX = (now->tm_hour * 0x2000) + (now->tm_min * 0x20) + now->tm_sec/2;
		DX = (now->tm_year * 0x200) + (now->tm_mon * 0x20) + now->tm_mday;

		ResetCflag;
	}
}

void SetFileDateTime(struct sigcontext_struct *context)
{
	int drive;
	char *dirname;
	char unixname[256];
	struct utimbuf filetime;
	
	dirname = (char *) pointer(DS,DX);

	ParseDOSFileName(unixname, dirname, &drive);

	{
		filetime.actime = 0L;
		filetime.modtime = filetime.actime;

		utime(unixname,&filetime);
		ResetCflag;
	}
}

void CreateTempFile(struct sigcontext_struct *context)
{
	char UnixFileName[256],TempString[256];
	int drive,handle;
	
	ParseDOSFileName(UnixFileName, (char *) pointer(DS,DX), &drive);
	
	sprintf(TempString,"%s%s%d",UnixFileName,"eb",(int) getpid());

	{
		handle = open(TempString, O_CREAT | O_TRUNC | O_RDWR);

		if (handle == -1) {
			AX = WriteProtected;
			SetCflag;
			return;
		}
		
		strcpy((char *) pointer(DS,DX), UnixFileName);
	
		AX = handle;
		ResetCflag;
	}
}

void CreateNewFile(struct sigcontext_struct *context)
{
	char UnixFileName[256];
	int drive,handle;
	
	ParseDOSFileName(UnixFileName, (char *) pointer(DS,DX), &drive);
	
	{
		handle = open(UnixFileName, O_CREAT | O_TRUNC | O_RDWR);

		if (handle == -1) {
			AX = WriteProtected;
			SetCflag;
			return;
		}

		AX = handle;
		ResetCflag;
	}
}

void FileLock(struct sigcontext_struct *context)
{

}

void GetExtendedCountryInfo(struct sigcontext_struct *context)
{
	ResetCflag;
}

int ValidDrive(int d)
{
	return 1;
}

void GetCurrentDirectory(struct sigcontext_struct *context)
{
	int drive;
	char *ptr;

	if ((DX & 0xff) == 0)
		drive = CurrentDrive;
	else
		drive = (DX & 0xff)-1;

	if (!ValidDrive(drive)) {
		AX = InvalidDrive;
		SetCflag;
		return;
	}
	
	strcpy((char *) pointer(DS,SI), DosDrives[drive].CurrentDirectory);
	ResetCflag;
	AX = 0x0100;
}

void GetCurrentPSP(struct sigcontext_struct *context)
{

}

void GetDiskSerialNumber(struct sigcontext_struct *context)
{
	int drive;
	struct diskinfo *ptr;
	
	if ((BX & 0xff)== 0)
		drive = CurrentDrive;
	else
		drive = (BX & 0xff)-1;

	if (!ValidDrive(drive)) {
		AX = InvalidDrive;
		SetCflag;
		return;
	}

	{
		ptr =(struct diskinfo *) pointer(DS,SI);

		ptr->infolevel = 0;
		ptr->serialnumber = 0xEBEBEB00 | drive;
		strcpy(ptr->label,"NO NAME    ");
		strcpy(ptr->fstype,"FAT16   ");
	
		AX = NoError;
		ResetCflag;
	}
}

void SetDiskSerialNumber(struct sigcontext_struct *context)
{
	AX &= 0xff00;
	AX |= 1;
	ResetCflag;
}

void CommitFile(struct sigcontext_struct *context)
{

}

/************************************************************************/

int do_int21(struct sigcontext_struct * context){
	int ah;

	fprintf(stderr,"int21: doing AX=%4x BX=%4x CX=%4x DX=%4x\n",
		AX & 0xffff,BX & 0xffff,CX & 0xffff,DX & 0xffff);

	ah = (AX >> 8) & 0xff;

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
	case 0x15: /* SEQUENTIAL WRITE TO FCB FILE
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

	case 0x18: /* NULL FUNCTIONS FOR CP/M COMPATIBILITY *
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
	        AX &= 0xff00;
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
	        AX = DosVersion; /* Hey folks, this is DOS V3.3! */
		BX = 0x0012;     /* 0x123456 is Wine's serial # */
		CX = 0x3456;
		break;

	case 0x31: /* TERMINATE AND STAY RESIDENT */
		break;

	case 0x33: /* MULTIPLEXED */
		switch (AX & 0xff) {
			case 0x00: /* GET CURRENT EXTENDED BREAK STATE */
				if (!(AX & 0xff)) 
					DX &= 0xff00;
				break;

			case 0x01: /* SET EXTENDED BREAK STATE */
				break;		
		
			case 0x02: /* GET AND SET EXTENDED CONTROL-BREAK CHECKING STATE */
				DX &= 0xff00;
				break;

			case 0x05: /* GET BOOT DRIVE */
				DX &= 0xff00;
				DX |= 2; /* c: is Wine's bootdrive */
				break;
				
			case 0x06: /* GET TRUE VERSION NUMBER */
				BX = DosVersion;
				DX = 0x00;
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
		BX = 0;
		break;

	case 0x36: /* GET FREE DISK SPACE */
		GetFreeDiskSpace(context);
		break;

	case 0x38: /* GET COUNTRY-SPECIFIC INFORMATION */
		AX &= 0xff00;
		AX |= 0x02; /* no country support available */
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
	case 0x68: /* "FFLUSH" - COMMIT FILE */
	case 0x6a: /* COMMIT FILE */

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
		switch (AX & 0xff) {
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
		AX = 0x0100; /* many Microsoft products for Windows rely
				on this */
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
		exit(AX & 0xff);

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
		switch (AX & 0xff) {
			case 0x00:
				GetFileDateTime(context);
				break;
			case 0x01:
				SetFileDateTime(context);
				break;
		}
		break;

	case 0x58: /* GET OR SET MEMORY/UMB ALLOCATION STRATEGY */
		switch (AX & 0xff) {
			case 0x00:
				AX = 0x01;
				break;
			case 0x02:
				AX &= 0xff00;
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
	case 0x5f:
		AX &= 0xff00;
		AX |= NoNetwork; /* network software not installed */
		SetCflag;
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
		switch (AX & 0xff) {
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
	
	case 0x69: /* DISK SERIAL NUMBER */
		switch (AX & 0xff) {
			case 0x00:
				GetDiskSerialNumber(context);
				break;			
			case 0x01: 
				SetDiskSerialNumber(context);
				break;
		}
		break;

	default:
		fprintf(stderr,"Unable to handle int 0x21 %x\n", context->sc_eax);
		return 1;
	};
	}
	return 1;
}
