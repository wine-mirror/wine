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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "module.h"
#include "dosexe.h"
#include "wincon.h"
#include "wine/debug.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "miscemu.h"

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

void WINAPI DOSVM_Int16Handler( CONTEXT86 *context )
{
   BIOSDATA *data = NULL;
   BYTE ascii, scan;

   switch AH_reg(context) {

   case 0x00: /* Get Keystroke */
      /* Returns: AH = Scan code
                  AL = ASCII character */
      TRACE("Get Keystroke\n");
      DOSVM_Int16ReadChar(&ascii, &scan, FALSE);
      SET_AL( context, ascii );
      SET_AH( context, scan );
      break;

   case 0x01: /* Check for Keystroke */
      /* Returns: ZF set if no keystroke */
      /*          AH = Scan code */
      /*          AL = ASCII character */
      TRACE("Check for Keystroke\n");
      if (!DOSVM_Int16ReadChar(&ascii, &scan, TRUE))
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
      data = BIOS_DATA;
      SET_AL( context, data->KbdFlags1 );

      TRACE("Get Shift Flags: returning 0x%02x\n", AL_reg(context));
      break;

   case 0x03: /* Set Typematic Rate and Delay */
      FIXME("Set Typematic Rate and Delay - Not Supported\n");
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
      DOSVM_Int16ReadChar(&ascii, &scan, FALSE);
      SET_AL( context, ascii );
      SET_AH( context, scan );
      break;


   case 0x11: /* Check for Enhanced Keystroke */
      /* Returns: ZF set if no keystroke */
      /*          AH = Scan code */
      /*          AL = ASCII character */
      TRACE("Check for Enhanced Keystroke - Partially supported\n");
      if (!DOSVM_Int16ReadChar(&ascii, &scan, TRUE))
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

int WINAPI DOSVM_Int16ReadChar(BYTE*ascii,BYTE*scan,BOOL peek)
{
  BIOSDATA *data = BIOS_DATA;
  WORD CurOfs = data->NextKbdCharPtr;

  /* check if there's data in buffer */
  if (peek) {
    if (CurOfs == data->FirstKbdCharPtr)
      return 0;
  } else {
    while (CurOfs == data->FirstKbdCharPtr) {
      /* no input available yet, so wait... */
      DOSVM_Wait( -1, 0 );
    }
  }
  /* read from keyboard queue */
  TRACE("(%p,%p,%d) -> %02x %02x\n",ascii,scan,peek,((BYTE*)data)[CurOfs],((BYTE*)data)[CurOfs+1]);
  if (ascii) *ascii = ((BYTE*)data)[CurOfs];
  if (scan) *scan = ((BYTE*)data)[CurOfs+1];
  if (!peek) {
    CurOfs += 2;
    if (CurOfs >= data->KbdBufferEnd) CurOfs = data->KbdBufferStart;
    data->NextKbdCharPtr = CurOfs;
  }
  return 1;
}

int WINAPI DOSVM_Int16AddChar(BYTE ascii,BYTE scan)
{
  BIOSDATA *data = BIOS_DATA;
  WORD CurOfs = data->FirstKbdCharPtr;
  WORD NextOfs = CurOfs + 2;

  TRACE("(%02x,%02x)\n",ascii,scan);
  if (NextOfs >= data->KbdBufferEnd) NextOfs = data->KbdBufferStart;
  /* check if buffer is full */
  if (NextOfs == data->NextKbdCharPtr) return 0;

  /* okay, insert character in ring buffer */
  ((BYTE*)data)[CurOfs] = ascii;
  ((BYTE*)data)[CurOfs+1] = scan;

  data->FirstKbdCharPtr = NextOfs;
  return 1;
}
