name	user
id	2

1   pascal MessageBox(word ptr ptr word) MessageBox
2   stub OldExitWindows
3   stub EnableOEMLayer
4   stub DisableOEMLayer
5   pascal InitApp(word) USER_InitApp
6   pascal PostQuitMessage(word) PostQuitMessage
7   pascal ExitWindows(long word) ExitWindows
10  pascal SetTimer(word word word segptr) SetTimer
11  pascal SetSystemTimer(word word word segptr) SetSystemTimer
12  pascal KillTimer(word word) KillTimer
13  pascal GetTickCount() GetTickCount
14  pascal GetTimerResolution() GetTimerResolution
15  pascal GetCurrentTime() GetTickCount
16  pascal ClipCursor(ptr) ClipCursor
17  pascal GetCursorPos(ptr) GetCursorPos
18  pascal SetCapture(word) SetCapture
19  pascal ReleaseCapture() ReleaseCapture
20  pascal SetDoubleClickTime(word) SetDoubleClickTime
21  pascal GetDoubleClickTime() GetDoubleClickTime
22  pascal SetFocus(word) SetFocus
23  pascal GetFocus() GetFocus
24  pascal16 RemoveProp(word segptr) RemoveProp
25  pascal16 GetProp(word segptr) GetProp
26  pascal16 SetProp(word segptr word) SetProp
27  pascal16 EnumProps(word segptr) EnumProps
28  pascal ClientToScreen(word ptr) ClientToScreen
29  pascal ScreenToClient(word ptr) ScreenToClient
30  pascal WindowFromPoint(long) WindowFromPoint
31  pascal IsIconic(word) IsIconic
32  pascal GetWindowRect(word ptr) GetWindowRect
33  pascal GetClientRect(word ptr) GetClientRect
34  pascal EnableWindow(word word) EnableWindow
35  pascal IsWindowEnabled(word) IsWindowEnabled
36  pascal GetWindowText(word segptr word) WIN16_GetWindowText
37  pascal SetWindowText(word segptr) WIN16_SetWindowText
38  pascal GetWindowTextLength(word) GetWindowTextLength
39  pascal BeginPaint(word ptr) BeginPaint
40  pascal EndPaint(word ptr) EndPaint
41  pascal CreateWindow(ptr ptr long s_word s_word s_word s_word
	                word word word segptr) CreateWindow
