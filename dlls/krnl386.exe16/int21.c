/*
 * DOS interrupt 21h handler
 *
 * Copyright 1993, 1994 Erik Bos
 * Copyright 1996 Alexandre Julliard
 * Copyright 1997 Andreas Mohr
 * Copyright 1998 Uwe Bonnes
 * Copyright 1998, 1999 Ove Kaaven
 * Copyright 2003 Thomas Mertes
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wincon.h"
#include "winternl.h"
#include "wine/winbase16.h"
#include "kernel16_private.h"
#include "dosexe.h"
#include "winerror.h"
#include "winuser.h"
#include "wine/debug.h"

/*
 * Note:
 * - Most of the file related functions are wrong. NT's kernel32
 *   doesn't maintain a per drive current directory, while DOS does. 
 *   We should in fact keep track in here of those per drive
 *   directories, and use this info while dealing with partial paths
 *   (drive defined, but only relative paths). This could even be
 *   created as an array of CDS (there should be an entry for that in
 *   the LOL)
 */

/*
 * Forward declarations.
 */
static BOOL INT21_RenameFile( CONTEXT *context );

WINE_DEFAULT_DEBUG_CHANNEL(int21);


#include "pshpack1.h"

/*
 * Extended Drive Parameter Block.
 * This structure is compatible with standard DOS4+ DPB and 
 * extended DOS7 DPB.
 */
typedef struct _INT21_DPB {
    BYTE   drive;                /* 00 drive number (0=A, ...) */
    BYTE   unit;                 /* 01 unit number within device driver */
    WORD   sector_bytes;         /* 02 bytes per sector */
    BYTE   cluster_sectors;      /* 04 highest sector number within a cluster */
    BYTE   shift;                /* 05 shift count to convert clusters into sectors */
    WORD   num_reserved;         /* 06 reserved sectors at beginning of drive */
    BYTE   num_FAT;              /* 08 number of FATs */
    WORD   num_root_entries;     /* 09 number of root directory entries */
    WORD   first_data_sector;    /* 0b number of first sector containing user data */
    WORD   num_clusters1;        /* 0d highest cluster number (number of data clusters + 1) */
    WORD   sectors_per_FAT;      /* 0f number of sectors per FAT */
    WORD   first_dir_sector;     /* 11 sector number of first directory sector */
    SEGPTR driver_header;        /* 13 address of device driver header */
    BYTE   media_ID;             /* 17 media ID byte */
    BYTE   access_flag;          /* 18 0x00 if disk accessed, 0xff if not */
    SEGPTR next;                 /* 19 pointer to next DPB */
    WORD   search_cluster1;      /* 1d cluster at which to start search for free space */
    WORD   free_clusters_lo;     /* 1f number of free clusters on drive or 0xffff if unknown */
    WORD   free_clusters_hi;     /* 21 hiword of clusters_free */
    WORD   mirroring_flags;      /* 23 active FAT/mirroring flags */
    WORD   info_sector;          /* 25 sector number of file system info sector or 0xffff for none */
    WORD   spare_boot_sector;    /* 27 sector number of backup boot sector or 0xffff for none */
    DWORD  first_cluster_sector; /* 29 sector number of the first cluster */
    DWORD  num_clusters2;        /* 2d maximum cluster number */
    DWORD  fat_clusters;         /* 31 number of clusters occupied by FAT */
    DWORD  root_cluster;         /* 35 cluster number of start of root directory */
    DWORD  search_cluster2;      /* 39 cluster at which to start searching for free space */
} INT21_DPB;


/*
 * Structure for DOS data that can be accessed directly from applications.
 * Real and protected mode pointers will be returned to this structure so
 * the structure must be correctly packed.
 */
typedef struct _INT21_HEAP {
    WORD uppercase_size;             /* Size of the following table in bytes */
    BYTE uppercase_table[128];       /* Uppercase equivalents of chars from 0x80 to 0xff. */

    WORD lowercase_size;             /* Size of the following table in bytes */
    BYTE lowercase_table[256];       /* Lowercase equivalents of chars from 0x00 to 0xff. */

    WORD collating_size;             /* Size of the following table in bytes */
    BYTE collating_table[256];       /* Values used to sort characters from 0x00 to 0xff. */

    WORD filename_size;              /* Size of the following filename data in bytes */
    BYTE filename_reserved1;         /* 0x01 for MS-DOS 3.30-6.00 */
    BYTE filename_lowest;            /* Lowest permissible character value for filename */
    BYTE filename_highest;           /* Highest permissible character value for filename */
    BYTE filename_reserved2;         /* 0x00 for MS-DOS 3.30-6.00 */
    BYTE filename_exclude_first;     /* First illegal character in permissible range */
    BYTE filename_exclude_last;      /* Last illegal character in permissible range */
    BYTE filename_reserved3;         /* 0x02 for MS-DOS 3.30-6.00 */
    BYTE filename_illegal_size;      /* Number of terminators in the following table */
    BYTE filename_illegal_table[16]; /* Characters which terminate a filename */

    WORD dbcs_size;                  /* Number of valid ranges in the following table */
    BYTE dbcs_table[16];             /* Start/end bytes for N ranges and 00/00 as terminator */

    BYTE      misc_indos;                    /* Interrupt 21 nesting flag */
    WORD      misc_selector;                 /* Protected mode selector for INT21_HEAP */
    INT21_DPB misc_dpb_list[MAX_DOS_DRIVES]; /* Drive parameter blocks for all drives */

} INT21_HEAP;


struct FCB {
    BYTE  drive_number;
    CHAR  file_name[8];
    CHAR  file_extension[3];
    WORD  current_block_number;
    WORD  logical_record_size;
    DWORD file_size;
    WORD  date_of_last_write;
    WORD  time_of_last_write;
    BYTE  file_number;
    BYTE  attributes;
    WORD  starting_cluster;
    WORD  sequence_number;
    BYTE  file_attributes;
    BYTE  unused;
    BYTE  record_within_current_block;
    BYTE  random_access_record_number[4];
};


struct XFCB {
    BYTE  xfcb_signature;
    BYTE  reserved[5];
    BYTE  xfcb_file_attribute;
    BYTE  fcb[37];
};

