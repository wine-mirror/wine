name	user32
type	win32
init	MAIN_UserInit

  1 stdcall ActivateKeyboardLayout(long long) ActivateKeyboardLayout
  2 stdcall AdjustWindowRect(ptr long long) AdjustWindowRect
  3 stdcall AdjustWindowRectEx(ptr long long long) AdjustWindowRectEx
  4 stdcall AnyPopup() AnyPopup
  5 stdcall AppendMenuA(long long long ptr) AppendMenuA
  6 stdcall AppendMenuW(long long long ptr) AppendMenuW
  7 stdcall ArrangeIconicWindows(long) ArrangeIconicWindows
  8 stdcall AttachThreadInput(long long long) AttachThreadInput
  9 stdcall BeginDeferWindowPos(long) BeginDeferWindowPos
 10 stdcall BeginPaint(long ptr) BeginPaint
 11 stdcall BringWindowToTop(long) BringWindowToTop
 12 stdcall BroadcastSystemMessage(long ptr long long long) BroadcastSystemMessage
 13 stub CalcChildScroll
 14 stub CallMsgFilter
 15 stdcall CallMsgFilterA(ptr long) CallMsgFilterA
 16 stdcall CallMsgFilterW(ptr long) CallMsgFilterW
 17 stdcall CallNextHookEx(long long long long) CallNextHookEx
 18 stdcall CallWindowProcA(ptr long long long long) CallWindowProcA
 19 stdcall CallWindowProcW(ptr long long long long) CallWindowProcW
 20 stub CascadeChildWindows
 21 stdcall CascadeWindows(long long ptr long ptr) CascadeWindows
 22 stdcall ChangeClipboardChain(long long) ChangeClipboardChain
 23 stdcall ChangeMenuA(long long ptr long long) ChangeMenuA
 24 stdcall ChangeMenuW(long long ptr long long) ChangeMenuW
 25 stdcall CharLowerA(str) CharLowerA
 26 stdcall CharLowerBuffA(str long) CharLowerBuffA
 27 stdcall CharLowerBuffW(wstr long) CharLowerBuffW
 28 stdcall CharLowerW(wstr) CharLowerW
 29 stdcall CharNextA(str) CharNextA
 30 stdcall CharNextExA(long str long) CharNextExA
 31 stdcall CharNextExW(long wstr long) CharNextExW
 32 stdcall CharNextW(wstr) CharNextW
 33 stdcall CharPrevA(str str) CharPrevA
 34 stdcall CharPrevExA(long str str long) CharPrevExA
 35 stdcall CharPrevExW(long wstr wstr long) CharPrevExW
 36 stdcall CharPrevW(wstr wstr) CharPrevW
 37 stdcall CharToOemA(str ptr) CharToOemA
 38 stdcall CharToOemBuffA(str ptr long) CharToOemBuffA
 39 stdcall CharToOemBuffW(wstr ptr long) CharToOemBuffW
 40 stdcall CharToOemW(wstr ptr) CharToOemW
 41 stdcall CharUpperA(str) CharUpperA
 42 stdcall CharUpperBuffA(str long) CharUpperBuffA
 43 stdcall CharUpperBuffW(wstr long) CharUpperBuffW
 44 stdcall CharUpperW(wstr) CharUpperW
 45 stdcall CheckDlgButton(long long long) CheckDlgButton
 46 stdcall CheckMenuItem(long long long) CheckMenuItem
 47 stdcall CheckMenuRadioItem(long long long long long) CheckMenuRadioItem
 48 stdcall CheckRadioButton(long long long long) CheckRadioButton
 49 stdcall ChildWindowFromPoint(long long long) ChildWindowFromPoint
 50 stdcall ChildWindowFromPointEx(long long long long) ChildWindowFromPointEx
 51 stub ClientThreadConnect
 52 stdcall ClientToScreen(long ptr) ClientToScreen
 53 stdcall ClipCursor(ptr) ClipCursor
 54 stdcall CloseClipboard() CloseClipboard
 55 stub CloseDesktop
 56 stdcall CloseWindow(long) CloseWindow
 57 stub CloseWindowStation
 58 stdcall CopyAcceleratorTableA(long ptr long) CopyAcceleratorTableA
 59 stdcall CopyAcceleratorTableW(long ptr long) CopyAcceleratorTableW
 60 stdcall CopyIcon(long) CopyIcon
 61 stdcall CopyImage(long long long long long) CopyImage
 62 stdcall CopyRect(ptr ptr) CopyRect
 63 stdcall CountClipboardFormats() CountClipboardFormats
 64 stdcall CreateAcceleratorTableA(ptr long) CreateAcceleratorTableA
 65 stub CreateAcceleratorTableW
 66 stdcall CreateCaret(long long long long) CreateCaret
 67 stdcall CreateCursor(long long long long long ptr ptr) CreateCursor
 68 stub CreateDesktopA
 69 stdcall CreateDesktopW(wstr wstr ptr long long ptr) CreateDesktopW
 70 stdcall CreateDialogIndirectParamA(long ptr long ptr long) CreateDialogIndirectParamA
 71 stdcall CreateDialogIndirectParamAorW (long ptr long ptr long) CreateDialogIndirectParamAorW
 72 stdcall CreateDialogIndirectParamW(long ptr long ptr long) CreateDialogIndirectParamW
 73 stdcall CreateDialogParamA(long ptr long ptr long) CreateDialogParamA
 74 stdcall CreateDialogParamW(long ptr long ptr long) CreateDialogParamW
 75 stdcall CreateIcon(long long long long long ptr ptr) CreateIcon
 76 stdcall CreateIconFromResource (ptr long long long) CreateIconFromResource
 77 stdcall CreateIconFromResourceEx(ptr long long long long long long) CreateIconFromResourceEx
 78 stdcall CreateIconIndirect(ptr) CreateIconIndirect
 79 stdcall CreateMDIWindowA(ptr ptr long long long long long long long long) CreateMDIWindowA
 80 stdcall CreateMDIWindowW(ptr ptr long long long long long long long long) CreateMDIWindowW
 81 stdcall CreateMenu() CreateMenu
 82 stdcall CreatePopupMenu() CreatePopupMenu
 83 stdcall CreateWindowExA(long str str long long long long long long long long ptr) CreateWindowExA
 84 stdcall CreateWindowExW(long wstr wstr long long long long long long long long ptr) CreateWindowExW
 85 stub CreateWindowStationA
 86 stdcall CreateWindowStationW(wstr long long ptr) CreateWindowStationW
 87 stdcall DdeAbandonTransaction(long long long)DdeAbandonTransaction
 88 stdcall DdeAccessData(long ptr) DdeAccessData
 89 stdcall DdeAddData(long ptr long long) DdeAddData
 90 stdcall DdeClientTransaction(ptr long long long long long long ptr) DdeClientTransaction
 91 stdcall DdeCmpStringHandles(long long) DdeCmpStringHandles
 92 stdcall DdeConnect(long long long ptr) DdeConnect
 93 stdcall DdeConnectList(long long long long ptr) DdeConnectList
 94 stdcall DdeCreateDataHandle(long ptr long long long long long) DdeCreateDataHandle
 95 stdcall DdeCreateStringHandleA(long str long) DdeCreateStringHandleA
 96 stdcall DdeCreateStringHandleW(long wstr long) DdeCreateStringHandleW
 97 stdcall DdeDisconnect(long) DdeDisconnect
 98 stdcall DdeDisconnectList(long) DdeDisconnectList
 99 stdcall DdeEnableCallback(long long long) DdeEnableCallback
