#ifndef _WINE_SOUND_H
#define _WINE_SOUND_H

#include "wintypes.h"

VOID        WINAPI CloseSound(VOID);
INT16       WINAPI CountVoiceNotes16(INT16);
DWORD       WINAPI CountVoiceNotes32(DWORD);
#define     CountVoiceNotes WINELIB_NAME(CountVoiceNotes)
LPINT16     WINAPI GetThresholdEvent16(void);
LPDWORD     WINAPI GetThresholdEvent32(void);
#define     GetThresholdEvent WINELIB_NAME(GetThresholdEvent)
INT16       WINAPI GetThresholdStatus16(void);
DWORD       WINAPI GetThresholdStatus32(void);
#define     GetThresholdStatus WINELIB_NAME(GetThresholdStatus)
INT16       WINAPI OpenSound16(void);
VOID        WINAPI OpenSound32(void);
#define     OpenSound WINELIB_NAME(OpenSound)
INT16       WINAPI SetSoundNoise16(INT16,INT16);
DWORD       WINAPI SetSoundNoise32(DWORD,DWORD);
#define     SetSoundNoise WINELIB_NAME(SetSoundNoise)
INT16       WINAPI SetVoiceAccent16(INT16,INT16,INT16,INT16,INT16);
DWORD       WINAPI SetVoiceAccent32(DWORD,DWORD,DWORD,DWORD,DWORD);
#define     SetVoiceAccent WINELIB_NAME(SetVoiceAccent)
INT16       WINAPI SetVoiceEnvelope16(INT16,INT16,INT16);
DWORD       WINAPI SetVoiceEnvelope32(DWORD,DWORD,DWORD);
#define     SetVoiceEnvelope WINELIB_NAME(SetVoiceEnvelope)
INT16       WINAPI SetVoiceNote16(INT16,INT16,INT16,INT16);
DWORD       WINAPI SetVoiceNote32(DWORD,DWORD,DWORD,DWORD);
#define     SetVoiceNote WINELIB_NAME(SetVoiceNote)
INT16       WINAPI SetVoiceQueueSize16(INT16,INT16);
DWORD       WINAPI SetVoiceQueueSize32(DWORD,DWORD);
#define     SetVoiceQueueSize WINELIB_NAME(SetVoiceQueueSize)
INT16       WINAPI SetVoiceSound16(INT16,DWORD,INT16);
DWORD       WINAPI SetVoiceSound32(DWORD,DWORD,DWORD);
#define     SetVoiceSound WINELIB_NAME(SetVoiceSound)
INT16       WINAPI SetVoiceThreshold16(INT16,INT16);
DWORD       WINAPI SetVoiceThreshold32(DWORD,DWORD);
#define     SetVoiceThreshold WINELIB_NAME(SetVoiceThreshold)
INT16       WINAPI StartSound16(void);
VOID        WINAPI StartSound32(void);
#define     StartSound WINELIB_NAME(StartSound)
INT16       WINAPI StopSound16(void);
VOID        WINAPI StopSound32(void);
#define     StopSound WINELIB_NAME(StopSound)
INT16       WINAPI SyncAllVoices16(void);
DWORD       WINAPI SyncAllVoices32(void);
#define     SyncAllVoices WINELIB_NAME(SyncAllVoices)
INT16       WINAPI WaitSoundState16(INT16);
DWORD       WINAPI WaitSoundState32(DWORD);
#define     WaitSoundState WINELIB_NAME(WaitSoundState)

#endif /* _WINE_SOUND_H */
