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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "miscemu.h"
#include "wine/debug.h"
#include "dosexe.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

#define QUEUELEN 31

static struct
{
  BYTE queuelen,queue[QUEUELEN],ascii[QUEUELEN];
} kbdinfo;


/**********************************************************************
 *	    DOSVM_Int09Handler
 *
 * Handler for int 09h.
 */
void WINAPI DOSVM_Int09Handler( CONTEXT86 *context )
{
  BYTE ascii, scan = DOSVM_Int09ReadScan(&ascii);
  BYTE ch[2];
  int cnt, c2;

  TRACE("scan=%02x\n",scan);
  if (!(scan & 0x80)) {
    if (ascii) {
      /* we already have an ASCII code, no translation necessary */
      ch[0] = ascii;
      cnt = 1;
    } else {
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
  DOSVM_PIC_ioport_out( 0x20, 0x20 ); /* send EOI */
}

static void KbdRelay( CONTEXT86 *context, void *data )
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

void WINAPI DOSVM_Int09SendScan( BYTE scan, BYTE ascii )
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

/**********************************************************************
 *	    KbdReadScan (WINEDOS.@)
 */
BYTE WINAPI DOSVM_Int09ReadScan( BYTE*ascii )
{
    if (ascii) *ascii = kbdinfo.ascii[0];
    return kbdinfo.queue[0];
}
