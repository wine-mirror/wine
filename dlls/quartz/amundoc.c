#include "config.h"

#include <math.h>
#include "windef.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

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

	FIXME( "(%08ld): undocumented API.\n", amp );

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

	FIXME( "(%08ld): undocumented API.\n", dB );

	if ( dB >= 0 )
		return 65535;
	if ( dB < -10000 )
		return 0;

	amp = (LONG)(pow(10.0,dB / 2000.0) * 65536.0 + 0.5);
	if ( amp <= 0 ) amp = 1;
	if ( amp >= 65536 ) amp = 65535;

	return amp;
}

