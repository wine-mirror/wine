name	sound
type	win16
owner	winmm

1  pascal16 OpenSound() OpenSound16
2  pascal16 CloseSound() CloseSound16
3  pascal16 SetVoiceQueueSize(word word) SetVoiceQueueSize16
4  pascal16 SetVoiceNote(word word word word) SetVoiceNote16
5  pascal16 SetVoiceAccent(word word word word word) SetVoiceAccent16
6  pascal16 SetVoiceEnvelope(word word word) SetVoiceEnvelope16
7  pascal16 SetSoundNoise(word word) SetSoundNoise16
8  pascal16 SetVoiceSound(word long word) SetVoiceSound16
9  pascal16 StartSound() StartSound16
10 pascal16 StopSound() StopSound16
11 pascal16 WaitSoundState(word) WaitSoundState16
12 pascal16 SyncAllVoices() SyncAllVoices16
13 pascal16 CountVoiceNotes(word) CountVoiceNotes16
14 pascal   GetThresholdEvent() GetThresholdEvent16
15 pascal16 GetThresholdStatus() GetThresholdStatus16
16 pascal16 SetVoiceThreshold(word word) SetVoiceThreshold16
17 pascal16 DoBeep() DoBeep16
18 stub MYOPENSOUND # W1.1, W2.0
