/*
 * Emulator initialisation code
 *
 */

#include "winbase.h"
#include "wine/winbase16.h"
#include "wingdi.h"
#include "winuser.h"

#include "builtin32.h"
#include "callback.h"
#include "options.h"
#include "process.h"
#include "debugtools.h"

/***********************************************************************
 *           Main loop of initial task
 */
static void initial_task(void)
{
    MSG msg;
    HINSTANCE16 instance;
    STARTUPINFOA info;

    GetStartupInfoA( &info );
    if (!(info.dwFlags & STARTF_USESHOWWINDOW)) info.wShowWindow = SW_SHOWNORMAL;

    if ((instance = WinExec16( GetCommandLineA(), info.wShowWindow )) < 32)
    {
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
    BUILTIN32_DESCRIPTOR descriptor;

    memset( &descriptor, 0, sizeof(descriptor) );
    descriptor.filename = argv[0];
    descriptor.dllentrypoint = initial_task;
    BUILTIN32_RegisterDLL( &descriptor );

    PROCESS_InitWine( argc, argv );
    return 1;  /* not reached */
}
