name	wprocs
type	win16

1  pascal WINPROC_CallProc16To32A(word word word long long) WINPROC_CallProc16To32A
2  pascal StaticWndProc(word word word long) StaticWndProc
3  pascal ScrollBarWndProc(word word word long) ScrollBarWndProc
4  pascal ListBoxWndProc(word word word long) ListBoxWndProc
5  pascal ComboBoxWndProc(word word word long) ComboBoxWndProc
6  pascal EditWndProc(word word word long) EditWndProc
7  pascal PopupMenuWndProc(word word word long) PopupMenuWndProc
9  pascal DefDlgProc(word word word long) DefDlgProc16
10 pascal MDIClientWndProc(word word word long) MDIClientWndProc
13 pascal SystemMessageBoxProc(word word word long) SystemMessageBoxProc
14 pascal FileOpenDlgProc(word word word long) FileOpenDlgProc
15 pascal FileSaveDlgProc(word word word long) FileSaveDlgProc
16 pascal ColorDlgProc(word word word long) ColorDlgProc
17 pascal FindTextDlgProc(word word word long) FindTextDlgProc
18 pascal ReplaceTextDlgProc(word word word long) ReplaceTextDlgProc
19 pascal PrintSetupDlgProc(word word word long) PrintSetupDlgProc
20 pascal PrintDlgProc(word word word long) PrintDlgProc
21 pascal AboutDlgProc(word word word long) AboutDlgProc
22 pascal ComboLBoxWndProc(word word word long) ComboLBoxWndProc
24 pascal16 TASK_Reschedule() TASK_Reschedule
26 register Win32CallToStart() PE_Win32CallToStart
27 pascal EntryAddrProc(word word) MODULE_GetEntryPoint
28 pascal MyAlloc(word word word) MODULE_AllocateSegment
30 pascal FormatCharDlgProc(word word word long) FormatCharDlgProc
31 pascal16 FontStyleEnumProc(ptr ptr word long)   FontStyleEnumProc
32 pascal16 FontFamilyEnumProc(ptr ptr word long)  FontFamilyEnumProc
 
