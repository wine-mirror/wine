#ifndef _WINE_SOUND_H
#define _WINE_SOUND_H

#include "wintypes.h"

VOID        WINAPI CloseSound16(VOID);
INT16       WINAPI CountVoiceNotes16(INT16);
DWORD       WINAPI CountVoiceNotes(DWORD);
LPINT16     WINAPI GetThresholdEvent16(void);
LPDWORD     WINAPI GetThresholdEvent(void);
INT16       WINAPI GetThresholdStatus16(void);
DWORD       WINAPI GetThresholdStatus(void);
INT16       WINAPI OpenSound16(void);
VOID        WINAPI OpenSound(void);
INT16       WINAPI SetSoundNoise16(INT16,INT16);
DWORD       WINAPI SetSoundNoise(DWORD,DWORD);
INT16       WINAPI SetVoiceAccent16(INT16,INT16,INT16,INT16,INT16);
DWORD       WINAPI SetVoiceAccent(DWORD,DWORD,DWORD,DWORD,DWORD);
INT16       WINAPI SetVoiceEnvelope16(INT16,INT16,INT16);
DWORD       WINAPI SetVoiceEnvelope(DWORD,DWORD,DWORD);
INT16       WINAPI SetVoiceNote16(INT16,INT16,INT16,INT16);
DWORD       WINAPI SetVoiceNote(DWORD,DWORD,DWORD,DWORD);
INT16       WINAPI SetVoiceQueueSize16(INT16,INT16);
DWORD       WINAPI SetVoiceQueueSize(DWORD,DWORD);
INT16       WINAPI SetVoiceSound16(INT16,DWORD,INT16);
DWORD       WINAPI SetVoiceSound(DWORD,DWORD,DWORD);
INT16       WINAPI SetVoiceThreshold16(INT16,INT16);
DWORD       WINAPI SetVoiceThreshold(DWORD,DWORD);
INT16       WINAPI StartSound16(void);
VOID        WINAPI StartSound(void);
INT16       WINAPI StopSound16(void);
VOID        WINAPI StopSound(void);
INT16       WINAPI SyncAllVoices16(void);
DWORD       WINAPI SyncAllVoices(void);
INT16       WINAPI WaitSoundState16(INT16);
DWORD       WINAPI WaitSoundState(DWORD);

#endif /* _WINE_SOUND_H */
