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
16  stdcall SHCreateThread(ptr ptr long ptr)
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
222 stdcall -noname _SHGlobalCounterCreate(long)
223 stdcall -noname _SHGlobalCounterGetValue(long)
224 stdcall -noname _SHGlobalCounterIncrement(long)
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
253 stdcall AssocCreate(long long long long ptr ptr)
254 stdcall AssocQueryKeyA(long long str ptr ptr)
255 stdcall AssocQueryKeyW(long long wstr ptr ptr)
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
356 stdcall -noname _CreateAllAccessSecurityAttributes(ptr ptr)
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
381 stdcall AssocQueryStringA(long long ptr ptr str ptr)
382 stdcall AssocQueryStringByKeyA(long long ptr ptr str ptr)
383 stdcall AssocQueryStringByKeyW(long long ptr ptr wstr ptr)
384 stdcall AssocQueryStringW(long long ptr ptr wstr ptr)
385 stdcall ChrCmpIA(long long)
386 stdcall ChrCmpIW(long long)
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
404 stdcall ColorHLSToRGB(long long long)
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
422 stdcall -noname _SHGlobalCounterCreateNamedA(str long)
423 stdcall -noname _SHGlobalCounterCreateNamedW(wstr long)
424 stdcall -noname _SHGlobalCounterDecrement(long)
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
445 stdcall ColorRGBToHLS(long ptr ptr ptr)
446 stub @

