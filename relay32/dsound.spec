name dsound
type win32

0 stub DirectSoundUnknown
1 stdcall DirectSoundCreate(ptr ptr ptr) DirectSoundCreate
2 stdcall DirectSoundEnumerateA(ptr ptr) DirectSoundEnumerateA
3 stub DirectSoundEnumerateW
4 stdcall DllCanUnloadNow() DSOUND_DllCanUnloadNow
5 stdcall DllGetClassObject(ptr ptr ptr) DSOUND_DllGetClassObject
6 stub DirectSoundCaptureCreate
7 stub DirectSoundCaptureEnumerateA
8 stub DirectSoundCaptureEnumerateW
