/*
 * DOS drive handling functions
 *
 * Copyright 1993 Erik Bos
 * Copyright 1996 Alexandre Julliard
 */

#include <string.h>
#include <stdlib.h>

#include "windows.h"
#include "dos_fs.h"
#include "drive.h"
#include "file.h"
#include "msdos.h"
#include "task.h"
#include "xmalloc.h"
#include "stddebug.h"
#include "debug.h"

typedef struct
{
    char *root;      /* root dir in Unix format without trailing '/' */
    char *dos_cwd;   /* cwd in DOS format without leading or trailing '\' */
    char *unix_cwd;  /* cwd in Unix format without leading or trailing '/' */
    char  label[12]; /* drive label */
    DWORD serial;    /* drive serial number */
    WORD  type;      /* drive type */
    BYTE  disabled;  /* disabled flag */
} DOSDRIVE;

static DOSDRIVE DOSDrives[MAX_DOS_DRIVES];
static int DRIVE_CurDrive = 0;

static HTASK DRIVE_LastTask = 0;

/***********************************************************************
 *           DRIVE_Init
 */
int DRIVE_Init(void)
{
    int i, count = 0;
    char drive[2] = "A";
    char path[MAX_PATHNAME_LEN];
    char *p;

    for (i = 0; i < MAX_DOS_DRIVES; i++, drive[0]++)
    {
        GetPrivateProfileString( "drives", drive, "",
                                 path, sizeof(path)-1, WineIniFileName() );
        if (path[0])
        {
            p = path + strlen(path) - 1;
            while ((p > path) && ((*p == '/') || (*p == '\\'))) *p-- = '\0';
            DOSDrives[i].root     = xstrdup( path );
            DOSDrives[i].dos_cwd  = xstrdup( "" );
            DOSDrives[i].unix_cwd = xstrdup( "" );
            sprintf( DOSDrives[i].label, "DRIVE-%c    ", drive[0] );
            DOSDrives[i].serial   = 0x12345678;
            DOSDrives[i].type     = (i < 2) ? DRIVE_REMOVABLE : DRIVE_FIXED;
            DOSDrives[i].disabled = 0;
            count++;
        }
        dprintf_dosfs( stddeb, "Drive %c -> %s\n", 'A' + i,
                       path[0] ? path : "** None **" );
    }

    if (!count) 
    {
        fprintf( stderr, "Warning: no valid DOS drive found\n" );
        /* Create a C drive pointing to Unix root dir */
        DOSDrives[i].root     = xstrdup( "/" );
        DOSDrives[i].dos_cwd  = xstrdup( "" );
        DOSDrives[i].unix_cwd = xstrdup( "" );
        sprintf( DOSDrives[i].label, "DRIVE-%c    ", drive[0] );
        DOSDrives[i].serial   = 0x12345678;
        DOSDrives[i].type     = DRIVE_FIXED;
        DOSDrives[i].disabled = 0;
    }

    /* Make the first hard disk the current drive */
    for (i = 0; i < MAX_DOS_DRIVES; i++, drive[0]++)
    {
        if (DOSDrives[i].root && !DOSDrives[i].disabled &&
            DOSDrives[i].type != DRIVE_REMOVABLE)
        {
            DRIVE_CurDrive = i;
            break;
        }
    }
    return 1;
}


/***********************************************************************
 *           DRIVE_IsValid
 */
int DRIVE_IsValid( int drive )
{
    if ((drive < 0) || (drive >= MAX_DOS_DRIVES)) return 0;
    return (DOSDrives[drive].root && !DOSDrives[drive].disabled);
}


/***********************************************************************
 *           DRIVE_GetCurrentDrive
 */
int DRIVE_GetCurrentDrive(void)
{
    TDB *pTask = (TDB *)GlobalLock( GetCurrentTask() );
    if (pTask && (pTask->curdrive & 0x80)) return pTask->curdrive & ~0x80;
    return DRIVE_CurDrive;
}


/***********************************************************************
 *           DRIVE_SetCurrentDrive
 */
