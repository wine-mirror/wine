name	user
type	win16
heap	65520
file	user.exe

1   pascal16 MessageBox(word str str word) MessageBox16
2   stub OldExitWindows
3   stub EnableOEMLayer
4   stub DisableOEMLayer
5   pascal16 InitApp(word) InitApp
6   pascal16 PostQuitMessage(word) PostQuitMessage16
7   pascal16 ExitWindows(long word) ExitWindows16
10  pascal16 SetTimer(word word word segptr) SetTimer16
11  pascal16 SetSystemTimer(word word word segptr) SetSystemTimer16
12  pascal16 KillTimer(word word) KillTimer16
13  pascal   GetTickCount() GetTickCount
14  pascal   GetTimerResolution() GetTimerResolution
15  pascal   GetCurrentTime() GetCurrentTime16
16  pascal16 ClipCursor(ptr) ClipCursor16
17  pascal16 GetCursorPos(ptr) GetCursorPos16
18  pascal16 SetCapture(word) SetCapture16
19  pascal16 ReleaseCapture() ReleaseCapture
20  pascal16 SetDoubleClickTime(word) SetDoubleClickTime16
21  pascal16 GetDoubleClickTime() GetDoubleClickTime16
22  pascal16 SetFocus(word) SetFocus16
23  pascal16 GetFocus() GetFocus16
24  pascal16 RemoveProp(word ptr) RemoveProp16
25  pascal16 GetProp(word ptr) GetProp16
26  pascal16 SetProp(word ptr word) SetProp16
27  pascal16 EnumProps(word segptr) THUNK_EnumProps16
28  pascal16 ClientToScreen(word ptr) ClientToScreen16
29  pascal16 ScreenToClient(word ptr) ScreenToClient16
30  pascal16 WindowFromPoint(long) WindowFromPoint16
31  pascal16 IsIconic(word) IsIconic16
32  pascal16 GetWindowRect(word ptr) GetWindowRect16
33  pascal16 GetClientRect(word ptr) GetClientRect16
34  pascal16 EnableWindow(word word) EnableWindow16
35  pascal16 IsWindowEnabled(word) IsWindowEnabled16
36  pascal16 GetWindowText(word segptr word) GetWindowText16
37  pascal16 SetWindowText(word segstr) SetWindowText16
38  pascal16 GetWindowTextLength(word) GetWindowTextLength16
39  pascal16 BeginPaint(word ptr) BeginPaint16
40  pascal16 EndPaint(word ptr) EndPaint16
41  pascal16 CreateWindow(str str long s_word s_word s_word s_word
	                  word word word segptr) CreateWindow16
