/*
 * Emulator initialisation code
 *
 */

#include <assert.h>
#include "callback.h"
#include "debug.h"
#include "debugger.h"
#include "main.h"
#include "miscemu.h"
#include "module.h"
#include "options.h"
#include "process.h"
#include "win16drv.h"
#include "psdrv.h"
#include "thread.h"
#include "task.h"
#include "stackframe.h"
#include "windows.h"

static int MAIN_argc;
static char **MAIN_argv;

/***********************************************************************
 *           Emulator initialisation
 */
BOOL32 MAIN_EmulatorInit(void)
{
    /* Main initialization */
    if (!MAIN_MainInit()) return FALSE;

    /* Initialize relay code */
    if (!RELAY_Init()) return FALSE;

    /* Initialize signal handling */
    if (!SIGNAL_InitEmulator()) return FALSE;

    /* Create the Win16 printer driver */
    if (!WIN16DRV_Init()) return FALSE;

    /* Create the Postscript printer driver (FIXME: should be in Winelib) */
    if (!PSDRV_Init()) return FALSE;

    return TRUE;
}

/***********************************************************************
 *           Main loop of initial task
 */
void MAIN_EmulatorRun( void )
{
    char startProg[256], defProg[256];
    int i,loaded;
    HINSTANCE32 handle;

    BOOL32 (*WINAPI pGetMessage)(MSG32* lpmsg,HWND32 hwnd,UINT32 min,UINT32 max);
    BOOL32 (*WINAPI pTranslateMessage)( const MSG32* msg );
    LONG   (*WINAPI pDispatchMessage)( const MSG32* msg );
    HMODULE32 hModule;
    MSG32 msg;

    /* Load system DLLs into the initial process (and initialize them) */
    if (   !LoadLibrary16("GDI.EXE" ) || !LoadLibrary32A("GDI32.DLL" )
        || !LoadLibrary16("USER.EXE") || !LoadLibrary32A("USER32.DLL"))
        ExitProcess( 1 );

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

    loaded=0;
    for (i = 1; i < MAIN_argc; i++)
    {
        if ((handle = WinExec32( MAIN_argv[i], SW_SHOWNORMAL )) < 32)
        {
            MSG("wine: can't exec '%s': ", MAIN_argv[i]);
            switch (handle)
            {
            case 2: MSG("file not found\n" ); break;
            case 11: MSG("invalid exe file\n" ); break;
            case 21: MSG("win32 executable\n" ); break; /* FIXME: Obsolete? */
            default: MSG("error=%d\n", handle ); break;
            }
            ExitProcess( 1 );
        }
	loaded++;
    }

    if (!loaded) { /* nothing loaded */
    	MAIN_Usage(MAIN_argv[0]);
        ExitProcess( 1 );
    }

    if (GetNumTasks() <= 1)
    {
        MSG("wine: no executable file found.\n" );
        ExitProcess( 0 );
    }

    if (Options.debug) DEBUG_AddModuleBreakpoints();


    /* Start message loop for desktop window */

    hModule = GetModuleHandle32A( "USER32" );
    pGetMessage       = GetProcAddress32( hModule, "GetMessageA" ); 
    pTranslateMessage = GetProcAddress32( hModule, "TranslateMessage" ); 
    pDispatchMessage  = GetProcAddress32( hModule, "DispatchMessageA" ); 

    assert( pGetMessage );
    assert( pTranslateMessage );
    assert( pDispatchMessage );

    while ( GetNumTasks() > 1 && pGetMessage( &msg, 0, 0, 0 ) )
    {
        pTranslateMessage( &msg );
        pDispatchMessage( &msg );
    }

    ExitProcess( 0 );
}


/**********************************************************************
 *           main
 */
int main( int argc, char *argv[] )
{
    NE_MODULE *pModule;
    HINSTANCE16 hInstance;
    extern char * DEBUG_argv0;

    __winelib = 0;  /* First of all, clear the Winelib flag */
    ctx_debug_call = ctx_debug;

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

    /* Handle -dll option (hack) */
    if (Options.dllFlags)
    {
        if (!BUILTIN_ParseDLLOptions( Options.dllFlags ))
        {
            MSG("%s: Syntax: -dll +xxx,... or -dll -xxx,...\n",
                     argv[0] );
            BUILTIN_PrintDLLs();
            exit(1);
        }
    }

    /* Initialize everything */
    if (!MAIN_EmulatorInit()) return 1;

    /* Load kernel modules */
    if (!LoadLibrary16(  "KERNEL" )) return 1;
    if (!LoadLibrary32A( "KERNEL32" )) return 1;

    /* Create initial task */
    if ( !(pModule = NE_GetPtr( GetModuleHandle16( "KERNEL32" ) )) ) return 1;
    hInstance = NE_CreateInstance( pModule, NULL, TRUE );
    PROCESS_Current()->task = TASK_Create( THREAD_Current(), pModule, hInstance, 0, FALSE );

    /* Initialize CALL32 routines */
    /* This needs to be done just before switching stacks */
    IF1632_CallLargeStack = (int (*)(int (*func)(), void *arg))CALL32_Init();

    /* Switch to initial task */
    CURRENT_STACK16->frame32->retaddr = (DWORD)MAIN_EmulatorRun;
    TASK_StartTask( PROCESS_Current()->task );
    MSG( "main: Should never happen: returned from TASK_StartTask()\n" );
    return 0;
}