# Interrupt vectors 0-255 are ordinals 100-355
# The 'word' parameter are the flags pushed on the stack by the interrupt
100 register INT_Int00Handler(word) INT_DummyHandler
101 register INT_Int01Handler(word) INT_DummyHandler
102 register INT_Int02Handler(word) INT_DummyHandler
103 register INT_Int03Handler(word) INT_DummyHandler
104 register INT_Int04Handler(word) INT_DummyHandler
105 register INT_Int05Handler(word) INT_DummyHandler
106 register INT_Int06Handler(word) INT_DummyHandler
107 register INT_Int07Handler(word) INT_DummyHandler
108 register INT_Int08Handler(word) INT_DummyHandler
109 register INT_Int09Handler(word) INT_DummyHandler
110 register INT_Int0aHandler(word) INT_DummyHandler
111 register INT_Int0bHandler(word) INT_DummyHandler
112 register INT_Int0cHandler(word) INT_DummyHandler
113 register INT_Int0dHandler(word) INT_DummyHandler
114 register INT_Int0eHandler(word) INT_DummyHandler
115 register INT_Int0fHandler(word) INT_DummyHandler
116 register INT_Int10Handler(word) INT_Int10Handler
117 register INT_Int11Handler(word) INT_Int11Handler
118 register INT_Int12Handler(word) INT_Int12Handler
119 register INT_Int13Handler(word) INT_Int13Handler
120 register INT_Int14Handler(word) INT_DummyHandler
121 register INT_Int15Handler(word) INT_Int15Handler
122 register INT_Int16Handler(word) INT_Int16Handler
123 register INT_Int17Handler(word) INT_DummyHandler
124 register INT_Int18Handler(word) INT_DummyHandler
125 register INT_Int19Handler(word) INT_DummyHandler
126 register INT_Int1aHandler(word) INT_Int1aHandler
127 register INT_Int1bHandler(word) INT_DummyHandler
128 register INT_Int1cHandler(word) INT_DummyHandler
129 register INT_Int1dHandler(word) INT_DummyHandler
130 register INT_Int1eHandler(word) INT_DummyHandler
131 register INT_Int1fHandler(word) INT_DummyHandler
132 register INT_Int20Handler(word) INT_DummyHandler
133 register INT_Int21Handler(word) DOS3Call
134 register INT_Int22Handler(word) INT_DummyHandler
135 register INT_Int23Handler(word) INT_DummyHandler
136 register INT_Int24Handler(word) INT_DummyHandler
# Note: int 25 and 26 don't pop the flags from the stack
137 register INT_Int25Handler()     INT_Int25Handler
138 register INT_Int26Handler()     INT_Int26Handler
139 register INT_Int27Handler(word) INT_DummyHandler
140 register INT_Int28Handler(word) INT_DummyHandler
141 register INT_Int29Handler(word) INT_DummyHandler
142 register INT_Int2aHandler(word) INT_Int2aHandler
143 register INT_Int2bHandler(word) INT_DummyHandler
144 register INT_Int2cHandler(word) INT_DummyHandler
145 register INT_Int2dHandler(word) INT_DummyHandler
146 register INT_Int2eHandler(word) INT_DummyHandler
147 register INT_Int2fHandler(word) INT_Int2fHandler
148 register INT_Int30Handler(word) INT_DummyHandler
149 register INT_Int31Handler(word) INT_Int31Handler
150 register INT_Int32Handler(word) INT_DummyHandler
151 register INT_Int33Handler(word) INT_DummyHandler
152 register INT_Int34Handler(word) INT_DummyHandler
153 register INT_Int35Handler(word) INT_DummyHandler
154 register INT_Int36Handler(word) INT_DummyHandler
155 register INT_Int37Handler(word) INT_DummyHandler
156 register INT_Int38Handler(word) INT_DummyHandler
157 register INT_Int39Handler(word) INT_DummyHandler
158 register INT_Int3aHandler(word) INT_DummyHandler
159 register INT_Int3bHandler(word) INT_DummyHandler
160 register INT_Int3cHandler(word) INT_DummyHandler
161 register INT_Int3dHandler(word) INT_DummyHandler
162 register INT_Int3eHandler(word) INT_DummyHandler
163 register INT_Int3fHandler(word) INT_DummyHandler
164 register INT_Int40Handler(word) INT_DummyHandler
165 register INT_Int41Handler(word) INT_DummyHandler
166 register INT_Int42Handler(word) INT_DummyHandler
167 register INT_Int43Handler(word) INT_DummyHandler
168 register INT_Int44Handler(word) INT_DummyHandler
169 register INT_Int45Handler(word) INT_DummyHandler
170 register INT_Int46Handler(word) INT_DummyHandler
171 register INT_Int47Handler(word) INT_DummyHandler
172 register INT_Int48Handler(word) INT_DummyHandler
173 register INT_Int49Handler(word) INT_DummyHandler
174 register INT_Int4aHandler(word) INT_DummyHandler
175 register INT_Int4bHandler(word) INT_Int4bHandler
176 register INT_Int4cHandler(word) INT_DummyHandler
177 register INT_Int4dHandler(word) INT_DummyHandler
178 register INT_Int4eHandler(word) INT_DummyHandler
179 register INT_Int4fHandler(word) INT_DummyHandler
180 register INT_Int50Handler(word) INT_DummyHandler
181 register INT_Int51Handler(word) INT_DummyHandler
182 register INT_Int52Handler(word) INT_DummyHandler
183 register INT_Int53Handler(word) INT_DummyHandler
184 register INT_Int54Handler(word) INT_DummyHandler
185 register INT_Int55Handler(word) INT_DummyHandler
186 register INT_Int56Handler(word) INT_DummyHandler
187 register INT_Int57Handler(word) INT_DummyHandler
188 register INT_Int58Handler(word) INT_DummyHandler
189 register INT_Int59Handler(word) INT_DummyHandler
190 register INT_Int5aHandler(word) INT_DummyHandler
191 register INT_Int5bHandler(word) INT_DummyHandler
192 register INT_Int5cHandler(word) NetBIOSCall
193 register INT_Int5dHandler(word) INT_DummyHandler
194 register INT_Int5eHandler(word) INT_DummyHandler
195 register INT_Int5fHandler(word) INT_DummyHandler
196 register INT_Int60Handler(word) INT_DummyHandler
197 register INT_Int61Handler(word) INT_DummyHandler
198 register INT_Int62Handler(word) INT_DummyHandler
199 register INT_Int63Handler(word) INT_DummyHandler
200 register INT_Int64Handler(word) INT_DummyHandler
201 register INT_Int65Handler(word) INT_DummyHandler
202 register INT_Int66Handler(word) INT_DummyHandler
203 register INT_Int67Handler(word) INT_DummyHandler
204 register INT_Int68Handler(word) INT_DummyHandler
205 register INT_Int69Handler(word) INT_DummyHandler
206 register INT_Int6aHandler(word) INT_DummyHandler
207 register INT_Int6bHandler(word) INT_DummyHandler
208 register INT_Int6cHandler(word) INT_DummyHandler
209 register INT_Int6dHandler(word) INT_DummyHandler
210 register INT_Int6eHandler(word) INT_DummyHandler
211 register INT_Int6fHandler(word) INT_DummyHandler
212 register INT_Int70Handler(word) INT_DummyHandler
213 register INT_Int71Handler(word) INT_DummyHandler
214 register INT_Int72Handler(word) INT_DummyHandler
215 register INT_Int73Handler(word) INT_DummyHandler
216 register INT_Int74Handler(word) INT_DummyHandler
217 register INT_Int75Handler(word) INT_DummyHandler
218 register INT_Int76Handler(word) INT_DummyHandler
219 register INT_Int77Handler(word) INT_DummyHandler
220 register INT_Int78Handler(word) INT_DummyHandler
221 register INT_Int79Handler(word) INT_DummyHandler
222 register INT_Int7aHandler(word) INT_DummyHandler
223 register INT_Int7bHandler(word) INT_DummyHandler
224 register INT_Int7cHandler(word) INT_DummyHandler
225 register INT_Int7dHandler(word) INT_DummyHandler
226 register INT_Int7eHandler(word) INT_DummyHandler
227 register INT_Int7fHandler(word) INT_DummyHandler
228 register INT_Int80Handler(word) INT_DummyHandler
229 register INT_Int81Handler(word) INT_DummyHandler
230 register INT_Int82Handler(word) INT_DummyHandler
231 register INT_Int83Handler(word) INT_DummyHandler
232 register INT_Int84Handler(word) INT_DummyHandler
233 register INT_Int85Handler(word) INT_DummyHandler
234 register INT_Int86Handler(word) INT_DummyHandler
235 register INT_Int87Handler(word) INT_DummyHandler
236 register INT_Int88Handler(word) INT_DummyHandler
237 register INT_Int89Handler(word) INT_DummyHandler
238 register INT_Int8aHandler(word) INT_DummyHandler
239 register INT_Int8bHandler(word) INT_DummyHandler
240 register INT_Int8cHandler(word) INT_DummyHandler
241 register INT_Int8dHandler(word) INT_DummyHandler
242 register INT_Int8eHandler(word) INT_DummyHandler
243 register INT_Int8fHandler(word) INT_DummyHandler
244 register INT_Int90Handler(word) INT_DummyHandler
245 register INT_Int91Handler(word) INT_DummyHandler
246 register INT_Int92Handler(word) INT_DummyHandler
247 register INT_Int93Handler(word) INT_DummyHandler
248 register INT_Int94Handler(word) INT_DummyHandler
249 register INT_Int95Handler(word) INT_DummyHandler
250 register INT_Int96Handler(word) INT_DummyHandler
251 register INT_Int97Handler(word) INT_DummyHandler
252 register INT_Int98Handler(word) INT_DummyHandler
253 register INT_Int99Handler(word) INT_DummyHandler
254 register INT_Int9aHandler(word) INT_DummyHandler
255 register INT_Int9bHandler(word) INT_DummyHandler
256 register INT_Int9cHandler(word) INT_DummyHandler
257 register INT_Int9dHandler(word) INT_DummyHandler
258 register INT_Int9eHandler(word) INT_DummyHandler
259 register INT_Int9fHandler(word) INT_DummyHandler
260 register INT_Inta0Handler(word) INT_DummyHandler
261 register INT_Inta1Handler(word) INT_DummyHandler
262 register INT_Inta2Handler(word) INT_DummyHandler
263 register INT_Inta3Handler(word) INT_DummyHandler
264 register INT_Inta4Handler(word) INT_DummyHandler
265 register INT_Inta5Handler(word) INT_DummyHandler
266 register INT_Inta6Handler(word) INT_DummyHandler
267 register INT_Inta7Handler(word) INT_DummyHandler
268 register INT_Inta8Handler(word) INT_DummyHandler
269 register INT_Inta9Handler(word) INT_DummyHandler
270 register INT_IntaaHandler(word) INT_DummyHandler
271 register INT_IntabHandler(word) INT_DummyHandler
272 register INT_IntacHandler(word) INT_DummyHandler
273 register INT_IntadHandler(word) INT_DummyHandler
274 register INT_IntaeHandler(word) INT_DummyHandler
275 register INT_IntafHandler(word) INT_DummyHandler
276 register INT_Intb0Handler(word) INT_DummyHandler
277 register INT_Intb1Handler(word) INT_DummyHandler
278 register INT_Intb2Handler(word) INT_DummyHandler
279 register INT_Intb3Handler(word) INT_DummyHandler
280 register INT_Intb4Handler(word) INT_DummyHandler
281 register INT_Intb5Handler(word) INT_DummyHandler
282 register INT_Intb6Handler(word) INT_DummyHandler
283 register INT_Intb7Handler(word) INT_DummyHandler
284 register INT_Intb8Handler(word) INT_DummyHandler
285 register INT_Intb9Handler(word) INT_DummyHandler
286 register INT_IntbaHandler(word) INT_DummyHandler
287 register INT_IntbbHandler(word) INT_DummyHandler
288 register INT_IntbcHandler(word) INT_DummyHandler
289 register INT_IntbdHandler(word) INT_DummyHandler
290 register INT_IntbeHandler(word) INT_DummyHandler
291 register INT_IntbfHandler(word) INT_DummyHandler
292 register INT_Intc0Handler(word) INT_DummyHandler
293 register INT_Intc1Handler(word) INT_DummyHandler
294 register INT_Intc2Handler(word) INT_DummyHandler
295 register INT_Intc3Handler(word) INT_DummyHandler
296 register INT_Intc4Handler(word) INT_DummyHandler
297 register INT_Intc5Handler(word) INT_DummyHandler
298 register INT_Intc6Handler(word) INT_DummyHandler
299 register INT_Intc7Handler(word) INT_DummyHandler
300 register INT_Intc8Handler(word) INT_DummyHandler
301 register INT_Intc9Handler(word) INT_DummyHandler
302 register INT_IntcaHandler(word) INT_DummyHandler
303 register INT_IntcbHandler(word) INT_DummyHandler
304 register INT_IntccHandler(word) INT_DummyHandler
305 register INT_IntcdHandler(word) INT_DummyHandler
306 register INT_IntceHandler(word) INT_DummyHandler
307 register INT_IntcfHandler(word) INT_DummyHandler
308 register INT_Intd0Handler(word) INT_DummyHandler
309 register INT_Intd1Handler(word) INT_DummyHandler
310 register INT_Intd2Handler(word) INT_DummyHandler
311 register INT_Intd3Handler(word) INT_DummyHandler
312 register INT_Intd4Handler(word) INT_DummyHandler
313 register INT_Intd5Handler(word) INT_DummyHandler
314 register INT_Intd6Handler(word) INT_DummyHandler
315 register INT_Intd7Handler(word) INT_DummyHandler
316 register INT_Intd8Handler(word) INT_DummyHandler
317 register INT_Intd9Handler(word) INT_DummyHandler
318 register INT_IntdaHandler(word) INT_DummyHandler
319 register INT_IntdbHandler(word) INT_DummyHandler
320 register INT_IntdcHandler(word) INT_DummyHandler
321 register INT_IntddHandler(word) INT_DummyHandler
322 register INT_IntdeHandler(word) INT_DummyHandler
323 register INT_IntdfHandler(word) INT_DummyHandler
324 register INT_Inte0Handler(word) INT_DummyHandler
325 register INT_Inte1Handler(word) INT_DummyHandler
326 register INT_Inte2Handler(word) INT_DummyHandler
327 register INT_Inte3Handler(word) INT_DummyHandler
328 register INT_Inte4Handler(word) INT_DummyHandler
329 register INT_Inte5Handler(word) INT_DummyHandler
330 register INT_Inte6Handler(word) INT_DummyHandler
331 register INT_Inte7Handler(word) INT_DummyHandler
332 register INT_Inte8Handler(word) INT_DummyHandler
333 register INT_Inte9Handler(word) INT_DummyHandler
334 register INT_InteaHandler(word) INT_DummyHandler
335 register INT_IntebHandler(word) INT_DummyHandler
336 register INT_IntecHandler(word) INT_DummyHandler
337 register INT_IntedHandler(word) INT_DummyHandler
338 register INT_InteeHandler(word) INT_DummyHandler
339 register INT_IntefHandler(word) INT_DummyHandler
340 register INT_Intf0Handler(word) INT_DummyHandler
341 register INT_Intf1Handler(word) INT_DummyHandler
342 register INT_Intf2Handler(word) INT_DummyHandler
343 register INT_Intf3Handler(word) INT_DummyHandler
344 register INT_Intf4Handler(word) INT_DummyHandler
345 register INT_Intf5Handler(word) INT_DummyHandler
346 register INT_Intf6Handler(word) INT_DummyHandler
347 register INT_Intf7Handler(word) INT_DummyHandler
348 register INT_Intf8Handler(word) INT_DummyHandler
349 register INT_Intf9Handler(word) INT_DummyHandler
350 register INT_IntfaHandler(word) INT_DummyHandler
351 register INT_IntfbHandler(word) INT_DummyHandler
352 register INT_IntfcHandler(word) INT_DummyHandler
353 register INT_IntfdHandler(word) INT_DummyHandler
354 register INT_IntfeHandler(word) INT_DummyHandler
355 register INT_IntffHandler(word) INT_DummyHandler

# VxDs. The first Vxd is at 400
#
#400+VXD_ID register <VxD handler>(word) <VxD handler>
#
414 register VXD_Comm(word) VXD_Comm
#415 register VXD_Printer(word) VXD_Printer
423 register VXD_Shell(word) VXD_Shell
433 register VXD_PageFile(word) VXD_PageFile
