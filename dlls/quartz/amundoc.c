/*
 * Copyright (C) Hidenori TAKESHIMA
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

#include <math.h>
#include "windef.h"
#include "wine/obj_base.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"

/***********************************************************************
 *		AmpFactorToDB (QUARTZ.@)
 *
 *	undocumented.
 *  converting from Amp to dB?
 *
 */
LONG WINAPI QUARTZ_AmpFactorToDB( LONG amp )
{
	LONG	dB;

	TRACE( "(%ld)\n", amp );

	if ( amp <= 0 || amp > 65536 )
		return 0;

	dB = (LONG)(2000.0 * log10((double)amp / 65536.0) + 0.5);
	if ( dB >= 0 ) dB = 0;
	if ( dB < -10000 ) dB = -10000;

	return dB;
}

/***********************************************************************
 *		DBToAmpFactor (QUARTZ.@)
 *
 *	undocumented.
 *  converting from dB to Amp?
 */
LONG WINAPI QUARTZ_DBToAmpFactor( LONG dB )
{
	LONG	amp;

	TRACE( "(%ld)\n", dB );

	if ( dB >= 0 )
		return 65535;
	if ( dB < -10000 )
		return 0;

	amp = (LONG)(pow(10.0,dB / 2000.0) * 65536.0 + 0.5);
	if ( amp <= 0 ) amp = 1;
	if ( amp >= 65536 ) amp = 65535;

	return amp;
}