42  pascal ShowWindow(word word) ShowWindow
43  pascal CloseWindow(word) CloseWindow
44  pascal OpenIcon(word) OpenIcon
45  pascal BringWindowToTop(word) BringWindowToTop
46  pascal GetParent(word) GetParent
47  pascal IsWindow(word) IsWindow
48  pascal IsChild(word word) IsChild
49  pascal IsWindowVisible(word) IsWindowVisible
50  pascal FindWindow(ptr ptr) FindWindow
#51 BEAR51
52  pascal AnyPopup() AnyPopup
53  pascal DestroyWindow(word) DestroyWindow
54  pascal EnumWindows(segptr long) EnumWindows
55  pascal EnumChildWindows(word segptr long) EnumChildWindows
56  pascal MoveWindow(word word word word word word) MoveWindow
57  pascal RegisterClass(ptr) RegisterClass
58  pascal GetClassName(word ptr word) GetClassName
59  pascal SetActiveWindow(word) SetActiveWindow
60  pascal GetActiveWindow() GetActiveWindow
61  pascal ScrollWindow(word s_word s_word ptr ptr) ScrollWindow
62  pascal SetScrollPos(word word s_word word) SetScrollPos
63  pascal GetScrollPos(word word) GetScrollPos
64  pascal SetScrollRange(word word s_word s_word word) SetScrollRange
65  pascal GetScrollRange(word word ptr ptr) GetScrollRange
66  pascal GetDC(word) GetDC
67  pascal GetWindowDC(word) GetWindowDC
68  pascal ReleaseDC(word word) ReleaseDC
69  pascal SetCursor(word) SetCursor
70  pascal SetCursorPos(word word) SetCursorPos
71  pascal ShowCursor(word) ShowCursor
72  pascal SetRect(ptr s_word s_word s_word s_word) SetRect
73  pascal SetRectEmpty(ptr) SetRectEmpty
74  pascal CopyRect(ptr ptr) CopyRect
75  pascal IsRectEmpty(ptr) IsRectEmpty
76  pascal PtInRect(ptr long) PtInRect
77  pascal OffsetRect(ptr s_word s_word) OffsetRect
78  pascal InflateRect(ptr s_word s_word) InflateRect
79  pascal IntersectRect(ptr ptr ptr) IntersectRect
80  pascal UnionRect(ptr ptr ptr) UnionRect
81  pascal FillRect(word ptr word) FillRect
82  pascal InvertRect(word ptr) InvertRect
83  pascal FrameRect(word ptr word) FrameRect
84  pascal DrawIcon(word s_word s_word word) DrawIcon
85  pascal DrawText(word ptr s_word ptr word) DrawText
87  pascal16 DialogBox(word segptr word segptr) DialogBox
88  pascal EndDialog(word s_word) EndDialog
89  pascal16 CreateDialog(word segptr word segptr) CreateDialog
90  pascal IsDialogMessage(word ptr) IsDialogMessage
91  pascal GetDlgItem(word word) GetDlgItem
92  pascal SetDlgItemText(word word segptr) SetDlgItemText
93  pascal GetDlgItemText(word word segptr word) GetDlgItemText
94  pascal SetDlgItemInt(word word word word) SetDlgItemInt
95  pascal GetDlgItemInt(word word ptr word) GetDlgItemInt
96  pascal CheckRadioButton(word word word word) CheckRadioButton
97  pascal CheckDlgButton(word word word) CheckDlgButton
98  pascal IsDlgButtonChecked(word word) IsDlgButtonChecked
99  pascal DlgDirSelect(word ptr word) DlgDirSelect
100 pascal DlgDirList(word ptr word word word) DlgDirList
101 pascal SendDlgItemMessage(word word word word long) SendDlgItemMessage
102 pascal AdjustWindowRect(ptr long word) AdjustWindowRect
103 pascal MapDialogRect(word ptr) MapDialogRect
104 pascal MessageBeep(word) MessageBeep
105 pascal FlashWindow(word word) FlashWindow
106 pascal GetKeyState(word) GetKeyState
107 pascal DefWindowProc(word word word long) DefWindowProc
108 pascal GetMessage(segptr word word word) GetMessage
109 pascal PeekMessage(ptr word word word word) PeekMessage
110 pascal PostMessage(word word word long) PostMessage
111 pascal SendMessage(word word word long) SendMessage
112 pascal WaitMessage() WaitMessage
113 pascal TranslateMessage(ptr) TranslateMessage
114 pascal DispatchMessage(ptr) DispatchMessage
115 stub ReplyMessage
116 stub PostAppMessage
118 pascal RegisterWindowMessage(ptr) RegisterWindowMessage
119 pascal GetMessagePos() GetMessagePos
120 pascal GetMessageTime() GetMessageTime
121 pascal SetWindowsHook(s_word segptr) SetWindowsHook
122 pascal CallWindowProc(segptr word word word long) CallWindowProc
123 pascal CallMsgFilter(segptr s_word) CallMsgFilter
124 pascal UpdateWindow(word) UpdateWindow
125 pascal InvalidateRect(word ptr word) InvalidateRect
126 pascal InvalidateRgn(word word word) InvalidateRgn
127 pascal ValidateRect(word ptr) ValidateRect
128 pascal ValidateRgn(word word) ValidateRgn
129 pascal GetClassWord(word s_word) GetClassWord
130 pascal SetClassWord(word s_word word) SetClassWord
131 pascal GetClassLong(word s_word) GetClassLong
132 pascal SetClassLong(word s_word long) SetClassLong
133 pascal GetWindowWord(word s_word) GetWindowWord
134 pascal SetWindowWord(word s_word word) SetWindowWord
135 pascal GetWindowLong(word s_word) GetWindowLong
136 pascal SetWindowLong(word s_word long) SetWindowLong
137 pascal OpenClipboard(word) OpenClipboard
138 pascal CloseClipboard() CloseClipboard
139 pascal EmptyClipboard() EmptyClipboard
140 pascal GetClipboardOwner() GetClipboardOwner
141 pascal SetClipboardData(word word) SetClipboardData
142 pascal GetClipboardData(word) GetClipboardData
143 pascal CountClipboardFormats() CountClipboardFormats
144 pascal EnumClipboardFormats(word) EnumClipboardFormats
145 pascal RegisterClipboardFormat(ptr) RegisterClipboardFormat
146 pascal GetClipboardFormatName(word ptr s_word) GetClipboardFormatName
147 pascal SetClipboardViewer(word) SetClipboardViewer
148 pascal GetClipboardViewer() GetClipboardViewer
149 pascal ChangeClipboardChain(word ptr) ChangeClipboardChain
150 pascal LoadMenu(word segptr) LoadMenu
151 pascal CreateMenu() CreateMenu
152 pascal DestroyMenu(word) DestroyMenu
153 pascal ChangeMenu(word word ptr word word) ChangeMenu
154 pascal CheckMenuItem(word word word) CheckMenuItem
155 pascal EnableMenuItem(word word word) EnableMenuItem
156 pascal GetSystemMenu(word word) GetSystemMenu
157 pascal GetMenu(word) GetMenu
158 pascal SetMenu(word word) SetMenu
159 pascal GetSubMenu(word word) GetSubMenu
160 pascal DrawMenuBar(word) DrawMenuBar
161 pascal GetMenuString(word word ptr s_word word) GetMenuString
162 pascal HiliteMenuItem(word word word word) HiliteMenuItem
163 pascal CreateCaret(word word word word) CreateCaret
164 pascal DestroyCaret() DestroyCaret
165 pascal SetCaretPos(word word) SetCaretPos
166 pascal HideCaret(word) HideCaret
167 pascal ShowCaret(word) ShowCaret
168 pascal SetCaretBlinkTime(word) SetCaretBlinkTime
169 pascal GetCaretBlinkTime() GetCaretBlinkTime
170 pascal ArrangeIconicWindows(word) ArrangeIconicWindows
171 pascal WinHelp(word ptr word long) WinHelp
172 stub SwitchToThisWindow
173 pascal16 LoadCursor(word segptr) LoadCursor
174 pascal16 LoadIcon(word segptr) LoadIcon
175 pascal16 LoadBitmap(word segptr) LoadBitmap
176 pascal16 LoadString(word word ptr s_word) LoadString
177 pascal16 LoadAccelerators(word segptr) LoadAccelerators
178 pascal TranslateAccelerator(word word ptr) TranslateAccelerator
179 pascal GetSystemMetrics(word) GetSystemMetrics
180 pascal GetSysColor(word) GetSysColor
181 pascal SetSysColors(word ptr ptr) SetSysColors
182 pascal KillSystemTimer(word word) KillSystemTimer
183 pascal GetCaretPos(ptr) GetCaretPos
184 stub QuerySendMessage
185 pascal GrayString(word word ptr ptr word word word word word) GrayString
186 pascal SwapMouseButton(word) SwapMouseButton
187 pascal EndMenu() EndMenu
188 pascal SetSysModalWindow(word) SetSysModalWindow
189 pascal GetSysModalWindow() GetSysModalWindow
190 pascal GetUpdateRect(word ptr word) GetUpdateRect
191 pascal ChildWindowFromPoint(word long) ChildWindowFromPoint
192 pascal16 InSendMessage() InSendMessage
193 pascal IsClipboardFormatAvailable(word) IsClipboardFormatAvailable
194 pascal DlgDirSelectComboBox(word ptr word) DlgDirSelectComboBox
195 pascal DlgDirListComboBox(word segptr word word word) DlgDirListComboBox
196 pascal TabbedTextOut(word s_word s_word ptr s_word s_word ptr s_word)
           TabbedTextOut
