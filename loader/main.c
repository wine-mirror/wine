/*
 * Main initialization code
 */

#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "windef.h"
#include "wingdi.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "bitmap.h"
#include "comm.h"
#include "neexe.h"
#include "main.h"
#include "menu.h"
#include "message.h"
#include "dialog.h"
#include "drive.h"
#include "queue.h"
#include "sysmetrics.h"
#include "file.h"
#include "heap.h"
#include "keyboard.h"
#include "mouse.h"
#include "input.h"
#include "display.h"
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
#include "win.h"
#include "winproc.h"
#include "syslevel.h"
#include "services.h"
#include "winsock.h"
#include "selectors.h"
#include "thread.h"
#include "task.h"
#include "debugtools.h"
#include "psdrv.h"
#include "win16drv.h"
#include "callback.h"
#include "server.h"
#include "loadorder.h"

DEFAULT_DEBUG_CHANNEL(server);

/***********************************************************************
 *           Main initialisation routine
 */
BOOL MAIN_MainInit( char *argv[] )
{
    /* store the program name */
    argv0 = argv[0];

    /* Create the initial process */
    if (!PROCESS_Init()) return FALSE;

    /* Parse command line arguments */
    OPTIONS_ParseOptions( argv );
    MAIN_WineInit();

    /* Load the configuration file */
    if (!PROFILE_LoadWineIni()) return FALSE;

    /* Initialise DOS drives */
    if (!DRIVE_Init()) return FALSE;

    /* Initialise DOS directories */
    if (!DIR_Init()) return FALSE;

    /* Registry initialisation */
    SHELL_LoadRegistry();
    
    /* Global boot finished, the rest is process-local */
    CLIENT_BootDone( TRACE_ON(server) );

    /* Initialize module loadorder */
    if (!MODULE_InitLoadOrder()) return FALSE;

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

    /* Initialize DOS memory */
    if (!DOSMEM_Init(0)) return FALSE;

    if (!LoadLibrary16( "KRNL386.EXE" )) return FALSE;

    /* Initialize special KERNEL entry points */
    hModule = GetModuleHandle16( "KERNEL" );
    if ( hModule )
    {
        /* Initialize KERNEL.178 (__WINFLAGS) with the correct flags value */
        NE_SetEntryPoint( hModule, 178, GetWinFlags16() );

        /* Initialize KERNEL.454/455 (__FLATCS/__FLATDS) */
        NE_SetEntryPoint( hModule, 454, __get_cs() );
        NE_SetEntryPoint( hModule, 455, __get_ds() );

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

    /* Initialize relay code */
    if (!RELAY_Init()) return FALSE;

    /* Initialize communications */
    COMM_Init();

    /* Initialize IO-port permissions */
    IO_port_init();

    /* Read DOS config.sys */
    if (!DOSCONF_ReadConfig()) return FALSE;

    return TRUE;
}


/***********************************************************************
 *           ExitKernel16 (KERNEL.2)
 *
 * Clean-up everything and exit the Wine process.
 *
 */
void WINAPI ExitKernel16( void )
{
    /* Do the clean-up stuff */

    WriteOutProfiles16();
    TerminateProcess( GetCurrentProcess(), 0 );
}

