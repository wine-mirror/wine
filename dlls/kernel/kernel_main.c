/*
 * Kernel initialization code
 */

#include <assert.h>

#include "winbase.h"
#include "wine/winbase16.h"

#include "neexe.h"
#include "module.h"
#include "task.h"
#include "comm.h"
#include "selectors.h"
#include "miscemu.h"
#include "global.h"

extern void CODEPAGE_Init(void);

/***********************************************************************
 *           KERNEL process initialisation routine
 */
static BOOL process_attach(void)
{
    HMODULE16 hModule;

    /* Setup codepage info */
    CODEPAGE_Init();

    /* Initialize DOS memory */
    if (!DOSMEM_Init(0)) return FALSE;

    if ((hModule = LoadLibrary16( "krnl386.exe" )) < 32) return FALSE;

    /* Initialize special KERNEL entry points */

    /* Initialize KERNEL.178 (__WINFLAGS) with the correct flags value */
    NE_SetEntryPoint( hModule, 178, GetWinFlags16() );

    /* Initialize KERNEL.454/455 (__FLATCS/__FLATDS) */
    NE_SetEntryPoint( hModule, 454, __get_cs() );
    NE_SetEntryPoint( hModule, 455, __get_ds() );

    /* Initialize KERNEL.THHOOK */
    TASK_InstallTHHook((THHOOK *)PTR_SEG_TO_LIN((SEGPTR)NE_GetEntryPoint( hModule, 332 )));

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

    /* Force loading of some dlls */
    if (LoadLibrary16( "system" ) < 32) return FALSE;
    if (LoadLibrary16( "wprocs" ) < 32) return FALSE;

    /* Initialize communications */
    COMM_Init();

    /* Read DOS config.sys */
    if (!DOSCONF_ReadConfig()) return FALSE;

    return TRUE;
}

/***********************************************************************
 *           KERNEL initialisation routine
 */
BOOL WINAPI MAIN_KernelInit( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    static int process_count;

    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        if (!process_count++) return process_attach();
        break;
    case DLL_PROCESS_DETACH:
        --process_count;
        break;
    }
    return TRUE;
}
