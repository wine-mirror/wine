1   stdcall @(str ptr) SHLWAPI_1
2   stdcall @(wstr ptr) SHLWAPI_2
3   stdcall @(str long) SHLWAPI_3
4   stdcall @(wstr long) SHLWAPI_4
5   stdcall @(str ptr long) SHLWAPI_5
6   stdcall @(wstr ptr long) SHLWAPI_6
7   stdcall @(long long ptr) SHLWAPI_7
8   stdcall @(long long) SHLWAPI_8
9   stdcall @(ptr) SHLWAPI_9
10  stdcall @(long long) SHLWAPI_10
11  stdcall @(long long long long long) SHLWAPI_11
12  stdcall @(ptr long) SHLWAPI_12
13  stdcall @(ptr ptr) SHLWAPI_13
14  stdcall @(ptr ptr) SHLWAPI_14
15  stdcall @(ptr ptr) SHLWAPI_15
16  stdcall SHCreateThread(ptr ptr long ptr) SHCreateThread
17  stdcall @ (ptr ptr) SHLWAPI_17
18  stdcall @ (ptr ptr) SHLWAPI_18
19  stdcall @ (ptr) SHLWAPI_19
20  stdcall @ (ptr ptr) SHLWAPI_20
21  stdcall @ (ptr long) SHLWAPI_21
22  stdcall @ (ptr long) SHLWAPI_22
23  stdcall @(ptr ptr long) SHLWAPI_23
24  stdcall @(ptr ptr long) SHLWAPI_24
25  stdcall @(long) SHLWAPI_25
26  stdcall @(long) SHLWAPI_26
27  stdcall @(long) SHLWAPI_27
28  stdcall @(long) SHLWAPI_28
29  stdcall @(long) SHLWAPI_29
30  stdcall @(long) SHLWAPI_30
31  stdcall @(long) SHLWAPI_31
32  stdcall @(ptr)  SHLWAPI_32
33  stdcall @(long) SHLWAPI_33
34  stdcall @(long) SHLWAPI_34
35  stdcall @(ptr long ptr) SHLWAPI_35
36  stdcall @(long long long wstr) SHLWAPI_36
37  stdcall @(ptr long long long long) user32.CallWindowProcW
38  stdcall @(wstr) user32.CharLowerW
39  stdcall @(wstr long) user32.CharLowerBuffW
40  stdcall @(wstr) user32.CharNextW
41  stdcall @(wstr wstr) user32.CharPrevW
42  stub @
43  stdcall @(wstr) user32.CharUpperW
44  stdcall @(wstr long) user32.CharUpperBuffW
45  stdcall @(long long wstr long wstr long) kernel32.CompareStringW
46  stdcall @(long ptr long) user32.CopyAcceleratorTableW
47  stdcall @(ptr long) user32.CreateAcceleratorTableW
48  stdcall @(wstr wstr wstr ptr) gdi32.CreateDCW
49  stdcall @(long ptr long ptr long) user32.CreateDialogParamA
50  stdcall @(wstr ptr) kernel32.CreateDirectoryW
51  stdcall @(ptr long long wstr) kernel32.CreateEventW
52  stdcall @(wstr long long ptr long long long) kernel32.CreateFileW
53  stdcall @(ptr) gdi32.CreateFontIndirectW
54  stdcall @(wstr wstr wstr ptr) gdi32.CreateICW
55  stdcall @(long wstr wstr long long long long long long long long ptr) user32.CreateWindowExW
56  stdcall @(long long long long) user32.DefWindowProcW
57  stdcall @(wstr) kernel32.DeleteFileW
58  stub @
59  stub @
60  stdcall @(ptr) user32.DispatchMessageW
61  stdcall @(long wstr long ptr long) user32.DrawTextW
62  stdcall @(long wstr ptr long) gdi32.EnumFontFamiliesW
63  stdcall @(long ptr ptr long long) gdi32.EnumFontFamiliesExW
64  stdcall @(long wstr ptr long) kernel32.EnumResourceNamesW
65  stdcall @(wstr ptr) kernel32.FindFirstFileW
66  stdcall @(long wstr wstr) kernel32.FindResourceW
67  stdcall @(wstr wstr) user32.FindWindowW
68  stdcall @(long ptr long long ptr long ptr) kernel32.FormatMessageW
69  stdcall @(long wstr ptr) user32.GetClassInfoW
70  stdcall @(long long) user32.GetClassLongW
71  stdcall @(long ptr long) user32.GetClassNameW
72  stdcall @(long ptr long) user32.GetClipboardFormatNameW
73  stdcall @(long ptr) kernel32.GetCurrentDirectoryW
74  stdcall @(long long wstr long) SHLWAPI_74
75  stdcall @(wstr) kernel32.GetFileAttributesW
76  stdcall @(wstr long ptr ptr) kernel32.GetFullPathNameW
77  stdcall @(long long ptr long) kernel32.GetLocaleInfoW
78  stdcall @(long long ptr long long) user32.GetMenuStringW
79  stdcall @(ptr long long long) user32.GetMessageW
80  stdcall @(long ptr long) kernel32.GetModuleFileNameW
81  stdcall @(ptr long) kernel32.GetSystemDirectoryW
82  stdcall @(wstr wstr wstr long ptr ptr) kernel32.SearchPathW
83  stdcall @(wstr) kernel32.GetModuleHandleW
84  stdcall @(long long ptr) gdi32.GetObjectW
85  stdcall @(wstr wstr long wstr) kernel32.GetPrivateProfileIntW
86  stdcall @(wstr wstr wstr ptr long) kernel32.GetProfileStringW
87  stdcall @(long wstr) user32.GetPropW
88  stdcall @(long long wstr long ptr) kernel32.GetStringTypeExW
89  stdcall @(wstr wstr long ptr) kernel32.GetTempFileNameW
90  stdcall @(long ptr) kernel32.GetTempPathW
91  stdcall @(long wstr long ptr) gdi32.GetTextExtentPoint32W
92  stdcall @(long long ptr) gdi32.GetTextFaceW
93  stdcall @(long ptr) gdi32.GetTextMetricsW
94  stdcall @(long long) user32.GetWindowLongW
95  stdcall @(long ptr long) user32.GetWindowTextW
96  stdcall @(long) user32.GetWindowTextLengthW
97  stdcall @(ptr long) kernel32.GetWindowsDirectoryW
98  stdcall @(long long long long ptr) user32.InsertMenuW
99  stdcall @(long ptr) user32.IsDialogMessageW
100 stdcall @(long wstr) user32.LoadAcceleratorsW
101 stdcall @(long wstr) user32.LoadBitmapW
102 stdcall @(long wstr) user32.LoadCursorW
103 stdcall @(long wstr) user32.LoadIconW
104 stdcall @(long wstr long long long long) user32.LoadImageW
105 stdcall @(wstr long long) kernel32.LoadLibraryExW
106 stdcall @(long wstr) user32.LoadMenuW
107 stdcall @(long long ptr long) user32.LoadStringW
108 stdcall @(ptr) user32.MessageBoxIndirectW
109 stdcall @(long long long long ptr) user32.ModifyMenuW
110 stdcall @(long long long long) gdi32.GetCharWidth32W
111 stdcall @(long wstr long long ptr long) gdi32.GetCharacterPlacementW
112 stdcall @(wstr wstr long) kernel32.CopyFileW
113 stdcall @(wstr wstr) kernel32.MoveFileW
114 stdcall @(ptr ptr) user32.OemToCharW
115 stdcall @(wstr) kernel32.OutputDebugStringW
116 stdcall @(ptr long long long long) user32.PeekMessageW
117 stdcall @(long long long long) user32.PostMessageW
118 stdcall @(long long long long) user32.PostThreadMessageW
119 stdcall @(long wstr ptr) advapi32.RegCreateKeyW
120 stdcall @(long wstr long ptr long long ptr ptr ptr) advapi32.RegCreateKeyExW
121 stdcall @(long wstr) advapi32.RegDeleteKeyW
122 stdcall @(long long ptr long) advapi32.RegEnumKeyW
123 stdcall @(long long ptr ptr ptr ptr ptr ptr) advapi32.RegEnumKeyExW
124 stdcall @(long wstr ptr) advapi32.RegOpenKeyW
125 stdcall @(long wstr long long ptr) advapi32.RegOpenKeyExW
126 stdcall @(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) advapi32.RegQueryInfoKeyW
127 stdcall @(long wstr ptr ptr) advapi32.RegQueryValueW
128 stdcall @(long wstr ptr ptr ptr ptr) advapi32.RegQueryValueExW
129 stdcall @(long wstr long ptr long) advapi32.RegSetValueW
130 stdcall @(long wstr long long ptr long) advapi32.RegSetValueExW
131 stdcall @(ptr) user32.RegisterClassW
132 stdcall @(wstr) user32.RegisterClipboardFormatW
133 stdcall @(wstr) user32.RegisterWindowMessageW
134 stdcall @(long wstr) user32.RemovePropW
135 stdcall @(long long long long long) user32.SendDlgItemMessageW
136 stdcall @(long long long long) user32.SendMessageW
137 stdcall @(wstr) kernel32.SetCurrentDirectoryW
138 stdcall @(long long wstr) SHLWAPI_138
139 stub @
140 stdcall @(long wstr long) user32.SetPropW
141 stdcall @(long long long) user32.SetWindowLongW
142 stdcall @(long long long long) user32.SetWindowsHookExW
143 stdcall @(long wstr) user32.SetWindowTextW
144 stdcall @(long ptr) gdi32.StartDocW
145 stdcall @(long long ptr long) user32.SystemParametersInfoW
146 stdcall @(long long ptr) user32.TranslateAcceleratorW
147 stdcall @(wstr long) user32.UnregisterClassW
148 stdcall @(long) user32.VkKeyScanW
149 stdcall @(long wstr long long) user32.WinHelpW
150 stdcall @(ptr wstr ptr) user32.wvsprintfW
151 stdcall @(str ptr long) SHLWAPI_151
152 stdcall @(wstr wstr long) SHLWAPI_152
153 stdcall @(long long long) SHLWAPI_153
154 stdcall @(wstr wstr long) SHLWAPI_154
155 stdcall @(str str) SHLWAPI_155
156 stdcall @(wstr wstr) SHLWAPI_156
157 stdcall @(str str) SHLWAPI_157
158 stdcall @(wstr wstr) SHLWAPI_158
159 stdcall @(long long wstr long wstr long) kernel32.CompareStringW
160 stub @
161 stub @
162 stdcall @(str long) SHLWAPI_162
163 stdcall @(ptr ptr long ptr ptr) SHLWAPI_163
164 stdcall @(ptr ptr long long ptr ptr) SHLWAPI_164
165 stdcall @(long long long long) SHLWAPI_165
166 stdcall @(ptr) SHLWAPI_166
167 stdcall @(long ptr) SHLWAPI_167
168 stdcall @(ptr ptr long ptr ptr ptr) SHLWAPI_168
169 stdcall @(long) SHLWAPI_169
170 stdcall @(str) SHLWAPI_170
171 stdcall @(ptr ptr) SHLWAPI_171
172 stdcall @(ptr ptr) SHLWAPI_172
173 stub @
174 stdcall @(ptr ptr) SHLWAPI_174
175 stdcall @(ptr ptr) SHLWAPI_175
176 stdcall @(ptr ptr ptr ptr) SHLWAPI_176
177 stub @
178 stub @
179 stub @
180 stdcall @(long) SHLWAPI_180
181 stdcall @(long long long) SHLWAPI_181
182 stdcall @(long long long) SHLWAPI_182
183 stdcall @(ptr) SHLWAPI_183
184 stdcall @(ptr ptr long) SHLWAPI_184
185 stub @
186 stub @
187 stdcall @(ptr ptr) SHLWAPI_187
188 stub @
189 stdcall @(ptr ptr) SHLWAPI_189
190 stub @
191 stub @
192 stub @
193 stdcall @() SHLWAPI_193
194 stub @
195 stub @
196 stub @
197 stdcall @(long ptr long) SHLWAPI_197
198 stub @
199 stdcall @(ptr ptr) SHLWAPI_199
200 stub @
201 stdcall @(ptr long ptr long long ptr ptr) SHLWAPI_201
202 stdcall @(ptr long ptr) SHLWAPI_202
203 stdcall @(str) SHLWAPI_203
204 stdcall @(long long) SHLWAPI_204
205 stdcall @(long str str ptr ptr ptr) SHLWAPI_205
206 stdcall @(long wstr wstr ptr ptr ptr) SHLWAPI_206
207 stub @
208 stdcall @(long long ptr ptr long) SHLWAPI_208
209 stdcall @(ptr) SHLWAPI_209
210 stdcall @(ptr long ptr) SHLWAPI_210
211 stdcall @(ptr long) SHLWAPI_211
212 stdcall @(ptr ptr long) SHLWAPI_212
213 stdcall @(ptr) SHLWAPI_213
214 stdcall @(ptr ptr) SHLWAPI_214
215 stdcall @(long long long) SHLWAPI_215
216 stub @
217 stdcall @(wstr ptr ptr) SHLWAPI_217
218 stdcall @(long wstr ptr ptr) SHLWAPI_218
219 stdcall @(long long long long) SHLWAPI_219
220 stub @
221 stub @
222 stdcall -noname _SHGlobalCounterCreate(long) _SHGlobalCounterCreate
223 stdcall -noname _SHGlobalCounterGetValue(long) _SHGlobalCounterGetValue
224 stdcall -noname _SHGlobalCounterIncrement(long) _SHGlobalCounterIncrement
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
236 stdcall @(ptr) SHLWAPI_236
237 stdcall @(ptr) SHLWAPI_237
238 stub @
239 stdcall @(long str long) SHLWAPI_239
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
253 stdcall AssocCreate(long long long long ptr ptr) AssocCreate
254 stdcall AssocQueryKeyA(long long str ptr ptr) AssocQueryKeyA
255 stdcall AssocQueryKeyW(long long wstr ptr ptr) AssocQueryKeyW
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
281 stdcall @(ptr ptr ptr ptr) SHLWAPI_281
282 stdcall @(ptr ptr ptr ptr) SHLWAPI_282
283 stub @
284 stdcall @(ptr ptr ptr) SHLWAPI_284
285 stub @
286 stub @
287 stdcall @(ptr ptr) SHLWAPI_287
288 stub @
289 stdcall @(wstr long long) SHLWAPI_289
290 stub @
291 stub @
292 stub @
293 stub @
294 stdcall @(long long long long long) SHLWAPI_294
295 stdcall @(wstr ptr wstr wstr) SHLWAPI_295
296 stub @
297 stub @
298 stdcall @(wstr wstr wstr wstr) kernel32.WritePrivateProfileStringW
299 stdcall @(long long long long ptr wstr long ptr) SHLWAPI_299
300 stdcall @(long long long long long long long long long long long long long wstr) gdi32.CreateFontW
301 stdcall @(long wstr long ptr long ptr) user32.DrawTextExW
302 stdcall @(long long long ptr) user32.GetMenuItemInfoW
303 stdcall @(long long long ptr) user32.InsertMenuItemW
304 stdcall @(wstr) gdi32.CreateMetaFileW
305 stdcall @(ptr long wstr) kernel32.CreateMutexW
306 stdcall @(wstr ptr long) kernel32.ExpandEnvironmentStringsW
307 stdcall @(ptr long long wstr) kernel32.CreateSemaphoreW
308 stdcall @(ptr long) kernel32.IsBadStringPtrW
309 stdcall @(wstr) kernel32.LoadLibraryW
310 stdcall @(long long ptr wstr ptr long) kernel32.GetTimeFormatW
311 stdcall @(long long ptr wstr ptr long) kernel32.GetDateFormatW
312 stdcall @(wstr wstr wstr ptr long wstr) kernel32.GetPrivateProfileStringW
313 stdcall @(ptr long ptr long long) SHLWAPI_313
314 stdcall @(ptr) user32.RegisterClassExW
315 stdcall @(long wstr ptr) user32.GetClassInfoExW
316 stub SHCreateStreamOnFileAOld
317 stub SHCreateStreamOnFileWOld
318 stdcall @(long long wstr long) SHLWAPI_318
319 stdcall @(long long wstr wstr) user32.FindWindowExW
320 stdcall @(str str) SHLWAPI_320
321 stdcall @(wstr wstr) SHLWAPI_321
322 stdcall @(str) SHLWAPI_322
323 stdcall @(wstr) SHLWAPI_323
324 stdcall @(str str) SHLWAPI_324
325 stdcall @(wstr wstr) SHLWAPI_325
326 stdcall @(str) SHLWAPI_326
327 stdcall @(wstr) SHLWAPI_327
328 stdcall @(str ptr long) SHLWAPI_328
329 stdcall @(wstr ptr long) SHLWAPI_329
330 stub @
331 stub @
332 stdcall @(ptr long) user32.CallMsgFilterW
333 stdcall @(ptr) SHLWAPI_333
334 stdcall @(ptr ptr) SHLWAPI_334
335 stdcall @(ptr) SHLWAPI_335
336 stdcall @(ptr) SHLWAPI_336
337 stdcall @(wstr long ptr ptr long) SHLWAPI_337
338 stdcall @(wstr long) kernel32.SetFileAttributesW
339 stdcall @(long long wstr ptr ptr long) kernel32.GetNumberFormatW
340 stdcall @(long wstr wstr long) user32.MessageBoxW
341 stdcall @(long ptr) kernel32.FindNextFileW
342 stdcall @(ptr long long) SHInterlockedCompareExchange
343 stub @
344 stub @
345 stub @
346 stdcall @(wstr ptr long) SHLWAPI_346
347 stdcall @(long wstr) advapi32.RegDeleteValueW
348 stub @
349 stub @
350 stdcall @(wstr ptr) SHLWAPI_350
351 stdcall @(wstr ptr long ptr) SHLWAPI_351
352 stdcall @(ptr wstr ptr ptr) SHLWAPI_352
353 stub @
354 stub @
355 stub @
356 stdcall -noname _CreateAllAccessSecurityAttributes(ptr ptr) _CreateAllAccessSecurityAttributes
357 stdcall @(wstr wstr wstr long long) SHLWAPI_357
358 stdcall @(wstr long long ptr ptr long) SHLWAPI_358
359 stdcall @(long long wstr) kernel32.OpenEventW
360 stdcall @(wstr) kernel32.RemoveDirectoryW
361 stdcall @(wstr ptr long) kernel32.GetShortPathNameW
362 stdcall @(ptr ptr) advapi32.GetUserNameW
363 stub @
364 stdcall @(str str long) SHLWAPI_364
365 stub @
366 stdcall @(long long ptr ptr ptr ptr ptr ptr) advapi32.RegEnumValueW
367 stdcall @(wstr wstr ptr long wstr) kernel32.WritePrivateProfileStructW
368 stdcall @(wstr wstr ptr long wstr) kernel32.GetPrivateProfileStructW
369 stdcall @(wstr wstr ptr ptr long long ptr wstr ptr ptr) kernel32.CreateProcessW
370 stdcall @(long wstr long) SHLWAPI_370
371 stub @
372 stub @
373 stub @
374 stub @
375 stub @
376 stdcall @() SHLWAPI_376  # kernel32.GetUserDefaultUILanguage
377 stdcall @(str long long) SHLWAPI_377
378 stdcall @(wstr long long) SHLWAPI_378
379 stub @
380 stub @
381 stdcall AssocQueryStringA(long long ptr ptr str ptr) AssocQueryStringA
382 stdcall AssocQueryStringByKeyA(long long ptr ptr str ptr) AssocQueryStringByKeyA
383 stdcall AssocQueryStringByKeyW(long long ptr ptr wstr ptr) AssocQueryStringByKeyW
384 stdcall AssocQueryStringW(long long ptr ptr wstr ptr) AssocQueryStringW
385 stdcall ChrCmpIA(long long) ChrCmpIA
386 stdcall ChrCmpIW(long long) ChrCmpIW
387 stub ColorAdjustLuma
388 stub @
389 stdcall @(ptr) SHLWAPI_389
390 stdcall @(long wstr) SHLWAPI_390
391 stdcall @(ptr ptr long ptr long) SHLWAPI_391
392 stub @
393 stdcall @(long ptr long ptr long) user32.CreateDialogIndirectParamW
394 stdcall @(long ptr long ptr long) user32.CreateDialogIndirectParamA
395 stub @
396 stub @
397 stub @
398 stub @
399 stdcall @(str str long) SHLWAPI_399
400 stdcall @(wstr wstr long) SHLWAPI_400
401 stdcall @(ptr) SHLWAPI_401
402 stdcall @(ptr) SHLWAPI_402
403 stdcall @(ptr) SHLWAPI_403
404 stdcall ColorHLSToRGB(long long long) ColorHLSToRGB
405 stub @
406 stdcall @(ptr ptr ptr ptr ptr ptr) SHLWAPI_406
407 stub @
408 stub @
409 stub @
410 stub @
411 stub @
412 stub @
413 stdcall @(long) SHLWAPI_413
414 stub @
415 stub @
416 stub @
417 stub @
418 stdcall @(long) SHLWAPI_418
419 stub @
420 stub @
421 stub @
422 stdcall -noname _SHGlobalCounterCreateNamedA(str long) _SHGlobalCounterCreateNamedA
423 stdcall -noname _SHGlobalCounterCreateNamedW(wstr long) _SHGlobalCounterCreateNamedW
424 stdcall -noname _SHGlobalCounterDecrement(long) _SHGlobalCounterDecrement
425 stub @
426 stub @
427 stub @
428 stdcall @(long long long long long ptr) user32.TrackPopupMenuEx
429 stub @
430 stdcall @(long long) SHLWAPI_430
431 stdcall @(long) SHLWAPI_431
432 stub @
433 stub @
434 stdcall @(long long long long long long ptr) user32.SendMessageTimeoutW
435 stub @
436 stdcall @(wstr ptr) SHLWAPI_436
437 stdcall @(long) SHLWAPI_437
438 stub @
439 stub @
440 stub @
441 stub @
442 stdcall @(wstr ptr long) kernel32.GetEnvironmentVariableW
443 stdcall @(ptr long) kernel32.GetSystemWindowsDirectoryA
444 stdcall @(ptr long) kernel32.GetSystemWindowsDirectoryW
445 stdcall ColorRGBToHLS(long ptr ptr ptr) ColorRGBToHLS
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
@ stdcall SHDeleteOrphanKeyA(long str) SHDeleteOrphanKeyA
@ stdcall SHDeleteOrphanKeyW(long wstr) SHDeleteOrphanKeyW
@ stdcall SHDeleteValueA(long  str  str) SHDeleteValueA
@ stdcall SHDeleteValueW(long wstr wstr) SHDeleteValueW
@ stdcall SHEnumKeyExA(long long str ptr) SHEnumKeyExA
@ stdcall SHEnumKeyExW(long long wstr ptr) SHEnumKeyExW
@ stdcall SHEnumValueA(long long str ptr ptr ptr ptr) SHEnumValueA
@ stdcall SHEnumValueW(long long wstr ptr ptr ptr ptr) SHEnumValueW
@ stdcall SHGetInverseCMAP ( ptr long ) SHGetInverseCMAP
@ stdcall SHGetValueA ( long str str ptr ptr ptr ) SHGetValueA
@ stdcall SHGetValueW ( long wstr wstr ptr ptr ptr ) SHGetValueW
@ stdcall SHIsLowMemoryMachine(long)SHIsLowMemoryMachine
@ stdcall SHOpenRegStreamA(long str str long)SHOpenRegStreamA
@ stdcall SHOpenRegStreamW(long wstr str long)SHOpenRegStreamW
@ stdcall SHOpenRegStream2A(long str str long)SHOpenRegStream2A
@ stdcall SHOpenRegStream2W(long wstr str long)SHOpenRegStream2W
@ stdcall SHQueryInfoKeyA(long ptr ptr ptr ptr) SHQueryInfoKeyA
@ stdcall SHQueryInfoKeyW(long ptr ptr ptr ptr) SHQueryInfoKeyW
@ stdcall SHQueryValueExA(long str ptr ptr ptr ptr) SHQueryValueExA
@ stdcall SHQueryValueExW(long wstr ptr ptr ptr ptr) SHQueryValueExW
@ stdcall SHRegCloseUSKey(ptr) SHRegCloseUSKey
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
@ stdcall SHRegGetUSValueA ( str str ptr ptr ptr long ptr long ) SHRegGetUSValueA
@ stdcall SHRegGetUSValueW ( wstr wstr ptr ptr ptr long ptr long ) SHRegGetUSValueW
@ stdcall SHRegOpenUSKeyA ( str long long long long ) SHRegOpenUSKeyA
@ stdcall SHRegOpenUSKeyW ( wstr long long long long ) SHRegOpenUSKeyW
@ stdcall SHRegQueryInfoUSKeyA ( long ptr ptr ptr ptr long ) SHRegQueryInfoUSKeyA
@ stdcall SHRegQueryInfoUSKeyW ( long ptr ptr ptr ptr long ) SHRegQueryInfoUSKeyW
@ stdcall SHRegQueryUSValueA ( long str ptr ptr ptr long ptr long ) SHRegQueryUSValueA
@ stdcall SHRegQueryUSValueW ( long wstr ptr ptr ptr long ptr long ) SHRegQueryUSValueW
@ stdcall SHRegSetUSValueA ( str str long ptr long long) SHRegSetUSValueA
@ stdcall SHRegSetUSValueW ( wstr wstr long ptr long long) SHRegSetUSValueW
@ stdcall SHRegWriteUSValueA (long str long ptr long long) SHRegWriteUSValueA
@ stdcall SHRegWriteUSValueW (long str long ptr long long) SHRegWriteUSValueW
@ stdcall SHSetValueA (long  str  str long ptr long) SHSetValueA
@ stdcall SHSetValueW (long wstr wstr long ptr long) SHSetValueW
@ stdcall StrCSpnA (str str) StrCSpnA
@ stdcall StrCSpnIA (str str) StrCSpnIA
@ stdcall StrCSpnIW (wstr wstr) StrCSpnIW
@ stdcall StrCSpnW (wstr wstr) StrCSpnW
@ stdcall StrCatBuffA (str str long) StrCatBuffA
@ stdcall StrCatBuffW (wstr wstr long) StrCatBuffW
@ stdcall StrCatW (ptr wstr) StrCatW
@ stdcall StrChrA (str long) StrChrA
@ stdcall StrChrIA (str long) StrChrIA
@ stdcall StrChrIW (wstr long) StrChrIW
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
@ stdcall StrFormatByteSizeA(long ptr long) StrFormatByteSizeA
@ stdcall StrFormatByteSizeW(long long ptr long) StrFormatByteSizeW
@ stdcall StrFromTimeIntervalA(ptr long long long) StrFromTimeIntervalA
@ stdcall StrFromTimeIntervalW(ptr long long long) StrFromTimeIntervalW
@ stdcall StrIsIntlEqualA(long str str long) StrIsIntlEqualA
@ stdcall StrIsIntlEqualW(long wstr wstr long) StrIsIntlEqualW
@ stdcall StrNCatA(str str long) StrNCatA
@ stdcall StrNCatW(wstr wstr long) StrNCatW
@ stdcall StrPBrkA(str str) StrPBrkA
@ stdcall StrPBrkW(wstr wstr) StrPBrkW
@ stdcall StrRChrA (str str long) StrRChrA
@ stdcall StrRChrIA (str str long) StrRChrIA
@ stdcall StrRChrIW (str str long) StrRChrIW
@ stdcall StrRChrW (wstr wstr long) StrRChrW
@ stdcall StrRStrIA (str str str) StrRStrIA
@ stdcall StrRStrIW (wstr wstr wstr) StrRStrIW
@ stdcall StrSpnA (str str) StrSpnA
@ stdcall StrSpnW (wstr wstr) StrSpnW
@ stdcall StrStrA(str str)StrStrA
@ stdcall StrStrIA(str str)StrStrIA
@ stdcall StrStrIW(wstr wstr)StrStrIW
@ stdcall StrStrW(wstr wstr)StrStrW
@ stdcall StrToIntA(str)StrToIntA
@ stdcall StrToIntExA(str long ptr) StrToIntExA
@ stdcall StrToIntExW(wstr long ptr) StrToIntExW
@ stdcall StrToIntW(wstr)StrToIntW
@ stdcall StrTrimA(str str) StrTrimA
@ stdcall StrTrimW(wstr wstr) StrTrimW
@ stdcall UrlApplySchemeA(str ptr ptr long) UrlApplySchemeA
@ stdcall UrlApplySchemeW(wstr ptr ptr long) UrlApplySchemeW
@ stdcall UrlCanonicalizeA(str ptr ptr long) UrlCanonicalizeA
@ stdcall UrlCanonicalizeW(wstr ptr ptr long) UrlCanonicalizeW
@ stdcall UrlCombineA(str str str ptr long) UrlCombineA
@ stdcall UrlCombineW(wstr wstr wstr ptr long) UrlCombineW
@ stdcall UrlCompareA(str str long) UrlCompareA
@ stdcall UrlCompareW(wstr wstr long) UrlCompareW
@ stdcall UrlCreateFromPathA(str ptr ptr long) UrlCreateFromPathA
@ stdcall UrlCreateFromPathW(wstr ptr ptr long) UrlCreateFromPathW
@ stdcall UrlEscapeA(str ptr ptr long)UrlEscapeA
@ stdcall UrlEscapeW(wstr ptr ptr long)UrlEscapeW
@ stdcall UrlGetLocationA(str) UrlGetLocationA
@ stdcall UrlGetLocationW(wstr) UrlGetLocationW
@ stdcall UrlGetPartA(str ptr ptr long long) UrlGetPartA
@ stdcall UrlGetPartW(wstr ptr ptr long long) UrlGetPartW
@ stdcall UrlHashA(str ptr long) UrlHashA
@ stdcall UrlHashW(wstr ptr long) UrlHashW
@ stdcall UrlIsA(str long) UrlIsA
@ stdcall UrlIsNoHistoryA(str) UrlIsNoHistoryA
@ stdcall UrlIsNoHistoryW(wstr) UrlIsNoHistoryW
@ stdcall UrlIsOpaqueA(str) UrlIsOpaqueA
@ stdcall UrlIsOpaqueW(wstr) UrlIsOpaqueW
@ stdcall UrlIsW(wstr long) UrlIsW
@ stdcall UrlUnescapeA(str ptr ptr long) UrlUnescapeA
@ stdcall UrlUnescapeW(wstr ptr ptr long) UrlUnescapeW
@ varargs wnsprintfA(ptr long str) wnsprintfA
@ varargs wnsprintfW(ptr long wstr) wnsprintfW
@ stdcall wvnsprintfA(ptr long str ptr) wvnsprintfA
@ stdcall wvnsprintfW(ptr long wstr ptr) wvnsprintfW


