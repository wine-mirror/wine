name shlwapi
type win32
init SHLWAPI_LibMain

import advapi32.dll
import user32.dll
import gdi32.dll
import kernel32.dll
import ntdll.dll

debug_channels (shell)

1   stdcall @(ptr ptr) SHLWAPI_1
2   stdcall @(wstr ptr) SHLWAPI_2
3   stub @
4   stub @
5   stub @
6   stub @
7   stub @
8   stub @
9   stub @
10  stub @
11  stub @
12  stub @
13  stub @
14  stub @
15  stub @
16  stdcall @(long long long long) SHLWAPI_16
17  stub @
18  stub @
19  stub @
20  stub @
21  stub @
22  stub @
23  stdcall @(ptr ptr long) SHLWAPI_23
24  stdcall @(ptr ptr long) SHLWAPI_24
25  stub @
26  stub @
27  stub @
28  stub @
29  stub @
30  stub @
31  stub @
32  stdcall @(ptr) SHLWAPI_32
33  stub @
34  stub @
35  stub @
36  stub @
37  forward @ user32.CallWindowProcW
38  forward @ user32.CharLowerW
39  forward @ user32.CharLowerBuffW
40  stdcall @(wstr) SHLWAPI_40
41  stub @
42  stub @
43  forward @ user32.CharUpperW
44  forward @ user32.CharUpperBuffW
45  forward @ kernel32.CompareStringW
46  forward @ user32.CopyAcceleratorTableW
47  forward @ user32.CreateAcceleratorTableW
48  forward @ gdi32.CreateDCW
49  stub @
50  forward @ kernel32.CreateDirectoryW
51  forward @ kernel32.CreateEventW
52  forward @ kernel32.CreateFileW
53  forward @ gdi32.CreateFontIndirectW
54  forward @ gdi32.CreateICW
55  forward @ user32.CreateWindowExW
56  forward @ user32.DefWindowProcW
57  forward @ kernel32.DeleteFileW
58  stub @
59  stub @
60  forward @ user32.DispatchMessageW
61  stub @
62  forward @ gdi32.EnumFontFamiliesW
63  forward @ gdi32.EnumFontFamiliesExW
64  forward @ kernel32.EnumResourceNamesW
65  forward @ kernel32.FindFirstFileW
66  forward @ kernel32.FindResourceW
67  forward @ user32.FindWindowW
68  forward @ kernel32.FormatMessageW
69  forward @ user32.GetClassInfoW
70  forward @ user32.GetClassLongW
71  forward @ user32.GetClassNameW
72  forward @ user32.GetClipboardFormatNameW
73  forward @ kernel32.GetCurrentDirectoryW
74  stdcall @(long long wstr long) SHLWAPI_74
75  forward @ kernel32.GetFileAttributesW
76  forward @ kernel32.GetFullPathNameW
77  forward @ kernel32.GetLocaleInfoW
78  forward @ user32.GetMenuStringW
79  forward @ user32.GetMessageW
80  forward @ kernel32.GetModuleFileNameW
81  forward @ kernel32.GetSystemDirectoryW
82  forward @ kernel32.SearchPathW
83  forward @ kernel32.GetModuleHandleW
84  forward @ gdi32.GetObjectW
85  forward @ kernel32.GetPrivateProfileIntW
86  forward @ kernel32.GetProfileStringW
87  forward @ user32.GetPropW
88  forward @ kernel32.GetStringTypeExW
89  forward @ kernel32.GetTempFileNameW
90  forward @ kernel32.GetTempPathW
91  forward @ gdi32.GetTextExtentPoint32W
92  forward @ gdi32.GetTextFaceW
93  forward @ gdi32.GetTextMetricsW
94  forward @ user32.GetWindowLongW
95  forward @ user32.GetWindowTextW
96  forward @ user32.GetWindowTextLengthW
97  forward @ kernel32.GetWindowsDirectoryW
98  forward @ user32.InsertMenuW
99  forward @ user32.IsDialogMessageW
100 forward @ user32.LoadAcceleratorsW
101 forward @ user32.LoadBitmapW
102 forward @ user32.LoadCursorW
103 forward @ user32.LoadIconW
104 forward @ user32.LoadImageW
105 forward @ kernel32.LoadLibraryExW
106 forward @ user32.LoadMenuW
107 forward @ user32.LoadStringW
108 forward @ user32.MessageBoxIndirectW
109 forward @ user32.ModifyMenuW
110 forward @ gdi32.GetCharWidth32W
111 forward @ gdi32.GetCharacterPlacementW
112 forward @ kernel32.CopyFileW
113 forward @ kernel32.MoveFileW
114 forward @ user32.OemToCharW
115 forward @ kernel32.OutputDebugStringW
116 forward @ user32.PeekMessageW
117 forward @ user32.PostMessageW
118 forward @ user32.PostThreadMessageW
119 forward @ advapi32.RegCreateKeyW
120 forward @ advapi32.RegCreateKeyExW
121 forward @ advapi32.RegDeleteKeyW
122 forward @ advapi32.RegEnumKeyW
123 forward @ advapi32.RegEnumKeyExW
124 forward @ advapi32.RegOpenKeyW
125 forward @ advapi32.RegOpenKeyExW
126 forward @ advapi32.RegQueryInfoKeyW
127 forward @ advapi32.RegQueryValueW
128 forward @ advapi32.RegQueryValueExW
129 forward @ advapi32.RegSetValueW
130 forward @ advapi32.RegSetValueExW
131 forward @ user32.RegisterClassW
132 forward @ user32.RegisterClipboardFormatW
133 forward @ user32.RegisterWindowMessageW
134 forward @ user32.RemovePropW
135 forward @ user32.SendDlgItemMessageW
136 forward @ user32.SendMessageW
137 forward @ kernel32.SetCurrentDirectoryW
138 stub @
139 stub @
140 forward @ user32.SetPropW
141 forward @ user32.SetWindowLongW
142 forward @ user32.SetWindowsHookExW
143 stub @
144 forward @ gdi32.StartDocW
145 forward @ user32.SystemParametersInfoW
146 forward @ user32.TranslateAcceleratorW
147 forward @ user32.UnregisterClassW
148 forward @ user32.VkKeyScanW
149 forward @ user32.WinHelpW
150 forward @ user32.wvsprintfW
151 stdcall @(str ptr long) SHLWAPI_151
152 stdcall @(wstr wstr long) SHLWAPI_152
153 stdcall @(long long long) SHLWAPI_153
154 stub @
155 stub @
156 stdcall @(wstr wstr) SHLWAPI_156
157 stub @
158 stub @ #(wstr wstr) SHLWAPI_158
159 forward @ kernel32.CompareStringW
160 stub @
161 stub @
162 stdcall @(str long) SHLWAPI_162
163 stub @
164 stub @
165 stdcall @(long long long long) SHLWAPI_165
166 stub @
167 stub @
168 stub @
169 stdcall @(long) SHLWAPI_169
170 stdcall @(str) SHLWAPI_170
171 stub @
172 stub @
173 stub @
174 stub @
175 stub @
176 stub @
177 stub @
178 stub @
179 stub @
180 stub @
181 stdcall @(long long long) SHLWAPI_181
182 stub @
183 stdcall @(ptr) SHLWAPI_183
184 stub @
185 stub @
186 stub @
187 stub @
188 stub @
189 stub @
190 stub @
191 stub @
192 stub @
193 stdcall @() SHLWAPI_193
194 stub @
195 stub @
196 stub @
197 stub @
198 stub @
199 stub @
200 stub @
201 stub @
202 stub @
203 stub @
204 stub @
205 stdcall @(long str str ptr ptr ptr) SHLWAPI_205
206 stdcall @(long wstr wstr ptr ptr ptr) SHLWAPI_206
207 stub @
208 stub @
209 stub @
210 stub @
211 stub @
212 stub @
213 stub @
214 stub @
215 stdcall @(long long long) SHLWAPI_215
216 stub @
217 stdcall @(wstr str ptr) SHLWAPI_217
218 stdcall @(long wstr str ptr) SHLWAPI_218
219 stdcall @(long long long long) SHLWAPI_219
220 stub @
221 stub @
222 stdcall @(long) SHLWAPI_222
223 stdcall @(long) SHLWAPI_223
224 stub @
225 stub @
226 stub @
227 stub @
228 stub @
229 stub @
230 stub @
231 stub @
232 stub @
233 stub @
234 stub @
235 stub @
236 stub @
237 stdcall @(ptr) SHLWAPI_237
238 stub @
239 stub @
240 stdcall @(long long long long) SHLWAPI_240
241 stdcall @() SHLWAPI_241
242 stub @
243 stub @
244 stub @
245 stub @
246 stub @
247 stub @
248 stub @
249 stub @
250 stub @
251 stub @
252 stub @
253 stub AssocCreate
254 stub AssocQueryKeyA
255 stub AssocQueryKeyW
256 stub @
257 stub @
258 stub @
259 stub @
260 stub @
261 stub @
262 stub @
263 stub @
264 stub @
265 stub @
266 stdcall @(long long long long) SHLWAPI_266
267 stdcall @(long long long long) SHLWAPI_267
268 stdcall @(long long) SHLWAPI_268
269 stub @
270 stub @
271 stub @
272 stub @
273 stub @
274 stub @
275 stub @
276 stdcall @() SHLWAPI_276
277 stub @
278 stdcall @(long long long long long long) SHLWAPI_278
279 stub @
280 stub @
281 stub @
282 stub @
283 stub @
284 stub @
285 stub @
286 stub @
287 stub @
288 stub @
289 stdcall @(wstr long long) SHLWAPI_289
290 stub @
291 stub @
292 stub @
293 stub @
294 stdcall @(long long long long long) SHLWAPI_294
295 stub @
296 stub @
297 stub @
298 forward @ kernel32.WritePrivateProfileStringW
299 stub @
300 stub @
301 forward @ gdi32.DrawTextExW # FIXME CreateFontW
302 forward @ user32.GetMenuItemInfoW
303 forward @ user32.InsertMenuItemW
304 forward @ gdi32.CreateMetaFileW
305 forward @ kernel32.CreateMutexW
306 forward @ kernel32.ExpandEnvironmentStringsW
307 forward @ kernel32.CreateSemaphoreW
308 forward @ kernel32.IsBadStringPtrW
309 forward @ kernel32.LoadLibraryW
310 forward @ kernel32.GetTimeFormatW
311 forward @ kernel32.GetDateFormatW
312 forward @ kernel32.GetPrivateProfileStringW
313 stdcall @(ptr long ptr long long) SHLWAPI_313
314 forward @ user32.RegisterClassExW
315 forward @ user32.GetClassInfoExW
316 stub SHCreateStreamOnFileAOld
317 stub SHCreateStreamOnFileWOld
318 stdcall @(long long wstr long) SHLWAPI_318
319 forward @ user32.FindWindowExW
320 stdcall @(str str) SHLWAPI_320
321 stdcall @(wstr wstr) SHLWAPI_321
322 stdcall @(str) SHLWAPI_322
323 stdcall @(wstr) SHLWAPI_323
324 stub @
325 stub @
326 stub @
327 stub @
328 stub @
329 stub @
330 stub @
331 stub @
332 forward @ user32.CallMsgFilterW
333 stdcall @(ptr) SHLWAPI_333
334 stdcall @(ptr wstr) SHLWAPI_334
335 stdcall @(ptr) SHLWAPI_335
336 stdcall @(ptr) SHLWAPI_336
337 stdcall @(wstr long ptr ptr long) SHLWAPI_337
338 forward @ kernel32.SetFileAttributesW
339 forward @ kernel32.GetNumberFormatW
340 forward @ user32.MessageBoxW
341 forward @ kernel32.FindNextFileW
342 stdcall @(long long long long) SHLWAPI_342
343 stub @
344 stub @
345 stub @
346 stdcall @(wstr ptr long) SHLWAPI_346
347 forward @ advapi32.RegDeleteValueW
348 stub @
349 stub @
350 stub @
351 stub @
352 stub @
353 stub @
354 stub @
355 stub @
356 stub @
357 stdcall @(wstr wstr wstr long long) SHLWAPI_357
358 stdcall @(ptr ptr ptr ptr ptr ptr) SHLWAPI_358
359 forward @ kernel32.OpenEventW
360 forward @ kernel32.RemoveDirectoryW
361 forward @ kernel32.GetShortPathNameW
362 stub @
363 stub @
364 stdcall @(str str long) SHLWAPI_364
365 stub @
366 forward @ advapi32.RegEnumValueW
367 forward @ kernel32.WritePrivateProfileStructW
368 forward @ kernel32.GetPrivateProfileStructW
369 forward @ kernel32.CreateProcessW
370 stdcall @(long wstr long) SHLWAPI_370
371 stub @
372 stub @
373 stub @
374 stub @
375 stub @
376 stdcall @(long) SHLWAPI_376  # kernel32.GetUserDefaultUILanguage
377 stdcall @(long long long) SHLWAPI_377
378 stdcall @(long long long) SHLWAPI_378
379 stub @
380 stub @
381 stub AssocQueryStringA
382 stub AssocQueryStringByKeyA
383 stub AssocQueryStringByKeyW
384 stub AssocQueryStringW
385 stub ChrCmpIA
386 stub ChrCmpIW
387 stub ColorAdjustLuma
388 stub @
389 stdcall @(ptr) SHLWAPI_389
390 stdcall @(ptr ptr) SHLWAPI_390
391 stdcall @(ptr ptr ptr ptr ptr ) SHLWAPI_391
392 stub @
393 stub @
394 stub @
395 stub @
396 stub @
397 stub @
398 stub @
399 stub @
400 stub @
401 stdcall @(ptr) SHLWAPI_401
402 stdcall @(ptr) SHLWAPI_402
403 stdcall @(ptr) SHLWAPI_403
404 stdcall ColorHLSToRGB(long long long) ColorHLSToRGB
405 stub @
406 stub @
407 stub @
408 stub @
409 stub @
410 stub @
411 stub @
412 stub @
413 stub @
414 stub @
415 stub @
416 stub @
417 stub @
418 stub @
419 stub @
420 stub @
421 stub @
422 stub @
423 stub @
424 stub @
425 stub @
426 stub @
427 stub @
428 stub @
429 stub @
430 stub @
431 stdcall @(long) SHLWAPI_431
432 stub @
433 stub @
434 forward @ user32.SendMessageTimeoutW
435 stub @
436 stub @
437 stdcall @(long) SHLWAPI_437
438 stub @
439 stub @
440 stub @
441 stub @
442 forward @ kernel32.GetEnvironmentVariableW
443 forward @ kernel32.GetSystemWindowsDirectoryA
444 forward @ kernel32.GetSystemWindowsDirectoryW
445 stub ColorRGBToHLS
446 stub @

