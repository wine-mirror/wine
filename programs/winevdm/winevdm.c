/*
 * Wine virtual DOS machine
 *
 * Copyright 2003 Alexandre Julliard
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
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "winuser.h"
#include "wincon.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winevdm);

#define DOSBOX "dosbox"

/*** PIF file structures ***/
#include "pshpack1.h"

/* header of a PIF file */
typedef struct {
    BYTE unk1[2];               /* 0x00 */
    CHAR windowtitle[ 30 ];     /* 0x02 seems to be padded with blanks*/ 
    WORD memmax;                /* 0x20 */
    WORD memmin;                /* 0x22 */
    CHAR program[63];           /* 0x24 seems to be zero terminated */
    BYTE hdrflags1;             /* 0x63 various flags:
                                 *  02 286: text mode selected
                                 *  10 close window at exit
                                 */
    BYTE startdrive;            /* 0x64 */
    char startdir[64];          /* 0x65 */
    char optparams[64];         /* 0xa5 seems to be zero terminated */
    BYTE videomode;             /* 0xe5 */
    BYTE unkn2;                 /* 0xe6 ?*/
    BYTE irqlow;                /* 0xe7 */
    BYTE irqhigh;               /* 0xe8 */
    BYTE rows;                  /* 0xe9 */
    BYTE cols;                  /* 0xea */
    BYTE winY;                  /* 0xeb */
    BYTE winX;                  /* 0xec */
    WORD unkn3;                 /* 0xed 7??? */
    CHAR unkn4[64];             /* 0xef */
    CHAR unkn5[64];             /* 0x12f */
    BYTE hdrflags2;             /* 0x16f */
    BYTE hdrflags3;             /* 0x170 */
    } pifhead_t;

/* record header: present on every record */
typedef struct {
    CHAR recordname[16];  /* zero terminated */
    WORD posofnextrecord; /* file offset, 0xffff if last */
    WORD startofdata;     /* file offset */
    WORD sizeofdata;      /* data is expected to follow directly */
} recordhead_t;

/* 386 -enhanced mode- record */
typedef struct {
    WORD memmax;         /* memory desired, overrides the pif header*/
    WORD memmin;         /* memory required, overrides the pif header*/
    WORD prifg;          /* foreground priority */
    WORD pribg;          /* background priority */
    WORD emsmax;         /* EMS memory limit */
    WORD emsmin;         /* EMS memory required */
    WORD xmsmax;         /* XMS memory limit */
    WORD xmsmin;         /* XMS memory required */
    WORD optflags;        /* option flags:
                           *  0008 full screen
                           *  0004 exclusive
                           *  0002 background
                           *  0001 close when active
                           */
    WORD memflags;       /* various memory flags*/
    WORD videoflags;     /* video flags:
                          *   0010 text
                          *   0020 med. res. graphics
                          *   0040 hi. res. graphics
                          */
    WORD hotkey[9];      /* Hot key info */
    CHAR optparams[64];  /* optional params, replaces those in the pif header */
} pif386rec_t;

#include "poppack.h"

/***********************************************************************
 *           start_dosbox
 */
