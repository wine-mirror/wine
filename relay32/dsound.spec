name dsound
type win32

0 stdcall DirectSoundCreate(ptr ptr ptr) DirectSoundCreate
1 stdcall DirectSoundEnumerateA(ptr ptr) DirectSoundEnumerate32A
2 stub DirectSoundEnumerateW
3 stdcall DllCanUnloadNow() DllCanUnloadNow
4 stdcall DllGetClassObject(ptr ptr ptr) DSOUND_DllGetClassObject
5 stub DirectSoundCaptureCreate
6 stub DirectSoundCaptureEnumerateA
7 stub DirectSoundCaptureEnumerateW
