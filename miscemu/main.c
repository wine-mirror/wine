/*
 * Emulator initialisation code
 *
 */

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
#include "windows.h"


/***********************************************************************
 *           Emulator initialisation
 */
BOOL32 MAIN_EmulatorInit(void)
{
    /* Initialize the kernel */
    if (!MAIN_KernelInit()) return FALSE;

    /* Initialize relay code */
    if (!RELAY_Init()) return FALSE;

      /* Initialize signal handling */
    if (!SIGNAL_InitEmulator()) return FALSE;

    /* Create the Win16 printer driver */
    if (!WIN16DRV_Init()) return FALSE;

    /* Create the Postscript printer driver (FIXME: should be in Winelib) */
    if (!PSDRV_Init()) return FALSE;

    /* Initialize all the USER stuff */
    return MAIN_UserInit();
}


/**********************************************************************
 *           main
 */
int main( int argc, char *argv[] )
{
    extern char * DEBUG_argv0;

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
    IF1632_CallLargeStack = (int (*)(int (*func)(), void *arg))CALL32_Init();

    loaded=0;
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

    Yield16();  /* Start the first task */
    MSG("WinMain: Should never happen: returned from Yield16()\n" );
    return 0;
}
