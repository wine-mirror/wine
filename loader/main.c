/*
 * Main initialization code
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


int __winelib = 1;  /* Winelib run-time flag */

/***********************************************************************
 *           Kernel initialisation routine
 */
BOOL32 MAIN_KernelInit(void)
{
    extern BOOL32 EVENT_Init(void);
    extern BOOL32 PROCESS_Init(void);
    extern BOOL32 VIRTUAL_Init(void);

    /* Initialize virtual memory management */
    if (!VIRTUAL_Init()) return FALSE;

    /* Create the system and SEGPTR heaps */
    if (!(SystemHeap = HeapCreate( HEAP_GROWABLE, 0x10000, 0 ))) return FALSE;
    if (!(SegptrHeap = HeapCreate( HEAP_WINE_SEGPTR, 0, 0 ))) return FALSE;

    /* Create the initial process */
    if (!PROCESS_Init()) return FALSE;

    /* Initialize signal handling */
    if (!SIGNAL_Init()) return FALSE;

    /* Load the configuration file */
    if (!PROFILE_LoadWineIni()) return FALSE;

      /* Initialize DOS memory */
    if (!DOSMEM_Init()) return FALSE;

    /* Initialise DOS drives */
    if (!DRIVE_Init()) return FALSE;

    /* Initialise DOS directories */
    if (!DIR_Init()) return FALSE;

    /* Initialize tasks */
    if (!TASK_Init()) return FALSE;

      /* Initialize event handling */
    if (!EVENT_Init()) return FALSE;

    /* Initialize communications */
    COMM_Init();

    /* Initialize IO-port permissions */
    IO_port_init();

    return TRUE;
}


/***********************************************************************
 *           USER (and GDI) initialisation routine
 */
BOOL32 MAIN_UserInit(void)
{
    extern BOOL32 WIDGETS_Init(void);

    int queueSize;

    /* Create USER and GDI heap */
    if (!USER_HeapSel)
    {
        USER_HeapSel = GlobalAlloc16( GMEM_FIXED, 0x10000 );
        LocalInit( USER_HeapSel, 0, 0xffff );
    }
    if (!GDI_HeapSel)
    {
        GDI_HeapSel  = GlobalAlloc16( GMEM_FIXED, GDI_HEAP_SIZE );
        LocalInit( GDI_HeapSel, 0, GDI_HEAP_SIZE-1 );
    }

    /* Initialize Wine tweaks */
    if (!TWEAK_Init()) return FALSE;

    /* Initialize OEM Bitmaps */
    if (!OBM_Init()) return FALSE;

    /* registry initialisation */
    SHELL_LoadRegistry();
    
    /* Global atom table initialisation */
    if (!ATOM_Init()) return FALSE;

    /* GDI initialisation */
    if (!GDI_Init()) return FALSE;

    /* Initialize system colors and metrics*/
    SYSMETRICS_Init();
    SYSCOLOR_Init();

    /* Create the DCEs */
    DCE_Init();

    /* Initialize keyboard */
    if (!KEYBOARD_Init()) return FALSE;

    /* Initialize window procedures */
    if (!WINPROC_Init()) return FALSE;

    /* Initialize built-in window classes */
    if (!WIDGETS_Init()) return FALSE;

    /* Initialize dialog manager */
    if (!DIALOG_Init()) return FALSE;

    /* Initialize menus */
    if (!MENU_Init()) return FALSE;

    /* Create desktop window */
    if (!WIN_CreateDesktopWindow()) return FALSE;

    /* Initialize message spying */
    if (!SPY_Init()) return FALSE;

    /* Check wine.conf for old/bad entries */
    if (!TWEAK_CheckConfiguration()) return FALSE;

    /* Create system message queue */
    queueSize = GetProfileInt32A( "windows", "TypeAhead", 120 );
    if (!QUEUE_CreateSysMsgQueue( queueSize )) return FALSE;

    /* Set double click time */
    SetDoubleClickTime32( GetProfileInt32A("windows","DoubleClickSpeed",452) );

    return TRUE;
}


/***********************************************************************
 *           Winelib initialisation routine
 */
int MAIN_WinelibInit(void)
{
    /* Initialize the kernel */
    if (!MAIN_KernelInit()) return 0;

    /* Initialize all the USER stuff */
    if (!MAIN_UserInit()) return 0;
    return 1;
}