/* DTA layout for FindFirst/FindNext */
typedef struct
{
    BYTE   drive;        /* 00 drive letter */
    char   mask[11];     /* 01 search template */
    BYTE   search_attr;  /* 0c search attributes */
    WORD   count;        /* 0d entry count within directory */
    WORD   cluster;      /* 0f cluster of parent directory */
    WCHAR *fullPath;     /* 11 full path (was: reserved) */
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
    WCHAR *fullPath;             /* 10 full path (was: reserved) */
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

#include "poppack.h"

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
#define ER_NoNetwork         0x49

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

/* BIOS Keyboard Scancodes */
#define KEY_LEFT        0x4B
#define KEY_RIGHT       0x4D
#define KEY_UP          0x48
#define KEY_DOWN        0x50
#define KEY_IC          0x52 /* insert char */
#define KEY_DC          0x53 /* delete char */
#define KEY_BACKSPACE   0x0E
#define KEY_HOME        0x47
#define KEY_END         0x4F
#define KEY_NPAGE       0x49
#define KEY_PPAGE       0x51

static int brk_flag;

static BYTE drive_number( WCHAR letter )
{
    if (letter >= 'A' && letter <= 'Z') return letter - 'A';
    if (letter >= 'a' && letter <= 'z') return letter - 'a';
    return MAX_DOS_DRIVES;
}


/* Many calls translate a drive argument like this:
   drive number (00h = default, 01h = A:, etc)
   */
/******************************************************************
 *		INT21_DriveName
 *
 * Many calls translate a drive argument like this:
 * drive number (00h = default, 01h = A:, etc)
 */
static const char *INT21_DriveName(int drive)
{
    if (drive > 0) 
    {
        if (drive <= 26) return wine_dbg_sprintf("%c:", 'A' + drive - 1);
        else return wine_dbg_sprintf( "<Bad drive: %d>", drive);
    }
    return "default";
}

/***********************************************************************
 *           INT21_GetCurrentDrive
 *
 * Return current drive using scheme (0=A:, 1=B:, 2=C:, ...) or
 * MAX_DOS_DRIVES on error.
 */
static BYTE INT21_GetCurrentDrive(void)
{
    WCHAR current_directory[MAX_PATH];

    if (!GetCurrentDirectoryW( MAX_PATH, current_directory ) ||
        current_directory[1] != ':')
    {
        TRACE( "Failed to get current drive.\n" );
        return MAX_DOS_DRIVES;
    }
    return drive_number( current_directory[0] );
}


/***********************************************************************
 *           INT21_MapDrive
 *
 * Convert drive number from scheme (0=default, 1=A:, 2=B:, ...) into
 * scheme (0=A:, 1=B:, 2=C:, ...) or MAX_DOS_DRIVES on error.
 */
static BYTE INT21_MapDrive( BYTE drive )
{
    if (drive)
    {
        WCHAR drivespec[] = {'A', ':', 0};
        UINT  drivetype;

        drivespec[0] += drive - 1;
        drivetype = GetDriveTypeW( drivespec );

        if (drivetype == DRIVE_UNKNOWN || drivetype == DRIVE_NO_ROOT_DIR)
            return MAX_DOS_DRIVES;

        return drive - 1;
    }

    return INT21_GetCurrentDrive();
}


/***********************************************************************
 *           INT21_SetCurrentDrive
 *
 * Set current drive. Uses scheme (0=A:, 1=B:, 2=C:, ...).
 */
static void INT21_SetCurrentDrive( BYTE drive )
{
    WCHAR drivespec[] = {'A', ':', 0};

    drivespec[0] += drive;

    if (!SetCurrentDirectoryW( drivespec ))
        TRACE( "Failed to set current drive.\n" );
}


/***********************************************************************
 *           INT21_GetSystemCountryCode
 *
 * Return DOS country code for default system locale.
 */
static WORD INT21_GetSystemCountryCode( void )
{
    return GetSystemDefaultLangID();
}


/***********************************************************************
 *           INT21_FillCountryInformation
 *
 * Fill 34-byte buffer with country information data using
 * default system locale.
 */
static void INT21_FillCountryInformation( BYTE *buffer )
{
    /* 00 - WORD: date format
     *          00 = mm/dd/yy
     *          01 = dd/mm/yy
     *          02 = yy/mm/dd
     */
    *(WORD*)(buffer + 0) = 0; /* FIXME: Get from locale */

    /* 02 - BYTE[5]: ASCIIZ currency symbol string */
    buffer[2] = '$'; /* FIXME: Get from locale */
    buffer[3] = 0;

    /* 07 - BYTE[2]: ASCIIZ thousands separator */
    buffer[7] = 0; /* FIXME: Get from locale */
    buffer[8] = 0;

    /* 09 - BYTE[2]: ASCIIZ decimal separator */
    buffer[9]  = '.'; /* FIXME: Get from locale */
    buffer[10] = 0;

    /* 11 - BYTE[2]: ASCIIZ date separator */
    buffer[11] = '/'; /* FIXME: Get from locale */
    buffer[12] = 0;

    /* 13 - BYTE[2]: ASCIIZ time separator */
    buffer[13] = ':'; /* FIXME: Get from locale */
    buffer[14] = 0;

    /* 15 - BYTE: Currency format
     *          bit 2 = set if currency symbol replaces decimal point
     *          bit 1 = number of spaces between value and currency symbol
     *          bit 0 = 0 if currency symbol precedes value
     *                  1 if currency symbol follows value
     */
    buffer[15] = 0; /* FIXME: Get from locale */

    /* 16 - BYTE: Number of digits after decimal in currency */
    buffer[16] = 0; /* FIXME: Get from locale */

    /* 17 - BYTE: Time format
     *          bit 0 = 0 if 12-hour clock
     *                  1 if 24-hour clock
     */
    buffer[17] = 1; /* FIXME: Get from locale */

    /* 18 - DWORD: Address of case map routine */
    *(DWORD*)(buffer + 18) = 0; /* FIXME: ptr to case map routine */

    /* 22 - BYTE[2]: ASCIIZ data-list separator */
    buffer[22] = ','; /* FIXME: Get from locale */
    buffer[23] = 0;

    /* 24 - BYTE[10]: Reserved */
    memset( buffer + 24, 0, 10 );
}


/***********************************************************************
 *           INT21_FillHeap
 *
 * Initialize DOS heap.
 *
 * Filename Terminator Table of w2k DE NTVDM:
 * 16 00 01 00 FF 00 00 20-02 0E 2E 22 2F 5C 5B 5D  ....... ..."/\[]
 * 3A 7C 3C 3E 2B 3D 3B 2C-00                       :|<>+=;,
 */
static void INT21_FillHeap( INT21_HEAP *heap )
{
    static const char terminators[] = ".\"/\\[]:|<>+=;,";
    int i;

    /*
     * Uppercase table.
     */
    heap->uppercase_size = 128;
    for (i = 0; i < 128; i++) 
        heap->uppercase_table[i] = toupper( 128 + i );

    /*
     * Lowercase table.
     */
    heap->lowercase_size = 256;
    for (i = 0; i < 256; i++) 
        heap->lowercase_table[i] = tolower( i );
    
    /*
     * Collating table.
     */
    heap->collating_size = 256;
    for (i = 0; i < 256; i++) 
        heap->collating_table[i] = i;

    /*
     * Filename table.
     */
    heap->filename_size = 8 + strlen(terminators);
    heap->filename_illegal_size = strlen(terminators);
    memcpy( heap->filename_illegal_table, terminators, heap->filename_illegal_size );

    heap->filename_reserved1 = 0x01;
    heap->filename_lowest = 0x00;
    heap->filename_highest = 0xff;
    heap->filename_reserved2 = 0x00;    
    heap->filename_exclude_first = 0x00;
    heap->filename_exclude_last = 0x20;
    heap->filename_reserved3 = 0x02;

    /*
     * DBCS lead byte table. This table is empty.
     */
    heap->dbcs_size = 0;
    memset( heap->dbcs_table, 0, sizeof(heap->dbcs_table) );

    /*
     * Initialize InDos flag.
     */
    heap->misc_indos = 0;

    /*
     * FIXME: Should drive parameter blocks (DPB) be
     *        initialized here and linked to DOS LOL?
     */
}


/***********************************************************************
 *           INT21_GetHeapPointer
 *
 * Get pointer for DOS heap (INT21_HEAP).
 * Creates and initializes heap on first call.
 */
static INT21_HEAP *INT21_GetHeapPointer( void )
{
    static INT21_HEAP *heap_pointer = NULL;

    if (!heap_pointer)
    {
        WORD heap_selector = GlobalAlloc16( GMEM_FIXED, sizeof(INT21_HEAP) );
        heap_pointer = GlobalLock16( heap_selector );
        heap_pointer->misc_selector = heap_selector;
        INT21_FillHeap( heap_pointer );
    }

    return heap_pointer;
}


/***********************************************************************
 *           INT21_GetHeapSelector
 *
 * Get segment/selector for DOS heap (INT21_HEAP).
 * Creates and initializes heap on first call.
 */
static WORD INT21_GetHeapSelector( CONTEXT *context )
{
    INT21_HEAP *heap = INT21_GetHeapPointer();
    return heap->misc_selector;
}


/***********************************************************************
 *           INT21_FillDrivePB
 *
 * Fill DOS heap drive parameter block for the specified drive.
 * Return TRUE if drive was valid and there were
 * no errors while reading drive information.
 */
static BOOL INT21_FillDrivePB( BYTE drive )
{
    WCHAR       drivespec[] = {'A', ':', 0};
    INT21_HEAP *heap = INT21_GetHeapPointer();
    INT21_DPB  *dpb;
    UINT        drivetype;
    DWORD       cluster_sectors;
    DWORD       sector_bytes;
    DWORD       free_clusters;
    DWORD       total_clusters;

    if (drive >= MAX_DOS_DRIVES)
        return FALSE;

    dpb = &heap->misc_dpb_list[drive];
    drivespec[0] += drive;
    drivetype = GetDriveTypeW( drivespec );

    /*
     * FIXME: Does this check work correctly with floppy/cdrom drives?
     */
    if (drivetype == DRIVE_NO_ROOT_DIR || drivetype == DRIVE_UNKNOWN)
        return FALSE;

    /*
     * FIXME: Does this check work correctly with floppy/cdrom drives?
     */
    if (!GetDiskFreeSpaceW( drivespec, &cluster_sectors, &sector_bytes,
                            &free_clusters, &total_clusters ))
        return FALSE;

    /*
     * FIXME: Most of the values listed below are incorrect.
     *        All values should be validated.
     */
 
    dpb->drive           = drive;
    dpb->unit            = 0;
    dpb->sector_bytes    = sector_bytes;
    dpb->cluster_sectors = cluster_sectors - 1;

    dpb->shift = 0;
    while (cluster_sectors > 1)
    {
        cluster_sectors /= 2;
        dpb->shift++;
    }

    dpb->num_reserved         = 0;
    dpb->num_FAT              = 1;
    dpb->num_root_entries     = 2;
    dpb->first_data_sector    = 2;
    dpb->num_clusters1        = total_clusters;
    dpb->sectors_per_FAT      = 1;
    dpb->first_dir_sector     = 1;
    dpb->driver_header        = 0;
    dpb->media_ID             = (drivetype == DRIVE_FIXED) ? 0xF8 : 0xF0;
    dpb->access_flag          = 0;
    dpb->next                 = 0;
    dpb->search_cluster1      = 0;
    dpb->free_clusters_lo     = LOWORD(free_clusters);
    dpb->free_clusters_hi     = HIWORD(free_clusters);
    dpb->mirroring_flags      = 0;
    dpb->info_sector          = 0xffff;
    dpb->spare_boot_sector    = 0xffff;
    dpb->first_cluster_sector = 0;
    dpb->num_clusters2        = total_clusters;
    dpb->fat_clusters         = 32;
    dpb->root_cluster         = 0;
    dpb->search_cluster2      = 0;

    return TRUE;
}


/***********************************************************************
 *           INT21_GetCurrentDirectory
 *
 * Handler for:
 * - function 0x47
 * - subfunction 0x47 of function 0x71
 */
static BOOL INT21_GetCurrentDirectory( CONTEXT *context, BOOL islong )
{
    char  *buffer = ldt_get_ptr(context->SegDs, context->Esi);
    BYTE   drive = INT21_MapDrive( DL_reg(context) );
    WCHAR  pathW[MAX_PATH];
    char   pathA[MAX_PATH];
    WCHAR *ptr = pathW;

    TRACE( "drive %d\n", DL_reg(context) );

    if (drive == MAX_DOS_DRIVES)
    {
        SetLastError(ERROR_INVALID_DRIVE);
        return FALSE;
    }
    
    /*
     * Grab current directory.
     */

    if (!GetCurrentDirectoryW( MAX_PATH, pathW )) return FALSE;

    if (drive_number( pathW[0] ) != drive || pathW[1] != ':')
    {
        /* cwd is not on the requested drive, get the environment string instead */

        WCHAR env_var[4];

        env_var[0] = '=';
        env_var[1] = 'A' + drive;
        env_var[2] = ':';
        env_var[3] = 0;
        if (!GetEnvironmentVariableW( env_var, pathW, MAX_PATH ))
        {
            /* return empty path */
            buffer[0] = 0;
            return TRUE;
        }
    }

    /*
     * Convert into short format.
     */

    if (!islong)
    {
        DWORD result = GetShortPathNameW( pathW, pathW, MAX_PATH );
        if (!result)
            return FALSE;
        if (result > MAX_PATH)
        {
            WARN( "Short path too long!\n" );
            SetLastError(ERROR_NETWORK_BUSY); /* Internal Wine error. */
            return FALSE;
        }
    }

    /*
     * The returned pathname does not include 
     * the drive letter, colon or leading backslash.
     */

    if (ptr[0] == '\\')
    {
        /*
         * FIXME: We should probably just strip host part from name...
         */
        FIXME( "UNC names are not supported.\n" );
        SetLastError(ERROR_NETWORK_BUSY); /* Internal Wine error. */
        return FALSE;
    }
    else if (!ptr[0] || ptr[1] != ':' || ptr[2] != '\\')
    {
        WARN( "Path is neither UNC nor DOS path: %s\n",
              wine_dbgstr_w(ptr) );
        SetLastError(ERROR_NETWORK_BUSY); /* Internal Wine error. */
        return FALSE;
    }
    else
    {
        /* Remove drive letter, colon and leading backslash. */
        ptr += 3;
    }

    /*
     * Convert into OEM string.
     */
    
    if (!WideCharToMultiByte(CP_OEMCP, 0, ptr, -1, pathA, 
                             MAX_PATH, NULL, NULL))
    {
        WARN( "Cannot convert path!\n" );
        SetLastError(ERROR_NETWORK_BUSY); /* Internal Wine error. */
        return FALSE;
    }

    /*
     * Success.
     */

    if (!islong)
    {
        /* Undocumented success code. */
        SET_AX( context, 0x0100 );
        
        /* Truncate buffer to 64 bytes. */
        pathA[63] = 0;
    }

    TRACE( "%c:=%s\n", 'A' + drive, pathA );

    strcpy( buffer, pathA );
    return TRUE;
}


/***********************************************************************
 *           INT21_SetCurrentDirectory
 *
 * Handler for:
 * - function 0x3b
 * - subfunction 0x3b of function 0x71
 */
static BOOL INT21_SetCurrentDirectory( CONTEXT *context )
{
    WCHAR dirW[MAX_PATH];
    WCHAR env_var[4];
    DWORD attr;
    char *dirA = ldt_get_ptr(context->SegDs, context->Edx);
    BYTE  drive = INT21_GetCurrentDrive();
    BOOL  result;

    TRACE( "SET CURRENT DIRECTORY %s\n", dirA );

    MultiByteToWideChar(CP_OEMCP, 0, dirA, -1, dirW, MAX_PATH);
    if (!GetFullPathNameW( dirW, MAX_PATH, dirW, NULL )) return FALSE;

    attr = GetFileAttributesW( dirW );
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    env_var[0] = '=';
    env_var[1] = dirW[0];
    env_var[2] = ':';
    env_var[3] = 0;
    result = SetEnvironmentVariableW( env_var, dirW );

    /* only set current directory if on the current drive */
    if (result && (drive_number( dirW[0] ) == drive)) result = SetCurrentDirectoryW( dirW );

    return result;
}

/***********************************************************************
 *           INT21_CreateFile
 *
 * Handler for:
 * - function 0x3c
 * - function 0x3d
 * - function 0x5b
 * - function 0x6c
 * - subfunction 0x6c of function 0x71
 */
static BOOL INT21_CreateFile( CONTEXT *context,
                              DWORD      pathSegOff,
                              BOOL       returnStatus,
                              WORD       dosAccessShare,
                              BYTE       dosAction )
{
    WORD   dosStatus;
    char  *pathA = ldt_get_ptr(context->SegDs, pathSegOff);
    WCHAR  pathW[MAX_PATH];   
    DWORD  winAccess;
    DWORD  winAttributes;
    HANDLE winHandle;
    DWORD  winMode;
    DWORD  winSharing;

    TRACE( "CreateFile called: function=%02x, action=%02x, access/share=%04x, "
           "create flags=%04x, file=%s.\n",
           AH_reg(context), dosAction, dosAccessShare, CX_reg(context), pathA );

    /*
     * Application tried to create/open a file whose name 
     * ends with a backslash. This is not allowed.
     *
     * FIXME: This needs to be validated, especially the return value.
     */
    if (pathA[strlen(pathA) - 1] == '/')
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
        return FALSE;
    }

    /*
     * Convert DOS action flags into Win32 creation disposition parameter.
     */ 
    switch(dosAction)
    {
    case 0x01:
        winMode = OPEN_EXISTING;
        break;
    case 0x02:
        winMode = TRUNCATE_EXISTING;
        break;
    case 0x10:
        winMode = CREATE_NEW;
        break;
    case 0x11:
        winMode = OPEN_ALWAYS;
        break;
    case 0x12:
        winMode = CREATE_ALWAYS;
        break;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    /*
     * Convert DOS access/share flags into Win32 desired access parameter.
     */ 
    switch(dosAccessShare & 0x07)
    {
    case OF_READ:
        winAccess = GENERIC_READ;
        break;
    case OF_WRITE:
        winAccess = GENERIC_WRITE;
        break;
    case OF_READWRITE:
        winAccess = GENERIC_READ | GENERIC_WRITE;
        break;
    case 0x04:
        /*
         * Read-only, do not modify file's last-access time (DOS7).
         *
         * FIXME: How to prevent modification of last-access time?
         */
        winAccess = GENERIC_READ;
        break;
    default:
        winAccess = 0;
    }

    /*
     * Convert DOS access/share flags into Win32 share mode parameter.
     */ 
    switch(dosAccessShare & 0x70)
    {
    case OF_SHARE_EXCLUSIVE:  
        winSharing = 0; 
        break;
    case OF_SHARE_DENY_WRITE: 
        winSharing = FILE_SHARE_READ; 
        break;
    case OF_SHARE_DENY_READ:  
        winSharing = FILE_SHARE_WRITE; 
        break;
    case OF_SHARE_DENY_NONE:
    case OF_SHARE_COMPAT:
    default:
        winSharing = FILE_SHARE_READ | FILE_SHARE_WRITE;
    }

    /*
     * FIXME: Bit (dosAccessShare & 0x80) represents inheritance.
     *        What to do with this bit?
     * FIXME: Bits in the high byte of dosAccessShare are not supported.
     *        See both function 0x6c and subfunction 0x6c of function 0x71 for
     *        definition of these bits.
     */

    /*
     * Convert DOS create attributes into Win32 flags and attributes parameter.
     */
    if (winMode == OPEN_EXISTING || winMode == TRUNCATE_EXISTING)
    {
        winAttributes = 0;
    }
    else
    {        
        WORD dosAttributes = CX_reg(context);

        if (dosAttributes & FA_LABEL)
        {
            /*
             * Application tried to create volume label entry.
             * This is difficult to support so we do not allow it.
             *
             * FIXME: If volume does not already have a label, 
             *        this function is supposed to succeed.
             */
            SetLastError( ERROR_ACCESS_DENIED );
            return TRUE;
        }

        winAttributes = dosAttributes & 
            (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | 
             FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE);
    }

    /*
     * Open the file.
     */
    MultiByteToWideChar(CP_OEMCP, 0, pathA, -1, pathW, MAX_PATH);

    winHandle = CreateFileW( pathW, winAccess, winSharing, NULL, winMode, winAttributes, 0 );
    /* DOS allows opening files on a CDROM R/W */
    if( winHandle == INVALID_HANDLE_VALUE &&
        (GetLastError() == ERROR_WRITE_PROTECT ||
         GetLastError() == ERROR_ACCESS_DENIED)) {
        winHandle = CreateFileW( pathW, winAccess & ~GENERIC_WRITE,
                                 winSharing, NULL, winMode, winAttributes, 0 );
    }

    if (winHandle == INVALID_HANDLE_VALUE)
        return FALSE;

    /*
     * Determine DOS file status.
     *
     * 1 = file opened
     * 2 = file created
     * 3 = file replaced
     */
    switch(winMode)
    {
    case OPEN_EXISTING:
        dosStatus = 1;
        break;
    case TRUNCATE_EXISTING:
        dosStatus = 3;
        break;
    case CREATE_NEW:
        dosStatus = 2;
        break;
    case OPEN_ALWAYS:
        dosStatus = (GetLastError() == ERROR_ALREADY_EXISTS) ? 1 : 2;
        break;
    case CREATE_ALWAYS:
        dosStatus = (GetLastError() == ERROR_ALREADY_EXISTS) ? 3 : 2;
        break;
    default:
        dosStatus = 0;
    }

    /*
     * Return DOS file handle and DOS status.
     */
    SET_AX( context, Win32HandleToDosFileHandle(winHandle) );

    if (returnStatus)
        SET_CX( context, dosStatus );

    TRACE( "CreateFile finished: handle=%d, status=%d.\n", 
           AX_reg(context), dosStatus );

    return TRUE;
}


/***********************************************************************
 *           INT21_GetCurrentDTA
 */
static BYTE *INT21_GetCurrentDTA( CONTEXT *context )
{
    TDB *pTask = GlobalLock16(GetCurrentTask());

    /* FIXME: This assumes DTA was set correctly! */
    return ldt_get_ptr( SELECTOROF(pTask->dta), OFFSETOF(pTask->dta) );
}


/***********************************************************************
 *           INT21_OpenFileUsingFCB
 *
 * Handler for function 0x0f.
 *
 * PARAMS
 *  DX:DX [I/O] File control block (FCB or XFCB) of unopened file
 *
 * RETURNS (in AL)
 *  0x00: successful
 *  0xff: failed
 *
 * NOTES
 *  Opens a FCB file for read/write in compatibility mode. Upon calling
 *  the FCB must have the drive_number, file_name, and file_extension
 *  fields filled and all other bytes cleared.
 */
static void INT21_OpenFileUsingFCB( CONTEXT *context )
{
    struct FCB *fcb;
    struct XFCB *xfcb;
    char file_path[16];
    char *pos;
    HANDLE handle;
    HFILE16 hfile16;
    BY_HANDLE_FILE_INFORMATION info;
    BYTE AL_result;

    fcb = ldt_get_ptr(context->SegDs, context->Edx);
    if (fcb->drive_number == 0xff) {
        xfcb = (struct XFCB *) fcb;
        fcb = (struct FCB *) xfcb->fcb;
    } /* if */

    AL_result = 0;
    file_path[0] = 'A' + INT21_MapDrive( fcb->drive_number );

    if (AL_result == 0) {
        file_path[1] = ':';
        pos = &file_path[2];
        memcpy(pos, fcb->file_name, 8);
        pos[8] = ' ';
        pos[9] = '\0';
        pos = strchr(pos, ' ');
        *pos = '.';
        pos++;
        memcpy(pos, fcb->file_extension, 3);
        pos[3] = ' ';
        pos[4] = '\0';
        pos = strchr(pos, ' ');
        *pos = '\0';

        handle = (HANDLE) _lopen(file_path, OF_READWRITE);
        if (handle == INVALID_HANDLE_VALUE) {
            TRACE("_lopen(\"%s\") failed: INVALID_HANDLE_VALUE\n", file_path);
            AL_result = 0xff; /* failed */
        } else {
            hfile16 = Win32HandleToDosFileHandle(handle);
            if (hfile16 == HFILE_ERROR16) {
                TRACE("Win32HandleToDosFileHandle(%p) failed: HFILE_ERROR\n", handle);
                CloseHandle(handle);
                AL_result = 0xff; /* failed */
            } else if (hfile16 > 255) {
                TRACE("hfile16 (=%d) larger than 255 for \"%s\"\n", hfile16, file_path);
                _lclose16(hfile16);
                AL_result = 0xff; /* failed */
            } else {
                if (!GetFileInformationByHandle(handle, &info)) {
                    TRACE("GetFileInformationByHandle(%d, %p) for \"%s\" failed\n",
                          hfile16, handle, file_path);
                    _lclose16(hfile16);
                    AL_result = 0xff; /* failed */
                } else {
                    fcb->drive_number = file_path[0] - 'A' + 1;
                    fcb->current_block_number = 0;
                    fcb->logical_record_size = 128;
                    fcb->file_size = info.nFileSizeLow;
                    FileTimeToDosDateTime(&info.ftLastWriteTime,
                        &fcb->date_of_last_write, &fcb->time_of_last_write);
                    fcb->file_number = hfile16;
                    fcb->attributes = 0xc2;
                    fcb->starting_cluster = 0; /* don't know correct init value */
                    fcb->sequence_number = 0; /* don't know correct init value */
                    fcb->file_attributes = info.dwFileAttributes;
                    /* The following fields are not initialized */
                    /* by the native function: */
                    /* unused */
                    /* record_within_current_block */
                    /* random_access_record_number */

                    TRACE("successful opened file \"%s\" as %d (handle %p)\n",
                          file_path, hfile16, handle);
                    AL_result = 0x00; /* successful */
                } /* if */
            } /* if */
        } /* if */
    } /* if */
    SET_AL(context, AL_result);
}


/***********************************************************************
 *           INT21_CloseFileUsingFCB
 *
 * Handler for function 0x10.
 *
 * PARAMS
 *  DX:DX [I/O] File control block (FCB or XFCB) of open file
 *
 * RETURNS (in AL)
 *  0x00: successful
 *  0xff: failed
 *
 * NOTES
 *  Closes a FCB file.
 */
static void INT21_CloseFileUsingFCB( CONTEXT *context )
{
    struct FCB *fcb;
    struct XFCB *xfcb;
    BYTE AL_result;

    fcb = ldt_get_ptr(context->SegDs, context->Edx);
    if (fcb->drive_number == 0xff) {
        xfcb = (struct XFCB *) fcb;
        fcb = (struct FCB *) xfcb->fcb;
    } /* if */

    if (_lclose16((HFILE16) fcb->file_number) != 0) {
        TRACE("_lclose16(%d) failed\n", fcb->file_number);
        AL_result = 0xff; /* failed */
    } else {
        TRACE("successful closed file %d\n", fcb->file_number);
        AL_result = 0x00; /* successful */
    } /* if */
    SET_AL(context, AL_result);
}


/***********************************************************************
 *           INT21_SequentialReadFromFCB
 *
 * Handler for function 0x14.
 *
 * PARAMS
 *  DX:DX [I/O] File control block (FCB or XFCB) of open file
 *
 * RETURNS (in AL)
 *  0: successful
 *  1: end of file, no data read
 *  2: segment wrap in DTA, no data read (not returned now)
 *  3: end of file, partial record read
 *
 * NOTES
 *  Reads a record with the size FCB->logical_record_size from the FCB
 *  to the disk transfer area. The position of the record is specified
 *  with FCB->current_block_number and FCB->record_within_current_block.
 *  Then FCB->current_block_number and FCB->record_within_current_block
 *  are updated to point to the next record. If a partial record is
 *  read, it is filled with zeros up to the FCB->logical_record_size.
 */
static void INT21_SequentialReadFromFCB( CONTEXT *context )
{
    struct FCB *fcb;
    struct XFCB *xfcb;
    HANDLE handle;
    DWORD record_number;
    DWORD position;
    BYTE *disk_transfer_area;
    UINT bytes_read;
    BYTE AL_result;

    fcb = ldt_get_ptr(context->SegDs, context->Edx);
    if (fcb->drive_number == 0xff) {
        xfcb = (struct XFCB *) fcb;
        fcb = (struct FCB *) xfcb->fcb;
    } /* if */

    handle = DosFileHandleToWin32Handle((HFILE16) fcb->file_number);
    if (handle == INVALID_HANDLE_VALUE) {
        TRACE("DosFileHandleToWin32Handle(%d) failed: INVALID_HANDLE_VALUE\n",
            fcb->file_number);
        AL_result = 0x01; /* end of file, no data read */
    } else {
        record_number = 128 * fcb->current_block_number + fcb->record_within_current_block;
        position = SetFilePointer(handle, record_number * fcb->logical_record_size, NULL, 0);
        if (position != record_number * fcb->logical_record_size) {
            TRACE("seek(%d, %ld, 0) failed with %lu\n",
                  fcb->file_number, record_number * fcb->logical_record_size, position);
            AL_result = 0x01; /* end of file, no data read */
        } else {
            disk_transfer_area = INT21_GetCurrentDTA(context);
            bytes_read = _lread((HFILE) handle, disk_transfer_area, fcb->logical_record_size);
            if (bytes_read != fcb->logical_record_size) {
                TRACE("_lread(%d, %p, %d) failed with %d\n",
                      fcb->file_number, disk_transfer_area, fcb->logical_record_size, bytes_read);
                if (bytes_read == 0) {
                    AL_result = 0x01; /* end of file, no data read */
                } else {
                    memset(&disk_transfer_area[bytes_read], 0, fcb->logical_record_size - bytes_read);
                    AL_result = 0x03; /* end of file, partial record read */
                } /* if */
            } else {
                TRACE("successful read %d bytes from record %ld (position %lu) of file %d (handle %p)\n",
                    bytes_read, record_number, position, fcb->file_number, handle);
                AL_result = 0x00; /* successful */
            } /* if */
        } /* if */
    } /* if */
    if (AL_result == 0x00 || AL_result == 0x03) {
        if (fcb->record_within_current_block == 127) {
            fcb->record_within_current_block = 0;
            fcb->current_block_number++;
        } else {
            fcb->record_within_current_block++;
        } /* if */
    } /* if */
    SET_AL(context, AL_result);
}


/***********************************************************************
 *           INT21_SequentialWriteToFCB
 *
 * Handler for function 0x15.
 *
 * PARAMS
 *  DX:DX [I/O] File control block (FCB or XFCB) of open file
 *
 * RETURNS (in AL)
 *  0: successful
 *  1: disk full
 *  2: segment wrap in DTA (not returned now)
 *
 * NOTES
 *  Writes a record with the size FCB->logical_record_size from the disk
 *  transfer area to the FCB. The position of the record is specified
 *  with FCB->current_block_number and FCB->record_within_current_block.
 *  Then FCB->current_block_number and FCB->record_within_current_block
 *  are updated to point to the next record. 
 */
static void INT21_SequentialWriteToFCB( CONTEXT *context )
{
    struct FCB *fcb;
    struct XFCB *xfcb;
    HANDLE handle;
    DWORD record_number;
    DWORD position;
    BYTE *disk_transfer_area;
    UINT bytes_written;
    BYTE AL_result;

    fcb = ldt_get_ptr(context->SegDs, context->Edx);
    if (fcb->drive_number == 0xff) {
        xfcb = (struct XFCB *) fcb;
        fcb = (struct FCB *) xfcb->fcb;
    } /* if */

    handle = DosFileHandleToWin32Handle((HFILE16) fcb->file_number);
    if (handle == INVALID_HANDLE_VALUE) {
        TRACE("DosFileHandleToWin32Handle(%d) failed: INVALID_HANDLE_VALUE\n",
            fcb->file_number);
        AL_result = 0x01; /* disk full */
    } else {
        record_number = 128 * fcb->current_block_number + fcb->record_within_current_block;
        position = SetFilePointer(handle, record_number * fcb->logical_record_size, NULL, 0);
        if (position != record_number * fcb->logical_record_size) {
            TRACE("seek(%d, %ld, 0) failed with %lu\n",
                  fcb->file_number, record_number * fcb->logical_record_size, position);
            AL_result = 0x01; /* disk full */
        } else {
            disk_transfer_area = INT21_GetCurrentDTA(context);
            bytes_written = _lwrite((HFILE) handle, (LPCSTR)disk_transfer_area, fcb->logical_record_size);
            if (bytes_written != fcb->logical_record_size) {
                TRACE("_lwrite(%d, %p, %d) failed with %d\n",
                      fcb->file_number, disk_transfer_area, fcb->logical_record_size, bytes_written);
                AL_result = 0x01; /* disk full */
            } else {
                TRACE("successful written %d bytes from record %ld (position %lu) of file %d (handle %p)\n",
                    bytes_written, record_number, position, fcb->file_number, handle);
                AL_result = 0x00; /* successful */
            } /* if */
        } /* if */
    } /* if */
    if (AL_result == 0x00) {
        if (fcb->record_within_current_block == 127) {
            fcb->record_within_current_block = 0;
            fcb->current_block_number++;
        } else {
            fcb->record_within_current_block++;
        } /* if */
    } /* if */
    SET_AL(context, AL_result);
}


/***********************************************************************
 *           INT21_ReadRandomRecordFromFCB
 *
 * Handler for function 0x21.
 *
 * PARAMS
 *  DX:DX [I/O] File control block (FCB or XFCB) of open file
 *
 * RETURNS (in AL)
 *  0: successful
 *  1: end of file, no data read
 *  2: segment wrap in DTA, no data read (not returned now)
 *  3: end of file, partial record read
 *
 * NOTES
 *  Reads a record with the size FCB->logical_record_size from
 *  the FCB to the disk transfer area. The position of the record
 *  is specified with FCB->random_access_record_number. The
 *  FCB->random_access_record_number is not updated. If a partial record
 *  is read, it is filled with zeros up to the FCB->logical_record_size.
 */
static void INT21_ReadRandomRecordFromFCB( CONTEXT *context )
{
    struct FCB *fcb;
    struct XFCB *xfcb;
    HANDLE handle;
    DWORD record_number;
    DWORD position;
    BYTE *disk_transfer_area;
    UINT bytes_read;
    BYTE AL_result;

    fcb = ldt_get_ptr(context->SegDs, context->Edx);
    if (fcb->drive_number == 0xff) {
        xfcb = (struct XFCB *) fcb;
        fcb = (struct FCB *) xfcb->fcb;
    } /* if */

    memcpy(&record_number, fcb->random_access_record_number, 4);
    handle = DosFileHandleToWin32Handle((HFILE16) fcb->file_number);
    if (handle == INVALID_HANDLE_VALUE) {
        TRACE("DosFileHandleToWin32Handle(%d) failed: INVALID_HANDLE_VALUE\n",
            fcb->file_number);
        AL_result = 0x01; /* end of file, no data read */
    } else {
        position = SetFilePointer(handle, record_number * fcb->logical_record_size, NULL, 0);
        if (position != record_number * fcb->logical_record_size) {
            TRACE("seek(%d, %ld, 0) failed with %lu\n",
                  fcb->file_number, record_number * fcb->logical_record_size, position);
            AL_result = 0x01; /* end of file, no data read */
        } else {
            disk_transfer_area = INT21_GetCurrentDTA(context);
            bytes_read = _lread((HFILE) handle, disk_transfer_area, fcb->logical_record_size);
            if (bytes_read != fcb->logical_record_size) {
                TRACE("_lread(%d, %p, %d) failed with %d\n",
                      fcb->file_number, disk_transfer_area, fcb->logical_record_size, bytes_read);
                if (bytes_read == 0) {
                    AL_result = 0x01; /* end of file, no data read */
                } else {
                    memset(&disk_transfer_area[bytes_read], 0, fcb->logical_record_size - bytes_read);
                    AL_result = 0x03; /* end of file, partial record read */
                } /* if */
            } else {
                TRACE("successful read %d bytes from record %ld (position %lu) of file %d (handle %p)\n",
                    bytes_read, record_number, position, fcb->file_number, handle);
                AL_result = 0x00; /* successful */
            } /* if */
        } /* if */
    } /* if */
    fcb->current_block_number = record_number / 128;
    fcb->record_within_current_block = record_number % 128;
    SET_AL(context, AL_result);
}


/***********************************************************************
 *           INT21_WriteRandomRecordToFCB
 *
 * Handler for function 0x22.
 *
 * PARAMS
 *  DX:DX [I/O] File control block (FCB or XFCB) of open file
 *
 * RETURNS (in AL)
 *  0: successful
 *  1: disk full
 *  2: segment wrap in DTA (not returned now)
 *
 * NOTES
 *  Writes a record with the size FCB->logical_record_size from
 *  the disk transfer area to the FCB. The position of the record
 *  is specified with FCB->random_access_record_number. The
 *  FCB->random_access_record_number is not updated.
 */
static void INT21_WriteRandomRecordToFCB( CONTEXT *context )
{
    struct FCB *fcb;
    struct XFCB *xfcb;
    HANDLE handle;
    DWORD record_number;
    DWORD position;
    BYTE *disk_transfer_area;
    UINT bytes_written;
    BYTE AL_result;

    fcb = ldt_get_ptr(context->SegDs, context->Edx);
    if (fcb->drive_number == 0xff) {
        xfcb = (struct XFCB *) fcb;
        fcb = (struct FCB *) xfcb->fcb;
    } /* if */

    memcpy(&record_number, fcb->random_access_record_number, 4);
    handle = DosFileHandleToWin32Handle((HFILE16) fcb->file_number);
    if (handle == INVALID_HANDLE_VALUE) {
        TRACE("DosFileHandleToWin32Handle(%d) failed: INVALID_HANDLE_VALUE\n",
            fcb->file_number);
        AL_result = 0x01; /* disk full */
    } else {
        position = SetFilePointer(handle, record_number * fcb->logical_record_size, NULL, 0);
        if (position != record_number * fcb->logical_record_size) {
            TRACE("seek(%d, %ld, 0) failed with %lu\n",
                  fcb->file_number, record_number * fcb->logical_record_size, position);
            AL_result = 0x01; /* disk full */
        } else {
            disk_transfer_area = INT21_GetCurrentDTA(context);
            bytes_written = _lwrite((HFILE) handle, (LPCSTR)disk_transfer_area, fcb->logical_record_size);
            if (bytes_written != fcb->logical_record_size) {
                TRACE("_lwrite(%d, %p, %d) failed with %d\n",
                      fcb->file_number, disk_transfer_area, fcb->logical_record_size, bytes_written);
                AL_result = 0x01; /* disk full */
            } else {
                TRACE("successful written %d bytes from record %ld (position %lu) of file %d (handle %p)\n",
                    bytes_written, record_number, position, fcb->file_number, handle);
                AL_result = 0x00; /* successful */
            } /* if */
        } /* if */
    } /* if */
    fcb->current_block_number = record_number / 128;
    fcb->record_within_current_block = record_number % 128;
    SET_AL(context, AL_result);
}


/***********************************************************************
 *           INT21_RandomBlockReadFromFCB
 *
 * Handler for function 0x27.
 *
 * PARAMS
 *  CX    [I/O] Number of records to read
 *  DX:DX [I/O] File control block (FCB or XFCB) of open file
 *
 * RETURNS (in AL)
 *  0: successful
 *  1: end of file, no data read
 *  2: segment wrap in DTA, no data read (not returned now)
 *  3: end of file, partial record read
 *
 * NOTES
 *  Reads several records with the size FCB->logical_record_size from
 *  the FCB to the disk transfer area. The number of records to be
 *  read is specified in the CX register. The position of the first
 *  record is specified with FCB->random_access_record_number. The
 *  FCB->random_access_record_number, the FCB->current_block_number
 *  and FCB->record_within_current_block are updated to point to the
 *  next record after the records read. If a partial record is read,
 *  it is filled with zeros up to the FCB->logical_record_size. The
 *  CX register is set to the number of successfully read records.
 */
static void INT21_RandomBlockReadFromFCB( CONTEXT *context )
{
    struct FCB *fcb;
    struct XFCB *xfcb;
    HANDLE handle;
    DWORD record_number;
    DWORD position;
    BYTE *disk_transfer_area;
    UINT records_requested;
    UINT bytes_requested;
    UINT bytes_read;
    UINT records_read;
    BYTE AL_result;

    fcb = ldt_get_ptr(context->SegDs, context->Edx);
    if (fcb->drive_number == 0xff) {
        xfcb = (struct XFCB *) fcb;
        fcb = (struct FCB *) xfcb->fcb;
    } /* if */

    memcpy(&record_number, fcb->random_access_record_number, 4);
    handle = DosFileHandleToWin32Handle((HFILE16) fcb->file_number);
    if (handle == INVALID_HANDLE_VALUE) {
        TRACE("DosFileHandleToWin32Handle(%d) failed: INVALID_HANDLE_VALUE\n",
            fcb->file_number);
        records_read = 0;
        AL_result = 0x01; /* end of file, no data read */
    } else {
        position = SetFilePointer(handle, record_number * fcb->logical_record_size, NULL, 0);
        if (position != record_number * fcb->logical_record_size) {
            TRACE("seek(%d, %ld, 0) failed with %lu\n",
                  fcb->file_number, record_number * fcb->logical_record_size, position);
            records_read = 0;
            AL_result = 0x01; /* end of file, no data read */
        } else {
            disk_transfer_area = INT21_GetCurrentDTA(context);
            records_requested = CX_reg(context);
            bytes_requested = records_requested * fcb->logical_record_size;
            bytes_read = _lread((HFILE) handle, disk_transfer_area, bytes_requested);
            if (bytes_read != bytes_requested) {
                TRACE("_lread(%d, %p, %d) failed with %d\n",
                      fcb->file_number, disk_transfer_area, bytes_requested, bytes_read);
                records_read = bytes_read / fcb->logical_record_size;
                if (bytes_read % fcb->logical_record_size == 0) {
                    AL_result = 0x01; /* end of file, no data read */
                } else {
                    records_read++;
                    memset(&disk_transfer_area[bytes_read], 0, records_read * fcb->logical_record_size - bytes_read);
                    AL_result = 0x03; /* end of file, partial record read */
                } /* if */
            } else {
                TRACE("successful read %d bytes from record %ld (position %lu) of file %d (handle %p)\n",
                    bytes_read, record_number, position, fcb->file_number, handle);
                records_read = records_requested;
                AL_result = 0x00; /* successful */
            } /* if */
        } /* if */
    } /* if */
    record_number += records_read;
    memcpy(fcb->random_access_record_number, &record_number, 4);
    fcb->current_block_number = record_number / 128;
    fcb->record_within_current_block = record_number % 128;
    SET_CX(context, records_read);
    SET_AL(context, AL_result);
}


/***********************************************************************
 *           INT21_RandomBlockWriteToFCB
 *
 * Handler for function 0x28.
 *
 * PARAMS
 *  CX    [I/O] Number of records to write
 *  DX:DX [I/O] File control block (FCB or XFCB) of open file
 *
 * RETURNS (in AL)
 *  0: successful
 *  1: disk full
 *  2: segment wrap in DTA (not returned now)
 *
 * NOTES
 *  Writes several records with the size FCB->logical_record_size from
 *  the disk transfer area to the FCB. The number of records to be
 *  written is specified in the CX register. The position of the first
 *  record is specified with FCB->random_access_record_number. The
 *  FCB->random_access_record_number, the FCB->current_block_number
 *  and FCB->record_within_current_block are updated to point to the
 *  next record after the records written. The CX register is set to
 *  the number of successfully written records.
 */
static void INT21_RandomBlockWriteToFCB( CONTEXT *context )
{
    struct FCB *fcb;
    struct XFCB *xfcb;
    HANDLE handle;
    DWORD record_number;
    DWORD position;
    BYTE *disk_transfer_area;
    UINT records_requested;
    UINT bytes_requested;
    UINT bytes_written;
    UINT records_written;
    BYTE AL_result;

    fcb = ldt_get_ptr(context->SegDs, context->Edx);
    if (fcb->drive_number == 0xff) {
        xfcb = (struct XFCB *) fcb;
        fcb = (struct FCB *) xfcb->fcb;
    } /* if */

    memcpy(&record_number, fcb->random_access_record_number, 4);
    handle = DosFileHandleToWin32Handle((HFILE16) fcb->file_number);
    if (handle == INVALID_HANDLE_VALUE) {
        TRACE("DosFileHandleToWin32Handle(%d) failed: INVALID_HANDLE_VALUE\n",
            fcb->file_number);
        records_written = 0;
        AL_result = 0x01; /* disk full */
    } else {
        position = SetFilePointer(handle, record_number * fcb->logical_record_size, NULL, 0);
        if (position != record_number * fcb->logical_record_size) {
            TRACE("seek(%d, %ld, 0) failed with %lu\n",
                  fcb->file_number, record_number * fcb->logical_record_size, position);
            records_written = 0;
            AL_result = 0x01; /* disk full */
        } else {
            disk_transfer_area = INT21_GetCurrentDTA(context);
            records_requested = CX_reg(context);
            bytes_requested = records_requested * fcb->logical_record_size;
            bytes_written = _lwrite((HFILE) handle, (LPCSTR)disk_transfer_area, bytes_requested);
            if (bytes_written != bytes_requested) {
                TRACE("_lwrite(%d, %p, %d) failed with %d\n",
                      fcb->file_number, disk_transfer_area, bytes_requested, bytes_written);
                records_written = bytes_written / fcb->logical_record_size;
                AL_result = 0x01; /* disk full */
            } else {
                TRACE("successful write %d bytes from record %ld (position %lu) of file %d (handle %p)\n",
                    bytes_written, record_number, position, fcb->file_number, handle);
                records_written = records_requested;
                AL_result = 0x00; /* successful */
            } /* if */
        } /* if */
    } /* if */
    record_number += records_written;
    memcpy(fcb->random_access_record_number, &record_number, 4);
    fcb->current_block_number = record_number / 128;
    fcb->record_within_current_block = record_number % 128;
    SET_CX(context, records_written);
    SET_AL(context, AL_result);
}


/***********************************************************************
 *           INT21_CreateDirectory
 *
 * Handler for:
 * - function 0x39
 * - subfunction 0x39 of function 0x71
 * - subfunction 0xff of function 0x43 (CL == 0x39)
 */
static BOOL INT21_CreateDirectory( CONTEXT *context )
{
    WCHAR dirW[MAX_PATH];
    char *dirA = ldt_get_ptr(context->SegDs, context->Edx);

    TRACE( "CREATE DIRECTORY %s\n", dirA );

    MultiByteToWideChar(CP_OEMCP, 0, dirA, -1, dirW, MAX_PATH);

    if (CreateDirectoryW(dirW, NULL))
        return TRUE;

    /*
     * FIXME: CreateDirectory's LastErrors will clash with the ones
     *        used by DOS. AH=39 only returns 3 (path not found) and 
     *        5 (access denied), while CreateDirectory return several
     *        ones. Remap some of them. -Marcus
     */
    switch (GetLastError()) 
    {
    case ERROR_ALREADY_EXISTS:
    case ERROR_FILENAME_EXCED_RANGE:
    case ERROR_DISK_FULL:
        SetLastError(ERROR_ACCESS_DENIED);
        break;
    default: 
        break;
    }

    return FALSE;
}


/***********************************************************************
 *           INT21_ExtendedCountryInformation
 *
 * Handler for function 0x65.
 */
static void INT21_ExtendedCountryInformation( CONTEXT *context )
{
    BYTE *dataptr = ldt_get_ptr( context->SegEs, context->Edi );
    BYTE buffsize = CX_reg (context);
    
    TRACE( "GET EXTENDED COUNTRY INFORMATION, subfunction %02x\n",
           AL_reg(context) );

    /*
     * Check subfunctions that are passed country and code page.
     */
    if (AL_reg(context) >= 0x01 && AL_reg(context) <= 0x07)
    {
        WORD country = DX_reg(context);
        WORD codepage = BX_reg(context);

        if (country != 0xffff && country != INT21_GetSystemCountryCode())
            FIXME( "Requested info on non-default country %04x\n", country );

        if (codepage != 0xffff && codepage != GetOEMCP())
            FIXME( "Requested info on non-default code page %04x\n", codepage );
    }

    switch (AL_reg(context)) {
    case 0x00: /* SET GENERAL INTERNATIONALIZATION INFO */
        INT_BARF( context, 0x21 );
        SET_CFLAG( context );
        break;

    case 0x01: /* GET GENERAL INTERNATIONALIZATION INFO */
        TRACE( "Get general internationalization info\n" );
        dataptr[0] = 0x01; /* Info ID */
        *(WORD*)(dataptr+1) = 38; /* Size of the following info */
        *(WORD*)(dataptr+3) = INT21_GetSystemCountryCode(); /* Country ID */
        *(WORD*)(dataptr+5) = GetOEMCP(); /* Code page */
        /* FIXME: fill buffer partially up to buffsize bytes*/
        if (buffsize >= 0x29){
            INT21_FillCountryInformation( dataptr + 7 );
            SET_CX( context, 0x29 ); /* Size of returned info */
        }else{
            SET_CX( context, 0x07 ); /* Size of returned info */        
        }
        break;
        
    case 0x02: /* GET POINTER TO UPPERCASE TABLE */
    case 0x04: /* GET POINTER TO FILENAME UPPERCASE TABLE */
        TRACE( "Get pointer to uppercase table\n" );
        dataptr[0] = AL_reg(context); /* Info ID */
        *(DWORD*)(dataptr+1) = MAKESEGPTR( INT21_GetHeapSelector( context ),
                                           offsetof(INT21_HEAP, uppercase_size) );
        SET_CX( context, 5 ); /* Size of returned info */
        break;

    case 0x03: /* GET POINTER TO LOWERCASE TABLE */
        TRACE( "Get pointer to lowercase table\n" );
        dataptr[0] = 0x03; /* Info ID */
        *(DWORD*)(dataptr+1) = MAKESEGPTR( INT21_GetHeapSelector( context ),
                                           offsetof(INT21_HEAP, lowercase_size) );
        SET_CX( context, 5 ); /* Size of returned info */
        break;

    case 0x05: /* GET POINTER TO FILENAME TERMINATOR TABLE */
        TRACE("Get pointer to filename terminator table\n");
        dataptr[0] = 0x05; /* Info ID */
        *(DWORD*)(dataptr+1) = MAKESEGPTR( INT21_GetHeapSelector( context ),
                                           offsetof(INT21_HEAP, filename_size) );
        SET_CX( context, 5 ); /* Size of returned info */
        break;

    case 0x06: /* GET POINTER TO COLLATING SEQUENCE TABLE */
        TRACE("Get pointer to collating sequence table\n");
        dataptr[0] = 0x06; /* Info ID */
        *(DWORD*)(dataptr+1) = MAKESEGPTR( INT21_GetHeapSelector( context ),
                                           offsetof(INT21_HEAP, collating_size) );
        SET_CX( context, 5 ); /* Size of returned info */
        break;

    case 0x07: /* GET POINTER TO DBCS LEAD BYTE TABLE */
        TRACE("Get pointer to DBCS lead byte table\n");
        dataptr[0] = 0x07; /* Info ID */
        *(DWORD*)(dataptr+1) = MAKESEGPTR( INT21_GetHeapSelector( context ),
                                           offsetof(INT21_HEAP, dbcs_size) );
        SET_CX( context, 5 ); /* Size of returned info */
        break;

    case 0x20: /* CAPITALIZE CHARACTER */
    case 0xa0: /* CAPITALIZE FILENAME CHARACTER */
        TRACE("Convert char to uppercase\n");
        SET_DL( context, toupper(DL_reg(context)) );
        break;

    case 0x21: /* CAPITALIZE STRING */
    case 0xa1: /* CAPITALIZE COUNTED FILENAME STRING */
        TRACE("Convert string to uppercase with length\n");
        {
            char *ptr = ldt_get_ptr( context->SegDs, context->Edx );
            WORD len = CX_reg(context);
            while (len--) { *ptr = toupper(*ptr); ptr++; }
        }
        break;

    case 0x22: /* CAPITALIZE ASCIIZ STRING */
    case 0xa2: /* CAPITALIZE ASCIIZ FILENAME */
        TRACE("Convert ASCIIZ string to uppercase\n");
        {
            char *p = ldt_get_ptr( context->SegDs, context->Edx );
            for ( ; *p; p++) *p = toupper(*p);
        }
        break;

    case 0x23: /* DETERMINE IF CHARACTER REPRESENTS YES/NO RESPONSE */
        INT_BARF( context, 0x21 );
        SET_CFLAG( context );
        break;

    default:
        INT_BARF( context, 0x21 );
        SET_CFLAG(context);
        break;
    }
}


/***********************************************************************
 *           INT21_FileAttributes
 *
 * Handler for:
 * - function 0x43
 * - subfunction 0x43 of function 0x71
 */
static BOOL INT21_FileAttributes( CONTEXT *context,
                                  BYTE       subfunction,
                                  BOOL       islong )
{
    WCHAR fileW[MAX_PATH];
    char *fileA = ldt_get_ptr(context->SegDs, context->Edx);
    HANDLE   handle;
    BOOL     status;
    FILETIME filetime;
    DWORD    result;
    WORD     date, time;
    int      len;

    switch (subfunction)
    {
    case 0x00: /* GET FILE ATTRIBUTES */
        TRACE( "GET FILE ATTRIBUTES for %s\n", fileA );
        len = MultiByteToWideChar(CP_OEMCP, 0, fileA, -1, fileW, MAX_PATH);

        /* Winbench 96 Disk Test fails if we don't complain
         * about a filename that ends in \
         */
        if (!len || (fileW[len-1] == '/') || (fileW[len-1] == '\\'))
            return FALSE;

        result = GetFileAttributesW( fileW );
        if (result == INVALID_FILE_ATTRIBUTES)
            return FALSE;
        else
        {
            SET_CX( context, (WORD)result );
            if (!islong)
                SET_AX( context, (WORD)result ); /* DR DOS */
        }
        break;

    case 0x01: /* SET FILE ATTRIBUTES */
        TRACE( "SET FILE ATTRIBUTES 0x%02x for %s\n", 
               CX_reg(context), fileA );
        MultiByteToWideChar(CP_OEMCP, 0, fileA, -1, fileW, MAX_PATH);

        if (!SetFileAttributesW( fileW, CX_reg(context) ))
            return FALSE;
        break;

    case 0x02: /* GET COMPRESSED FILE SIZE */
        TRACE( "GET COMPRESSED FILE SIZE for %s\n", fileA );
        MultiByteToWideChar(CP_OEMCP, 0, fileA, -1, fileW, MAX_PATH);

        result = GetCompressedFileSizeW( fileW, NULL );
        if (result == INVALID_FILE_SIZE)
            return FALSE;
        else
        {
            SET_AX( context, LOWORD(result) );
            SET_DX( context, HIWORD(result) );
        }
        break;

    case 0x03: /* SET FILE LAST-WRITTEN DATE AND TIME */
        if (!islong)
            INT_BARF( context, 0x21 );
        else
        {
            TRACE( "SET FILE LAST-WRITTEN DATE AND TIME, file %s\n", fileA );
            MultiByteToWideChar(CP_OEMCP, 0, fileA, -1, fileW, MAX_PATH);

            handle = CreateFileW( fileW, GENERIC_WRITE, 
                                  FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                  NULL, OPEN_EXISTING, 0, 0 );
            if (handle == INVALID_HANDLE_VALUE)
                return FALSE;

            DosDateTimeToFileTime( DI_reg(context), 
                                   CX_reg(context),
                                   &filetime );
            status = SetFileTime( handle, NULL, NULL, &filetime );

            CloseHandle( handle );
            return status;
        }
        break;

    case 0x04: /* GET FILE LAST-WRITTEN DATE AND TIME */
        if (!islong)
            INT_BARF( context, 0x21 );
        else
        {
            TRACE( "GET FILE LAST-WRITTEN DATE AND TIME, file %s\n", fileA );
            MultiByteToWideChar(CP_OEMCP, 0, fileA, -1, fileW, MAX_PATH);

            handle = CreateFileW( fileW, GENERIC_READ, 
                                  FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                  NULL, OPEN_EXISTING, 0, 0 );
            if (handle == INVALID_HANDLE_VALUE)
                return FALSE;

            status = GetFileTime( handle, NULL, NULL, &filetime );
            if (status)
            {
                FileTimeToDosDateTime( &filetime, &date, &time );
                SET_DI( context, date );
                SET_CX( context, time );
            }

            CloseHandle( handle );
            return status;
        }
        break;

    case 0x05: /* SET FILE LAST ACCESS DATE */
        if (!islong)
            INT_BARF( context, 0x21 );
        else
        {
            TRACE( "SET FILE LAST ACCESS DATE, file %s\n", fileA );
            MultiByteToWideChar(CP_OEMCP, 0, fileA, -1, fileW, MAX_PATH);

            handle = CreateFileW( fileW, GENERIC_WRITE, 
                                  FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                  NULL, OPEN_EXISTING, 0, 0 );
            if (handle == INVALID_HANDLE_VALUE)
                return FALSE;

            DosDateTimeToFileTime( DI_reg(context), 
                                   0,
                                   &filetime );
            status = SetFileTime( handle, NULL, &filetime, NULL );

            CloseHandle( handle );
            return status;
        }
        break;

    case 0x06: /* GET FILE LAST ACCESS DATE */
        if (!islong)
            INT_BARF( context, 0x21 );
        else
        {
            TRACE( "GET FILE LAST ACCESS DATE, file %s\n", fileA );
            MultiByteToWideChar(CP_OEMCP, 0, fileA, -1, fileW, MAX_PATH);

            handle = CreateFileW( fileW, GENERIC_READ, 
                                  FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                  NULL, OPEN_EXISTING, 0, 0 );
            if (handle == INVALID_HANDLE_VALUE)
                return FALSE;

            status = GetFileTime( handle, NULL, &filetime, NULL );
            if (status)
            {
                FileTimeToDosDateTime( &filetime, &date, NULL );
                SET_DI( context, date );
            }

            CloseHandle( handle );
            return status;
        }
        break;

    case 0x07: /* SET FILE CREATION DATE AND TIME */
        if (!islong)
            INT_BARF( context, 0x21 );
        else
        {
            TRACE( "SET FILE CREATION DATE AND TIME, file %s\n", fileA );
            MultiByteToWideChar(CP_OEMCP, 0, fileA, -1, fileW, MAX_PATH);

            handle = CreateFileW( fileW, GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                  NULL, OPEN_EXISTING, 0, 0 );
            if (handle == INVALID_HANDLE_VALUE)
                return FALSE;
            
            /*
             * FIXME: SI has number of 10-millisecond units past time in CX.
             */
            DosDateTimeToFileTime( DI_reg(context),
                                   CX_reg(context),
                                   &filetime );
            status = SetFileTime( handle, &filetime, NULL, NULL );

            CloseHandle( handle );
            return status;
        }
        break;

    case 0x08: /* GET FILE CREATION DATE AND TIME */
        if (!islong)
            INT_BARF( context, 0x21 );
        else
        {
            TRACE( "GET FILE CREATION DATE AND TIME, file %s\n", fileA );
            MultiByteToWideChar(CP_OEMCP, 0, fileA, -1, fileW, MAX_PATH);

            handle = CreateFileW( fileW, GENERIC_READ, 
                                  FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                  NULL, OPEN_EXISTING, 0, 0 );
            if (handle == INVALID_HANDLE_VALUE)
                return FALSE;

            status = GetFileTime( handle, &filetime, NULL, NULL );
            if (status)
            {            
                FileTimeToDosDateTime( &filetime, &date, &time );
                SET_DI( context, date );
                SET_CX( context, time );
                /*
                 * FIXME: SI has number of 10-millisecond units past 
                 *        time in CX.
                 */
                SET_SI( context, 0 );
            }

            CloseHandle(handle);
            return status;
        }
        break;

    case 0xff: /* EXTENDED-LENGTH FILENAME OPERATIONS */
        if (islong || context->Ebp != 0x5053)
            INT_BARF( context, 0x21 );
        else
        {
            switch(CL_reg(context))
            {
            case 0x39:
                if (!INT21_CreateDirectory( context ))
                    return FALSE;
                break;

            case 0x56:
                if (!INT21_RenameFile( context ))
                    return FALSE;
                break;

            default:
                INT_BARF( context, 0x21 );
            }
        }
        break;

    default:
        INT_BARF( context, 0x21 );
    }

    return TRUE;
}


/***********************************************************************
 *           INT21_FileDateTime
 *
 * Handler for function 0x57.
 */
static BOOL INT21_FileDateTime( CONTEXT *context )
{
    HANDLE   handle = DosFileHandleToWin32Handle(BX_reg(context));
    FILETIME filetime;
    WORD     date, time;

    switch (AL_reg(context)) {
    case 0x00:  /* Get last-written stamp */
        TRACE( "GET FILE LAST-WRITTEN DATE AND TIME, handle %d\n",
               BX_reg(context) );
        {
            if (!GetFileTime( handle, NULL, NULL, &filetime ))
                return FALSE;
            FileTimeToDosDateTime( &filetime, &date, &time );
            SET_DX( context, date );
            SET_CX( context, time );
            break;
        }

    case 0x01:  /* Set last-written stamp */
        TRACE( "SET FILE LAST-WRITTEN DATE AND TIME, handle %d\n",
               BX_reg(context) );
        {
            DosDateTimeToFileTime( DX_reg(context), 
                                   CX_reg(context),
                                   &filetime );
            if (!SetFileTime( handle, NULL, NULL, &filetime ))
                return FALSE;
            break;
        }

    case 0x04:  /* Get last access stamp, DOS 7 */
        TRACE( "GET FILE LAST ACCESS DATE AND TIME, handle %d\n",
               BX_reg(context) );
        {
            if (!GetFileTime( handle, NULL, &filetime, NULL ))
                return FALSE;
            FileTimeToDosDateTime( &filetime, &date, &time );
            SET_DX( context, date );
            SET_CX( context, time );
            break;
        }

    case 0x05:  /* Set last access stamp, DOS 7 */
        TRACE( "SET FILE LAST ACCESS DATE AND TIME, handle %d\n",
               BX_reg(context) );
        {
            DosDateTimeToFileTime( DX_reg(context), 
                                   CX_reg(context),
                                   &filetime );
            if (!SetFileTime( handle, NULL, &filetime, NULL ))
                return FALSE;
            break;
        }

    case 0x06:  /* Get creation stamp, DOS 7 */
        TRACE( "GET FILE CREATION DATE AND TIME, handle %d\n",
               BX_reg(context) );
        {
            if (!GetFileTime( handle, &filetime, NULL, NULL ))
                return FALSE;
            FileTimeToDosDateTime( &filetime, &date, &time );
            SET_DX( context, date );
            SET_CX( context, time );
            /*
             * FIXME: SI has number of 10-millisecond units past time in CX.
             */
            SET_SI( context, 0 );
            break;
        }

    case 0x07:  /* Set creation stamp, DOS 7 */
        TRACE( "SET FILE CREATION DATE AND TIME, handle %d\n",
               BX_reg(context) );
        {
            /*
             * FIXME: SI has number of 10-millisecond units past time in CX.
             */
            DosDateTimeToFileTime( DX_reg(context), 
                                   CX_reg(context),
                                   &filetime );
            if (!SetFileTime( handle, &filetime, NULL, NULL ))
                return FALSE;
            break;
        }

    default:
        INT_BARF( context, 0x21 );
        break;
    }

    return TRUE;
}


/***********************************************************************
 *           INT21_GetPSP
 *
 * Handler for functions 0x51 and 0x62.
 */
static void INT21_GetPSP( CONTEXT *context )
{
    TRACE( "GET CURRENT PSP ADDRESS (%02x)\n", AH_reg(context) );

    SET_BX( context, LOWORD(GetCurrentPDB16()) );
}

static inline void setword( BYTE *ptr, WORD w )
{
    ptr[0] = (BYTE)w;
    ptr[1] = (BYTE)(w >> 8);
}

static void CreateBPB(int drive, BYTE *data, BOOL16 limited)
/* limited == TRUE is used with INT 0x21/0x440d */
{
    /* FIXME: we're forcing some values without checking that those are valid */
    if (drive > 1) 
    {
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
        if (!limited) 
        {
            setword(&data[0x1f], 800);
            data[0x21] = 5;
            setword(&data[0x22], 1);
        }
    }
    else
    { /* 1.44mb */
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
        if (!limited) 
        {
            setword(&data[0x1f], 80);
            data[0x21] = 7;
            setword(&data[0x22], 2);
        }
    }
}

static inline DWORD INT21_Ioctl_CylHeadSect2Lin(DWORD cyl, WORD head, WORD sec, WORD cyl_cnt,
                                                 WORD head_cnt, WORD sec_cnt)
{
    DWORD res = (cyl * head_cnt*sec_cnt + head * sec_cnt + sec);
    return res;
}

/***********************************************************************
 *           INT21_Ioctl_Block
 *
 * Handler for block device IOCTLs.
 */
static void INT21_Ioctl_Block( CONTEXT *context )
{
    BYTE *dataptr;
    BYTE  drive = INT21_MapDrive( BL_reg(context) );
    WCHAR drivespec[] = {'A', ':', '\\', 0};
    UINT  drivetype;

    drivespec[0] += drive;
    drivetype = GetDriveTypeW( drivespec );

    RESET_CFLAG(context);
    if (drivetype == DRIVE_UNKNOWN || drivetype == DRIVE_NO_ROOT_DIR)
    {
        TRACE( "IOCTL - SUBFUNCTION %d - INVALID DRIVE %c:\n", 
               AL_reg(context), 'A' + drive );
        SetLastError( ERROR_INVALID_DRIVE );
        SET_AX( context, ERROR_INVALID_DRIVE );
        SET_CFLAG( context );
        return;
    }

    switch (AL_reg(context))
    {
    case 0x04: /* READ FROM BLOCK DEVICE CONTROL CHANNEL */
    case 0x05: /* WRITE TO BLOCK DEVICE CONTROL CHANNEL */
        INT_BARF( context, 0x21 );
        break;

    case 0x08: /* CHECK IF BLOCK DEVICE REMOVABLE */
        TRACE( "IOCTL - CHECK IF BLOCK DEVICE REMOVABLE - %c:\n",
               'A' + drive );

        if (drivetype == DRIVE_REMOVABLE)
            SET_AX( context, 0 ); /* removable */
        else
            SET_AX( context, 1 ); /* not removable */
        break;

    case 0x09: /* CHECK IF BLOCK DEVICE REMOTE */
        TRACE( "IOCTL - CHECK IF BLOCK DEVICE REMOTE - %c:\n",
               'A' + drive );

        if (drivetype == DRIVE_REMOTE)
            SET_DX( context, (1<<9) | (1<<12) ); /* remote + no direct IO */
        else if (drivetype == DRIVE_CDROM)
            /* CDROM should be set to remote. If it set the app will
             * call int2f to check if it cdrom or remote drive. */
            SET_DX( context, (1<<12) );
        else if (drivetype == DRIVE_FIXED)
            /* This should define if drive support 0x0d, 0x0f and 0x08
             * requests. The local fixed drive should do. */
            SET_DX( context, (1<<11) );
        else
            SET_DX( context, 0 ); /* FIXME: use driver attr here */
        break;

    case 0x0d: /* GENERIC BLOCK DEVICE REQUEST */
        /* Get pointer to IOCTL parameter block */
        dataptr = ldt_get_ptr(context->SegDs, context->Edx);

        switch (CX_reg(context))
        {
        case 0x0841: /* write logical device track */
            TRACE( "GENERIC IOCTL - Write logical device track - %c:\n",
                   'A' + drive);
            {
                WORD head   = *(WORD *)(dataptr+1);
                WORD cyl    = *(WORD *)(dataptr+3);
                WORD sect   = *(WORD *)(dataptr+5);
                WORD nrsect = *(WORD *)(dataptr+7);
                BYTE *data  = ldt_get_ptr(*(WORD *)(dataptr+11), *(WORD *)(dataptr+9));
                WORD cyl_cnt, head_cnt, sec_cnt;

                /* FIXME: we're faking some values here */
                if (drive > 1)
                {
                    /* cyl_cnt = 0x300;
                    head_cnt = 16;
                    sec_cnt = 255; */
                    SET_AX( context, ERROR_WRITE_FAULT );
                    SET_CFLAG(context);
                    break;
                }
                else
                { /* floppy */
                    cyl_cnt = 80;
                    head_cnt = 2;
                    sec_cnt = 18;
                }

                if (!DOSVM_RawWrite(drive, INT21_Ioctl_CylHeadSect2Lin(cyl, head, sect, cyl_cnt, head_cnt, sec_cnt), nrsect, data, FALSE))
                {
                    SET_AX( context, ERROR_WRITE_FAULT );
                    SET_CFLAG(context);
                }
            }
            break;

        case 0x084a: /* lock logical volume */
            TRACE( "GENERIC IOCTL - Lock logical volume, level %d mode %d - %c:\n",
                   BH_reg(context), DX_reg(context), 'A' + drive );
            break;

        case 0x0860: /* get device parameters */
            /* FIXME: we're faking some values here */
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

        case 0x0861: /* read logical device track */
            TRACE( "GENERIC IOCTL - Read logical device track - %c:\n",
                   'A' + drive);
            {
                WORD head   = *(WORD *)(dataptr+1);
                WORD cyl    = *(WORD *)(dataptr+3);
                WORD sect   = *(WORD *)(dataptr+5);
                WORD nrsect = *(WORD *)(dataptr+7);
                BYTE *data  = ldt_get_ptr(*(WORD *)(dataptr+11), *(WORD *)(dataptr+9));
                WORD cyl_cnt, head_cnt, sec_cnt;

                /* FIXME: we're faking some values here */
                if (drive > 1)
                {
                    cyl_cnt = 0x300;
                    head_cnt = 16;
                    sec_cnt = 255;
                }
                else
                { /* floppy */
                    cyl_cnt = 80;
                    head_cnt = 2;
                    sec_cnt = 18;
                }

                if (!DOSVM_RawRead(drive, INT21_Ioctl_CylHeadSect2Lin(cyl, head, sect, cyl_cnt, head_cnt, sec_cnt), nrsect, data, FALSE))
                {
                    SET_AX( context, ERROR_READ_FAULT );
                    SET_CFLAG(context);
                }
            }
            break;

        case 0x0866: /* get volume serial number */
            {
                WCHAR	label[12],fsname[9];
                DWORD	serial;
                TRACE( "GENERIC IOCTL - Get media id - %c:\n",
                       'A' + drive );

                GetVolumeInformationW(drivespec, label, 12, &serial, NULL, NULL, fsname, 9);
                *(WORD*)dataptr	= 0;
                memcpy(dataptr+2,&serial,4);
                WideCharToMultiByte(CP_OEMCP, 0, label, 11, (LPSTR)dataptr + 6, 11, NULL, NULL);
                WideCharToMultiByte(CP_OEMCP, 0, fsname, 8, (LPSTR)dataptr + 17, 8, NULL, NULL);
            }
            break;

        case 0x086a: /* unlock logical volume */
            TRACE( "GENERIC IOCTL - Logical volume unlocked - %c:\n", 
                   'A' + drive );
            break;

        case 0x086f: /* get drive map information */
            memset(dataptr+1, '\0', dataptr[0]-1);
            dataptr[1] = dataptr[0];
            dataptr[2] = 0x07; /* protected mode driver; no eject; no notification */
            dataptr[3] = 0xFF; /* no physical drive */
            break;

        case 0x0872:
            /* Trial and error implementation */
            SET_AX( context, drivetype == DRIVE_UNKNOWN ? 0x0f : 0x01 );
            SET_CFLAG(context);	/* Seems to be set all the time */
            break;

        default:
            INT_BARF( context, 0x21 );            
        }
        break;

    case 0x0e: /* GET LOGICAL DRIVE MAP */
        TRACE( "IOCTL - GET LOGICAL DRIVE MAP - %c:\n",
               'A' + drive );
        /* FIXME: this is not correct if drive has mappings */
        SET_AL( context, 0 ); /* drive has no mapping */
        break;

    case 0x0f: /* SET LOGICAL DRIVE MAP */
        TRACE("IOCTL - SET LOGICAL DRIVE MAP for drive %s\n",
              INT21_DriveName( BL_reg(context)));
        /* FIXME: as of today, we don't support logical drive mapping... */
        SET_AL( context, 0 );
        break;

    case 0x11: /* QUERY GENERIC IOCTL CAPABILITY */
    default:
        INT_BARF( context, 0x21 );
    }
}


/***********************************************************************
 *           INT21_Ioctl_Char
 *
 * Handler for character device IOCTLs.
 */
static void INT21_Ioctl_Char( CONTEXT *context )
{
    HANDLE handle = DosFileHandleToWin32Handle(BX_reg(context));
    BOOL IsConsoleIOHandle = VerifyConsoleIoHandle( handle );

    switch (AL_reg(context))
    {
    case 0x00: /* GET DEVICE INFORMATION */
        TRACE( "IOCTL - GET DEVICE INFORMATION - %d\n", BX_reg(context) );
        if (IsConsoleIOHandle || GetFileType(handle) == FILE_TYPE_CHAR)
        {
            /*
             * Returns attribute word in DX: 
             *   Bit 14 - Device driver can process IOCTL requests.
             *   Bit 13 - Output until busy supported.
             *   Bit 11 - Driver supports OPEN/CLOSE calls.
             *   Bit  8 - Unknown.
             *   Bit  7 - Set (indicates device).
             *   Bit  6 - EOF on input.
             *   Bit  5 - Raw (binary) mode.
             *   Bit  4 - Device is special (uses int29).
             *   Bit  3 - Clock device.
             *   Bit  2 - NUL device.
             *   Bit  1 - Console output device.
             *   Bit  0 - Console input device.
             */
            SET_DX( context, IsConsoleIOHandle ? 0x80c3 : 0x80c0 /* FIXME */ );
        }
        else
        {
            /*
             * Returns attribute word in DX: 
             *   Bit 15    - File is remote.
             *   Bit 14    - Don't set file date/time on closing.
             *   Bit 11    - Media not removable.
             *   Bit  8    - Generate int24 if no disk space on write 
             *               or read past end of file
             *   Bit  7    - Clear (indicates file).
             *   Bit  6    - File has not been written.
             *   Bit  5..0 - Drive number (0=A:,...)
             *
             * FIXME: Should check if file is on remote or removable drive.
             * FIXME: Should use drive file is located on (and not current).
             */
            SET_DX( context, 0x0140 + INT21_GetCurrentDrive() );
        }
        break;

    case 0x0a: /* CHECK IF HANDLE IS REMOTE */
        TRACE( "IOCTL - CHECK IF HANDLE IS REMOTE - %d\n", BX_reg(context) );
        /*
         * Returns attribute word in DX:
         *   Bit 15 - Set if remote.
         *   Bit 14 - Set if date/time not set on close.
         *
         * FIXME: Should check if file is on remote drive.
         */
        SET_DX( context, 0 );
        break;

    case 0x01: /* SET DEVICE INFORMATION */
    case 0x02: /* READ FROM CHARACTER DEVICE CONTROL CHANNEL */
    case 0x03: /* WRITE TO CHARACTER DEVICE CONTROL CHANNEL */
    case 0x06: /* GET INPUT STATUS */
    case 0x07: /* GET OUTPUT STATUS */
    case 0x0c: /* GENERIC CHARACTER DEVICE REQUEST */
    case 0x10: /* QUERY GENERIC IOCTL CAPABILITY */
    default:
        INT_BARF( context, 0x21 );
        break;
    }
}


/***********************************************************************
 *           INT21_Ioctl
 *
 * Handler for function 0x44.
 */
static void INT21_Ioctl( CONTEXT *context )
{
    switch (AL_reg(context))
    {
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
        INT21_Ioctl_Char( context );
        break;

    case 0x04:
    case 0x05:
        INT21_Ioctl_Block( context );
        break;

    case 0x06:
    case 0x07:
        INT21_Ioctl_Char( context );
        break;

    case 0x08:
    case 0x09:
        INT21_Ioctl_Block( context );
        break;

    case 0x0a:
        INT21_Ioctl_Char( context );
        break;

    case 0x0b: /* SET SHARING RETRY COUNT */
        TRACE( "SET SHARING RETRY COUNT: Pause %d, retries %d.\n",
               CX_reg(context), DX_reg(context) );
        if (!CX_reg(context))
        {
            SET_AX( context, 1 );
            SET_CFLAG( context );
        }
        else
        {
            RESET_CFLAG( context );
        }
        break;

    case 0x0c:
        INT21_Ioctl_Char( context );
        break;

    case 0x0d:
    case 0x0e:
    case 0x0f:
        INT21_Ioctl_Block( context );
        break;

    case 0x10:
        INT21_Ioctl_Char( context );
        break;

    case 0x11:
        INT21_Ioctl_Block( context );
        break;

    case 0x12: /*  DR DOS - DETERMINE DOS TYPE (OBSOLETE FUNCTION) */
        TRACE( "DR DOS - DETERMINE DOS TYPE (OBSOLETE FUNCTION)\n" );
        SET_CFLAG(context);        /* Error / This is not DR DOS. */
        SET_AX( context, 0x0001 ); /* Invalid function */
        break;

    case 0x52: /* DR DOS - DETERMINE DOS TYPE */
        TRACE( "DR DOS - DETERMINE DOS TYPE\n" );
        SET_CFLAG(context);        /* Error / This is not DR DOS. */
        SET_AX( context, 0x0001 ); /* Invalid function */
        break;

    case 0xe0:  /* Sun PC-NFS API */
        TRACE( "Sun PC-NFS API\n" );
        /* not installed */
        break;

    default:
        INT_BARF( context, 0x21 );
    }
}


/***********************************************************************
 *           INT21_Fat32
 *
 * Handler for function 0x73.
 */
static BOOL INT21_Fat32( CONTEXT *context )
{
    switch (AL_reg(context))
    {
    case 0x02: /* FAT32 - GET EXTENDED DPB */
        {
            BYTE drive = INT21_MapDrive( DL_reg(context) );
            WORD *ptr = ldt_get_ptr(context->SegEs, context->Edi);
            INT21_DPB *target = (INT21_DPB*)(ptr + 1);
            INT21_DPB *source;

            TRACE( "FAT32 - GET EXTENDED DPB %d\n", DL_reg(context) );

            if ( CX_reg(context) < sizeof(INT21_DPB) + 2 || *ptr < sizeof(INT21_DPB) )
            {
                SetLastError( ERROR_BAD_LENGTH );
                return FALSE;
            }

            if ( !INT21_FillDrivePB( drive ) )
            {
                SetLastError( ERROR_INVALID_DRIVE );
                return FALSE;
            }

            source = &INT21_GetHeapPointer()->misc_dpb_list[drive];

            *ptr = sizeof(INT21_DPB);
            *target = *source;

            if (LOWORD(context->Esi) != 0xF1A6)
            {
                target->driver_header = 0;
                target->next          = 0;
            }
            else
            {
                FIXME( "Caller requested driver and next DPB pointers!\n" );
            }
        }
        break;

    case 0x03: /* FAT32 - GET EXTENDED FREE SPACE ON DRIVE */
        {
            WCHAR dirW[MAX_PATH];
            char *dirA = ldt_get_ptr( context->SegDs, context->Edx );
            BYTE *data = ldt_get_ptr( context->SegEs, context->Edi );
            DWORD cluster_sectors;
            DWORD sector_bytes;
            DWORD free_clusters;
            DWORD total_clusters;

            TRACE( "FAT32 - GET EXTENDED FREE SPACE ON DRIVE %s\n", dirA );
            MultiByteToWideChar(CP_OEMCP, 0, dirA, -1, dirW, MAX_PATH);

            if (CX_reg(context) < 44)
            {
                SetLastError( ERROR_BAD_LENGTH );
                return FALSE;
            }

            if (!GetDiskFreeSpaceW( dirW, &cluster_sectors, &sector_bytes,
                                    &free_clusters, &total_clusters ))
                return FALSE;

            *(WORD*) (data +  0) = 44; /* size of structure */
            *(WORD*) (data +  2) = 0;  /* version */
            *(DWORD*)(data +  4) = cluster_sectors;
            *(DWORD*)(data +  8) = sector_bytes;
            *(DWORD*)(data + 12) = free_clusters;
            *(DWORD*)(data + 16) = total_clusters;

            /*
             * Below we have free/total sectors and
             * free/total allocation units without adjustment
             * for compression. We fake both using cluster information.
             */
            *(DWORD*)(data + 20) = free_clusters * cluster_sectors;
            *(DWORD*)(data + 24) = total_clusters * cluster_sectors;
            *(DWORD*)(data + 28) = free_clusters;
            *(DWORD*)(data + 32) = total_clusters;
            
            /*
             * Between (data + 36) and (data + 43) there
             * are eight reserved bytes.
             */
        }
        break;

    default:
        INT_BARF( context, 0x21 );
    }

    return TRUE;
}

static void INT21_ConvertFindDataWtoA(WIN32_FIND_DATAA *dataA,
                                      const WIN32_FIND_DATAW *dataW)
{
    dataA->dwFileAttributes = dataW->dwFileAttributes;
    dataA->ftCreationTime   = dataW->ftCreationTime;
    dataA->ftLastAccessTime = dataW->ftLastAccessTime;
    dataA->ftLastWriteTime  = dataW->ftLastWriteTime;
    dataA->nFileSizeHigh    = dataW->nFileSizeHigh;
    dataA->nFileSizeLow     = dataW->nFileSizeLow;
    WideCharToMultiByte( CP_OEMCP, 0, dataW->cFileName, -1,
                         dataA->cFileName, sizeof(dataA->cFileName), NULL, NULL );
    WideCharToMultiByte( CP_OEMCP, 0, dataW->cAlternateFileName, -1,
                         dataA->cAlternateFileName, sizeof(dataA->cAlternateFileName), NULL, NULL );
}

/***********************************************************************
 *           INT21_LongFilename
 *
 * Handler for function 0x71.
 */
static void INT21_LongFilename( CONTEXT *context )
{
    BOOL bSetDOSExtendedError = FALSE;
    WCHAR pathW[MAX_PATH];
    char* pathA;

    switch (AL_reg(context))
    {
    case 0x0d: /* RESET DRIVE */
        INT_BARF( context, 0x21 );
        break;

    case 0x39: /* LONG FILENAME - MAKE DIRECTORY */
        if (!INT21_CreateDirectory( context ))
            bSetDOSExtendedError = TRUE;
        break;

    case 0x3a: /* LONG FILENAME - REMOVE DIRECTORY */
        pathA = ldt_get_ptr(context->SegDs, context->Edx);

        TRACE( "LONG FILENAME - REMOVE DIRECTORY %s\n", pathA);
        MultiByteToWideChar(CP_OEMCP, 0, pathA, -1, pathW, MAX_PATH);
        if (!RemoveDirectoryW( pathW )) bSetDOSExtendedError = TRUE;
        break;

    case 0x3b: /* LONG FILENAME - CHANGE DIRECTORY */
        if (!INT21_SetCurrentDirectory( context ))
            bSetDOSExtendedError = TRUE;
        break;

    case 0x41: /* LONG FILENAME - DELETE FILE */
        pathA = ldt_get_ptr(context->SegDs, context->Edx);

        TRACE( "LONG FILENAME - DELETE FILE %s\n", pathA );
        MultiByteToWideChar(CP_OEMCP, 0, pathA, -1, pathW, MAX_PATH);

        if (!DeleteFileW( pathW )) bSetDOSExtendedError = TRUE;
        break;

    case 0x43: /* LONG FILENAME - EXTENDED GET/SET FILE ATTRIBUTES */
        if (!INT21_FileAttributes( context, BL_reg(context), TRUE ))
            bSetDOSExtendedError = TRUE;
        break;

    case 0x47: /* LONG FILENAME - GET CURRENT DIRECTORY */
        if (!INT21_GetCurrentDirectory( context, TRUE ))
            bSetDOSExtendedError = TRUE;
        break;

    case 0x4e: /* LONG FILENAME - FIND FIRST MATCHING FILE */
        {
            HANDLE              handle;
            HGLOBAL16           h16;
            WIN32_FIND_DATAW    dataW;
            WIN32_FIND_DATAA*   dataA;

            pathA = ldt_get_ptr(context->SegDs,context->Edx);
            TRACE(" LONG FILENAME - FIND FIRST MATCHING FILE for %s\n", pathA);

            MultiByteToWideChar(CP_OEMCP, 0, pathA, -1, pathW, MAX_PATH);
            handle = FindFirstFileW(pathW, &dataW);

            dataA = ldt_get_ptr(context->SegEs, context->Edi);
            if (handle != INVALID_HANDLE_VALUE && 
                (h16 = GlobalAlloc16(GMEM_MOVEABLE, sizeof(handle))))
            {
                HANDLE* ptr = GlobalLock16( h16 );
                *ptr = handle;
                GlobalUnlock16( h16 );
                SET_AX( context, h16 );
                INT21_ConvertFindDataWtoA(dataA, &dataW);
            }
            else
            {           
                if (handle != INVALID_HANDLE_VALUE) FindClose(handle);
                SET_AX( context, INVALID_HANDLE_VALUE16);
                bSetDOSExtendedError = TRUE;
            }
        }
        break;

    case 0x4f: /* LONG FILENAME - FIND NEXT MATCHING FILE */
        {
            HGLOBAL16           h16 = BX_reg(context);
            HANDLE*             ptr;
            WIN32_FIND_DATAW    dataW;
            WIN32_FIND_DATAA*   dataA;

            TRACE("LONG FILENAME - FIND NEXT MATCHING FILE for handle %d\n",
                  BX_reg(context));

            dataA = ldt_get_ptr(context->SegEs, context->Edi);

            if (h16 != INVALID_HANDLE_VALUE16 && (ptr = GlobalLock16( h16 )))
            {
                if (!FindNextFileW(*ptr, &dataW)) bSetDOSExtendedError = TRUE;
                else INT21_ConvertFindDataWtoA(dataA, &dataW);
                GlobalUnlock16( h16 );
            }
            else
            {
                SetLastError( ERROR_INVALID_HANDLE );
                bSetDOSExtendedError = TRUE;
            }
        }
        break;

    case 0x56: /* LONG FILENAME - RENAME FILE */
        if (!INT21_RenameFile(context))
            bSetDOSExtendedError = TRUE;
        break;

    case 0x60: /* LONG FILENAME - CONVERT PATH */
        {
            WCHAR   res[MAX_PATH];

            switch (CL_reg(context))
            {
            case 0x00:  /* "truename" - Canonicalize path */
                /* 
                 * FIXME: This is not 100% equal to 0x01 case, 
                 *        if you fix this, fix int21 subfunction 0x60, too.
                 */

            case 0x01:  /* Get short filename or path */
                MultiByteToWideChar(CP_OEMCP, 0, ldt_get_ptr(context->SegDs, context->Esi), -1, pathW, MAX_PATH);
                if (!GetShortPathNameW(pathW, res, 67))
                    bSetDOSExtendedError = TRUE;
                else
                {
                    SET_AX( context, 0 );
                    WideCharToMultiByte(CP_OEMCP, 0, res, -1, ldt_get_ptr(context->SegEs, context->Edi), 67, NULL, NULL);
                }
                break;
	    
            case 0x02:  /* Get canonical long filename or path */
                MultiByteToWideChar(CP_OEMCP, 0, ldt_get_ptr(context->SegDs, context->Esi), -1, pathW, MAX_PATH);
                if (!GetFullPathNameW(pathW, 128, res, NULL))
                    bSetDOSExtendedError = TRUE;
                else
                {
                    SET_AX( context, 0 );
                    WideCharToMultiByte(CP_OEMCP, 0, res, -1, ldt_get_ptr(context->SegEs, context->Edi), 128, NULL, NULL);
                }
                break;
            default:
                FIXME("Unimplemented long file name function:\n");
                INT_BARF( context, 0x21 );
                SET_CFLAG(context);
                SET_AL( context, 0 );
                break;
            }
        }
        break;

    case 0x6c: /* LONG FILENAME - CREATE OR OPEN FILE */
        if (!INT21_CreateFile( context, context->Esi, TRUE,
                               BX_reg(context), DL_reg(context) ))
            bSetDOSExtendedError = TRUE;
        break;

    case 0xa0: /* LONG FILENAME - GET VOLUME INFORMATION */
        {
            DWORD filename_len, flags;
            WCHAR dstW[8];

            pathA = ldt_get_ptr(context->SegDs,context->Edx);

            TRACE("LONG FILENAME - GET VOLUME INFORMATION for drive having root dir '%s'.\n", pathA);
            SET_AX( context, 0 );
            MultiByteToWideChar(CP_OEMCP, 0, pathA, -1, pathW, MAX_PATH);
            if (!GetVolumeInformationW( pathW, NULL, 0, NULL, &filename_len,
                                        &flags, dstW, 8 ))
            {
                INT_BARF( context, 0x21 );
                SET_CFLAG(context);
                break;
            }
            SET_BX( context, flags | 0x4000 ); /* support for LFN functions */
            SET_CX( context, filename_len );
            SET_DX( context, MAX_PATH ); /* FIXME: which len if DRIVE_SHORT_NAMES ? */
            WideCharToMultiByte(CP_OEMCP, 0, dstW, -1, ldt_get_ptr(context->SegEs, context->Edi), 8, NULL, NULL);
        }
        break;

    case 0xa1: /* LONG FILENAME - "FindClose" - TERMINATE DIRECTORY SEARCH */
        {
            HGLOBAL16 h16 = BX_reg(context);
            HANDLE* ptr;

            TRACE("LONG FILENAME - FINDCLOSE for handle %d\n",
                  BX_reg(context));
            if (h16 != INVALID_HANDLE_VALUE16 && (ptr = GlobalLock16( h16 )))
            {
                if (!FindClose( *ptr )) bSetDOSExtendedError = TRUE;
                GlobalUnlock16( h16 );
                GlobalFree16( h16 );
            }
            else
            {
                SetLastError( ERROR_INVALID_HANDLE );
                bSetDOSExtendedError = TRUE;
            }
        }
        break;

    case 0xa6: /* LONG FILENAME - GET FILE INFO BY HANDLE */
        {
            HANDLE handle = DosFileHandleToWin32Handle(BX_reg(context));
            BY_HANDLE_FILE_INFORMATION *info = ldt_get_ptr(context->SegDs, context->Edx);

            TRACE( "LONG FILENAME - GET FILE INFO BY HANDLE\n" );

            if (!GetFileInformationByHandle(handle, info))
                bSetDOSExtendedError = TRUE;
        }
        break;

    case 0xa7: /* LONG FILENAME - CONVERT TIME */
        switch (BL_reg(context))
        {
        case 0x00: /* FILE TIME TO DOS TIME */
            {
                WORD      date, time;
                FILETIME *filetime = ldt_get_ptr(context->SegDs, context->Esi);

                TRACE( "LONG FILENAME - FILE TIME TO DOS TIME\n" );

                FileTimeToDosDateTime( filetime, &date, &time );

                SET_DX( context, date );
                SET_CX( context, time );

                /*
                 * FIXME: BH has number of 10-millisecond units 
                 * past time in CX.
                 */
                SET_BH( context, 0 );
            }
            break;

        case 0x01: /* DOS TIME TO FILE TIME */
            {
                FILETIME *filetime = ldt_get_ptr(context->SegEs, context->Edi);

                TRACE( "LONG FILENAME - DOS TIME TO FILE TIME\n" );

                /*
                 * FIXME: BH has number of 10-millisecond units 
                 * past time in CX.
                 */
                DosDateTimeToFileTime( DX_reg(context), CX_reg(context),
                                       filetime );
            }
            break;

        default:
            INT_BARF( context, 0x21 );
            break;
        }
        break;

    case 0xa8: /* LONG FILENAME - GENERATE SHORT FILENAME */
    case 0xa9: /* LONG FILENAME - SERVER CREATE OR OPEN FILE */
    case 0xaa: /* LONG FILENAME - SUBST */
    default:
        FIXME("Unimplemented long file name function:\n");
        INT_BARF( context, 0x21 );
        SET_CFLAG(context);
        SET_AL( context, 0 );
        break;
    }

    if (bSetDOSExtendedError)
    {
        SET_AX( context, GetLastError() );
        SET_CFLAG( context );
    }
}


/***********************************************************************
 *           INT21_RenameFile
 *
 * Handler for:
 * - function 0x56
 * - subfunction 0x56 of function 0x71
 * - subfunction 0xff of function 0x43 (CL == 0x56)
 */
static BOOL INT21_RenameFile( CONTEXT *context )
{
    WCHAR fromW[MAX_PATH];
    WCHAR toW[MAX_PATH];
    char *fromA = ldt_get_ptr(context->SegDs, context->Edx);
    char *toA = ldt_get_ptr(context->SegEs, context->Edi);

    TRACE( "RENAME FILE %s to %s\n", fromA, toA );
    MultiByteToWideChar(CP_OEMCP, 0, fromA, -1, fromW, MAX_PATH);
    MultiByteToWideChar(CP_OEMCP, 0, toA, -1, toW, MAX_PATH);

    return MoveFileW( fromW, toW );
}


/***********************************************************************
 *           INT21_NetworkFunc
 *
 * Handler for:
 * - function 0x5e
 */
static BOOL INT21_NetworkFunc (CONTEXT *context)
{
    switch (AL_reg(context)) 
    {
    case 0x00: /* Get machine name. */
        {
            WCHAR dstW[MAX_COMPUTERNAME_LENGTH + 1];
            DWORD s = ARRAY_SIZE(dstW);
            int len;

            char *dst = ldt_get_ptr(context->SegDs, context->Edx);
            TRACE("getting machine name to %p\n", dst);
            if (!GetComputerNameW(dstW, &s) ||
                !WideCharToMultiByte(CP_OEMCP, 0, dstW, -1, dst, 16, NULL, NULL))
            {
                WARN("failed!\n");
                SetLastError( ER_NoNetwork );
                return TRUE;
            }
            for (len = strlen(dst); len < 15; len++) dst[len] = ' ';
            dst[15] = 0;
            SET_CH( context, 1 ); /* Valid */
            SET_CL( context, 1 ); /* NETbios number??? */
            TRACE("returning %s\n", debugstr_an(dst, 16));
            return FALSE;
        }

    default:
        SetLastError( ER_NoNetwork );
        return TRUE;
    }
}

/******************************************************************
 *		INT21_GetDiskSerialNumber
 *
 */
static int INT21_GetDiskSerialNumber( CONTEXT *context )
{
    BYTE *dataptr = ldt_get_ptr(context->SegDs, context->Edx);
    WCHAR path[] = {'A',':','\\',0}, label[11];
    DWORD serial;

    path[0] += INT21_MapDrive(BL_reg(context));
    if (!GetVolumeInformationW( path, label, 11, &serial, NULL, NULL, NULL, 0))
    {
        SetLastError( ERROR_INVALID_DRIVE );
        return 0;
    }

    *(WORD *)dataptr = 0;
    memcpy(dataptr + 2, &serial, sizeof(DWORD));
    WideCharToMultiByte(CP_OEMCP, 0, label, 11, (LPSTR)dataptr + 6, 11, NULL, NULL);
    memcpy(dataptr + 17, "FAT16   ", 8);
    return 1;
}


/******************************************************************
 *		INT21_SetDiskSerialNumber
 *
 */
static BOOL INT21_SetDiskSerialNumber( CONTEXT *context )
{
#if 0
    BYTE *dataptr = ldt_get_ptr(context->SegDs, context->Edx);
    int drive = INT21_MapDrive(BL_reg(context));

    if (!is_valid_drive(drive))
    {
        SetLastError( ERROR_INVALID_DRIVE );
        return FALSE;
    }

    DRIVE_SetSerialNumber( drive, *(DWORD *)(dataptr + 2) );
    return TRUE;
#else
    FIXME("Setting drive serial number is no longer supported\n");
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
#endif
}


/******************************************************************
 *		INT21_GetFreeDiskSpace
 *
 */
static BOOL INT21_GetFreeDiskSpace( CONTEXT *context )
{
    DWORD cluster_sectors, sector_bytes, free_clusters, total_clusters;
    WCHAR root[] = {'A',':','\\',0};
    const DWORD max_clusters = 0x3d83;
    const DWORD max_sectors_per_cluster = 0x7f;
    const DWORD max_bytes_per_sector = 0x200;

    root[0] += INT21_MapDrive(DL_reg(context));
    if (!GetDiskFreeSpaceW( root, &cluster_sectors, &sector_bytes,
                            &free_clusters, &total_clusters ))
        return FALSE;

    /* Some old win31 apps (Lotus SmartSuite 5.1) crap out if there's too
     * much disk space, so Windows XP seems to apply the following limits:
     *  cluster_sectors <= 0x7f
     *  sector size <= 0x200
     *  clusters <= 0x3D83
     * This means total reported space is limited to about 1GB.
     */

    /* Make sure bytes-per-sector is in [, max] */
    while (sector_bytes > max_bytes_per_sector) {
        sector_bytes >>= 1;
        free_clusters <<= 1;
        total_clusters <<= 1;
    }
    /* Then make sure sectors-per-cluster is in [max/2, max]. */
    while (cluster_sectors <= max_sectors_per_cluster/2) {
        cluster_sectors <<= 1;
        free_clusters >>= 1;
        total_clusters >>= 1;
    }
    while (cluster_sectors > max_sectors_per_cluster) {
        cluster_sectors >>= 1;
        free_clusters <<= 1;
        total_clusters <<= 1;
    }

    /* scale up sectors_per_cluster to exactly max_sectors_per_cluster.
     * We could skip this, but that would impose an artificially low
     * limit on reported disk space.
     * To avoid overflow, first apply a preliminary cap on sector count;
     * this will not affect the correctness of the final result,
     * because if the preliminary cap hits, the final one will, too.
     */
    if (total_clusters > 4 * max_clusters)
        total_clusters = 4 * max_clusters;
    if (free_clusters > 4 * max_clusters)
        free_clusters = 4 * max_clusters;
    if (cluster_sectors < max_sectors_per_cluster) {
        free_clusters *= cluster_sectors;
        free_clusters /= max_sectors_per_cluster;
        total_clusters *= cluster_sectors;
        total_clusters /= max_sectors_per_cluster;
        cluster_sectors = max_sectors_per_cluster;
    }

    /* Finally, apply real cluster count cap. */
    if (total_clusters > max_clusters)
        total_clusters = max_clusters;
    if (free_clusters > max_clusters)
        free_clusters = max_clusters;

    SET_AX( context, cluster_sectors );
    SET_BX( context, free_clusters );
    SET_CX( context, sector_bytes );
    SET_DX( context, total_clusters );
    return TRUE;
}

/******************************************************************
 *		INT21_GetDriveAllocInfo
 *
 */
static BOOL INT21_GetDriveAllocInfo( CONTEXT *context, BYTE drive )
{
    INT21_DPB  *dpb;

    drive = INT21_MapDrive( drive );
    if (!INT21_FillDrivePB( drive )) return FALSE;
    dpb = &(INT21_GetHeapPointer()->misc_dpb_list[drive]);
    SET_AL( context, dpb->cluster_sectors + 1 );
    SET_CX( context, dpb->sector_bytes );
    SET_DX( context, dpb->num_clusters1 );

    context->SegDs = INT21_GetHeapSelector( context );
    SET_BX( context, offsetof( INT21_HEAP, misc_dpb_list[drive].media_ID ) );
    return TRUE;
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
    case ERROR_DISK_FULL:
    case ERROR_HANDLE_DISK_FULL:
        class  = EC_MediaError;
        action = SA_Abort;
        locus  = EL_Disk;
        break;
    case ERROR_FILE_EXISTS:
    case ERROR_ALREADY_EXISTS:
        class  = EC_Exists;
        action = SA_Abort;
        locus  = EL_Disk;
        break;
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_INVALID_NAME:
        class  = EC_NotFound;
        action = SA_Abort;
        locus  = EL_Disk;
        break;
    case ERROR_GEN_FAILURE:
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
        FIXME("Unknown error %d\n", error );
        class  = EC_SystemFailure;
        action = SA_Abort;
        locus  = EL_Unknown;
        break;
    }
    TRACE("GET EXTENDED ERROR code 0x%02x class 0x%02x action 0x%02x locus %02x\n",
           error, class, action, locus );
    SET_AX( context, error );
    SET_BH( context, class );
    SET_BL( context, action );
    SET_CH( context, locus );
}

