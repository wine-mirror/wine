/*
 * Emulator initialisation code
 *
 */

#include <stdlib.h>
#include <assert.h>
#include "wine/winbase16.h"
#include "callback.h"
#include "main.h"
#include "miscemu.h"
#include "module.h"
#include "options.h"
#include "process.h"
#include "thread.h"
#include "task.h"
#include "stackframe.h"
#include "wine/exception.h"
#include "debugtools.h"

extern DWORD DEBUG_WinExec(LPCSTR lpCmdLine, int sw);


static BOOL exec_program( LPCSTR cmdline )
{
    HINSTANCE handle;

    if (Options.debug) 
        handle = DEBUG_WinExec( cmdline, SW_SHOWNORMAL );
    else
        handle = WinExec( cmdline, SW_SHOWNORMAL );
       
    if (handle < 32) 
    {
        MESSAGE( "%s: can't exec '%s': ", argv0, cmdline );
        switch (handle) 
        {
        case  2: MESSAGE("file not found\n" ); break;
        case 11: MESSAGE("invalid exe file\n" ); break;
        default: MESSAGE("error=%d\n", handle ); break;
        }
    }
    return (handle >= 32);
}

/***********************************************************************
 *           Main loop of initial task
 */
void MAIN_EmulatorRun( void )
{
    char startProg[256], defProg[256];
    int i, tasks = 0;
    MSG msg;

    /* Load system DLLs into the initial process (and initialize them) */
    if (   !LoadLibrary16("GDI.EXE" ) || !LoadLibraryA("GDI32.DLL" )
        || !LoadLibrary16("USER.EXE") || !LoadLibraryA("USER32.DLL"))
        ExitProcess( 1 );

    /* Get pointers to USER routines called by KERNEL */
    THUNK_InitCallout();

    /* Call FinalUserInit routine */
    Callout.FinalUserInit16();

    /* Call InitApp for initial task */
    Callout.InitApp16( MapHModuleLS( 0 ) );

    /* Add the Startup Program to the run list */
    PROFILE_GetWineIniString( "programs", "Startup", "", 
			       startProg, sizeof(startProg) );
    if (startProg[0]) tasks += exec_program( startProg );

    /* Add the Default Program if no program on the command line */
    if (!Options.argv[1])
    {
        PROFILE_GetWineIniString( "programs", "Default", "",
                                  defProg, sizeof(defProg) );
        if (defProg[0]) tasks += exec_program( defProg );
        else if (!tasks && !startProg[0]) OPTIONS_Usage();
    }
    else
    {
        /* Load and run executables given on command line */
        for (i = 1; Options.argv[i]; i++)
        {
            tasks += exec_program( Options.argv[i] );
        }
    }
    if (!tasks) ExitProcess( 0 );

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
    NE_MODULE *pModule;

    /* Initialize everything */
    if (!MAIN_MainInit( &argc, argv, FALSE )) return 1;

    /* Create initial task */
    if ( !(pModule = NE_GetPtr( GetModuleHandle16( "KERNEL" ) )) ) return 1;
    if ( !TASK_Create( pModule, FALSE ) ) return 1;

    /* Switch to initial task */
    PostEvent16( PROCESS_Current()->task );
    TASK_Reschedule();

    /* Switch stacks and jump to MAIN_EmulatorRun */
    CALL32_Init( &IF1632_CallLargeStack, MAIN_EmulatorRun, NtCurrentTeb()->stack_top );

    MESSAGE( "main: Should never happen: returned from CALL32_Init()\n" );
    return 0;
}