42  pascal16 ShowWindow(word word) ShowWindow16
43  pascal16 CloseWindow(word) CloseWindow16
44  pascal16 OpenIcon(word) OpenIcon16
45  pascal16 BringWindowToTop(word) BringWindowToTop16
46  pascal16 GetParent(word) GetParent16
47  register IsWindow(word) WIN16_IsWindow16
48  pascal16 IsChild(word word) IsChild16
49  pascal16 IsWindowVisible(word) IsWindowVisible16
50  pascal16 FindWindow(segstr str) FindWindow16
51  stub BEAR51 # IsTwoByteCharPrefix
52  pascal16 AnyPopup() AnyPopup16
53  pascal16 DestroyWindow(word) DestroyWindow16
54  pascal16 EnumWindows(segptr long) THUNK_EnumWindows16
55  pascal16 EnumChildWindows(word segptr long) THUNK_EnumChildWindows16
56  pascal16 MoveWindow(word word word word word word) MoveWindow16
57  pascal16 RegisterClass(ptr) RegisterClass16
58  pascal16 GetClassName(word ptr word) GetClassName16
59  pascal16 SetActiveWindow(word) SetActiveWindow16
60  pascal16 GetActiveWindow() GetActiveWindow16
61  pascal16 ScrollWindow(word s_word s_word ptr ptr) ScrollWindow16
62  pascal16 SetScrollPos(word word s_word word) SetScrollPos16
63  pascal16 GetScrollPos(word word) GetScrollPos16
64  pascal16 SetScrollRange(word word s_word s_word word) SetScrollRange16
65  pascal16 GetScrollRange(word word ptr ptr) GetScrollRange16
66  pascal16 GetDC(word) GetDC16
67  pascal16 GetWindowDC(word) GetWindowDC16
68  pascal16 ReleaseDC(word word) ReleaseDC16
69  pascal16 SetCursor(word) SetCursor16
70  pascal16 SetCursorPos(word word) SetCursorPos16
71  pascal16 ShowCursor(word) ShowCursor16
72  pascal16 SetRect(ptr s_word s_word s_word s_word) SetRect16
73  pascal16 SetRectEmpty(ptr) SetRectEmpty16
74  pascal16 CopyRect(ptr ptr) CopyRect16
75  pascal16 IsRectEmpty(ptr) IsRectEmpty16
76  pascal16 PtInRect(ptr long) PtInRect16
77  pascal16 OffsetRect(ptr s_word s_word) OffsetRect16
78  pascal16 InflateRect(ptr s_word s_word) InflateRect16
79  pascal16 IntersectRect(ptr ptr ptr) IntersectRect16
80  pascal16 UnionRect(ptr ptr ptr) UnionRect16
81  pascal16 FillRect(word ptr word) FillRect16
82  pascal16 InvertRect(word ptr) InvertRect16
83  pascal16 FrameRect(word ptr word) FrameRect16
84  pascal16 DrawIcon(word s_word s_word word) DrawIcon16
85  pascal16 DrawText(word str s_word ptr word) DrawText16
86  stub BEAR86 # IconSize
87  pascal16 DialogBox(word segstr word segptr) DialogBox16
88  pascal16 EndDialog(word s_word) EndDialog16
89  pascal16 CreateDialog(word segstr word segptr) CreateDialog16
90  pascal16 IsDialogMessage(word segptr) WIN16_IsDialogMessage16
91  pascal16 GetDlgItem(word word) GetDlgItem16
92  pascal16 SetDlgItemText(word word segstr) SetDlgItemText16
93  pascal16 GetDlgItemText(word word segptr word) GetDlgItemText16
94  pascal16 SetDlgItemInt(word word word word) SetDlgItemInt16
95  pascal16 GetDlgItemInt(word s_word ptr word) GetDlgItemInt16
96  pascal16 CheckRadioButton(word word word word) CheckRadioButton16
97  pascal16 CheckDlgButton(word word word) CheckDlgButton16
98  pascal16 IsDlgButtonChecked(word word) IsDlgButtonChecked16
99  pascal16 DlgDirSelect(word ptr word) DlgDirSelect
100 pascal16 DlgDirList(word ptr word word word) DlgDirList16
101 pascal   SendDlgItemMessage(word word word word long) SendDlgItemMessage16
102 pascal16 AdjustWindowRect(ptr long word) AdjustWindowRect16
103 pascal16 MapDialogRect(word ptr) MapDialogRect16
104 pascal16 MessageBeep(word) MessageBeep16
105 pascal16 FlashWindow(word word) FlashWindow16
106 pascal16 GetKeyState(word) GetKeyState16
107 pascal   DefWindowProc(word word word long) DefWindowProc16
108 pascal16 GetMessage(segptr word word word) GetMessage16
109 pascal16 PeekMessage(ptr word word word word) PeekMessage16
110 pascal16 PostMessage(word word word long) PostMessage16
111 pascal   SendMessage(word word word long) SendMessage16
112 pascal16 WaitMessage() WaitMessage
113 pascal16 TranslateMessage(ptr) TranslateMessage16
114 pascal   DispatchMessage(ptr) DispatchMessage16
115 pascal16 ReplyMessage(long) ReplyMessage16
116 pascal16 PostAppMessage(word word word long) PostAppMessage16
118 pascal16 RegisterWindowMessage(segstr) RegisterWindowMessage16
117 pascal16 WindowFromDC(word) WindowFromDC16
119 pascal   GetMessagePos() GetMessagePos
120 pascal   GetMessageTime() GetMessageTime
121 pascal   SetWindowsHook(s_word segptr) THUNK_SetWindowsHook16
122 pascal   CallWindowProc(segptr word word word long) CallWindowProc16
123 pascal16 CallMsgFilter(segptr s_word) CallMsgFilter16
124 pascal16 UpdateWindow(word) UpdateWindow16
125 pascal16 InvalidateRect(word ptr word) InvalidateRect16
126 pascal16 InvalidateRgn(word word word) InvalidateRgn16
127 pascal16 ValidateRect(word ptr) ValidateRect16
128 pascal16 ValidateRgn(word word) ValidateRgn16
129 pascal16 GetClassWord(word s_word) GetClassWord16
130 pascal16 SetClassWord(word s_word word) SetClassWord16
131 pascal   GetClassLong(word s_word) GetClassLong16
132 pascal   SetClassLong(word s_word long) SetClassLong16
133 pascal16 GetWindowWord(word s_word) GetWindowWord16
134 pascal16 SetWindowWord(word s_word word) SetWindowWord16
135 pascal   GetWindowLong(word s_word) GetWindowLong16
136 pascal   SetWindowLong(word s_word long) SetWindowLong16
137 pascal16 OpenClipboard(word) OpenClipboard16
138 pascal16 CloseClipboard() CloseClipboard16
139 pascal16 EmptyClipboard() EmptyClipboard16
140 pascal16 GetClipboardOwner() GetClipboardOwner16
141 pascal16 SetClipboardData(word word) SetClipboardData16
142 pascal16 GetClipboardData(word) GetClipboardData16
143 pascal16 CountClipboardFormats() CountClipboardFormats16
144 pascal16 EnumClipboardFormats(word) EnumClipboardFormats16
145 pascal16 RegisterClipboardFormat(ptr) RegisterClipboardFormat16
146 pascal16 GetClipboardFormatName(word ptr s_word) GetClipboardFormatName16
147 pascal16 SetClipboardViewer(word) SetClipboardViewer16
148 pascal16 GetClipboardViewer() GetClipboardViewer16
149 pascal16 ChangeClipboardChain(word word) ChangeClipboardChain16
150 pascal16 LoadMenu(word segstr) LoadMenu16
151 pascal16 CreateMenu() CreateMenu16
152 pascal16 DestroyMenu(word) DestroyMenu16
153 pascal16 ChangeMenu(word word segstr word word) ChangeMenu16
154 pascal16 CheckMenuItem(word word word) CheckMenuItem16
155 pascal16 EnableMenuItem(word word word) EnableMenuItem16
156 pascal16 GetSystemMenu(word word) GetSystemMenu16
157 pascal16 GetMenu(word) GetMenu16
158 pascal16 SetMenu(word word) SetMenu16
159 pascal16 GetSubMenu(word word) GetSubMenu16
160 pascal16 DrawMenuBar(word) DrawMenuBar16
161 pascal16 GetMenuString(word word ptr s_word word) GetMenuString16
162 pascal16 HiliteMenuItem(word word word word) HiliteMenuItem16
163 pascal16 CreateCaret(word word word word) CreateCaret16
164 pascal16 DestroyCaret() DestroyCaret16
165 pascal16 SetCaretPos(word word) SetCaretPos16
166 pascal16 HideCaret(word) HideCaret16
167 pascal16 ShowCaret(word) ShowCaret16
168 pascal16 SetCaretBlinkTime(word) SetCaretBlinkTime16
169 pascal16 GetCaretBlinkTime() GetCaretBlinkTime16
170 pascal16 ArrangeIconicWindows(word) ArrangeIconicWindows16
171 pascal16 WinHelp(word str word long) WinHelp16
172 pascal16 SwitchToThisWindow(word word) SwitchToThisWindow16
173 pascal16 LoadCursor(word segstr) LoadCursor16
174 pascal16 LoadIcon(word segstr) LoadIcon16
175 pascal16 LoadBitmap(word segstr) LoadBitmap16
176 pascal16 LoadString(word word ptr s_word) LoadString16
177 pascal16 LoadAccelerators(word segstr) LoadAccelerators16
178 pascal16 TranslateAccelerator(word word ptr) TranslateAccelerator16
179 pascal16 GetSystemMetrics(s_word) GetSystemMetrics16
180 pascal   GetSysColor(word) GetSysColor16
181 pascal16 SetSysColors(word ptr ptr) SetSysColors16
182 pascal16 KillSystemTimer(word word) KillSystemTimer16
183 pascal16 GetCaretPos(ptr) GetCaretPos16
184 stub QuerySendMessage
185 pascal16 GrayString(word word segptr segptr s_word s_word s_word s_word s_word) THUNK_GrayString16
186 pascal16 SwapMouseButton(word) SwapMouseButton16
187 pascal16 EndMenu() EndMenu
188 pascal16 SetSysModalWindow(word) SetSysModalWindow16
189 pascal16 GetSysModalWindow() GetSysModalWindow16
190 pascal16 GetUpdateRect(word ptr word) GetUpdateRect16
191 pascal16 ChildWindowFromPoint(word long) ChildWindowFromPoint16
192 pascal16 InSendMessage() InSendMessage16
193 pascal16 IsClipboardFormatAvailable(word) IsClipboardFormatAvailable16
194 pascal16 DlgDirSelectComboBox(word ptr word) DlgDirSelectComboBox
195 pascal16 DlgDirListComboBox(word ptr word word word) DlgDirListComboBox16
196 pascal   TabbedTextOut(word s_word s_word ptr s_word s_word ptr s_word)
             TabbedTextOut16