static BOOL INT21_CreateTempFile( CONTEXT *context )
{
    static int counter = 0;
    char *name = ldt_get_ptr( context->SegDs, context->Edx );
    char *p = name + strlen(name);

    /* despite what Ralf Brown says, some programs seem to call without
     * ending backslash (DOS accepts that, so we accept it too) */
    if ((p == name) || (p[-1] != '\\')) *p++ = '\\';

    for (;;)
    {
        sprintf( p, "wine%04lx.%03d", GetCurrentThreadId(), counter );
        counter = (counter + 1) % 1000;

        SET_AX( context, 
                Win32HandleToDosFileHandle( 
                    CreateFileA( name, GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                 CREATE_NEW, 0, 0 ) ) );
        if (AX_reg(context) != HFILE_ERROR16)
        {
            TRACE("created %s\n", name );
            return TRUE;
        }
        if (GetLastError() != ERROR_FILE_EXISTS) return FALSE;
    }
}

/***********************************************************************
 *           DOSFS_ToDosFCBFormat
 *
 * Convert a file name to DOS FCB format (8+3 chars, padded with blanks),
 * expanding wild cards and converting to upper-case in the process.
 * File name can be terminated by '\0', '\\' or '/'.
 * Return FALSE if the name is not a valid DOS name.
 * 'buffer' must be at least 12 characters long.
 */
