/*
 * 16-bit sound support
 *
 *  Copyright  Robert J. Amstadt, 1993
 */

#include <stdlib.h>
#include "windef.h"
#include "wine/windef16.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(sound);

/***********************************************************************
 *		OpenSound16 (SOUND.1)
 */
INT16 WINAPI OpenSound16(void)
{
  FIXME("(void): stub\n");
  return -1;
}

/***********************************************************************
 *		CloseSound16 (SOUND.2)
 */
void WINAPI CloseSound16(void)
{
  FIXME("(void): stub\n");
}

/***********************************************************************
 *		SetVoiceQueueSize16 (SOUND.3)
 */
INT16 WINAPI SetVoiceQueueSize16(INT16 nVoice, INT16 nBytes)
{
  FIXME("(%d,%d): stub\n",nVoice,nBytes);
  return 0;
}

/***********************************************************************
 *		SetVoiceNote16 (SOUND.4)
 */
INT16 WINAPI SetVoiceNote16(INT16 nVoice, INT16 nValue, INT16 nLength,
                            INT16 nCdots)
{
  FIXME("(%d,%d,%d,%d): stub\n",nVoice,nValue,nLength,nCdots);
  return 0;
}

/***********************************************************************
 *		SetVoiceAccent16 (SOUND.5)
 */
INT16 WINAPI SetVoiceAccent16(INT16 nVoice, INT16 nTempo, INT16 nVolume,
                              INT16 nMode, INT16 nPitch)
{
  FIXME("(%d,%d,%d,%d,%d): stub\n", nVoice, nTempo, 
	nVolume, nMode, nPitch);
  return 0;
}

/***********************************************************************
 *		SetVoiceEnvelope16 (SOUND.6)
 */
INT16 WINAPI SetVoiceEnvelope16(INT16 nVoice, INT16 nShape, INT16 nRepeat)
{
  FIXME("(%d,%d,%d): stub\n",nVoice,nShape,nRepeat);
  return 0;
}

/***********************************************************************
 *		SetSoundNoise16 (SOUND.7)
 */
INT16 WINAPI SetSoundNoise16(INT16 nSource, INT16 nDuration)
{
  FIXME("(%d,%d): stub\n",nSource,nDuration);
  return 0;
}

/***********************************************************************
 *		SetVoiceSound16 (SOUND.8)
 */
INT16 WINAPI SetVoiceSound16(INT16 nVoice, DWORD lFrequency, INT16 nDuration)
{
  FIXME("(%d, %ld, %d): stub\n",nVoice,lFrequency, nDuration);
  return 0;
}

/***********************************************************************
 *		StartSound16 (SOUND.9)
 */
INT16 WINAPI StartSound16(void)
{
  return 0;
}

/***********************************************************************
 *		StopSound16 (SOUND.10)
 */
INT16 WINAPI StopSound16(void)
{
  return 0;
}

/***********************************************************************
 *		WaitSoundState16 (SOUND.11)
 */
INT16 WINAPI WaitSoundState16(INT16 x)
{
    FIXME("(%d): stub\n", x);
    return 0;
}

/***********************************************************************
 *		SyncAllVoices16 (SOUND.12)
 */
INT16 WINAPI SyncAllVoices16(void)
{
    FIXME("(void): stub\n");
    return 0;
}

/***********************************************************************
 *		CountVoiceNotes16 (SOUND.13)
 */
INT16 WINAPI CountVoiceNotes16(INT16 x)
{
    FIXME("(%d): stub\n", x);
    return 0;
}

/***********************************************************************
 *		GetThresholdEvent16 (SOUND.14)
 */
LPINT16 WINAPI GetThresholdEvent16(void)
{
    FIXME("(void): stub\n");
    return NULL;
}

/***********************************************************************
 *		GetThresholdStatus16 (SOUND.15)
 */
INT16 WINAPI GetThresholdStatus16(void)
{
    FIXME("(void): stub\n");
    return 0;
}

/***********************************************************************
 *		SetVoiceThreshold16 (SOUND.16)
 */
INT16 WINAPI SetVoiceThreshold16(INT16 a, INT16 b)
{
    FIXME("(%d,%d): stub\n", a, b);
    return 0;
}

/***********************************************************************
 *		DoBeep16 (SOUND.17)
 */
void WINAPI DoBeep16(void)
{
    FIXME("(void): stub!\n");
}




