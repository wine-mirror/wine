/*
 * Copyright 2002 Uwe Bonnes
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

#include "winbase.h"
#include "winerror.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msimg32);

BOOL WINAPI GradientFill(HDC hdc, void * vert_array, ULONG nvert, 
		    void * grad_array, ULONG ngrad, ULONG mode)
{
  FIXME("stub: %ld vertices %ld gradients mode %lx\n", nvert, ngrad, mode);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
