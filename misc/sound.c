/*
 *  Copyright  Robert J. Amstadt, 1993
 */

#include <stdlib.h>
#include "windef.h"
#include "wine/winesound.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(sound)

INT16 WINAPI OpenSound16(void)
{
  FIXME("(void): stub\n");
  return -1;
}

void WINAPI OpenSound(void)
{
  FIXME("(void): stub\n");
}

void WINAPI CloseSound16(void)
{
  FIXME("(void): stub\n");
}

INT16 WINAPI SetVoiceQueueSize16(INT16 nVoice, INT16 nBytes)
{
  FIXME("(%d,%d): stub\n",nVoice,nBytes);
  return 0;
}

DWORD WINAPI SetVoiceQueueSize(DWORD nVoice, DWORD nBytes)
{
  FIXME("(%ld,%ld): stub\n",nVoice,nBytes);
  return 0;
}

INT16 WINAPI SetVoiceNote16(INT16 nVoice, INT16 nValue, INT16 nLength,
                            INT16 nCdots)
{
  FIXME("(%d,%d,%d,%d): stub\n",nVoice,nValue,nLength,nCdots);
  return 0;
}

DWORD WINAPI SetVoiceNote(DWORD nVoice, DWORD nValue, DWORD nLength,
                            DWORD nCdots)
{
  FIXME("(%ld,%ld,%ld,%ld): stub\n",nVoice,nValue,nLength,nCdots);
  return 0;
}

INT16 WINAPI SetVoiceAccent16(INT16 nVoice, INT16 nTempo, INT16 nVolume,
                              INT16 nMode, INT16 nPitch)
{
  FIXME("(%d,%d,%d,%d,%d): stub\n", nVoice, nTempo, 
	nVolume, nMode, nPitch);
  return 0;
}

DWORD WINAPI SetVoiceAccent(DWORD nVoice, DWORD nTempo, DWORD nVolume,
                              DWORD nMode, DWORD nPitch)
{
  FIXME("(%ld,%ld,%ld,%ld,%ld): stub\n", nVoice, nTempo, 
	nVolume, nMode, nPitch);
  return 0;
}

INT16 WINAPI SetVoiceEnvelope16(INT16 nVoice, INT16 nShape, INT16 nRepeat)
{
  FIXME("(%d,%d,%d): stub\n",nVoice,nShape,nRepeat);
  return 0;
}

DWORD WINAPI SetVoiceEnvelope(DWORD nVoice, DWORD nShape, DWORD nRepeat)
{
  FIXME("(%ld,%ld,%ld): stub\n",nVoice,nShape,nRepeat);
  return 0;
}

INT16 WINAPI SetSoundNoise16(INT16 nSource, INT16 nDuration)
{
  FIXME("(%d,%d): stub\n",nSource,nDuration);
  return 0;
}

DWORD WINAPI SetSoundNoise(DWORD nSource, DWORD nDuration)
{
  FIXME("(%ld,%ld): stub\n",nSource,nDuration);
  return 0;
}

INT16 WINAPI SetVoiceSound16(INT16 nVoice, DWORD lFrequency, INT16 nDuration)
{
  FIXME("(%d, %ld, %d): stub\n",nVoice,lFrequency, nDuration);
  return 0;
}

DWORD WINAPI SetVoiceSound(DWORD nVoice, DWORD lFrequency, DWORD nDuration)
{
  FIXME("(%ld, %ld, %ld): stub\n",nVoice,lFrequency, nDuration);
  return 0;
}

INT16 WINAPI StartSound16(void)
{
  return 0;
}

INT16 WINAPI StopSound16(void)
{
  return 0;
}

INT16 WINAPI WaitSoundState16(INT16 x)
{
    FIXME("(%d): stub\n", x);
    return 0;
}

DWORD WINAPI WaitSoundState(DWORD x)
{
    FIXME("(%ld): stub\n", x);
    return 0;
}

INT16 WINAPI SyncAllVoices16(void)
{
    FIXME("(void): stub\n");
    return 0;
}

DWORD WINAPI SyncAllVoices(void)
{
    FIXME("(void): stub\n");
    return 0;
}

INT16 WINAPI CountVoiceNotes16(INT16 x)
{
    FIXME("(%d): stub\n", x);
    return 0;
}

DWORD WINAPI CountVoiceNotes(DWORD x)
{
    FIXME("(%ld): stub\n", x);
    return 0;
}

LPINT16 WINAPI GetThresholdEvent16(void)
{
    FIXME("(void): stub\n");
    return NULL;
}

LPDWORD WINAPI GetThresholdEvent(void)
{
    FIXME("(void): stub\n");
    return NULL;
}

INT16 WINAPI GetThresholdStatus16(void)
{
    FIXME("(void): stub\n");
    return 0;
}

DWORD WINAPI GetThresholdStatus(void)
{
    FIXME("(void): stub\n");
    return 0;
}

INT16 WINAPI SetVoiceThreshold16(INT16 a, INT16 b)
{
    FIXME("(%d,%d): stub\n", a, b);
    return 0;
}

DWORD WINAPI SetVoiceThreshold(DWORD a, DWORD b)
{
    FIXME("(%ld,%ld): stub\n", a, b);
    return 0;
}

void WINAPI DoBeep16(void)
{
    FIXME("(void): stub!\n");
}




