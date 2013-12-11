/*
 * DOS interrupt 16h handler
 *
 * Copyright 1998 Joseph Pranevich
 * Copyright 1999 Ove Kåven
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "dosexe.h"
#include "wincon.h"
#include "wine/debug.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

/**********************************************************************
 *	    DOSVM_Int16Handler
 *
 * Handler for int 16h (keyboard)
 *
 * NOTE:
 *
 *    KEYB.COM (DOS >3.2) adds functions to this interrupt, they are
 *    not currently listed here.
 */

void WINAPI DOSVM_Int16Handler( CONTEXT *context )
{
   BIOSDATA *data = NULL;
   BYTE ascii, scan;

   switch (AH_reg(context)) {

   case 0x00: /* Get Keystroke */
      /* Returns: AH = Scan code
                  AL = ASCII character */
      TRACE("Get Keystroke\n");
      DOSVM_Int16ReadChar(&ascii, &scan, context);
      SET_AL( context, ascii );
      SET_AH( context, scan );
      break;

   case 0x01: /* Check for Keystroke */
      /* Returns: ZF set if no keystroke */
      /*          AH = Scan code */
      /*          AL = ASCII character */
      TRACE("Check for Keystroke\n");
      if (!DOSVM_Int16ReadChar(&ascii, &scan, NULL))
      {
          SET_ZFLAG(context);
      }
      else
      {
          SET_AL( context, ascii );
          SET_AH( context, scan );
          RESET_ZFLAG(context);
      }
      /* don't miss the opportunity to break some tight timing loop in DOS
       * programs causing 100% CPU usage (by doing a Sleep here) */
      Sleep(5);
      break;

   case 0x02: /* Get Shift Flags */

      /* read value from BIOS data segment's keyboard status flags field */
      data = DOSVM_BiosData();
      SET_AL( context, data->KbdFlags1 );

      TRACE("Get Shift Flags: returning 0x%02x\n", AL_reg(context));
      break;

   case 0x03: /* Set Typematic Rate and Delay */
      FIXME("Set Typematic Rate and Delay - Not Supported\n");
      break;
      
   case 0x05:/*simulate  Keystroke*/ 
      FIXME("Simulating a keystroke is not supported yet\n");
      break;

   case 0x09: /* Get Keyboard Functionality */
      FIXME("Get Keyboard Functionality - Not Supported\n");
      /* As a temporary measure, say that "nothing" is supported... */
      SET_AL( context, 0 );
      break;

   case 0x0a: /* Get Keyboard ID */
      FIXME("Get Keyboard ID - Not Supported\n");
      break;

   case 0x10: /* Get Enhanced Keystroke */
      TRACE("Get Enhanced Keystroke - Partially supported\n");
      /* Returns: AH = Scan code
                  AL = ASCII character */
      DOSVM_Int16ReadChar(&ascii, &scan, context);
      SET_AL( context, ascii );
      SET_AH( context, scan );
      break;


   case 0x11: /* Check for Enhanced Keystroke */
      /* Returns: ZF set if no keystroke */
      /*          AH = Scan code */
      /*          AL = ASCII character */
      TRACE("Check for Enhanced Keystroke - Partially supported\n");
      if (!DOSVM_Int16ReadChar(&ascii, &scan, NULL))
      {
          SET_ZFLAG(context);
      }
      else
      {
          SET_AL( context, ascii );
          SET_AH( context, scan );
          RESET_ZFLAG(context);
      }
      break;

   case 0x12: /* Get Extended Shift States */
      FIXME("Get Extended Shift States - Not Supported\n");
      break;

   default:
      FIXME("Unknown INT 16 function - 0x%x\n", AH_reg(context));
      break;

   }
}

/**********************************************************************
 *	    DOSVM_Int16ReadChar
 *
 * Either peek into keyboard buffer or wait for next keystroke.
 *
 * If waitctx is NULL, return TRUE if buffer had keystrokes and
 * FALSE if buffer is empty. Returned keystroke will be left into buffer.
 * 
 * If waitctx is non-NULL, wait until keystrokes are available.
 * Return value will always be TRUE and returned keystroke will be
 * removed from buffer.
 */
BOOL DOSVM_Int16ReadChar(BYTE *ascii, BYTE *scan, CONTEXT *waitctx)
{
    BIOSDATA *data = DOSVM_BiosData();
    WORD CurOfs = data->NextKbdCharPtr;

    /* check if there's data in buffer */
    if (waitctx)
    {
        /* wait until input is available... */
        while (CurOfs == data->FirstKbdCharPtr)
            DOSVM_Wait( waitctx );
    }
    else
    {
        if (CurOfs == data->FirstKbdCharPtr)
            return FALSE;
    }

    /* read from keyboard queue */
    TRACE( "(%p,%p,%p) -> %02x %02x\n", ascii, scan, waitctx,
           ((BYTE*)data)[CurOfs], ((BYTE*)data)[CurOfs+1] );

    if (ascii) *ascii = ((BYTE*)data)[CurOfs];
    if (scan) *scan = ((BYTE*)data)[CurOfs+1];

    if (waitctx) 
    {
        CurOfs += 2;
        if (CurOfs >= data->KbdBufferEnd) CurOfs = data->KbdBufferStart;
        data->NextKbdCharPtr = CurOfs;
    }

    return TRUE;
}

BOOL DOSVM_Int16AddChar(BYTE ascii,BYTE scan)
{
  BIOSDATA *data = DOSVM_BiosData();
  WORD CurOfs = data->FirstKbdCharPtr;
  WORD NextOfs = CurOfs + 2;

  TRACE("(%02x,%02x)\n",ascii,scan);
  if (NextOfs >= data->KbdBufferEnd) NextOfs = data->KbdBufferStart;
  /* check if buffer is full */
  if (NextOfs == data->NextKbdCharPtr) return FALSE;

  /* okay, insert character in ring buffer */
  ((BYTE*)data)[CurOfs] = ascii;
  ((BYTE*)data)[CurOfs+1] = scan;

  data->FirstKbdCharPtr = NextOfs;
  return TRUE;
}
