name	ver
type	win16
owner	version

#1 DLLENTRYPOINT

2 pascal GetFileResourceSize(str segptr segptr ptr) GetFileResourceSize16
3 pascal GetFileResource(str segptr segptr long long ptr) GetFileResource16
6 pascal GetFileVersionInfoSize(str ptr) GetFileVersionInfoSize16
7 pascal GetFileVersionInfo(str long long ptr) GetFileVersionInfo16
8 pascal VerFindFile(word str str str ptr ptr ptr ptr) VerFindFile16
9 pascal VerInstallFile(word str str str str str ptr ptr) VerInstallFile16
10 pascal VerLanguageName(word ptr word) VerLanguageName16
11 pascal VerQueryValue(segptr str ptr ptr) VerQueryValue16
20 stub GETFILEVERSIONINFORAW
#21 VERFTHK_THUNKDATA16
#22 VERTHKSL_THUNKDATA16
