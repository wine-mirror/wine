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
void wine_initial_task(void)
{
    MSG msg;
    HINSTANCE16 instance;
    STARTUPINFOA info;

    if (!LoadLibraryA( "user32.dll" ))
    {
        MESSAGE( "Cannot load user32.dll\n" );
        ExitProcess( GetLastError() );
    }
    THUNK_InitCallout();

    GetStartupInfoA( &info );
    if (!(info.dwFlags & STARTF_USESHOWWINDOW)) info.wShowWindow = SW_SHOWNORMAL;

    if ((instance = WinExec16( GetCommandLineA(), info.wShowWindow )) < 32)
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
