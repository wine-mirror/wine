/*
 * 16-bit sound support
 *
 *  Copyright  Robert J. Amstadt, 1993
 */

#include <stdlib.h>
#include "windef.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(sound);

/***********************************************************************
 *		OpenSound16
 */
INT16 WINAPI OpenSound16(void)
{
  FIXME("(void): stub\n");
  return -1;
}

/***********************************************************************
 *		CloseSound16
 */
void WINAPI CloseSound16(void)
{
  FIXME("(void): stub\n");
}

/***********************************************************************
 *		SetVoiceQueueSize16
 */
INT16 WINAPI SetVoiceQueueSize16(INT16 nVoice, INT16 nBytes)
{
  FIXME("(%d,%d): stub\n",nVoice,nBytes);
  return 0;
}

/***********************************************************************
 *		SetVoiceNote16
 */
INT16 WINAPI SetVoiceNote16(INT16 nVoice, INT16 nValue, INT16 nLength,
                            INT16 nCdots)
{
  FIXME("(%d,%d,%d,%d): stub\n",nVoice,nValue,nLength,nCdots);
  return 0;
}

/***********************************************************************
 *		SetVoiceAccent16
 */
INT16 WINAPI SetVoiceAccent16(INT16 nVoice, INT16 nTempo, INT16 nVolume,
                              INT16 nMode, INT16 nPitch)
{
  FIXME("(%d,%d,%d,%d,%d): stub\n", nVoice, nTempo, 
	nVolume, nMode, nPitch);
  return 0;
}

/***********************************************************************
 *		SetVoiceEnvelope16
 */
INT16 WINAPI SetVoiceEnvelope16(INT16 nVoice, INT16 nShape, INT16 nRepeat)
{
  FIXME("(%d,%d,%d): stub\n",nVoice,nShape,nRepeat);
  return 0;
}

/***********************************************************************
 *		SetSoundNoise16
 */
INT16 WINAPI SetSoundNoise16(INT16 nSource, INT16 nDuration)
{
  FIXME("(%d,%d): stub\n",nSource,nDuration);
  return 0;
}

/***********************************************************************
 *		SetVoiceSound16
 */
INT16 WINAPI SetVoiceSound16(INT16 nVoice, DWORD lFrequency, INT16 nDuration)
{
  FIXME("(%d, %ld, %d): stub\n",nVoice,lFrequency, nDuration);
  return 0;
}

/***********************************************************************
 *		StartSound16
 */
INT16 WINAPI StartSound16(void)
{
  return 0;
}

/***********************************************************************
 *		StopSound16
 */
INT16 WINAPI StopSound16(void)
{
  return 0;
}

/***********************************************************************
 *		WaitSoundState16
 */
INT16 WINAPI WaitSoundState16(INT16 x)
{
    FIXME("(%d): stub\n", x);
    return 0;
}

/***********************************************************************
 *		SyncAllVoices16
 */
INT16 WINAPI SyncAllVoices16(void)
{
    FIXME("(void): stub\n");
    return 0;
}

/***********************************************************************
 *		CountVoiceNotes16
 */
INT16 WINAPI CountVoiceNotes16(INT16 x)
{
    FIXME("(%d): stub\n", x);
    return 0;
}

/***********************************************************************
 *		GetThresholdEvent16
 */
LPINT16 WINAPI GetThresholdEvent16(void)
{
    FIXME("(void): stub\n");
    return NULL;
}

/***********************************************************************
 *		GetThresholdStatus16
 */
INT16 WINAPI GetThresholdStatus16(void)
{
    FIXME("(void): stub\n");
    return 0;
}

/***********************************************************************
 *		SetVoiceThreshold16
 */
INT16 WINAPI SetVoiceThreshold16(INT16 a, INT16 b)
{
    FIXME("(%d,%d): stub\n", a, b);
    return 0;
}

/***********************************************************************
 *		DoBeep16
 */
void WINAPI DoBeep16(void)
{
    FIXME("(void): stub!\n");
}




