/*
 * Main initialization code
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "windows.h"
#include "bitmap.h"
#include "comm.h"
#include "win.h"
#include "main.h"
#include "menu.h"
#include "message.h"
#include "multimedia.h"
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
#include "options.h"
#include "process.h"
#include "spy.h"
#include "tweak.h"
#include "user.h"
#include "dce.h"
#include "shell.h"
#include "winproc.h"
#include "debug.h"


int __winelib = 1;  /* Winelib run-time flag */

/***********************************************************************
 *           Kernel initialisation routine
 */
BOOL32 MAIN_KernelInit(void)
{
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

    /* Initialize multimedia */
    if (!MULTIMEDIA_Init()) return FALSE;

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
BOOL32 MAIN_WinelibInit( int *argc, char *argv[] )
{
    /* Create the initial process */
    if (!PROCESS_Init()) return FALSE;

    /* Parse command line arguments */
    MAIN_WineInit( argc, argv );

    /* Initialize the kernel */
    if (!MAIN_KernelInit()) return FALSE;

    /* Initialize all the USER stuff */
    if (!MAIN_UserInit()) return FALSE;

    return TRUE;
}