@ stdcall DllGetVersion (ptr) SHLWAPI_DllGetVersion
@ stdcall GetMenuPosFromID(ptr long)
@ stdcall HashData (ptr long ptr long)
@ stub    IntlStrEqWorkerA
@ stub    IntlStrEqWorkerW
@ stdcall PathAddBackslashA (str)
@ stdcall PathAddBackslashW (wstr)
@ stdcall PathAddExtensionA (str str)
@ stdcall PathAddExtensionW (wstr wstr)
@ stdcall PathAppendA (str str)
@ stdcall PathAppendW (wstr wstr)
@ stdcall PathBuildRootA (ptr long)
@ stdcall PathBuildRootW (ptr long)
@ stdcall PathCanonicalizeA (ptr str)
@ stdcall PathCanonicalizeW (ptr wstr)
@ stdcall PathCombineA (ptr ptr ptr)
@ stdcall PathCombineW (ptr ptr ptr)
@ stdcall PathCommonPrefixA(str str ptr)
@ stdcall PathCommonPrefixW(wstr wstr ptr)
@ stdcall PathCompactPathA(long str long)
@ stdcall PathCompactPathExA(ptr str long long)
@ stdcall PathCompactPathExW(ptr wstr long long)
@ stdcall PathCompactPathW(long wstr long)
@ stdcall PathCreateFromUrlA(str ptr ptr long)
@ stdcall PathCreateFromUrlW(wstr ptr ptr long)
@ stdcall PathFileExistsA (str)
@ stdcall PathFileExistsW (wstr)
@ stdcall PathFindExtensionA (str)
@ stdcall PathFindExtensionW (wstr)
@ stdcall PathFindFileNameA (str)
@ stdcall PathFindFileNameW (wstr)
@ stdcall PathFindNextComponentA (str)
@ stdcall PathFindNextComponentW (wstr)
@ stdcall PathFindOnPathA (str ptr)
@ stdcall PathFindOnPathW (wstr ptr)
@ stdcall PathGetArgsA (str)
@ stdcall PathGetArgsW (wstr)
@ stdcall PathGetCharTypeA(long)
@ stdcall PathGetCharTypeW(long)
@ stdcall PathGetDriveNumberA (str)
@ stdcall PathGetDriveNumberW (wstr)
@ stdcall PathIsContentTypeA(str str)
@ stdcall PathIsContentTypeW(wstr wstr)
@ stdcall PathIsDirectoryA(str)
@ stdcall PathIsDirectoryW(wstr)
@ stdcall PathIsFileSpecA(str)
@ stdcall PathIsFileSpecW(wstr)
@ stdcall PathIsPrefixA(str str)
@ stdcall PathIsPrefixW(wstr wstr)
@ stdcall PathIsRelativeA (str)
@ stdcall PathIsRelativeW (wstr)
@ stdcall PathIsRootA(str)
@ stdcall PathIsRootW(wstr)
@ stdcall PathIsSameRootA(str str)
@ stdcall PathIsSameRootW(wstr wstr)
@ stdcall PathIsSystemFolderA(str long)
@ stdcall PathIsSystemFolderW(wstr long)
@ stdcall PathIsUNCA (str)
@ stdcall PathIsUNCServerA(str)
@ stdcall PathIsUNCServerShareA(str)
@ stdcall PathIsUNCServerShareW(wstr)
@ stdcall PathIsUNCServerW(wstr)
@ stdcall PathIsUNCW(wstr)
@ stdcall PathIsURLA(str)
@ stdcall PathIsURLW(wstr)
@ stdcall PathMakePrettyA(str)
@ stdcall PathMakePrettyW(wstr)
@ stdcall PathMakeSystemFolderA(str)
@ stdcall PathMakeSystemFolderW(wstr)
@ stdcall PathMatchSpecA  (str str)
@ stdcall PathMatchSpecW  (wstr wstr)
@ stdcall PathParseIconLocationA (str)
@ stdcall PathParseIconLocationW (wstr)
@ stdcall PathQuoteSpacesA (str)
@ stdcall PathQuoteSpacesW (wstr)
@ stdcall PathRelativePathToA(ptr str long str long)
@ stdcall PathRelativePathToW(ptr str long str long)
@ stdcall PathRemoveArgsA(str)
@ stdcall PathRemoveArgsW(wstr)
@ stdcall PathRemoveBackslashA (str)
@ stdcall PathRemoveBackslashW (wstr)
@ stdcall PathRemoveBlanksA(str)
@ stdcall PathRemoveBlanksW(wstr)
@ stdcall PathRemoveExtensionA(str)
@ stdcall PathRemoveExtensionW(wstr)
@ stdcall PathRemoveFileSpecA (str)
@ stdcall PathRemoveFileSpecW (wstr)
@ stdcall PathRenameExtensionA(str str)
@ stdcall PathRenameExtensionW(wstr wstr)
@ stdcall PathSearchAndQualifyA(str ptr long)
@ stdcall PathSearchAndQualifyW(wstr ptr long)
@ stdcall PathSetDlgItemPathA (long long ptr)
@ stdcall PathSetDlgItemPathW (long long ptr)
@ stdcall PathSkipRootA(str)
@ stdcall PathSkipRootW(wstr)
@ stdcall PathStripPathA(str)
@ stdcall PathStripPathW(wstr)
@ stdcall PathStripToRootA(str)
@ stdcall PathStripToRootW(wstr)
@ stdcall PathUnmakeSystemFolderA(str)
@ stdcall PathUnmakeSystemFolderW(wstr)
@ stdcall PathUnquoteSpacesA (str)
@ stdcall PathUnquoteSpacesW (wstr)
@ stdcall SHCreateShellPalette(long)
@ stdcall SHDeleteEmptyKeyA(long ptr)
@ stdcall SHDeleteEmptyKeyW(long ptr)
@ stdcall SHDeleteKeyA(long str)
@ stdcall SHDeleteKeyW(long wstr)
@ stdcall SHDeleteOrphanKeyA(long str)
@ stdcall SHDeleteOrphanKeyW(long wstr)
@ stdcall SHDeleteValueA(long  str  str)
@ stdcall SHDeleteValueW(long wstr wstr)
@ stdcall SHEnumKeyExA(long long str ptr)
@ stdcall SHEnumKeyExW(long long wstr ptr)
@ stdcall SHEnumValueA(long long str ptr ptr ptr ptr)
@ stdcall SHEnumValueW(long long wstr ptr ptr ptr ptr)
@ stdcall SHGetInverseCMAP ( ptr long )
@ stdcall SHGetValueA ( long str str ptr ptr ptr )
@ stdcall SHGetValueW ( long wstr wstr ptr ptr ptr )
@ stdcall SHIsLowMemoryMachine(long)
@ stdcall SHOpenRegStreamA(long str str long)
@ stdcall SHOpenRegStreamW(long wstr str long)
@ stdcall SHOpenRegStream2A(long str str long)
@ stdcall SHOpenRegStream2W(long wstr str long)
@ stdcall SHQueryInfoKeyA(long ptr ptr ptr ptr)
@ stdcall SHQueryInfoKeyW(long ptr ptr ptr ptr)
@ stdcall SHQueryValueExA(long str ptr ptr ptr ptr)
@ stdcall SHQueryValueExW(long wstr ptr ptr ptr ptr)
@ stdcall SHRegCloseUSKey(ptr)
@ stub    SHRegCreateUSKeyA
@ stub    SHRegCreateUSKeyW
@ stub    SHRegDeleteEmptyUSKeyA
@ stub    SHRegDeleteEmptyUSKeyW
@ stub    SHRegDeleteUSValueA
@ stub    SHRegDeleteUSValueW
@ stdcall SHRegEnumUSKeyA(long long str ptr long)
@ stdcall SHRegEnumUSKeyW(long long wstr ptr long)
@ stub    SHRegEnumUSValueA
@ stub    SHRegEnumUSValueW
@ stdcall SHRegGetBoolUSValueA(str str long long)
@ stdcall SHRegGetBoolUSValueW(wstr wstr long long)
@ stdcall SHRegGetUSValueA ( str str ptr ptr ptr long ptr long )
@ stdcall SHRegGetUSValueW ( wstr wstr ptr ptr ptr long ptr long )
@ stdcall SHRegOpenUSKeyA ( str long long long long )
@ stdcall SHRegOpenUSKeyW ( wstr long long long long )
@ stdcall SHRegQueryInfoUSKeyA ( long ptr ptr ptr ptr long )
@ stdcall SHRegQueryInfoUSKeyW ( long ptr ptr ptr ptr long )
@ stdcall SHRegQueryUSValueA ( long str ptr ptr ptr long ptr long )
@ stdcall SHRegQueryUSValueW ( long wstr ptr ptr ptr long ptr long )
@ stdcall SHRegSetUSValueA ( str str long ptr long long)
@ stdcall SHRegSetUSValueW ( wstr wstr long ptr long long)
@ stdcall SHRegWriteUSValueA (long str long ptr long long)
@ stdcall SHRegWriteUSValueW (long str long ptr long long)
@ stdcall SHSetValueA (long  str  str long ptr long)
@ stdcall SHSetValueW (long wstr wstr long ptr long)
@ stdcall StrCSpnA (str str)
@ stdcall StrCSpnIA (str str)
@ stdcall StrCSpnIW (wstr wstr)
@ stdcall StrCSpnW (wstr wstr)
@ stdcall StrCatBuffA (str str long)
@ stdcall StrCatBuffW (wstr wstr long)
@ stdcall StrCatW (ptr wstr)
@ stdcall StrChrA (str long)
@ stdcall StrChrIA (str long)
@ stdcall StrChrIW (wstr long)
@ stdcall StrChrW (wstr long)
@ stdcall StrCmpIW (wstr wstr)
@ stdcall StrCmpNA (str str long)
@ stdcall StrCmpNIA (str str long)
@ stdcall StrCmpNIW (wstr wstr long)
@ stdcall StrCmpNW (wstr wstr long)
@ stdcall StrCmpW (wstr wstr)
@ stdcall StrCpyNW (wstr wstr long)
@ stdcall StrCpyW (ptr wstr)
@ stdcall StrDupA (str)
@ stdcall StrDupW (wstr)
@ stdcall StrFormatByteSizeA(long ptr long)
@ stdcall StrFormatByteSizeW(long long ptr long)
@ stdcall StrFromTimeIntervalA(ptr long long long)
@ stdcall StrFromTimeIntervalW(ptr long long long)
@ stdcall StrIsIntlEqualA(long str str long)
@ stdcall StrIsIntlEqualW(long wstr wstr long)
@ stdcall StrNCatA(str str long)
@ stdcall StrNCatW(wstr wstr long)
@ stdcall StrPBrkA(str str)
@ stdcall StrPBrkW(wstr wstr)
@ stdcall StrRChrA (str str long)
@ stdcall StrRChrIA (str str long)
@ stdcall StrRChrIW (str str long)
@ stdcall StrRChrW (wstr wstr long)
@ stdcall StrRStrIA (str str str)
@ stdcall StrRStrIW (wstr wstr wstr)
@ stdcall StrSpnA (str str)
@ stdcall StrSpnW (wstr wstr)
@ stdcall StrStrA(str str)
@ stdcall StrStrIA(str str)
@ stdcall StrStrIW(wstr wstr)
@ stdcall StrStrW(wstr wstr)
@ stdcall StrToIntA(str)
@ stdcall StrToIntExA(str long ptr)
@ stdcall StrToIntExW(wstr long ptr)
@ stdcall StrToIntW(wstr)
@ stdcall StrTrimA(str str)
@ stdcall StrTrimW(wstr wstr)
@ stdcall UrlApplySchemeA(str ptr ptr long)
@ stdcall UrlApplySchemeW(wstr ptr ptr long)
@ stdcall UrlCanonicalizeA(str ptr ptr long)
@ stdcall UrlCanonicalizeW(wstr ptr ptr long)
@ stdcall UrlCombineA(str str str ptr long)
@ stdcall UrlCombineW(wstr wstr wstr ptr long)
@ stdcall UrlCompareA(str str long)
@ stdcall UrlCompareW(wstr wstr long)
@ stdcall UrlCreateFromPathA(str ptr ptr long)
@ stdcall UrlCreateFromPathW(wstr ptr ptr long)
@ stdcall UrlEscapeA(str ptr ptr long)
@ stdcall UrlEscapeW(wstr ptr ptr long)
@ stdcall UrlGetLocationA(str)
@ stdcall UrlGetLocationW(wstr)
@ stdcall UrlGetPartA(str ptr ptr long long)
@ stdcall UrlGetPartW(wstr ptr ptr long long)
@ stdcall UrlHashA(str ptr long)
@ stdcall UrlHashW(wstr ptr long)
@ stdcall UrlIsA(str long)
@ stdcall UrlIsNoHistoryA(str)
@ stdcall UrlIsNoHistoryW(wstr)
@ stdcall UrlIsOpaqueA(str)
@ stdcall UrlIsOpaqueW(wstr)
@ stdcall UrlIsW(wstr long)
@ stdcall UrlUnescapeA(str ptr ptr long)
@ stdcall UrlUnescapeW(wstr ptr ptr long)
@ varargs wnsprintfA(ptr long str)
@ varargs wnsprintfW(ptr long wstr)
@ stdcall wvnsprintfA(ptr long str ptr)
@ stdcall wvnsprintfW(ptr long wstr ptr)


