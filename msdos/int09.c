/*
 * DOS interrupt 09h handler (IRQ1 - KEYBOARD)
 */

#include <stdlib.h>
#include <string.h>
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "miscemu.h"
#include "debugtools.h"
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
#if 0  /* FIXME: cannot call USER functions here */
      UINT vkey = MapVirtualKeyA(scan&0x7f, 1);
      /* as in TranslateMessage, windows/input.c */
      cnt = ToAscii(vkey, scan, QueueKeyStateTable, (LPWORD)ch, 0);
#else
      cnt = 0;
#endif
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
  DOSVM_PIC_ioport_out(0x20, 0x20); /* send EOI */
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
  DOSVM_QueueEvent(1,DOS_PRIORITY_KEYBOARD,KbdRelay,NULL);
}

BYTE WINAPI INT_Int09ReadScan( BYTE*ascii )
{
    if (ascii) *ascii = kbdinfo.ascii[0];
    return kbdinfo.queue[0];
}
