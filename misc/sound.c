/*
static char RCSId[] = "$Id: heap.c,v 1.3 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";
*/

#include <stdlib.h>
#include <stdio.h>
#include "windows.h"

INT16 OpenSound16(void)
{
  printf("OpenSound16()\n");
  return -1;
}

void OpenSound32(void)
{
  printf("OpenSound32()\n");
}

void CloseSound(void)
{
  printf("CloseSound()\n");
}

INT16 SetVoiceQueueSize16(INT16 nVoice, INT16 nBytes)
{
  printf("SetVoiceQueueSize16 (%d,%d)\n",nVoice,nBytes);
  return 0;
}

DWORD SetVoiceQueueSize32(DWORD nVoice, DWORD nBytes)
{
  printf("SetVoiceQueueSize32 (%ld,%ld)\n",nVoice,nBytes);
  return 0;
}

INT16 SetVoiceNote16(INT16 nVoice, INT16 nValue, INT16 nLength, INT16 nCdots)
{
  printf("SetVoiceNote16 (%d,%d,%d,%d)\n",nVoice,nValue,nLength,nCdots);
  return 0;
}

DWORD SetVoiceNote32(DWORD nVoice, DWORD nValue, DWORD nLength, DWORD nCdots)
{
  printf("SetVoiceNote32 (%ld,%ld,%ld,%ld)\n",nVoice,nValue,nLength,nCdots);
  return 0;
}

INT16 SetVoiceAccent16(INT16 nVoice, INT16 nTempo, INT16 nVolume,
                       INT16 nMode, INT16 nPitch)
{
  printf("SetVoiceAccent16(%d,%d,%d,%d,%d)\n", nVoice, nTempo, 
	 nVolume, nMode, nPitch);
  return 0;
}

DWORD SetVoiceAccent32(DWORD nVoice, DWORD nTempo, DWORD nVolume,
                       DWORD nMode, DWORD nPitch)
{
  printf("SetVoiceAccent32(%ld,%ld,%ld,%ld,%ld)\n", nVoice, nTempo, 
	 nVolume, nMode, nPitch);
  return 0;
}

INT16 SetVoiceEnvelope16(INT16 nVoice, INT16 nShape, INT16 nRepeat)
{
  printf("SetVoiceEnvelope16(%d,%d,%d)\n",nVoice,nShape,nRepeat);
  return 0;
}

DWORD SetVoiceEnvelope32(DWORD nVoice, DWORD nShape, DWORD nRepeat)
{
  printf("SetVoiceEnvelope32(%ld,%ld,%ld)\n",nVoice,nShape,nRepeat);
  return 0;
}

INT16 SetSoundNoise16(INT16 nSource, INT16 nDuration)
{
  printf("SetSoundNoise16(%d,%d)\n",nSource,nDuration);
  return 0;
}

DWORD SetSoundNoise32(DWORD nSource, DWORD nDuration)
{
  printf("SetSoundNoise32(%ld,%ld)\n",nSource,nDuration);
  return 0;
}

INT16 SetVoiceSound16(INT16 nVoice, DWORD lFrequency, INT16 nDuration)
{
  printf("SetVoiceSound16(%d, %ld, %d)\n",nVoice,lFrequency, nDuration);
  return 0;
}

DWORD SetVoiceSound32(DWORD nVoice, DWORD lFrequency, DWORD nDuration)
{
  printf("SetVoiceSound32(%ld, %ld, %ld)\n",nVoice,lFrequency, nDuration);
  return 0;
}

INT16 StartSound16(void)
{
  return 0;
}

INT16 StopSound16(void)
{
  return 0;
}

INT16 WaitSoundState16(INT16 x)
{
    fprintf(stderr, "WaitSoundState16(%d)\n", x);
    return 0;
}

DWORD WaitSoundState32(DWORD x)
{
    fprintf(stderr, "WaitSoundState32(%ld)\n", x);
    return 0;
}

INT16 SyncAllVoices16(void)
{
    fprintf(stderr, "SyncAllVoices16()\n");
    return 0;
}

DWORD SyncAllVoices32(void)
{
    fprintf(stderr, "SyncAllVoices32()\n");
    return 0;
}

INT16 CountVoiceNotes16(INT16 x)
{
    fprintf(stderr, "CountVoiceNotes16(%d)\n", x);
    return 0;
}

DWORD CountVoiceNotes32(DWORD x)
{
    fprintf(stderr, "CountVoiceNotes32(%ld)\n", x);
    return 0;
}

LPINT16 GetThresholdEvent16(void)
{
    fprintf(stderr, "GetThresholdEvent16()\n");
    return NULL;
}

LPDWORD GetThresholdEvent32(void)
{
    fprintf(stderr, "GetThresholdEvent32()\n");
    return NULL;
}

INT16 GetThresholdStatus16(void)
{
    fprintf(stderr, "GetThresholdStatus16()\n");
    return 0;
}

DWORD GetThresholdStatus32(void)
{
    fprintf(stderr, "GetThresholdStatus32()\n");
    return 0;
}

INT16 SetVoiceThreshold16(INT16 a, INT16 b)
{
	fprintf(stderr, "SetVoiceThreshold16(%d,%d)\n", a, b);
        return 0;
}

DWORD SetVoiceThreshold32(DWORD a, DWORD b)
{
    fprintf(stderr, "SetVoiceThreshold32(%ld,%ld)\n", a, b);
    return 0;
}

void DoBeep(void)
{
	fprintf(stderr, "BEEP!\n");
}