static void start_dosbox( const char *appname, const char *args )
{
    const WCHAR *config_dir = _wgetenv( L"WINECONFIGDIR" );
    WCHAR path[MAX_PATH], config[MAX_PATH];
    HANDLE file;
    char *p, *prefix, *buffer, app[MAX_PATH];
    int i;
    NTSTATUS ret = STATUS_OBJECT_NAME_NOT_FOUND;
    DWORD written, drives = GetLogicalDrives();

    if (!config_dir || !(prefix = wine_get_unix_file_name( config_dir ))) return;
    if (!GetTempPathW( MAX_PATH, path )) return;
    if (!GetTempFileNameW( path, L"cfg", 0, config )) return;
    if (!GetCurrentDirectoryW( MAX_PATH, path )) return;
    if (!GetShortPathNameA( appname, app, MAX_PATH )) return;
    GetShortPathNameW( path, path, MAX_PATH );
    file = CreateFileW( config, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    if (file == INVALID_HANDLE_VALUE) return;

    buffer = HeapAlloc( GetProcessHeap(), 0, sizeof("[autoexec]") +
                        sizeof("mount -z c") + sizeof("config -securemode") +
                        26 * (strlen(prefix) + sizeof("mount c /dosdevices/c:")) +
                        4 * lstrlenW( path ) +
                        6 + strlen( app ) + strlen( args ) + 20 );
    p = buffer;
    p += sprintf( p, "[autoexec]\n" );
    for (i = 25; i >= 0; i--)
        if (!(drives & (1 << i)))
        {
            p += sprintf( p, "mount -z %c\n", 'a' + i );
            break;
        }
    for (i = 0; i <= 25; i++)
    {
        if (!(drives & (1 << i))) continue;
        p += sprintf( p, "mount %c %s/dosdevices/%c:\n", 'a' + i, prefix, 'a' + i );
    }
    p += sprintf( p, "%c:\ncd ", path[0] );
    p += WideCharToMultiByte( CP_UNIXCP, 0, path + 2, -1, p, 4 * lstrlenW(path), NULL, NULL ) - 1;
    p += sprintf( p, "\nconfig -securemode\n" );
    p += sprintf( p, "%s %s\n", app, args );
    p += sprintf( p, "exit\n" );
    if (WriteFile( file, buffer, strlen(buffer), &written, NULL ) && written == strlen(buffer))
    {
        const char *args[5];
        char *config_file = wine_get_unix_file_name( config );
        args[0] = DOSBOX;
        args[1] = "-userconf";
        args[2] = "-conf";
        args[3] = config_file;
        args[4] = NULL;
        ret = __wine_unix_spawnvp( (char **)args, TRUE );
    }
    CloseHandle( file );
    DeleteFileW( config );
    HeapFree( GetProcessHeap(), 0, buffer );
    if (FAILED(ret)) MESSAGE( "winevdm: %s is a DOS application, you need to install DOSBox.\n", appname );
    ExitProcess( ret );
}


/***********************************************************************
 *           read_pif_file
 *pif386rec_tu
 * Read a pif file and return the header and possibly the 286 (real mode)
 * record or 386 (enhanced mode) record. Returns FALSE if the file is
 * invalid otherwise TRUE.
 */
static BOOL read_pif_file( HANDLE hFile, char *progname, char *title,
        char *optparams, char *startdir, int *closeonexit, int *textmode)
{
    DWORD nread;
    LARGE_INTEGER filesize;
    recordhead_t rhead;
    BOOL found386rec = FALSE;
    pif386rec_t pif386rec;
    pifhead_t pifheader;
    if( !GetFileSizeEx( hFile, &filesize) ||
            filesize.QuadPart <  (sizeof(pifhead_t) + sizeof(recordhead_t))) {
        WINE_ERR("Invalid pif file: size error %d\n", (int)filesize.QuadPart);
        return FALSE;
    }
    SetFilePointer( hFile, 0, NULL, FILE_BEGIN);
    if( !ReadFile( hFile, &pifheader, sizeof(pifhead_t), &nread, NULL))
        return FALSE;
    WINE_TRACE("header: program %s title %s startdir %s params %s\n",
            wine_dbgstr_a(pifheader.program),
            wine_dbgstr_an(pifheader.windowtitle, sizeof(pifheader.windowtitle)),
            wine_dbgstr_a(pifheader.startdir),
            wine_dbgstr_a(pifheader.optparams)); 
    WINE_TRACE("header: memory req'd %d desr'd %d drive %d videomode %d\n",
            pifheader.memmin, pifheader.memmax, pifheader.startdrive,
            pifheader.videomode);
    WINE_TRACE("header: flags 0x%x 0x%x 0x%x\n",
            pifheader.hdrflags1, pifheader.hdrflags2, pifheader.hdrflags3);
    ReadFile( hFile, &rhead, sizeof(recordhead_t), &nread, NULL);
    if( strncmp( rhead.recordname, "MICROSOFT PIFEX", 15)) {
        WINE_ERR("Invalid pif file: magic string not found\n");
        return FALSE;
    }
    /* now process the following records */
    while( 1) {
        WORD nextrecord = rhead.posofnextrecord;
        if( (nextrecord & 0x8000) || 
                filesize.QuadPart <( nextrecord + sizeof(recordhead_t))) break;
        if( !SetFilePointer( hFile, nextrecord, NULL, FILE_BEGIN) ||
                !ReadFile( hFile, &rhead, sizeof(recordhead_t), &nread, NULL))
            return FALSE;
        if( !rhead.recordname[0]) continue; /* deleted record */
        WINE_TRACE("reading record %s size %d next 0x%x\n",
                wine_dbgstr_a(rhead.recordname), rhead.sizeofdata,
                rhead.posofnextrecord );
        if( !strncmp( rhead.recordname, "WINDOWS 386", 11)) {
            found386rec = TRUE;
            ReadFile( hFile, &pif386rec, sizeof(pif386rec_t), &nread, NULL);
            WINE_TRACE("386rec: memory req'd %d des'd %d EMS req'd %d des'd %d XMS req'd %d des'd %d\n",
                    pif386rec.memmin, pif386rec.memmax,
                    pif386rec.emsmin, pif386rec.emsmax,
                    pif386rec.xmsmin, pif386rec.xmsmax);
            WINE_TRACE("386rec: option 0x%x memory 0x%x video 0x%x\n",
                    pif386rec.optflags, pif386rec.memflags,
                    pif386rec.videoflags);
            WINE_TRACE("386rec: optional parameters %s\n",
                    wine_dbgstr_a(pif386rec.optparams));
        }
    }
    /* prepare the return data */
    lstrcpynA( progname, pifheader.program, sizeof(pifheader.program)+1);
    lstrcpynA( title, pifheader.windowtitle, sizeof(pifheader.windowtitle)+1);
    if( found386rec)
        lstrcpynA( optparams, pif386rec.optparams, sizeof( pif386rec.optparams)+1);
    else
        lstrcpynA( optparams, pifheader.optparams, sizeof(pifheader.optparams)+1);
    lstrcpynA( startdir, pifheader.startdir, sizeof(pifheader.startdir)+1);
    *closeonexit = pifheader.hdrflags1 & 0x10;
    *textmode = found386rec ? pif386rec.videoflags & 0x0010
                            : pifheader.hdrflags1 & 0x0002;
    return TRUE;
}

/***********************************************************************
 *              pif_cmd
 *
 * execute a pif file.
 */
static VOID pif_cmd( char *filename, char *cmdline)
{ 
    HANDLE hFile;
    char progpath[MAX_PATH];
    char buf[308];
    char progname[64];
    char title[31];
    char optparams[65];
    char startdir[65];
    char *p;
    int closeonexit;
    int textmode;
    if( (hFile = CreateFileA( filename, GENERIC_READ, FILE_SHARE_READ,
                    NULL, OPEN_EXISTING, 0, 0 )) == INVALID_HANDLE_VALUE)
    {
        WINE_ERR("open file %s failed\n", wine_dbgstr_a(filename));
        return;
    }
    if( !read_pif_file( hFile, progname, title, optparams, startdir,
                &closeonexit, &textmode)) {
        WINE_ERR( "failed to read %s\n", wine_dbgstr_a(filename));
        CloseHandle( hFile);
        sprintf( buf, "%s\nInvalid file format. Check your pif file.", 
                filename);
        MessageBoxA( NULL, buf, "16 bit DOS subsystem", MB_OK|MB_ICONWARNING);
        SetLastError( ERROR_BAD_FORMAT);
        return;
    }
    CloseHandle( hFile);
    if( (p = strrchr( progname, '.')) && !strcasecmp( p, ".bat"))
        WINE_FIXME(".bat programs in pif files are not supported.\n"); 
    /* first change dir, so the search below can start from there */
    if( startdir[0] && !SetCurrentDirectoryA( startdir)) {
        WINE_ERR("Cannot change directory %s\n", wine_dbgstr_a( startdir));
        sprintf( buf, "%s\nInvalid startup directory. Check your pif file.", 
                filename);
        MessageBoxA( NULL, buf, "16 bit DOS subsystem", MB_OK|MB_ICONWARNING);
    }
    /* search for the program */
    if( !SearchPathA( NULL, progname, NULL, MAX_PATH, progpath, NULL )) {
        sprintf( buf, "%s\nInvalid program file name. Check your pif file.", 
                filename);
        MessageBoxA( NULL, buf, "16 bit DOS subsystem", MB_OK|MB_ICONERROR);
        SetLastError( ERROR_FILE_NOT_FOUND);
        return;
    }
    if( textmode)
        if( AllocConsole())
            SetConsoleTitleA( title) ;
    /* if no arguments on the commandline, use them from the pif file */
    if( !cmdline[0] && optparams[0])
        cmdline = optparams;
    /* FIXME: do something with:
     * - close on exit
     * - graphic modes
     * - hot key's
     * - etc.
     */ 
    start_dosbox( progpath, cmdline );
}

/***********************************************************************
 *           build_command_line
 *
 * Build the command line of a process from the argv array.
 * Copied from ENV_BuildCommandLine.
 */
static char *build_command_line( char **argv )
{
    int len;
    char *p, **arg, *cmd_line;

    len = 0;
    for (arg = argv; *arg; arg++)
    {
        BOOL has_space;
        int bcount;
        char* a;

        has_space=FALSE;
        bcount=0;
        a=*arg;
        if( !*a ) has_space=TRUE;
        while (*a!='\0') {
            if (*a=='\\') {
                bcount++;
            } else {
                if (*a==' ' || *a=='\t') {
                    has_space=TRUE;
                } else if (*a=='"') {
                    /* doubling of '\' preceding a '"',
                     * plus escaping of said '"'
                     */
                    len+=2*bcount+1;
                }
                bcount=0;
            }
            a++;
        }
        len+=(a-*arg)+1 /* for the separating space */;
        if (has_space)
            len+=2; /* for the quotes */
    }

    if (!(cmd_line = HeapAlloc( GetProcessHeap(), 0, len ? len + 1 : 2 ))) 
        return NULL;

    p = cmd_line;
    *p++ = (len < 256) ? len : '\xff';
    for (arg = argv; *arg; arg++)
    {
        BOOL has_space,has_quote;
        char* a;

        /* Check for quotes and spaces in this argument */
        has_space=has_quote=FALSE;
        a=*arg;
        if( !*a ) has_space=TRUE;
        while (*a!='\0') {
            if (*a==' ' || *a=='\t') {
                has_space=TRUE;
                if (has_quote)
                    break;
            } else if (*a=='"') {
                has_quote=TRUE;
                if (has_space)
                    break;
            }
            a++;
        }

        /* Now transfer it to the command line */
        if (has_space)
            *p++='"';
        if (has_quote) {
            int bcount;

            bcount=0;
            a=*arg;
            while (*a!='\0') {
                if (*a=='\\') {
                    *p++=*a;
                    bcount++;
                } else {
                    if (*a=='"') {
                        int i;

                        /* Double all the '\\' preceding this '"', plus one */
                        for (i=0;i<=bcount;i++)
                            *p++='\\';
                        *p++='"';
                    } else {
                        *p++=*a;
                    }
                    bcount=0;
                }
                a++;
            }
        } else {
            strcpy(p,*arg);
            p+=strlen(*arg);
        }
        if (has_space)
            *p++='"';
        *p++=' ';
    }
    if (len) p--;  /* remove last space */
    *p = '\0';
    return cmd_line;
}


/***********************************************************************
 *           usage
 */
static void usage(void)
{
    WINE_MESSAGE( "Usage: winevdm.exe [--app-name app.exe] command line\n\n" );
    ExitProcess(1);
}


/***********************************************************************
 *           main
 */
int main( int argc, char *argv[] )
{
    DWORD count;
    HINSTANCE16 instance;
    LOADPARAMS16 params;
    WORD showCmd[2];
    char buffer[MAX_PATH];
    STARTUPINFOA info;
    char *cmdline, *appname, **first_arg;
    char *p;

    if (!argv[1]) usage();

    if (!strcmp( argv[1], "--app-name" ))
    {
        if (!(appname = argv[2])) usage();
        first_arg = argv + 3;
    }
    else
    {
        if (!SearchPathA( NULL, argv[1], ".exe", sizeof(buffer), buffer, NULL ))
        {
            WINE_MESSAGE( "winevdm: unable to exec '%s': file not found\n", argv[1] );
            ExitProcess(1);
        }
        appname = buffer;
        first_arg = argv + 1;
    }

    if (*first_arg) first_arg++;  /* skip program name */
    cmdline = build_command_line( first_arg );

    if (WINE_TRACE_ON(winevdm))
    {
        int i;
        WINE_TRACE( "GetCommandLine = '%s'\n", GetCommandLineA() );
        WINE_TRACE( "appname = '%s'\n", appname );
        WINE_TRACE( "cmdline = '%.*s'\n", cmdline[0], cmdline+1 );
        for (i = 0; argv[i]; i++) WINE_TRACE( "argv[%d]: '%s'\n", i, argv[i] );
    }

    GetStartupInfoA( &info );
    showCmd[0] = 2;
    showCmd[1] = (info.dwFlags & STARTF_USESHOWWINDOW) ? info.wShowWindow : SW_SHOWNORMAL;

    params.hEnvironment = 0;
    params.cmdLine = MapLS( cmdline );
    params.showCmd = MapLS( showCmd );
    params.reserved = 0;

    RestoreThunkLock(1);  /* grab the Win16 lock */

    /* some programs assume mmsystem is always present */
    LoadLibrary16( "gdi.exe" );
    LoadLibrary16( "user.exe" );
    LoadLibrary16( "mmsystem.dll" );

    if ((instance = LoadModule16( appname, &params )) < 32)
    {
        if (instance == 11)
        {
            /* first see if it is a .pif file */
            if( ( p = strrchr( appname, '.' )) && !strcasecmp( p, ".pif"))
                pif_cmd( appname, cmdline + 1);
            else
                /* try DOS format */
                start_dosbox( appname, cmdline + 1 );
        }

        WINE_MESSAGE( "winevdm: can't exec '%s': ", appname );
        switch (instance)
        {
        case  2: WINE_MESSAGE("file not found\n" ); break;
        case 11: WINE_MESSAGE("invalid program file\n" ); break;
        default: WINE_MESSAGE("error=%d\n", instance ); break;
        }
        ExitProcess(instance);
    }

    /* wait forever; the process will be killed when the last task exits */
    ReleaseThunkLock( &count );
    Sleep( INFINITE );
    return 0;
}