197 pascal   GetTabbedTextExtent(word ptr word word ptr) GetTabbedTextExtent16
198 pascal16 CascadeChildWindows(word word) CascadeChildWindows
199 pascal16 TileChildWindows(word word) TileChildWindows
200 pascal16 OpenComm(str word word) OpenComm
201 pascal16 SetCommState(ptr) SetCommState16
202 pascal16 GetCommState(word ptr) GetCommState16
203 pascal16 GetCommError(word ptr) GetCommError
204 pascal16 ReadComm(word ptr word) ReadComm
205 pascal16 WriteComm(word ptr word) WriteComm
206 pascal16 TransmitCommChar(word word) TransmitCommChar16
207 pascal16 CloseComm(word) CloseComm
208 pascal   SetCommEventMask(word word) SetCommEventMask
209 pascal16 GetCommEventMask(word word) GetCommEventMask
210 pascal16 SetCommBreak(word) SetCommBreak16
211 pascal16 ClearCommBreak(word) ClearCommBreak16
212 pascal16 UngetCommChar(word word) UngetCommChar
213 pascal16 BuildCommDCB(ptr ptr) BuildCommDCB16
214 pascal   EscapeCommFunction(word word) EscapeCommFunction16
215 pascal16 FlushComm(word word) FlushComm
216 pascal   UserSeeUserDo(word word word word) UserSeeUserDo
217 pascal16 LookupMenuHandle(word s_word) LookupMenuHandle
218 pascal16 DialogBoxIndirect(word word word segptr) DialogBoxIndirect16
219 pascal16 CreateDialogIndirect(word ptr word segptr) CreateDialogIndirect16
220 pascal16 LoadMenuIndirect(ptr) LoadMenuIndirect16
221 pascal16 ScrollDC(word s_word s_word ptr ptr word ptr) ScrollDC16
222 pascal16 GetKeyboardState(ptr) GetKeyboardState
223 pascal16 SetKeyboardState(ptr) SetKeyboardState
224 pascal16 GetWindowTask(word) GetWindowTask16
225 pascal16 EnumTaskWindows(word segptr long) THUNK_EnumTaskWindows16
226 stub LockInput
227 pascal16 GetNextDlgGroupItem(word word word) GetNextDlgGroupItem16
228 pascal16 GetNextDlgTabItem(word word word) GetNextDlgTabItem16
229 pascal16 GetTopWindow(word) GetTopWindow16
230 pascal16 GetNextWindow(word word) GetNextWindow16
231 return GetSystemDebugState 0 0  #FIXME
232 pascal16 SetWindowPos(word word word word word word word) SetWindowPos16
233 pascal16 SetParent(word word) SetParent16
234 pascal16 UnhookWindowsHook(s_word segptr) THUNK_UnhookWindowsHook16
235 pascal   DefHookProc(s_word word long ptr) DefHookProc16
236 pascal16 GetCapture() GetCapture16
237 pascal16 GetUpdateRgn(word word word) GetUpdateRgn16
238 pascal16 ExcludeUpdateRgn(word word) ExcludeUpdateRgn16
239 pascal16 DialogBoxParam(word segstr word segptr long) DialogBoxParam16
240 pascal16 DialogBoxIndirectParam(word word word segptr long)
             DialogBoxIndirectParam16
