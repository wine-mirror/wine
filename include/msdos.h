#ifndef __WINE_MSDOS_H
#define __WINE_MSDOS_H

#include <sys/types.h>
#include <dirent.h>
#include "windows.h"
#include "comm.h"
#include "winnt.h"

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

typedef struct
{
    WORD  CX_Int21_5e01;	/* contents of CX from INT 21/AX=5E01h */
    WORD  LRU_count_FCB_cache;	
    WORD  LRU_count_FCB_open;
    DWORD OEM_func_handler WINE_PACKED; /* OEM function of INT 21/AH=F8h */
    WORD  INT21_offset;/* offset in DOS CS of code to return from INT 21 call */
    WORD  sharing_retry_count;
    WORD  sharing_retry_delay;
    DWORD ptr_disk_buf;		/* ptr to current disk buf */
    WORD  offs_unread_CON;	/* pointer in DOS data segment of unread CON input */
    WORD  seg_first_MCB;
    DWORD ptr_first_DPB;
    DWORD ptr_first_SysFileTable;
    DWORD ptr_clock_dev_hdr;
    DWORD ptr_CON_dev_hdr;
    WORD  max_byte_per_sec;   /* maximum bytes per sector of any block device */
    DWORD ptr_disk_buf_info WINE_PACKED;
    DWORD ptr_array_CDS WINE_PACKED; /* current directory structure */
    DWORD ptr_sys_FCB WINE_PACKED;
    WORD  nr_protect_FCB;
    BYTE  nr_block_dev;
    BYTE  nr_avail_drive_letters;
    BYTE  NUL_dev_header[18];
    BYTE  nr_drives_JOINed;
    WORD  ptr_spec_prg_names WINE_PACKED;
    DWORD ptr_SETVER_prg_list WINE_PACKED;
    WORD DOS_HIGH_A20_func_offs WINE_PACKED;
    WORD PSP_last_exec WINE_PACKED; /* if DOS in HMA: PSP of program executed last; if DOS low: 0000h */
    WORD BUFFERS_val WINE_PACKED;
    WORD BUFFERS_nr_lookahead WINE_PACKED;
    BYTE boot_drive WINE_PACKED;
    BYTE flag_DWORD_moves WINE_PACKED; /* 01h for 386+, 00h otherwise */
    WORD size_extended_mem WINE_PACKED; /* size of extended mem in KB */
} DOS_LISTOFLISTS;

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
    ( SetLastError(err), \
      DOS_ErrorClass = (class), DOS_ErrorAction = (action), \
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

void WINAPI DOS3Call( CONTEXT *context );
void do_mscdex( CONTEXT *context );
void do_mscdex_dd (CONTEXT * context, int dorealmode);

#endif /* __WINE_MSDOS_H */
