/*
 * Emulator initialisation code
 *
 */

#include <stdlib.h>
#include <assert.h>
#include "wine/winbase16.h"
#include "callback.h"
#include "debugger.h"
#include "main.h"
#include "miscemu.h"
#include "win16drv.h"
#include "module.h"
#include "options.h"
#include "process.h"
#include "thread.h"
#include "task.h"
#include "stackframe.h"
#include "wine/exception.h"
#include "debug.h"

static int MAIN_argc;
static char **MAIN_argv;

extern int (*INSTR_IsRelay)( const void *addr );

static int is_relay_addr( const void *addr )
{
    extern char CallFrom16_Start, CallFrom16_End, CALLTO16_Start, CALLTO16_End;

    return ((((char *)addr >= &CallFrom16_Start) &&
             ((char *)addr < &CallFrom16_End)) ||
            (((char *)addr >= &CALLTO16_Start) &&
             ((char *)addr < &CALLTO16_End)));
}

/***********************************************************************
 *           Emulator initialisation
 */
BOOL MAIN_EmulatorInit(void)
{
    /* Main initialization */
    if (!MAIN_MainInit()) return FALSE;

    /* Initialize relay code */
    if (!RELAY_Init()) return FALSE;

    /* Create the Win16 printer driver */
    if (!WIN16DRV_Init()) return FALSE;

    return TRUE;
}


/***********************************************************************
 *           Main loop of initial task
 */
void MAIN_EmulatorRun( void )
{
    extern void THUNK_InitCallout( void );
    char startProg[256], defProg[256];
    HINSTANCE handle;
    int i, tasks = 0;
    MSG msg;

    /* Load system DLLs into the initial process (and initialize them) */
    if (   !LoadLibrary16("GDI.EXE" ) || !LoadLibraryA("GDI32.DLL" )
        || !LoadLibrary16("USER.EXE") || !LoadLibraryA("USER32.DLL"))
        ExitProcess( 1 );

    /* Get pointers to USER routines called by KERNEL */
    THUNK_InitCallout();

    /* Call InitApp for initial task */
    Callout.InitApp16( MapHModuleLS( 0 ) );

    /* Add the Default Program if no program on the command line */
    if (!MAIN_argv[1])
    {
        PROFILE_GetWineIniString( "programs", "Default", "",
                                  defProg, sizeof(defProg) );
        if (defProg[0]) MAIN_argv[MAIN_argc++] = defProg;
    }
    
    /* Add the Startup Program to the run list */
    PROFILE_GetWineIniString( "programs", "Startup", "", 
			       startProg, sizeof(startProg) );
    if (startProg[0]) MAIN_argv[MAIN_argc++] = startProg;

    /* Abort if no executable on command line */
    if (MAIN_argc <= 1) 
    {
    	MAIN_Usage(MAIN_argv[0]);
        ExitProcess( 1 );
    }

    /* Load and run executables given on command line */
    for (i = 1; i < MAIN_argc; i++)
    {
        if ((handle = WinExec( MAIN_argv[i], SW_SHOWNORMAL )) < 32)
        {
            MSG("wine: can't exec '%s': ", MAIN_argv[i]);
            switch (handle)
            {
            case 2: MSG("file not found\n" ); break;
            case 11: MSG("invalid exe file\n" ); break;
            default: MSG("error=%d\n", handle ); break;
            }
        }
        else tasks++;
    }

    if (!tasks)
    {
        MSG("wine: no executable file found.\n" );
        ExitProcess( 0 );
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
    NE_MODULE *pModule;
    extern char * DEBUG_argv0;

    __winelib = 0;  /* First of all, clear the Winelib flag */

    /*
     * Save this so that the internal debugger can get a hold of it if
     * it needs to.
     */
    DEBUG_argv0 = argv[0];

    /* Create the initial process */
    if (!PROCESS_Init()) return FALSE;

    /* Parse command-line */
    if (!MAIN_WineInit( &argc, argv )) return 1;
    MAIN_argc = argc; MAIN_argv = argv;

    /* Set up debugger hook */
    INSTR_IsRelay = is_relay_addr;
    EXC_SetDebugEventHook( wine_debugger );

    if (Options.debug) 
        TASK_AddTaskEntryBreakpoint = DEBUG_AddTaskEntryBreakpoint;

    /* Initialize everything */
    if (!MAIN_EmulatorInit()) return 1;

    /* Load kernel modules */
    if (!LoadLibrary16(  "KERNEL" )) return 1;
    if (!LoadLibraryA( "KERNEL32" )) return 1;

    /* Create initial task */
    if ( !(pModule = NE_GetPtr( GetModuleHandle16( "KERNEL" ) )) ) return 1;
    if ( !TASK_Create( pModule, 0, 0, FALSE ) ) return 1;

    /* Switch to initial task */
    PostEvent16( PROCESS_Current()->task );
    TASK_Reschedule();

    /* Switch stacks and jump to MAIN_EmulatorRun */
    CALL32_Init( &IF1632_CallLargeStack, MAIN_EmulatorRun, NtCurrentTeb()->stack_top );

    MSG( "main: Should never happen: returned from CALL32_Init()\n" );
    return 0;
}
