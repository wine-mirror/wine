/*
 * Copyright 1994 Erik Bos
 * Copyright 1999 Ove Kaaven
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

#ifndef __WINE_MSDOS_H
#define __WINE_MSDOS_H

#include "wine/windef16.h"

#include "pshpack1.h"

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
    BYTE   drive;        /* 00 drive letter */
    char   mask[11];     /* 01 search template */
    BYTE   search_attr;  /* 0c search attributes */
    WORD   count;        /* 0d entry count within directory */
    WORD   cluster;      /* 0f cluster of parent directory */
    char  *unixPath;     /* 11 unix path (was: reserved) */
    BYTE   fileattr;     /* 15 file attributes */
    WORD   filetime;     /* 16 file time */
    WORD   filedate;     /* 18 file date */
    DWORD  filesize;     /* 1a file size */
    char   filename[13]; /* 1e file name + extension */
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
    DWORD next_dev;
    WORD  attr;
    WORD  strategy;
    WORD  interrupt;
    char  name[8];
} DOS_DEVICE_HEADER;

/* Warning: need to return LOL ptr w/ offset 0 (&ptr_first_DPB) to programs ! */
typedef struct _DOS_LISTOFLISTS
{
    WORD  CX_Int21_5e01;	/* -24d contents of CX from INT 21/AX=5E01h */
    WORD  LRU_count_FCB_cache;	/* -22d */
    WORD  LRU_count_FCB_open;	/* -20d */
    DWORD OEM_func_handler;	/* -18d OEM function of INT 21/AH=F8h */
    WORD  INT21_offset;		/* -14d offset in DOS CS of code to return from INT 21 call */
    WORD  sharing_retry_count;	/* -12d */
    WORD  sharing_retry_delay;	/* -10d */
    DWORD ptr_disk_buf;		/* -8d ptr to current disk buf */
    WORD  offs_unread_CON;	/* -4d pointer in DOS data segment of unread CON input */
    WORD  seg_first_MCB;	/* -2d */
    DWORD ptr_first_DPB;	/* 00 */
    DWORD ptr_first_SysFileTable; /* 04 */
    DWORD ptr_clock_dev_hdr;	/* 08 */
    DWORD ptr_CON_dev_hdr;	/* 0C */
    WORD  max_byte_per_sec;	/* 10 maximum bytes per sector of any block device */
    DWORD ptr_disk_buf_info;	/* 12 */
    DWORD ptr_array_CDS;	/* 16 current directory structure */
    DWORD ptr_sys_FCB;	 	/* 1A */
    WORD  nr_protect_FCB;	/* 1E */
    BYTE  nr_block_dev;		/* 20 */
    BYTE  nr_avail_drive_letters; /* 21 */
    DOS_DEVICE_HEADER NUL_dev;	/* 22 */
    BYTE  nr_drives_JOINed;	/* 34 */
    WORD  ptr_spec_prg_names;	/* 35 */
    DWORD ptr_SETVER_prg_list;	/* 37 */
    WORD DOS_HIGH_A20_func_offs;/* 3B */
    WORD PSP_last_exec;		/* 3D if DOS in HMA: PSP of program executed last; if DOS low: 0000h */
    WORD BUFFERS_val;		/* 3F */
    WORD BUFFERS_nr_lookahead;	/* 41 */
    BYTE boot_drive;		/* 43 */
    BYTE flag_DWORD_moves;	/* 44 01h for 386+, 00h otherwise */
    WORD size_extended_mem;	/* 45 size of extended mem in KB */
    SEGPTR wine_rm_lol;         /* -- wine: Real mode pointer to LOL */
    SEGPTR wine_pm_lol;         /* -- wine: Protected mode pointer to LOL */
} DOS_LISTOFLISTS;

#include "poppack.h"

#define MAX_DOS_DRIVES	26

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

#define DOSCONF_MEM_HIGH        0x0001
#define DOSCONF_MEM_UMB         0x0002
#define DOSCONF_NUMLOCK         0x0004
#define DOSCONF_KEYB_CONV       0x0008

typedef struct {
        char lastdrive;
        int brk_flag;
        int files;
        int stacks_nr;
        int stacks_sz;
        int buf;
        int buf2;
        int fcbs;
        int flags;
        char *shell;
        char *country;
} DOSCONF;

extern DOSCONF DOSCONF_config;

#endif /* __WINE_MSDOS_H */
