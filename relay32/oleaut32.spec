name oleaut32
type win32

1 stub DllGetClassObject
2 stdcall SysAllocString(wstr) SysAllocString32
3 stdcall SysReAllocString(ptr wstr) SysReAllocString32
4 stdcall SysAllocStringLen(wstr long) SysAllocStringLen32
5 stdcall SysReAllocStringLen(ptr ptr long) SysReAllocStringLen32
6 stdcall SysFreeString(wstr) SysFreeString32
7 stdcall SysStringLen(wstr) SysStringLen32
8 stdcall VariantInit(ptr) VariantInit32
9 stdcall VariantClear(ptr) VariantClear32
10 stdcall VariantCopy(ptr ptr) VariantCopy32
11 stdcall VariantCopyInd(ptr ptr) VariantCopyInd32
12 stdcall VariantChangeType(ptr ptr) VariantChangeType32
13 stub VariantTimeToDosDateTime
14 stub DosDateTimeToVariantTime
15 stub SafeArrayCreate
16 stub SafeArrayDestroy
17 stub SafeArrayGetDim
18 stub SafeArrayGetElemsize
19 stub SafeArrayGetUBound
20 stub SafeArrayGetLBound
21 stub SafeArrayLock
22 stub SafeArrayUnlock
23 stub SafeArrayAccessData
24 stub SafeArrayUnaccessData
25 stub SafeArrayGetElement
26 stub SafeArrayPutElement
27 stub SafeArrayCopy
28 stub DispGetParam
29 stub DispGetIDsOfNames
30 stub DispInvoke
31 stub CreateDispTypeInfo
32 stub CreateStdDispatch
33 stub RegisterActiveObject
34 stub RevokeActiveObject
35 stub GetActiveObject
36 stub SafeArrayAllocDescriptor
37 stub SafeArrayAllocData
38 stub SafeArrayDestroyDescriptor
39 stub SafeArrayDestroyData
40 stub SafeArrayRedim
41 stub OACreateTypeLib2
46 stub VarParseNumFromStr
47 stub VarNumFromParseNum
48 stdcall VarI2FromUI1(long ptr) VarI2FromUI132
49 stdcall VarI2FromI4(long ptr) VarI2FromI432
50 stdcall VarI2FromR4(long ptr) VarI2FromR432
51 stdcall VarI2FromR8(double ptr) VarI2FromR832
52 stub VarI2FromCy
53 stdcall VarI2FromDate(long ptr) VarI2FromDate32
54 stdcall VarI2FromStr(wstr long long ptr) VarI2FromStr32
55 stub VarI2FromDisp
56 stdcall VarI2FromBool(long ptr) VarI2FromBool32
58 stdcall VarI4FromUI1(long ptr) VarI4FromUI132
59 stdcall VarI4FromI2(long ptr) VarI4FromI232
60 stdcall VarI4FromR4(long ptr) VarI4FromR432
61 stdcall VarI4FromR8(double ptr) VarI4FromR832
62 stub VarI4FromCy
63 stdcall VarI4FromDate(long ptr) VarI4FromDate32
64 stdcall VarI4FromStr(wstr long long ptr) VarI4FromStr32
65 stub VarI4FromDisp
66 stdcall VarI4FromBool(long ptr) VarI4FromBool32
68 stdcall VarR4FromUI1(long ptr) VarR4FromUI132
69 stdcall VarR4FromI2(long ptr) VarR4FromI232
70 stdcall VarR4FromI4(long ptr) VarR4FromI432
71 stdcall VarR4FromR8(double ptr) VarR4FromR832
72 stub VarR4FromCy
73 stdcall VarR4FromDate(long ptr) VarR4FromDate32
74 stdcall VarR4FromStr(wstr long long ptr) VarR4FromStr32
75 stub VarR4FromDisp
76 stdcall VarR4FromBool(long ptr) VarR4FromBool32
78 stdcall VarR8FromUI1(long ptr) VarR8FromUI132
79 stdcall VarR8FromI2(long ptr) VarR8FromI232
80 stdcall VarR8FromI4(long ptr) VarR8FromI432
81 stdcall VarR8FromR4(long ptr) VarR8FromR432
82 stub VarR8FromCy
83 stdcall VarR8FromDate(long ptr) VarR8FromDate32
84 stdcall VarR8FromStr(wstr long long ptr) VarR8FromStr32
85 stub VarR8FromDisp
86 stdcall VarR8FromBool(long ptr) VarR8FromBool32
88 stdcall VarDateFromUI1(long ptr) VarDateFromUI132
89 stdcall VarDateFromI2(long ptr) VarDateFromI232
90 stdcall VarDateFromI4(long ptr) VarDateFromI432
91 stdcall VarDateFromR4(long ptr) VarDateFromR432
92 stdcall VarDateFromR8(double ptr) VarDateFromR832
93 stub VarDateFromCy
94 stdcall VarDateFromStr(wstr long long ptr) VarDateFromStr32
95 stub VarDateFromDisp
96 stdcall VarDateFromBool(long ptr) VarDateFromBool32
98 stub VarCyFromUI1
99 stub VarCyFromI2
100 stub VarCyFromI4
101 stub VarCyFromR4
102 stub VarCyFromR8
103 stub VarCyFromDate
104 stub VarCyFromStr
105 stub VarCyFromDisp
106 stub VarCyFromBool
108 stdcall VarBstrFromUI1(long long long ptr) VarBstrFromUI132
109 stdcall VarBstrFromI2(long long long ptr) VarBstrFromI232
110 stdcall VarBstrFromI4(long long long ptr) VarBstrFromI432
111 stdcall VarBstrFromR4(long long long ptr) VarBstrFromR432
112 stdcall VarBstrFromR8(double long long ptr) VarBstrFromR832
113 stub VarBstrFromCy
114 stdcall VarBstrFromDate(long long long ptr) VarBstrFromDate32
115 stub VarBstrFromDisp
116 stdcall VarBstrFromBool(long long long ptr) VarBstrFromBool32
118 stdcall VarBoolFromUI1(long ptr) VarBoolFromUI132
119 stdcall VarBoolFromI2(long ptr) VarBoolFromI232
120 stdcall VarBoolFromI4(long ptr) VarBoolFromI432
121 stdcall VarBoolFromR4(long ptr) VarBoolFromR432
122 stdcall VarBoolFromR8(double ptr) VarBoolFromR832
123 stdcall VarBoolFromDate(long ptr) VarBoolFromDate32
124 stub VarBoolFromCy
125 stdcall VarBoolFromStr(wstr long long ptr) VarBoolFromStr32
126 stub VarBoolFromDisp
130 stdcall VarUI1FromI2(long ptr) VarUI1FromI232
131 stdcall VarUI1FromI4(long ptr) VarUI1FromI432
132 stdcall VarUI1FromR4(long ptr) VarUI1FromR432
133 stdcall VarUI1FromR8(double ptr) VarUI1FromR832
134 stub VarUI1FromCy
135 stdcall VarUI1FromDate(long ptr) VarUI1FromDate32
136 stdcall VarUI1FromStr(wstr long long ptr) VarUI1FromStr32
137 stub VarUI1FromDisp
138 stdcall VarUI1FromBool(long ptr) VarUI1FromBool32
146 stub DispCallFunc
147 stdcall VariantChangeTypeEx(ptr ptr) VariantChangeTypeEx32
148 stub SafeArrayPtrOfIndex
149 stub SysStringByteLen
150 stub SysAllocStringByteLen
160 stub CreateTypeLib
161 stdcall LoadTypeLib (ptr ptr) LoadTypeLib32
162 stub LoadRegTypeLib 
163 stdcall RegisterTypeLib(ptr str str) RegisterTypeLib32
164 stdcall QueryPathOfRegTypeLib(ptr long long long ptr) QueryPathOfRegTypeLib32
165 stub LHashValOfNameSys
166 stub LHashValOfNameSysA
170 stdcall OaBuildVersion() OaBuildVersion
171 stub ClearCustData
180 stub CreateTypeLib2
183 stub LoadTypeLibEx
184 stub SystemTimeToVariantTime
185 stub VariantTimeToSystemTime
186 stub UnRegisterTypeLib
190 stub VarDecFromUI1
191 stub VarDecFromI2
192 stub VarDecFromI4
193 stub VarDecFromR4
194 stub VarDecFromR8
195 stub VarDecFromDate
196 stub VarDecFromCy
197 stub VarDecFromStr
198 stub VarDecFromDisp
199 stub VarDecFromBool
200 stub GetErrorInfo
201 stub SetErrorInfo
202 stub CreateErrorInfo
205 stub VarI2FromI1
206 stdcall VarI2FromUI2(long ptr) VarI2FromUI232
207 stdcall VarI2FromUI4(long ptr) VarI2FromUI432
208 stub VarI2FromDec
209 stdcall VarI4FromI1(long ptr) VarI4FromI132
210 stdcall VarI4FromUI2(long ptr) VarI4FromUI232
211 stdcall VarI4FromUI4(long ptr) VarI4FromUI432
212 stub VarI4FromDec
213 stdcall VarR4FromI1(long ptr) VarR4FromI132
214 stdcall VarR4FromUI2(long ptr) VarR4FromUI232
215 stdcall VarR4FromUI4(long ptr) VarR4FromUI432
216 stub VarR4FromDec
217 stdcall VarR8FromI1(long ptr) VarR8FromI132
218 stdcall VarR8FromUI2(long ptr) VarR8FromUI232
219 stdcall VarR8FromUI4(long ptr) VarR8FromUI432
220 stub VarR8FromDec
221 stdcall VarDateFromI1(long ptr) VarDateFromI132
222 stdcall VarDateFromUI2(long ptr) VarDateFromUI232
223 stdcall VarDateFromUI4(long ptr) VarDateFromUI432
224 stub VarDateFromDec
225 stub VarCyFromI1
226 stub VarCyFromUI2
227 stub VarCyFromUI4
228 stub VarCyFromDec
229 stdcall VarBstrFromI1(long long long ptr) VarBstrFromI132
230 stdcall VarBstrFromUI2(long long long ptr) VarBstrFromUI232
231 stdcall VarBstrFromUI4(long long long ptr) VarBstrFromUI432
232 stub VarBstrFromDec
233 stdcall VarBoolFromI1(long ptr) VarBoolFromI132
234 stdcall VarBoolFromUI2(long ptr) VarBoolFromUI232
235 stdcall VarBoolFromUI4(long ptr) VarBoolFromUI432
236 stub VarBoolFromDec
237 stdcall VarUI1FromI1(long ptr) VarUI1FromI132
238 stdcall VarUI1FromUI2(long ptr) VarUI1FromUI232
239 stdcall VarUI1FromUI4(long ptr) VarUI1FromUI432
240 stub VarUI1FromDec
241 stub VarDecFromI1
242 stub VarDecFromUI2
243 stub VarDecFromUI4
244 stdcall VarI1FromUI1(long ptr) VarI1FromUI132
245 stdcall VarI1FromI2(long ptr) VarI1FromI232
246 stdcall VarI1FromI4(long ptr) VarI1FromI432
247 stdcall VarI1FromR4(long ptr) VarI1FromR432
248 stdcall VarI1FromR8(double ptr) VarI1FromR832
249 stdcall VarI1FromDate(long ptr) VarI1FromDate32
250 stub VarI1FromCy
251 stdcall VarI1FromStr(wstr long long ptr) VarI1FromStr32
252 stub VarI1FromDisp
253 stdcall VarI1FromBool(long ptr) VarI1FromBool32
254 stdcall VarI1FromUI2(long ptr) VarI1FromUI232
255 stdcall VarI1FromUI4(long ptr) VarI1FromUI432
256 stub VarI1FromDec
257 stdcall VarUI2FromUI1(long ptr) VarUI2FromUI132
258 stdcall VarUI2FromI2(long ptr) VarUI2FromI232
259 stdcall VarUI2FromI4(long ptr) VarUI2FromI432
260 stdcall VarUI2FromR4(long ptr) VarUI2FromR432
261 stdcall VarUI2FromR8(double ptr) VarUI2FromR832
262 stdcall VarUI2FromDate(long ptr) VarUI2FromDate32
263 stub VarUI2FromCy
264 stdcall VarUI2FromStr(wstr long long ptr) VarUI2FromStr32
265 stub VarUI2FromDisp
266 stdcall VarUI2FromBool(long ptr) VarUI2FromBool32
267 stdcall VarUI2FromI1(long ptr) VarUI2FromI132
268 stdcall VarUI2FromUI4(long ptr) VarUI2FromUI432
269 stub VarUI2FromDec
270 stdcall VarUI4FromUI1(long ptr) VarUI4FromUI132
271 stdcall VarUI4FromI2(long ptr) VarUI4FromI232
272 stdcall VarUI4FromI4(long ptr) VarUI4FromI432
273 stdcall VarUI4FromR4(long ptr) VarUI4FromR432
274 stdcall VarUI4FromR8(double ptr) VarUI4FromR832
275 stdcall VarUI4FromDate(long ptr) VarUI4FromDate32
276 stub VarUI4FromCy
277 stdcall VarUI4FromStr(wstr long long ptr) VarUI4FromStr32
278 stub VarUI4FromDisp
279 stdcall VarUI4FromBool(long ptr) VarUI4FromBool32
280 stdcall VarUI4FromI1(long ptr) VarUI4FromI132
281 stdcall VarUI4FromUI2(long ptr) VarUI4FromUI232
282 stub VarUI4FromDec
283 stub BSTR_UserSize
284 stub BSTR_UserMarshal
285 stub BSTR_UserUnmarshal
286 stub BSTR_UserFree
287 stub VARIANT_UserSize
288 stub VARIANT_UserMarshal
289 stub VARIANT_UserUnmarshal
290 stub VARIANT_UserFree
291 stub LPSAFEARRAY_UserSize
292 stub LPSAFEARRAY_UserMarshal
293 stub LPSAFEARRAY_UserUnmarshal
294 stub LPSAFEARRAY_UserFree
295 stub LPSAFEARRAY_Size
296 stub LPSAFEARRAY_Marshal
297 stub LPSAFEARRAY_Unmarshal
320 stub DllRegisterServer
321 stub DllUnregisterServer
330 stub VarDateFromUdate
331 stub VarUdateFromDate
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
410 stub DllCanUnloadNow
411 stub SafeArrayCreateVector
412 stub SafeArrayCopyData
413 stub VectorFromBstr
414 stub BstrFromVector
415 stub OleIconToCursor
416 stub OleCreatePropertyFrameIndirect
417 stub OleCreatePropertyFrame
418 stub OleLoadPicture
419 stub OleCreatePictureIndirect
420 stub OleCreateFontIndirect
421 stub OleTranslateColor
422 stub OleLoadPictureFile
423 stub OleSavePictureFile
424 stub OleLoadPicturePath
