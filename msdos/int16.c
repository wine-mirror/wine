/*
 * DOS interrupt 16h handler
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "module.h"
#include "console.h"
#include "wincon.h"
#include "debugtools.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "miscemu.h"

DEFAULT_DEBUG_CHANNEL(int16)

/**********************************************************************
 *	    INT_Int16Handler
 *
 * Handler for int 16h (keyboard)
 *
 * NOTE:
 * 
 *    KEYB.COM (DOS >3.2) adds functions to this interrupt, they are
 *    not currently listed here.
 */

void WINAPI INT_Int16Handler( CONTEXT86 *context )
{
   switch AH_reg(context) {

   case 0x00: /* Get Keystroke */
      /* Returns: AH = Scan code
                  AL = ASCII character */   
      TRACE("Get Keystroke\n");
      CONSOLE_GetKeystroke(&AH_reg(context), &AL_reg(context));
      break;

   case 0x01: /* Check for Keystroke */
      /* Returns: ZF set if no keystroke */
      /*          AH = Scan code */
      /*          AL = ASCII character */
      TRACE("Check for Keystroke\n");
      if (!CONSOLE_CheckForKeystroke(&AH_reg(context), &AL_reg(context)))
      {
          SET_ZFLAG(context);
      }
      else
      {
          RESET_ZFLAG(context);
      }
      break;

   case 0x02: /* Get Shift Flags */      
      AL_reg(context) = 0;

      if (GetAsyncKeyState(VK_RSHIFT))
          AL_reg(context) |= 0x01;
      if (GetAsyncKeyState(VK_LSHIFT))
          AL_reg(context) |= 0x02;
      if (GetAsyncKeyState(VK_LCONTROL) || GetAsyncKeyState(VK_RCONTROL))
          AL_reg(context) |= 0x04;
      if (GetAsyncKeyState(VK_LMENU) || GetAsyncKeyState(VK_RMENU))
          AL_reg(context) |= 0x08;
      if (GetAsyncKeyState(VK_SCROLL))
          AL_reg(context) |= 0x10;
      if (GetAsyncKeyState(VK_NUMLOCK))
          AL_reg(context) |= 0x20;
      if (GetAsyncKeyState(VK_CAPITAL))
          AL_reg(context) |= 0x40;
      if (GetAsyncKeyState(VK_INSERT))
          AL_reg(context) |= 0x80;
      TRACE("Get Shift Flags: returning 0x%02x\n", AL_reg(context));
      break;

   case 0x03: /* Set Typematic Rate and Delay */
      FIXME("Set Typematic Rate and Delay - Not Supported\n");
      break;

   case 0x09: /* Get Keyboard Functionality */
      FIXME("Get Keyboard Functionality - Not Supported\n");
      /* As a temporary measure, say that "nothing" is supported... */
      AL_reg(context) = 0;
      break;

   case 0x0a: /* Get Keyboard ID */
      FIXME("Get Keyboard ID - Not Supported\n");
      break;

   case 0x10: /* Get Enhanced Keystroke */
      TRACE("Get Enhanced Keystroke - Partially supported\n");
      /* Returns: AH = Scan code
                  AL = ASCII character */   
      CONSOLE_GetKeystroke(&AH_reg(context), &AL_reg(context));
      break;
  

   case 0x11: /* Check for Enhanced Keystroke */
      /* Returns: ZF set if no keystroke */
      /*          AH = Scan code */
      /*          AL = ASCII character */
      TRACE("Check for Enhanced Keystroke - Partially supported\n");
      if (!CONSOLE_CheckForKeystroke(&AH_reg(context), &AL_reg(context)))
      {
          SET_ZFLAG(context);
      }
      else
      {
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

int WINAPI INT_Int16ReadChar(BYTE*ascii,BYTE*scan,BOOL peek)
{
  BIOSDATA *data = DOSMEM_BiosData();
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

int WINAPI INT_Int16AddChar(BYTE ascii,BYTE scan)
{
  BIOSDATA *data = DOSMEM_BiosData();
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
