/*
 * Emulator initialisation code
 *
 */

#include "winbase.h"
#include "wine/winbase16.h"
#include "wingdi.h"
#include "winuser.h"

#include "callback.h"
#include "options.h"
#include "dosexe.h"
#include "debugtools.h"

extern void PROCESS_InitWine( int argc, char *argv[] ) WINE_NORETURN;

/***********************************************************************
 *           Main loop of initial task
 */
int WINAPI wine_initial_task( HINSTANCE inst, HINSTANCE prev, LPSTR cmdline, INT show )
{
    MSG msg;
    HINSTANCE16 instance;

    if (!LoadLibraryA( "user32.dll" ))
    {
        MESSAGE( "Cannot load user32.dll\n" );
        ExitProcess( GetLastError() );
    }
    THUNK_InitCallout();

    if ((instance = WinExec16( GetCommandLineA(), show )) < 32)
    {
        if (instance == 11)  /* try DOS format */
        {
            MZ_LoadImage( GetCommandLineA() );
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

    /* Start message loop for desktop window */

    while ( GetNumTasks16() > 1  && Callout.GetMessageA( &msg, 0, 0, 0 ) )
    {
        Callout.TranslateMessage( &msg );
        Callout.DispatchMessageA( &msg );
    }

    ExitProcess( 0 );
}


/**********************************************************************
 *           main
 */
int main( int argc, char *argv[] )
{
    PROCESS_InitWine( argc, argv );
    return 1;  /* not reached */
}