197 pascal GetTabbedTextExtent(word ptr word word ptr) GetTabbedTextExtent
198 pascal CascadeChildWindows(word word) CascadeChildWindows
199 pascal TileChildWindows(word word) TileChildWindows
200 pascal OpenComm(ptr word word) OpenComm
201 pascal SetCommState(ptr) SetCommState
202 pascal GetCommState(word ptr) GetCommState
203 pascal GetCommError(word ptr) GetCommError
204 pascal ReadComm(word ptr word) ReadComm
205 pascal WriteComm(word ptr word) WriteComm
206 pascal TransmitCommChar(word byte) TransmitCommChar 
207 pascal CloseComm(word) CloseComm
208 pascal SetCommEventMask(word word) SetCommEventMask
209 pascal GetCommEventMask(word word) GetCommEventMask
210 pascal SetCommBreak(word) SetCommBreak
211 pascal ClearCommBreak(word) ClearCommBreak
212 pascal UngetCommChar(word byte) UngetCommChar
213 pascal BuildCommDCB(ptr ptr) BuildCommDCB
214 pascal EscapeCommFunction(word word) EscapeCommFunction
215 pascal FlushComm(word word) FlushComm
#216 USERSEEUSERDO
217 pascal LookupMenuHandle(word s_word) LookupMenuHandle
218 pascal16 DialogBoxIndirect(word word word segptr) DialogBoxIndirect
219 pascal16 CreateDialogIndirect(word ptr word segptr) CreateDialogIndirect
220 pascal LoadMenuIndirect(ptr) LoadMenuIndirect
221 pascal ScrollDC(word s_word s_word ptr ptr word ptr) ScrollDC
222 pascal16 GetKeyboardState(ptr) GetKeyboardState
223 stub SetKeyboardState
224 pascal16 GetWindowTask(word) GetWindowTask
225 pascal EnumTaskWindows(word segptr long) EnumTaskWindows
226 stub LockInput
227 pascal GetNextDlgGroupItem(word word word) GetNextDlgGroupItem
228 pascal GetNextDlgTabItem(word word word) GetNextDlgTabItem
229 pascal GetTopWindow(word) GetTopWindow
230 pascal GetNextWindow(word word) GetNextWindow
231 stub GetSystemDebugState
232 pascal SetWindowPos(word word word word word word word) SetWindowPos
233 pascal SetParent(word word) SetParent
234 pascal UnhookWindowsHook(s_word segptr) UnhookWindowsHook
235 pascal DefHookProc(s_word word long ptr) DefHookProc
236 pascal GetCapture() GetCapture
237 pascal GetUpdateRgn(word word word) GetUpdateRgn
238 pascal ExcludeUpdateRgn(word word) ExcludeUpdateRgn
239 pascal16 DialogBoxParam(word segptr word segptr long) DialogBoxParam
240 pascal16 DialogBoxIndirectParam(word word word segptr long)
             DialogBoxIndirectParam