/* Chars we don't want to see in DOS file names */
static BOOL INT21_ToDosFCBFormat( LPCWSTR name, LPWSTR buffer )
{
    static const WCHAR invalid_chars[] = {'*','?','<','>','|','\\','"','+','=',',',';','[',']',' ','\345',0};
    LPCWSTR p = name;
    int i;

    /* Check for "." and ".." */
    if (*p == '.')
    {
        p++;
        buffer[0] = '.';
        for(i = 1; i < 11; i++) buffer[i] = ' ';
        buffer[11] = 0;
        if (*p == '.')
        {
            buffer[1] = '.';
            p++;
        }
        return (!*p || (*p == '/') || (*p == '\\'));
    }

    for (i = 0; i < 8; i++)
    {
        switch(*p)
        {
        case '\0':
        case '\\':
        case '/':
        case '.':
            buffer[i] = ' ';
            break;
        case '?':
            p++;
            /* fall through */
        case '*':
            buffer[i] = '?';
            break;
        default:
            if (wcschr( invalid_chars, *p )) return FALSE;
            buffer[i] = *p++;
            break;
        }
    }

    if (*p == '*')
    {
        /* Skip all chars after wildcard up to first dot */
        while (*p && (*p != '/') && (*p != '\\') && (*p != '.')) p++;
    }
    else
    {
        /* Check if name too long */
        if (*p && (*p != '/') && (*p != '\\') && (*p != '.')) return FALSE;
    }
    if (*p == '.') p++;  /* Skip dot */

    for (i = 8; i < 11; i++)
    {
        switch(*p)
        {
        case '\0':
        case '\\':
        case '/':
            buffer[i] = ' ';
            break;
        case '.':
            return FALSE;  /* Second extension not allowed */
        case '?':
            p++;
            /* fall through */
        case '*':
            buffer[i] = '?';
            break;
        default:
            if (wcschr( invalid_chars, *p )) return FALSE;
            buffer[i] = *p++;
            break;
        }
    }
    buffer[11] = '\0';
    wcsupr( buffer );

    /* at most 3 character of the extension are processed
     * is something behind this ?
     */
    while (*p == '*' || *p == ' ') p++; /* skip wildcards and spaces */
    return (!*p || (*p == '/') || (*p == '\\'));
}

