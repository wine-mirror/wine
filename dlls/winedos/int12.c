/*
 * BIOS interrupt 12h handler
 */

#include "miscemu.h"

/**********************************************************************
 *         DOSVM_Int12Handler (WINEDOS16.118)
 *
 * Handler for int 12h (get memory size).
 */
void WINAPI DOSVM_Int12Handler( CONTEXT86 *context )
{
    SET_AX( context, 640 );
}