int DRIVE_SetCurrentDrive( int drive )
{
    TDB *pTask = (TDB *)GlobalLock( GetCurrentTask() );
    if (!DRIVE_IsValid( drive ))
    {
        DOS_ERROR( ER_InvalidDrive, EC_MediaError, SA_Abort, EL_Disk );
        return 0;
    }
    dprintf_dosfs( stddeb, "DRIVE_SetCurrentDrive: %c:\n", 'A' + drive );
    DRIVE_CurDrive = drive;
    if (pTask) pTask->curdrive = drive | 0x80;
    return 1;
}


/***********************************************************************
 *           DRIVE_FindDriveRoot
 *
 * Find a drive for which the root matches the begginning of the given path.
 * This can be used to translate a Unix path into a drive + DOS path.
 * Return value is the drive, or -1 on error. On success, path is modified
 * to point to the beginning of the DOS path.
 * FIXME: this only does a textual comparison of the path names, and won't
 *        work well in the presence of symbolic links.
 */
int DRIVE_FindDriveRoot( const char **path )
{
    int drive;
    const char *p1, *p2;

    dprintf_dosfs( stddeb, "DRIVE_FindDriveRoot: searching '%s'\n", *path );
    for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
    {
        if (!DOSDrives[drive].root || DOSDrives[drive].disabled) continue;
        p1 = *path;
        p2 = DOSDrives[drive].root;
        dprintf_dosfs( stddeb, "DRIVE_FindDriveRoot: checking %c: '%s'\n",
                       'A' + drive, p2 );
        for (;;)
        {
            while ((*p1 == '\\') || (*p1 == '/')) p1++;
            while (*p2 == '/') p2++;
            while ((*p1 == *p2) && (*p2) && (*p2 != '/')) p1++, p2++;
            if (!*p2)
            {
                if (IS_END_OF_NAME(*p1)) /* OK, found it */
                {
                    *path = p1;
                    return drive;
                }
            }
            else if (*p2 == '/')
            {
                if (IS_END_OF_NAME(*p1))
                    continue;  /* Go to next path element */
            }
            break;  /* No match, go to next drive */
        }
    }
    return -1;
}


/***********************************************************************
 *           DRIVE_GetRoot
 */
const char * DRIVE_GetRoot( int drive )
{
    if (!DRIVE_IsValid( drive )) return NULL;
    return DOSDrives[drive].root;
}


/***********************************************************************
 *           DRIVE_GetDosCwd
 */
const char * DRIVE_GetDosCwd( int drive )
{
    TDB *pTask = (TDB *)GlobalLock( GetCurrentTask() );
    if (!DRIVE_IsValid( drive )) return NULL;

    /* Check if we need to change the directory to the new task. */
    if (pTask && (pTask->curdrive & 0x80) &&    /* The task drive is valid */
        ((pTask->curdrive & ~0x80) == drive) && /* and it's the one we want */
        (DRIVE_LastTask != GetCurrentTask()))   /* and the task changed */
    {
        /* Perform the task-switch */
        if (!DRIVE_Chdir( drive, pTask->curdir )) DRIVE_Chdir( drive, "\\" );
        DRIVE_LastTask = GetCurrentTask();
    }
    return DOSDrives[drive].dos_cwd;
}


/***********************************************************************
 *           DRIVE_GetUnixCwd
 */
const char * DRIVE_GetUnixCwd( int drive )
{
    TDB *pTask = (TDB *)GlobalLock( GetCurrentTask() );
    if (!DRIVE_IsValid( drive )) return NULL;

    /* Check if we need to change the directory to the new task. */
    if (pTask && (pTask->curdrive & 0x80) &&    /* The task drive is valid */
        ((pTask->curdrive & ~0x80) == drive) && /* and it's the one we want */
        (DRIVE_LastTask != GetCurrentTask()))   /* and the task changed */
    {
        /* Perform the task-switch */
        if (!DRIVE_Chdir( drive, pTask->curdir )) DRIVE_Chdir( drive, "\\" );
        DRIVE_LastTask = GetCurrentTask();
    }
    return DOSDrives[drive].unix_cwd;
}


/***********************************************************************
 *           DRIVE_GetLabel
 */
