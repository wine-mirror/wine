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
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "bitmap.h"
#include "comm.h"
#include "neexe.h"
#include "main.h"
#include "menu.h"
#include "message.h"
#include "multimedia.h"
#include "dialog.h"
#include "drive.h"
#include "queue.h"
#include "sysmetrics.h"
#include "file.h"
#include "heap.h"
#include "keyboard.h"
#include "mouse.h"
#include "input.h"
#include "miscemu.h"
#include "options.h"
#include "process.h"
#include "spy.h"
#include "tweak.h"
#include "user.h"
#include "cursoricon.h"
#include "global.h"
#include "dce.h"
#include "shell.h"
#include "winproc.h"
#include "syslevel.h"
#include "thread.h"
#include "task.h"
#include "debug.h"
#include "psdrv.h"
#include "server.h"

int __winelib = 1;  /* Winelib run-time flag */

/***********************************************************************
 *           Main initialisation routine
 */
BOOL MAIN_MainInit(void)
{
    /* Set server debug level */
    /* To fool make_debug: TRACE(server) */
    CLIENT_SetDebug( TRACE_ON(server) );

    /* Initialize syslevel handling */
    SYSLEVEL_Init();

    /* Initialize signal handling */
    if (!SIGNAL_Init()) return FALSE;

    /* Load the configuration file */
    if (!PROFILE_LoadWineIni()) return FALSE;

      /* Initialize DOS memory */
    if (!DOSMEM_Init(0)) return FALSE;

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

    /* registry initialisation */
    SHELL_LoadRegistry();
    
    /* Read DOS config.sys */
    if (!DOSCONF_ReadConfig()) return FALSE;

    return TRUE;
}

/***********************************************************************
 *           KERNEL initialisation routine
 */
BOOL WINAPI MAIN_KernelInit(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    static BOOL initDone = FALSE;

    HMODULE16 hModule;

    if ( initDone ) return TRUE;
    initDone = TRUE;

    /* Initialize special KERNEL entry points */
    hModule = GetModuleHandle16( "KERNEL" );
    if ( hModule )
    {
        WORD cs, ds;

        /* Initialize KERNEL.178 (__WINFLAGS) with the correct flags value */
        NE_SetEntryPoint( hModule, 178, GetWinFlags16() );

        /* Initialize KERNEL.454/455 (__FLATCS/__FLATDS) */
        GET_CS(cs); GET_DS(ds);
        NE_SetEntryPoint( hModule, 454, cs );
        NE_SetEntryPoint( hModule, 455, ds );

        /* Initialize KERNEL.THHOOK */
        TASK_InstallTHHook((THHOOK *)PTR_SEG_TO_LIN(
                                  (SEGPTR)NE_GetEntryPoint( hModule, 332 )));

        /* Initialize the real-mode selector entry points */
#define SET_ENTRY_POINT( num, addr ) \
        NE_SetEntryPoint( hModule, (num), GLOBAL_CreateBlock( GMEM_FIXED, \
                          DOSMEM_MapDosToLinear(addr), 0x10000, hModule, \
                          FALSE, FALSE, FALSE, NULL ))

        SET_ENTRY_POINT( 183, 0x00000 );  /* KERNEL.183: __0000H */
        SET_ENTRY_POINT( 174, 0xa0000 );  /* KERNEL.174: __A000H */
        SET_ENTRY_POINT( 181, 0xb0000 );  /* KERNEL.181: __B000H */
        SET_ENTRY_POINT( 182, 0xb8000 );  /* KERNEL.182: __B800H */
        SET_ENTRY_POINT( 195, 0xc0000 );  /* KERNEL.195: __C000H */
        SET_ENTRY_POINT( 179, 0xd0000 );  /* KERNEL.179: __D000H */
        SET_ENTRY_POINT( 190, 0xe0000 );  /* KERNEL.190: __E000H */
        NE_SetEntryPoint( hModule, 173, DOSMEM_BiosSysSeg );  /* KERNEL.173: __ROMBIOS */
        NE_SetEntryPoint( hModule, 193, DOSMEM_BiosDataSeg ); /* KERNEL.193: __0040H */
        NE_SetEntryPoint( hModule, 194, DOSMEM_BiosSysSeg );  /* KERNEL.194: __F000H */
#undef SET_ENTRY_POINT
    }
    return TRUE;
}

/***********************************************************************
 *           GDI initialisation routine
 */
