#ifndef __MSDOS_H
#define __MSDOS_H

#include <dirent.h>
#include "windows.h"
#include "comm.h"

struct fcb {
        BYTE drive;
	char name[8];
	char extension[3];
	BYTE dummy1[4];
	int filesize;
	WORD date_write;
	WORD time_write;
	struct dosdirent *directory;
	BYTE dummy2[9];
};

/* DTA layout for FindFirst/FindNext */
typedef struct
{
    BYTE   drive;                /* 00 drive letter */
    char   mask[11];             /* 01 search template */
    BYTE   search_attr;          /* 0c search attributes */
    WORD   count WINE_PACKED;    /* 0d entry count within directory */
    WORD   cluster WINE_PACKED;  /* 0f cluster of parent directory */
    char  *unixPath WINE_PACKED; /* 11 unix path (was: reserved) */
    BYTE   fileattr;             /* 15 file attributes */
    WORD   filetime;             /* 16 file time */
    WORD   filedate;             /* 18 file date */
    DWORD  filesize WINE_PACKED; /* 1a file size */
    char   filename[13];         /* 1e file name + extension */
} FINDFILE_DTA;

/* FCB layout for FindFirstFCB/FindNextFCB */
typedef struct
{
    BYTE   drive;                /* 00 drive letter */
    char   filename[11];         /* 01 filename 8+3 format */
    int    count;                /* 0c entry count (was: reserved) */
    char  *unixPath;             /* 10 unix path (was: reserved) */
} FINDFILE_FCB;

/* DOS directory entry for FindFirstFCB/FindNextFCB */
typedef struct
{
    char   filename[11];         /* 00 filename 8+3 format */
    BYTE   fileattr;             /* 0b file attributes */
    BYTE   reserved[10];         /* 0c reserved */
    WORD   filetime;             /* 16 file time */
    WORD   filedate;             /* 18 file date */
    WORD   cluster;              /* 1a file first cluster */
    DWORD  filesize;             /* 1c file size */
} DOS_DIRENTRY_LAYOUT;

#define DOSVERSION 0x1606      /* Major version in low byte: DOS 6.22 */
#define WINDOSVER  0x0616      /* Windows reports the DOS version reversed */
#define WINVERSION 0x0a03      /* Windows version 3.10 */

#define MAX_DOS_DRIVES	26

extern struct DosDeviceStruct COM[MAX_PORTS];
extern struct DosDeviceStruct LPT[MAX_PORTS];

#define setword(a,b)	do { *(BYTE*)(a)	  = (b) & 0xff; \
                             *((BYTE*)((a)+1)) = ((b)>>8) & 0xff;\
                        } while(0)
			

/* dos file attributes */

#define FA_NORMAL    0x00        /* Normal file, no attributes */
#define FA_RDONLY    0x01        /* Read only attribute */
#define FA_HIDDEN    0x02        /* Hidden file */
#define FA_SYSTEM    0x04        /* System file */
#define FA_LABEL     0x08        /* Volume label */
#define FA_DIRECTORY 0x10        /* Directory */
#define FA_ARCHIVE   0x20        /* Archive */
#define FA_UNUSED    0x40        /* Unused */


extern WORD DOS_ExtendedError;
extern BYTE DOS_ErrorClass, DOS_ErrorAction, DOS_ErrorLocus;

#define DOS_ERROR(err,class,action,locus) \
    ( DOS_ErrorClass = (class), DOS_ErrorAction = (action), \
      DOS_ErrorLocus = (locus), DOS_ExtendedError = (err) )

/* Error codes */

#define ER_NoError           0x00
#define ER_InvalidFunction   0x01
#define ER_FileNotFound      0x02
#define ER_PathNotFound      0x03
#define ER_TooManyOpenFiles  0x04
#define ER_AccessDenied      0x05
#define ER_InvalidHandle     0x06
#define ER_MCBDestroyed      0x07
#define ER_OutOfMemory       0x08
#define ER_MCBInvalid        0x09
#define ER_EnvironInvalid    0x0a
#define ER_FormatInvalid     0x0b
#define ER_AccessCodeInvalid 0x0c
#define ER_DataInvalid       0x0d
#define ER_InvalidDrive      0x0f
#define ER_CanNotRemoveCwd   0x10
#define ER_NotSameDevice     0x11
#define ER_NoMoreFiles       0x12
#define ER_WriteProtected    0x13
#define ER_UnknownUnit       0x14
#define ER_DriveNotReady     0x15
#define ER_UnknownCommand    0x16
#define ER_CRCError          0x17
#define ER_BadRqLength       0x18
#define ER_SeekError         0x19
#define ER_UnknownMedia      0x1a
#define ER_SectorNotFound    0x1b
#define ER_OutOfPaper        0x1c
#define ER_WriteFault        0x1d
#define ER_ReadFault         0x1e
#define ER_GeneralFailure    0x1f
#define ER_ShareViolation    0x20
#define ER_LockViolation     0x21
#define ER_DiskFull          0x27
#define ER_NoNetwork         0x49
#define ER_FileExists        0x50
#define ER_CanNotMakeDir     0x52

/* Error classes */

#define EC_OutOfResource     0x01
#define EC_Temporary         0x02
#define EC_AccessDenied      0x03
#define EC_InternalError     0x04
#define EC_HardwareFailure   0x05
#define EC_SystemFailure     0x06
#define EC_ProgramError      0x07
#define EC_NotFound          0x08
#define EC_MediaError        0x0b
#define EC_Exists            0x0c
#define EC_Unknown           0x0d

/* Suggested actions */

#define SA_Retry             0x01
#define SA_DelayedRetry      0x02
#define SA_Abort             0x04
#define SA_Ignore            0x06
#define SA_Ask4Retry         0x07

/* Error locus */

#define EL_Unknown           0x01
#define EL_Disk              0x02
#define EL_Network           0x03
#define EL_Serial            0x04
#define EL_Memory            0x05

#endif /* __MSDOS_H */
