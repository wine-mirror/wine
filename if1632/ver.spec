name	ver
type	win16

#1 DLLENTRYPOINT

2 pascal GetFileResourceSize(ptr segptr segptr ptr) GetFileResourceSize
3 pascal GetFileResource(ptr segptr segptr long long ptr) GetFileResource
6 pascal GetFileVersionInfoSize(ptr ptr) GetFileVersionInfoSize
7 pascal GetFileVersionInfo(ptr long long ptr) GetFileVersionInfo
8 pascal VerFindFile(word ptr ptr ptr ptr ptr ptr ptr) VerFindFile
9 pascal VerInstallFile(word ptr ptr ptr ptr ptr ptr ptr) VerInstallFile
10 pascal VerLanguageName(word ptr word) VerLanguageName
11 pascal VerQueryValue(segptr ptr ptr ptr) VerQueryValue
20 stub GETFILEVERSIONINFORAW
#21 VERFTHK_THUNKDATA16
#22 VERTHKSL_THUNKDATA16
