2 stdcall InitializeCriticalSection(ptr) WINECE_InitializeCriticalSection
3 stdcall DeleteCriticalSection(ptr) WINECE_DeleteCriticalSection
4 stdcall EnterCriticalSection(ptr) WINECE_EnterCriticalSection
5 stdcall LeaveCriticalSection(ptr) WINECE_LeaveCriticalSection
6 stdcall ExitThread(long) WINECE_ExitThread
7 stub -noname PSLNotify
8 stub -noname InitLocale
9 stub -noname InterlockedTestExchange
10 stdcall -arch=i386 InterlockedIncrement(ptr) WINECE_InterlockedIncrement
11 stdcall -arch=i386 InterlockedDecrement(ptr) WINECE_InterlockedDecrement
12 stdcall -arch=i386 InterlockedExchange(ptr long) WINECE_InterlockedExchange
13 stub -noname ThreadBaseFunc
14 stub -noname MainThreadBaseFunc
15 stdcall TlsGetValue(long) WINECE_TlsGetValue
16 stdcall TlsSetValue(long ptr) WINECE_TlsSetValue
17 stub -noname GetVersionEx
18 stdcall CompareFileTime(ptr ptr) WINECE_CompareFileTime
19 stdcall SystemTimeToFileTime(ptr ptr) WINECE_SystemTimeToFileTime
20 stdcall FileTimeToSystemTime(ptr ptr) WINECE_FileTimeToSystemTime
21 stdcall FileTimeToLocalFileTime(ptr ptr) WINECE_FileTimeToLocalFileTime
22 stdcall LocalFileTimeToFileTime(ptr ptr) WINECE_LocalFileTimeToFileTime
23 stdcall GetLocalTime(ptr) WINECE_GetLocalTime
24 stdcall SetLocalTime(ptr) WINECE_SetLocalTime
25 stdcall GetSystemTime(ptr) WINECE_GetSystemTime
26 stdcall SetSystemTime(ptr) WINECE_SetSystemTime
27 stdcall GetTimeZoneInformation(ptr) WINECE_GetTimeZoneInformation
28 stdcall SetTimeZoneInformation(ptr) WINECE_SetTimeZoneInformation
29 stub -noname GetCurrentFT
30 stub -noname IsAPIReady
31 cdecl memchr(ptr long long) WINECE_memchr
32 stub -noname GetAPIAddress
33 stdcall LocalAlloc(long long) WINECE_LocalAlloc
34 stdcall LocalReAlloc(long long long) WINECE_LocalReAlloc
35 stdcall LocalSize(long) WINECE_LocalSize
36 stdcall LocalFree(long) WINECE_LocalFree
37 stub -noname RemoteLocalAlloc
38 stub -noname RemoteLocalReAlloc
39 stub -noname RemoteLocalSize
40 stub -noname RemoteLocalFree
41 stub -noname LocalAllocInProcess
42 stub -noname LocalFreeInProcess
43 stub -noname LocalSizeInProcess
44 stdcall HeapCreate(long long long) WINECE_HeapCreate
45 stdcall HeapDestroy(long) WINECE_HeapDestroy
46 stdcall HeapAlloc(long long long) WINECE_HeapAlloc
47 stdcall HeapReAlloc(long long ptr long) WINECE_HeapReAlloc
48 stdcall HeapSize(long long ptr) WINECE_HeapSize
49 stdcall HeapFree(long long long) WINECE_HeapFree
50 stdcall GetProcessHeap() WINECE_GetProcessHeap
51 stdcall HeapValidate(long long ptr) WINECE_HeapValidate
52 stub -noname GetHeapSnapshot
53 stub -noname CeModuleJit
54 stub -noname CompactAllHeaps
55 stub LocalAllocTrace
56 varargs wsprintfW(wstr wstr) user32.wsprintfW
57 stdcall wvsprintfW(ptr wstr ptr) WINECE_wvsprintfW
58 cdecl wcscat(wstr wstr) WINECE_wcscat
59 cdecl wcschr(wstr long) WINECE_wcschr
60 cdecl wcscmp(wstr wstr) WINECE_wcscmp
61 cdecl wcscpy(ptr wstr) WINECE_wcscpy
62 cdecl wcscspn(wstr wstr) WINECE_wcscspn
63 cdecl wcslen(wstr) WINECE_wcslen
64 cdecl wcsncat(wstr wstr long) WINECE_wcsncat
65 cdecl wcsncmp(wstr wstr long) WINECE_wcsncmp
66 cdecl wcsncpy(ptr wstr long) WINECE_wcsncpy
67 cdecl _wcsnset(wstr long long) WINECE__wcsnset
68 cdecl wcspbrk(wstr wstr) WINECE_wcspbrk
69 cdecl wcsrchr(wstr long) WINECE_wcsrchr
70 cdecl _wcsrev(wstr) WINECE__wcsrev
71 cdecl _wcsset(wstr long) WINECE__wcsset
72 cdecl wcsspn(wstr wstr) WINECE_wcsspn
73 cdecl wcsstr(wstr wstr) WINECE_wcsstr
74 cdecl _wcsdup(wstr) WINECE__wcsdup
75 cdecl wcstombs(ptr ptr long) WINECE_wcstombs
76 cdecl mbstowcs(ptr str long) WINECE_mbstowcs
77 cdecl wcstok(wstr wstr) WINECE_wcstok
78 cdecl _wtol(wstr) WINECE__wtol
79 stub -noname _wtoll
80 stub -noname Random
82 stub -noname ProfileStart
83 stub -noname ProfileStop
87 stub -noname __C_specific_handler(ptr long ptr ptr)
88 stdcall GlobalMemoryStatus(ptr) WINECE_GlobalMemoryStatus
89 stdcall SystemParametersInfoW(long long ptr long) WINECE_SystemParametersInfoW
90 stdcall CreateDIBSection(long ptr long ptr long long) WINECE_CreateDIBSection
91 stdcall EqualRgn(long long) WINECE_EqualRgn
92 stdcall CreateAcceleratorTableW(ptr long) WINECE_CreateAcceleratorTableW
93 stdcall DestroyAcceleratorTable(long) WINECE_DestroyAcceleratorTable
94 stdcall LoadAcceleratorsW(long wstr) WINECE_LoadAcceleratorsW
95 stdcall RegisterClassW(ptr) WINECE_RegisterClassW
96 stdcall CopyRect(ptr ptr) WINECE_CopyRect
97 stdcall EqualRect(ptr ptr) WINECE_EqualRect
98 stdcall InflateRect(ptr long long) WINECE_InflateRect
99 stdcall IntersectRect(ptr ptr ptr) WINECE_IntersectRect
100 stdcall IsRectEmpty(ptr) WINECE_IsRectEmpty
101 stdcall OffsetRect(ptr long long) WINECE_OffsetRect
102 stdcall PtInRect(ptr int64) WINECE_PtInRect
103 stdcall SetRect(ptr long long long long) WINECE_SetRect
104 stdcall SetRectEmpty(ptr) WINECE_SetRectEmpty
105 stdcall SubtractRect(ptr ptr ptr) WINECE_SubtractRect
106 stdcall UnionRect(ptr ptr ptr) WINECE_UnionRect
107 stdcall ClearCommBreak(long) WINECE_ClearCommBreak
108 stdcall ClearCommError(long ptr ptr) WINECE_ClearCommError
109 stdcall EscapeCommFunction(long long) WINECE_EscapeCommFunction
110 stdcall GetCommMask(long ptr) WINECE_GetCommMask
111 stdcall GetCommModemStatus(long ptr) WINECE_GetCommModemStatus
112 stdcall GetCommProperties(long ptr) WINECE_GetCommProperties
113 stdcall GetCommState(long ptr) WINECE_GetCommState
114 stdcall GetCommTimeouts(long ptr) WINECE_GetCommTimeouts
115 stdcall PurgeComm(long long) WINECE_PurgeComm
116 stdcall SetCommBreak(long) WINECE_SetCommBreak
117 stdcall SetCommMask(long ptr) WINECE_SetCommMask
118 stdcall SetCommState(long ptr) WINECE_SetCommState
119 stdcall SetCommTimeouts(long ptr) WINECE_SetCommTimeouts
120 stdcall SetupComm(long long long) WINECE_SetupComm
121 stdcall TransmitCommChar(long long) WINECE_TransmitCommChar
122 stdcall WaitCommEvent(long ptr ptr) WINECE_WaitCommEvent
123 stub -noname EnumPnpIds
124 stub -noname EnumDevices
125 stub -noname GetDeviceKeys
126 stdcall CryptAcquireContextW(ptr wstr wstr long long) WINECE_CryptAcquireContextW
127 stub -noname CryptReleaseContext
128 stub -noname CryptGenKey
129 stub -noname CryptDeriveKey
130 stub -noname CryptDestroyKey
131 stub -noname CryptSetKeyParam
132 stub -noname CryptGetKeyParam
133 stub -noname CryptExportKey
134 stub -noname CryptImportKey
135 stub -noname CryptEncrypt
136 stub -noname CryptDecrypt
137 stub -noname CryptCreateHash
138 stub -noname CryptHashSessionKey
139 stub -noname CryptHashData
140 stub -noname CryptDestroyHash
141 stdcall CryptSignHashW(long long ptr long ptr ptr) WINECE_CryptSignHashW
142 stdcall CryptVerifySignatureW(long ptr long long ptr long) WINECE_CryptVerifySignatureW
143 stub -noname CryptGenRandom
144 stub -noname CryptGetUserKey
145 stdcall CryptSetProviderW(wstr long) WINECE_CryptSetProviderW
146 stub -noname CryptGetHashParam
147 stub -noname CryptSetHashParam
148 stub -noname CryptGetProvParam
149 stub -noname CryptSetProvParam
150 stdcall CryptSetProviderExW(wstr long ptr long) WINECE_CryptSetProviderExW
151 stdcall CryptGetDefaultProviderW(long ptr long ptr ptr) WINECE_CryptGetDefaultProviderW
152 stdcall CryptEnumProviderTypesW(long ptr long ptr ptr ptr) WINECE_CryptEnumProviderTypesW
153 stdcall CryptEnumProvidersW(long ptr long ptr ptr ptr) WINECE_CryptEnumProvidersW
154 stub -noname CryptContextAddRef
155 stub -noname CryptDuplicateKey
156 stub -noname CryptDuplicateHash
157 stub -noname AttachDebugger
158 stub -noname SetInterruptEvent
159 stub -noname IsExiting
160 stdcall CreateDirectoryW(wstr ptr) WINECE_CreateDirectoryW
161 stdcall RemoveDirectoryW(wstr) WINECE_RemoveDirectoryW
162 stdcall GetTempPathW(long ptr) WINECE_GetTempPathW
163 stdcall MoveFileW(wstr wstr) WINECE_MoveFileW
164 stdcall CopyFileW(wstr wstr long) WINECE_CopyFileW
165 stdcall DeleteFileW(wstr) WINECE_DeleteFileW
166 stdcall GetFileAttributesW(wstr) WINECE_GetFileAttributesW
167 stdcall FindFirstFileW(wstr ptr) WINECE_FindFirstFileW
168 stdcall CreateFileW(wstr long long ptr long long long) WINECE_CreateFileW
169 stdcall SetFileAttributesW(wstr long) WINECE_SetFileAttributesW
170 stdcall ReadFile(long ptr long ptr ptr) WINECE_ReadFile
171 stdcall WriteFile(long ptr long ptr ptr) WINECE_WriteFile
172 stdcall GetFileSize(long ptr) WINECE_GetFileSize
173 stdcall SetFilePointer(long long ptr long) WINECE_SetFilePointer
174 stdcall GetFileInformationByHandle(long ptr) WINECE_GetFileInformationByHandle
175 stdcall FlushFileBuffers(long) WINECE_FlushFileBuffers
176 stdcall GetFileTime(long ptr ptr ptr) WINECE_GetFileTime
177 stdcall SetFileTime(long ptr ptr ptr) WINECE_SetFileTime
178 stdcall SetEndOfFile(long) WINECE_SetEndOfFile
179 stdcall DeviceIoControl(long long ptr long ptr long ptr ptr) WINECE_DeviceIoControl
180 stdcall FindClose(long) WINECE_FindClose
181 stdcall FindNextFileW(long ptr) WINECE_FindNextFileW
182 stub -noname CheckPassword
183 stub -noname DeleteAndRenameFile
184 stdcall GetDiskFreeSpaceExW(wstr ptr ptr ptr) WINECE_GetDiskFreeSpaceExW
185 stdcall IsValidCodePage(long) WINECE_IsValidCodePage
186 stdcall GetACP() WINECE_GetACP
187 stdcall GetOEMCP() WINECE_GetOEMCP
188 stdcall GetCPInfo(long ptr) WINECE_GetCPInfo
189 stub -noname SetACP
190 stub -noname SetOEMCP
191 stdcall IsDBCSLeadByte(long) WINECE_IsDBCSLeadByte
192 stdcall IsDBCSLeadByteEx(long long) WINECE_IsDBCSLeadByteEx
193 cdecl iswctype(long long) WINECE_iswctype
194 cdecl towlower(long) WINECE_towlower
195 cdecl towupper(long) WINECE_towupper
196 stdcall MultiByteToWideChar(long long str long ptr long) WINECE_MultiByteToWideChar
197 stdcall WideCharToMultiByte(long long wstr long ptr long ptr ptr) WINECE_WideCharToMultiByte
198 stdcall CompareStringW(long long wstr long wstr long) WINECE_CompareStringW
199 stdcall LCMapStringW(long long wstr long ptr long) WINECE_LCMapStringW
200 stdcall GetLocaleInfoW(long long ptr long) WINECE_GetLocaleInfoW
201 stdcall SetLocaleInfoW(long long wstr) WINECE_SetLocaleInfoW
202 stdcall GetTimeFormatW(long long ptr wstr ptr long) WINECE_GetTimeFormatW
203 stdcall GetDateFormatW(long long ptr wstr ptr long) WINECE_GetDateFormatW
204 stdcall GetNumberFormatW(long long wstr ptr ptr long) WINECE_GetNumberFormatW
205 stdcall GetCurrencyFormatW(long long str ptr str long) WINECE_GetCurrencyFormatW
206 stdcall EnumCalendarInfoW(ptr long long long) WINECE_EnumCalendarInfoW
207 stdcall EnumTimeFormatsW(ptr long long) WINECE_EnumTimeFormatsW
208 stdcall EnumDateFormatsW(ptr long long) WINECE_EnumDateFormatsW
209 stdcall IsValidLocale(long long) WINECE_IsValidLocale
210 stdcall ConvertDefaultLocale(long) WINECE_ConvertDefaultLocale
211 stdcall GetSystemDefaultLangID() WINECE_GetSystemDefaultLangID
212 stdcall GetUserDefaultLangID() WINECE_GetUserDefaultLangID
213 stdcall GetSystemDefaultLCID() WINECE_GetSystemDefaultLCID
214 stub -noname SetSystemDefaultLCID
215 stdcall GetUserDefaultLCID() WINECE_GetUserDefaultLCID
216 stdcall GetStringTypeW(long wstr long ptr) WINECE_GetStringTypeW
217 stdcall GetStringTypeExW(long long wstr long ptr) WINECE_GetStringTypeExW
218 stdcall FoldStringW(long wstr long ptr long) WINECE_FoldStringW
219 stdcall EnumSystemLocalesW(ptr long) WINECE_EnumSystemLocalesW
220 stdcall EnumSystemCodePagesW(ptr long) WINECE_EnumSystemCodePagesW
221 stdcall CharLowerW(wstr) WINECE_CharLowerW
222 stdcall CharLowerBuffW(wstr long) WINECE_CharLowerBuffW
223 stdcall CharUpperBuffW(wstr long) WINECE_CharUpperBuffW
224 stdcall CharUpperW(wstr) WINECE_CharUpperW
225 stdcall CharPrevW(wstr wstr) WINECE_CharPrevW
226 stdcall CharNextW(wstr) WINECE_CharNextW
227 stdcall lstrcmpW(wstr wstr) WINECE_lstrcmpW
228 stdcall lstrcmpiW(wstr wstr) WINECE_lstrcmpiW
229 cdecl _wcsnicmp(wstr wstr long) WINECE__wcsnicmp
230 cdecl _wcsicmp(wstr wstr) WINECE__wcsicmp
231 cdecl _wcslwr(wstr) WINECE__wcslwr
232 cdecl _wcsupr(wstr) WINECE__wcsupr
233 stub -noname DBCanonicalize
234 stdcall FormatMessageW(long ptr long long ptr long ptr) WINECE_FormatMessageW
235 stub -noname RegisterDevice
236 stub -noname DeregisterDevice
237 stub -noname LoadFSD
238 stub -noname SetPassword
239 stub -noname GetPasswordActive
240 stub -noname SetPasswordActive
241 stub -noname FileSystemPowerFunction
242 stub -noname CloseAllFileHandles
243 stub -noname ReadFileWithSeek
245 stub -noname CreateDeviceHandle
246 stdcall CreateWindowExW(long wstr wstr long long long long long long long long ptr) WINECE_CreateWindowExW
247 stdcall SetWindowPos(long long long long long long long) WINECE_SetWindowPos
248 stdcall GetWindowRect(long ptr) WINECE_GetWindowRect
249 stdcall GetClientRect(long long) WINECE_GetClientRect
250 stdcall InvalidateRect(long ptr long) WINECE_InvalidateRect
251 stdcall GetWindow(long long) WINECE_GetWindow
252 stdcall WindowFromPoint(int64) WINECE_WindowFromPoint
253 stdcall ChildWindowFromPoint(long int64) WINECE_ChildWindowFromPoint
254 stdcall ClientToScreen(long ptr) WINECE_ClientToScreen
255 stdcall ScreenToClient(long ptr) WINECE_ScreenToClient
256 stdcall SetWindowTextW(long wstr) WINECE_SetWindowTextW
257 stdcall GetWindowTextW(long ptr long) WINECE_GetWindowTextW
258 stdcall SetWindowLongW(long long long) WINECE_SetWindowLongW
259 stdcall GetWindowLongW(long long) WINECE_GetWindowLongW
260 stdcall BeginPaint(long ptr) WINECE_BeginPaint
261 stdcall EndPaint(long ptr) WINECE_EndPaint
262 stdcall GetDC(long) WINECE_GetDC
263 stdcall ReleaseDC(long long) WINECE_ReleaseDC
264 stdcall -register DefWindowProcW(long long long long) WINECE_DefWindowProcW
265 stdcall DestroyWindow(long) WINECE_DestroyWindow
266 stdcall ShowWindow(long long) WINECE_ShowWindow
267 stdcall UpdateWindow(long) WINECE_UpdateWindow
268 stdcall SetParent(long long) WINECE_SetParent
269 stdcall GetParent(long) WINECE_GetParent
270 stdcall GetWindowDC(long) WINECE_GetWindowDC
271 stdcall IsWindow(long) WINECE_IsWindow
272 stdcall MoveWindow(long long long long long long) WINECE_MoveWindow
273 stdcall GetUpdateRgn(long long long) WINECE_GetUpdateRgn
274 stdcall GetUpdateRect(long ptr long) WINECE_GetUpdateRect
275 stdcall BringWindowToTop(long) WINECE_BringWindowToTop
276 stdcall GetWindowTextLengthW(long) WINECE_GetWindowTextLengthW
277 stdcall IsChild(long long) WINECE_IsChild
278 stdcall ValidateRect(long ptr) WINECE_ValidateRect
279 stdcall SetScrollInfo(long long ptr long) WINECE_SetScrollInfo
280 stdcall SetScrollPos(long long long long) WINECE_SetScrollPos
281 stdcall SetScrollRange(long long long long long) WINECE_SetScrollRange
282 stdcall GetScrollInfo(long long ptr) WINECE_GetScrollInfo
283 stdcall GetClassNameW(long ptr long) WINECE_GetClassNameW
284 stdcall MapWindowPoints(long long ptr long) WINECE_MapWindowPoints
285 stdcall CallWindowProcW(ptr long long long long) WINECE_CallWindowProcW
286 stdcall FindWindowW(wstr wstr) WINECE_FindWindowW
287 stdcall EnableWindow(long long) WINECE_EnableWindow
288 stdcall IsWindowEnabled(long) WINECE_IsWindowEnabled
289 stdcall ScrollWindowEx(long long long ptr ptr long ptr long) WINECE_ScrollWindowEx
290 stdcall PostThreadMessageW(long long long long) WINECE_PostThreadMessageW
291 stdcall EnumWindows(ptr long) WINECE_EnumWindows
292 stdcall GetWindowThreadProcessId(long ptr) WINECE_GetWindowThreadProcessId
293 stub -noname RegisterSIPanel
294 stub -noname RectangleAnimation
295 stub -noname SHGetSpecialFolderPath
296 stub -noname GwesPowerOffSystem
297 stub -noname BatteryDrvrGetLevels
298 stub -noname BatteryDrvrSupportsChangeNotification
299 stub -noname SetAssociatedMenu
300 stub -noname GetAssociatedMenu
301 stub -noname PegOidGetInfo
302 stub -noname PegFindFirstDatabase
303 stub -noname PegFindNextDatabase
304 stub -noname PegCreateDatabase
305 stub -noname PegSetDatabaseInfo
306 stub -noname PegOpenDatabase
307 stub -noname PegDeleteDatabase
308 stub -noname PegSeekDatabase
309 stub -noname PegDeleteRecord
310 stub -noname PegReadRecordProps
311 stub -noname PegWriteRecordProps
312 stub -noname CeOidGetInfo
313 stub -noname CeFindFirstDatabase
314 stub -noname CeFindNextDatabase
315 stub -noname CeCreateDatabase
316 stub -noname CeSetDatabaseInfo
317 stub -noname CeOpenDatabase
318 stub -noname CeDeleteDatabase
319 stub -noname CeSeekDatabase
320 stub -noname CeDeleteRecord
321 stub -noname CeReadRecordProps
322 stub -noname CeWriteRecordProps
323 stub -noname GetStoreInformation
324 stub -noname CeGetReplChangeMask
325 stub -noname CeSetReplChangeMask
326 stub -noname CeGetReplChangeBitsEx
327 stub -noname CeSetReplChangeBitsEx
328 stub -noname CeClearReplChangeBitsEx
329 stub -noname CeGetReplOtherBitsEx
330 stub -noname CeSetReplOtherBitsEx
331 stub -noname CeRegisterFileSystemNotification
332 stub -noname CeRegisterReplNotification
335 stub -noname DeregisterAFS
336 stub -noname GetSystemMemoryDivision
337 stub -noname SetSystemMemoryDivision
338 stub -noname RegisterAFSName
339 stub -noname DeregisterAFSName
340 stub -noname CeChangeDatabaseLCID
341 stub -noname DumpFileSystemHeap
342 stub -noname RasDial
343 stub -noname RasHangup
344 stub -noname RasHangUp
345 stub -noname RasEnumEntries
346 stub -noname RasGetEntryDialParams
347 stub -noname RasSetEntryDialParams
348 stub -noname RasGetEntryProperties
349 stub -noname RasSetEntryProperties
350 stub -noname RasValidateEntryName
351 stub -noname RasDeleteEntry
352 stub -noname RasRenameEntry
353 stub -noname RasEnumConnections
354 stub -noname RasGetConnectStatus
355 stub -noname RasGetEntryDevConfig
356 stub -noname RasSetEntryDevConfig
357 stub -noname RasIOControl
358 stub -noname lineClose
359 stub -noname lineDeallocateCall
360 stub -noname lineDrop
361 stub -noname lineGetDevCaps
362 stub -noname lineGetDevConfig
363 stub -noname lineGetTranslateCaps
364 stub -noname lineInitialize
365 stub -noname lineMakeCall
366 stub -noname lineNegotiateAPIVersion
367 stub -noname lineOpen
368 stub -noname lineSetDevConfig
369 stub -noname lineSetStatusMessages
370 stub -noname lineShutdown
371 stub -noname lineTranslateAddress
372 stub -noname lineGetID
373 stub -noname lineTranslateDialog
374 stub -noname lineConfigDialogEdit
375 stub -noname lineAddProvider
376 stub -noname AudioUpdateFromRegistry
377 stdcall sndPlaySoundW(ptr long) WINECE_sndPlaySoundW
378 stdcall PlaySoundW(ptr long long) WINECE_PlaySoundW
379 stub -noname waveOutGetNumDevs
380 stub -noname waveOutGetDevCaps
381 stub -noname waveOutGetVolume
382 stub -noname waveOutSetVolume
383 stub -noname waveOutGetErrorText
384 stub -noname waveOutClose
385 stub -noname waveOutPrepareHeader
386 stub -noname waveOutUnprepareHeader
387 stub -noname waveOutWrite
388 stub -noname waveOutPause
389 stub -noname waveOutRestart
390 stub -noname waveOutReset
391 stub -noname waveOutBreakLoop
392 stub -noname waveOutGetPosition
393 stub -noname waveOutGetPitch
394 stub -noname waveOutSetPitch
395 stub -noname waveOutGetPlaybackRate
396 stub -noname waveOutSetPlaybackRate
397 stub -noname waveOutGetID
398 stub -noname waveOutMessage
399 stub -noname waveOutOpen
400 stub -noname waveInGetNumDevs
401 stub -noname waveInGetDevCaps
402 stub -noname waveInGetErrorText
403 stub -noname waveInClose
404 stub -noname waveInPrepareHeader
405 stub -noname waveInUnprepareHeader
406 stub -noname waveInAddBuffer
407 stub -noname waveInStart
408 stub -noname waveInStop
409 stub -noname waveInReset
410 stub -noname waveInGetPosition
411 stub -noname waveInGetID
412 stub -noname waveInMessage
413 stub -noname waveInOpen
414 stub -noname acmDriverAdd
415 stub -noname acmDriverClose
416 stub -noname acmDriverDetails
417 stub -noname acmDriverEnum
418 stub -noname acmDriverID
419 stub -noname acmDriverMessage
420 stub -noname acmDriverOpen
421 stub -noname acmDriverPriority
422 stub -noname acmDriverRemove
423 stub -noname acmFilterChoose
424 stub -noname acmFilterDetails
425 stub -noname acmFilterEnum
426 stub -noname acmFilterTagDetails
427 stub -noname acmFilterTagEnum
428 stub -noname acmFormatChoose
429 stub -noname acmFormatDetails
430 stub -noname acmFormatEnum
431 stub -noname acmFormatSuggest
432 stub -noname acmFormatTagDetails
433 stub -noname acmFormatTagEnum
434 stub -noname acmStreamClose
435 stub -noname acmStreamConvert
436 stub -noname acmStreamMessage
437 stub -noname acmStreamOpen
438 stub -noname acmStreamPrepareHeader
439 stub -noname acmStreamReset
440 stub -noname acmStreamSize
441 stub -noname acmStreamUnprepareHeader
442 stub -noname acmGetVersion
443 stub -noname acmMetrics
444 stdcall WNetAddConnection3W(long ptr wstr wstr long) WINECE_WNetAddConnection3W
445 stdcall WNetCancelConnection2W(wstr long long) WINECE_WNetCancelConnection2W
446 stdcall WNetConnectionDialog1W(ptr) WINECE_WNetConnectionDialog1W
447 stub -noname WNetDisconnectDialog
448 stdcall WNetDisconnectDialog1W(ptr) WINECE_WNetDisconnectDialog1W
449 stdcall WNetGetConnectionW(wstr ptr ptr) WINECE_WNetGetConnectionW
450 stdcall WNetGetUniversalNameW(wstr long ptr ptr) WINECE_WNetGetUniversalNameW
451 stdcall WNetGetUserW(wstr wstr ptr) WINECE_WNetGetUserW
452 stdcall WNetOpenEnumW(long long long ptr ptr) WINECE_WNetOpenEnumW
453 stub -noname WNetCloseEnum
454 stdcall WNetEnumResourceW(long ptr ptr ptr) WINECE_WNetEnumResourceW
455 stdcall RegCloseKey(long) WINECE_RegCloseKey
456 stdcall RegCreateKeyExW(long wstr long ptr long long ptr ptr ptr) WINECE_RegCreateKeyExW
457 stdcall RegDeleteKeyW(long wstr) WINECE_RegDeleteKeyW
458 stdcall RegDeleteValueW(long wstr) WINECE_RegDeleteValueW
459 stdcall RegEnumValueW(long long ptr ptr ptr ptr ptr ptr) WINECE_RegEnumValueW
460 stdcall RegEnumKeyExW(long long ptr ptr ptr ptr ptr ptr) WINECE_RegEnumKeyExW
461 stdcall RegOpenKeyExW(long wstr long long ptr) WINECE_RegOpenKeyExW
462 stdcall RegQueryInfoKeyW(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) WINECE_RegQueryInfoKeyW
463 stdcall RegQueryValueExW(long wstr ptr ptr ptr ptr) WINECE_RegQueryValueExW
464 stdcall RegSetValueExW(long wstr long long ptr long) WINECE_RegSetValueExW
465 stub -noname RegCopyFile
466 stub -noname RegRestoreFile
467 stub -noname PegSetUserNotification
468 stub -noname PegClearUserNotification
469 stub -noname PegRunAppAtTime
470 stub -noname PegRunAppAtEvent
471 stub -noname PegHandleAppNotifications
472 stub -noname PegGetUserNotificationPreferences
473 stub -noname CeSetUserNotification
474 stub -noname CeClearUserNotification
475 stub -noname CeRunAppAtTime
476 stub -noname CeRunAppAtEvent
477 stub -noname CeHandleAppNotifications
478 stub -noname CeGetUserNotificationPreferences
479 stub -noname CeEventHasOccurred
480 stdcall ShellExecuteEx(long) WINECE_ShellExecuteEx
481 stub -noname Shell_NotifyIcon
482 stdcall SHGetFileInfo(ptr long ptr long long) WINECE_SHGetFileInfo
483 stub -noname SHAddToRecentDocs
484 stub -noname SHCreateShortcut
485 stub -noname SHGetShortcutTarget
486 stub -noname SHShowOutOfMemory
487 stub -noname SHLoadDIBitmap
488 stdcall GetOpenFileNameW(ptr) WINECE_GetOpenFileNameW
489 stdcall GetSaveFileNameW(ptr) WINECE_GetSaveFileNameW
490 stub -noname QueryAPISetID
491 stdcall TerminateThread(long long) WINECE_TerminateThread
492 stdcall CreateThread(ptr long ptr long long ptr) WINECE_CreateThread
493 stdcall CreateProcessW(wstr wstr ptr ptr long long ptr wstr ptr ptr) WINECE_CreateProcessW
494 stdcall EventModify(ptr long) COREDLL_EventModify
495 stdcall CreateEventW(ptr long long wstr) WINECE_CreateEventW
496 stdcall Sleep(long) WINECE_Sleep
497 stdcall WaitForSingleObject(long long) WINECE_WaitForSingleObject
498 stdcall WaitForMultipleObjects(long ptr long long) WINECE_WaitForMultipleObjects
499 stdcall SuspendThread(long) WINECE_SuspendThread
500 stdcall ResumeThread(long) WINECE_ResumeThread
502 stdcall SetThreadContext(long ptr) WINECE_SetThreadContext
503 stdcall WaitForDebugEvent(ptr long) WINECE_WaitForDebugEvent
504 stdcall ContinueDebugEvent(long long long) WINECE_ContinueDebugEvent
505 stdcall DebugActiveProcess(long) WINECE_DebugActiveProcess
506 stdcall ReadProcessMemory(long ptr ptr long ptr) WINECE_ReadProcessMemory
507 stdcall WriteProcessMemory(long ptr ptr long ptr) WINECE_WriteProcessMemory
508 stdcall FlushInstructionCache(long long long) WINECE_FlushInstructionCache
509 stdcall OpenProcess(long long long) WINECE_OpenProcess
510 stub -noname DumpKCallProfile
511 stub -noname THCreateSnapshot
512 stub -noname THGrow
513 stub -noname NotifyForceCleanboot
514 stdcall SetThreadPriority(long long) WINECE_SetThreadPriority
515 stdcall GetThreadPriority(long) WINECE_GetThreadPriority
516 stdcall GetLastError() WINECE_GetLastError
517 stdcall SetLastError(long) WINECE_SetLastError
518 stdcall GetExitCodeThread(long ptr) WINECE_GetExitCodeThread
519 stdcall GetExitCodeProcess(long ptr) WINECE_GetExitCodeProcess
520 stub -noname TlsCall
521 stdcall IsBadCodePtr(ptr) WINECE_IsBadCodePtr
522 stdcall IsBadReadPtr(ptr long) WINECE_IsBadReadPtr
523 stdcall IsBadWritePtr(ptr long) WINECE_IsBadWritePtr
524 stdcall VirtualAlloc(ptr long long long) WINECE_VirtualAlloc
525 stdcall VirtualFree(ptr long long) WINECE_VirtualFree
526 stdcall VirtualProtect(ptr long long ptr) WINECE_VirtualProtect
527 stdcall VirtualQuery(ptr ptr long) WINECE_VirtualQuery
528 stdcall LoadLibraryW(wstr) WINECE_LoadLibraryW
529 stdcall FreeLibrary(long) WINECE_FreeLibrary
530 stdcall GetProcAddressW(long wstr) COREDLL_GetProcAddressW
531 stub -noname FindResource
532 stdcall FindResourceW(long wstr wstr) WINECE_FindResourceW
533 stdcall LoadResource(long long) WINECE_LoadResource
534 stdcall SizeofResource(long long) WINECE_SizeofResource
535 stdcall GetTickCount() WINECE_GetTickCount
536 stdcall GetProcessVersion(long) WINECE_GetProcessVersion
537 stdcall GetModuleFileNameW(long ptr long) WINECE_GetModuleFileNameW
538 stdcall QueryPerformanceCounter(ptr) WINECE_QueryPerformanceCounter
539 stdcall QueryPerformanceFrequency(ptr) WINECE_QueryPerformanceFrequency
540 stub -noname ForcePageout
541 stdcall OutputDebugStringW(wstr) WINECE_OutputDebugStringW
542 stdcall GetSystemInfo(ptr) WINECE_GetSystemInfo
543 stdcall RaiseException(long long long ptr) WINECE_RaiseException
544 stdcall TerminateProcess(long long) WINECE_TerminateProcess
545 stub -noname NKDbgPrintfW
546 stub -noname RegisterDbgZones
547 stub -noname SetDaylightTime
548 stdcall CreateFileMappingW(long ptr long long long wstr) WINECE_CreateFileMappingW
549 stdcall MapViewOfFile(long long long long long) WINECE_MapViewOfFile
550 stdcall UnmapViewOfFile(ptr) WINECE_UnmapViewOfFile
551 stdcall FlushViewOfFile(ptr long) WINECE_FlushViewOfFile
552 stub -noname CreateFileForMapping
553 stdcall CloseHandle(long) WINECE_CloseHandle
555 stdcall CreateMutexW(ptr long wstr) WINECE_CreateMutexW
556 stdcall ReleaseMutex(long) WINECE_ReleaseMutex
557 stub -noname KernelIoControl
558 stub -noname AddEventAccess
559 stub -noname CreateAPISet
560 stub -noname VirtualCopy
563 stub -noname U_ropen
564 stub -noname U_rread
565 stub -noname U_rwrite
566 stub -noname U_rlseek
567 stub -noname U_rclose
568 stub -noname NKvDbgPrintfW
569 stub -noname ProfileSyscall
570 stub -noname GetRealTime
571 stub -noname SetRealTime
572 stub -noname ProcessDetachAllDLLs
573 stub -noname ExtractResource
574 stub -noname KernExtractIcons
575 stub -noname GetRomFileInfo
576 stub -noname GetRomFileBytes
577 stub -noname CacheSync
578 stub -noname AddTrackedItem
579 stub -noname DeleteTrackedItem
580 stub -noname PrintTrackedItem
581 stub -noname GetKPhys
582 stub -noname GiveKPhys
583 stub -noname SetExceptionHandler
584 stub -noname RegisterTrackedItem
585 stub -noname FilterTrackedItem
586 stub -noname SetKernelAlarm
587 stub -noname RefreshKernelAlarm
589 stub -noname CloseProcOE
590 stub -noname SetGwesOOMEvent
591 stub -noname StringCompress
592 stub -noname StringDecompress
593 stub -noname BinaryCompress
594 stub -noname BinaryDecompress
595 stub -noname InputDebugCharW
596 stub -noname TakeCritSec
597 stub -noname LeaveCritSec
598 stub -noname MapPtrToProcess
599 stub -noname MapPtrUnsecure
600 stub -noname GetProcFromPtr
601 stub -noname IsBadPtr
602 stub -noname GetProcAddrBits
603 stub -noname GetFSHeapInfo
604 stub -noname OtherThreadsRunning
605 stub -noname KillAllOtherThreads
606 stub -noname GetOwnerProcess
607 stub -noname GetCallerProcess
608 stub -noname GetIdleTime
609 stub -noname SetLowestScheduledPriority
610 stub -noname IsPrimaryThread
611 stub -noname SetProcPermissions
612 stub -noname GetCurrentPermissions
613 stub -noname IsEncryptionPermitted
614 stub -noname SetTimeZoneBias
615 stub -noname SetCleanRebootFlag
616 stub -noname CreateCrit
617 stub -noname PowerOffSystem
618 stub -noname SetDbgZone
619 stub -noname TurnOnProfiling
620 stub -noname TurnOffProfiling
621 stub -noname CeSetThreadPriority
622 stub -noname CeGetThreadPriority
623 stub -noname NKTerminateThread
624 stub -noname GetProcName
625 stub -noname SetHandleOwner
626 stub -noname LoadDriver
627 stub -noname InterruptInitialize
628 stub -noname InterruptDone
629 stub -noname InterruptDisable
630 stub -noname SetKMode
631 stub -noname SetPowerOffHandler
632 stub -noname SetGwesPowerHandler
633 stub -noname ConnectDebugger
634 stub -noname SetHardwareWatch
635 stub -noname RegisterAPISet
636 stub -noname CreateAPIHandle
637 stub -noname VerifyAPIHandle
638 stub -noname PPSHRestart
639 stub -noname SignalStarted
640 stub -noname GetProcessIndexFromID
641 stub -noname GetCallerProcessIndex
642 stub -noname DebugNotify
643 stub -noname AFS_Unmount
644 stub -noname AFS_CreateDirectoryW
645 stub -noname AFS_RemoveDirectoryW
646 stub -noname AFS_GetFileAttributesW
647 stub -noname AFS_SetFileAttributesW
648 stub -noname AFS_CreateFileW
649 stub -noname AFS_DeleteFileW
650 stub -noname AFS_MoveFileW
651 stub -noname AFS_FindFirstFileW
652 stub -noname AFS_RegisterFileSystemFunction
653 varargs sscanf(str str) msvcrt.sscanf
654 stub -noname AFS_PrestoChangoFileName
655 stub -noname AFS_CloseAllFileHandles
656 stub -noname AFS_GetDiskFreeSpace
657 stub -noname AFS_NotifyMountedFS
658 stdcall CreateCaret(long long long long) WINECE_CreateCaret
659 stdcall DestroyCaret() WINECE_DestroyCaret
660 stdcall HideCaret(long) WINECE_HideCaret
661 stdcall ShowCaret(long) WINECE_ShowCaret
662 stdcall SetCaretPos(long long) WINECE_SetCaretPos
663 stdcall GetCaretPos(ptr) WINECE_GetCaretPos
664 stdcall SetCaretBlinkTime(long) WINECE_SetCaretBlinkTime
665 stdcall GetCaretBlinkTime() WINECE_GetCaretBlinkTime
666 stub -noname DisableCaretSystemWide
667 stub -noname EnableCaretSystemWide
668 stdcall OpenClipboard(long) WINECE_OpenClipboard
669 stdcall CloseClipboard() WINECE_CloseClipboard
670 stdcall GetClipboardOwner() WINECE_GetClipboardOwner
671 stdcall SetClipboardData(long long) WINECE_SetClipboardData
672 stdcall GetClipboardData(long) WINECE_GetClipboardData
673 stdcall RegisterClipboardFormatW(wstr) WINECE_RegisterClipboardFormatW
674 stdcall CountClipboardFormats() WINECE_CountClipboardFormats
675 stdcall EnumClipboardFormats(long) WINECE_EnumClipboardFormats
676 stdcall GetClipboardFormatNameW(long ptr long) WINECE_GetClipboardFormatNameW
677 stdcall EmptyClipboard() WINECE_EmptyClipboard
678 stdcall IsClipboardFormatAvailable(long) WINECE_IsClipboardFormatAvailable
679 stdcall GetPriorityClipboardFormat(ptr long) WINECE_GetPriorityClipboardFormat
680 stdcall GetOpenClipboardWindow() WINECE_GetOpenClipboardWindow
681 stub -noname GetClipboardDataAlloc
682 stdcall SetCursor(long) WINECE_SetCursor
683 stdcall LoadCursorW(long wstr) WINECE_LoadCursorW
684 stdcall CheckRadioButton(long long long long) WINECE_CheckRadioButton
685 stdcall SendDlgItemMessageW(long long long long long) WINECE_SendDlgItemMessageW
686 stdcall SetDlgItemTextW(long long wstr) WINECE_SetDlgItemTextW
687 stdcall GetDlgItemTextW(long long ptr long) WINECE_GetDlgItemTextW
688 stdcall CreateDialogIndirectParamW(long ptr long ptr long) WINECE_CreateDialogIndirectParamW
689 stdcall DefDlgProcW(long long long long) WINECE_DefDlgProcW
690 stdcall DialogBoxIndirectParamW(long ptr long ptr long) WINECE_DialogBoxIndirectParamW
691 stdcall EndDialog(long long) WINECE_EndDialog
692 stdcall GetDlgItem(long long) WINECE_GetDlgItem
693 stdcall GetDlgCtrlID(long) WINECE_GetDlgCtrlID
694 stdcall GetDialogBaseUnits() WINECE_GetDialogBaseUnits
695 stdcall GetDlgItemInt(long long ptr long) WINECE_GetDlgItemInt
696 stdcall GetNextDlgTabItem(long long long) WINECE_GetNextDlgTabItem
697 stdcall GetNextDlgGroupItem(long long long) WINECE_GetNextDlgGroupItem
698 stdcall IsDialogMessageW(long ptr) WINECE_IsDialogMessageW
699 stdcall MapDialogRect(long ptr) WINECE_MapDialogRect
700 stdcall SetDlgItemInt(long long long long) WINECE_SetDlgItemInt
701 stdcall GetForegroundWindow() WINECE_GetForegroundWindow
702 stdcall SetForegroundWindow(long) WINECE_SetForegroundWindow
703 stdcall SetActiveWindow(long) WINECE_SetActiveWindow
704 stdcall SetFocus(long) WINECE_SetFocus
705 stdcall GetFocus() WINECE_GetFocus
706 stdcall GetActiveWindow() WINECE_GetActiveWindow
707 stdcall GetCapture() WINECE_GetCapture
708 stdcall SetCapture(long) WINECE_SetCapture
709 stdcall ReleaseCapture() WINECE_ReleaseCapture
710 stub -noname SetKeyboardTarget
711 stub -noname GetKeyboardTarget
712 stub -noname ShellModalEnd
713 stub -noname BatteryGetLifeTimeInfo
714 stub -noname BatteryNotifyOfTimeChange
715 stub -noname GetSystemPowerStatusEx
716 stub -noname NotifyWinUserSystem
717 stdcall GetVersionExW(ptr) WINECE_GetVersionExW
718 stub -noname WriteFileWithSeek
719 varargs sprintf(ptr str) msvcrt.sprintf
720 stub -noname SystemMemoryLow
721 cdecl vfwprintf(ptr wstr ptr) WINECE_vfwprintf
723 stdcall CreateIconIndirect(ptr) WINECE_CreateIconIndirect
725 stdcall DestroyIcon(long) WINECE_DestroyIcon
726 stdcall DrawIconEx(long long long long long long long long long) WINECE_DrawIconEx
727 stdcall ExtractIconExW(wstr long ptr ptr long) WINECE_ExtractIconExW
728 stdcall LoadIconW(long wstr) WINECE_LoadIconW
729 varargs _snprintf(ptr long str) msvcrt._snprintf
730 stdcall LoadImageW(long wstr long long long long) WINECE_LoadImageW
731 stdcall ClipCursor(ptr) WINECE_ClipCursor
732 stdcall GetClipCursor(ptr) WINECE_GetClipCursor
733 stdcall GetCursor() WINECE_GetCursor
734 stdcall GetCursorPos(ptr) WINECE_GetCursorPos
735 varargs fwscanf(ptr wstr) msvcrt.fwscanf
736 stdcall SetCursorPos(long long) WINECE_SetCursorPos
737 stdcall ShowCursor(long) WINECE_ShowCursor
738 stdcall ImageList_Add(ptr long long) WINECE_ImageList_Add
739 stdcall ImageList_AddMasked(ptr long long) WINECE_ImageList_AddMasked
740 stdcall ImageList_BeginDrag(ptr long long long) WINECE_ImageList_BeginDrag
741 stub -noname ImageList_CopyDitherImage
742 stdcall ImageList_Create(long long long long long) WINECE_ImageList_Create
743 stdcall ImageList_Destroy(ptr) WINECE_ImageList_Destroy
744 stdcall ImageList_DragEnter(long long long) WINECE_ImageList_DragEnter
745 stdcall ImageList_DragLeave(long) WINECE_ImageList_DragLeave
746 stdcall ImageList_DragMove(long long) WINECE_ImageList_DragMove
747 stdcall ImageList_DragShowNolock(long) WINECE_ImageList_DragShowNolock
748 stdcall ImageList_Draw(ptr long long long long long) WINECE_ImageList_Draw
749 stdcall ImageList_DrawEx(ptr long long long long long long long long long) WINECE_ImageList_DrawEx
750 stdcall ImageList_DrawIndirect(ptr) WINECE_ImageList_DrawIndirect
751 stdcall ImageList_EndDrag() WINECE_ImageList_EndDrag
752 stdcall ImageList_GetBkColor(ptr) WINECE_ImageList_GetBkColor
753 stdcall ImageList_GetDragImage(ptr ptr) WINECE_ImageList_GetDragImage
754 stdcall ImageList_GetIcon(ptr long long) WINECE_ImageList_GetIcon
755 stdcall ImageList_GetIconSize(ptr ptr ptr) WINECE_ImageList_GetIconSize
756 stdcall ImageList_GetImageCount(ptr) WINECE_ImageList_GetImageCount
757 stdcall ImageList_GetImageInfo(ptr long ptr) WINECE_ImageList_GetImageInfo
758 stdcall ImageList_LoadImage(long str long long long long long) WINECE_ImageList_LoadImage
759 stdcall ImageList_Merge(ptr long ptr long long long) WINECE_ImageList_Merge
760 stdcall ImageList_Remove(ptr long) WINECE_ImageList_Remove
761 stdcall ImageList_Replace(ptr long long long) WINECE_ImageList_Replace
762 stdcall ImageList_ReplaceIcon(ptr long long) WINECE_ImageList_ReplaceIcon
763 stdcall ImageList_SetBkColor(ptr long) WINECE_ImageList_SetBkColor
764 stdcall ImageList_SetDragCursorImage(ptr long long long) WINECE_ImageList_SetDragCursorImage
765 stdcall ImageList_SetIconSize(ptr long long) WINECE_ImageList_SetIconSize
766 stdcall ImageList_SetOverlayImage(ptr long long) WINECE_ImageList_SetOverlayImage
767 stdcall ImageList_Copy(ptr long ptr long long) WINECE_ImageList_Copy
768 stdcall ImageList_Duplicate(ptr) WINECE_ImageList_Duplicate
769 stdcall ImageList_SetImageCount(ptr long) WINECE_ImageList_SetImageCount
770 stdcall ImmAssociateContext(long long) WINECE_ImmAssociateContext
771 stdcall ImmConfigureIMEW(long long long ptr) WINECE_ImmConfigureIMEW
772 stdcall ImmCreateIMCC(long) WINECE_ImmCreateIMCC
773 stdcall ImmDestroyIMCC(long) WINECE_ImmDestroyIMCC
774 stdcall ImmEnumRegisterWordW(long ptr wstr long wstr ptr) WINECE_ImmEnumRegisterWordW
775 stdcall ImmEscapeW(long long long ptr) WINECE_ImmEscapeW
776 stdcall ImmGenerateMessage(ptr) WINECE_ImmGenerateMessage
777 stdcall ImmGetCandidateListW(long long ptr long) WINECE_ImmGetCandidateListW
778 stdcall ImmGetCandidateListCountW(long ptr) WINECE_ImmGetCandidateListCountW
779 stdcall ImmGetCandidateWindow(long long ptr) WINECE_ImmGetCandidateWindow
780 stdcall ImmGetCompositionFontW(long ptr) WINECE_ImmGetCompositionFontW
781 stdcall ImmGetCompositionStringW(long long ptr long) WINECE_ImmGetCompositionStringW
782 stdcall ImmGetCompositionWindow(long ptr) WINECE_ImmGetCompositionWindow
783 stdcall ImmGetContext(long) WINECE_ImmGetContext
784 stdcall ImmGetConversionListW(long long wstr ptr long long) WINECE_ImmGetConversionListW
785 stdcall ImmGetConversionStatus(long ptr ptr) WINECE_ImmGetConversionStatus
786 stdcall ImmGetDefaultIMEWnd(long) WINECE_ImmGetDefaultIMEWnd
787 stdcall ImmGetDescriptionW(long ptr long) WINECE_ImmGetDescriptionW
788 stdcall ImmGetGuideLineW(long long ptr long) WINECE_ImmGetGuideLineW
789 stdcall ImmGetIMCCLockCount(long) WINECE_ImmGetIMCCLockCount
790 stdcall ImmGetIMCCSize(long) WINECE_ImmGetIMCCSize
791 stdcall ImmGetIMCLockCount(long) WINECE_ImmGetIMCLockCount
792 stdcall ImmGetOpenStatus(long) WINECE_ImmGetOpenStatus
793 stdcall ImmGetProperty(long long) WINECE_ImmGetProperty
794 stdcall ImmGetRegisterWordStyleW(long long ptr) WINECE_ImmGetRegisterWordStyleW
796 stdcall ImmIsUIMessageW(long long long long) WINECE_ImmIsUIMessageW
797 stdcall ImmLockIMC(long) WINECE_ImmLockIMC
798 stdcall ImmLockIMCC(long) WINECE_ImmLockIMCC
800 stdcall ImmNotifyIME(long long long long) WINECE_ImmNotifyIME
801 stdcall ImmReSizeIMCC(long long) WINECE_ImmReSizeIMCC
802 stdcall ImmRegisterWordW(long wstr long wstr) WINECE_ImmRegisterWordW
803 stdcall ImmReleaseContext(long long) WINECE_ImmReleaseContext
804 stub -noname ImmSIPanelState
806 stub -noname ?ImmSetActiveContext@@YAHPAUHWND__@@KH@Z
807 stdcall ImmSetCandidateWindow(long ptr) WINECE_ImmSetCandidateWindow
808 stdcall ImmSetCompositionFontW(long ptr) WINECE_ImmSetCompositionFontW
809 stdcall ImmSetCompositionStringW(long long ptr long ptr long) WINECE_ImmSetCompositionStringW
810 stdcall ImmSetCompositionWindow(long ptr) WINECE_ImmSetCompositionWindow
811 stdcall ImmSetConversionStatus(long long long) WINECE_ImmSetConversionStatus
812 stub -noname ImmSetHotKey
813 stdcall ImmGetHotKey(long ptr ptr ptr) WINECE_ImmGetHotKey
814 stdcall ImmSetOpenStatus(long long) WINECE_ImmSetOpenStatus
815 stdcall ImmSetStatusWindowPos(long ptr) WINECE_ImmSetStatusWindowPos
816 stdcall ImmSimulateHotKey(long long) WINECE_ImmSimulateHotKey
817 stdcall ImmUnlockIMC(long) WINECE_ImmUnlockIMC
818 stdcall ImmUnlockIMCC(long) WINECE_ImmUnlockIMCC
819 stdcall ImmUnregisterWordW(long wstr long wstr) WINECE_ImmUnregisterWordW
820 stub -noname GetMouseMovePoints
821 stub -noname QASetWindowsJournalHook
822 stub -noname QAUnhookWindowsJournalHook
823 stdcall SendInput(long ptr long) WINECE_SendInput
824 stdcall mouse_event(long long long long long) WINECE_mouse_event
825 stub -noname EnableHardwareKeyboard
826 stdcall GetAsyncKeyState(long) WINECE_GetAsyncKeyState
827 stub -noname GetKeyboardStatus
828 stub -noname KeybdGetDeviceInfo
829 stub -noname KeybdInitStates
830 stub -noname KeybdVKeyToUnicode
831 stdcall MapVirtualKeyW(long long) WINECE_MapVirtualKeyW
832 stub -noname PostKeybdMessage
833 stdcall keybd_event(long long long long) WINECE_keybd_event
834 stub -noname GetAsyncShiftFlags
835 stdcall RegisterHotKey(long long long long) WINECE_RegisterHotKey
836 stdcall UnregisterHotKey(long long) WINECE_UnregisterHotKey
837 stub -noname SystemIdleTimerReset
838 stdcall TranslateAcceleratorW(long long ptr) WINECE_TranslateAcceleratorW
839 stub -noname NLedGetDeviceInfo
840 stub -noname NLedSetDevice
841 stdcall InsertMenuW(long long long long ptr) WINECE_InsertMenuW
842 stdcall AppendMenuW(long long long ptr) WINECE_AppendMenuW
843 stdcall RemoveMenu(long long long) WINECE_RemoveMenu
844 stdcall DestroyMenu(long) WINECE_DestroyMenu
845 stdcall TrackPopupMenuEx(long long long long long ptr) WINECE_TrackPopupMenuEx
846 stdcall LoadMenuW(long wstr) WINECE_LoadMenuW
847 stdcall EnableMenuItem(long long long) WINECE_EnableMenuItem
848 stdcall CheckMenuItem(long long long) WINECE_CheckMenuItem
849 stdcall CheckMenuRadioItem(long long long long long) WINECE_CheckMenuRadioItem
850 stdcall DeleteMenu(long long long) WINECE_DeleteMenu
851 stdcall CreateMenu() WINECE_CreateMenu
852 stdcall CreatePopupMenu() WINECE_CreatePopupMenu
853 stdcall SetMenuItemInfoW(long long long ptr) WINECE_SetMenuItemInfoW
854 stdcall GetMenuItemInfoW(long long long ptr) WINECE_GetMenuItemInfoW
855 stdcall GetSubMenu(long long) WINECE_GetSubMenu
856 stdcall DrawMenuBar(long) WINECE_DrawMenuBar
857 stdcall MessageBeep(long) WINECE_MessageBeep
858 stdcall MessageBoxW(long wstr wstr long) WINECE_MessageBoxW
859 stdcall -register DispatchMessageW(ptr) WINECE_DispatchMessageW
860 stdcall GetKeyState(long) WINECE_GetKeyState
861 stdcall -register GetMessageW(ptr long long long) WINECE_GetMessageW
862 stdcall GetMessagePos() WINECE_GetMessagePos
863 stub -noname GetMessageWNoWait
864 stdcall PeekMessageW(ptr long long long long) WINECE_PeekMessageW
865 stdcall PostMessageW(long long long long) WINECE_PostMessageW
866 stdcall PostQuitMessage(long) WINECE_PostQuitMessage
867 varargs fwprintf(ptr wstr) msvcrt.fwprintf
868 stdcall SendMessageW(long long long long) WINECE_SendMessageW
869 stdcall SendNotifyMessageW(long long long long) WINECE_SendNotifyMessageW
870 stdcall -register TranslateMessage(ptr) WINECE_TranslateMessage
871 stdcall MsgWaitForMultipleObjectsEx(long ptr long long long) WINECE_MsgWaitForMultipleObjectsEx
872 stub -noname GetMessageSource
873 stdcall LoadBitmapW(long wstr) WINECE_LoadBitmapW
874 stdcall LoadStringW(long long ptr long) WINECE_LoadStringW
875 stdcall SetTimer(long long long ptr) WINECE_SetTimer
876 stdcall KillTimer(long long) WINECE_KillTimer
877 stub -noname TouchCalibrate
878 stdcall GetClassInfoW(long wstr ptr) WINECE_GetClassInfoW
879 stdcall GetClassLongW(long long) WINECE_GetClassLongW
880 stdcall SetClassLongW(long long long) WINECE_SetClassLongW
881 stub -noname GetClassLong
882 stub -noname SetClassLong
884 stdcall UnregisterClassW(wstr long) WINECE_UnregisterClassW
885 stdcall GetSystemMetrics(long) WINECE_GetSystemMetrics
886 stdcall IsWindowVisible(long) WINECE_IsWindowVisible
887 stdcall AdjustWindowRectEx(ptr long long long) WINECE_AdjustWindowRectEx
888 stdcall GetDoubleClickTime() WINECE_GetDoubleClickTime
889 stdcall GetSysColor(long) WINECE_GetSysColor
890 stdcall SetSysColors(long ptr ptr) WINECE_SetSysColors
891 stdcall RegisterWindowMessageW(wstr) WINECE_RegisterWindowMessageW
892 stub -noname RegisterTaskBar
893 stdcall AddFontResourceW(wstr) WINECE_AddFontResourceW
894 stub -noname CeRemoveFontResource
895 stdcall CreateFontIndirectW(ptr) WINECE_CreateFontIndirectW
896 stdcall ExtTextOutW(long long long long ptr wstr long ptr) WINECE_ExtTextOutW
897 stdcall GetTextExtentExPointW(long wstr long long ptr ptr ptr) WINECE_GetTextExtentExPointW
898 stdcall GetTextMetricsW(long ptr) WINECE_GetTextMetricsW
899 stub -noname PegRemoveFontResource
900 stdcall RemoveFontResourceW(wstr) WINECE_RemoveFontResourceW
901 stdcall CreateBitmap(long long long long ptr) WINECE_CreateBitmap
902 stdcall CreateCompatibleBitmap(long long long) WINECE_CreateCompatibleBitmap
903 stdcall BitBlt(long long long long long long long long long) WINECE_BitBlt
904 stdcall MaskBlt(long long long long long long long long long long long long) WINECE_MaskBlt
905 stdcall StretchBlt(long long long long long long long long long long long) WINECE_StretchBlt
906 stub -noname TransparentImage
907 stdcall RestoreDC(long long) WINECE_RestoreDC
908 stdcall SaveDC(long) WINECE_SaveDC
909 stdcall CreateDCW(wstr wstr wstr ptr) WINECE_CreateDCW
910 stdcall CreateCompatibleDC(long) WINECE_CreateCompatibleDC
911 stdcall DeleteDC(long) WINECE_DeleteDC
912 stdcall DeleteObject(long) WINECE_DeleteObject
913 stdcall GetBkColor(long) WINECE_GetBkColor
914 stdcall GetBkMode(long) WINECE_GetBkMode
915 stdcall GetCurrentObject(long long) WINECE_GetCurrentObject
916 stdcall GetDeviceCaps(long long) WINECE_GetDeviceCaps
917 stdcall GetObjectType(long) WINECE_GetObjectType
918 stdcall GetObjectW(long long ptr) WINECE_GetObjectW
919 stdcall GetStockObject(long) WINECE_GetStockObject
920 stdcall GetTextColor(long) WINECE_GetTextColor
921 stdcall SelectObject(long long) WINECE_SelectObject
922 stdcall SetBkColor(long long) WINECE_SetBkColor
923 stdcall SetBkMode(long long) WINECE_SetBkMode
924 stdcall SetTextColor(long long) WINECE_SetTextColor
925 stdcall CreatePatternBrush(long) WINECE_CreatePatternBrush
926 stdcall CreatePen(long long long) WINECE_CreatePen
927 stdcall FillRgn(long long long) WINECE_FillRgn
928 stdcall SetROP2(long long) WINECE_SetROP2
929 stdcall CreateDIBPatternBrushPt(long long) WINECE_CreateDIBPatternBrushPt
930 stdcall CreatePenIndirect(ptr) WINECE_CreatePenIndirect
931 stdcall CreateSolidBrush(long) WINECE_CreateSolidBrush
932 stdcall DrawEdge(long ptr long long) WINECE_DrawEdge
933 stdcall DrawFocusRect(long ptr) WINECE_DrawFocusRect
934 stdcall Ellipse(long long long long long) WINECE_Ellipse
935 stdcall FillRect(long ptr long) WINECE_FillRect
936 stdcall GetPixel(long long long) WINECE_GetPixel
937 stdcall GetSysColorBrush(long) WINECE_GetSysColorBrush
938 stdcall PatBlt(long long long long long long) WINECE_PatBlt
939 stdcall Polygon(long ptr long) WINECE_Polygon
940 stdcall Polyline(long ptr long) WINECE_Polyline
941 stdcall Rectangle(long long long long long) WINECE_Rectangle
942 stdcall RoundRect(long long long long long long long) WINECE_RoundRect
943 stdcall SetBrushOrgEx(long long long ptr) WINECE_SetBrushOrgEx
944 stdcall SetPixel(long long long long) WINECE_SetPixel
945 stdcall DrawTextW(long wstr long ptr long) WINECE_DrawTextW
946 stub -noname CreateBitmapFromPointer
947 stdcall CreatePalette(ptr) WINECE_CreatePalette
948 stdcall GetNearestPaletteIndex(long long) WINECE_GetNearestPaletteIndex
949 stdcall GetPaletteEntries(long long long ptr) WINECE_GetPaletteEntries
950 stdcall GetSystemPaletteEntries(long long long ptr) WINECE_GetSystemPaletteEntries
951 stdcall SetPaletteEntries(long long long ptr) WINECE_SetPaletteEntries
952 stdcall GetNearestColor(long long) WINECE_GetNearestColor
953 stdcall RealizePalette(long) WINECE_RealizePalette
954 stdcall SelectPalette(long long long) WINECE_SelectPalette
955 stdcall AbortDoc(long) WINECE_AbortDoc
956 stub -noname ?CloseEnhMetaFile@@YAPAUHENHMETAFILE__@@PAUHDC__@@@Z
957 stub -noname ?CreateEnhMetaFileW@@YAPAUHDC__@@PAU1@PBGPBUtagRECT@@1@Z
958 stub -noname ?DeleteEnhMetaFile@@YAHPAUHENHMETAFILE__@@@Z
959 stdcall EndDoc(long) WINECE_EndDoc
960 stdcall EndPage(long) WINECE_EndPage
961 stub -noname ?PlayEnhMetaFile@@YAHPAUHDC__@@PAUHENHMETAFILE__@@PBUtagRECT@@@Z
962 stdcall SetAbortProc(long ptr) WINECE_SetAbortProc
963 stdcall StartDocW(long ptr) WINECE_StartDocW
964 stdcall StartPage(long) WINECE_StartPage
965 stdcall EnumFontFamiliesW(long wstr ptr long) WINECE_EnumFontFamiliesW
966 stdcall EnumFontsW(long wstr ptr long) WINECE_EnumFontsW
967 stdcall GetTextFaceW(long long ptr) WINECE_GetTextFaceW
968 stdcall CombineRgn(long long long long) WINECE_CombineRgn
969 stdcall CreateRectRgnIndirect(ptr) WINECE_CreateRectRgnIndirect
970 stdcall ExcludeClipRect(long long long long long) WINECE_ExcludeClipRect
971 stdcall GetClipBox(long ptr) WINECE_GetClipBox
972 stdcall GetClipRgn(long long) WINECE_GetClipRgn
973 stdcall GetRegionData(long long ptr) WINECE_GetRegionData
974 stdcall GetRgnBox(long ptr) WINECE_GetRgnBox
975 stdcall IntersectClipRect(long long long long long) WINECE_IntersectClipRect
976 stdcall OffsetRgn(long long long) WINECE_OffsetRgn
977 stdcall PtInRegion(long long long) WINECE_PtInRegion
978 stdcall RectInRegion(long ptr) WINECE_RectInRegion
979 stdcall SelectClipRgn(long long) WINECE_SelectClipRgn
980 stdcall CreateRectRgn(long long long long) WINECE_CreateRectRgn
981 stdcall RectVisible(long ptr) WINECE_RectVisible
982 stdcall SetRectRgn(long long long long long) WINECE_SetRectRgn
983 stdcall SetViewportOrgEx(long long long ptr) WINECE_SetViewportOrgEx
984 stub -noname ?SetObjectOwner@@YAHPAX0@Z
985 stdcall ScrollDC(long long long ptr ptr long ptr) WINECE_ScrollDC
986 stdcall EnableEUDC(long) WINECE_EnableEUDC
987 stdcall DrawFrameControl(long ptr long long) WINECE_DrawFrameControl
988 cdecl abs(long) WINECE_abs
989 cdecl acos(double) WINECE_acos
990 cdecl asin(double) WINECE_asin
991 cdecl atan(double) WINECE_atan
992 cdecl atan2(double double) WINECE_atan2
993 cdecl atoi(str) WINECE_atoi
994 cdecl atol(str) WINECE_atol
995 cdecl atof(str) WINECE_atof
996 cdecl _atodbl(ptr str) WINECE__atodbl
997 stub -noname _atoflt
998 cdecl _cabs(long) WINECE__cabs
999 cdecl ceil(double) WINECE_ceil
1000 cdecl _chgsign(double) WINECE__chgsign
1001 cdecl _clearfp() WINECE__clearfp
1002 cdecl _controlfp(long long) WINECE__controlfp
1003 cdecl _copysign(double double) WINECE__copysign
1004 cdecl cos(double) WINECE_cos
1005 cdecl cosh(double) WINECE_cosh
1006 cdecl difftime(long long) WINECE_difftime
1007 cdecl -ret64 div(long long) WINECE_div
1008 cdecl _ecvt(double long ptr ptr) WINECE__ecvt
1009 cdecl exp(double) WINECE_exp
1010 cdecl fabs(double) WINECE_fabs
1011 cdecl _fcvt(double long ptr ptr) WINECE__fcvt
1012 cdecl _finite(double) WINECE__finite
1013 cdecl floor(double) WINECE_floor
1014 cdecl fmod(double double) WINECE_fmod
1015 cdecl _fpclass(double) WINECE__fpclass
1016 cdecl _fpieee_flt(long ptr ptr) WINECE__fpieee_flt
1017 cdecl _fpreset() WINECE__fpreset
1018 cdecl free(ptr) WINECE_free
1019 cdecl frexp(double ptr) WINECE_frexp
1020 stub -noname _frnd
1021 stub -noname _fsqrt
1022 cdecl _gcvt(double long str) WINECE__gcvt
1023 cdecl _hypot(double double) WINECE__hypot
1024 cdecl _isnan(double) WINECE__isnan
1025 cdecl _itoa(long ptr long) WINECE__itoa
1026 cdecl _itow(long ptr long) WINECE__itow
1027 cdecl _j0(double) WINECE__j0
1028 cdecl _j1(double) WINECE__j1
1029 cdecl _jn(long double) WINECE__jn
1030 cdecl labs(long) WINECE_labs
1031 cdecl ldexp(double long) WINECE_ldexp
1032 cdecl ldiv(long long) WINECE_ldiv
1033 cdecl log(double) WINECE_log
1034 cdecl log10(double) WINECE_log10
1035 cdecl _logb(double) WINECE__logb
1036 cdecl -arch=i386 longjmp(ptr long) WINECE_longjmp
1037 cdecl _lrotl(long long) WINECE__lrotl
1038 cdecl _lrotr(long long) WINECE__lrotr
1039 cdecl _ltoa(long ptr long) WINECE__ltoa
1040 cdecl _ltow(long ptr long) WINECE__ltow
1041 cdecl malloc(long) WINECE_malloc
1042 cdecl _memccpy(ptr ptr long long) WINECE__memccpy
1043 cdecl memcmp(ptr ptr long) WINECE_memcmp
1044 cdecl memcpy(ptr ptr long) WINECE_memcpy
1045 cdecl _memicmp(str str long) WINECE__memicmp
1046 cdecl memmove(ptr ptr long) WINECE_memmove
1047 cdecl memset(ptr long long) WINECE_memset
1048 cdecl modf(double ptr) WINECE_modf
1049 cdecl _msize(ptr) WINECE__msize
1050 cdecl _nextafter(double double) WINECE__nextafter
1051 cdecl pow(double double) WINECE_pow
1052 cdecl qsort(ptr long long ptr) WINECE_qsort
1053 cdecl rand() WINECE_rand
1054 cdecl realloc(ptr long) WINECE_realloc
1055 cdecl _rotl(long long) WINECE__rotl
1056 cdecl _rotr(long long) WINECE__rotr
1057 cdecl _scalb(double long) WINECE__scalb
1058 cdecl sin(double) WINECE_sin
1059 cdecl sinh(double) WINECE_sinh
1060 cdecl sqrt(double) WINECE_sqrt
1061 cdecl srand(long) WINECE_srand
1062 cdecl _statusfp() WINECE__statusfp
1063 cdecl strcat(str str) WINECE_strcat
1064 cdecl strchr(str long) WINECE_strchr
1065 cdecl strcmp(str str) WINECE_strcmp
1066 cdecl strcpy(ptr str) WINECE_strcpy
1067 cdecl strcspn(str str) WINECE_strcspn
1068 cdecl strlen(str) WINECE_strlen
1069 cdecl strncat(str str long) WINECE_strncat
1070 cdecl strncmp(str str long) WINECE_strncmp
1071 cdecl strncpy(ptr str long) WINECE_strncpy
1072 cdecl strstr(str str) WINECE_strstr
1073 cdecl strtok(str str) WINECE_strtok
1074 cdecl _swab(str str long) WINECE__swab
1075 cdecl tan(double) WINECE_tan
1076 cdecl tanh(double) WINECE_tanh
1079 cdecl _ultoa(long ptr long) WINECE__ultoa
1080 cdecl _ultow(long ptr long) WINECE__ultow
1081 cdecl wcstod(wstr ptr) WINECE_wcstod
1082 cdecl wcstol(wstr ptr long) WINECE_wcstol
1083 cdecl wcstoul(wstr ptr long) WINECE_wcstoul
1084 cdecl _y0(double) WINECE__y0
1085 cdecl _y1(double) WINECE__y1
1086 cdecl _yn(long double) WINECE__yn
1087 stub -noname _ld12tod
1088 stub -noname _ld12tof
1089 stub -noname __strgtold12
1090 cdecl tolower(long) WINECE_tolower
1091 cdecl toupper(long) WINECE_toupper
1092 cdecl _purecall() WINECE__purecall
1093 stub -noname _fltused
1094 cdecl -arch=win32 ??3@YAXPAX@Z(ptr) msvcrt.??3@YAXPAX@Z
1095 cdecl -arch=win32 ??2@YAPAXI@Z(long) msvcrt.??2@YAPAXI@Z
1096 varargs _snwprintf(ptr long wstr) msvcrt._snwprintf
1097 varargs swprintf(ptr wstr) msvcrt.swprintf
1098 varargs swscanf(wstr wstr) msvcrt.swscanf
1099 cdecl vswprintf(ptr wstr ptr) WINECE_vswprintf
1100 stub -noname _getstdfilex
1101 varargs scanf(str) msvcrt.scanf
1102 varargs printf(str) msvcrt.printf
1103 cdecl vprintf(str ptr) WINECE_vprintf
1104 cdecl getchar() WINECE_getchar
1105 cdecl putchar(long) WINECE_putchar
1106 cdecl gets(str) WINECE_gets
1107 cdecl puts(str) WINECE_puts
1108 cdecl fgetc(ptr) WINECE_fgetc
1109 cdecl fgets(ptr long ptr) WINECE_fgets
1110 cdecl fputc(long ptr) WINECE_fputc
1111 cdecl fputs(str ptr) WINECE_fputs
1112 cdecl ungetc(long ptr) WINECE_ungetc
1113 cdecl fopen(str str) WINECE_fopen
1114 varargs fscanf(ptr str) msvcrt.fscanf
1115 varargs fprintf(ptr str) msvcrt.fprintf
1116 cdecl vfprintf(ptr str ptr) WINECE_vfprintf
1117 cdecl _wfdopen(long wstr) WINECE__wfdopen
1118 cdecl fclose(ptr) WINECE_fclose
1119 cdecl _fcloseall() WINECE__fcloseall
1120 cdecl fread(ptr long long ptr) WINECE_fread
1121 cdecl fwrite(ptr long long ptr) WINECE_fwrite
1122 cdecl fflush(ptr) WINECE_fflush
1123 cdecl _flushall() WINECE__flushall
1124 cdecl _fileno(ptr) WINECE__fileno
1125 cdecl feof(ptr) WINECE_feof
1126 cdecl ferror(ptr) WINECE_ferror
1127 cdecl clearerr(ptr) WINECE_clearerr
1128 cdecl fgetpos(ptr ptr) WINECE_fgetpos
1129 cdecl fsetpos(ptr ptr) WINECE_fsetpos
1130 cdecl fseek(ptr long long) WINECE_fseek
1131 cdecl ftell(ptr) WINECE_ftell
1132 cdecl _vsnwprintf(ptr long wstr ptr) WINECE__vsnwprintf
1133 varargs wscanf(wstr) msvcrt.wscanf
1134 varargs wprintf(wstr) msvcrt.wprintf
1135 cdecl vwprintf(wstr ptr) WINECE_vwprintf
1136 cdecl getwchar() WINECE_getwchar
1137 cdecl putwchar(long) WINECE_putwchar
1138 cdecl _getws(ptr) WINECE__getws
1139 cdecl _putws(wstr) WINECE__putws
1140 cdecl fgetwc(ptr) WINECE_fgetwc
1141 cdecl fputwc(long ptr) WINECE_fputwc
1142 cdecl ungetwc(long ptr) WINECE_ungetwc
1143 cdecl fgetws(ptr long ptr) WINECE_fgetws
1144 cdecl fputws(wstr ptr) WINECE_fputws
1145 cdecl _wfopen(wstr wstr) WINECE__wfopen
1146 cdecl vsprintf(ptr str ptr) WINECE_vsprintf
1147 cdecl _vsnprintf(ptr long str ptr) WINECE__vsnprintf
1148 stdcall GetThreadContext(long ptr) WINECE_GetThreadContext
1149 stub -noname GetStdioPathW
1150 stub -noname SetStdioPathW
1151 stub -noname _InitStdioLib
1152 stdcall RegFlushKey(long) WINECE_RegFlushKey
1153 stub -noname ReadRegistryFromOEM
1154 stub -noname WriteRegistryToOEM
1155 stub -noname WriteDebugLED
1156 stub -noname UnregisterFunc1
1157 stdcall BeginDeferWindowPos(long) WINECE_BeginDeferWindowPos
1158 stdcall DeferWindowPos(long long long long long long long long) WINECE_DeferWindowPos
1159 stdcall EndDeferWindowPos(long) WINECE_EndDeferWindowPos
1160 stdcall GetKeyboardLayoutNameW(ptr) WINECE_GetKeyboardLayoutNameW
1161 stub -noname LockPages
1162 stub -noname UnlockPages
1163 stub -noname SHCreateExplorerInstance
1164 stub -noname CeMountDBVol
1165 stub -noname CeEnumDBVolumes
1166 stdcall TranslateCharsetInfo(ptr ptr long) WINECE_TranslateCharsetInfo
1167 stub -noname CreateFileForMappingW
1168 stub -noname lineSetCurrentLocation
1169 stub -noname SipStatus
1170 stub -noname SipRegisterNotification
1171 stub -noname SipShowIM
1172 stub -noname SipGetInfo
1173 stub -noname SipSetInfo
1174 stub -noname SipEnumIM
1175 stub -noname SipGetCurrentIM
1176 stub -noname SipSetCurrentIM
1177 stdcall GetModuleHandleW(wstr) WINECE_GetModuleHandleW
1179 stub -noname ActivateDevice
1180 stub -noname DeactivateDevice
1182 stdcall ExtEscape(long long long ptr long ptr) WINECE_ExtEscape
1185 stdcall GetDCEx(long long long) WINECE_GetDCEx
1186 stdcall GetThreadTimes(long ptr ptr ptr ptr) WINECE_GetThreadTimes
1187 cdecl _setmode(long long) WINECE__setmode
1189 stub -noname CeFindNextDatabaseEx
1190 stub -noname CeCreateDatabaseEx
1191 stub -noname CeSetDatabaseInfoEx
1192 stub -noname CeOpenDatabaseEx
1193 stub -noname CeDeleteDatabaseEx
1194 stub -noname CeReadRecordPropsEx
1195 stub -noname CeOidGetInfoEx
1196 stub -noname CeFindFirstDatabaseEx
1197 stub -noname CeUnmountDBVol
1198 stdcall ImmCreateContext() WINECE_ImmCreateContext
1199 stdcall ImmDestroyContext(long) WINECE_ImmDestroyContext
1200 stdcall ImmGetStatusWindowPos(long ptr) WINECE_ImmGetStatusWindowPos
1201 cdecl _wfreopen(wstr wstr ptr) WINECE__wfreopen
1202 stdcall SetWindowsHookExW(long long long long) WINECE_SetWindowsHookExW
1203 stdcall UnhookWindowsHookEx(long) WINECE_UnhookWindowsHookEx
1204 stdcall CallNextHookEx(long long long long) WINECE_CallNextHookEx
1205 stdcall ImmAssociateContextEx(long long long) WINECE_ImmAssociateContextEx
1206 stdcall ImmDisableIME(long) WINECE_ImmDisableIME
1207 stdcall ImmGetIMEFileNameW(long ptr long) WINECE_ImmGetIMEFileNameW
1209 stdcall ImmIsIME(long) WINECE_ImmIsIME
1210 stdcall ImmGetVirtualKey(long) WINECE_ImmGetVirtualKey
1211 stdcall ImmGetImeMenuItemsW(long long long ptr ptr long) WINECE_ImmGetImeMenuItemsW
1213 stub -noname IsProcessDying
1214 stub -noname SipSetDefaultRect
1215 stub -noname FlushViewOfFileMaybe
1216 stdcall FreeLibraryAndExitThread(long long) WINECE_FreeLibraryAndExitThread
1217 stub -noname CeFlushDBVol
1218 stub -noname ?DefaultImcGet@@YAKXZ
1219 stub -noname ?DefaultImeWndGet@@YAPAUHWND__@@XZ
1220 stub -noname ?ImmProcessKey@@YAKPAUHWND__@@IJKI@Z
1221 stub -noname ?ImmTranslateMessage@@YAHPAUHWND__@@IIJHIIPAH@Z
1222 stub -noname ImmSetImeWndIMC
1223 stub -noname ?ImmGetUIClassName@@YAPAGXZ
1224 stub -noname GetForegroundInfo
1225 stub -noname GetForegroundKeyboardTarget
1226 stub -noname CeFreeNotification
1227 stub -noname GetCRTStorageEx
1228 stub -noname GetCRTFlags
1229 stdcall GetKeyboardLayout(long) WINECE_GetKeyboardLayout
1230 stub -noname GetProcAddressA
1231 stdcall GetCommandLineW() WINECE_GetCommandLineW
1232 stdcall DisableThreadLibraryCalls(long) WINECE_DisableThreadLibraryCalls
1233 stdcall TryEnterCriticalSection(ptr) WINECE_TryEnterCriticalSection
1234 stdcall GetTempFileNameW(wstr wstr long ptr) WINECE_GetTempFileNameW
1235 stdcall FindFirstFileExW(wstr long ptr long ptr long) WINECE_FindFirstFileExW
1236 stub -noname GetDeviceByIndex
1237 stdcall GetFileAttributesExW(wstr long ptr) WINECE_GetFileAttributesExW
1238 stdcall CreateSemaphoreW(ptr long long wstr) WINECE_CreateSemaphoreW
1239 stdcall ReleaseSemaphore(long long ptr) WINECE_ReleaseSemaphore
1240 stub -noname ComThreadBaseFunc
1241 stdcall LoadLibraryExW(wstr long long) WINECE_LoadLibraryExW
1242 stdcall ImmRequestMessageW(ptr long long) WINECE_ImmRequestMessageW
1244 stub -noname CeSetThreadQuantum
1245 stub -noname CeGetThreadQuantum
1246 stub -noname lineAccept
1247 stub -noname lineAddToConference
1248 stub -noname lineAnswer
1249 stub -noname lineBlindTransfer
1250 stub -noname lineCompleteTransfer
1251 stub -noname lineDevSpecific
1252 stub -noname lineDial
1253 stub -noname lineForward
1254 stub -noname lineGenerateDigits
1255 stub -noname lineGenerateTone
1256 stub -noname lineGetAddressCaps
1257 stub -noname lineGetAddressID
1258 stub -noname lineGetAddressStatus
1259 stub -noname lineGetAppPriority
1260 stub -noname lineGetCallInfo
1261 stub -noname lineGetCallStatus
1262 stub -noname lineGetConfRelatedCalls
1263 stub -noname lineGetIcon
1264 stub -noname lineGetLineDevStatus
1265 stub -noname lineGetMessage
1266 stub -noname lineGetNewCalls
1267 stub -noname lineGetNumRings
1268 stub -noname lineGetProviderList
1269 stub -noname lineGetStatusMessages
1270 stub -noname lineHandoff
1271 stub -noname lineHold
1272 stub -noname lineInitializeEx
1273 stub -noname lineMonitorDigits
1274 stub -noname lineMonitorMedia
1275 stub -noname lineNegotiateExtVersion
1276 stub -noname linePickup
1277 stub -noname linePrepareAddToConference
1278 stub -noname lineRedirect
1279 stub -noname lineReleaseUserUserInfo
1280 stub -noname lineRemoveFromConference
1281 stub -noname lineSendUserUserInfo
1282 stub -noname lineSetAppPriority
1283 stub -noname lineSetCallParams
1284 stub -noname lineSetCallPrivilege
1285 stub -noname lineSetMediaMode
1286 stub -noname lineSetNumRings
1287 stub -noname lineSetTerminal
1288 stub -noname lineSetTollList
1289 stub -noname lineSetupConference
1290 stub -noname lineSetupTransfer
1291 stub -noname lineSwapHold
1292 stub -noname lineUnhold
1293 stub -noname phoneClose
1294 stub -noname phoneConfigDialog
1295 stub -noname phoneDevSpecific
1296 stub -noname phoneGetDevCaps
1297 stub -noname phoneGetGain
1298 stub -noname phoneGetHookSwitch
1299 stub -noname phoneGetIcon
1300 stub -noname phoneGetID
1301 stub -noname phoneGetMessage
1302 stub -noname phoneGetRing
1303 stub -noname phoneGetStatus
1304 stub -noname phoneGetStatusMessages
1305 stub -noname phoneGetVolume
1306 stub -noname phoneInitializeEx
1307 stub -noname phoneNegotiateAPIVersion
1308 stub -noname phoneNegotiateExtVersion
1309 stub -noname phoneOpen
1310 stub -noname phoneSetGain
1311 stub -noname phoneSetHookSwitch
1312 stub -noname phoneSetRing
1313 stub -noname phoneSetStatusMessages
1314 stub -noname phoneSetVolume
1315 stub -noname phoneShutdown
1317 stdcall GetSystemDefaultUILanguage() WINECE_GetSystemDefaultUILanguage
1318 stdcall GetUserDefaultUILanguage() WINECE_GetUserDefaultUILanguage
1319 stub -noname SetUserDefaultUILanguage
1320 stdcall EnumUILanguagesW(ptr long long) WINECE_EnumUILanguagesW
1346 cdecl calloc(long long) WINECE_calloc
1352 stub -noname CeSetUserNotificationEx
1353 stub -noname CeGetUserNotificationHandles
1354 stub -noname CeGetUserNotification
1357 stub -noname CeGetCurrentTrust
1358 stub -noname GetSystemPowerStatusEx2
1395 stub -noname CeGetCallerTrust
1396 stub -noname OpenDeviceKey
1397 stdcall GetDesktopWindow() WINECE_GetDesktopWindow
1398 stdcall SetWindowRgn(long long long) WINECE_SetWindowRgn
1399 stdcall GetWindowRgn(long long) WINECE_GetWindowRgn
1403 cdecl strtod(str ptr) WINECE_strtod
1404 cdecl strtol(str ptr long) WINECE_strtol
1405 cdecl strtoul(str ptr long) WINECE_strtoul
1406 cdecl strpbrk(str str) WINECE_strpbrk
1407 cdecl strrchr(str long) WINECE_strrchr
1408 cdecl strspn(str str) WINECE_strspn
1409 cdecl _strdup(str) WINECE__strdup
1410 cdecl _stricmp(str str) WINECE__stricmp
1411 cdecl _strnicmp(str str long) WINECE__strnicmp
1412 cdecl _strnset(str long long) WINECE__strnset
1413 cdecl _strrev(str) WINECE__strrev
1414 cdecl _strset(str long) WINECE__strset
1415 cdecl _strlwr(str) WINECE__strlwr
1416 cdecl _strupr(str) WINECE__strupr
1417 cdecl _isctype(long long) WINECE__isctype
1418 cdecl -ret64 _atoi64(str) WINECE__atoi64
1419 stdcall InSendMessage() WINECE_InSendMessage
1420 stdcall GetQueueStatus(long) WINECE_GetQueueStatus
1421 stub -noname LoadFSDEx
1424 stdcall RasEnumDevicesW(ptr ptr ptr) WINECE_RasEnumDevicesW
1425 stub -noname CeResyncFilesys
1443 stub -noname CeGetRandomSeed
1446 stub -noname CeMapArgumentArray
1447 stub -noname UpdateNLSInfo
1448 stub -noname PerformCallBack4
1451 stub -noname CeLogData
1452 stub -noname CeLogSetZones
1453 stub -noname AllKeys
1454 stub -noname GetWindowTextWDirect
1455 stub -noname CeSetExtendedPdata
1456 cdecl -arch=win32 ??_U@YAPAXI@Z(long) msvcrt.??_U@YAPAXI@Z
1457 cdecl -arch=win32 ??_V@YAXPAX@Z(ptr) msvcrt.??_V@YAXPAX@Z
1458 stdcall RasGetProjectionInfoW(long long ptr ptr) WINECE_RasGetProjectionInfoW
1459 stdcall VerQueryValueW(ptr wstr ptr ptr) WINECE_VerQueryValueW
1460 stdcall GetFileVersionInfoW(wstr long long ptr) WINECE_GetFileVersionInfoW
1461 stdcall GetFileVersionInfoSizeW(wstr ptr) WINECE_GetFileVersionInfoSizeW
1462 stub -noname SetOOMEvent
1463 stub -noname RasGetLinkStatistics
1464 stub -noname RasGetDispPhoneNumW
1465 stub -noname RasDevConfigDialogEditW
1466 stub -noname CreateLocaleView
1467 stub -noname CeLogReSync
1468 stub -noname CeCreateDatabaseEx2
1469 stub -noname CeOpenDatabaseEx2
1470 stub -noname CeSeekDatabaseEx
1471 stub -noname CeSetDatabaseInfoEx2
1472 stub -noname CeOidGetInfoEx2
1473 stub -noname CeGetDBInformationByHandle
1474 stub -noname ThreadExceptionExit
1475 stub -noname LoadIntChainHandler
1476 stub -noname FreeIntChainHandler
1477 stub -noname GetMessageQueueReadyTimeStamp
1478 stub -noname RegSaveKey
1479 stub -noname RegReplaceKey
1486 stub -noname AllocPhysMem
1487 stub -noname FreePhysMem
1488 stub -noname SHCreateShortcutEx
1489 stub -noname KernelLibIoControl
1490 stub -noname RegisterAFSEx
1491 stdcall -arch=i386 InterlockedExchangeAdd(ptr long ) WINECE_InterlockedExchangeAdd
1492 stdcall -arch=i386 InterlockedCompareExchange(ptr long long) WINECE_InterlockedCompareExchange
1493 stub -noname LoadAnimatedCursor
1494 stub -noname ActivateDeviceEx
1495 stub -noname SendMessageTimeout
1496 stdcall OpenEventW(long long wstr) WINECE_OpenEventW
1497 stub -noname SetProp
1498 stub -noname GetProp
1499 stub -noname RemoveProp
1500 stub -noname EnumPropsEx
1501 stub -noname SetCurrentUser
1502 stub -noname SetUserData
1503 stub -noname GetUserNameExW
1504 stub -noname RequestDeviceNotifications
1505 stub -noname StopDeviceNotifications
1506 stub -noname RegisterTaskBarEx
1507 stub -noname RegisterDesktop
1508 stub -noname ActivateService
1509 stub -noname RegisterService
1510 stub -noname DeregisterService
1511 stub -noname CloseAllServiceHandles
1512 stub -noname CreateServiceHandle
1513 stub -noname GetServiceByIndex
1514 stub -noname ServiceIoControl
1515 stub -noname ServiceAddPort
1516 stub -noname ServiceUnbindPorts
1517 stub -noname EnumServices
1518 stub -noname GetServiceHandle
1519 stdcall GlobalAddAtomW(wstr) WINECE_GlobalAddAtomW
1520 stdcall GlobalDeleteAtom(long) WINECE_GlobalDeleteAtom
1521 stdcall GlobalFindAtomW(wstr) WINECE_GlobalFindAtomW
1522 stdcall MonitorFromPoint(int64 long) WINECE_MonitorFromPoint
1523 stdcall MonitorFromRect(ptr long) WINECE_MonitorFromRect
1524 stdcall MonitorFromWindow(long long) WINECE_MonitorFromWindow
1525 stub -noname GetMonitorInfo
1526 stdcall EnumDisplayMonitors(long ptr ptr long) WINECE_EnumDisplayMonitors
1527 stub -noname GetEventData
1528 stub -noname SetEventData
1529 stub -noname CreateMsgQueue
1530 stub -noname ReadMsgQueue
1531 stub -noname WriteMsgQueue
1532 stub -noname GetMsgQueueInfo
1533 stub -noname CloseMsgQueue
1534 stub -noname SleepTillTick
1535 stdcall DuplicateHandle(long long long ptr long long long) WINECE_DuplicateHandle
1536 stub -noname OpenMsgQueue
1537 stub -noname SetPasswordStatus
1538 stub -noname GetPasswordStatus
1539 stub -noname CreateStaticMapping
1540 stub -noname AccessibilitySoundSentryEvent
1541 stub -noname ImmEnableIME
1542 stub -noname RegOpenProcessKey
1550 cdecl -arch=i386 -norelay __CxxFrameHandler(ptr ptr ptr ptr) WINECE___CxxFrameHandler
1551 stub -noname __CxxThrowException
1552 stub -noname ?set_terminate@std@@YAP6AXXZP6AXXZ@Z
1553 stub -noname ?set_unexpected@std@@YAP6AXXZP6AXXZ@Z
1555 stub -noname ?__set_inconsistency@@YAP6AXXZP6AXXZ@Z
1556 stub -noname ?terminate@std@@YAXXZ
1557 stub -noname ?unexpected@std@@YAXXZ
1558 stub -noname ?_inconsistency@@YAXXZ
1559 cdecl __RTCastToVoid(ptr) WINECE___RTCastToVoid
1560 cdecl __RTtypeid(ptr) WINECE___RTtypeid
1561 cdecl __RTDynamicCast(ptr long ptr ptr long) WINECE___RTDynamicCast
1562 stub -noname ??1type_info@@UAA@XZ
1563 stub -noname ??8type_info@@QBAHABV0@@Z
1564 stub -noname ??9type_info@@QBAHABV0@@Z
1565 stub -noname ?before@type_info@@QBAHABV1@@Z
1566 stub -noname ?name@type_info@@QBAPBDXZ
1567 stub -noname ?raw_name@type_info@@QBAPBDXZ
1568 stub -noname ??0type_info@@AAA@ABV0@@Z
1569 stub -noname ??4type_info@@AAAAAV0@ABV0@@Z
1570 stub -noname ??0exception@std@@QAA@XZ
1571 stub -noname ??0exception@std@@QAA@PBD@Z
1572 stub -noname ??0exception@std@@QAA@ABV01@@Z
1573 stub -noname ??4exception@std@@QAAAAV01@ABV01@@Z
1574 stub -noname ??1exception@std@@UAA@XZ
1575 stub -noname ?what@exception@std@@UBAPBDXZ
1576 stub -noname ??_L@YAXPAXIHP6AX0@Z1@Z
1577 stub -noname ??_N@YAXPAXIHP6AX0@Z1@Z
1578 stub -noname ??_M@YAXPAXIHP6AX0@Z@Z
1579 stub -noname ??_7exception@std@@6B@
1580 stub -noname ??_7type_info@@6B@
1581 stub -noname GetSystemPowerState
1582 stdcall SetSystemPowerState(long long) WINECE_SetSystemPowerState
1583 stub -noname SetPowerRequirement
1584 stub -noname ReleasePowerRequirement
1585 stub -noname RequestPowerNotifications
1586 stub -noname StopPowerNotifications
1588 stub -noname DevicePowerNotify
1589 stub -noname mixerGetControlDetails
1590 stub -noname mixerGetID
1591 stub -noname mixerGetDevCaps
1592 stub -noname mixerGetLineControls
1593 stub -noname mixerGetLineInfo
1594 stub -noname mixerGetNumDevs
1595 stub -noname mixerOpen
1596 stub -noname mixerMessage
1597 stub -noname mixerSetControlDetails
1598 stub -noname mixerClose
1599 stdcall CryptProtectData(ptr wstr ptr ptr ptr long ptr) WINECE_CryptProtectData
1600 stdcall CryptUnprotectData(ptr ptr ptr ptr ptr long ptr) WINECE_CryptUnprotectData
1601 stub -noname CeGenRandom
1602 stub -noname MapCallerPtr
1603 stub -noname MapPtrToProcWithSize
1604 stub -noname RemoteHeapAlloc
1605 stub -noname RemoteHeapReAlloc
1606 stub -noname RemoteHeapFree
1607 stub -noname RemoteHeapSize
1608 cdecl setvbuf(ptr str long long) WINECE_setvbuf
1609 stub -noname RegisterPowerRelationship
1610 stub -noname ReleasePowerRelationship
1611 stub -noname ChangeDisplaySettingsEx
1612 stub -noname ResourceCreateList
1613 stub -noname ResourceRequest
1614 stub -noname ResourceRelease
1615 stdcall InvalidateRgn(long long long) WINECE_InvalidateRgn
1616 stdcall ValidateRgn(long long) WINECE_ValidateRgn
1617 stdcall ExtCreateRegion(ptr long ptr) WINECE_ExtCreateRegion
1618 cdecl -arch=win32 ?_query_new_handler@@YAP6AHI@ZXZ() msvcrt.?_query_new_handler@@YAP6AHI@ZXZ
1619 cdecl ?set_new_handler@@YAP6AXXZP6AXXZ@Z(ptr) msvcrt.?set_new_handler@@YAP6AXXZP6AXXZ@Z
1621 cdecl -ret64 _abs64(int64) WINECE__abs64
1622 stub -noname _byteswap_uint64
1623 stub -noname _byteswap_ulong
1624 stub -noname _byteswap_ushort
1625 stub -noname _CountLeadingOnes
1626 stub -noname _CountLeadingOnes64
1627 stub -noname _CountLeadingSigns
1628 stub -noname _CountLeadingSigns64
1629 stub -noname _CountLeadingZeros
1630 stub -noname _CountLeadingZeros64
1631 stub -noname _CountOneBits
1632 stub -noname _CountOneBits64
1633 stub -noname _isnanf
1634 stub -noname _isunordered
1635 stub -noname _isunorderedf
1636 stub -noname _MulHigh
1637 stub -noname _MulUnsignedHigh
1638 cdecl -ret64 _rotl64(int64 long) WINECE__rotl64
1639 cdecl -ret64 _rotr64(int64 long) WINECE__rotr64
1640 stub -noname ceilf(float)
1641 stub -noname fabsf
1642 stub -noname floorf(float)
1643 stub -noname fmodf(float float)
1644 stub -noname sqrtf(float)
1645 cdecl _XcptFilter(long ptr) WINECE__XcptFilter
1646 stub -noname ??2@YAPAXIABUnothrow_t@std@@@Z
1647 stub -noname ?nothrow@std@@3Unothrow_t@1@B
1648 cdecl ?_set_new_mode@@YAHH@Z(long) msvcrt.?_set_new_mode@@YAHH@Z
1649 cdecl ?_query_new_mode@@YAHXZ() msvcrt.?_query_new_mode@@YAHXZ
1650 cdecl -arch=win32 ?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z(ptr) msvcrt.?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z
1651 stdcall MoveToEx(long long long ptr) WINECE_MoveToEx
1652 stdcall LineTo(long long long) WINECE_LineTo
1653 stdcall GetCurrentPositionEx(long ptr) WINECE_GetCurrentPositionEx
1654 stdcall SetTextAlign(long long) WINECE_SetTextAlign
1655 stdcall GetTextAlign(long) WINECE_GetTextAlign
1658 stub -noname ?_Xlen@std@@YAXXZ
1659 stub -noname ?_Xran@std@@YAXXZ
1660 stub -noname ?_Nomemory@std@@YAXXZ
1661 stub -noname ??_U@YAPAXIABUnothrow_t@std@@@Z
1662 stub -noname ??3@YAXPAXABUnothrow_t@std@@@Z
1663 stub -noname ??_V@YAXPAXABUnothrow_t@std@@@Z
1664 stdcall GetCharWidth32(long long long long) WINECE_GetCharWidth32
1665 stdcall GetDIBColorTable(long long long ptr) WINECE_GetDIBColorTable
1666 stdcall SetDIBColorTable(long long long ptr) WINECE_SetDIBColorTable
1667 stdcall StretchDIBits(long long long long long long long long long ptr ptr long long) WINECE_StretchDIBits
1668 stub -noname DDKReg_GetWindowInfo
1669 stub -noname DDKReg_GetIsrInfo
1670 stub -noname DDKReg_GetPciInfo
1671 stub -noname LoadKernelLibrary
1672 stdcall RedrawWindow(long ptr long long) WINECE_RedrawWindow
1673 stub -noname RasGetEapUserData
1674 stub -noname RasSetEapUserData
1675 stub -noname RasGetEapConnectionData
1676 stub -noname RasSetEapConnectionData
1677 stub -noname QueryInstructionSet
1678 stub -noname SetDevicePower
1679 stub -noname GetDevicePower
1680 stub -noname IsSystemFile
1681 stub -noname CeLogGetZones
1682 stdcall FindFirstChangeNotificationW(wstr long long) WINECE_FindFirstChangeNotificationW
1683 stdcall FindNextChangeNotification(long) WINECE_FindNextChangeNotification
1684 stdcall FindCloseChangeNotification(long) WINECE_FindCloseChangeNotification
1685 stub -noname AFS_FindFirstChangeNotificationW
1686 stub -noname GetUserDirectory
1687 stub -noname AdvertiseInterface
1688 stub -noname CeSetPowerOnEvent
1689 stub -noname StringCchCopyW
1690 stub -noname StringCbCopyW
1691 stub -noname StringCchCopyExW
1692 stub -noname StringCbCopyExW
1693 stub -noname StringCchCatW
1694 stub -noname StringCbCatW
1695 stub -noname StringCchCatExW
1696 stub -noname StringCbCatExW
1697 stub -noname StringCchVPrintfW
1698 stub -noname StringCbVPrintfW
1699 stub -noname StringCchPrintfW
1700 stub -noname StringCbPrintfW
1701 stub -noname StringCchPrintfExW
1702 stub -noname StringCbPrintfExW
1703 stub -noname StringCchVPrintfExW
1704 stub -noname StringCbVPrintfExW
1705 stub -noname StringCchCopyA
1706 stub -noname StringCbCopyA
1707 stub -noname StringCchCopyExA
1708 stub -noname StringCbCopyExA
1709 stub -noname StringCchCatA
1710 stub -noname StringCbCatA
1711 stub -noname StringCchCatExA
1712 stub -noname StringCbCatExA
1713 stub -noname StringCchVPrintfA
1714 stub -noname StringCbVPrintfA
1715 stub -noname StringCchPrintfA
1716 stub -noname StringCbPrintfA
1717 stub -noname StringCchPrintfExA
1718 stub -noname StringCbPrintfExA
1719 stub -noname StringCchVPrintfExA
1720 stub -noname StringCbVPrintfExA
1721 stub -noname GetModuleInformation
1722 stub -noname GwesPowerDown
1723 stub -noname GwesPowerUp
1724 stub -noname VirtualSetAttributes
1725 stdcall SetBitmapBits(long long ptr) WINECE_SetBitmapBits
1726 stdcall SetDIBitsToDevice(long long long long long long long long long ptr ptr long) WINECE_SetDIBitsToDevice
1727 stub -noname GetProcessIDFromIndex
1742 stub -noname StringCchCopyNW
1743 stub -noname StringCbCopyNW
1744 stub -noname StringCchCatNW
1745 stub -noname StringCbCatNW
1746 stub -noname StringCchCatNExW
1747 stub -noname StringCbCatNExW
1748 stub -noname StringCchLengthW
1749 stub -noname StringCbLengthW
1750 stub -noname StringCchCopyNA
1751 stub -noname StringCbCopyNA
1752 stub -noname StringCchCatNA
1753 stub -noname StringCbCatNA
1754 stub -noname StringCchCatNExA
1755 stub -noname StringCbCatNExA
1756 stub -noname StringCchLengthA
1757 stub -noname StringCbLengthA
1758 stdcall IsProcessorFeaturePresent(long) WINECE_IsProcessorFeaturePresent
1759 stub -noname ServiceClosePort
1760 stub -noname GetCallStackSnapshot
1761 stub -noname Int_CreateEventW
1762 stub -noname Int_CloseHandle
1763 stub -noname GradientFill
1764 stub -noname PowerPolicyNotify
1765 stub -noname CacheRangeFlush
1766 stdcall ActivateKeyboardLayout(long long) WINECE_ActivateKeyboardLayout
1767 stdcall GetKeyboardLayoutList(long ptr) WINECE_GetKeyboardLayoutList
1768 stdcall LoadKeyboardLayoutW(wstr long) WINECE_LoadKeyboardLayoutW
1769 stub -noname ImmGetKeyboardLayout
1770 stdcall InvertRect(long ptr) WINECE_InvertRect
1771 stdcall GetKeyboardType(long) WINECE_GetKeyboardType
1775 stub -noname CeSetProcessVersion
1776 stub -noname DecompressBinaryBlock
1777 stub -noname EnumDisplaySettings
1778 stub -noname EnumDisplayDevices
1779 stub -noname GetCharABCWidths
1780 stub -noname PageOutModule
1781 stub -noname CeZeroPointer
1789 stub -noname A_SHAInit
1790 stub -noname A_SHAUpdate
1791 stub -noname A_SHAFinal
1792 stub -noname MD5Init
1793 stub -noname MD5Update
1794 stub -noname MD5Final
1795 stub -noname SetUserDefaultLCID
1796 stub -noname UpdateNLSInfoEx
1797 stub -noname InterruptMask
1798 stub -noname CeGetFileNotificationInfo
1799 stub -noname ReinitLocale
1800 stub -noname ProfileCaptureStatus
1801 stub -noname ProfileStartEx
1802 stub -noname GetForegroundKeyboardLayoutHandle
1810 stub -noname ShowStartupWindow
1811 stub -noname GetThreadCallStack
1812 stub -noname CeVirtualSharedAlloc
1813 stub -noname waveOutGetProperty
1814 stub -noname waveOutSetProperty
1815 stub -noname waveInGetProperty
1816 stub -noname waveInSetProperty
1818 stub -noname ClearEventLogW
1819 stub -noname ReportEventW
1820 stub -noname RegisterEventSourceW
1821 stub -noname DeregisterEventSource
1822 stdcall GetIconInfo(long ptr) WINECE_GetIconInfo
1823 stub -noname CeGetNotificationThreadId
1824 stdcall GetStretchBltMode(long) WINECE_GetStretchBltMode
1825 stdcall SetStretchBltMode(long long) WINECE_SetStretchBltMode
1826 stub -noname DeleteStaticMapping
1827 stub -noname VerifyUser
1828 stub -noname LASSReloadConfig
1829 stub -noname ForcePixelDoubling
1830 stub -noname IsForcePixelDoubling
1831 stdcall ReadFileScatter(long ptr long ptr ptr) WINECE_ReadFileScatter
1832 stdcall WriteFileGather(long ptr long ptr ptr) WINECE_WriteFileGather
1833 stub -noname ResourceRequestEx
1834 stub -noname ResourceMarkAsShareable
1835 stub -noname ResourceDestroyList
1836 stub -noname CeHeapCreate
1837 stdcall -ordinal DPA_Create(long) WINECE_DPA_Create
1838 stdcall -ordinal DPA_CreateEx(long long) WINECE_DPA_CreateEx
1839 stdcall -ordinal DPA_Clone(ptr ptr) WINECE_DPA_Clone
1840 stdcall -ordinal DPA_DeleteAllPtrs(ptr) WINECE_DPA_DeleteAllPtrs
1841 stdcall -ordinal DPA_DeletePtr(ptr long) WINECE_DPA_DeletePtr
1842 stdcall -ordinal DPA_Destroy(ptr) WINECE_DPA_Destroy
1843 stdcall -ordinal DPA_DestroyCallback(ptr ptr long) WINECE_DPA_DestroyCallback
1844 stdcall -ordinal DPA_EnumCallback(long long long) WINECE_DPA_EnumCallback
1845 stdcall -ordinal DPA_GetPtr(ptr long) WINECE_DPA_GetPtr
1846 stdcall -ordinal DPA_GetPtrIndex(ptr ptr) WINECE_DPA_GetPtrIndex
1847 stdcall -ordinal DPA_Grow(ptr long) WINECE_DPA_Grow
1848 stdcall -ordinal DPA_InsertPtr(ptr long ptr) WINECE_DPA_InsertPtr
1849 stdcall -ordinal DPA_Search(ptr ptr long ptr long long) WINECE_DPA_Search
1850 stdcall -ordinal DPA_SetPtr(ptr long ptr) WINECE_DPA_SetPtr
1851 stdcall -ordinal DPA_Sort(ptr ptr long) WINECE_DPA_Sort
1852 stdcall -ordinal DSA_Create(long long) WINECE_DSA_Create
1853 stub -noname DSA_Clone
1854 stdcall -ordinal DSA_DeleteAllItems(ptr) WINECE_DSA_DeleteAllItems
1855 stdcall -ordinal DSA_DeleteItem(ptr long) WINECE_DSA_DeleteItem
1856 stdcall -ordinal DSA_Destroy(ptr) WINECE_DSA_Destroy
1857 stdcall -ordinal DSA_DestroyCallback(ptr ptr long) WINECE_DSA_DestroyCallback
1858 stdcall -ordinal DSA_EnumCallback(ptr ptr long) WINECE_DSA_EnumCallback
1859 stdcall -ordinal DSA_GetItem(ptr long long) WINECE_DSA_GetItem
1860 stdcall -ordinal DSA_GetItemPtr(ptr long) WINECE_DSA_GetItemPtr
1861 stub -noname DSA_Grow
1862 stdcall -ordinal DSA_InsertItem(ptr long long) WINECE_DSA_InsertItem
1863 stub -noname DSA_Search
1864 stdcall -ordinal DSA_SetItem(ptr long long) WINECE_DSA_SetItem
1865 stub -noname DSA_SetRange
1866 stub -noname DSA_Sort
1867 stub -noname GetGweApiSetTables
1868 stub -noname StringCchCopyNExW
1869 stub -noname StringCbCopyNExW
1870 stub -noname GetDeviceInformationByDeviceHandle
1871 stub -noname GetDeviceInformationByFileHandle
1872 stub -noname FindFirstDevice
1873 stub -noname FindNextDevice
1874 stub -noname EnumDeviceInterfaces
1875 stub -noname __security_gen_cookie
1876 stub -noname __report_gsfailure
1877 stdcall MulDiv(long long long) WINECE_MulDiv
1878 stub -noname CredWrite
1879 stub -noname CredRead
1880 stub -noname CredUpdate
1881 stub -noname CredDelete
1882 stub -noname CredFree
1883 stub -noname AlphaBlend
1884 stdcall HeapCompact(long long) WINECE_HeapCompact
1885 stdcall EnumFontFamiliesExW(long ptr ptr long long) WINECE_EnumFontFamiliesExW
1886 stub -noname GetNlsTables
1887 stdcall GetCharABCWidthsI(long long long ptr ptr) WINECE_GetCharABCWidthsI
1888 stdcall GetFontData(long long long ptr long) WINECE_GetFontData
1889 stdcall GetOutlineTextMetricsW(long long ptr) WINECE_GetOutlineTextMetricsW
1890 stdcall SetLayout(long long) WINECE_SetLayout
1891 stdcall GetLayout(long) WINECE_GetLayout
1892 stub -noname CeAddDatabaseProps
1893 stub -noname CeAddSyncPartner
1894 stub -noname CeAttachCustomTrackingData
1895 stub -noname CeBeginSyncSession
1896 stub -noname CeBeginTransaction
1897 stub -noname CeCreateDatabaseWithProps
1898 stub -noname CeCreateSession
1899 stub -noname CeEndSyncSession
1900 stub -noname CeEndTransaction
1901 stub -noname CeFindNextChangedRecord
1902 stub -noname CeGetChangedRecordCnt
1903 stub -noname CeGetChangedRecords
1904 stub -noname CeGetCustomTrackingData
1905 stub -noname CeGetDatabaseProps
1906 stub -noname CeGetDatabaseSession
1907 stub -noname CeGetPropChangeInfo
1908 stub -noname CeGetRecordChangeInfo
1909 stub -noname CeMarkRecord
1910 stub -noname CeMountDBVolEx
1911 stub -noname CeOpenDatabaseInSession
1912 stub -noname CeOpenStream
1913 stub -noname CePurgeTrackingData
1914 stub -noname CePurgeTrackingGenerations
1915 stub -noname CeRemoveDatabaseProps
1916 stub -noname CeRemoveDatabaseTracking
1917 stub -noname CeRemoveSyncPartner
1918 stub -noname CeSetSessionOption
1919 stub -noname CeStreamRead
1920 stub -noname CeStreamSaveChanges
1921 stub -noname CeStreamSeek
1922 stub -noname CeStreamSetSize
1923 stub -noname CeStreamWrite
1924 stub -noname CeTrackDatabase
1925 stub -noname CeTrackProperty
1950 stub -noname CeFindFirstRegChange
1951 stub -noname CeFindNextRegChange
1952 stub -noname CeFindCloseRegChange
1953 stub -noname ConnectHdstub
1954 stub -noname ConnectOsAxsT0
1955 stub -noname AttachHdstub
1956 stub -noname AttachOsAxsT0
1957 stub -noname CeGetCanonicalPathNameW
1958 stdcall CopyFileExW(wstr wstr ptr ptr ptr long) WINECE_CopyFileExW
1959 stub -noname MatchesWildcardMask
1960 stub -noname CaptureDumpFileOnDevice
1961 stub -noname GetDeviceHandleFromContext
1962 stdcall SetTextCharacterExtra(long long) WINECE_SetTextCharacterExtra
1963 stdcall GetTextCharacterExtra(long) WINECE_GetTextCharacterExtra
1964 stub -noname ReportFault
1965 stub -noname CeFsIoControlW
1966 stub -noname AFS_FsIoControlW
1967 cdecl ?uncaught_exception@std@@YA_NXZ() msvcp71.?uncaught_exception@std@@YA_NXZ
1968 stdcall LockFileEx(long long long long long ptr) WINECE_LockFileEx
1969 stdcall UnlockFileEx(long long long long ptr) WINECE_UnlockFileEx
1970 stub -noname OpenEventLogW
1971 stub -noname CloseEventLog
1972 stub -noname BackupEventLogW
1973 stub -noname LockEventLog
1974 stub -noname UnLockEventLog
1975 stub -noname ReadEventLogRaw
1976 stub -noname CreateEnrollmentConfigDialog
1977 stdcall -ordinal SHLoadIndirectString(wstr ptr long ptr) WINECE_SHLoadIndirectString
1978 stub -noname CeGetVolumeInfoW
1979 stub -noname ImmActivateLayout
1980 stub -noname ImmSendNotification
1981 stub -noname IsNamedEventSignaled
1982 stub -noname AttachOsAxsT1
1983 stub -noname ConnectOsAxsT1
1984 stdcall SetWindowOrgEx(long long long ptr) WINECE_SetWindowOrgEx
1985 stdcall GetWindowOrgEx(long ptr) WINECE_GetWindowOrgEx
1986 stdcall GetWindowExtEx(long ptr) WINECE_GetWindowExtEx
1987 stdcall OffsetViewportOrgEx(long long long ptr) WINECE_OffsetViewportOrgEx
1988 stdcall GetViewportOrgEx(long ptr) WINECE_GetViewportOrgEx
1989 stdcall GetViewportExtEx(long ptr) WINECE_GetViewportExtEx
1990 stdcall GetROP2(long) WINECE_GetROP2
1991 stdcall DebugActiveProcessStop(long) WINECE_DebugActiveProcessStop
1992 stdcall DebugSetProcessKillOnExit(long) WINECE_DebugSetProcessKillOnExit
1993 stub -noname GetDeviceUniqueID
1994 stub -noname CeGetModuleInfo
1995 stub -noname RequestBluetoothNotifications
1996 stub -noname StopBluetoothNotifications
1997 stub -noname Int_HeapCreate
1998 stub -noname Int_HeapDestroy
1999 stub -noname HeapAllocTrace
2000 stub -noname __rt_sdiv64by64
2001 stub -noname __rt_srem64by64
2002 stub -noname __rt_udiv64by64
2003 stub -noname __rt_urem64by64
2005 stdcall __rt_sdiv(long long) COREDLL_rt_sdiv
2006 stub -noname __rt_sdiv10
2008 stdcall __rt_udiv(long long) COREDLL_rt_udiv
2009 stub -noname __rt_udiv10
2010 stub -noname __rt_srsh
2011 stub -noname __rt_ursh
2012 stub -noname __utod
2013 stub -noname __u64tos
2014 stub -noname __u64tod
2015 stub -noname __subs
2016 stub -noname __subd
2017 stub -noname __stou64
2018 stub -noname __stou
2019 stub -noname __stoi64
2020 stub -noname __stoi
2021 stub -noname __stod
2022 stub -noname __nes
2023 stub -noname __negs
2024 stub -noname __negd
2025 stub -noname __ned
2026 stub -noname __muls
2027 stub -noname __muld
2028 stub -noname __lts
2029 stub -noname __ltd
2030 stub -noname __les
2031 stub -noname __led
2032 stub -noname __itos
2033 stub -noname __itod
2034 stub -noname __i64tos
2035 stub -noname __i64tod
2036 stub -noname __gts
2037 stub -noname __gtd
2038 stub -noname __ges
2039 stub -noname __ged
2040 stub -noname __eqs
2041 stub -noname __eqd
2042 stub -noname __dtou64
2043 stub -noname __dtou
2044 stub -noname __dtos
2045 stub -noname __dtoi64
2046 stub -noname __dtoi
2047 stub -noname __divs
2048 stub -noname __divd
2049 stub -noname __cmps
2050 stub -noname __cmpd
2051 stub -noname __adds
2052 stub -noname __utos
2053 stub -noname __addd
2054 stub -noname -norelay -private setjmp(ptr)
2055 stub -noname _mbmemset
2500 stub -noname Int_HeapAlloc
2501 stub -noname Int_HeapReAlloc
2502 stub -noname Int_HeapSize
2503 stub -noname Int_HeapFree
2504 stub -noname CeRegTestSetValueW
2505 stub -noname CeRegGetInfo
2506 stub -noname CeRegGetNotificationInfo
2507 stdcall CheckRemoteDebuggerPresent(long ptr) WINECE_CheckRemoteDebuggerPresent
2508 stub -noname CeSafeCopyMemory
2509 stub -noname CeCertVerify
2510 stub -noname CeGetProcessTrust
2511 stub -noname CeOpenFileHandle
