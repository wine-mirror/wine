/*
 * hidenori@a2.ctktv.ne.jp
 */

#include "config.h"

#include "windef.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

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