@ stdcall DllGetVersion (ptr) SHLWAPI_DllGetVersion
@ stdcall GetMenuPosFromID(ptr long) GetMenuPosFromID
@ stdcall HashData (ptr long ptr long) HashData
@ stub    IntlStrEqWorkerA
@ stub    IntlStrEqWorkerW
@ stdcall PathAddBackslashA (str) PathAddBackslashA
@ stdcall PathAddBackslashW (wstr) PathAddBackslashW
@ stdcall PathAddExtensionA (str str) PathAddExtensionA
@ stdcall PathAddExtensionW (wstr wstr) PathAddExtensionW
@ stdcall PathAppendA (str str) PathAppendA
@ stdcall PathAppendW (wstr wstr) PathAppendW
@ stdcall PathBuildRootA (ptr long) PathBuildRootA
@ stdcall PathBuildRootW (ptr long) PathBuildRootW
@ stdcall PathCanonicalizeA (ptr str) PathCanonicalizeA
@ stdcall PathCanonicalizeW (ptr wstr) PathCanonicalizeW
@ stdcall PathCombineA (ptr ptr ptr) PathCombineA
@ stdcall PathCombineW (ptr ptr ptr) PathCombineW
@ stdcall PathCommonPrefixA(str str ptr)PathCommonPrefixA
@ stdcall PathCommonPrefixW(wstr wstr ptr)PathCommonPrefixW
@ stdcall PathCompactPathA(long str long)PathCompactPathA
@ stdcall PathCompactPathExA(ptr str long long)PathCompactPathExA
@ stdcall PathCompactPathExW(ptr wstr long long)PathCompactPathExW
@ stdcall PathCompactPathW(long wstr long)PathCompactPathW
@ stdcall PathCreateFromUrlA(str ptr ptr long)PathCreateFromUrlA
@ stdcall PathCreateFromUrlW(wstr ptr ptr long)PathCreateFromUrlW
@ stdcall PathFileExistsA (str) PathFileExistsA
@ stdcall PathFileExistsW (wstr) PathFileExistsW
@ stdcall PathFindExtensionA (str) PathFindExtensionA
@ stdcall PathFindExtensionW (wstr) PathFindExtensionW
@ stdcall PathFindFileNameA (str) PathFindFileNameA
@ stdcall PathFindFileNameW (wstr) PathFindFileNameW
@ stdcall PathFindNextComponentA (str) PathFindNextComponentA
@ stdcall PathFindNextComponentW (wstr) PathFindNextComponentW
@ stdcall PathFindOnPathA (str ptr) PathFindOnPathA
@ stdcall PathFindOnPathW (wstr ptr) PathFindOnPathW
@ stdcall PathGetArgsA (str) PathGetArgsA
@ stdcall PathGetArgsW (wstr) PathGetArgsW
@ stdcall PathGetCharTypeA(long)PathGetCharTypeA
@ stdcall PathGetCharTypeW(long)PathGetCharTypeW
@ stdcall PathGetDriveNumberA (str) PathGetDriveNumberA
@ stdcall PathGetDriveNumberW (wstr) PathGetDriveNumberW
@ stdcall PathIsContentTypeA(str str)PathIsContentTypeA
@ stdcall PathIsContentTypeW(wstr wstr)PathIsContentTypeW
@ stdcall PathIsDirectoryA(str) PathIsDirectoryA
@ stdcall PathIsDirectoryW(wstr) PathIsDirectoryW
@ stdcall PathIsFileSpecA(str)PathIsFileSpecA
@ stdcall PathIsFileSpecW(wstr)PathIsFileSpecW
@ stdcall PathIsPrefixA(str str)PathIsPrefixA
@ stdcall PathIsPrefixW(wstr wstr)PathIsPrefixW
@ stdcall PathIsRelativeA (str) PathIsRelativeA
@ stdcall PathIsRelativeW (wstr) PathIsRelativeW
@ stdcall PathIsRootA(str) PathIsRootA
@ stdcall PathIsRootW(wstr) PathIsRootW
@ stdcall PathIsSameRootA(str str) PathIsSameRootA
@ stdcall PathIsSameRootW(wstr wstr) PathIsSameRootW
@ stdcall PathIsSystemFolderA(str long)PathIsSystemFolderA
@ stdcall PathIsSystemFolderW(wstr long)PathIsSystemFolderW
@ stdcall PathIsUNCA (str) PathIsUNCA
@ stdcall PathIsUNCServerA(str)PathIsUNCServerA
@ stdcall PathIsUNCServerShareA(str)PathIsUNCServerShareA
@ stdcall PathIsUNCServerShareW(wstr)PathIsUNCServerShareW
@ stdcall PathIsUNCServerW(wstr)PathIsUNCServerW
@ stdcall PathIsUNCW(wstr) PathIsUNCW
@ stdcall PathIsURLA(str) PathIsURLA
@ stdcall PathIsURLW(wstr) PathIsURLW
@ stdcall PathMakePrettyA(str) PathMakePrettyA
@ stdcall PathMakePrettyW(wstr) PathMakePrettyW
@ stdcall PathMakeSystemFolderA(str) PathMakeSystemFolderA
@ stdcall PathMakeSystemFolderW(wstr) PathMakeSystemFolderW
@ stdcall PathMatchSpecA  (str str) PathMatchSpecA
@ stdcall PathMatchSpecW  (wstr wstr) PathMatchSpecW
@ stdcall PathParseIconLocationA (str) PathParseIconLocationA
@ stdcall PathParseIconLocationW (wstr) PathParseIconLocationW
@ stdcall PathQuoteSpacesA (str) PathQuoteSpacesA
@ stdcall PathQuoteSpacesW (wstr) PathQuoteSpacesW
@ stdcall PathRelativePathToA(ptr str long str long)PathRelativePathToA
@ stdcall PathRelativePathToW(ptr str long str long)PathRelativePathToW
@ stdcall PathRemoveArgsA(str)PathRemoveArgsA
@ stdcall PathRemoveArgsW(wstr)PathRemoveArgsW
@ stdcall PathRemoveBackslashA (str) PathRemoveBackslashA
@ stdcall PathRemoveBackslashW (wstr) PathRemoveBackslashW
@ stdcall PathRemoveBlanksA(str) PathRemoveBlanksA
@ stdcall PathRemoveBlanksW(wstr) PathRemoveBlanksW
@ stdcall PathRemoveExtensionA(str)PathRemoveExtensionA
@ stdcall PathRemoveExtensionW(wstr)PathRemoveExtensionW
@ stdcall PathRemoveFileSpecA (str) PathRemoveFileSpecA
@ stdcall PathRemoveFileSpecW (wstr) PathRemoveFileSpecW
@ stdcall PathRenameExtensionA(str str)PathRenameExtensionA
@ stdcall PathRenameExtensionW(wstr wstr)PathRenameExtensionW
@ stdcall PathSearchAndQualifyA(str ptr long)PathSearchAndQualifyA
@ stdcall PathSearchAndQualifyW(wstr ptr long)PathSearchAndQualifyW
@ stdcall PathSetDlgItemPathA (long long ptr) PathSetDlgItemPathA
@ stdcall PathSetDlgItemPathW (long long ptr) PathSetDlgItemPathW
@ stdcall PathSkipRootA(str) PathSkipRootA
@ stdcall PathSkipRootW(wstr) PathSkipRootW
@ stdcall PathStripPathA(str) PathStripPathA
@ stdcall PathStripPathW(wstr) PathStripPathW
@ stdcall PathStripToRootA(str) PathStripToRootA
@ stdcall PathStripToRootW(wstr) PathStripToRootW
@ stdcall PathUnmakeSystemFolderA(str)PathUnmakeSystemFolderA
@ stdcall PathUnmakeSystemFolderW(wstr)PathUnmakeSystemFolderW
@ stdcall PathUnquoteSpacesA (str) PathUnquoteSpacesA
@ stdcall PathUnquoteSpacesW (wstr) PathUnquoteSpacesW
@ stdcall SHCreateShellPalette(long)SHCreateShellPalette
@ stdcall SHDeleteEmptyKeyA(long ptr) SHDeleteEmptyKeyA
@ stdcall SHDeleteEmptyKeyW(long ptr) SHDeleteEmptyKeyW
@ stdcall SHDeleteKeyA(long str) SHDeleteKeyA
@ stdcall SHDeleteKeyW(long wstr) SHDeleteKeyW
@ stub    SHDeleteOrphanKeyA
@ stub    SHDeleteOrphanKeyW
@ stdcall SHDeleteValueA(long  str  str) SHDeleteValueA
@ stdcall SHDeleteValueW(long wstr wstr) SHDeleteValueW
@ stub    SHEnumKeyExA
@ stub    SHEnumKeyExW
@ stub    SHEnumValueA
@ stub    SHEnumValueW
@ stdcall SHGetInverseCMAP ( ptr long ) SHGetInverseCMAP
@ stdcall SHGetValueA ( long str str ptr ptr ptr ) SHGetValueA
@ stdcall SHGetValueW ( long wstr wstr ptr ptr ptr ) SHGetValueW
@ stdcall SHIsLowMemoryMachine(long)SHIsLowMemoryMachine
@ stdcall SHOpenRegStreamA(long str str long)SHOpenRegStreamA
@ stdcall SHOpenRegStreamW(long wstr str long)SHOpenRegStreamW
@ stdcall SHOpenRegStream2A(long str str long)SHOpenRegStreamA
@ stdcall SHOpenRegStream2W(long wstr str long)SHOpenRegStreamW
@ stub    SHQueryInfoKeyA
@ stub    SHQueryInfoKeyW
@ stdcall SHQueryValueExA(long str ptr ptr ptr ptr) SHQueryValueExA
@ stdcall SHQueryValueExW(long wstr ptr ptr ptr ptr) SHQueryValueExW
@ stub    SHRegCloseUSKey
@ stub    SHRegCreateUSKeyA
@ stub    SHRegCreateUSKeyW
@ stub    SHRegDeleteEmptyUSKeyA
@ stub    SHRegDeleteEmptyUSKeyW
@ stub    SHRegDeleteUSValueA
@ stub    SHRegDeleteUSValueW
@ stdcall SHRegEnumUSKeyA(long long str ptr long) SHRegEnumUSKeyA
@ stdcall SHRegEnumUSKeyW(long long wstr ptr long) SHRegEnumUSKeyW
@ stub    SHRegEnumUSValueA
@ stub    SHRegEnumUSValueW
@ stdcall SHRegGetBoolUSValueA(str str long long)SHRegGetBoolUSValueA
@ stdcall SHRegGetBoolUSValueW(wstr wstr long long)SHRegGetBoolUSValueW
@ stdcall SHRegGetUSValueA ( ptr str ptr ptr ptr long ptr long ) SHRegGetUSValueA
@ stdcall SHRegGetUSValueW ( ptr wstr ptr ptr ptr long ptr long ) SHRegGetUSValueW
@ stdcall SHRegOpenUSKeyA ( str long long long long ) SHRegOpenUSKeyA
@ stdcall SHRegOpenUSKeyW ( wstr long long long long ) SHRegOpenUSKeyW
@ stub    SHRegQueryInfoUSKeyA
@ stub    SHRegQueryInfoUSKeyW
@ stdcall SHRegQueryUSValueA ( long str ptr ptr ptr long ptr long ) SHRegQueryUSValueA
@ stdcall SHRegQueryUSValueW ( long wstr ptr ptr ptr long ptr long ) SHRegQueryUSValueW
@ stub    SHRegSetUSValueA
@ stub    SHRegSetUSValueW
@ stub    SHRegWriteUSValueA
@ stub    SHRegWriteUSValueW
@ stdcall SHSetValueA (long  str  str long ptr long) SHSetValueA
@ stdcall SHSetValueW (long wstr wstr long ptr long) SHSetValueW
@ stdcall StrCSpnA (str str) StrCSpnA
@ stub    StrCSpnIA
@ stub    StrCSpnIW
@ stdcall StrCSpnW (wstr wstr) StrCSpnW
@ stdcall StrCatBuffA (str str long) StrCatBuffA
@ stdcall StrCatBuffW (wstr wstr long) StrCatBuffW
@ stdcall StrCatW (ptr wstr) StrCatW
@ stdcall StrChrA (str long) StrChrA
@ stub    StrChrIA
@ stub    StrChrIW
@ stdcall StrChrW (wstr long) StrChrW
@ stdcall StrCmpIW (wstr wstr) StrCmpIW
@ stdcall StrCmpNA (str str long) StrCmpNA
@ stdcall StrCmpNIA (str str long) StrCmpNIA
@ stdcall StrCmpNIW (wstr wstr long) StrCmpNIW
@ stdcall StrCmpNW (wstr wstr long) StrCmpNW
@ stdcall StrCmpW (wstr wstr) StrCmpW
@ stdcall StrCpyNW (wstr wstr long) StrCpyNW
@ stdcall StrCpyW (ptr wstr) StrCpyW
@ stdcall StrDupA (str) StrDupA
@ stdcall StrDupW (wstr) StrDupW
@ stdcall StrFormatByteSizeA(long str long) StrFormatByteSizeA
@ stdcall StrFormatByteSizeW(long wstr long) StrFormatByteSizeW
@ stub    StrFromTimeIntervalA
@ stub    StrFromTimeIntervalW
@ stub    StrIsIntlEqualA
@ stub    StrIsIntlEqualW
@ stdcall StrNCatA(str str long) StrNCatA
@ stdcall StrNCatW(wstr wstr long) StrNCatW
@ stub    StrPBrkA
@ stub    StrPBrkW
@ stdcall StrRChrA (str str long) StrRChrA
@ stub    StrRChrIA
@ stub    StrRChrIW
@ stdcall StrRChrW (wstr wstr long) StrRChrW
@ stub    StrRStrIA
@ stub    StrRStrIW
@ stub    StrSpnA
@ stub    StrSpnW
@ stdcall StrStrA(str str)StrStrA
@ stdcall StrStrIA(str str)StrStrIA
@ stdcall StrStrIW(wstr wstr)StrStrIW
@ stdcall StrStrW(wstr wstr)StrStrW
@ stdcall StrToIntA(str)StrToIntA
@ stdcall StrToIntExA(str long ptr) StrToIntExA
@ stdcall StrToIntExW(wstr long ptr) StrToIntExW
@ stdcall StrToIntW(wstr)StrToIntW
@ stdcall StrTrimA(str str) StrTrimA
@ stub    StrTrimW
@ stub    UrlApplySchemeA
@ stdcall UrlApplySchemeW(str ptr ptr long) UrlApplySchemeW
@ stdcall UrlCanonicalizeA(str ptr ptr long) UrlCanonicalizeA
@ stdcall UrlCanonicalizeW(wstr ptr ptr long) UrlCanonicalizeW
@ stub    UrlCombineA
@ stub    UrlCombineW
@ stub    UrlCompareA
@ stub    UrlCompareW
@ stub    UrlCreateFromPathA
@ stub    UrlCreateFromPathW
@ stdcall UrlEscapeA(str ptr ptr long)UrlEscapeA
@ stdcall UrlEscapeW(wstr ptr ptr long)UrlEscapeW
@ stub    UrlGetLocationA
@ stub    UrlGetLocationW
@ stub    UrlGetPartA
@ stub    UrlGetPartW
@ stdcall UrlHashA(str ptr long) UrlHashA
@ stub    UrlHashW
@ stub    UrlIsA
@ stub    UrlIsNoHistoryA
@ stub    UrlIsNoHistoryW
@ stub    UrlIsOpaqueA
@ stub    UrlIsOpaqueW
@ stub    UrlIsW
@ stdcall UrlUnescapeA(str ptr ptr long) UrlUnescapeA
@ stdcall UrlUnescapeW(wstr ptr ptr long) UrlUnescapeW
@ varargs wnsprintfA(ptr long str) wnsprintfA
@ varargs wnsprintfW(ptr long wstr) wnsprintfW
@ forward wvnsprintfA user32.wvsnprintfA
@ forward wvnsprintfW user32.wvsnprintfW


