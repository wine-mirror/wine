/*
static char RCSId[] = "$Id: heap.c,v 1.3 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";
*/

#include <stdlib.h>
#include <stdio.h>
#include "windows.h"

INT16 WINAPI OpenSound16(void)
{
  printf("OpenSound16()\n");
  return -1;
}

void WINAPI OpenSound32(void)
{
  printf("OpenSound32()\n");
}

void WINAPI CloseSound(void)
{
  printf("CloseSound()\n");
}

INT16 WINAPI SetVoiceQueueSize16(INT16 nVoice, INT16 nBytes)
{
  printf("SetVoiceQueueSize16 (%d,%d)\n",nVoice,nBytes);
  return 0;
}

DWORD WINAPI SetVoiceQueueSize32(DWORD nVoice, DWORD nBytes)
{
  printf("SetVoiceQueueSize32 (%ld,%ld)\n",nVoice,nBytes);
  return 0;
}

INT16 WINAPI SetVoiceNote16(INT16 nVoice, INT16 nValue, INT16 nLength,
                            INT16 nCdots)
{
  printf("SetVoiceNote16 (%d,%d,%d,%d)\n",nVoice,nValue,nLength,nCdots);
  return 0;
}

DWORD WINAPI SetVoiceNote32(DWORD nVoice, DWORD nValue, DWORD nLength,
                            DWORD nCdots)
{
  printf("SetVoiceNote32 (%ld,%ld,%ld,%ld)\n",nVoice,nValue,nLength,nCdots);
  return 0;
}

INT16 WINAPI SetVoiceAccent16(INT16 nVoice, INT16 nTempo, INT16 nVolume,
                              INT16 nMode, INT16 nPitch)
{
  printf("SetVoiceAccent16(%d,%d,%d,%d,%d)\n", nVoice, nTempo, 
	 nVolume, nMode, nPitch);
  return 0;
}

DWORD WINAPI SetVoiceAccent32(DWORD nVoice, DWORD nTempo, DWORD nVolume,
                              DWORD nMode, DWORD nPitch)
{
  printf("SetVoiceAccent32(%ld,%ld,%ld,%ld,%ld)\n", nVoice, nTempo, 
	 nVolume, nMode, nPitch);
  return 0;
}

INT16 WINAPI SetVoiceEnvelope16(INT16 nVoice, INT16 nShape, INT16 nRepeat)
{
  printf("SetVoiceEnvelope16(%d,%d,%d)\n",nVoice,nShape,nRepeat);
  return 0;
}

DWORD WINAPI SetVoiceEnvelope32(DWORD nVoice, DWORD nShape, DWORD nRepeat)
{
  printf("SetVoiceEnvelope32(%ld,%ld,%ld)\n",nVoice,nShape,nRepeat);
  return 0;
}

INT16 WINAPI SetSoundNoise16(INT16 nSource, INT16 nDuration)
{
  printf("SetSoundNoise16(%d,%d)\n",nSource,nDuration);
  return 0;
}

DWORD WINAPI SetSoundNoise32(DWORD nSource, DWORD nDuration)
{
  printf("SetSoundNoise32(%ld,%ld)\n",nSource,nDuration);
  return 0;
}

INT16 WINAPI SetVoiceSound16(INT16 nVoice, DWORD lFrequency, INT16 nDuration)
{
  printf("SetVoiceSound16(%d, %ld, %d)\n",nVoice,lFrequency, nDuration);
  return 0;
}

DWORD WINAPI SetVoiceSound32(DWORD nVoice, DWORD lFrequency, DWORD nDuration)
{
  printf("SetVoiceSound32(%ld, %ld, %ld)\n",nVoice,lFrequency, nDuration);
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
    fprintf(stderr, "WaitSoundState16(%d)\n", x);
    return 0;
}

DWORD WINAPI WaitSoundState32(DWORD x)
{
    fprintf(stderr, "WaitSoundState32(%ld)\n", x);
    return 0;
}

INT16 WINAPI SyncAllVoices16(void)
{
    fprintf(stderr, "SyncAllVoices16()\n");
    return 0;
}

DWORD WINAPI SyncAllVoices32(void)
{
    fprintf(stderr, "SyncAllVoices32()\n");
    return 0;
}

INT16 WINAPI CountVoiceNotes16(INT16 x)
{
    fprintf(stderr, "CountVoiceNotes16(%d)\n", x);
    return 0;
}

DWORD WINAPI CountVoiceNotes32(DWORD x)
{
    fprintf(stderr, "CountVoiceNotes32(%ld)\n", x);
    return 0;
}

LPINT16 WINAPI GetThresholdEvent16(void)
{
    fprintf(stderr, "GetThresholdEvent16()\n");
    return NULL;
}

LPDWORD WINAPI GetThresholdEvent32(void)
{
    fprintf(stderr, "GetThresholdEvent32()\n");
    return NULL;
}

INT16 WINAPI GetThresholdStatus16(void)
{
    fprintf(stderr, "GetThresholdStatus16()\n");
    return 0;
}

DWORD WINAPI GetThresholdStatus32(void)
{
    fprintf(stderr, "GetThresholdStatus32()\n");
    return 0;
}

INT16 WINAPI SetVoiceThreshold16(INT16 a, INT16 b)
{
	fprintf(stderr, "SetVoiceThreshold16(%d,%d)\n", a, b);
        return 0;
}

DWORD WINAPI SetVoiceThreshold32(DWORD a, DWORD b)
{
    fprintf(stderr, "SetVoiceThreshold32(%ld,%ld)\n", a, b);
    return 0;
}

void WINAPI DoBeep(void)
{
	fprintf(stderr, "BEEP!\n");
}