# exported in later versions
@ stdcall StrRetToBufA (ptr ptr ptr long)
@ stdcall StrRetToBufW (ptr ptr ptr long)
@ stdcall StrRetToStrA (ptr ptr ptr)
@ stdcall StrRetToStrW (ptr ptr ptr)
@ stdcall SHRegGetPathA(long str str ptr long)
@ stdcall SHRegGetPathW(long wstr wstr ptr long)
@ stub    MLLoadLibraryA
@ stub    MLLoadLibraryW
@ stdcall PathIsDirectoryEmptyA(str)
@ stdcall PathIsDirectoryEmptyW(wstr)
@ stdcall PathIsNetworkPathA(str)
@ stdcall PathIsNetworkPathW(wstr)
@ stdcall PathIsLFNFileSpecA(str)
@ stdcall PathIsLFNFileSpecW(wstr)
@ stdcall PathFindSuffixArrayA(str ptr long)
@ stdcall PathFindSuffixArrayW(wstr ptr long)
@ stdcall _SHGetInstanceExplorer(ptr)
@ stdcall PathUndecorateA(str)
@ stdcall PathUndecorateW(wstr)
@ stub    PathUnExpandEnvStringsA
@ stub    PathUnExpandEnvStringsW
@ stdcall SHCopyKeyA(long str long long)
@ stdcall SHCopyKeyW(long wstr long long)
@ stdcall SHAutoComplete(ptr long)
@ stdcall SHCreateStreamOnFileA(str long ptr)
@ stdcall SHCreateStreamOnFileW(wstr long ptr)
@ stdcall SHCreateStreamOnFileEx(wstr long long long ptr ptr)
@ stdcall SHCreateStreamWrapper(ptr ptr long ptr)
@ stdcall SHGetThreadRef (ptr)
@ stdcall SHRegDuplicateHKey (long)
@ stdcall SHRegSetPathA(long str str str long)
@ stdcall SHRegSetPathW(long wstr wstr wstr long)
@ stub    SHRegisterValidateTemplate
@ stdcall SHSetThreadRef (ptr)
@ stdcall SHReleaseThreadRef()
@ stdcall SHSkipJunction(ptr ptr)
@ stdcall SHStrDupA (str ptr)
@ stdcall SHStrDupW (wstr ptr)
@ stdcall StrFormatByteSize64A(long long ptr long)
@ stdcall StrFormatKBSizeA(long long str long)
@ stdcall StrFormatKBSizeW(long long wstr long)
@ stdcall StrCmpLogicalW(wstr wstr)