# exported in later versions
@ stdcall StrRetToBufA (ptr ptr ptr long) StrRetToBufA
@ stdcall StrRetToBufW (ptr ptr ptr long) StrRetToBufW
#@ stdcall StrRetToStrA (ptr ptr ptr) StrRetToStrA
#@ stdcall StrRetToStrW (ptr ptr ptr) StrRetToStrW
@ stdcall SHRegGetPathA(long str str ptr long)SHRegGetPathA
@ stdcall SHRegGetPathW(long wstr wstr ptr long)SHRegGetPathW
@ stub    PathIsDirectoryEmptyA
@ stub    PathIsDirectoryEmptyW
@ stub    PathIsNetworkPathA
@ stub    PathIsNetworkPathW
@ stub    PathIsLFNFileSpecA
@ stub    PathIsLFNFileSpecW
@ stub    PathFindSuffixArrayA
@ stub    PathFindSuffixArrayW
@ stdcall _SHGetInstanceExplorer@4(ptr) _SHGetInstanceExplorer
@ stub    PathUndecorateA
@ stub    PathUndecorateW
@ stub    SHCopyKeyA
@ stub    SHCopyKeyW
@ stub    SHAutoComplete
@ stub    SHCreateStreamOnFileA
@ stub    SHCreateStreamOnFileW
@ stub    SHCreateStreamWrapper
@ stub    SHCreateThread
@ stub    SHGetThreadRef
@ stub    SHRegDuplicateHKey
@ stub    SHSetThreadRef
@ stub    SHSkipJunction
@ stub    SHStrDupA
@ stub    SHStrDupW
@ stub    StrFormatByteSize64A
@ stub    StrFormatKBSizeA
@ stub    StrFormatKBSizeW