# exported in later versions
@ stdcall StrRetToBufA (ptr ptr ptr long) StrRetToBufA
@ stdcall StrRetToBufW (ptr ptr ptr long) StrRetToBufW
@ stdcall StrRetToStrA (ptr ptr ptr) StrRetToStrA
@ stdcall StrRetToStrW (ptr ptr ptr) StrRetToStrW
@ stdcall SHRegGetPathA(long str str ptr long)SHRegGetPathA
@ stdcall SHRegGetPathW(long wstr wstr ptr long)SHRegGetPathW
@ stub    MLLoadLibraryA
@ stub    MLLoadLibraryW
@ stdcall PathIsDirectoryEmptyA(str) PathIsDirectoryEmptyA
@ stdcall PathIsDirectoryEmptyW(wstr) PathIsDirectoryEmptyW
@ stdcall PathIsNetworkPathA(str) PathIsNetworkPathA
@ stdcall PathIsNetworkPathW(wstr) PathIsNetworkPathW
@ stdcall PathIsLFNFileSpecA(str) PathIsLFNFileSpecA
@ stdcall PathIsLFNFileSpecW(wstr) PathIsLFNFileSpecW
@ stdcall PathFindSuffixArrayA(str ptr long) PathFindSuffixArrayA
@ stdcall PathFindSuffixArrayW(wstr ptr long) PathFindSuffixArrayW
@ stdcall _SHGetInstanceExplorer(ptr) _SHGetInstanceExplorer
@ stdcall PathUndecorateA(str) PathUndecorateA
@ stdcall PathUndecorateW(wstr) PathUndecorateW
@ stub    PathUnExpandEnvStringsA
@ stub    PathUnExpandEnvStringsW
@ stdcall SHCopyKeyA(long str long long) SHCopyKeyA
@ stdcall SHCopyKeyW(long wstr long long) SHCopyKeyW
@ stub    SHAutoComplete
@ stdcall SHCreateStreamOnFileA(str long ptr) SHCreateStreamOnFileA
@ stdcall SHCreateStreamOnFileW(wstr long ptr) SHCreateStreamOnFileW
@ stdcall SHCreateStreamOnFileEx(wstr long long long ptr ptr) SHCreateStreamOnFileEx
@ stdcall SHCreateStreamWrapper(ptr ptr long ptr) SHCreateStreamWrapper
@ stdcall SHGetThreadRef (ptr) SHGetThreadRef
@ stdcall SHRegDuplicateHKey (long) SHRegDuplicateHKey
@ stdcall SHRegSetPathA(long str str str long) SHRegSetPathA
@ stdcall SHRegSetPathW(long wstr wstr wstr long) SHRegSetPathW
@ stub    SHRegisterValidateTemplate
@ stdcall SHSetThreadRef (ptr) SHSetThreadRef
@ stdcall SHReleaseThreadRef() SHReleaseThreadRef
@ stdcall SHSkipJunction(ptr ptr) SHSkipJunction
@ stdcall SHStrDupA (str ptr) SHStrDupA
@ stdcall SHStrDupW (wstr ptr) SHStrDupW
@ stdcall StrFormatByteSize64A(long long ptr long) StrFormatByteSize64A
@ stdcall StrFormatKBSizeA(long long str long) StrFormatKBSizeA
@ stdcall StrFormatKBSizeW(long long wstr long) StrFormatKBSizeW
@ stdcall StrCmpLogicalW(wstr wstr) StrCmpLogicalW