241 pascal16 CreateDialogParam(word segptr word segptr long) CreateDialogParam
242 pascal16 CreateDialogIndirectParam(word ptr word segptr long)
             CreateDialogIndirectParam
243 pascal GetDialogBaseUnits() GetDialogBaseUnits
244 pascal EqualRect(ptr ptr) EqualRect
245 stub EnableCommNotification
246 stub ExitWindowsExec
247 pascal GetCursor() GetCursor
248 pascal GetOpenClipboardWindow() GetOpenClipboardWindow
249 pascal GetAsyncKeyState(word) GetAsyncKeyState
250 pascal GetMenuState(word word word) GetMenuState
251 pascal SendDriverMessage(word word long long) SendDriverMessage
252 pascal OpenDriver(ptr ptr long) OpenDriver
253 pascal CloseDriver(word word long) CloseDriver
254 pascal GetDriverModuleHandle(word) GetDriverModuleHandle
255 pascal DefDriverProc(long word word long long) DefDriverProc
256 pascal GetDriverInfo(word ptr) GetDriverInfo
257 pascal GetNextDriver(word long) GetNextDriver
258 pascal MapWindowPoints(word word ptr word) MapWindowPoints
259 pascal16 BeginDeferWindowPos(s_word) BeginDeferWindowPos
260 pascal16 DeferWindowPos(word word word s_word s_word s_word s_word word)
             DeferWindowPos
