/*
 * Copyright (C) Hidenori TAKESHIMA <hidenori@a2.ctktv.ne.jp>
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

#include "windef.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(quartz);

#include "audioutl.h"


void AUDIOUTL_ChangeSign8( BYTE* pbData, DWORD cbData )
{
	BYTE*	pbEnd = pbData + cbData;

	while ( pbData < pbEnd )
	{
		*pbData ^= 0x80;
		pbData ++;
	}
}

void AUDIOUTL_ChangeSign16LE( BYTE* pbData, DWORD cbData )
{
	BYTE*	pbEnd = pbData + cbData;

	pbData ++;
	while ( pbData < pbEnd )
	{
		*pbData ^= 0x80;
		pbData += 2;
	}
}

void AUDIOUTL_ChangeSign16BE( BYTE* pbData, DWORD cbData )
{
	BYTE*	pbEnd = pbData + cbData;

	while ( pbData < pbEnd )
	{
		*pbData ^= 0x80;
		pbData += 2;
	}
}

void AUDIOUTL_ByteSwap( BYTE* pbData, DWORD cbData )
{
	BYTE*	pbEnd = pbData + cbData - 1;
	BYTE	bTemp;

	while ( pbData < pbEnd )
	{
		bTemp = pbData[0];
		pbData[0] = pbData[1];
		pbData[1] = bTemp;
		pbData += 2;
	}
}