241 pascal16 CreateDialogParam(word segstr word segptr long) CreateDialogParam16
242 pascal16 CreateDialogIndirectParam(word ptr word segptr long)
             CreateDialogIndirectParam16
243 pascal   GetDialogBaseUnits() GetDialogBaseUnits
244 pascal16 EqualRect(ptr ptr) EqualRect16
245 pascal16 EnableCommNotification(s_word word s_word s_word) EnableCommNotification
246 pascal16 ExitWindowsExec(str str) ExitWindowsExec16
247 pascal16 GetCursor() GetCursor16
248 pascal16 GetOpenClipboardWindow() GetOpenClipboardWindow16
249 pascal16 GetAsyncKeyState(word) GetAsyncKeyState16
250 pascal16 GetMenuState(word word word) GetMenuState16
251 pascal   SendDriverMessage(word word long long) SendDriverMessage16
252 pascal16 OpenDriver(str str long) OpenDriver16
253 pascal   CloseDriver(word long long) CloseDriver16
254 pascal16 GetDriverModuleHandle(word) GetDriverModuleHandle16
255 pascal   DefDriverProc(long word word long long) DefDriverProc16
256 pascal16 GetDriverInfo(word ptr) GetDriverInfo
257 pascal16 GetNextDriver(word long) GetNextDriver
258 pascal16 MapWindowPoints(word word ptr word) MapWindowPoints16
259 pascal16 BeginDeferWindowPos(s_word) BeginDeferWindowPos16
260 pascal16 DeferWindowPos(word word word s_word s_word s_word s_word word)
             DeferWindowPos16
