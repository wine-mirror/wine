/*
 * DOS interrupt 33h handler
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
#include "dosexe.h"
#include "vga.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

static struct
{
  DWORD x, y, but;
  WORD lbcount, rbcount, rlastx, rlasty, llastx, llasty;
  FARPROC16 callback;
  WORD callmask;
  WORD VMPratio, HMPratio, oldx, oldy;
} mouse_info;

/**********************************************************************
 *	    DOSVM_Int33Handler (WINEDOS16.151)
 *
 * Handler for int 33h (MS MOUSE).
 */
void WINAPI DOSVM_Int33Handler( CONTEXT86 *context )
{
  switch (LOWORD(context->Eax)) {
  case 0x00:
    TRACE("Reset mouse driver and request status\n");
    SET_AX( context, 0xFFFF ); /* installed */
    SET_BX( context, 3 );      /* # of buttons */
    memset( &mouse_info, 0, sizeof(mouse_info) );
    /* Set the default mickey/pixel ratio */
    mouse_info.HMPratio = 8;
    mouse_info.VMPratio = 16;
    break;
  case 0x01:
    FIXME("Show mouse cursor\n");
    break;
  case 0x02:
    FIXME("Hide mouse cursor\n");
    break;
  case 0x03:
    TRACE("Return mouse position and button status: (%ld,%ld) and %ld\n",
         mouse_info.x, mouse_info.y, mouse_info.but);
    SET_BX( context, mouse_info.but );
    SET_CX( context, mouse_info.x );
    SET_DX( context, mouse_info.y );
    break;
  case 0x04:
    FIXME("Position mouse cursor\n");
    break;
  case 0x05:
    TRACE("Return Mouse button press Information for %s mouse button\n",
          BX_reg(context) ? "right" : "left");
    if (BX_reg(context)) {
      SET_BX( context, mouse_info.rbcount );
      mouse_info.rbcount = 0;
      SET_CX( context, mouse_info.rlastx );
      SET_DX( context, mouse_info.rlasty );
    } else {
      SET_BX( context, mouse_info.lbcount );
      mouse_info.lbcount = 0;
      SET_CX( context, mouse_info.llastx );
      SET_DX( context, mouse_info.llasty );
    }
    SET_AX( context, mouse_info.but );
    break;
  case 0x07:
    FIXME("Define horizontal mouse cursor range %d..%d\n",
          CX_reg(context), DX_reg(context));
    break;
  case 0x08:
    FIXME("Define vertical mouse cursor range %d..%d\n",
          CX_reg(context), DX_reg(context));
    break;
  case 0x09:
    FIXME("Define graphics mouse cursor\n");
    break;
  case 0x0A:
    FIXME("Define text mouse cursor\n");
    break;
  case 0x0B:
    TRACE("Read Mouse motion counters\n");
    SET_CX( context, (mouse_info.x - mouse_info.oldx) * (mouse_info.HMPratio / 8) );
    SET_DX( context, (mouse_info.y - mouse_info.oldy) * (mouse_info.VMPratio / 8) );
    mouse_info.oldx = mouse_info.x;
    mouse_info.oldy = mouse_info.y;
    break;
  case 0x0C:
    TRACE("Define mouse interrupt subroutine\n");
    mouse_info.callmask = CX_reg(context);
    mouse_info.callback = (FARPROC16)MAKESEGPTR(context->SegEs, LOWORD(context->Edx));
    break;
  case 0x0F:
    TRACE("Set mickey/pixel ratio\n");
    mouse_info.HMPratio = CX_reg(context);
    mouse_info.VMPratio = DX_reg(context);
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

  if (!ISV86(&ctx))
  {
      ctx.EFlags |= V86_FLAG;
      ctx.SegSs = 0; /* Allocate new stack. */
  }

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

static void QueueMouseRelay(DWORD mx, DWORD my, WORD mask)
{
  mouse_info.x = mx;
  mouse_info.y = my;

  /* Left button down */
  if(mask & 0x02) {
    mouse_info.but |= 0x01;
    mouse_info.llastx = mx;
    mouse_info.llasty = my;
    mouse_info.lbcount++;
  }

  /* Left button up */
  if(mask & 0x04) {
    mouse_info.but &= ~0x01;
  }

  /* Right button down */
  if(mask & 0x08) {
    mouse_info.but |= 0x02;
    mouse_info.rlastx = mx;
    mouse_info.rlasty = my;
    mouse_info.rbcount++;
  }

  /* Right button up */
  if(mask & 0x10) {
    mouse_info.but &= ~0x02;
  }

  /* Middle button down */
  if(mask & 0x20) {
    mouse_info.but |= 0x04;
  }

  /* Middle button up */
  if(mask & 0x40) {
    mouse_info.but &= ~0x04;
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

void WINAPI DOSVM_Int33Message(UINT message,WPARAM wParam,LPARAM lParam)
{
  WORD mask = 0;
  unsigned Height, Width, SX=1, SY=1;

  if (!VGA_GetMode(&Height,&Width,NULL)) {
    /* may need to do some coordinate scaling */
    if (Width)
      SX = 640/Width;
    if (!SX) SX=1;
  }

  switch (message) {
  case WM_MOUSEMOVE:
    mask |= 0x01;
    break;
  case WM_LBUTTONDOWN:
  case WM_LBUTTONDBLCLK:
    mask |= 0x02;
    break;
  case WM_LBUTTONUP:
    mask |= 0x04;
    break;
  case WM_RBUTTONDOWN:
  case WM_RBUTTONDBLCLK:
    mask |= 0x08;
    break;
  case WM_RBUTTONUP:
    mask |= 0x10;
    break;
  case WM_MBUTTONDOWN:
  case WM_MBUTTONDBLCLK:
    mask |= 0x20;
    break;
  case WM_MBUTTONUP:
    mask |= 0x40;
    break;
  }

  QueueMouseRelay(LOWORD(lParam) * SX,
                 HIWORD(lParam) * SY,
                 mask);
}

void WINAPI DOSVM_Int33Console(MOUSE_EVENT_RECORD *record)
{
  unsigned Height, Width;
  WORD mask = 0;
  BOOL newLeftButton = record->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED;
  BOOL oldLeftButton = mouse_info.but & 0x01;
  BOOL newRightButton = record->dwButtonState & RIGHTMOST_BUTTON_PRESSED;
  BOOL oldRightButton = mouse_info.but & 0x02;
  BOOL newMiddleButton = record->dwButtonState & FROM_LEFT_2ND_BUTTON_PRESSED;
  BOOL oldMiddleButton = mouse_info.but & 0x04;

  if(newLeftButton && !oldLeftButton)
    mask |= 0x02;
  else if(!newLeftButton && oldLeftButton)
    mask |= 0x04;

  if(newRightButton && !oldRightButton)
    mask |= 0x08;
  else if(!newRightButton && oldRightButton)
    mask |= 0x10;

  if(newMiddleButton && !oldMiddleButton)
    mask |= 0x20;
  else if(!newMiddleButton && oldMiddleButton)
    mask |= 0x40;
  
  if (VGA_GetAlphaMode(&Width, &Height))
    QueueMouseRelay( 640 / Width * record->dwMousePosition.X,
                     200 / Height * record->dwMousePosition.Y,
                     mask );
}
