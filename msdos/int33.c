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

DEFAULT_DEBUG_CHANNEL(int);

static struct
{
  DWORD x, y, but;
  FARPROC16 callback;
  WORD callmask;
} mouse_info;

/**********************************************************************
 *	    INT_Int33Handler
 *
 * Handler for int 33h (MS MOUSE).
 */
void WINAPI INT_Int33Handler( CONTEXT86 *context )
{
  switch (LOWORD(context->Eax)) {
  case 0x00:
    TRACE("Reset mouse driver and request status\n");
    AX_reg(context) = 0xFFFF; /* installed */
    BX_reg(context) = 3;      /* # of buttons */
    memset( &mouse_info, 0, sizeof(mouse_info) );
    break;
  case 0x01:
    FIXME("Show mouse cursor\n");
    break;
  case 0x02:
    FIXME("Hide mouse cursor\n");
    break;
  case 0x03:
    TRACE("Return mouse position and button status\n");
    BX_reg(context) = mouse_info.but;
    CX_reg(context) = mouse_info.x;
    DX_reg(context) = mouse_info.y;
    break;
  case 0x04:
    FIXME("Position mouse cursor\n");
    break;
  case 0x07:
    FIXME("Define horizontal mouse cursor range\n");
    break;
  case 0x08:
    FIXME("Define vertical mouse cursor range\n");
    break;
  case 0x09:
    FIXME("Define graphics mouse cursor\n");
    break;
  case 0x0A:
    FIXME("Define text mouse cursor\n");
    break;
  case 0x0C:
    TRACE("Define mouse interrupt subroutine\n");
    mouse_info.callmask = CX_reg(context);
    mouse_info.callback = (FARPROC16)PTR_SEG_OFF_TO_SEGPTR(context->SegEs, LOWORD(context->Edx));
    break;
  case 0x10:
    FIXME("Define screen region for update\n");
    break;
  default:
    INT_BARF(context,0x33);
  }
}

typedef struct {
  FARPROC16 proc;
  WORD mask,but,x,y,mx,my;
} MCALLDATA;

static void MouseRelay(CONTEXT86 *context,void *mdata)
{
  MCALLDATA *data = (MCALLDATA *)mdata;
  CONTEXT86 ctx = *context;

  ctx.Eax   = data->mask;
  ctx.Ebx   = data->but;
  ctx.Ecx   = data->x;
  ctx.Edx   = data->y;
  ctx.Esi   = data->mx;
  ctx.Edi   = data->my;
  ctx.SegCs = SELECTOROF(data->proc);
  ctx.Eip   = OFFSETOF(data->proc);
  free(data);
  DPMI_CallRMProc(&ctx, NULL, 0, 0);
}

void WINAPI INT_Int33Message(UINT message,WPARAM wParam,LPARAM lParam)
{
  WORD mask = 0;
  unsigned Height, Width, SX=1, SY=1;

  if (!VGA_GetMode(&Height,&Width,NULL)) {
    /* may need to do some coordinate scaling */
    SX = 640/Width;
    if (!SX) SX=1;
  }
  mouse_info.x = LOWORD(lParam) * SX;
  mouse_info.y = HIWORD(lParam) * SY;
  switch (message) {
  case WM_MOUSEMOVE:
    mask |= 0x01;
    break;
  case WM_LBUTTONDOWN:
  case WM_LBUTTONDBLCLK:
    mouse_info.but |= 0x01;
    mask |= 0x02;
    break;
  case WM_LBUTTONUP:
    mouse_info.but &= ~0x01;
    mask |= 0x04;
    break;
  case WM_RBUTTONDOWN:
  case WM_RBUTTONDBLCLK:
    mouse_info.but |= 0x02;
    mask |= 0x08;
    break;
  case WM_RBUTTONUP:
    mouse_info.but &= ~0x02;
    mask |= 0x10;
    break;
  case WM_MBUTTONDOWN:
  case WM_MBUTTONDBLCLK:
    mouse_info.but |= 0x04;
    mask |= 0x20;
    break;
  case WM_MBUTTONUP:
    mouse_info.but &= ~0x04;
    mask |= 0x40;
    break;
  }

  if ((mask & mouse_info.callmask) && mouse_info.callback) {
    MCALLDATA *data = calloc(1,sizeof(MCALLDATA));
    data->proc = mouse_info.callback;
    data->mask = mask & mouse_info.callmask;
    data->but = mouse_info.but;
    data->x = mouse_info.x;
    data->y = mouse_info.y;
    DOSVM_QueueEvent(-1, DOS_PRIORITY_MOUSE, MouseRelay, data);
  }
}
