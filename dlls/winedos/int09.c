/*
 * DOS interrupt 09h handler (IRQ1 - KEYBOARD)
 */

#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "miscemu.h"
#include "debugtools.h"
#include "callback.h"
#include "dosexe.h"

DEFAULT_DEBUG_CHANNEL(int);

#define QUEUELEN 31

static struct
{
  BYTE queuelen,queue[QUEUELEN],ascii[QUEUELEN];
} kbdinfo;


/**********************************************************************
 *	    INT_Int09Handler
 *
 * Handler for int 09h.
 */
void WINAPI INT_Int09Handler( CONTEXT86 *context )
{
  BYTE ascii, scan = INT_Int09ReadScan(&ascii);
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
        INT_Int16AddChar(ch[c2], scan);
    } else
    if (cnt==0) {
      /* FIXME: need to handle things like shift-F-keys,
       * 0xE0 extended keys, etc */
      INT_Int16AddChar(0, scan);
    }
  }
  Dosvm.OutPIC(0x20, 0x20); /* send EOI */
}

static void KbdRelay( CONTEXT86 *context, void *data )
{
  if (kbdinfo.queuelen) {
    /* cleanup operation, called from Dosvm.OutPIC:
     * we'll remove current scancode from keyboard buffer here,
     * rather than in ReadScan, because some DOS apps depend on
     * the scancode being available for reading multiple times... */
    if (--kbdinfo.queuelen) {
      memmove(kbdinfo.queue,kbdinfo.queue+1,kbdinfo.queuelen);
      memmove(kbdinfo.ascii,kbdinfo.ascii+1,kbdinfo.queuelen);
    }
  }
}

void WINAPI INT_Int09SendScan( BYTE scan, BYTE ascii )
{
  if (kbdinfo.queuelen == QUEUELEN) {
    ERR("keyboard queue overflow\n");
    return;
  }
  /* add scancode to queue */
  kbdinfo.queue[kbdinfo.queuelen] = scan;
  kbdinfo.ascii[kbdinfo.queuelen++] = ascii;
  /* tell app to read it by triggering IRQ 1 (int 09) */
  Dosvm.QueueEvent(1,DOS_PRIORITY_KEYBOARD,KbdRelay,NULL);
}

BYTE WINAPI INT_Int09ReadScan( BYTE*ascii )
{
    if (ascii) *ascii = kbdinfo.ascii[0];
    return kbdinfo.queue[0];
}
