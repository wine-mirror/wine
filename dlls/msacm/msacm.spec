name	msacm
type	win16

  1 stub     WEP
  2 stub     DRIVERPROC
  3 stub     ___EXPORTEDSTUB
  7 pascal   acmGetVersion() acmGetVersion16
  8 pascal16 acmMetrics(word word ptr) acmMetrics16
 10 pascal16 acmDriverEnum(ptr long long) acmDriverEnum16
 11 pascal16 acmDriverDetails(word ptr long) acmDriverDetails16
 12 pascal16 acmDriverAdd(ptr word long long long) acmDriverAdd16
 13 pascal16 acmDriverRemove(word long) acmDriverRemove16
 14 pascal16 acmDriverOpen(ptr word long) acmDriverOpen16
 15 pascal16 acmDriverClose(word long) acmDriverClose16
 16 pascal   acmDriverMessage(word word long long) acmDriverMessage16
 17 pascal16 acmDriverID(word ptr long) acmDriverID16
 18 pascal16 acmDriverPriority(word long long) acmDriverPriority16
 30 pascal16 acmFormatTagDetails(word ptr long) acmFormatTagDetails16
 31 pascal16 acmFormatTagEnum(word ptr ptr long long) acmFormatTagEnum16
 40 pascal16 acmFormatChoose(ptr) acmFormatChoose16
 41 pascal16 acmFormatDetails(word ptr long) acmFormatDetails16
 42 pascal16 acmFormatEnum(word ptr ptr long long) acmFormatEnum16
 45 pascal16 acmFormatSuggest(word ptr ptr long long) acmFormatSuggest16
 50 pascal16 acmFilterTagDetails(word ptr long) acmFilterTagDetails16
 51 pascal16 acmFilterTagEnum(word ptr ptr long long) acmFilterTagEnum16
 60 pascal16 acmFilterChoose(ptr) acmFilterChoose16
 61 pascal16 acmFilterDetails(word ptr long) acmFilterDetails16
 62 pascal16 acmFilterEnum(word ptr ptr long long) acmFilterEnum16
 70 pascal16 acmStreamOpen(ptr word ptr ptr ptr long long long) acmStreamOpen16
 71 pascal16 acmStreamClose(word long) acmStreamClose16
 72 pascal16 acmStreamSize(word long ptr long) acmStreamSize16
 75 pascal16 acmStreamConvert(word ptr long) acmStreamConvert16
 76 pascal16 acmStreamReset(word long) acmStreamReset16
 77 pascal16 acmStreamPrepareHeader(word ptr long) acmStreamPrepareHeader16
 78 pascal16 acmStreamUnprepareHeader(word ptr long) acmStreamUnprepareHeader16
150 stub     ACMAPPLICATIONEXIT
175 stub     ACMHUGEPAGELOCK
176 stub     ACMHUGEPAGEUNLOCK
200 stub     ACMOPENCONVERSION
201 stub     ACMCLOSECONVERSION
202 stub     ACMCONVERT
203 stub     ACMCHOOSEFORMAT
