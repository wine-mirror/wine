1 stdcall DllGetClassObject(ptr ptr ptr) OLEAUT32_DllGetClassObject
2 stdcall SysAllocString(wstr)
3 stdcall SysReAllocString(ptr wstr)
4 stdcall SysAllocStringLen(wstr long)
5 stdcall SysReAllocStringLen(ptr ptr long)
6 stdcall SysFreeString(wstr)
7 stdcall SysStringLen(wstr)
8 stdcall VariantInit(ptr)
9 stdcall VariantClear(ptr)
10 stdcall VariantCopy(ptr ptr)
11 stdcall VariantCopyInd(ptr ptr)
12 stdcall VariantChangeType(ptr ptr long long)
13 stdcall VariantTimeToDosDateTime(double ptr ptr)
14 stdcall DosDateTimeToVariantTime(long long ptr)
15 stdcall SafeArrayCreate(long long ptr)
16 stdcall SafeArrayDestroy(ptr)
17 stdcall SafeArrayGetDim(ptr)
18 stdcall SafeArrayGetElemsize(ptr)
19 stdcall SafeArrayGetUBound(ptr long long)
20 stdcall SafeArrayGetLBound(ptr long long)
21 stdcall SafeArrayLock(ptr)
22 stdcall SafeArrayUnlock(ptr)
23 stdcall SafeArrayAccessData(ptr ptr)
24 stdcall SafeArrayUnaccessData(ptr)
25 stdcall SafeArrayGetElement(ptr ptr ptr)
26 stdcall SafeArrayPutElement(ptr ptr ptr)
27 stdcall SafeArrayCopy(ptr ptr)
28 stdcall DispGetParam(ptr long long ptr ptr)
29 stdcall DispGetIDsOfNames(ptr ptr long ptr)
30 stdcall DispInvoke(ptr ptr long long ptr ptr ptr ptr)
31 stdcall CreateDispTypeInfo(ptr long ptr)
32 stdcall CreateStdDispatch(ptr ptr ptr ptr)
33 stdcall RegisterActiveObject(ptr ptr long ptr)
34 stdcall RevokeActiveObject(long ptr)
35 stdcall GetActiveObject(ptr ptr ptr)
36 stdcall SafeArrayAllocDescriptor(long ptr)
37 stdcall SafeArrayAllocData(ptr)
38 stdcall SafeArrayDestroyDescriptor(ptr)
39 stdcall SafeArrayDestroyData(ptr)
40 stdcall SafeArrayRedim(ptr ptr)
41 stdcall SafeArrayAllocDescriptorEx(long long ptr)
42 stub SafeArrayCreateEx
43 stub SafeArrayCreateVectorEx
44 stdcall SafeArraySetRecordInfo(ptr ptr)
45 stdcall SafeArrayGetRecordInfo(ptr ptr)
46 stdcall VarParseNumFromStr(wstr long long ptr ptr)
47 stdcall VarNumFromParseNum(ptr ptr long ptr)
48 stdcall VarI2FromUI1(long ptr)
49 stdcall VarI2FromI4(long ptr)
50 stdcall VarI2FromR4(long ptr)
51 stdcall VarI2FromR8(double ptr)
52 stdcall VarI2FromCy(double ptr)
53 stdcall VarI2FromDate(double ptr)
54 stdcall VarI2FromStr(wstr long long ptr)
55 stub VarI2FromDisp
56 stdcall VarI2FromBool(long ptr)
57 stdcall SafeArraySetIID(ptr ptr)
58 stdcall VarI4FromUI1(long ptr)
59 stdcall VarI4FromI2(long ptr)
60 stdcall VarI4FromR4(long ptr)
61 stdcall VarI4FromR8(double ptr)
62 stdcall VarI4FromCy(double ptr)
63 stdcall VarI4FromDate(double ptr)
64 stdcall VarI4FromStr(wstr long long ptr)
65 stub VarI4FromDisp
66 stdcall VarI4FromBool(long ptr)
67 stdcall SafeArrayGetIID(ptr ptr)
68 stdcall VarR4FromUI1(long ptr)
69 stdcall VarR4FromI2(long ptr)
70 stdcall VarR4FromI4(long ptr)
71 stdcall VarR4FromR8(double ptr)
72 stdcall VarR4FromCy(double ptr)
73 stdcall VarR4FromDate(double ptr)
74 stdcall VarR4FromStr(wstr long long ptr)
75 stub VarR4FromDisp
76 stdcall VarR4FromBool(long ptr)
77 stdcall SafeArrayGetVartype(ptr ptr)
78 stdcall VarR8FromUI1(long ptr)
79 stdcall VarR8FromI2(long ptr)
80 stdcall VarR8FromI4(long ptr)
81 stdcall VarR8FromR4(long ptr)
82 stdcall VarR8FromCy(double ptr)
83 stdcall VarR8FromDate(double ptr)
84 stdcall VarR8FromStr(wstr long long ptr)
85 stub VarR8FromDisp
86 stdcall VarR8FromBool(long ptr)
87 stdcall VarFormat(ptr ptr long long long ptr)
88 stdcall VarDateFromUI1(long ptr)
89 stdcall VarDateFromI2(long ptr)
90 stdcall VarDateFromI4(long ptr)
91 stdcall VarDateFromR4(long ptr)
92 stdcall VarDateFromR8(double ptr)
93 stdcall VarDateFromCy(double ptr)
94 stdcall VarDateFromStr(wstr long long ptr)
95 stub VarDateFromDisp
96 stdcall VarDateFromBool(long ptr)
97 stdcall VarFormatDateTime(ptr long long ptr)
98 stdcall VarCyFromUI1(long ptr)
99 stdcall VarCyFromI2(long ptr)
100 stdcall VarCyFromI4(long ptr)
101 stdcall VarCyFromR4(long ptr)
102 stdcall VarCyFromR8(double ptr)
103 stdcall VarCyFromDate(double ptr)
104 stdcall VarCyFromStr(ptr long long ptr)
105 stub VarCyFromDisp
106 stdcall VarCyFromBool(long ptr)
107 stub VarFormatNumber # stdcall (ptr long long long long long ptr)
108 stdcall VarBstrFromUI1(long long long ptr)
109 stdcall VarBstrFromI2(long long long ptr)
110 stdcall VarBstrFromI4(long long long ptr)
111 stdcall VarBstrFromR4(long long long ptr)
112 stdcall VarBstrFromR8(double long long ptr)
113 stdcall VarBstrFromCy(double long long ptr)
114 stdcall VarBstrFromDate(double long long ptr)
115 stub VarBstrFromDisp
116 stdcall VarBstrFromBool(long long long ptr)
117 stub VarFormatPercent # stdcall (ptr long long long long long ptr)
118 stdcall VarBoolFromUI1(long ptr)
119 stdcall VarBoolFromI2(long ptr)
120 stdcall VarBoolFromI4(long ptr)
121 stdcall VarBoolFromR4(long ptr)
122 stdcall VarBoolFromR8(double ptr)
123 stdcall VarBoolFromDate(double ptr)
124 stdcall VarBoolFromCy(double ptr)
125 stdcall VarBoolFromStr(wstr long long ptr)
126 stub VarBoolFromDisp
127 stdcall VarFormatCurrency(ptr long long long long long ptr)
128 stub VarWeekdayName # stdcall (long long long long ptr)
129 stub VarMonthName # stdcall (long long long ptr)
130 stdcall VarUI1FromI2(long ptr)
131 stdcall VarUI1FromI4(long ptr)
132 stdcall VarUI1FromR4(long ptr)
133 stdcall VarUI1FromR8(double ptr)
134 stdcall VarUI1FromCy(double ptr)
135 stdcall VarUI1FromDate(double ptr)
136 stdcall VarUI1FromStr(wstr long long ptr)
137 stub VarUI1FromDisp
138 stdcall VarUI1FromBool(long ptr)
139 stdcall VarFormatFromTokens (ptr ptr ptr long ptr long)
140 stdcall VarTokenizeFormatString (ptr ptr long long long long ptr)
141 stdcall VarAdd(ptr ptr ptr)
142 stdcall VarAnd(ptr ptr ptr)
143 stdcall VarDiv(ptr ptr ptr)
144 stub OACreateTypeLib2
146 stdcall DispCallFunc(ptr long long long long ptr ptr ptr)
147 stdcall VariantChangeTypeEx(ptr ptr long long long)
148 stdcall SafeArrayPtrOfIndex(ptr ptr ptr)
149 stdcall SysStringByteLen(ptr)
150 stdcall SysAllocStringByteLen(ptr long)
152 stub VarEqv # stdcall (ptr ptr ptr)
153 stub VarIdiv # stdcall (ptr ptr ptr)
154 stub VarImp # stdcall (ptr ptr ptr)
155 stub VarMod # stdcall (ptr ptr ptr)
156 stdcall VarMul(ptr ptr ptr)
157 stdcall VarOr(ptr ptr ptr)
158 stub VarPow # stdcall (ptr ptr ptr)
159 stdcall VarSub(ptr ptr ptr)
160 stdcall CreateTypeLib(long wstr ptr)
161 stdcall LoadTypeLib (wstr ptr)
162 stdcall LoadRegTypeLib (ptr long long long ptr)
163 stdcall RegisterTypeLib(ptr wstr wstr)
164 stdcall QueryPathOfRegTypeLib(ptr long long long ptr)
165 stdcall LHashValOfNameSys(long long wstr)
166 stdcall LHashValOfNameSysA(long long str)
167 stub VarXor # stdcall (ptr ptr ptr)
168 stub VarAbs # stdcall (ptr ptr)
169 stub VarFix # stdcall (ptr ptr)
170 stdcall OaBuildVersion()
171 stub ClearCustData
172 stub VarInt # stdcall (ptr ptr)
173 stub VarNeg # stdcall (ptr ptr)
174 stdcall VarNot(ptr ptr)
175 stub VarRound # stdcall (ptr long ptr)
176 stdcall VarCmp(ptr ptr long long)
177 stub VarDecAdd # stdcall (ptr ptr ptr)
178 stub VarDecDiv # stdcall (ptr ptr ptr)
179 stub VarDecMul # stdcall (ptr ptr ptr)
180 stub CreateTypeLib2
181 stub VarDecSub # stdcall (ptr ptr ptr)
182 stub VarDecAbs # stdcall (ptr ptr)
183 stdcall LoadTypeLibEx (wstr long ptr)
184 stdcall SystemTimeToVariantTime(ptr ptr)
185 stdcall VariantTimeToSystemTime(double ptr)
186 stdcall UnRegisterTypeLib (ptr long long long long)
187 stub VarDecFix # stdcall (ptr ptr)
188 stub VarDecInt # stdcall (ptr ptr)
189 stub VarDecNeg # stdcall (ptr ptr)
190 stub VarDecFromUI1
191 stub VarDecFromI2
192 stub VarDecFromI4
193 stub VarDecFromR4
194 stub VarDecFromR8
195 stub VarDecFromDate
196 stub VarDecFromCy
197 stdcall VarDecFromStr(wstr long long ptr)
198 stub VarDecFromDisp
199 stub VarDecFromBool
200 stdcall GetErrorInfo(long ptr) ole32.GetErrorInfo
201 stdcall SetErrorInfo(long ptr) ole32.SetErrorInfo
202 stdcall CreateErrorInfo(ptr) ole32.CreateErrorInfo
203 stub VarDecRound # stdcall (ptr long ptr)
204 stub VarDecCmp # stdcall (ptr ptr)
205 stdcall VarI2FromI1(long ptr)
206 stdcall VarI2FromUI2(long ptr)
207 stdcall VarI2FromUI4(long ptr)
208 stub VarI2FromDec
209 stdcall VarI4FromI1(long ptr)
210 stdcall VarI4FromUI2(long ptr)
211 stdcall VarI4FromUI4(long ptr)
212 stub VarI4FromDec
213 stdcall VarR4FromI1(long ptr)
214 stdcall VarR4FromUI2(long ptr)
215 stdcall VarR4FromUI4(long ptr)
216 stub VarR4FromDec
217 stdcall VarR8FromI1(long ptr)
218 stdcall VarR8FromUI2(long ptr)
219 stdcall VarR8FromUI4(long ptr)
220 stub VarR8FromDec
221 stdcall VarDateFromI1(long ptr)
222 stdcall VarDateFromUI2(long ptr)
223 stdcall VarDateFromUI4(long ptr)
224 stub VarDateFromDec
225 stdcall VarCyFromI1(long ptr)
226 stdcall VarCyFromUI2(long ptr)
227 stdcall VarCyFromUI4(long ptr)
228 stub VarCyFromDec
229 stdcall VarBstrFromI1(long long long ptr)
230 stdcall VarBstrFromUI2(long long long ptr)
231 stdcall VarBstrFromUI4(long long long ptr)
232 stub VarBstrFromDec
233 stdcall VarBoolFromI1(long ptr)
234 stdcall VarBoolFromUI2(long ptr)
235 stdcall VarBoolFromUI4(long ptr)
236 stub VarBoolFromDec
237 stdcall VarUI1FromI1(long ptr)
238 stdcall VarUI1FromUI2(long ptr)
239 stdcall VarUI1FromUI4(long ptr)
240 stub VarUI1FromDec
241 stub VarDecFromI1
242 stub VarDecFromUI2
243 stub VarDecFromUI4
244 stdcall VarI1FromUI1(long ptr)
245 stdcall VarI1FromI2(long ptr)
246 stdcall VarI1FromI4(long ptr)
247 stdcall VarI1FromR4(long ptr)
248 stdcall VarI1FromR8(double ptr)
249 stdcall VarI1FromDate(double ptr)
250 stdcall VarI1FromCy(double ptr)
251 stdcall VarI1FromStr(wstr long long ptr)
252 stub VarI1FromDisp
253 stdcall VarI1FromBool(long ptr)
254 stdcall VarI1FromUI2(long ptr)
255 stdcall VarI1FromUI4(long ptr)
256 stub VarI1FromDec
257 stdcall VarUI2FromUI1(long ptr)
258 stdcall VarUI2FromI2(long ptr)
259 stdcall VarUI2FromI4(long ptr)
260 stdcall VarUI2FromR4(long ptr)
261 stdcall VarUI2FromR8(double ptr)
262 stdcall VarUI2FromDate(double ptr)
263 stdcall VarUI2FromCy(double ptr)
264 stdcall VarUI2FromStr(wstr long long ptr)
265 stub VarUI2FromDisp
266 stdcall VarUI2FromBool(long ptr)
267 stdcall VarUI2FromI1(long ptr)
268 stdcall VarUI2FromUI4(long ptr)
269 stub VarUI2FromDec
270 stdcall VarUI4FromUI1(long ptr)
271 stdcall VarUI4FromI2(long ptr)
272 stdcall VarUI4FromI4(long ptr)
273 stdcall VarUI4FromR4(long ptr)
274 stdcall VarUI4FromR8(double ptr)
275 stdcall VarUI4FromDate(double ptr)
276 stdcall VarUI4FromCy(double ptr)
277 stdcall VarUI4FromStr(wstr long long ptr)
278 stub VarUI4FromDisp
279 stdcall VarUI4FromBool(long ptr)
280 stdcall VarUI4FromI1(long ptr)
281 stdcall VarUI4FromUI2(long ptr)
282 stub VarUI4FromDec
283 stdcall BSTR_UserSize(ptr long ptr)
284 stdcall BSTR_UserMarshal(ptr ptr ptr)
285 stdcall BSTR_UserUnmarshal(ptr ptr ptr)
286 stdcall BSTR_UserFree(ptr ptr)
287 stdcall VARIANT_UserSize(ptr long ptr)
288 stdcall VARIANT_UserMarshal(ptr ptr ptr)
289 stdcall VARIANT_UserUnmarshal(ptr ptr ptr)
290 stdcall VARIANT_UserFree(ptr ptr)
291 stub LPSAFEARRAY_UserSize
292 stub LPSAFEARRAY_UserMarshal
293 stub LPSAFEARRAY_UserUnmarshal
294 stub LPSAFEARRAY_UserFree
295 stub LPSAFEARRAY_Size
296 stub LPSAFEARRAY_Marshal
297 stub LPSAFEARRAY_Unmarshal
298 stub VarDecCmpR8 # stdcall (ptr double)
299 stub VarCyAdd
303 stub VarCyMul
304 stdcall VarCyMulI4(double long ptr)
305 stub VarCySub
306 stub VarCyAbs
307 stub VarCyFix
308 stub VarCyInt
309 stub VarCyNeg
310 stub VarCyRound
311 stub VarCyCmp
312 stub VarCyCmpR8
313 stdcall VarBstrCat(wstr wstr ptr)
314 stdcall VarBstrCmp(wstr wstr long long)
315 stub VarR8Pow # stdcall (double double ptr)
316 stub VarR4CmpR8
317 stub VarR8Round # stdcall (double long ptr)
318 stdcall VarCat(ptr ptr ptr)
319 stub VarDateFromUdateEx # stdcall (ptr long long ptr)
320 stdcall DllRegisterServer() OLEAUT32_DllRegisterServer
321 stdcall DllUnregisterServer() OLEAUT32_DllUnregisterServer
322 stub GetRecordInfoFromGuids # stdcall (ptr long long long ptr ptr)
323 stub GetRecordInfoFromTypeInfo # stdcall (ptr ptr)
330 stdcall VarDateFromUdate(ptr long ptr)
331 stdcall VarUdateFromDate(double long ptr)
332 stub GetAltMonthNames
380 stub UserHWND_from_local
381 stub UserHWND_to_local
382 stub UserHWND_free_inst
383 stub UserHWND_free_local
384 stub UserBSTR_from_local
385 stub UserBSTR_to_local
386 stub UserBSTR_free_inst
387 stub UserBSTR_free_local
388 stub UserVARIANT_from_local
389 stub UserVARIANT_to_local
390 stub UserVARIANT_free_inst
391 stub UserVARIANT_free_local
392 stub UserEXCEPINFO_from_local
393 stub UserEXCEPINFO_to_local
394 stub UserEXCEPINFO_free_inst
395 stub UserEXCEPINFO_free_local
396 stub UserMSG_from_local
397 stub UserMSG_to_local
398 stub UserMSG_free_inst
399 stub UserMSG_free_local
401 stdcall OleLoadPictureEx(ptr long long long long long long ptr)
402 stub OleLoadPictureFileEx
410 stdcall DllCanUnloadNow() OLEAUT32_DllCanUnloadNow
411 stdcall SafeArrayCreateVector(long long long)
412 stdcall SafeArrayCopyData(ptr ptr)
413 stub VectorFromBstr
414 stub BstrFromVector
415 stdcall OleIconToCursor(long long)
416 stdcall OleCreatePropertyFrameIndirect(ptr)
417 stdcall OleCreatePropertyFrame(ptr long long ptr long ptr long ptr ptr long ptr)
418 stdcall OleLoadPicture(ptr long long ptr ptr)
419 stdcall OleCreatePictureIndirect(ptr ptr long ptr)
420 stdcall OleCreateFontIndirect(ptr ptr ptr)
421 stdcall OleTranslateColor(long long long)
422 stub OleLoadPictureFile
423 stub OleSavePictureFile
424 stub OleLoadPicturePath