261 pascal16 EndDeferWindowPos(word) EndDeferWindowPos16
262 pascal16 GetWindow(word word) GetWindow16
263 pascal16 GetMenuItemCount(word) GetMenuItemCount16
264 pascal16 GetMenuItemID(word word) GetMenuItemID16
265 pascal16 ShowOwnedPopups(word word) ShowOwnedPopups16
266 pascal16 SetMessageQueue(word) SetMessageQueue16
267 pascal16 ShowScrollBar(word word word) ShowScrollBar16
268 pascal16 GlobalAddAtom(segstr) GlobalAddAtom16
269 pascal16 GlobalDeleteAtom(word) GlobalDeleteAtom
270 pascal16 GlobalFindAtom(segstr) GlobalFindAtom16
271 pascal16 GlobalGetAtomName(word ptr s_word) GlobalGetAtomName16
272 pascal16 IsZoomed(word) IsZoomed16
273 stub ControlPanelInfo
274 stub GetNextQueueWindow
275 stub RepaintScreen
276 stub LockMyTask
277 pascal16 GetDlgCtrlID(word) GetDlgCtrlID16
278 pascal16 GetDesktopHwnd() GetDesktopHwnd
279 pascal16 OldSetDeskPattern() SetDeskPattern
280 pascal16 SetSystemMenu(word word) SetSystemMenu16
281 pascal16 GetSysColorBrush(word) GetSysColorBrush16
282 pascal16 SelectPalette(word word word) SelectPalette16
283 pascal16 RealizePalette(word) RealizePalette16
284 pascal16 GetFreeSystemResources(word) GetFreeSystemResources
285 pascal16 SetDeskWallPaper(ptr) SetDeskWallPaper16
286 pascal16 GetDesktopWindow() GetDesktopWindow16
287 pascal16 GetLastActivePopup(word) GetLastActivePopup16
288 pascal   GetMessageExtraInfo() GetMessageExtraInfo
289 register keybd_event() WIN16_keybd_event
290 pascal16 RedrawWindow(word ptr word word) RedrawWindow16
291 pascal   SetWindowsHookEx(s_word segptr word word) THUNK_SetWindowsHookEx16
292 pascal16 UnhookWindowsHookEx(segptr) THUNK_UnhookWindowsHookEx16
293 pascal   CallNextHookEx(segptr s_word word long) CallNextHookEx16
294 pascal16 LockWindowUpdate(word) LockWindowUpdate16
299 register mouse_event() WIN16_mouse_event
300 stub UnloadInstalledDrivers
#301 BOZOSLIVEHERE :-))	<- this is actually EditWndProc
#306 BEAR306
308 pascal   DefDlgProc(word word word long) DefDlgProc16
309 pascal16 GetClipCursor(ptr) GetClipCursor16
#310 ContScroll
#312 SendMessage2
#313 PostMessage2
314 pascal16 SignalProc(word word word word word) USER_SignalProc
#315 XCStoDS
#316 CompUpdateRect
#317 CompUpdateRgn
#318 GetWC2
319 pascal16 ScrollWindowEx(word s_word s_word ptr ptr word ptr word) ScrollWindowEx16
320 stub SysErrorBox
321 pascal   SetEventHook(segptr) SetEventHook
322 stub WinOldAppHackOMatic
323 stub GetMessage2
324 pascal16 FillWindow(word word word word) FillWindow
325 pascal16 PaintRect(word word word word ptr) PaintRect
326 pascal16 GetControlBrush(word word word) GetControlBrush
#327 KillTimer2
#328 SetTimer2
#330 SetGetKbdState
331 pascal16 EnableHardwareInput(word) EnableHardwareInput
332 pascal16 UserYield() UserYield
333 pascal16 IsUserIdle() IsUserIdle
334 pascal   GetQueueStatus(word) GetQueueStatus16
335 pascal16 GetInputState() GetInputState16
336 pascal16 LoadCursorIconHandler(word word word) LoadCursorIconHandler
337 pascal   GetMouseEventProc() GetMouseEventProc
#340 WinFarFrame
#341 _FFFE_FARFRAME
343 stub GetFilePortName
#354 TabTheTextOutForWimps
#355 BroadcastMessage
356 pascal16 LoadDIBCursorHandler(word word word) LoadDIBCursorHandler
357 pascal16 LoadDIBIconHandler(word word word) LoadDIBIconHandler
358 pascal16 IsMenu(word) IsMenu16
359 pascal16 GetDCEx(word word long) GetDCEx16
362 pascal16 DCHook(word word long long) DCHook
364 pascal16 LookupIconIdFromDirectoryEx(ptr word word word word) LookupIconIdFromDirectoryEx16
368 pascal16 CopyIcon(word word) CopyIcon16
369 pascal16 CopyCursor(word word) CopyCursor16
370 pascal16 GetWindowPlacement(word ptr) GetWindowPlacement16
371 pascal16 SetWindowPlacement(word ptr) SetWindowPlacement16
372 stub GetInternalIconHeader
373 pascal16 SubtractRect(ptr ptr ptr) SubtractRect16
375 stub DrawTextEx
376 stub SetMessageExtraInfo
378 stub SetPropEx
379 stub GetPropEx
380 stub RemovePropEx
381 stub UsrMPR_ThunkData16
382 stub SetWindowContextHelpID
383 stub GetWindowContextHelpID
384 pascal16 SetMenuContextHelpId(word word) SetMenuContextHelpId16
385 pascal16 GetMenuContextHelpId(word) GetMenuContextHelpId16
389 pascal LoadImage(word segstr word word word word) LoadImage16
390 stub CopyImage
391 stub SignalProc32
394 pascal16 DrawIconEx(word word word word word word word word word) DrawIconEx16
395 stub GetIconInfo
397 pascal16 RegisterClassEx(ptr) RegisterClassEx16
398 pascal16 GetClassInfoEx(word segstr ptr) GetClassInfoEx16
399 pascal16 ChildWindowFromPointEx(word long word) ChildWindowFromPointEx16
400 stub FinalUserInit
402 pascal16 GetPriorityClipboardFormat(ptr s_word) GetPriorityClipboardFormat16
403 pascal16 UnregisterClass(segstr word) UnregisterClass16
404 pascal16 GetClassInfo(word segstr ptr) GetClassInfo16
406 pascal16 CreateCursor(word word word word word ptr ptr) CreateCursor16
407 pascal16 CreateIcon(word word word word word ptr ptr) CreateIcon16
408 pascal16 CreateCursorIconIndirect(word ptr ptr ptr)
	     CreateCursorIconIndirect
