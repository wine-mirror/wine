/*
 * DOS interrupt 29h handler
 *
 * Copyright 1998 Joseph Pranevich
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

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#include "miscemu.h"
#include "dosexe.h"

/**********************************************************************
 *	    DOSVM_Int29Handler (WINEDOS16.141)
 *
 * Handler for int 29h (fast console output)
 */
void WINAPI DOSVM_Int29Handler( CONTEXT86 *context )
{
   /* Yes, it seems that this is really all this interrupt does. */
   DOSVM_PutChar(AL_reg(context));
}