const char * DRIVE_GetLabel( int drive )
{
    if (!DRIVE_IsValid( drive )) return NULL;
    return DOSDrives[drive].label;
}


/***********************************************************************
 *           DRIVE_GetSerialNumber
 */
DWORD DRIVE_GetSerialNumber( int drive )
{
    if (!DRIVE_IsValid( drive )) return 0;
    return DOSDrives[drive].serial;
}


/***********************************************************************
 *           DRIVE_SetSerialNumber
 */
int DRIVE_SetSerialNumber( int drive, DWORD serial )
{
    if (!DRIVE_IsValid( drive )) return 0;
    DOSDrives[drive].serial = serial;
    return 1;
}


/***********************************************************************
 *           DRIVE_Chdir
 */
int DRIVE_Chdir( int drive, const char *path )
{
    char buffer[MAX_PATHNAME_LEN];
    const char *unix_cwd, *dos_cwd;
    BYTE attr;
    TDB *pTask = (TDB *)GlobalLock( GetCurrentTask() );

    dprintf_dosfs( stddeb, "DRIVE_Chdir(%c:,%s)\n", 'A' + drive, path );
    strcpy( buffer, "A:" );
    buffer[0] += drive;
    lstrcpyn( buffer + 2, path, sizeof(buffer) - 2 );

    if (!(unix_cwd = DOSFS_GetUnixFileName( buffer, TRUE ))) return 0;
    if (!FILE_Stat( unix_cwd, &attr, NULL, NULL, NULL )) return 0;
    if (!(attr & FA_DIRECTORY))
    {
        DOS_ERROR( ER_FileNotFound, EC_NotFound, SA_Abort, EL_Disk );
        return 0;
    }
    unix_cwd += strlen( DOSDrives[drive].root );
    while (*unix_cwd == '/') unix_cwd++;
    buffer[2] = '/';
    lstrcpyn( buffer + 3, unix_cwd, sizeof(buffer) - 3 );
    if (!(dos_cwd = DOSFS_GetDosTrueName( buffer, TRUE ))) return 0;

    dprintf_dosfs( stddeb, "DRIVE_Chdir(%c:): unix_cwd=%s dos_cwd=%s\n",
                   'A' + drive, unix_cwd, dos_cwd + 3 );

    free( DOSDrives[drive].dos_cwd );
    free( DOSDrives[drive].unix_cwd );
    DOSDrives[drive].dos_cwd  = xstrdup( dos_cwd + 3 );
    DOSDrives[drive].unix_cwd = xstrdup( unix_cwd );

    if (pTask && (pTask->curdrive & 0x80) && 
        ((pTask->curdrive & ~0x80) == drive))
    {
        lstrcpyn( pTask->curdir, dos_cwd + 2, sizeof(pTask->curdir) );
        DRIVE_LastTask = GetCurrentTask();
    }
    return 1;
}


/***********************************************************************
 *           DRIVE_Disable
 */
int DRIVE_Disable( int drive  )
{
    if ((drive < 0) || (drive >= MAX_DOS_DRIVES) || !DOSDrives[drive].root)
    {
        DOS_ERROR( ER_InvalidDrive, EC_MediaError, SA_Abort, EL_Disk );
        return 0;
    }
    DOSDrives[drive].disabled = 1;
    return 1;
}


/***********************************************************************
 *           DRIVE_Enable
 */
int DRIVE_Enable( int drive  )
{
    if ((drive < 0) || (drive >= MAX_DOS_DRIVES) || !DOSDrives[drive].root)
    {
        DOS_ERROR( ER_InvalidDrive, EC_MediaError, SA_Abort, EL_Disk );
        return 0;
    }
    DOSDrives[drive].disabled = 0;
    return 1;
}


/***********************************************************************
 *           GetDriveType   (KERNEL.136)
 */
WORD GetDriveType( INT drive )
{
    dprintf_dosfs( stddeb, "GetDriveType(%c:)\n", 'A' + drive );
    if (!DRIVE_IsValid(drive)) return 0;
    return DOSDrives[drive].type;
}
