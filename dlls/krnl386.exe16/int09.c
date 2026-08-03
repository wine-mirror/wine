/*
 * DOS interrupt 09h handler (IRQ1 - KEYBOARD)
 *
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"
#include "dosexe.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

#define QUEUELEN 31

static struct
{
  BYTE queuelen,queue[QUEUELEN],ascii[QUEUELEN];
} kbdinfo;


/*
 * Update the BIOS data segment's keyboard status flags (mem 0x40:0x17/0x18)
 * if modifier/special keys have been pressed.
 * FIXME: we merely toggle key status and don't actively set it instead,
 * so we might be out of sync with the real current system status of these keys.
 * Probably doesn't matter too much, though.
 */
static void DOSVM_Int09UpdateKbdStatusFlags(BYTE scan, BOOL extended, BIOSDATA *data, BOOL *modifier)
{
    BYTE realscan = scan & 0x7f; /* remove 0x80 make/break flag */
    BYTE bit1 = 255, bit2 = 255;
    INPUT_RECORD msg;
    DWORD res;

    *modifier = TRUE;

    switch (realscan)
    {
      case 0x36: /* r shift */
	      bit1 = 0;
	      break;
      case 0x2a: /* l shift */
	      bit1 = 1;
	      break;
      case 0x1d: /* l/r control */
	      bit1 = 2;
	      if (!extended) /* left control only */
		  bit2 = 0;
	      break;
      case 0x37: /* SysRq inner parts */
	      /* SysRq scan code sequence: 38, e0, 37, e0, b7, b8 */
	      FIXME("SysRq not handled yet.\n");
	      break;
      case 0x38: /* l/r menu/alt, SysRq outer parts */
	      bit1 = 3;
	      if (!extended) /* left alt only */
	          bit2 = 1;
	      break;
      case 0x46: /* scroll lock */
	      bit1 = 4;
	      if (!extended) /* left ctrl only */
	          bit2 = 4;
	      break;
      case 0x45: /* num lock, pause */
	      if (extended) /* distinguish from non-extended Pause key */
              { /* num lock */
	          bit1 = 5;
	          bit2 = 5;
              }
	      else
              { /* pause */
                  if (!(scan & 0x80)) /* "make" code */
                      bit2 = 3;
              }
	      break;
      case 0x3a: /* caps lock */
	      bit1 = 6;
	      bit2 = 6;
	      break;
      case 0x52: /* insert */
	      bit1 = 7;
	      bit2 = 7;
	      *modifier = FALSE; /* insert is no modifier: thus pass to int16 */
	      break;
    }
    /* now that we know which bits to set, update them */
    if (!(scan & 0x80)) /* "make" code (keypress) */
    {
        if (bit2 != 255)
        {
	    if (bit2 == 3)
	    {
                data->KbdFlags2 |= 1 << bit2; /* set "Pause" flag */
                TRACE("PAUSE key, sleeping !\n");
                /* wait for keypress to unlock pause */
		do {
                    Sleep(55);
		} while (!(ReadConsoleInputA(GetStdHandle(STD_INPUT_HANDLE),&msg,1,&res) && (msg.EventType == KEY_EVENT)));
                data->KbdFlags2 &= ~(1 << bit2); /* release "Pause" flag */
            }
	    else
                data->KbdFlags2 |= 1 << bit2;
        }
        if (bit1 != 255)
        {
            if (bit1 < 4) /* key "pressed" flag */
	        data->KbdFlags1 |= 1 << bit1;
            else /* key "active" flag */
                data->KbdFlags1 ^= 1 << bit1;
        }
    }
    else /* "break" / release */
    {
        if (bit2 != 255)
            data->KbdFlags2 &= ~(1 << bit2);
        if (bit1 < 4) /* is it a key "pressed" bit ? */
	    data->KbdFlags1 &= ~(1 << bit1);
    }
    TRACE("ext. %d, bits %d/%d, KbdFlags %02x/%02x\n", extended, bit1, bit2, data->KbdFlags1, data->KbdFlags2);
}

/**********************************************************************
 *	    DOSVM_Int09Handler
 *
 * Handler for int 09h.
 * See http://www.execpc.com/~geezer/osd/kbd/ for a very good description
 * of keyboard mapping modes.
 */