409 stub InitThreadInput
410 pascal16 InsertMenu(word word word word segptr) InsertMenu16
411 pascal16 AppendMenu(word word word segptr) AppendMenu16
412 pascal16 RemoveMenu(word word word) RemoveMenu16
413 pascal16 DeleteMenu(word word word) DeleteMenu16
414 pascal16 ModifyMenu(word word word word segptr) ModifyMenu16
415 pascal16 CreatePopupMenu() CreatePopupMenu16
416 pascal16 TrackPopupMenu(word word s_word s_word s_word word ptr) TrackPopupMenu16
417 pascal   GetMenuCheckMarkDimensions() GetMenuCheckMarkDimensions
418 pascal16 SetMenuItemBitmaps(word word word word word) SetMenuItemBitmaps16
420 pascal16 wsprintf() WIN16_wsprintf16
421 pascal16 wvsprintf(ptr str ptr) wvsprintf16
422 pascal16 DlgDirSelectEx(word ptr word word) DlgDirSelectEx16
423 pascal16 DlgDirSelectComboBoxEx(word ptr word word) DlgDirSelectComboBoxEx16
427 pascal16 FindWindowEx(word word segstr str) FindWindowEx16
428 stub TileWindows
429 stub CascadeWindows
430 pascal16 lstrcmp(str str) lstrcmp16
431 pascal   AnsiUpper(segstr) AnsiUpper16
432 pascal   AnsiLower(segstr) AnsiLower16
433 pascal16 IsCharAlpha(word) IsCharAlpha16
434 pascal16 IsCharAlphaNumeric(word) IsCharAlphaNumeric16
435 pascal16 IsCharUpper(word) IsCharUpper16
436 pascal16 IsCharLower(word) IsCharLower16
437 pascal16 AnsiUpperBuff(str word) AnsiUpperBuff16
438 pascal16 AnsiLowerBuff(str word) AnsiLowerBuff16
441 pascal16 InsertMenuItem(word word word ptr) InsertMenuItem16
443 stub GetMenuItemInfo
445 pascal   DefFrameProc(word word word word long) DefFrameProc16
446 stub SetMenuItemInfo
447 pascal   DefMDIChildProc(word word word long) DefMDIChildProc16
448 stub DrawAnimatedRects
449 stub DrawState
450 pascal16 CreateIconFromResourceEx(ptr long word long word word word) CreateIconFromResourceEx16
451 pascal16 TranslateMDISysAccel(word ptr) TranslateMDISysAccel16
452 pascal16 CreateWindowEx(long str str long s_word s_word s_word s_word
                            word word word segptr) CreateWindowEx16
