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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "winbase.h"
#include "wine/winbase16.h"
#include "winuser.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winevdm);

extern void WINAPI wine_load_dos_exe( LPCSTR filename, LPCSTR cmdline );


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
        int has_space,bcount;
        char* a;

        has_space=0;
        bcount=0;
        a=*arg;
        while (*a!='\0') {
            if (*a=='\\') {
                bcount++;
            } else {
                if (*a==' ' || *a=='\t') {
                    has_space=1;
                } else if (*a=='"') {
                    /* doubling of '\' preceeding a '"',
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

    if (!(cmd_line = HeapAlloc( GetProcessHeap(), 0, len + 1 ))) return NULL;

    p = cmd_line;
    *p++ = len;
    for (arg = argv; *arg; arg++)
    {
        int has_space,has_quote;
        char* a;

        /* Check for quotes and spaces in this argument */
        has_space=has_quote=0;
        a=*arg;
        while (*a!='\0') {
            if (*a==' ' || *a=='\t') {
                has_space=1;
                if (has_quote)
                    break;
            } else if (*a=='"') {
                has_quote=1;
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
            char* a;

            bcount=0;
            a=*arg;
            while (*a!='\0') {
                if (*a=='\\') {
                    *p++=*a;
                    bcount++;
                } else {
                    if (*a=='"') {
                        int i;

                        /* Double all the '\\' preceeding this '"', plus one */
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
    if (p > cmd_line) p--;  /* remove last space */
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
            WINE_MESSAGE( "winevdm: can't exec '%s': file not found\n", argv[1] );
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
    LoadLibrary16( "mmsystem.dll" );

    if ((instance = LoadModule16( appname, &params )) < 32)
    {
        if (instance == 11)  /* try DOS format */
        {
            wine_load_dos_exe( appname, cmdline );
            /* if we get back here it failed */
            instance = GetLastError();
        }

        WINE_MESSAGE( "winevdm: can't exec '%s': ", appname );
        switch (instance)
        {
        case  2: WINE_MESSAGE("file not found\n" ); break;
        case 11: WINE_MESSAGE("invalid exe file\n" ); break;
        default: WINE_MESSAGE("error=%d\n", instance ); break;
        }
        ExitProcess(instance);
    }

    /* wait forever; the process will be killed when the last task exits */
    ReleaseThunkLock( &count );
    Sleep( INFINITE );
    return 0;
}