100 stdcall DdeFreeDataHandle(long) DdeFreeDataHandle
101 stdcall DdeFreeStringHandle(long long) DdeFreeStringHandle
102 stdcall DdeGetData(long ptr long long) DdeGetData
103 stdcall DdeGetLastError(long) DdeGetLastError
104 stub DdeGetQualityOfService
105 stdcall DdeImpersonateClient(long) DdeImpersonateClient
106 stdcall DdeInitializeA(ptr ptr long long) DdeInitializeA
107 stdcall DdeInitializeW(ptr ptr long long) DdeInitializeW
108 stdcall DdeKeepStringHandle(long long) DdeKeepStringHandle
109 stdcall DdeNameService(long long long long) DdeNameService
110 stdcall DdePostAdvise(long long long) DdePostAdvise
111 stdcall DdeQueryConvInfo(long long ptr) DdeQueryConvInfo
112 stdcall DdeQueryNextServer(long long) DdeQueryNextServer
113 stdcall DdeQueryStringA(long long ptr long long) DdeQueryStringA
114 stdcall DdeQueryStringW(long long ptr long long) DdeQueryStringW
115 stdcall DdeReconnect(long) DdeReconnect
116 stdcall DdeSetQualityOfService(long ptr ptr) DdeSetQualityOfService
117 stdcall DdeSetUserHandle (long long long) DdeSetUserHandle
118 stdcall DdeUnaccessData(long) DdeUnaccessData
119 stdcall DdeUninitialize(long) DdeUninitialize
120 stdcall DefDlgProcA(long long long long) DefDlgProcA
121 stdcall DefDlgProcW(long long long long) DefDlgProcW
122 stdcall DefFrameProcA(long long long long long) DefFrameProcA
123 stdcall DefFrameProcW(long long long long long) DefFrameProcW
124 stdcall DefMDIChildProcA(long long long long) DefMDIChildProcA
125 stdcall DefMDIChildProcW(long long long long) DefMDIChildProcW
126 stdcall DefWindowProcA(long long long long) DefWindowProcA
127 stdcall DefWindowProcW(long long long long) DefWindowProcW
128 stdcall DeferWindowPos(long long long long long long long long) DeferWindowPos
129 stdcall DeleteMenu(long long long) DeleteMenu
130 stdcall DestroyAcceleratorTable(long) DestroyAcceleratorTable
131 stdcall DestroyCaret() DestroyCaret
132 stdcall DestroyCursor(long) DestroyCursor
133 stdcall DestroyIcon(long) DestroyIcon
134 stdcall DestroyMenu(long) DestroyMenu
135 stdcall DestroyWindow(long) DestroyWindow
136 stdcall DialogBoxIndirectParamA(long ptr long ptr long) DialogBoxIndirectParamA
137 stdcall DialogBoxIndirectParamAorW(long ptr long ptr long) DialogBoxIndirectParamA
138 stdcall DialogBoxIndirectParamW(long ptr long ptr long) DialogBoxIndirectParamW
139 stdcall DialogBoxParamA(long str long ptr long) DialogBoxParamA
140 stdcall DialogBoxParamW(long wstr long ptr long) DialogBoxParamW
141 stdcall DispatchMessageA(ptr) DispatchMessageA
142 stdcall DispatchMessageW(ptr) DispatchMessageW
143 stdcall DlgDirListA(long ptr long long long) DlgDirListA
144 stdcall DlgDirListComboBoxA(long ptr long long long) DlgDirListComboBoxA
145 stdcall DlgDirListComboBoxW(long ptr long long long) DlgDirListComboBoxW
146 stdcall DlgDirListW(long ptr long long long) DlgDirListW
147 stdcall DlgDirSelectComboBoxExA(long ptr long long) DlgDirSelectComboBoxExA
148 stdcall DlgDirSelectComboBoxExW(long ptr long long) DlgDirSelectComboBoxExW
149 stdcall DlgDirSelectExA(long ptr long long) DlgDirSelectExA
150 stdcall DlgDirSelectExW(long ptr long long) DlgDirSelectExW
151 stdcall DragDetect(long long long) DragDetect
152 stub DragObject
153 stdcall DrawAnimatedRects(long long ptr ptr) DrawAnimatedRects
154 stdcall DrawCaption(long long ptr long) DrawCaption
155 stdcall DrawEdge(long ptr long long) DrawEdge
156 stdcall DrawFocusRect(long ptr) DrawFocusRect
157 stub DrawFrame
158 stdcall DrawFrameControl(long ptr long long) DrawFrameControl
159 stdcall DrawIcon(long long long long) DrawIcon
160 stdcall DrawIconEx(long long long long long long long long long) DrawIconEx
161 stdcall DrawMenuBar(long) DrawMenuBar
162 stdcall DrawStateA(long long ptr long long long long long long long) DrawStateA
163 stdcall DrawStateW(long long ptr long long long long long long long) DrawStateW
164 stdcall DrawTextA(long str long ptr long) DrawTextA
165 stdcall DrawTextExA(long str long ptr long ptr) DrawTextExA
166 stdcall DrawTextExW(long wstr long ptr long ptr) DrawTextExW
167 stdcall DrawTextW(long wstr long ptr long) DrawTextW
168 stub EditWndProc
169 stdcall EmptyClipboard() EmptyClipboard
170 stdcall EnableMenuItem(long long long) EnableMenuItem
171 stdcall EnableScrollBar(long long long) EnableScrollBar
172 stdcall EnableWindow(long long) EnableWindow
173 stdcall EndDeferWindowPos(long) EndDeferWindowPos
174 stdcall EndDialog(long long) EndDialog
175 stdcall EndMenu() EndMenu
176 stdcall EndPaint(long ptr) EndPaint
177 stub EndTask
178 stdcall EnumChildWindows(long ptr long) EnumChildWindows
179 stdcall EnumClipboardFormats(long) EnumClipboardFormats
180 stub EnumDesktopsA
181 stub EnumDesktopsW
182 stub EnumDisplayDeviceModesA
183 stub EnumDisplayDeviceModesW
184 stub EnumDisplayDevicesA
185 stub EnumDisplayDevicesW
186 stdcall EnumPropsA(long ptr) EnumPropsA
187 stdcall EnumPropsExA(long ptr long) EnumPropsExA
188 stdcall EnumPropsExW(long ptr long) EnumPropsExW
189 stdcall EnumPropsW(long ptr) EnumPropsW
190 stdcall EnumThreadWindows(long ptr long) EnumThreadWindows
191 stub EnumWindowStationsA
192 stub EnumWindowStationsW
193 stdcall EnumWindows(ptr long) EnumWindows
194 stdcall EqualRect(ptr ptr) EqualRect
195 stdcall ExcludeUpdateRgn(long long) ExcludeUpdateRgn
196 stdcall ExitWindowsEx(long long) ExitWindowsEx
197 stdcall FillRect(long ptr long) FillRect
198 stdcall FindWindowA(str str) FindWindowA
199 stdcall FindWindowExA(long long str str) FindWindowExA
200 stdcall FindWindowExW(long long wstr wstr) FindWindowExW
201 stdcall FindWindowW(wstr wstr) FindWindowW
202 stdcall FlashWindow(long long) FlashWindow
203 stdcall FrameRect(long ptr long) FrameRect
204 stdcall FreeDDElParam(long long) FreeDDElParam
205 stdcall GetActiveWindow() GetActiveWindow
206 stdcall GetAppCompatFlags(long) GetAppCompatFlags
207 stdcall GetAsyncKeyState(long) GetAsyncKeyState
208 stdcall GetCapture() GetCapture
209 stdcall GetCaretBlinkTime() GetCaretBlinkTime
210 stdcall GetCaretPos(ptr) GetCaretPos
211 stdcall GetClassInfoA(long str ptr) GetClassInfoA
212 stdcall GetClassInfoExA(long str ptr) GetClassInfoExA
213 stdcall GetClassInfoExW(long wstr ptr) GetClassInfoExW
214 stdcall GetClassInfoW(long wstr ptr) GetClassInfoW
215 stdcall GetClassLongA(long long) GetClassLongA
216 stdcall GetClassLongW(long long) GetClassLongW
217 stdcall GetClassNameA(long ptr long) GetClassNameA
218 stdcall GetClassNameW(long ptr long) GetClassNameW
219 stdcall GetClassWord(long long) GetClassWord
220 stdcall GetClientRect(long long) GetClientRect
221 stdcall GetClipCursor(ptr) GetClipCursor
222 stdcall GetClipboardData(long) GetClipboardData
223 stdcall GetClipboardFormatNameA(long ptr long) GetClipboardFormatNameA
224 stdcall GetClipboardFormatNameW(long ptr long) GetClipboardFormatNameW
225 stdcall GetClipboardOwner() GetClipboardOwner
226 stdcall GetClipboardViewer(long) GetClipboardViewer
227 stdcall GetCursor() GetCursor
228 stub GetCursorInfo
229 stdcall GetCursorPos(ptr) GetCursorPos
230 stdcall GetDC(long) GetDC
231 stdcall GetDCEx(long long long) GetDCEx
232 stdcall GetDesktopWindow() GetDesktopWindow
233 stdcall GetDialogBaseUnits() GetDialogBaseUnits
234 stdcall GetDlgCtrlID(long) GetDlgCtrlID
235 stdcall GetDlgItem(long long) GetDlgItem
236 stdcall GetDlgItemInt(long long ptr long) GetDlgItemInt
237 stdcall GetDlgItemTextA(long long ptr long) GetDlgItemTextA
238 stdcall GetDlgItemTextW(long long ptr long) GetDlgItemTextW
239 stdcall GetDoubleClickTime() GetDoubleClickTime
240 stdcall GetFocus() GetFocus
241 stdcall GetForegroundWindow() GetForegroundWindow
242 stdcall GetIconInfo(long ptr) GetIconInfo
243 stub GetInputDesktop
244 stdcall GetInputState() GetInputState
245 stdcall GetInternalWindowPos(long ptr ptr) GetInternalWindowPos
246 stdcall GetKBCodePage() GetKBCodePage
247 stdcall GetKeyNameTextA(long ptr long) GetKeyNameTextA
248 stdcall GetKeyNameTextW(long ptr long) GetKeyNameTextW
249 stdcall GetKeyState(long) GetKeyState
250 stdcall GetKeyboardLayout(long) GetKeyboardLayout
251 stdcall GetKeyboardLayoutList(long ptr) GetKeyboardLayoutList
252 stdcall GetKeyboardLayoutNameA(ptr) GetKeyboardLayoutNameA
253 stdcall GetKeyboardLayoutNameW(ptr) GetKeyboardLayoutNameW
254 stdcall GetKeyboardState(ptr) GetKeyboardState
255 stdcall GetKeyboardType(long) GetKeyboardType
256 stdcall GetLastActivePopup(long) GetLastActivePopup
257 stdcall GetMenu(long) GetMenu
258 stdcall GetMenuCheckMarkDimensions() GetMenuCheckMarkDimensions
259 stdcall GetMenuContextHelpId(long) GetMenuContextHelpId
260 stdcall GetMenuDefaultItem(long long long) GetMenuDefaultItem
261 stub GetMenuIndex
262 stdcall GetMenuItemCount(long) GetMenuItemCount
263 stdcall GetMenuItemID(long long) GetMenuItemID
264 stdcall GetMenuItemInfoA(long long long ptr) GetMenuItemInfoA
265 stdcall GetMenuItemInfoW(long long long ptr) GetMenuItemInfoW
266 stdcall GetMenuItemRect(long long long ptr) GetMenuItemRect
267 stdcall GetMenuState(long long long) GetMenuState
268 stdcall GetMenuStringA(long long ptr long long) GetMenuStringA
269 stdcall GetMenuStringW(long long ptr long long) GetMenuStringW
270 stdcall GetMessageA(ptr long long long) GetMessageA
271 stdcall GetMessageExtraInfo() GetMessageExtraInfo
272 stdcall GetMessagePos() GetMessagePos
273 stdcall GetMessageTime() GetMessageTime
274 stdcall GetMessageW(ptr long long long) GetMessageW
275 stdcall GetNextDlgGroupItem(long long long) GetNextDlgGroupItem
276 stdcall GetNextDlgTabItem(long long long) GetNextDlgTabItem
277 stdcall GetOpenClipboardWindow() GetOpenClipboardWindow
278 stdcall GetParent(long) GetParent
279 stdcall GetPriorityClipboardFormat(ptr long) GetPriorityClipboardFormat
280 stdcall GetProcessWindowStation() GetProcessWindowStation
281 stdcall GetPropA(long ptr) GetPropA
282 stdcall GetPropW(long ptr) GetPropW
283 stdcall GetQueueStatus(long) GetQueueStatus
284 stdcall GetScrollInfo(long long ptr) GetScrollInfo
285 stdcall GetScrollPos(long long) GetScrollPos
286 stdcall GetScrollRange(long long ptr ptr) GetScrollRange
287 stdcall GetShellWindow() GetShellWindow
288 stdcall GetSubMenu(long long) GetSubMenu
289 stdcall GetSysColor(long) GetSysColor
290 stdcall GetSysColorBrush(long) GetSysColorBrush
291 stdcall GetSystemMenu(long long) GetSystemMenu
292 stdcall GetSystemMetrics(long) GetSystemMetrics
293 stdcall GetTabbedTextExtentA(long str long long ptr) GetTabbedTextExtentA
294 stdcall GetTabbedTextExtentW(long wstr long long ptr) GetTabbedTextExtentW
295 stdcall GetThreadDesktop(long) GetThreadDesktop
296 stdcall GetTopWindow(long) GetTopWindow
297 stdcall GetUpdateRect(long ptr long) GetUpdateRect
298 stdcall GetUpdateRgn(long long long) GetUpdateRgn
299 stdcall GetUserObjectInformationA (long long ptr long ptr) GetUserObjectInformationA
300 stdcall GetUserObjectInformationW (long long ptr long ptr) GetUserObjectInformationW
301 stdcall GetUserObjectSecurity (long ptr ptr long ptr) GetUserObjectSecurity
302 stdcall GetWindow(long long) GetWindow
303 stdcall GetWindowContextHelpId(long) GetWindowContextHelpId
304 stdcall GetWindowDC(long) GetWindowDC
305 stdcall GetWindowLongA(long long) GetWindowLongA
306 stdcall GetWindowLongW(long long) GetWindowLongW
307 stdcall GetWindowPlacement(long ptr) GetWindowPlacement
308 stdcall GetWindowRect(long ptr) GetWindowRect
309 stdcall GetWindowTextA(long ptr long) GetWindowTextA
310 stdcall GetWindowTextLengthA(long) GetWindowTextLengthA
311 stdcall GetWindowTextLengthW(long) GetWindowTextLengthW
312 stdcall GetWindowTextW(long ptr long) GetWindowTextW
313 stdcall GetWindowThreadProcessId(long ptr) GetWindowThreadProcessId
314 stdcall GetWindowWord(long long) GetWindowWord
315 stdcall GrayStringA(long long ptr long long long long long long) GrayStringA
316 stdcall GrayStringW(long long ptr long long long long long long) GrayStringW
317 stdcall HideCaret(long) HideCaret
318 stdcall HiliteMenuItem(long long long long) HiliteMenuItem
319 stub ImpersonateDdeClientWindow
320 stdcall InSendMessage() InSendMessage
321 stdcall InflateRect(ptr long long) InflateRect
322 stdcall InsertMenuA(long long long long ptr) InsertMenuA
323 stdcall InsertMenuItemA(long long long ptr) InsertMenuItemA
324 stdcall InsertMenuItemW(long long long ptr) InsertMenuItemW
325 stdcall InsertMenuW(long long long long ptr) InsertMenuW
326 stdcall InternalGetWindowText(long long long) InternalGetWindowText
327 stdcall IntersectRect(ptr ptr ptr) IntersectRect
328 stdcall InvalidateRect(long ptr long) InvalidateRect
329 stdcall InvalidateRgn(long long long) InvalidateRgn
330 stdcall InvertRect(long ptr) InvertRect
331 stdcall IsCharAlphaA(long) IsCharAlphaA
332 stdcall IsCharAlphaNumericA(long) IsCharAlphaNumericA
333 stdcall IsCharAlphaNumericW(long) IsCharAlphaNumericW
334 stdcall IsCharAlphaW(long) IsCharAlphaW
335 stdcall IsCharLowerA(long) IsCharLowerA
336 stdcall IsCharLowerW(long) IsCharLowerW
337 stdcall IsCharUpperA(long) IsCharUpperA
338 stdcall IsCharUpperW(long) IsCharUpperW
339 stdcall IsChild(long long) IsChild
340 stdcall IsClipboardFormatAvailable(long) IsClipboardFormatAvailable
341 stub IsDialogMessage
342 stdcall IsDialogMessageA(long ptr) IsDialogMessageA
343 stdcall IsDialogMessageW(long ptr) IsDialogMessageW
344 stdcall IsDlgButtonChecked(long long) IsDlgButtonChecked
345 stdcall IsIconic(long) IsIconic
346 stdcall IsMenu(long) IsMenu
347 stdcall IsRectEmpty(ptr) IsRectEmpty
348 stdcall IsWindow(long) IsWindow
349 stdcall IsWindowEnabled(long) IsWindowEnabled
350 stdcall IsWindowUnicode(long) IsWindowUnicode
351 stdcall IsWindowVisible(long) IsWindowVisible
352 stdcall IsZoomed(long) IsZoomed
353 stdcall KillSystemTimer(long long) KillSystemTimer
354 stdcall KillTimer(long long) KillTimer
355 stdcall LoadAcceleratorsA(long str) LoadAcceleratorsA
356 stdcall LoadAcceleratorsW(long wstr) LoadAcceleratorsW
357 stdcall LoadBitmapA(long str) LoadBitmapA
358 stdcall LoadBitmapW(long wstr) LoadBitmapW
359 stdcall LoadCursorA(long str) LoadCursorA
360 stdcall LoadCursorFromFileA(str) LoadCursorFromFileA
361 stdcall LoadCursorFromFileW(wstr) LoadCursorFromFileW
362 stdcall LoadCursorW(long wstr) LoadCursorW
363 stdcall LoadIconA(long str) LoadIconA
364 stdcall LoadIconW(long wstr) LoadIconW
365 stdcall LoadImageA(long str long long long long) LoadImageA
366 stdcall LoadImageW(long wstr long long long long) LoadImageW
367 stdcall LoadKeyboardLayoutA(str long) LoadKeyboardLayoutA
368 stdcall LoadKeyboardLayoutW(wstr long) LoadKeyboardLayoutW
369 stdcall LoadLocalFonts() LoadLocalFonts
370 stdcall LoadMenuA(long str) LoadMenuA
371 stdcall LoadMenuIndirectA(ptr) LoadMenuIndirectA
372 stdcall LoadMenuIndirectW(ptr) LoadMenuIndirectW
373 stdcall LoadMenuW(long wstr) LoadMenuW
374 stub LoadRemoteFonts
375 stdcall LoadStringA(long long ptr long) LoadStringA
376 stdcall LoadStringW(long long ptr long) LoadStringW
377 stub LockWindowStation
378 stdcall LockWindowUpdate(long) LockWindowUpdate
379 stdcall LookupIconIdFromDirectory(ptr long) LookupIconIdFromDirectory
380 stdcall LookupIconIdFromDirectoryEx(ptr long long long long) LookupIconIdFromDirectoryEx
381 stub MBToWCSEx
382 stdcall MapDialogRect(long ptr) MapDialogRect
383 stdcall MapVirtualKeyA(long long) MapVirtualKeyA
384 stdcall MapVirtualKeyExA(long long long) MapVirtualKeyEx32A
385 stdcall MapVirtualKeyW(long long) MapVirtualKeyA
386 stdcall MapWindowPoints(long long ptr long) MapWindowPoints
387 stub MenuItemFromPoint
388 stub MenuWindowProcA
389 stub MenuWindowProcW
390 stdcall MessageBeep(long) MessageBeep
391 stdcall MessageBoxA(long str str long) MessageBoxA
392 stdcall MessageBoxExA(long str str long long) MessageBoxExA
393 stdcall MessageBoxExW(long wstr wstr long long) MessageBoxExW
394 stdcall MessageBoxIndirectA(ptr) MessageBoxIndirectA
395 stdcall MessageBoxIndirectW(ptr) MessageBoxIndirectW
396 stdcall MessageBoxW(long wstr wstr long) MessageBoxW
397 stdcall ModifyMenuA(long long long long ptr) ModifyMenuA
398 stdcall ModifyMenuW(long long long long ptr) ModifyMenuW
399 stdcall MoveWindow(long long long long long long) MoveWindow
400 stdcall MsgWaitForMultipleObjects(long ptr long long long) MsgWaitForMultipleObjects
401 stdcall OemKeyScan(long) OemKeyScan
402 stdcall OemToCharA(ptr ptr) OemToCharA
403 stdcall OemToCharBuffA(ptr ptr long) OemToCharBuffA
404 stdcall OemToCharBuffW(ptr ptr long) OemToCharBuffW
405 stdcall OemToCharW(ptr ptr) OemToCharW
406 stdcall OffsetRect(ptr long long) OffsetRect
407 stdcall OpenClipboard(long) OpenClipboard
408 stdcall OpenDesktopA(str long long long) OpenDesktopA
409 stub OpenDesktopW
410 stdcall OpenIcon(long) OpenIcon
411 stub OpenInputDesktop
412 stub OpenWindowStationA
413 stub OpenWindowStationW
414 stdcall PackDDElParam(long long long) PackDDElParam
415 stdcall PaintDesktop(long) PaintDesktop
416 stdcall PeekMessageA(ptr long long long long) PeekMessageA
417 stdcall PeekMessageW(ptr long long long long) PeekMessageW
418 stub PlaySoundEvent
419 stdcall PostMessageA(long long long long) PostMessageA
420 stdcall PostMessageW(long long long long) PostMessageW
421 stdcall PostQuitMessage(long) PostQuitMessage
422 stdcall PostThreadMessageA(long long long long) PostThreadMessageA
423 stdcall PostThreadMessageW(long long long long) PostThreadMessageW
424 stdcall PtInRect(ptr long long) PtInRect
425 stub QuerySendMessage
426 stdcall RedrawWindow(long ptr long long) RedrawWindow
427 stdcall RegisterClassA(ptr) RegisterClassA
428 stdcall RegisterClassExA(ptr) RegisterClassExA
429 stdcall RegisterClassExW(ptr) RegisterClassExW
430 stdcall RegisterClassW(ptr) RegisterClassW
431 stdcall RegisterClipboardFormatA(str) RegisterClipboardFormatA
432 stdcall RegisterClipboardFormatW(wstr) RegisterClipboardFormatW
433 stdcall RegisterHotKey(long long long long) RegisterHotKey
434 stdcall RegisterLogonProcess(long long) RegisterLogonProcess
435 stub RegisterSystemThread
436 stdcall RegisterTasklist (long) RegisterTaskList
437 stdcall RegisterWindowMessageA(str) RegisterWindowMessageA
438 stdcall RegisterWindowMessageW(wstr) RegisterWindowMessageW
439 stdcall ReleaseCapture() ReleaseCapture
440 stdcall ReleaseDC(long long) ReleaseDC
441 stdcall RemoveMenu(long long long) RemoveMenu
442 stdcall RemovePropA(long str) RemovePropA
443 stdcall RemovePropW(long wstr) RemovePropW
444 stdcall ReplyMessage(long) ReplyMessage
445 stub ResetDisplay
446 stdcall ReuseDDElParam(long long long long long) ReuseDDElParam
447 stdcall ScreenToClient(long ptr) ScreenToClient
448 stdcall ScrollChildren(long long long long) ScrollChildren
449 stdcall ScrollDC(long long long ptr ptr long ptr) ScrollDC
450 stdcall ScrollWindow(long long long ptr ptr) ScrollWindow
451 stdcall ScrollWindowEx(long long long ptr ptr long ptr long) ScrollWindowEx
452 stdcall SendDlgItemMessageA(long long long long long) SendDlgItemMessageA
453 stdcall SendDlgItemMessageW(long long long long long) SendDlgItemMessageW
454 stdcall SendMessageA(long long long long) SendMessageA
455 stdcall SendMessageCallbackA(long long long long ptr long) SendMessageCallBackA
456 stub SendMessageCallbackW
457 stdcall SendMessageTimeoutA(long long long long ptr ptr) SendMessageTimeoutA
458 stdcall SendMessageTimeoutW(long long long long ptr ptr) SendMessageTimeoutW
459 stdcall SendMessageW(long long long long) SendMessageW
460 stdcall SendNotifyMessageA(long long long long) SendNotifyMessageA
461 stdcall SendNotifyMessageW(long long long long) SendNotifyMessageW
462 stub ServerSetFunctionPointers
463 stdcall SetActiveWindow(long) SetActiveWindow
464 stdcall SetCapture(long) SetCapture
465 stdcall SetCaretBlinkTime(long) SetCaretBlinkTime
466 stdcall SetCaretPos(long long) SetCaretPos
467 stdcall SetClassLongA(long long long) SetClassLongA
468 stdcall SetClassLongW(long long long) SetClassLongW
469 stdcall SetClassWord(long long long) SetClassWord
470 stdcall SetClipboardData(long long) SetClipboardData
471 stdcall SetClipboardViewer(long) SetClipboardViewer
472 stdcall SetCursor(long) SetCursor
473 stub SetCursorContents
474 stdcall SetCursorPos(long long) SetCursorPos
475 stdcall SetDebugErrorLevel(long) SetDebugErrorLevel
476 stdcall SetDeskWallPaper(str) SetDeskWallPaper
477 stdcall SetDlgItemInt(long long long long) SetDlgItemInt
478 stdcall SetDlgItemTextA(long long str) SetDlgItemTextA
479 stdcall SetDlgItemTextW(long long wstr) SetDlgItemTextW
480 stdcall SetDoubleClickTime(long) SetDoubleClickTime
481 stdcall SetFocus(long) SetFocus
482 stdcall SetForegroundWindow(long) SetForegroundWindow
483 stdcall SetInternalWindowPos(long long ptr ptr) SetInternalWindowPos
484 stdcall SetKeyboardState(ptr) SetKeyboardState
485 stdcall SetLastErrorEx(long long) SetLastErrorEx
486 stdcall SetLogonNotifyWindow(long long) SetLogonNotifyWindow
487 stdcall SetMenu(long long) SetMenu
488 stdcall SetMenuContextHelpId(long long) SetMenuContextHelpId
489 stdcall SetMenuDefaultItem(long long long) SetMenuDefaultItem
490 stdcall SetMenuItemBitmaps(long long long long long) SetMenuItemBitmaps
491 stdcall SetMenuItemInfoA(long long long ptr) SetMenuItemInfoA
492 stdcall SetMenuItemInfoW(long long long ptr) SetMenuItemInfoW
493 stub SetMessageExtraInfo
494 stdcall SetMessageQueue(long) SetMessageQueue
495 stdcall SetParent(long long) SetParent
496 stdcall SetProcessWindowStation(long) SetProcessWindowStation
497 stdcall SetPropA(long str long) SetPropA
498 stdcall SetPropW(long wstr long) SetPropW
499 stdcall SetRect(ptr long long long long) SetRect
500 stdcall SetRectEmpty(ptr) SetRectEmpty
501 stdcall SetScrollInfo(long long ptr long) SetScrollInfo
502 stdcall SetScrollPos(long long long long) SetScrollPos
503 stdcall SetScrollRange(long long long long long) SetScrollRange
504 stdcall SetShellWindow(long) SetShellWindow
505 stdcall SetSysColors(long ptr ptr) SetSysColors
506 stub SetSysColorsTemp
507 stub SetSystemCursor
508 stdcall SetSystemMenu(long long) SetSystemMenu
509 stdcall SetSystemTimer(long long long ptr) SetSystemTimer
510 stdcall SetThreadDesktop(long) SetThreadDesktop
511 stdcall SetTimer(long long long ptr) SetTimer
512 stdcall SetUserObjectInformationA(long long long long) SetUserObjectInformationA
513 stub SetUserObjectInformationW
514 stdcall SetUserObjectSecurity(long ptr ptr) SetUserObjectSecurity
515 stdcall SetWindowContextHelpId(long long) SetWindowContextHelpId
516 stub SetWindowFullScreenState
517 stdcall SetWindowLongA(long long long) SetWindowLongA
518 stdcall SetWindowLongW(long long long) SetWindowLongW
519 stdcall SetWindowPlacement(long ptr) SetWindowPlacement
520 stdcall SetWindowPos(long long long long long long long) SetWindowPos
521 stdcall SetWindowStationUser(long long) SetWindowStationUser
522 stdcall SetWindowTextA(long str) SetWindowTextA
523 stdcall SetWindowTextW(long wstr) SetWindowTextW
524 stdcall SetWindowWord(long long long) SetWindowWord
525 stdcall SetWindowsHookA(long ptr) SetWindowsHookA
526 stdcall SetWindowsHookExA(long long long long) SetWindowsHookExA
527 stdcall SetWindowsHookExW(long long long long) SetWindowsHookExW
528 stdcall SetWindowsHookW(long long long long) SetWindowsHookW
529 stdcall ShowCaret(long) ShowCaret
530 stdcall ShowCursor(long) ShowCursor
531 stdcall ShowOwnedPopups(long long) ShowOwnedPopups
532 stdcall ShowScrollBar(long long long) ShowScrollBar
533 stub ShowStartGlass
534 stdcall ShowWindow(long long) ShowWindow
535 stdcall ShowWindowAsync(long long) ShowWindowAsync
536 stdcall SubtractRect(ptr ptr ptr) SubtractRect
537 stdcall SwapMouseButton(long) SwapMouseButton
538 stub SwitchDesktop
539 stdcall SwitchToThisWindow(long long) SwitchToThisWindow
540 stdcall SystemParametersInfoA(long long ptr long) SystemParametersInfoA
541 stdcall SystemParametersInfoW(long long ptr long) SystemParametersInfoW
542 stdcall TabbedTextOutA(long long long str long long ptr long) TabbedTextOutA
543 stdcall TabbedTextOutW(long long long wstr long long ptr long) TabbedTextOutW
544 stub TileChildWindows
545 stdcall TileWindows(long long ptr long ptr) TileWindows
546 stdcall ToAscii(long long ptr ptr long) ToAscii
547 stub ToAsciiEx
548 stdcall ToUnicode(long long ptr wstr long long) ToUnicode
549 stdcall TrackPopupMenu(long long long long long long ptr) TrackPopupMenu
550 stdcall TrackPopupMenuEx(long long long long long ptr) TrackPopupMenuEx
551 stdcall TranslateAccelerator(long long ptr) TranslateAccelerator
552 stdcall TranslateAcceleratorA(long long ptr) TranslateAccelerator
553 stdcall TranslateAcceleratorW(long long ptr) TranslateAccelerator
554 stub TranslateCharsetInfo
555 stdcall TranslateMDISysAccel(long ptr) TranslateMDISysAccel
556 stdcall TranslateMessage(ptr) TranslateMessage
557 stdcall UnhookWindowsHook(long ptr) UnhookWindowsHook
558 stdcall UnhookWindowsHookEx(long) UnhookWindowsHookEx
559 stdcall UnionRect(ptr ptr ptr) UnionRect
560 stub UnloadKeyboardLayout
561 stub UnlockWindowStation
562 stdcall UnpackDDElParam(long long ptr ptr) UnpackDDElParam
563 stdcall UnregisterClassA(str long) UnregisterClassA
564 stdcall UnregisterClassW(wstr long) UnregisterClassW
565 stdcall UnregisterHotKey(long long) UnregisterHotKey
566 stub UpdatePerUserSystemParameters
567 stdcall UpdateWindow(long) UpdateWindow
568 stub UserClientDllInitialize
569 stub UserRealizePalette
570 stub UserRegisterWowHandlers
571 stdcall ValidateRect(long ptr) ValidateRect
572 stdcall ValidateRgn(long long) ValidateRgn
573 stdcall VkKeyScanA(long) VkKeyScanA
574 stub VkKeyScanExA
575 stub VkKeyScanExW
576 stdcall VkKeyScanW(long) VkKeyScanW
577 stdcall WaitForInputIdle(long long) WaitForInputIdle
578 stdcall WaitMessage() WaitMessage
579 stdcall WinHelpA(long str long long) WinHelpA
580 stdcall WinHelpW(long wstr long long) WinHelpW
581 stdcall WindowFromDC(long) WindowFromDC
582 stdcall WindowFromPoint(long long) WindowFromPoint
583 stdcall keybd_event(long long long long) keybd_event
584 stdcall mouse_event(long long long long long) mouse_event
585 varargs wsprintfA() wsprintfA
586 varargs wsprintfW() wsprintfW
587 stdcall wvsprintfA(ptr str ptr) wvsprintfA
588 stdcall wvsprintfW(ptr wstr ptr) wvsprintfW
#late additions
589 stdcall ChangeDisplaySettingsA(ptr long) ChangeDisplaySettingsA
590 stub ChangeDisplaySettingsW
591 stub EnumDesktopWindows
592 stdcall EnumDisplaySettingsA(str long ptr) EnumDisplaySettingsA
593 stdcall EnumDisplaySettingsW(wstr long ptr ) EnumDisplaySettingsW
594 stdcall GetWindowRgn(long long) GetWindowRgn
595 stub MapVirtualKeyExW
596 stub RegisterServicesProcess
597 stdcall SetWindowRgn(long long long) SetWindowRgn
598 stub ToUnicodeEx
599 stdcall DrawCaptionTempA(long long ptr long long str long) DrawCaptionTempA
600 stub RegisterNetworkCapabilities
601 stub WNDPROC_CALLBACK
602 stdcall DrawCaptionTempW(long long ptr long long wstr long) DrawCaptionTempW
603 stub IsHungAppWindow
604 stub ChangeDisplaySettingsA
605 stub ChangeDisplaySettingsW
606 stdcall SetWindowText(long str) SetWindowTextA
607 stdcall GetMonitorInfoA(long ptr) GetMonitorInfoA
608 stdcall GetMonitorInfoW(long ptr) GetMonitorInfoW
609 stdcall MonitorFromWindow(long long) MonitorFromWindow
610 stdcall MonitorFromRect(ptr long) MonitorFromRect
611 stdcall MonitorFromPoint(long long long) MonitorFromPoint
612 stdcall EnumDisplayMonitors(long ptr ptr long) EnumDisplayMonitors
613 stdcall PrivateExtractIconExA (long long long long long) PrivateExtractIconExA
614 stdcall PrivateExtractIconExW (long long long long long) PrivateExtractIconExW
615 stdcall PrivateExtractIconsW (long long long long long long long long) PrivateExtractIconsW
616 stdcall RegisterShellHookWindow (long) RegisterShellHookWindow
617 stdcall DeregisterShellHookWindow (long) DeregisterShellHookWindow
618 stdcall SetShellWindowEx (long long) SetShellWindowEx
619 stdcall SetProgmanWindow (long) SetProgmanWindow
620 stdcall GetTaskmanWindow () GetTaskmanWindow
621 stdcall SetTaskmanWindow (long) SetTaskmanWindow
622 stdcall GetProgmanWindow () GetProgmanWindow
