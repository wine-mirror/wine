/*
 * DOS interrupt 33h handler
 */

#include <stdlib.h>
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "miscemu.h"
#include "dosexe.h"
#include "vga.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(int)

typedef struct {
  DWORD x, y, but;
  FARPROC16 callback;
  WORD callmask;
} MOUSESYSTEM;

/**********************************************************************
 *	    INT_Int33Handler
 *
 * Handler for int 33h (MS MOUSE).
 */
void WINAPI INT_Int33Handler( CONTEXT86 *context )
{
  MOUSESYSTEM *sys = (MOUSESYSTEM *)DOSVM_GetSystemData(0x33);

  switch (AX_reg(context)) {
  case 0x00:
    TRACE("Reset mouse driver and request status\n");
    AX_reg(context) = 0xFFFF; /* installed */
    BX_reg(context) = 3;      /* # of buttons */
    sys = calloc(1,sizeof(MOUSESYSTEM));
    DOSVM_SetSystemData(0x33, sys);
    break;
  case 0x03:
    TRACE("Return mouse position and button status\n");
    BX_reg(context) = sys->but;
    CX_reg(context) = sys->x;
    DX_reg(context) = sys->y;
    break;
  case 0x04:
    FIXME("Position mouse cursor\n");
    break;
  case 0x0C:
    TRACE("Define mouse interrupt subroutine\n");
    sys->callmask = CX_reg(context);
    sys->callback = (FARPROC16)PTR_SEG_OFF_TO_SEGPTR(ES_reg(context), DX_reg(context));
    break;
  default:
    INT_BARF(context,0x33);
  }
}

typedef struct {
  FARPROC16 proc;
  WORD mask,but,x,y,mx,my;
} MCALLDATA;

static void MouseRelay(LPDOSTASK lpDosTask,CONTEXT86 *context,void *mdata)
{
  MCALLDATA *data = (MCALLDATA *)mdata;
  CONTEXT86 ctx = *context;

  EAX_reg(&ctx) = data->mask;
  EBX_reg(&ctx) = data->but;
  ECX_reg(&ctx) = data->x;
  EDX_reg(&ctx) = data->y;
  ESI_reg(&ctx) = data->mx;
  EDI_reg(&ctx) = data->my;
  CS_reg(&ctx)  = SELECTOROF(data->proc);
  EIP_reg(&ctx) = OFFSETOF(data->proc);
  free(data);
  DPMI_CallRMProc(&ctx, NULL, 0, 0);
}

void WINAPI INT_Int33Message(UINT message,WPARAM wParam,LPARAM lParam)
{
  MOUSESYSTEM *sys = (MOUSESYSTEM *)DOSVM_GetSystemData(0x33);
  WORD mask = 0;
  unsigned Height, Width, SX=1, SY=1;

  if (!sys) return;
  if (!VGA_GetMode(&Height,&Width,NULL)) {
    /* may need to do some coordinate scaling */
    SX = 640/Width;
    if (!SX) SX=1;
  }
  sys->x = LOWORD(lParam) * SX;
  sys->y = HIWORD(lParam) * SY;
  switch (message) {
  case WM_MOUSEMOVE:
    mask |= 0x01;
    break;
  case WM_LBUTTONDOWN:
  case WM_LBUTTONDBLCLK:
    sys->but |= 0x01;
    mask |= 0x02;
    break;
  case WM_LBUTTONUP:
    sys->but &= ~0x01;
    mask |= 0x04;
    break;
  case WM_RBUTTONDOWN:
  case WM_RBUTTONDBLCLK:
    sys->but |= 0x02;
    mask |= 0x08;
    break;
  case WM_RBUTTONUP:
    sys->but &= ~0x02;
    mask |= 0x10;
    break;
  case WM_MBUTTONDOWN:
  case WM_MBUTTONDBLCLK:
    sys->but |= 0x04;
    mask |= 0x20;
    break;
  case WM_MBUTTONUP:
    sys->but &= ~0x04;
    mask |= 0x40;
    break;
  }

  if ((mask & sys->callmask) && sys->callback) {
    MCALLDATA *data = calloc(1,sizeof(MCALLDATA));
    data->proc = sys->callback;
    data->mask = mask & sys->callmask;
    data->but = sys->but;
    data->x = sys->x;
    data->y = sys->y;
    DOSVM_QueueEvent(-1, DOS_PRIORITY_MOUSE, MouseRelay, data);
  }
}
