/*
 * WINDEBUG.DLL
 *
 * Copyright (c) 1997 Andreas Mohr
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
#include <string.h>
#include <stdlib.h>
#include "windef.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dll);

/***********************************************************************
 *           WinNotify       (WINDEBUG.3)
 *  written without _any_ docu
 */
void WINAPI WinNotify16(CONTEXT *context)
{
	FIXME("(AX=%04x):stub.\n", LOWORD(context->Eax));
	switch (LOWORD(context->Eax))
	{
		case 0x000D:
		case 0x000E:
		case 0x0060:	/* do nothing */
				break;
		case 0x0062:
				break;
		case 0x0063:	/* do something complicated */
				break;
		case 0x0064:	/* do something complicated */
				break;
		case 0x0065:	/* do something complicated */
				break;
		case 0x0050:	/* do something complicated, now just return error */
				context->EFlags |= 0x0001;  /* set C flag */
				break;
		case 0x0052:	/* do something complicated */
				break;
		default:
				break;
	}
}
