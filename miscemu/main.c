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
#include "windows.h"


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

    /* Load system DLLs into the initial process (and initialize them) */
    if (!LoadLibrary16(  "KERNEL" )) return FALSE;         /* always built-in */
    if (!LoadLibrary32A( "KERNEL32" )) return FALSE;       /* always built-in */

    if (!LoadLibrary16(  "GDI.EXE" )) return FALSE;
    if (!LoadLibrary32A( "GDI32.DLL" )) return FALSE;

    if (!LoadLibrary16(  "USER.EXE" )) return FALSE;
    if (!LoadLibrary32A( "USER32.DLL" )) return FALSE;
    
    return TRUE;
}


/***********************************************************************
 *           Main loop of initial task
 */
void MAIN_EmulatorRun( void )
{
    BOOL32 (*WINAPI pGetMessage)(MSG32* lpmsg,HWND32 hwnd,UINT32 min,UINT32 max);
    BOOL32 (*WINAPI pTranslateMessage)( const MSG32* msg );
    LONG   (*WINAPI pDispatchMessage)( const MSG32* msg );
    MSG32 msg;

    HMODULE32 hModule = GetModuleHandle32A( "USER32" );
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
    extern char * DEBUG_argv0;
    char startProg[256], defProg[256];
    int i,loaded;
    HINSTANCE32 handle;

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

    /* Initialize CALL32 routines */
    /* This needs to be done just before task-switching starts */

    loaded=0;
    
    /* Add the Default Program if no program on the command line */
    if (!argv[1])
    {
        PROFILE_GetWineIniString( "programs", "Default", "",
                                  defProg, sizeof(defProg) );
        if (defProg[0]) argv[argc++] = defProg;
    }
    
    /* Add the Startup Program to the run list */
    PROFILE_GetWineIniString( "programs", "Startup", "", 
			       startProg, sizeof(startProg) );
    if (startProg[0]) argv[argc++] = startProg;

    for (i = 1; i < argc; i++)
    {
        if ((handle = WinExec32( argv[i], SW_SHOWNORMAL )) < 32)
        {
            MSG("wine: can't exec '%s': ", argv[i]);
            switch (handle)
            {
            case 2: MSG("file not found\n" ); break;
            case 11: MSG("invalid exe file\n" ); break;
            case 21: MSG("win32 executable\n" ); break; /* FIXME: Obsolete? */
            default: MSG("error=%d\n", handle ); break;
            }
            return 1;
        }
	loaded++;
    }

    if (!loaded) { /* nothing loaded */
    	MAIN_Usage(argv[0]);
	return 1;
    }

    if (!GetNumTasks())
    {
        MSG("wine: no executable file found.\n" );
        return 0;
    }

    if (Options.debug) DEBUG_AddModuleBreakpoints();

    ctx_debug_call = ctx_debug;
#if 0  /* FIXME!! */
    IF1632_CallLargeStack = (int (*)(int (*func)(), void *arg))CALL32_Init();
#endif
    MAIN_EmulatorRun();
    MSG("WinMain: Should never happen: returned from Yield16()\n" );
    return 0;
}
