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
#include "comm.h"
#include "win.h"
#include "menu.h"
#include "atom.h"
#include "dialog.h"
#include "directory.h"
#include "drive.h"
#include "queue.h"
#include "syscolor.h"
#include "sysmetrics.h"
#include "gdi.h"
#include "heap.h"
#include "debugger.h"
#include "miscemu.h"
#include "neexe.h"
#include "options.h"
#include "spy.h"
#include "task.h"
#include "user.h"
#include "dce.h"
#include "pe_image.h"
#include "shell.h"
#include "winproc.h"
#include "stddebug.h"
#include "debug.h"


/* Winelib run-time flag */
#ifdef WINELIB
int __winelib = 1;
#else
int __winelib = 0;
#endif

HANDLE32 SystemHeap = 0;
HANDLE32 SegptrHeap = 0;

/***********************************************************************
 *           Main initialisation routine
 */
int MAIN_Init(void)
{
    extern BOOL32 RELAY_Init(void);
    extern BOOL32 SIGNAL_Init(void);
    extern BOOL32 WIDGETS_Init(void);
    extern int KERN32_Init(void);

    int queueSize;

    /* Create the system and SEGPTR heaps */
    if (!(SystemHeap = HeapCreate( HEAP_GROWABLE, 0x10000, 0 ))) return 0;
    if (!(SegptrHeap = HeapCreate( HEAP_WINE_SEGPTR, 0, 0 ))) return 0;

    /* Load the configuration file */
    if (!PROFILE_LoadWineIni()) return 0;

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

    /* Initialize interrupt vectors */
    if (!INT_Init()) return 0;

      /* Initialize DOS memory */
    if (!DOSMEM_Init()) return 0;

      /* Initialize the DOS interrupt */
    if (!INT21_Init()) return 0;

      /* Initialize signal handling */
    if (!SIGNAL_Init()) return 0;
#endif  /* WINELIB */

    /* Initialise DOS drives */
    if (!DRIVE_Init()) return 0;

    /* Initialise DOS directories */
    if (!DIR_Init()) return 0;

      /* Initialize tasks */
    if (!TASK_Init()) return 0;

      /* Initialize communications */
    COMM_Init();

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

      /* Initialize Win32 data structures */
    if (!KERN32_Init()) return 0;

      /* Create system message queue */
    queueSize = GetProfileInt( "windows", "TypeAhead", 120 );
    if (!QUEUE_CreateSysMsgQueue( queueSize )) return 0;

    /* Set double click time */
    SetDoubleClickTime( GetProfileInt( "windows", "DoubleClickSpeed", 452 ) );

    return 1;
}


#ifndef WINELIB
/**********************************************************************
 *					main
 */
int _WinMain(int argc, char **argv)
{
    int i;
    HANDLE handle;

    if (!MAIN_Init()) return 0;

    for (i = 1; i < argc; i++)
    {
        if ((handle = WinExec( argv[i], SW_SHOWNORMAL )) < 32)
        {
            fprintf(stderr, "wine: can't exec '%s': ", argv[i]);
            switch (handle)
            {
            case 2: fprintf( stderr, "file not found\n" ); break;
            case 11: fprintf( stderr, "invalid exe file\n" ); break;
            case 21: fprintf( stderr, "win32 executable\n" ); break;
            default: fprintf( stderr, "error=%d\n", handle ); break;
            }
            exit(1);
        }
    }

    if (Options.debug) DEBUG_SetBreakpoints( TRUE );  /* Setup breakpoints */

    Yield();  /* Start the first task */
    fprintf( stderr, "WinMain: Should never happen: returned from Yield()\n" );
    return 0;
}

#endif /* #ifndef WINELIB */
