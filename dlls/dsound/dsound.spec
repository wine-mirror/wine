name dsound
type win32

0 stub DirectSoundUnknown
1 stdcall DirectSoundCreate(ptr ptr ptr) DirectSoundCreate
2 stdcall DirectSoundEnumerateA(ptr ptr) DirectSoundEnumerateA
3 stdcall DirectSoundEnumerateW(ptr ptr) DirectSoundEnumerateW
4 stdcall DllCanUnloadNow() DSOUND_DllCanUnloadNow
5 stdcall DllGetClassObject(ptr ptr ptr) DSOUND_DllGetClassObject
6 stdcall DirectSoundCaptureCreate(ptr ptr ptr) DirectSoundCaptureCreate
7 stdcall DirectSoundCaptureEnumerateA(ptr ptr) DirectSoundCaptureEnumerateA
8 stdcall DirectSoundCaptureEnumerateW(ptr ptr) DirectSoundCaptureEnumerateW