BOOL WINAPI MAIN_GdiInit(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    NE_MODULE *pModule;

    if ( GDI_HeapSel ) return TRUE;

    /* Create GDI heap */
    pModule = NE_GetPtr( GetModuleHandle16( "GDI" ) );
    if ( pModule )
    {
        GDI_HeapSel = GlobalHandleToSel16( (NE_SEG_TABLE( pModule ) + 
                                          pModule->dgroup - 1)->hSeg );
    }
    else
    {
        GDI_HeapSel = GlobalAlloc16( GMEM_FIXED, GDI_HEAP_SIZE );
        LocalInit16( GDI_HeapSel, 0, GDI_HEAP_SIZE-1 );
    }

    if (!TWEAK_Init()) return FALSE;

    /* GDI initialisation */
    if(!GDI_Init()) return FALSE;


    /* PSDRV initialization */
    if(!PSDRV_Init()) return FALSE;

    return TRUE;
}

/***********************************************************************
 *           USER initialisation routine
 */
BOOL WINAPI MAIN_UserInit(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    NE_MODULE *pModule;
    int queueSize;

    if ( USER_HeapSel ) return TRUE;

    /* Create USER heap */
    pModule = NE_GetPtr( GetModuleHandle16( "USER" ) );
    if ( pModule )
    {
        USER_HeapSel = GlobalHandleToSel16( (NE_SEG_TABLE( pModule ) + 
                                           pModule->dgroup - 1)->hSeg );
    }
    else
    {
        USER_HeapSel = GlobalAlloc16( GMEM_FIXED, 0x10000 );
        LocalInit16( USER_HeapSel, 0, 0xffff );
    }

     /* Global atom table initialisation */
    if (!ATOM_Init( USER_HeapSel )) return FALSE;

    /* Initialize system colors and metrics*/
    SYSMETRICS_Init();
    SYSCOLOR_Init();

    /* Create the DCEs */
    DCE_Init();

    /* Initialize timers */
    if (!TIMER_Init()) return FALSE;
    
    /* Initialize window procedures */
    if (!WINPROC_Init()) return FALSE;

    /* Initialize built-in window classes */
    if (!WIDGETS_Init()) return FALSE;

    /* Initialize dialog manager */
    if (!DIALOG_Init()) return FALSE;

    /* Initialize menus */
    if (!MENU_Init()) return FALSE;

    /* Initialize cursor/icons */
    CURSORICON_Init();

    /* Initialize multimedia */
    if (!MULTIMEDIA_Init()) return FALSE;

    /* Initialize message spying */
    if (!SPY_Init()) return FALSE;

    /* Check wine.conf for old/bad entries */
    if (!TWEAK_CheckConfiguration()) return FALSE;

    /* Create system message queue */
    queueSize = GetProfileIntA( "windows", "TypeAhead", 120 );
    if (!QUEUE_CreateSysMsgQueue( queueSize )) return FALSE;

    /* Set double click time */
    SetDoubleClickTime( GetProfileIntA("windows","DoubleClickSpeed",452) );

    /* Create task message queue for the initial task */
    queueSize = GetProfileIntA( "windows", "DefaultQueueSize", 8 );
    if (!SetMessageQueue( queueSize )) return FALSE;

    /* Create desktop window */
    if (!WIN_CreateDesktopWindow()) return FALSE;

    /* Install default USER Signal Handler */
    SetTaskSignalProc( 0, (FARPROC16)USER_SignalProc );

    /* Initialize keyboard driver */
    KEYBOARD_Enable( keybd_event, InputKeyStateTable );

    /* Initialize mouse driver */
    MOUSE_Enable( mouse_event );

    return TRUE;
}


/***********************************************************************
 *           Winelib initialisation routine
 */
HINSTANCE MAIN_WinelibInit( int *argc, char *argv[] )
{
    WINE_MODREF *wm;
    NE_MODULE *pModule;
    OFSTRUCT ofs;
    HMODULE16 hModule;

    /* Create the initial process */
    if (!PROCESS_Init()) return 0;

    /* Parse command line arguments */
    MAIN_WineInit( argc, argv );

    /* Main initialization */
    if (!MAIN_MainInit()) return 0;

    /* Initialize KERNEL */
    if (!MAIN_KernelInit(0, 0, NULL)) return 0;

    /* Create and switch to initial task */
    if (!(wm = ELF_CreateDummyModule( argv[0], argv[0] )))
        return 0;
    PROCESS_Current()->exe_modref = wm;

    strcpy( ofs.szPathName, wm->modname );
    if ((hModule = MODULE_CreateDummyModule( &ofs, NULL )) < 32) return 0;
    pModule = (NE_MODULE *)GlobalLock16( hModule );
    pModule->flags = NE_FFLAGS_WIN32;
    pModule->module32 = wm->module;

    if (!TASK_Create( THREAD_Current(), pModule, 0, 0, FALSE )) return 0;
    TASK_StartTask( PROCESS_Current()->task );

    /* Initialize GDI and USER */
    if (!MAIN_GdiInit(0, 0, NULL)) return 0;
    if (!MAIN_UserInit(0, 0, NULL)) return 0;

    return wm->module;
}