static HANDLE       INT21_FindHandle;
static const WCHAR *INT21_FindPath; /* will point to current dta->fullPath search */

/******************************************************************
 *		INT21_FindFirst
 */
static BOOL INT21_FindFirst( CONTEXT *context )
{
    WCHAR *p, *q;
    const char *path;
    FINDFILE_DTA *dta = (FINDFILE_DTA *)INT21_GetCurrentDTA(context);
    WCHAR maskW[12], pathW[MAX_PATH];
    static const WCHAR wildcardW[] = {'*','.','*',0};

    path = ldt_get_ptr(context->SegDs, context->Edx);
    MultiByteToWideChar(CP_OEMCP, 0, path, -1, pathW, MAX_PATH);

    p = wcsrchr( pathW, '\\');
    q = wcsrchr( pathW, '/');
    if (q>p) p = q;
    if (!p)
    {
        if (pathW[0] && pathW[1] == ':') p = pathW + 2;
        else p = pathW;
    }
    else p++;

    /* Note: terminating NULL in dta->mask overwrites dta->search_attr
     *       (doesn't matter as it is set below anyway)
     */
    if (!INT21_ToDosFCBFormat( p, maskW ))
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
        SET_AX( context, ERROR_FILE_NOT_FOUND );
        SET_CFLAG(context);
        return FALSE;
    }
    WideCharToMultiByte(CP_OEMCP, 0, maskW, 12, dta->mask, sizeof(dta->mask), NULL, NULL);

    dta->fullPath = HeapAlloc( GetProcessHeap(), 0, sizeof(wildcardW) + (p - pathW)*sizeof(WCHAR) );
    memcpy( dta->fullPath, pathW, (p - pathW) * sizeof(WCHAR) );
    memcpy( dta->fullPath + (p - pathW), wildcardW, sizeof(wildcardW) );
    /* we must have a fully qualified file name in dta->fullPath
     * (we could have a UNC path, but this would lead to some errors later on)
     */
    dta->drive = drive_number( dta->fullPath[0] );
    dta->count = 0;
    dta->search_attr = CL_reg(context);
    return TRUE;
}

