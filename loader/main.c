/*
static char RCSId[] = "$Id: wine.c,v 1.2 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "windows.h"
#include "module.h"
#include "selectors.h"
#include "bitmap.h"
#include "comm.h"
#include "win.h"
#include "menu.h"
#include "atom.h"
#include "dialog.h"
#include "drive.h"
#include "queue.h"
#include "syscolor.h"
#include "sysmetrics.h"
#include "file.h"
#include "gdi.h"
#include "heap.h"
#include "keyboard.h"
#include "miscemu.h"
#include "neexe.h"
#include "options.h"
#include "spy.h"
#include "task.h"
#include "tweak.h"
#include "user.h"
#include "dce.h"
#include "shell.h"
#include "winproc.h"
#include "stddebug.h"
#include "debug.h"

#ifndef WINELIB
#include "debugger.h"
#endif

int __winelib = 1;  /* Winelib run-time flag */

HANDLE32 SystemHeap = 0;
HANDLE32 SegptrHeap = 0;

/***********************************************************************
 *           Main initialisation routine
 */
int MAIN_Init(void)
{
    extern BOOL32 RELAY_Init(void);
    extern BOOL32 WIN16DRV_Init(void);
    extern BOOL32 VIRTUAL_Init(void);
    extern BOOL32 WIDGETS_Init(void);

    int queueSize;

    /* Initialize virtual memory management */
    if (!VIRTUAL_Init()) return 0;

    /* Create the system and SEGPTR heaps */
    if (!(SystemHeap = HeapCreate( HEAP_GROWABLE, 0x10000, 0 ))) return 0;
    if (!(SegptrHeap = HeapCreate( HEAP_WINE_SEGPTR, 0, 0 ))) return 0;

    /* Load the configuration file */
    if (!PROFILE_LoadWineIni()) return 0;

      /* Initialize DOS memory */
    if (!DOSMEM_Init()) return 0;

      /* Initialize signal handling */
    if (!SIGNAL_Init()) return 0;

#ifdef WINELIB
    /* Create USER and GDI heap */
    USER_HeapSel = GlobalAlloc16( GMEM_FIXED, 0x10000 );
    LocalInit( USER_HeapSel, 0, 0xffff );
    GDI_HeapSel  = GlobalAlloc16( GMEM_FIXED, GDI_HEAP_SIZE );
    LocalInit( GDI_HeapSel, 0, GDI_HEAP_SIZE-1 );
#else
    /* Initialize relay code */
    if (!RELAY_Init()) return 0;

    /* Create built-in modules */
    if (!BUILTIN_Init()) return 0;

      /* Initialize signal handling */
    if (!SIGNAL_InitEmulator()) return 0;

    /* Create the Win16 printer driver */
    if (!WIN16DRV_Init()) return 0;
#endif  /* WINELIB */

    /* Initialize Wine tweaks */
    if (!TWEAK_Init()) return 0;

    /* Initialize OEM Bitmaps */
    if (!OBM_Init()) return 0;

    /* Initialise DOS drives */
    if (!DRIVE_Init()) return 0;

    /* Initialise DOS directories */
    if (!DIR_Init()) return 0;

    /* Initialize tasks */
    if (!TASK_Init()) return 0;

    /* Initialize communications */
    COMM_Init();

    /* Initialize IO-port permissions */
    IO_port_init();

    /* registry initialisation */
    SHELL_LoadRegistry();
    
    /* Global atom table initialisation */
    if (!ATOM_Init()) return 0;

    /* GDI initialisation */
    if (!GDI_Init()) return 0;

    /* Initialize system colors and metrics*/
    SYSMETRICS_Init();
    SYSCOLOR_Init();

    /* Create the DCEs */
    DCE_Init();

    /* Initialize keyboard */
    if (!KEYBOARD_Init()) return 0;

    /* Initialize window procedures */
    if (!WINPROC_Init()) return 0;

    /* Initialize built-in window classes */
    if (!WIDGETS_Init()) return 0;

    /* Initialize dialog manager */
    if (!DIALOG_Init()) return 0;

    /* Initialize menus */
    if (!MENU_Init()) return 0;

    /* Create desktop window */
    if (!WIN_CreateDesktopWindow()) return 0;

    /* Initialize message spying */
    if (!SPY_Init()) return 0;

    /* Check wine.conf for old/bad entries */
    if (!TWEAK_CheckConfiguration()) return 0;

    /* Create system message queue */
    queueSize = GetProfileInt32A( "windows", "TypeAhead", 120 );
    if (!QUEUE_CreateSysMsgQueue( queueSize )) return 0;

    /* Set double click time */
    SetDoubleClickTime32( GetProfileInt32A("windows","DoubleClickSpeed",452) );

    return 1;
}


#ifndef WINELIB
/**********************************************************************
 *					main
 */
int main(int argc, char *argv[] )
{
    extern BOOL32 MAIN_WineInit( int *argc, char *argv[] );
    extern char * DEBUG_argv0;

    int i,loaded;
    HINSTANCE16 handle;

    __winelib = 0;  /* First of all, clear the Winelib flag */

    /*
     * Save this so that the internal debugger can get a hold of it if
     * it needs to.
     */
    DEBUG_argv0 = argv[0];

    if (!MAIN_WineInit( &argc, argv )) return 1;
    if (!MAIN_Init()) return 1;

    loaded=0;
    for (i = 1; i < argc; i++)
    {
        if ((handle = WinExec32( argv[i], SW_SHOWNORMAL )) < 32)
        {
            fprintf(stderr, "wine: can't exec '%s': ", argv[i]);
            switch (handle)
            {
            case 2: fprintf( stderr, "file not found\n" ); break;
            case 11: fprintf( stderr, "invalid exe file\n" ); break;
            case 21: fprintf( stderr, "win32 executable\n" ); break;
            default: fprintf( stderr, "error=%d\n", handle ); break;
            }
            return 1;
        }
	loaded++;
    }

    if (!loaded) { /* nothing loaded */
    	extern void MAIN_Usage(char*);
    	MAIN_Usage(argv[0]);
	return 1;
    }

    if (!GetNumTasks())
    {
        fprintf( stderr, "wine: no executable file found.\n" );
        return 0;
    }

    if (Options.debug) DEBUG_SetBreakpoints( TRUE );  /* Setup breakpoints */

    Yield();  /* Start the first task */
    fprintf( stderr, "WinMain: Should never happen: returned from Yield()\n" );
    return 0;
}

#endif /* #ifndef WINELIB */
