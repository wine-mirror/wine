/*
 * DOS interrupt 16h handler
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "debug.h"

#include "ldt.h"
#include "drive.h"
#include "msdos.h"
#include "miscemu.h"
#include "module.h"
#include "console.h"

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
      FIXME(int16, "Get Shift Flags - Not Supported\n");
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
      FIXME(int16, "Get Enhanced Keystroke - Not Supported\n");
      break;

   case 0x11: /* Check for Enhanced Keystroke */
      FIXME(int16, "Check for Enhanced Keystroke - Not Supported\n");
      break;

   case 0x12: /* Get Extended Shift States */
      FIXME(int16, "Get Extended Shift States - Not Supported\n");
      break;
 
   default:
      FIXME(int16, "Unknown INT 16 function - 0x%x\n", AH_reg(context));   
      break;

   }
}

