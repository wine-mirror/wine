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
#include "debug.h"
#include "winuser.h"

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

void WINAPI INT_Int16Handler( CONTEXT *context )
{
   switch AH_reg(context) {

   case 0x00: /* Get Keystroke */
      /* Returns: AH = Scan code
                  AL = ASCII character */   
      TRACE(int16, "Get Keystroke\n");
      CONSOLE_GetKeystroke(&AH_reg(context), &AL_reg(context));
      break;

   case 0x01: /* Check for Keystroke */
      /* Returns: ZF set if no keystroke */
      /*          AH = Scan code */
      /*          AL = ASCII character */
      TRACE(int16, "Check for Keystroke\n");
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
      TRACE(int16, "Get Shift Flags: returning 0x%02x\n", AL_reg(context));
      break;

   case 0x03: /* Set Typematic Rate and Delay */
      FIXME(int16, "Set Typematic Rate and Delay - Not Supported\n");
      break;

   case 0x09: /* Get Keyboard Functionality */
      FIXME(int16, "Get Keyboard Functionality - Not Supported\n");
      /* As a temporary measure, say that "nothing" is supported... */
      AL_reg(context) = 0;
      break;

   case 0x0a: /* Get Keyboard ID */
      FIXME(int16, "Get Keyboard ID - Not Supported\n");
      break;

   case 0x10: /* Get Enhanced Keystroke */
      TRACE(int16, "Get Enhanced Keystroke - Partially supported\n");
      /* Returns: AH = Scan code
                  AL = ASCII character */   
      CONSOLE_GetKeystroke(&AH_reg(context), &AL_reg(context));
      break;
  

   case 0x11: /* Check for Enhanced Keystroke */
      /* Returns: ZF set if no keystroke */
      /*          AH = Scan code */
      /*          AL = ASCII character */
      TRACE(int16, "Check for Enhanced Keystroke - Partially supported\n");
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
      FIXME(int16, "Get Extended Shift States - Not Supported\n");
      break;
 
   default:
      FIXME(int16, "Unknown INT 16 function - 0x%x\n", AH_reg(context));   
      break;

   }
}