/******************************************************************
 *		match_short
 *
 * Check is a short path name (DTA unicode) matches a mask (FCB ansi)
 */
static BOOL match_short(LPCWSTR shortW, LPCSTR maskA)
{
    WCHAR mask[11], file[12];
    int i;

    if (!INT21_ToDosFCBFormat( shortW, file )) return FALSE;
    MultiByteToWideChar(CP_OEMCP, 0, maskA, 11, mask, 11);
    for (i = 0; i < 11; i++)
        if (mask[i] != '?' && mask[i] != file[i]) return FALSE;
    return TRUE;
}

static unsigned INT21_FindHelper(LPCWSTR fullPath, unsigned drive, unsigned count, 
                                 LPCSTR mask, unsigned search_attr, 
                                 WIN32_FIND_DATAW* entry)
{
    unsigned ncalls;

    if ((search_attr & ~(FA_UNUSED | FA_ARCHIVE | FA_RDONLY)) == FA_LABEL)
    {
        WCHAR path[] = {' ',':','\\',0};

        if (count) return 0;
        path[0] = drive + 'A';
        if (!GetVolumeInformationW(path, entry->cAlternateFileName, 13, NULL, NULL, NULL, NULL, 0)) return 0;
        if (!entry->cAlternateFileName[0]) return 0;
        RtlSecondsSince1970ToTime( 0, (LARGE_INTEGER *)&entry->ftCreationTime );
        RtlSecondsSince1970ToTime( 0, (LARGE_INTEGER *)&entry->ftLastAccessTime );
        RtlSecondsSince1970ToTime( 0, (LARGE_INTEGER *)&entry->ftLastWriteTime );
        entry->dwFileAttributes = FA_LABEL;
        entry->nFileSizeHigh = entry->nFileSizeLow = 0;
        TRACE("returning %s as label\n", debugstr_w(entry->cAlternateFileName));
        return 1;
    }

    if (!INT21_FindHandle || INT21_FindPath != fullPath || count == 0)
    {
        if (INT21_FindHandle) FindClose(INT21_FindHandle);
        INT21_FindHandle = FindFirstFileW(fullPath, entry);
        if (INT21_FindHandle == INVALID_HANDLE_VALUE)
        {
            INT21_FindHandle = 0;
            return 0;
        }
        INT21_FindPath = fullPath;
        /* we need to resync search */
        ncalls = count;
    }
    else ncalls = 1;

    while (ncalls-- != 0)
    {
        if (!FindNextFileW(INT21_FindHandle, entry))
        {
            FindClose(INT21_FindHandle); INT21_FindHandle = 0;
            return 0;
        }
    }
    while (count < 0xffff)
    {
        count++;
        /* Check the file attributes, and path */
        if (!(entry->dwFileAttributes & ~search_attr) &&
            match_short(entry->cAlternateFileName[0] ? entry->cAlternateFileName : entry->cFileName,
                        mask))
        {
            return count;
        }
        if (!FindNextFileW(INT21_FindHandle, entry))
        {
            FindClose(INT21_FindHandle); INT21_FindHandle = 0;
            return 0;
        }
    }
    WARN("Too many directory entries in %s\n", debugstr_w(fullPath) );
    return 0;
}

/******************************************************************
 *		INT21_FindNext
 */
static BOOL INT21_FindNext( CONTEXT *context )
{
    FINDFILE_DTA *dta = (FINDFILE_DTA *)INT21_GetCurrentDTA(context);
    DWORD attr = dta->search_attr | FA_UNUSED | FA_ARCHIVE | FA_RDONLY;
    WIN32_FIND_DATAW entry;
    int n;

    if (!dta->fullPath) return FALSE;

    n = INT21_FindHelper(dta->fullPath, dta->drive, dta->count, 
                         dta->mask, attr, &entry);
    if (n)
    {
        dta->fileattr = entry.dwFileAttributes;
        dta->filesize = entry.nFileSizeLow;
        FileTimeToDosDateTime( &entry.ftLastWriteTime, &dta->filedate, &dta->filetime );
        if (entry.cAlternateFileName[0])
            WideCharToMultiByte(CP_OEMCP, 0, entry.cAlternateFileName, -1,
                                dta->filename, 13, NULL, NULL);
        else
            WideCharToMultiByte(CP_OEMCP, 0, entry.cFileName, -1, dta->filename, 13, NULL, NULL);

        if (!memchr(dta->mask,'?',11))
        {
            /* wildcardless search, release resources in case no findnext will
             * be issued, and as a workaround in case file creation messes up
             * findnext, as sometimes happens with pkunzip
             */
            HeapFree( GetProcessHeap(), 0, dta->fullPath );
            INT21_FindPath = dta->fullPath = NULL;
        }
        dta->count = n;
        return TRUE;
    }
    HeapFree( GetProcessHeap(), 0, dta->fullPath );
    INT21_FindPath = dta->fullPath = NULL;
    return FALSE;
}

/* microsoft's programmers should be shot for using CP/M style int21
   calls in Windows for Workgroup's winfile.exe */

/******************************************************************
 *		INT21_FindFirstFCB
 *
 */
static BOOL INT21_FindFirstFCB( CONTEXT *context )
{
    BYTE *fcb = ldt_get_ptr(context->SegDs, context->Edx);
    FINDFILE_FCB *pFCB;
    int drive;
    WCHAR p[] = {' ',':',};

    if (*fcb == 0xff) pFCB = (FINDFILE_FCB *)(fcb + 7);
    else pFCB = (FINDFILE_FCB *)fcb;
    drive = INT21_MapDrive( pFCB->drive );
    if (drive == MAX_DOS_DRIVES) return FALSE;

    p[0] = 'A' + drive;
    pFCB->fullPath = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
    if (!pFCB->fullPath) return FALSE;
    GetLongPathNameW(p, pFCB->fullPath, MAX_PATH);
    pFCB->count = 0;
    return TRUE;
}

/******************************************************************
 *		INT21_FindNextFCB
 *
 */
static BOOL INT21_FindNextFCB( CONTEXT *context )
{
    BYTE *fcb = ldt_get_ptr(context->SegDs, context->Edx);
    FINDFILE_FCB *pFCB;
    LPBYTE pResult = INT21_GetCurrentDTA(context);
    DOS_DIRENTRY_LAYOUT *ddl;
    WIN32_FIND_DATAW entry;
    BYTE attr;
    int n;
    WCHAR nameW[12];

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

    if (!pFCB->fullPath) return FALSE;
    n = INT21_FindHelper(pFCB->fullPath, INT21_MapDrive( pFCB->drive ),
                         pFCB->count, pFCB->filename, attr, &entry);
    if (!n)
    {
        HeapFree( GetProcessHeap(), 0, pFCB->fullPath );
        INT21_FindPath = pFCB->fullPath = NULL;
        return FALSE;
    }
    pFCB->count += n;

    if (*fcb == 0xff)
    {
        /* place extended FCB header before pResult if called with extended FCB */
	*pResult = 0xff;
	 pResult += 6; /* leave reserved field behind */
	*pResult++ = entry.dwFileAttributes;
    }
    *pResult++ = INT21_MapDrive( pFCB->drive ); /* DOS_DIRENTRY_LAYOUT after current drive number */
    ddl = (DOS_DIRENTRY_LAYOUT*)pResult;
    ddl->fileattr = entry.dwFileAttributes;
    ddl->cluster  = 0;  /* what else? */
    ddl->filesize = entry.nFileSizeLow;
    memset( ddl->reserved, 0, sizeof(ddl->reserved) );
    FileTimeToDosDateTime( &entry.ftLastWriteTime,
                           &ddl->filedate, &ddl->filetime );

    /* Convert file name to FCB format */
    if (entry.cAlternateFileName[0])
        INT21_ToDosFCBFormat( entry.cAlternateFileName, nameW );
    else
        INT21_ToDosFCBFormat( entry.cFileName, nameW );
    WideCharToMultiByte(CP_OEMCP, 0, nameW, 11, ddl->filename, 11, NULL, NULL);
    return TRUE;
}


/******************************************************************
 *		INT21_ParseFileNameIntoFCB
 *
 */
static void INT21_ParseFileNameIntoFCB( CONTEXT *context )
{
    char *filename = ldt_get_ptr( context->SegDs, context->Esi );
    char *fcb = ldt_get_ptr( context->SegEs, context->Edi );
    char *s;
    WCHAR *buffer;
    WCHAR fcbW[12];
    INT buffer_len, len;

    SET_AL( context, 0xff ); /* failed */

    TRACE("filename: '%s'\n", filename);

    s = filename;
    while (*s && (*s != ' ') && (*s != '\r') && (*s != '\n'))
        s++;
    len = filename - s;

    buffer_len = MultiByteToWideChar(CP_OEMCP, 0, filename, len, NULL, 0);
    buffer = HeapAlloc( GetProcessHeap(), 0, (buffer_len + 1) * sizeof(WCHAR));
    len = MultiByteToWideChar(CP_OEMCP, 0, filename, len, buffer, buffer_len);
    buffer[len] = 0;
    INT21_ToDosFCBFormat(buffer, fcbW);
    HeapFree(GetProcessHeap(), 0, buffer);
    WideCharToMultiByte(CP_OEMCP, 0, fcbW, 12, fcb + 1, 12, NULL, NULL);
    *fcb = 0;
    TRACE("FCB: '%s'\n", fcb + 1);

    SET_AL( context, ((strchr(filename, '*')) || (strchr(filename, '$'))) != 0 );

    /* point DS:SI to first unparsed character */
    SET_SI( context, context->Esi + (int)s - (int)filename );
}