454 pascal16 AdjustWindowRectEx(ptr long word long) AdjustWindowRectEx16
455 pascal16 GetIconID(word long) GetIconID
456 pascal16 LoadIconHandler(word word) LoadIconHandler
457 pascal16 DestroyIcon(word) DestroyIcon16
458 pascal16 DestroyCursor(word) DestroyCursor16
459 pascal   DumpIcon(segptr ptr ptr ptr) DumpIcon
460 pascal16 GetInternalWindowPos(word ptr ptr) GetInternalWindowPos16
461 pascal16 SetInternalWindowPos(word word ptr ptr) SetInternalWindowPos16
462 pascal16 CalcChildScroll(word word) CalcChildScroll
463 pascal16 ScrollChildren(word word word long) ScrollChildren16
464 pascal   DragObject(word word word word word word) DragObject16
465 pascal16 DragDetect(word long) DragDetect16
466 pascal16 DrawFocusRect(word ptr) DrawFocusRect16
470 stub StringFunc
471 pascal16 lstrcmpi(str str) lstrcmpi16
472 pascal   AnsiNext(segptr) AnsiNext16
473 pascal   AnsiPrev(segptr segptr) AnsiPrev16
475 pascal16 SetScrollInfo(word s_word ptr word) SetScrollInfo16
476 pascal16 GetScrollInfo(word s_word ptr) GetScrollInfo16
477 pascal16 GetKeyboardLayoutName(ptr) GetKeyboardLayoutName16
478 stub LoadKeyboardLayout
479 stub MenuItemFromPoint
480 stub GetUserLocalObjType
#481 HARDWARE_EVENT
482 pascal16 EnableScrollBar(word word word) EnableScrollBar16
483 pascal16 SystemParametersInfo(word word ptr word) SystemParametersInfo16
#484 __GP
# Stubs for Hebrew version
489 pascal16 USER_489() stub_USER_489
490 pascal16 USER_490() stub_USER_490
492 pascal16 USER_492() stub_USER_492
496 pascal16 USER_496() stub_USER_496
499 pascal16 WNetErrorText(word ptr word) WNetErrorText
500 stub FARCALLNETDRIVER 			# Undocumented Windows
501 pascal16 WNetOpenJob(ptr ptr word ptr)  WNetOpenJob
502 pascal16 WNetCloseJob(word ptr ptr) WNetCloseJob
503 pascal16 WNetAbortJob(ptr word) WNetAbortJob
504 pascal16 WNetHoldJob(ptr word) WNetHoldJob
505 pascal16 WNetReleaseJob(ptr word) WNetReleaseJob
506 pascal16 WNetCancelJob(ptr word) WNetCancelJob
507 pascal16 WNetSetJobCopies(ptr word word) WNetSetJobCopies
508 pascal16 WNetWatchQueue(word ptr ptr word) WNetWatchQueue
509 pascal16 WNetUnwatchQueue(word ptr ptr word) WNetUnwatchQueue
510 pascal16 WNetLockQueueData(ptr ptr ptr) WNetLockQueueData
511 pascal16 WNetUnlockQueueData(ptr) WNetUnlockQueueData
512 pascal16 WNetGetConnection(ptr ptr ptr) WNetGetConnection16
513 pascal16 WNetGetCaps(word) WNetGetCaps
514 pascal16 WNetDeviceMode(word) WNetDeviceMode
515 pascal16 WNetBrowseDialog(word word ptr) WNetBrowseDialog
516 pascal16 WNetGetUser(ptr ptr ptr) WNetGetUser
517 pascal16 WNetAddConnection(str str str) WNetAddConnection16
518 pascal16 WNetCancelConnection(str word) WNetCancelConnection
519 pascal16 WNetGetError(ptr) WNetGetError
520 pascal16 WNetGetErrorText(word ptr ptr) WNetGetErrorText
521 stub WNetEnable
522 stub WNetDisable
523 pascal16 WNetRestoreConnection(word ptr) WNetRestoreConnection
524 pascal16 WNetWriteJob(word ptr ptr) WNetWriteJob
525 pascal16 WnetConnectDialog(word word) WNetConnectDialog
526 pascal16 WNetDisconnectDialog(word word) WNetDisconnectDialog
527 pascal16 WNetConnectionDialog(word word) WNetConnectionDialog
528 pascal16 WNetViewQueueDialog(word ptr) WNetViewQueueDialog
529 pascal16 WNetPropertyDialog(word word ptr word) WNetPropertyDialog
530 pascal16 WNetGetDirectoryType(ptr ptr) WNetGetDirectoryType16
531 pascal16 WNetDirectoryNotify(word ptr word) WNetDirectoryNotify
532 pascal16 WNetGetPropertyText(word word str str word word) WNetGetPropertyText
533 stub WNetInitialize
534 stub WNetLogon
600 pascal16 GetShellWindow() GetShellWindow16
601 stub DoHotkeyStuff
602 stub SetCheckCursorTimer
604 stub BroadcastSystemMessage
605 stub HackTaskMonitor
606 stub FormatMessage
608 pascal16 GetForegroundWindow() GetForegroundWindow16
609 pascal16 SetForegroundWindow(word) SetForegroundWindow16
610 stub DestroyIcon32
620 stub ChangeDisplaySettings
621 stub EnumDisplaySettings
640 stub MsgWaitForMultipleObjects
650 stub ActivateKeyboardLayout
651 stub GetKeyboardLayout
652 stub GetKeyboardLayoutList
654 stub UnloadKeyboardLayout
655 stub PostPostedMessages
656 pascal16 DrawFrameControl(word ptr word word) DrawFrameControl16
657 pascal16 DrawCaptionTemp(word word ptr word word ptr word) DrawCaptionTemp16
658 stub DispatchInput
659 pascal16 DrawEdge(word ptr word word) DrawEdge16
660 pascal16 DrawCaption(word word ptr word) DrawCaption16
661 stub SetSysColorsTemp
662 stub DrawMenubarTemp
663 stub GetMenuDefaultItem
664 stub SetMenuDefaultItem
665 pascal16 GetMenuItemRect(word word word ptr) GetMenuItemRect16
666 stub CheckMenuRadioItem
667 stub TrackPopupMenuEx
668 pascal16 SetWindowRgn(word word word) SetWindowRgn16
669 stub GetWindowRgn
800 stub CHOOSEFONT_CALLBACK16
801 stub FINDREPLACE_CALLBACK16
802 stub OPENFILENAME_CALLBACK16
803 stub PRINTDLG_CALLBACK16
804 stub CHOOSECOLOR_CALLBACK16
819 stub PeekMessage32
820 stub GetMessage32
821 stub TranslateMessage32	
#821 stub IsDialogMessage32		# FIXME: two ordinal 821???
822 stub DispatchMessage32
823 stub CallMsgFilter32
825 stub PostMessage32
826 stub PostThreadMessage32
827 pascal16 MessageBoxIndirect(ptr) MessageBoxIndirect16
851 stub MsgThkConnectionDataLS
853 stub FT_USRFTHKTHKCONNECTIONDATA
854 stub FT__USRF2THKTHKCONNECTIONDATA
855 stub Usr32ThkConnectionDataSL
890 stub InstallIMT
891 stub UninstallIMT
# Stubs for Hebrew version
902 pascal16 USER_902() stub_USER_902
905 pascal16 USER_905() stub_USER_905
906 pascal16 USER_906() stub_USER_906
907 pascal16 USER_907() stub_USER_907
909 pascal16 USER_909() stub_USER_909
910 pascal16 USER_910() stub_USER_910
911 pascal16 USER_911() stub_USER_911
912 pascal16 USER_912() stub_USER_912
913 pascal16 USER_913() stub_USER_913
914 pascal16 USER_914() stub_USER_914
915 pascal16 USER_915() stub_USER_915
916 pascal16 USER_916() stub_USER_916
918 pascal16 USER_918() stub_USER_918
919 pascal16 USER_919() stub_USER_919
920 pascal16 USER_920() stub_USER_920
922 pascal16 USER_922() stub_USER_922
923 pascal16 USER_923() stub_USER_923
924 stub UsrMpr_ThunkData16