261 pascal16 EndDeferWindowPos(word) EndDeferWindowPos
262 pascal GetWindow(word word) GetWindow
263 pascal GetMenuItemCount(word) GetMenuItemCount
264 pascal GetMenuItemID(word word) GetMenuItemID
265 stub ShowOwnedPopups
266 pascal SetMessageQueue(word) SetMessageQueue
267 pascal ShowScrollBar(word word word) ShowScrollBar
268 pascal GlobalAddAtom(ptr) GlobalAddAtom
269 pascal GlobalDeleteAtom(word) GlobalDeleteAtom
270 pascal GlobalFindAtom(ptr) GlobalFindAtom
271 pascal GlobalGetAtomName(word ptr s_word) GlobalGetAtomName
272 pascal IsZoomed(word) IsZoomed
273 stub ControlPanelInfo
274 stub GetNextQueueWindow
275 stub RepaintScreen
276 stub LockMyTask
277 pascal GetDlgCtrlID(word) GetDlgCtrlID
278 pascal GetDeskTopHwnd() GetDesktopWindow
279 stub OldSetDeskPattern
280 stub SetSystemMenu
281 pascal GetSysColorBrush(word) GetSysColorBrush
282 pascal SelectPalette(word word word) SelectPalette
283 pascal RealizePalette(word) RealizePalette
284 pascal16 GetFreeSystemResources(word) GetFreeSystemResources
#285 BEAR285
286 pascal GetDesktopWindow() GetDesktopWindow
287 pascal GetLastActivePopup(word) GetLastActivePopup
288 pascal GetMessageExtraInfo() GetMessageExtraInfo
#289 KEYB_EVENT
290 pascal RedrawWindow(word ptr word word) RedrawWindow
291 pascal SetWindowsHookEx(s_word segptr word word) SetWindowsHookEx
292 pascal UnhookWindowsHookEx(segptr) UnhookWindowsHookEx
293 pascal CallNextHookEx(segptr s_word word long) CallNextHookEx
294 stub LockWindowUpdate
#299 MOUSE_EVENT
#301 BOZOSLIVEHERE :-))
#306 BEAR306
308 pascal DefDlgProc(word word word long) DefDlgProc
314 stub SignalProc
319 pascal ScrollWindowEx(word s_word s_word ptr ptr word ptr word)
           ScrollWindowEx
320 stub SysErrorBox
321 stub SetEventHook
322 stub WinOldAppHackOMatic
323 stub GetMessage2
324 pascal FillWindow(word word word word) FillWindow
325 pascal PaintRect(word word word word ptr) PaintRect
326 pascal16 GetControlBrush(word word word) GetControlBrush
331 pascal EnableHardwareInput(word) EnableHardwareInput
332 return UserYield 0 0
333 stub IsUserIdle
334 pascal GetQueueStatus(word) GetQueueStatus
335 pascal GetInputState() GetInputState
336 stub LoadCursorIconHandler
337 stub GetMouseEventProc
#341 _FFFE_FARFRAME
343 stub GetFilePortName
356 stub LoadDIBCursorHandler
357 stub LoadDIBIconHandler
358 pascal IsMenu(word) IsMenu
359 pascal GetDCEx(word word long) GetDCEx
362 stub DCHook
368 stub CopyIcon
369 stub CopyCursor
370 pascal GetWindowPlacement(word ptr) GetWindowPlacement
371 pascal SetWindowPlacement(word ptr) SetWindowPlacement
372 stub GetInternalIconHeader
373 pascal SubtractRect(ptr ptr ptr) SubtractRect
400 stub FinalUserInit
402 pascal GetPriorityClipboardFormat(word ptr s_word)
           GetPriorityClipboardFormat
403 pascal UnregisterClass(ptr word) UnregisterClass
404 pascal GetClassInfo(word segptr ptr) GetClassInfo
406 pascal CreateCursor(word word word word word ptr ptr) CreateCursor
407 pascal CreateIcon(word word word byte byte ptr ptr) CreateIcon
408 stub CreateCursorIconIndirect
410 pascal InsertMenu(word word word word ptr) InsertMenu
411 pascal AppendMenu(word word word ptr) AppendMenu
412 pascal RemoveMenu(word word word) RemoveMenu
413 pascal DeleteMenu(word word word) DeleteMenu
414 pascal ModifyMenu(word word word word ptr) ModifyMenu
415 pascal CreatePopupMenu() CreatePopupMenu
416 pascal TrackPopupMenu(word word word word word word ptr) TrackPopupMenu
417 pascal GetMenuCheckMarkDimensions() GetMenuCheckMarkDimensions
418 pascal SetMenuItemBitmaps(word word word word word) SetMenuItemBitmaps
420 pascal wsprintf() windows_wsprintf
# windows_wsprintf() handles arguments itself, as libc can't handle an
# 16-bit stack. DLLRelay() will pass 16-bit stack pointer as 1st arg.
421 pascal wvsprintf(ptr ptr ptr) wvsprintf
422 stub DlgDirSelectEx
423 stub DlgDirSelectComboBoxEx
430 pascal16 lstrcmp(ptr ptr) lstrcmp
431 pascal AnsiUpper(segptr) WIN16_AnsiUpper
432 pascal AnsiLower(segptr) WIN16_AnsiLower
433 pascal16 IsCharAlpha(byte) IsCharAlpha
434 pascal16 IsCharAlphanumeric(byte) IsCharAlphanumeric
435 pascal16 IsCharUpper(byte) IsCharUpper
436 pascal16 IsCharLower(byte) IsCharLower
437 pascal16 AnsiUpperBuff(ptr word) AnsiUpperBuff
438 pascal16 AnsiLowerBuff(ptr word) AnsiLowerBuff
445 pascal DefFrameProc(word word word word long) DefFrameProc
447 pascal DefMDIChildProc(word word word long) DefMDIChildProc
451 pascal TranslateMDISysAccel(word ptr) TranslateMDISysAccel
452 pascal CreateWindowEx(long ptr ptr long s_word s_word s_word s_word
                          word word word segptr) CreateWindowEx
