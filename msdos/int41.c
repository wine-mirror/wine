/*
 * DOS interrupt 41h handler  -- Windows Kernel Debugger
 * 
 * Check debugsys.inc from the DDK for docu.
 */

#include <stdio.h>
#include "miscemu.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(int);

/***********************************************************************
 *           INT_Int41Handler
 *
 */
void WINAPI INT_Int41Handler( CONTEXT86 *context )
{
    if ( ISV86(context) )
    {
        /* Real-mode debugger services */
        switch ( AX_reg(context) )
        {
        default:
            INT_BARF( context, 0x41 );
            break;
        }
    }
    else
    {
        /* Protected-mode debugger services */
        switch ( AX_reg(context) )
        {
        case 0x4f:
        case 0x50:
        case 0x150:
        case 0x51:
        case 0x52:
        case 0x152:
        case 0x59:
        case 0x5a:
        case 0x5b:
        case 0x5c:
        case 0x5d:
            /* Notifies the debugger of a lot of stuff. We simply ignore it
               for now, but some of the info might actually be useful ... */
            break;

        default:
            INT_BARF( context, 0x41 );
            break;
        }
    }
}

