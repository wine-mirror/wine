/*
 * Emulator initialisation code
 *
 * Copyright 2000 Alexandre Julliard
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
#include "wingdi.h"
#include "winuser.h"

#include "miscemu.h"
#include "callback.h"
#include "options.h"
#include "wine/debug.h"

static char main_exe_name[MAX_PATH];
static HANDLE main_exe_file;

static BOOL (WINAPI *pGetMessageA)(LPMSG,HWND,UINT,UINT);
static BOOL (WINAPI *pTranslateMessage)(const MSG*);
static LONG (WINAPI *pDispatchMessageA)(const MSG*);

extern void DECLSPEC_NORETURN PROCESS_InitWine(
	int argc, char *argv[], LPSTR win16_exe_name,
	HANDLE *win16_exe_file );
extern HINSTANCE16 NE_StartMain( LPCSTR name, HANDLE file );

/***********************************************************************
 *           Main loop of initial task
 */
int WINAPI wine_initial_task( HINSTANCE inst, HINSTANCE prev, LPSTR cmdline, INT show )
{
    MSG msg;
    HINSTANCE16 instance;
    HMODULE user32;

    /* some programs assume mmsystem is always present */
    LoadLibrary16( "mmsystem.dll" );

    if ((instance = NE_StartMain( main_exe_name, main_exe_file )) < 32)
    {
        if (instance == 11)  /* try DOS format */
        {
            if (DPMI_LoadDosSystem())
                Dosvm.LoadDosExe( main_exe_name, main_exe_file );
            /* if we get back here it failed */
            instance = GetLastError();
        }

        MESSAGE( "%s: can't exec '%s': ", argv0, GetCommandLineA() );
        switch (instance)
        {
        case  2: MESSAGE("file not found\n" ); break;
        case 11: MESSAGE("invalid exe file\n" ); break;
        default: MESSAGE("error=%d\n", instance ); break;
        }
        ExitProcess(instance);
    }
    CloseHandle( main_exe_file );  /* avoid file sharing problems */

    /* Start message loop for desktop window */

    if (!(user32 = LoadLibraryA( "user32.dll" )))
    {
        MESSAGE( "Cannot load user32.dll\n" );
        ExitProcess( GetLastError() );
    }
    pGetMessageA      = (void *)GetProcAddress( user32, "GetMessageA" );
    pTranslateMessage = (void *)GetProcAddress( user32, "TranslateMessage" );
    pDispatchMessageA = (void *)GetProcAddress( user32, "DispatchMessageA" );

    while ( GetNumTasks16() > 1  && pGetMessageA( &msg, 0, 0, 0 ) )
    {
        pTranslateMessage( &msg );
        pDispatchMessageA( &msg );
    }

    ExitProcess( 0 );
}


/**********************************************************************
 *           main
 */
int main( int argc, char *argv[] )
{
    PROCESS_InitWine( argc, argv, main_exe_name, &main_exe_file );
    return 1;  /* not reached */
}