454 pascal AdjustWindowRectEx(ptr long word long) AdjustWindowRectEx
455 stub GetIconId
456 stub LoadIconHandler
457 pascal DestroyIcon(word) DestroyIcon
458 pascal DestroyCursor(word) DestroyCursor
459 stub DumpIcon
460 pascal GetInternalWindowPos(word ptr ptr) GetInternalWindowPos
461 pascal SetInternalWindowPos(word word ptr ptr) SetInternalWindowPos
462 stub CalcChildScroll
463 stub ScrollChildren
464 stub DragObject
465 stub DragDetect
466 pascal DrawFocusRect(word ptr) DrawFocusRect
470 stub StringFunc
471 pascal16 lstrcmpi(ptr ptr) lstrcmpi
472 pascal AnsiNext(segptr) AnsiNext
473 pascal AnsiPrev(segptr segptr) AnsiPrev
480 stub GetUserLocalObjType
#481 HARDWARE_EVENT
482 pascal16 EnableScrollBar(word word word) EnableScrollBar
483 pascal SystemParametersInfo(word word ptr word) SystemParametersInfo
#484 __GP
499 pascal WNetErrorText(word ptr word) WNetErrorText
501 pascal WNetOpenJob(ptr ptr word ptr)  WNetOpenJob
502 pascal WNetCloseJob(word ptr ptr) WNetCloseJob
503 pascal WNetAbortJob(ptr word) WNetAbortJob
504 pascal WNetHoldJob(ptr word) WNetHoldJob
505 pascal WNetReleaseJob(ptr word) WNetReleaseJob
506 pascal WNetCancelJob(ptr word) WNetCancelJob
507 pascal WNetSetJobCopies(ptr word word) WNetSetJobCopies
508 pascal WNetWatchQueue(word ptr ptr word) WNetWatchQueue
509 pascal WNetUnwatchQueue(word ptr ptr word) WNetUnwatchQueue
510 pascal WNetLockQueueData(ptr ptr ptr) WNetLockQueueData
511 pascal WNetUnlockQueueData(ptr) WNetUnlockQueueData
512 pascal16 WNetGetConnection(ptr ptr ptr) WNetGetConnection
513 pascal WNetGetCaps(word) WNetGetCaps
514 pascal WNetDeviceMode(word) WNetDeviceMode
515 pascal WNetBrowseDialog(word word ptr) WNetBrowseDialog
516 pascal WNetGetUser(ptr ptr ptr) WNetGetUser
517 pascal16 WNetAddConnection(ptr ptr ptr) WNetAddConnection
518 pascal16 WNetCancelConnection(ptr word) WNetCancelConnection
519 pascal WNetGetError(ptr) WNetGetError
520 pascal WNetGetErrorText(word ptr ptr) WNetGetErrorText
521 stub WNetEnable
522 stub WNetDisable
523 pascal WNetRestoreConnection(word ptr) WNetRestoreConnection
524 pascal WNetWriteJob(word ptr ptr) WNetWriteJob
525 pascal WnetConnectDialog(word word) WNetConnectDialog
526 pascal WNetDisconnectDialog(word word) WNetDisconnectDialog
527 pascal WNetConnectionDialog(word word) WNetConnectionDialog
528 pascal WNetViewQueueDialog(word ptr) WNetViewQueueDialog
529 pascal WNetPropertyDialog(word word ptr word) WNetPropertyDialog
530 pascal WNetGetDirectoryType(ptr ptr) WNetGetDirectoryType
531 pascal WNetDirectoryNotify(word ptr word) WNetDirectoryNotify
532 pascal WNetGetPropertyText(word word word ptr word) WNetGetPropertyText