static BOOL     INT21_Dup2(HFILE16 hFile1, HFILE16 hFile2)
{
    HFILE16     res = HFILE_ERROR16;
    HANDLE      handle, new_handle;
#define DOS_TABLE_SIZE  256
    DWORD       map[DOS_TABLE_SIZE / 32];
    int         i;

    handle = DosFileHandleToWin32Handle(hFile1);
    if (handle == INVALID_HANDLE_VALUE)
        return FALSE;

    _lclose16(hFile2);
    /* now loop to allocate the same one... */
    memset(map, 0, sizeof(map));
    for (i = 0; i < DOS_TABLE_SIZE; i++)
    {
        if (!DuplicateHandle(GetCurrentProcess(), handle,
                             GetCurrentProcess(), &new_handle,
                             0, FALSE, DUPLICATE_SAME_ACCESS))
        {
            res = HFILE_ERROR16;
            break;
        }
        res = Win32HandleToDosFileHandle(new_handle);
        if (res == HFILE_ERROR16 || res == hFile2) break;
        map[res / 32] |= 1 << (res % 32);
    }
    /* clean up the allocated slots */
    for (i = 0; i < DOS_TABLE_SIZE; i++)
    {
        if (map[i / 32] & (1 << (i % 32)))
            _lclose16((HFILE16)i);
    }
    return res == hFile2;
}


/***********************************************************************
 *           DOSVM_Int21Handler
 *
 * Interrupt 0x21 handler.
 */
