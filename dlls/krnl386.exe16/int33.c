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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "dosexe.h"
#include "vga.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

static struct
{
    WORD x, y, but;
    WORD lbcount, rbcount, rlastx, rlasty, llastx, llasty;
    FARPROC16 callback;
    WORD callmask;
    WORD VMPratio, HMPratio, oldx, oldy;
    WORD hide_count;
} mouse_info;


/**********************************************************************
 *          INT33_ResetMouse
 *
 * Handler for:
 * - subfunction 0x00 (reset mouse)
 * - subfunction 0x21 (software reset)
 */
static void INT33_ResetMouse( CONTEXT *context )
{
    memset( &mouse_info, 0, sizeof(mouse_info) );
    
    /* Set the default mickey/pixel ratio */
    mouse_info.HMPratio = 8;
    mouse_info.VMPratio = 16;

    /* Hide the mouse cursor */
    mouse_info.hide_count = 1;
    VGA_ShowMouse( FALSE );    

    if (context)
    {
        SET_AX( context, 0xFFFF ); /* driver installed */
        SET_BX( context, 3 );      /* number of buttons */
    }
}


/**********************************************************************
 *	    DOSVM_Int33Handler
 *
 * Handler for int 33h (MS MOUSE).
 */
void WINAPI DOSVM_Int33Handler( CONTEXT *context )
{
    switch (AX_reg(context))
    {
    case 0x0000:
        TRACE("Reset mouse driver and request status\n");
        INT33_ResetMouse( context );
        break;

    case 0x0001:
        TRACE("Show mouse cursor, old hide count: %d\n",
              mouse_info.hide_count);
        if (mouse_info.hide_count >= 1)
            mouse_info.hide_count--;
        if (!mouse_info.hide_count)
            VGA_ShowMouse( TRUE );
        break;

    case 0x0002:
        TRACE("Hide mouse cursor, old hide count: %d\n",
              mouse_info.hide_count);
        if(!mouse_info.hide_count)
            VGA_ShowMouse( FALSE );            
        mouse_info.hide_count++;
        break;

    case 0x0003:
        TRACE("Return mouse position and button status: (%d,%d) and %d\n",
              mouse_info.x, mouse_info.y, mouse_info.but);
        SET_BX( context, mouse_info.but );
        SET_CX( context, mouse_info.x );
        SET_DX( context, mouse_info.y );
        break;

    case 0x0004:
        FIXME("Position mouse cursor\n");
        break;

    case 0x0005:
        TRACE("Return Mouse button press Information for %s mouse button\n",
              BX_reg(context) ? "right" : "left");
        if (BX_reg(context)) 
        {
            SET_BX( context, mouse_info.rbcount );
            mouse_info.rbcount = 0;
            SET_CX( context, mouse_info.rlastx );
            SET_DX( context, mouse_info.rlasty );
        } 
        else 
        {
            SET_BX( context, mouse_info.lbcount );
            mouse_info.lbcount = 0;
            SET_CX( context, mouse_info.llastx );
            SET_DX( context, mouse_info.llasty );
        }
        SET_AX( context, mouse_info.but );
        break;

    case 0x0007:
        FIXME("Define horizontal mouse cursor range %d..%d\n",
              CX_reg(context), DX_reg(context));
        break;

    case 0x0008:
        FIXME("Define vertical mouse cursor range %d..%d\n",
              CX_reg(context), DX_reg(context));
        break;

    case 0x0009:
        FIXME("Define graphics mouse cursor\n");
        break;

    case 0x000A:
        FIXME("Define text mouse cursor\n");
        break;

    case 0x000B:
        TRACE("Read Mouse motion counters\n");
        {
            int dx = ((int)mouse_info.x - (int)mouse_info.oldx)
                * (mouse_info.HMPratio / 8);
            int dy = ((int)mouse_info.y - (int)mouse_info.oldy)
                * (mouse_info.VMPratio / 8);

            SET_CX( context, (WORD)dx );
            SET_DX( context, (WORD)dy );

            mouse_info.oldx = mouse_info.x;
            mouse_info.oldy = mouse_info.y;
        }
        break;

    case 0x000C:
        TRACE("Define mouse interrupt subroutine\n");
        mouse_info.callmask = CX_reg(context);
        mouse_info.callback = (FARPROC16)MAKESEGPTR(context->SegEs, 
                                                    DX_reg(context));
        break;

    case 0x000F:
        TRACE("Set mickey/pixel ratio\n");
        mouse_info.HMPratio = CX_reg(context);
        mouse_info.VMPratio = DX_reg(context);
        break;

    case 0x0010:
        FIXME("Define screen region for update\n");
        break;

    case 0x0015:
        TRACE("Get mouse driver state and memory requirements\n");
        SET_BX(context, sizeof(mouse_info));
        break;

    case 0x0021:
        TRACE("Software reset\n");
        INT33_ResetMouse( context );
        break;

    default:
        INT_BARF(context,0x33);
    }
}

typedef struct {
  FARPROC16 proc;
  WORD mask,but,x,y,mx,my;
} MCALLDATA;

static void MouseRelay(CONTEXT *context,void *mdata)
{
  MCALLDATA *data = mdata;
  CONTEXT ctx = *context;

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
  HeapFree(GetProcessHeap(), 0, data);
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
    MCALLDATA *data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MCALLDATA));
    data->proc = mouse_info.callback;
    data->mask = mask & mouse_info.callmask;
    data->but = mouse_info.but;
    data->x = mouse_info.x;
    data->y = mouse_info.y;

    /*
     * Fake mickeys. 
     *
     * FIXME: This is not entirely correct. If mouse if moved to the edge
     *        of the screen, mouse will stop moving and mickeys won't
     *        be updated even though they should be.
     */
    data->mx = mouse_info.x * (mouse_info.HMPratio / 8);
    data->my = mouse_info.y * (mouse_info.VMPratio / 8);

    DOSVM_QueueEvent(-1, DOS_PRIORITY_MOUSE, MouseRelay, data);
  }
}

void DOSVM_Int33Message(UINT message,WPARAM wParam,LPARAM lParam)
{
  WORD mask = 0;
  unsigned Height, Width, SX=1, SY=1;

  if (VGA_GetMode(&Height, &Width, NULL)) {
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

void DOSVM_Int33Console(MOUSE_EVENT_RECORD *record)
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
