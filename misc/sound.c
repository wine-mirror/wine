/*
 *  Copyright  Robert J. Amstadt, 1993
 */

#include <stdlib.h>
#include "windows.h"
#include "debug.h"

INT16 WINAPI OpenSound16(void)
{
  FIXME(sound, "(void): stub\n");
  return -1;
}

void WINAPI OpenSound32(void)
{
  FIXME(sound, "(void): stub\n");
}

void WINAPI CloseSound(void)
{
  FIXME(sound, "(void): stub\n");
}

INT16 WINAPI SetVoiceQueueSize16(INT16 nVoice, INT16 nBytes)
{
  FIXME(sound, "(%d,%d): stub\n",nVoice,nBytes);
  return 0;
}

DWORD WINAPI SetVoiceQueueSize32(DWORD nVoice, DWORD nBytes)
{
  FIXME(sound, "(%ld,%ld): stub\n",nVoice,nBytes);
  return 0;
}

INT16 WINAPI SetVoiceNote16(INT16 nVoice, INT16 nValue, INT16 nLength,
                            INT16 nCdots)
{
  FIXME(sound, "(%d,%d,%d,%d): stub\n",nVoice,nValue,nLength,nCdots);
  return 0;
}

DWORD WINAPI SetVoiceNote32(DWORD nVoice, DWORD nValue, DWORD nLength,
                            DWORD nCdots)
{
  FIXME(sound, "(%ld,%ld,%ld,%ld): stub\n",nVoice,nValue,nLength,nCdots);
  return 0;
}

INT16 WINAPI SetVoiceAccent16(INT16 nVoice, INT16 nTempo, INT16 nVolume,
                              INT16 nMode, INT16 nPitch)
{
  FIXME(sound, "(%d,%d,%d,%d,%d): stub\n", nVoice, nTempo, 
	nVolume, nMode, nPitch);
  return 0;
}

DWORD WINAPI SetVoiceAccent32(DWORD nVoice, DWORD nTempo, DWORD nVolume,
                              DWORD nMode, DWORD nPitch)
{
  FIXME(sound, "(%ld,%ld,%ld,%ld,%ld): stub\n", nVoice, nTempo, 
	nVolume, nMode, nPitch);
  return 0;
}

INT16 WINAPI SetVoiceEnvelope16(INT16 nVoice, INT16 nShape, INT16 nRepeat)
{
  FIXME(sound, "(%d,%d,%d): stub\n",nVoice,nShape,nRepeat);
  return 0;
}

DWORD WINAPI SetVoiceEnvelope32(DWORD nVoice, DWORD nShape, DWORD nRepeat)
{
  FIXME(sound, "(%ld,%ld,%ld): stub\n",nVoice,nShape,nRepeat);
  return 0;
}

INT16 WINAPI SetSoundNoise16(INT16 nSource, INT16 nDuration)
{
  FIXME(sound, "(%d,%d): stub\n",nSource,nDuration);
  return 0;
}

DWORD WINAPI SetSoundNoise32(DWORD nSource, DWORD nDuration)
{
  FIXME(sound, "(%ld,%ld): stub\n",nSource,nDuration);
  return 0;
}

INT16 WINAPI SetVoiceSound16(INT16 nVoice, DWORD lFrequency, INT16 nDuration)
{
  FIXME(sound, "(%d, %ld, %d): stub\n",nVoice,lFrequency, nDuration);
  return 0;
}

DWORD WINAPI SetVoiceSound32(DWORD nVoice, DWORD lFrequency, DWORD nDuration)
{
  FIXME(sound, "(%ld, %ld, %ld): stub\n",nVoice,lFrequency, nDuration);
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
    FIXME(sound, "(%d): stub\n", x);
    return 0;
}

DWORD WINAPI WaitSoundState32(DWORD x)
{
    FIXME(sound, "(%ld): stub\n", x);
    return 0;
}

INT16 WINAPI SyncAllVoices16(void)
{
    FIXME(sound, "(void): stub\n");
    return 0;
}

DWORD WINAPI SyncAllVoices32(void)
{
    FIXME(sound, "(void): stub\n");
    return 0;
}

INT16 WINAPI CountVoiceNotes16(INT16 x)
{
    FIXME(sound, "(%d): stub\n", x);
    return 0;
}

DWORD WINAPI CountVoiceNotes32(DWORD x)
{
    FIXME(sound, "(%ld): stub\n", x);
    return 0;
}

LPINT16 WINAPI GetThresholdEvent16(void)
{
    FIXME(sound, "(void): stub\n");
    return NULL;
}

LPDWORD WINAPI GetThresholdEvent32(void)
{
    FIXME(sound, "(void): stub\n");
    return NULL;
}

INT16 WINAPI GetThresholdStatus16(void)
{
    FIXME(sound, "(void): stub\n");
    return 0;
}

DWORD WINAPI GetThresholdStatus32(void)
{
    FIXME(sound, "(void): stub\n");
    return 0;
}

INT16 WINAPI SetVoiceThreshold16(INT16 a, INT16 b)
{
    FIXME(sound, "(%d,%d): stub\n", a, b);
    return 0;
}

DWORD WINAPI SetVoiceThreshold32(DWORD a, DWORD b)
{
    FIXME(sound, "(%ld,%ld): stub\n", a, b);
    return 0;
}

void WINAPI DoBeep(void)
{
    FIXME(sound, "(void): stub!\n");
}