void WINAPI DOSVM_Int21Handler( CONTEXT *context )
{
    BOOL bSetDOSExtendedError = FALSE;

    TRACE( "AX=%04x BX=%04x CX=%04x DX=%04x "
           "SI=%04x DI=%04x DS=%04x ES=%04x EFL=%08lx\n",
           AX_reg(context), BX_reg(context), 
           CX_reg(context), DX_reg(context),
           SI_reg(context), DI_reg(context),
           (WORD)context->SegDs, (WORD)context->SegEs,
           context->EFlags );

   /*
    * Extended error is used by (at least) functions 0x2f to 0x62.
    * Function 0x59 returns extended error and error should not
    * be cleared before handling the function.
    */
    if (AH_reg(context) >= 0x2f && AH_reg(context) != 0x59) 
        SetLastError(0);

    RESET_CFLAG(context); /* Not sure if this is a good idea. */

    switch(AH_reg(context))
    {
    case 0x00: /* TERMINATE PROGRAM */
        TRACE("TERMINATE PROGRAM\n");
        DOSVM_Exit( 0 );
        break;

    case 0x01: /* READ CHARACTER FROM STANDARD INPUT, WITH ECHO */
        TRACE("DIRECT CHARACTER INPUT WITH ECHO - not supported\n");
        break;

    case 0x02: /* WRITE CHARACTER TO STANDARD OUTPUT */
        TRACE("Write Character to Standard Output\n");
        break;

    case 0x03: /* READ CHARACTER FROM STDAUX  */
    case 0x04: /* WRITE CHARACTER TO STDAUX */
    case 0x05: /* WRITE CHARACTER TO PRINTER */
        INT_BARF( context, 0x21 );
        break;

    case 0x06: /* DIRECT CONSOLE IN/OUTPUT */
        if (DL_reg(context) == 0xff) 
        {
            TRACE("Direct Console Input\n");

            /* no character available */
            SET_AL( context, 0 );
            SET_ZFLAG( context );
        } 
        else 
        {
            TRACE("Direct Console Output\n");
            /*
             * At least DOS versions 2.1-7.0 return character 
             * that was written in AL register.
             */
            SET_AL( context, DL_reg(context) );
        }
        break;

    case 0x07: /* DIRECT CHARACTER INPUT WITHOUT ECHO */
        TRACE("DIRECT CHARACTER INPUT WITHOUT ECHO - not supported\n");
        SET_AL( context, 0 );
        break;

    case 0x08: /* CHARACTER INPUT WITHOUT ECHO */
        TRACE("CHARACTER INPUT WITHOUT ECHO - not supported\n");
        SET_AL( context, 0 );
        break;

    case 0x09: /* WRITE STRING TO STANDARD OUTPUT */
        TRACE("WRITE '$'-terminated string from %04lX:%04X to stdout\n",
	      context->SegDs, DX_reg(context) );
        {
            LPSTR data = ldt_get_ptr( context->SegDs, context->Edx );
            LPSTR p = data;
            DWORD w;
            /*
             * Do NOT use strchr() to calculate the string length,
             * as '\0' is valid string content, too!
             * Maybe we should check for non-'$' strings, but DOS doesn't.
             */
            while (*p != '$') p++;

            WriteFile( DosFileHandleToWin32Handle(1), data, p - data, &w, NULL );
            SET_AL( context, '$' ); /* yes, '$' (0x24) gets returned in AL */
        }
        break;

    case 0x0a: /* BUFFERED INPUT */
        TRACE( "BUFFERED INPUT - not supported\n" );
        break;

    case 0x0b: /* GET STDIN STATUS */
        TRACE( "GET STDIN STATUS - not supported\n" );
        SET_AL( context, 0 ); /* no character available */
        break;

    case 0x0c: /* FLUSH BUFFER AND READ STANDARD INPUT */
        {
            BYTE al = AL_reg(context); /* Input function to execute after flush. */

            TRACE( "FLUSH BUFFER AND READ STANDARD INPUT - 0x%02x\n", al );

            /* FIXME: buffers are not flushed */

            /*
             * If AL is one of 0x01, 0x06, 0x07, 0x08, or 0x0a,
             * int21 function identified by AL will be called.
             */
            if(al == 0x01 || al == 0x06 || al == 0x07 || al == 0x08 || al == 0x0a)
            {
                SET_AH( context, al );
                DOSVM_Int21Handler( context );
            }
        }
        break;

    case 0x0d: /* DISK BUFFER FLUSH */
        TRACE("DISK BUFFER FLUSH ignored\n");
        break;

    case 0x0e: /* SELECT DEFAULT DRIVE */
        TRACE( "SELECT DEFAULT DRIVE - %c:\n", 'A' + DL_reg(context) );
        INT21_SetCurrentDrive( DL_reg(context) );
        SET_AL( context, MAX_DOS_DRIVES );
        break;

    case 0x0f: /* OPEN FILE USING FCB */
        INT21_OpenFileUsingFCB( context );
        break;

    case 0x10: /* CLOSE FILE USING FCB */
        INT21_CloseFileUsingFCB( context );
        break;

    case 0x11: /* FIND FIRST MATCHING FILE USING FCB */
	TRACE("FIND FIRST MATCHING FILE USING FCB %p\n",
	      ldt_get_ptr(context->SegDs, context->Edx));
        if (!INT21_FindFirstFCB(context))
        {
            SET_AL( context, 0xff );
            break;
        }
        /* else fall through */

    case 0x12: /* FIND NEXT MATCHING FILE USING FCB */
        SET_AL( context, INT21_FindNextFCB(context) ? 0x00 : 0xff );
        break;

     case 0x13: /* DELETE FILE USING FCB */
        INT_BARF( context, 0x21 );
        break;

    case 0x14: /* SEQUENTIAL READ FROM FCB FILE */
        INT21_SequentialReadFromFCB( context );
        break;

    case 0x15: /* SEQUENTIAL WRITE TO FCB FILE */
        INT21_SequentialWriteToFCB( context );
        break;

    case 0x16: /* CREATE OR TRUNCATE FILE USING FCB */
    case 0x17: /* RENAME FILE USING FCB */
        INT_BARF( context, 0x21 );
        break;

    case 0x18: /* NULL FUNCTION FOR CP/M COMPATIBILITY */
        SET_AL( context, 0 );
        break;

    case 0x19: /* GET CURRENT DEFAULT DRIVE */
        SET_AL( context, INT21_GetCurrentDrive() );
        TRACE( "GET CURRENT DRIVE -> %c:\n", 'A' + AL_reg( context ) );
        break;

    case 0x1a: /* SET DISK TRANSFER AREA ADDRESS */
        TRACE( "SET DISK TRANSFER AREA ADDRESS %04lX:%04X\n",
               context->SegDs, DX_reg(context) );
        {
            TDB *task = GlobalLock16( GetCurrentTask() );
            task->dta = MAKESEGPTR( context->SegDs, DX_reg(context) );
        }
        break;

    case 0x1b: /* GET ALLOCATION INFORMATION FOR DEFAULT DRIVE */
        if (!INT21_GetDriveAllocInfo(context, 0))
            SET_AX( context, 0xffff );
        break;

    case 0x1c: /* GET ALLOCATION INFORMATION FOR SPECIFIC DRIVE */
        if (!INT21_GetDriveAllocInfo(context, DL_reg(context)))
            SET_AX( context, 0xffff );
        break;

    case 0x1d: /* NULL FUNCTION FOR CP/M COMPATIBILITY */
    case 0x1e: /* NULL FUNCTION FOR CP/M COMPATIBILITY */
        SET_AL( context, 0 );
        break;

    case 0x1f: /* GET DRIVE PARAMETER BLOCK FOR DEFAULT DRIVE */
        {
            BYTE drive = INT21_MapDrive( 0 ); 
            TRACE( "GET DPB FOR DEFAULT DRIVE\n" );

            if (INT21_FillDrivePB( drive ))
            {
                SET_AL( context, 0x00 ); /* success */
                SET_BX( context, offsetof( INT21_HEAP, misc_dpb_list[drive] ) );
                context->SegDs = INT21_GetHeapSelector( context );
            }
            else
            {
                SET_AL( context, 0xff ); /* invalid or network drive */
            }
        }
        break;

    case 0x20: /* NULL FUNCTION FOR CP/M COMPATIBILITY */
        SET_AL( context, 0 );
        break;

    case 0x21: /* READ RANDOM RECORD FROM FCB FILE */
        INT21_ReadRandomRecordFromFCB( context );
        break;

    case 0x22: /* WRITE RANDOM RECORD TO FCB FILE */
        INT21_WriteRandomRecordToFCB( context );
        break;

    case 0x23: /* GET FILE SIZE FOR FCB */
    case 0x24: /* SET RANDOM RECORD NUMBER FOR FCB */
        INT_BARF( context, 0x21 );
        break;

    case 0x25: /* SET INTERRUPT VECTOR */
        TRACE("SET INTERRUPT VECTOR 0x%02x\n",AL_reg(context));
        {
            FARPROC16 ptr = (FARPROC16)MAKESEGPTR( context->SegDs, DX_reg(context) );
            DOSVM_SetPMHandler16(  AL_reg(context), ptr );
        }
        break;

    case 0x26: /* CREATE NEW PROGRAM SEGMENT PREFIX */
        INT_BARF( context, 0x21 );
        break;

    case 0x27: /* RANDOM BLOCK READ FROM FCB FILE */
        INT21_RandomBlockReadFromFCB( context );
        break;

    case 0x28: /* RANDOM BLOCK WRITE TO FCB FILE */
        INT21_RandomBlockWriteToFCB( context );
        break;

    case 0x29: /* PARSE FILENAME INTO FCB */
        INT21_ParseFileNameIntoFCB(context);
        break;

    case 0x2a: /* GET SYSTEM DATE */
        TRACE( "GET SYSTEM DATE\n" );
        {
            SYSTEMTIME systime;
            GetLocalTime( &systime );
            SET_CX( context, systime.wYear );
            SET_DH( context, systime.wMonth );
            SET_DL( context, systime.wDay );
            SET_AL( context, systime.wDayOfWeek );
        }
        break;

    case 0x2b: /* SET SYSTEM DATE */
        TRACE( "SET SYSTEM DATE\n" );
        {
            WORD year  = CX_reg(context);
            BYTE month = DH_reg(context);
            BYTE day   = DL_reg(context);

            if (year  >= 1980 && year  <= 2099 &&
                month >= 1    && month <= 12   &&
                day   >= 1    && day   <= 31)
            {
                FIXME( "SetSystemDate(%02d/%02d/%04d): not allowed\n",
                       day, month, year );
                SET_AL( context, 0 );  /* Let's pretend we succeeded */
            }
            else
            {
                SET_AL( context, 0xff ); /* invalid date */
                TRACE( "SetSystemDate(%02d/%02d/%04d): invalid date\n",
                       day, month, year );
            }
        }
        break;

    case 0x2c: /* GET SYSTEM TIME */
        TRACE( "GET SYSTEM TIME\n" );
        {
            SYSTEMTIME systime;
            GetLocalTime( &systime );
            SET_CH( context, systime.wHour );
            SET_CL( context, systime.wMinute );
            SET_DH( context, systime.wSecond );
            SET_DL( context, systime.wMilliseconds / 10 );
        }
        break;

    case 0x2d: /* SET SYSTEM TIME */
        if( CH_reg(context) >= 24 || CL_reg(context) >= 60 || DH_reg(context) >= 60 || DL_reg(context) >= 100 ) {
            TRACE("SetSystemTime(%02d:%02d:%02d.%02d): wrong time\n",
              CH_reg(context), CL_reg(context),
              DH_reg(context), DL_reg(context) );
            SET_AL( context, 0xFF );
        }
        else
        {
            FIXME("SetSystemTime(%02d:%02d:%02d.%02d): not allowed\n",
                  CH_reg(context), CL_reg(context),
                  DH_reg(context), DL_reg(context) );
            SET_AL( context, 0 );  /* Let's pretend we succeeded */
        }
        break;

    case 0x2e: /* SET VERIFY FLAG */
        TRACE("SET VERIFY FLAG ignored\n");
        /* we cannot change the behaviour anyway, so just ignore it */
        break;

    case 0x2f: /* GET DISK TRANSFER AREA ADDRESS */
        TRACE( "GET DISK TRANSFER AREA ADDRESS\n" );
        {
            TDB *task = GlobalLock16( GetCurrentTask() );
            context->SegEs = SELECTOROF( task->dta );
            SET_BX( context, OFFSETOF( task->dta ) );
        }
        break;

    case 0x30: /* GET DOS VERSION */
        TRACE( "GET DOS VERSION - %s requested\n",
               (AL_reg(context) == 0x00) ? "OEM number" : "version flag" );

        if (AL_reg(context) == 0x00)
            SET_BH( context, 0xff ); /* OEM number => undefined */
        else
            SET_BH( context, 0x08 ); /* version flag => DOS is in ROM */

        SET_AL( context, HIBYTE(HIWORD(GetVersion16())) ); /* major version */
        SET_AH( context, LOBYTE(HIWORD(GetVersion16())) ); /* minor version */

        SET_BL( context, 0x12 );     /* 0x123456 is 24-bit Wine's serial # */
        SET_CX( context, 0x3456 );
        break;

    case 0x31: /* TERMINATE AND STAY RESIDENT */
        FIXME("TERMINATE AND STAY RESIDENT stub\n");
        break;

    case 0x32: /* GET DOS DRIVE PARAMETER BLOCK FOR SPECIFIC DRIVE */
        {
            BYTE drive = INT21_MapDrive( DL_reg(context) );           
            TRACE( "GET DPB FOR SPECIFIC DRIVE %d\n", DL_reg(context) );

            if (INT21_FillDrivePB( drive ))
            {
                SET_AL( context, 0x00 ); /* success */
                SET_BX( context, offsetof( INT21_HEAP, misc_dpb_list[drive] ) );
                context->SegDs = INT21_GetHeapSelector( context );
            }
            else
            {
                SET_AL( context, 0xff ); /* invalid or network drive */
            }
        }
        break;

    case 0x33: /* MULTIPLEXED */
        switch (AL_reg(context))
        {
        case 0x00: /* GET CURRENT EXTENDED BREAK STATE */
            TRACE("GET CURRENT EXTENDED BREAK STATE\n");
            SET_DL( context, brk_flag );
            break;

        case 0x01: /* SET EXTENDED BREAK STATE */
            TRACE("SET CURRENT EXTENDED BREAK STATE\n");
            brk_flag = (DL_reg(context) > 0) ? 1 : 0;
            break;

        case 0x02: /* GET AND SET EXTENDED CONTROL-BREAK CHECKING STATE*/
            TRACE("GET AND SET EXTENDED CONTROL-BREAK CHECKING STATE\n");
            /* ugly coding in order to stay reentrant */
            if (DL_reg(context))
            {
                SET_DL( context, brk_flag );
                brk_flag = 1;
            }
            else
            {
                SET_DL( context, brk_flag );
                brk_flag = 0;
            }
            break;

        case 0x05: /* GET BOOT DRIVE */
            TRACE("GET BOOT DRIVE\n");
            SET_DL( context, 3 );
            /* c: is Wine's bootdrive (a: is 1)*/
            break;

        case 0x06: /* GET TRUE VERSION NUMBER */
            TRACE("GET TRUE VERSION NUMBER\n");
            SET_BL( context, HIBYTE(HIWORD(GetVersion16())) ); /* major */
            SET_BH( context, LOBYTE(HIWORD(GetVersion16())) ); /* minor */
            SET_DL( context, 0x00 ); /* revision */
            SET_DH( context, 0x08 ); /* DOS is in ROM */
            break;

        default:
            INT_BARF( context, 0x21 );
            break;
        }
        break;

    case 0x34: /* GET ADDRESS OF INDOS FLAG */
        TRACE( "GET ADDRESS OF INDOS FLAG\n" );
        context->SegEs = INT21_GetHeapSelector( context );
        SET_BX( context, offsetof(INT21_HEAP, misc_indos) );
        break;

    case 0x35: /* GET INTERRUPT VECTOR */
        TRACE("GET INTERRUPT VECTOR 0x%02x\n",AL_reg(context));
        {
            FARPROC16 addr = DOSVM_GetPMHandler16( AL_reg(context) );
            context->SegEs = SELECTOROF(addr);
            SET_BX( context, OFFSETOF(addr) );
        }
        break;

    case 0x36: /* GET FREE DISK SPACE */
	TRACE("GET FREE DISK SPACE FOR DRIVE %s (limited to about 1GB)\n",
	      INT21_DriveName( DL_reg(context) ));
        if (!INT21_GetFreeDiskSpace(context)) SET_AX( context, 0xffff );
        break;

    case 0x37: /* SWITCHAR */
        {
            switch (AL_reg(context))
            {
            case 0x00: /* "SWITCHAR" - GET SWITCH CHARACTER */
                TRACE( "SWITCHAR - GET SWITCH CHARACTER\n" );
                SET_AL( context, 0x00 ); /* success*/
                SET_DL( context, '/' );
                break;
            case 0x01: /*"SWITCHAR" - SET SWITCH CHARACTER*/
                FIXME( "SWITCHAR - SET SWITCH CHARACTER: %c\n",
                       DL_reg( context ));
                SET_AL( context, 0x00 ); /* success*/
                break;
            default:
                INT_BARF( context, 0x21 );
                break;
            }
	}
	break;

    case 0x38: /* GET COUNTRY-SPECIFIC INFORMATION */
        TRACE( "GET COUNTRY-SPECIFIC INFORMATION\n" );
        if (AL_reg(context))
        {
            WORD country = AL_reg(context);
            if (country == 0xff)
                country = BX_reg(context);
            if (country != INT21_GetSystemCountryCode()) {
                FIXME( "Requested info on non-default country %04x\n", country );
                SET_AX(context, 2);
                SET_CFLAG(context);
            }
        }
        if(AX_reg(context) != 2 )
        {
            INT21_FillCountryInformation( ldt_get_ptr(context->SegDs, context->Edx) );
            SET_AX( context, INT21_GetSystemCountryCode() );
            SET_BX( context, INT21_GetSystemCountryCode() );
            RESET_CFLAG(context);
        }
        break;

    case 0x39: /* "MKDIR" - CREATE SUBDIRECTORY */
        if (!INT21_CreateDirectory( context ))
            bSetDOSExtendedError = TRUE;
        else
            RESET_CFLAG(context);
        break;

    case 0x3a: /* "RMDIR" - REMOVE DIRECTORY */
        {
            WCHAR dirW[MAX_PATH];
            char *dirA = ldt_get_ptr(context->SegDs, context->Edx);

            TRACE( "REMOVE DIRECTORY %s\n", dirA );

            MultiByteToWideChar(CP_OEMCP, 0, dirA, -1, dirW, MAX_PATH);

            if (!RemoveDirectoryW( dirW ))
                bSetDOSExtendedError = TRUE;
            else
                RESET_CFLAG(context);
        }
        break;

    case 0x3b: /* "CHDIR" - SET CURRENT DIRECTORY */
        if (!INT21_SetCurrentDirectory( context ))
            bSetDOSExtendedError = TRUE;
        else
            RESET_CFLAG(context);
        break;

    case 0x3c: /* "CREAT" - CREATE OR TRUNCATE FILE */
        if (!INT21_CreateFile( context, context->Edx, FALSE, 
                               OF_READWRITE | OF_SHARE_COMPAT, 0x12 ))
            bSetDOSExtendedError = TRUE;
        else
            RESET_CFLAG(context);
        break;

    case 0x3d: /* "OPEN" - OPEN EXISTING FILE */
        if (!INT21_CreateFile( context, context->Edx, FALSE, 
                               AL_reg(context), 0x01 ))
            bSetDOSExtendedError = TRUE;
        else
            RESET_CFLAG(context);
        break;

    case 0x3e: /* "CLOSE" - CLOSE FILE */
        TRACE( "CLOSE handle %d\n", BX_reg(context) );
        if (_lclose16( BX_reg(context) ) == HFILE_ERROR16)
            bSetDOSExtendedError = TRUE;
        else
            RESET_CFLAG(context);
        break;

    case 0x3f: /* "READ" - READ FROM FILE OR DEVICE */
        TRACE( "READ from %d to %04lX:%04X for %d bytes\n",
               BX_reg(context),
               context->SegDs,
               DX_reg(context),
               CX_reg(context) );
        {
            DWORD result;
            WORD  count  = CX_reg(context);
            BYTE *buffer = ldt_get_ptr( context->SegDs, context->Edx );

            /* Some programs pass a count larger than the allocated buffer */
            DWORD maxcount = GetSelectorLimit16( context->SegDs ) - DX_reg(context) + 1;
            if (count > maxcount) count = maxcount;

            /*
             * FIXME: Reading from console (BX=1) in DOS mode
             *        does not work as it is supposed to work.
             */

            RESET_CFLAG(context); /* set if error */
            if (ReadFile( DosFileHandleToWin32Handle(BX_reg(context)), buffer, count, &result, NULL ))
                SET_AX( context, (WORD)result );
            else
                bSetDOSExtendedError = TRUE;
        }
        break;

    case 0x40:  /* "WRITE" - WRITE TO FILE OR DEVICE */
        TRACE( "WRITE from %04lX:%04X to handle %d for %d byte\n",
               context->SegDs, DX_reg(context),
               BX_reg(context), CX_reg(context) );
        {
            char *ptr = ldt_get_ptr(context->SegDs, context->Edx);
            HFILE handle = (HFILE)DosFileHandleToWin32Handle(BX_reg(context));
            LONG result = _hwrite( handle, ptr, CX_reg(context) );
            if (result == HFILE_ERROR)
                bSetDOSExtendedError = TRUE;
            else
            {
                SET_AX( context, (WORD)result );
                RESET_CFLAG(context);
            }
        }
        break;

    case 0x41: /* "UNLINK" - DELETE FILE */
        {
            WCHAR fileW[MAX_PATH];
            char *fileA = ldt_get_ptr(context->SegDs, context->Edx);

            TRACE( "UNLINK %s\n", fileA );
            MultiByteToWideChar(CP_OEMCP, 0, fileA, -1, fileW, MAX_PATH);

            if (!DeleteFileW( fileW ))
                bSetDOSExtendedError = TRUE;
            else
                RESET_CFLAG(context);
        }
        break;

    case 0x42: /* "LSEEK" - SET CURRENT FILE POSITION */
        TRACE( "LSEEK handle %d offset %ld from %s\n",
               BX_reg(context), 
               MAKELONG( DX_reg(context), CX_reg(context) ),
               (AL_reg(context) == 0) ? 
               "start of file" : ((AL_reg(context) == 1) ? 
                                  "current file position" : "end of file") );
        {
            HANDLE handle = DosFileHandleToWin32Handle(BX_reg(context));
            LONG   offset = MAKELONG( DX_reg(context), CX_reg(context) );
            DWORD  status = SetFilePointer( handle, offset, 
                                            NULL, AL_reg(context) );
            if (status == INVALID_SET_FILE_POINTER)
                bSetDOSExtendedError = TRUE;
            else
            {
                SET_AX( context, LOWORD(status) );
                SET_DX( context, HIWORD(status) );
                RESET_CFLAG(context);
            }
        }
        break;

    case 0x43: /* FILE ATTRIBUTES */
        if (!INT21_FileAttributes( context, AL_reg(context), FALSE ))
            bSetDOSExtendedError = TRUE;
        else
            RESET_CFLAG(context);
        break;

    case 0x44: /* IOCTL */
        INT21_Ioctl( context );
        break;

    case 0x45: /* "DUP" - DUPLICATE FILE HANDLE */
        TRACE( "DUPLICATE FILE HANDLE %d\n", BX_reg(context) );
        {
            HANDLE handle32;
            HFILE  handle16 = HFILE_ERROR;

            if (DuplicateHandle( GetCurrentProcess(),
                                 DosFileHandleToWin32Handle(BX_reg(context)),
                                 GetCurrentProcess(), 
                                 &handle32,
                                 0, TRUE, DUPLICATE_SAME_ACCESS ))
                handle16 = Win32HandleToDosFileHandle(handle32);

            if (handle16 == HFILE_ERROR)
                bSetDOSExtendedError = TRUE;
            else
            {
                SET_AX( context, handle16 );
                RESET_CFLAG(context);
            }
        }
        break;

    case 0x46: /* "DUP2", "FORCEDUP" - FORCE DUPLICATE FILE HANDLE */
        TRACE( "FORCEDUP - FORCE DUPLICATE FILE HANDLE %d to %d\n",
               BX_reg(context), CX_reg(context) );
        if (!INT21_Dup2(BX_reg(context), CX_reg(context)))
            bSetDOSExtendedError = TRUE;
        else
            RESET_CFLAG(context);
        break;

    case 0x47: /* "CWD" - GET CURRENT DIRECTORY */
        if (!INT21_GetCurrentDirectory( context, FALSE ))
            bSetDOSExtendedError = TRUE;
        else
            RESET_CFLAG(context);
        break;

    case 0x48: /* ALLOCATE MEMORY */
        TRACE( "ALLOCATE MEMORY for %d paragraphs\n", BX_reg(context) );
        {
            DWORD bytes = (DWORD)BX_reg(context) << 4;
            DWORD rv = GlobalDOSAlloc16( bytes );
            WORD selector = LOWORD( rv );

            if (selector)
            {
                SET_AX( context, selector );
                RESET_CFLAG(context);
            }
            else
            {
                SET_CFLAG(context);
                SET_AX( context, 0x0008 ); /* insufficient memory */
                SET_BX( context, DOSMEM_Available() >> 4 );
            }
        }
	break;

    case 0x49: /* FREE MEMORY */
        TRACE( "FREE MEMORY segment %04lX\n", context->SegEs );
        {
            BOOL ok = !GlobalDOSFree16( context->SegEs );

            /* If we don't reset ES_reg, we will fail in the relay code */
            if (ok) context->SegEs = 0;
            else
            {
                TRACE("FREE MEMORY failed\n");
                SET_CFLAG(context);
                SET_AX( context, 0x0009 ); /* memory block address invalid */
            }
	}
        break;

    case 0x4a: /* RESIZE MEMORY BLOCK */
        TRACE( "RESIZE MEMORY segment %04lX to %d paragraphs\n",
               context->SegEs, BX_reg(context) );
        {
            FIXME( "Resize memory block - unsupported under Win16\n" );
            SET_CFLAG(context);
        }
        break;

    case 0x4b: /* "EXEC" - LOAD AND/OR EXECUTE PROGRAM */
        {
            char *program = ldt_get_ptr(context->SegDs, context->Edx);
            HINSTANCE16 instance;

            TRACE( "EXEC %s\n", program );

            RESET_CFLAG(context);
            instance = WinExec16( program, SW_NORMAL );
            if (instance < 32)
            {
                SET_CFLAG( context );
                SET_AX( context, instance );
            }
        }
        break;

    case 0x4c: /* "EXIT" - TERMINATE WITH RETURN CODE */
        TRACE( "EXIT with return code %d\n", AL_reg(context) );
        DOSVM_Exit( AL_reg(context) );
        break;

    case 0x4d: /* GET RETURN CODE */
        TRACE("GET RETURN CODE (ERRORLEVEL)\n");
        SET_AX( context, 0 );
        break;

    case 0x4e: /* "FINDFIRST" - FIND FIRST MATCHING FILE */
        TRACE("FINDFIRST mask 0x%04x spec %s\n",CX_reg(context),
	      debugstr_a( ldt_get_ptr( context->SegDs, context->Edx )));
        if (!INT21_FindFirst(context)) break;
        /* fall through */

    case 0x4f: /* "FINDNEXT" - FIND NEXT MATCHING FILE */
        TRACE("FINDNEXT\n");
        if (!INT21_FindNext(context))
        {
            SetLastError( ERROR_NO_MORE_FILES );
            SET_AX( context, ERROR_NO_MORE_FILES );
            SET_CFLAG(context);
        }
        else SET_AX( context, 0 );  /* OK */
        break;

    case 0x50: /* SET CURRENT PROCESS ID (SET PSP ADDRESS) */
        TRACE("SET CURRENT PROCESS ID (SET PSP ADDRESS)\n");
        DOSVM_psp = BX_reg(context);
        break;

    case 0x51: /* GET PSP ADDRESS */
        INT21_GetPSP( context );
        break;

    case 0x52: /* "SYSVARS" - GET LIST OF LISTS */
        TRACE("Get List of Lists - not supported\n");
        context->SegEs = 0;
        SET_BX( context, 0 );
        break;

    case 0x54: /* Get Verify Flag */
        TRACE("Get Verify Flag - Not Supported\n");
        SET_AL( context, 0x00 );  /* pretend we can tell. 00h = off 01h = on */
        break;

    case 0x56: /* "RENAME" - RENAME FILE */
        if (!INT21_RenameFile( context ))
            bSetDOSExtendedError = TRUE;
        else
            RESET_CFLAG(context);
        break;

    case 0x57: /* FILE DATE AND TIME */
        if (!INT21_FileDateTime( context ))
            bSetDOSExtendedError = TRUE;
        else
            RESET_CFLAG(context);
        break;

    case 0x58: /* GET OR SET MEMORY ALLOCATION STRATEGY */
        switch (AL_reg(context))
        {
        case 0x00: /* GET MEMORY ALLOCATION STRATEGY */
            TRACE( "GET MEMORY ALLOCATION STRATEGY\n" );
            SET_AX( context, 0 ); /* low memory first fit */
            break;

        case 0x01: /* SET ALLOCATION STRATEGY */
            TRACE( "SET MEMORY ALLOCATION STRATEGY to %d - ignored\n",
                   BL_reg(context) );
            break;

        case 0x02: /* GET UMB LINK STATE */
            TRACE( "GET UMB LINK STATE\n" );
            SET_AL( context, 0 ); /* UMBs not part of DOS memory chain */
            break;

        case 0x03: /* SET UMB LINK STATE */
            TRACE( "SET UMB LINK STATE to %d - ignored\n",
                   BX_reg(context) );
            break;

        default:
            INT_BARF( context, 0x21 );
        }
        break;

    case 0x59: /* GET EXTENDED ERROR INFO */
        INT21_GetExtendedError( context );
        break;

    case 0x5a: /* CREATE TEMPORARY FILE */
        TRACE("CREATE TEMPORARY FILE\n");
        bSetDOSExtendedError = !INT21_CreateTempFile(context);
        break;

    case 0x5b: /* CREATE NEW FILE */ 
        if (!INT21_CreateFile( context, context->Edx, FALSE,
                               OF_READWRITE | OF_SHARE_COMPAT, 0x10 ))
            bSetDOSExtendedError = TRUE;
        else
            RESET_CFLAG(context);
        break;

    case 0x5c: /* "FLOCK" - RECORD LOCKING */
        {
            DWORD  offset = MAKELONG(DX_reg(context), CX_reg(context));
            DWORD  length = MAKELONG(DI_reg(context), SI_reg(context));
            HANDLE handle = DosFileHandleToWin32Handle(BX_reg(context));

            RESET_CFLAG(context);
            switch (AL_reg(context))
            {
            case 0x00: /* LOCK */
                TRACE( "lock handle %d offset %ld length %ld\n",
                       BX_reg(context), offset, length );
                if (!LockFile( handle, offset, 0, length, 0 ))
                    bSetDOSExtendedError = TRUE;
                break;

            case 0x01: /* UNLOCK */
                TRACE( "unlock handle %d offset %ld length %ld\n",
                       BX_reg(context), offset, length );
                if (!UnlockFile( handle, offset, 0, length, 0 ))
                    bSetDOSExtendedError = TRUE;
                break;

            default:
                INT_BARF( context, 0x21 );
            }
        }
        break;

    case 0x5d: /* NETWORK 5D */
        FIXME( "Network function 5D not implemented.\n" );
        SetLastError( ER_NoNetwork );
        bSetDOSExtendedError = TRUE;
        break;

    case 0x5e: /* NETWORK 5E */
        bSetDOSExtendedError = INT21_NetworkFunc( context);
        break;

    case 0x5f: /* NETWORK 5F */
        /* FIXME: supporting this would need to 1:
         * - implement per drive current directory (as kernel32 doesn't)
         * - assign enabled/disabled flag on a per drive basis
         */
        /* network software not installed */
        TRACE("NETWORK function AX=%04x not implemented\n",AX_reg(context));
        SetLastError( ER_NoNetwork );
        bSetDOSExtendedError = TRUE;
        break;

    case 0x60: /* "TRUENAME" - CANONICALIZE FILENAME OR PATH */
        {
            WCHAR       pathW[MAX_PATH], res[MAX_PATH];
            /* FIXME: likely to be broken */

            TRACE("TRUENAME %s\n", debugstr_a(ldt_get_ptr(context->SegDs,context->Esi)));
            MultiByteToWideChar(CP_OEMCP, 0, ldt_get_ptr(context->SegDs, context->Esi), -1, pathW, MAX_PATH);
            if (!GetFullPathNameW( pathW, 128, res, NULL ))
		bSetDOSExtendedError = TRUE;
            else
            {
                SET_AX( context, 0 );
                WideCharToMultiByte(CP_OEMCP, 0, res, -1, ldt_get_ptr(context->SegEs, context->Edi), 128, NULL, NULL);
            }
        }
        break;

    case 0x61: /* UNUSED */
        SET_AL( context, 0 );
        break;

    case 0x62: /* GET PSP ADDRESS */
        INT21_GetPSP( context );
        break;

    case 0x63: /* MISC. LANGUAGE SUPPORT */
        switch (AL_reg(context)) {
        case 0x00: /* GET DOUBLE BYTE CHARACTER SET LEAD-BYTE TABLE */
            TRACE( "GET DOUBLE BYTE CHARACTER SET LEAD-BYTE TABLE\n" );
            context->SegDs = INT21_GetHeapSelector( context );
            SET_SI( context, offsetof(INT21_HEAP, dbcs_table) );
            SET_AL( context, 0 ); /* success */
            break;
        }
        break;

    case 0x64: /* OS/2 DOS BOX */
        INT_BARF( context, 0x21 );
        SET_CFLAG(context);
    	break;

    case 0x65: /* EXTENDED COUNTRY INFORMATION */
        INT21_ExtendedCountryInformation( context );
        break;

    case 0x66: /* GLOBAL CODE PAGE TABLE */
        switch (AL_reg(context))
        {
        case 0x01:
            TRACE( "GET GLOBAL CODE PAGE TABLE\n" );
            SET_BX( context, GetOEMCP() );
            SET_DX( context, GetOEMCP() );
            break;
        case 0x02:
            FIXME( "SET GLOBAL CODE PAGE TABLE, active %d, system %d - ignored\n",
                   BX_reg(context), DX_reg(context) );
            break;
        }
        break;

    case 0x67: /* SET HANDLE COUNT */
        TRACE( "SET HANDLE COUNT to %d\n", BX_reg(context) );
        if (SetHandleCount( BX_reg(context) ) < BX_reg(context) )
            bSetDOSExtendedError = TRUE;
        break;

    case 0x68: /* "FFLUSH" - COMMIT FILE */
        TRACE( "FFLUSH - handle %d\n", BX_reg(context) );
        if (!FlushFileBuffers( DosFileHandleToWin32Handle(BX_reg(context)) ) &&
                GetLastError() != ERROR_ACCESS_DENIED)
            bSetDOSExtendedError = TRUE;
        break;

    case 0x69: /* DISK SERIAL NUMBER */
        switch (AL_reg(context))
        {
        case 0x00:
	    TRACE("GET DISK SERIAL NUMBER for drive %s\n",
		  INT21_DriveName(BL_reg(context)));
            if (!INT21_GetDiskSerialNumber(context)) bSetDOSExtendedError = TRUE;
            else SET_AX( context, 0 );
            break;

        case 0x01:
	    TRACE("SET DISK SERIAL NUMBER for drive %s\n",
		  INT21_DriveName(BL_reg(context)));
            if (!INT21_SetDiskSerialNumber(context)) bSetDOSExtendedError = TRUE;
            else SET_AX( context, 1 );
            break;
        }
        break;

    case 0x6a: /* COMMIT FILE */
        TRACE( "COMMIT FILE - handle %d\n", BX_reg(context) );
        if (!FlushFileBuffers( DosFileHandleToWin32Handle(BX_reg(context)) ) &&
                GetLastError() != ERROR_ACCESS_DENIED)
            bSetDOSExtendedError = TRUE;
        break;

    case 0x6b: /* NULL FUNCTION FOR CP/M COMPATIBILITY */
        SET_AL( context, 0 );
        break;

    case 0x6c: /* EXTENDED OPEN/CREATE */
        if (!INT21_CreateFile( context, context->Esi, TRUE,
                               BX_reg(context), DL_reg(context) ))
            bSetDOSExtendedError = TRUE;
        break;

    case 0x70: /* MSDOS 7 - GET/SET INTERNATIONALIZATION INFORMATION */
        FIXME( "MS-DOS 7 - GET/SET INTERNATIONALIZATION INFORMATION\n" );
        SET_CFLAG( context );
        SET_AL( context, 0 );
        break;

    case 0x71: /* MSDOS 7 - LONG FILENAME FUNCTIONS */
        INT21_LongFilename( context );
        break;

    case 0x73: /* MSDOS7 - FAT32 */
        RESET_CFLAG( context );
        if (!INT21_Fat32( context ))
            bSetDOSExtendedError = TRUE;
        break;

    case 0xdc: /* CONNECTION SERVICES - GET CONNECTION NUMBER */
        TRACE( "CONNECTION SERVICES - GET CONNECTION NUMBER - ignored\n" );
        break;

    case 0xea: /* NOVELL NETWARE - RETURN SHELL VERSION */
        TRACE( "NOVELL NETWARE - RETURN SHELL VERSION - ignored\n" );
        break;

    case 0xff: /* DOS32 EXTENDER (DOS/4GW) - API */
	/* we don't implement a DOS32 extender */
        TRACE( "DOS32 EXTENDER API - ignored\n" );
        break;

    default:
        INT_BARF( context, 0x21 );
        break;

    } /* END OF SWITCH */

    /* Set general error condition. */
    if (bSetDOSExtendedError)
    {
        SET_AX( context, GetLastError() );
        SET_CFLAG( context );
    }

    /* Print error code if carry flag is set. */
    if (context->EFlags & 0x0001)
        TRACE("failed, error %ld\n", GetLastError() );

    TRACE( "returning: AX=%04x BX=%04x CX=%04x DX=%04x "
           "SI=%04x DI=%04x DS=%04x ES=%04x EFL=%08lx\n",
           AX_reg(context), BX_reg(context), 
           CX_reg(context), DX_reg(context), 
           SI_reg(context), DI_reg(context),
           (WORD)context->SegDs, (WORD)context->SegEs,
           context->EFlags );
}