void WINAPI DOSVM_Int09Handler( CONTEXT *context )
{
  BIOSDATA *data = DOSVM_BiosData();
  BYTE ascii, scan = DOSVM_Int09ReadScan(&ascii);
  BYTE realscan = scan & 0x7f; /* remove 0x80 make/break flag */
  BOOL modifier = FALSE;
  static BOOL extended = FALSE; /* indicates start of extended key sequence */
  BYTE ch[2];
  int cnt, c2;

  TRACE("scan=%02x, ascii=%02x[%c]\n",scan, ascii, ascii ? ascii : ' ');

  if (scan == 0xe0) /* extended keycode */
      extended = TRUE;
  
  /* check for keys concerning keyboard status flags */
  if ((realscan == 0x52 /* insert */)
  ||  (realscan == 0x3a /* caps lock */)
  ||  (realscan == 0x45 /* num lock (extended) or pause/break */)
  ||  (realscan == 0x46 /* scroll lock */)
  ||  (realscan == 0x2a /* l shift */)
  ||  (realscan == 0x36 /* r shift */)
  ||  (realscan == 0x37 /* SysRq */)
  ||  (realscan == 0x38 /* l/r menu/alt, SysRq */)
  ||  (realscan == 0x1d /* l/r control */))
      DOSVM_Int09UpdateKbdStatusFlags(scan, extended, data, &modifier);

  if (scan != 0xe0)
      extended = FALSE; /* reset extended flag now */

  /* only interested in "make" (press) codes, not "break" (release),
   * and also not in "modifier key only" (w/o ascii) notifications */
  if (!(scan & 0x80) && !(modifier && !ascii))
  {
    if (ascii) {
      /* we already have an ASCII code, no translation necessary */
      if (data->KbdFlags1 & 8) /* Alt key ? */
	ch[0] = 0; /* ASCII code needs to be 0 if Alt also pressed */
      else
        ch[0] = ascii;
      /* FIXME: need to handle things such as Shift-F1 etc. */
      cnt = 1;
    } else {
      /* translate */
      UINT vkey = MapVirtualKeyA(scan&0x7f, 1);
      BYTE keystate[256];
      GetKeyboardState(keystate);
      cnt = ToAscii(vkey, scan, keystate, (LPWORD)ch, 0);
    }
    if (cnt>0) {
      for (c2=0; c2<cnt; c2++)
        DOSVM_Int16AddChar(ch[c2], scan);
    } else
    if (cnt==0) {
      /* FIXME: need to handle things like shift-F-keys,
       * 0xE0 extended keys, etc */
      DOSVM_Int16AddChar(0, scan);
    }
  }

  DOSVM_AcknowledgeIRQ( context );
}

static void KbdRelay( CONTEXT *context, void *data )
{
  if (kbdinfo.queuelen) {
    /* cleanup operation, called from DOSVM_PIC_ioport_out:
     * we'll remove current scancode from keyboard buffer here,
     * rather than in ReadScan, because some DOS apps depend on
     * the scancode being available for reading multiple times... */
    if (--kbdinfo.queuelen) {
      memmove(kbdinfo.queue,kbdinfo.queue+1,kbdinfo.queuelen);
      memmove(kbdinfo.ascii,kbdinfo.ascii+1,kbdinfo.queuelen);
    }
  }
}

void DOSVM_Int09SendScan( BYTE scan, BYTE ascii )
{
  if (kbdinfo.queuelen == QUEUELEN) {
    ERR("keyboard queue overflow\n");
    return;
  }
  /* add scancode to queue */
  kbdinfo.queue[kbdinfo.queuelen] = scan;
  kbdinfo.ascii[kbdinfo.queuelen++] = ascii;
  /* tell app to read it by triggering IRQ 1 (int 09) */
  DOSVM_QueueEvent(1,DOS_PRIORITY_KEYBOARD,KbdRelay,NULL);
}

BYTE DOSVM_Int09ReadScan( BYTE*ascii )
{
    if (ascii) *ascii = kbdinfo.ascii[0];
    return kbdinfo.queue[0];
}
