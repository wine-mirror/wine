name	user32
type	win32
base	1

0000 stub ActivateKeyboardLayout
0001 stub AdjustWindowRect
0002 stub AdjustWindowRectEx
0003 stub AnyPopup
0004 stub AppendMenuA
0005 stub AppendMenuW
0006 stub ArrangeIconicWindows
0007 stub AttachThreadInput
0008 stub BeginDeferWindowPos
0009 stdcall BeginPaint(long ptr) USER32_BeginPaint
0010 stub BringWindowToTop
0011 stub BroadcastSystemMessage
0012 stub CalcChildScroll
0013 stub CallMsgFilter
0014 stub CallMsgFilterA
0015 stub CallMsgFilterW
0016 stub CallNextHookEx
0017 stub CallWindowProcA
0018 stub CallWindowProcW
0019 stub CascadeChildWindows
0020 stub CascadeWindows
0021 stub ChangeClipboardChain
0022 stub ChangeMenuA
0023 stub ChangeMenuW
0024 stub CharLowerA
0025 stub CharLowerBuffA
0026 stub CharLowerBuffW
0027 stub CharLowerW
0028 stub CharNextA
0029 stub CharNextExA
0030 stub CharNextExW
0031 stub CharNextW
0032 stub CharPrevA
0033 stub CharPrevExA
0034 stub CharPrevExW
0035 stub CharPrevW
0036 stub CharToOemA
0037 stub CharToOemBuffA
0038 stub CharToOemBuffW
0039 stub CharToOemW
0040 stub CharUpperA
0041 stub CharUpperBuffA
0042 stub CharUpperBuffW
0043 stub CharUpperW
0044 stub CheckDlgButton
0045 stdcall CheckMenuItem(long long long) CheckMenuItem
0046 stub CheckMenuRadioItem
0047 stub CheckRadioButton
0048 stub ChildWindowFromPoint
0049 stub ChildWindowFromPointEx
0050 stub ClientThreadConnect
0051 stub ClientToScreen
0052 stub ClipCursor
0053 stub CloseClipboard
0054 stub CloseDesktop
0055 stub CloseWindow
0056 stub CloseWindowStation
0057 stub CopyAcceleratorTableA
0058 stub CopyAcceleratorTableW
0059 stub CopyIcon
0060 stub CopyImage
0061 stub CopyRect
0062 stub CountClipboardFormats
0063 stub CreateAcceleratorTableA
0064 stub CreateAcceleratorTableW
0065 stub CreateCaret
0066 stub CreateCursor
0067 stub CreateDesktopA
0068 stub CreateDesktopW
0069 stdcall CreateDialogIndirectParamA(long ptr long ptr long)	
	USER32_CreateDialogIndirectParamA
0070 stub CreateDialogIndirectParamAorW
0071 stdcall CreateDialogIndirectParamW(long ptr long ptr long)	
	USER32_CreateDialogIndirectParamW
0072 stdcall CreateDialogParamA(long ptr long ptr long)	
	USER32_CreateDialogParamA
0073 stdcall CreateDialogParamW(long ptr long ptr long)	
	USER32_CreateDialogParamW
0074 stub CreateIcon
0075 stub CreateIconFromResource
0076 stub CreateIconFromResourceEx
0077 stub CreateIconIndirect
0078 stub CreateMDIWindowA
0079 stub CreateMDIWindowW
0080 stub CreateMenu
0081 stub CreatePopupMenu
0082 stdcall CreateWindowExA(long ptr ptr long long long long long 
							long long long ptr)	USER32_CreateWindowExA
0083 stub CreateWindowExW
0084 stub CreateWindowStationA
0085 stub CreateWindowStationW
0086 stub DdeAbandonTransaction
0087 stub DdeAccessData
0088 stub DdeAddData
0089 stub DdeClientTransaction
0090 stub DdeCmpStringHandles
0091 stub DdeConnect
0092 stub DdeConnectList
0093 stub DdeCreateDataHandle
0094 stub DdeCreateStringHandleA
0095 stub DdeCreateStringHandleW
0096 stub DdeDisconnect
0097 stub DdeDisconnectList
0098 stub DdeEnableCallback
0099 stub DdeFreeDataHandle
0100 stub DdeFreeStringHandle
0101 stub DdeGetData
0102 stub DdeGetLastError
0103 stub DdeGetQualityOfService
0104 stub DdeImpersonateClient
0105 stub DdeInitializeA
0106 stub DdeInitializeW
0107 stub DdeKeepStringHandle
0108 stub DdeNameService
0109 stub DdePostAdvise
0110 stub DdeQueryConvInfo
0111 stub DdeQueryNextServer
0112 stub DdeQueryStringA
0113 stub DdeQueryStringW
0114 stub DdeReconnect
0115 stub DdeSetQualityOfService
0116 stub DdeSetUserHandle
0117 stub DdeUnaccessData
0118 stub DdeUninitialize
0119 stub DefDlgProcA
0120 stub DefDlgProcW
0121 stub DefFrameProcA
0122 stub DefFrameProcW
0123 stub DefMDIChildProcA
0124 stub DefMDIChildProcW
0125 stdcall DefWindowProcA(long long long long) USER32_DefWindowProcA
0126 stub DefWindowProcW
0127 stub DeferWindowPos
0128 stub DeleteMenu
0129 stub DestroyAcceleratorTable
0130 stub DestroyCaret
0131 stub DestroyCursor
0132 stub DestroyIcon
0133 stub DestroyMenu
0134 stub DestroyWindow
0135 stdcall DialogBoxIndirectParamA(long ptr long ptr long)	
			USER32_DialogBoxIndirectParamA
0136 stub DialogBoxIndirectParamAorW
0137 stdcall DialogBoxIndirectParamW(long ptr long ptr long)	
			USER32_DialogBoxIndirectParamW
0138 stdcall DialogBoxParamA(long ptr long ptr long)	USER32_DialogBoxParamA
0139 stdcall DialogBoxParamW(long ptr long ptr long)	USER32_DialogBoxParamW
0140 stdcall DispatchMessageA(ptr) USER32_DispatchMessageA
0141 stub DispatchMessageW
0142 stub DlgDirListA
0143 stub DlgDirListComboBoxA
0144 stub DlgDirListComboBoxW
0145 stub DlgDirListW
0146 stub DlgDirSelectComboBoxExA
0147 stub DlgDirSelectComboBoxExW
0148 stub DlgDirSelectExA
0149 stub DlgDirSelectExW
0150 stub DragDetect
0151 stub DragObject
0152 stub DrawAnimatedRects
0153 stub DrawCaption
0154 stub DrawEdge
0155 stub DrawFocusRect
0156 stub DrawFrame
0157 stub DrawFrameControl
0158 stub DrawIcon
0159 stub DrawIconEx
0160 stdcall DrawMenuBar(long) DrawMenuBar
0161 stub DrawStateA
0162 stub DrawStateW
0163 stdcall DrawTextA(long ptr long ptr long) USER32_DrawTextA
0164 stub DrawTextExA
0165 stub DrawTextExW
0166 stub DrawTextW
0167 stub EditWndProc
0168 stdcall EmptyClipboard()				EmptyClipboard
0169 stdcall EnableMenuItem(long long long) EnableMenuItem
0170 stdcall EnableScrollBar(long long long)	EnableScrollBar
0171 stdcall EnableWindow(long long)		EnableWindow
0172 stub EndDeferWindowPos
0173 stdcall EndDialog(long long) EndDialog
0174 stub EndMenu
0175 stdcall EndPaint(long ptr) USER32_EndPaint
0176 stub EndTask
0177 stub EnumChildWindows
0178 stub EnumClipboardFormats
0179 stub EnumDesktopsA
0180 stub EnumDesktopsW
0181 stub EnumDisplayDeviceModesA
0182 stub EnumDisplayDeviceModesW
0183 stub EnumDisplayDevicesA
0184 stub EnumDisplayDevicesW
0185 stub EnumPropsA
0186 stub EnumPropsExA
0187 stub EnumPropsExW
0188 stub EnumPropsW
0189 stub EnumThreadWindows
0190 stub EnumWindowStationsA
0191 stub EnumWindowStationsW
0192 stub EnumWindows
0193 stub EqualRect
0194 stub ExcludeUpdateRgn
0195 stub ExitWindowsEx
0196 stub FillRect
0197 	return FindWindowA 8 0
0198 stub FindWindowExA
0199 stub FindWindowExW
0200 stub FindWindowW
0201 stdcall FlashWindow(long long) FlashWindow
0202 stub FrameRect
0203 stub FreeDDElParam
0204 stub GetActiveWindow
0205 stub GetAppCompatFlags
0206 stub GetAsyncKeyState
0207 stub GetCapture
0208 stub GetCaretBlinkTime
0209 stub GetCaretPos
0210 stub GetClassInfoA
0211 stub GetClassInfoExA
0212 stub GetClassInfoExW
0213 stub GetClassInfoW
0214 stub GetClassLongA
0215 stub GetClassLongW
0216 stub GetClassNameA
0217 stub GetClassNameW
0218 stub GetClassWord
0219 stdcall GetClientRect(long long) USER32_GetClientRect
0220 stub GetClipCursor
0221 stub GetClipboardData
0222 stub GetClipboardFormatNameA
0223 stub GetClipboardFormatNameW
0224 stub GetClipboardOwner
0225 stub GetClipboardViewer
0226 stub GetCursor
0227 stub GetCursorInfo
0228 stub GetCursorPos
0229 stdcall GetDC(long) GetDC
0230 stub GetDCEx
0231 stdcall GetDesktopWindow() GetDesktopWindow
0232 stub GetDialogBaseUnits
0233 stub GetDlgCtrlID
0234 stdcall GetDlgItem(long long) GetDlgItem
0235 stdcall GetDlgItemInt(long long long long) GetDlgItemInt
0236 stub GetDlgItemTextA
0237 stub GetDlgItemTextW
0238 stub GetDoubleClickTime
0239 stub GetFocus
0240 stub GetForegroundWindow
0241 stub GetIconInfo
0242 stub GetInputDesktop
0243 stub GetInputState
0244 stub GetInternalWindowPos
0245 stub GetKBCodePage
0246 stub GetKeyNameTextA
0247 stub GetKeyNameTextW
0248 stub GetKeyState
0249 stub GetKeyboardLayout
0250 stub GetKeyboardLayoutList
0251 stub GetKeyboardLayoutNameA
0252 stub GetKeyboardLayoutNameW
0253 stub GetKeyboardState
0254 stub GetKeyboardType
0255 stub GetLastActivePopup
0256 stdcall GetMenu(long) GetMenu
0257 stub GetMenuCheckMarkDimensions
0258 stub GetMenuContextHelpId
0259 stub GetMenuDefaultItem
0260 stub GetMenuIndex
0261 stub GetMenuItemCount
0262 stub GetMenuItemID
0263 stub GetMenuItemInfoA
0264 stub GetMenuItemInfoW
0265 stub GetMenuItemRect
0266 stub GetMenuState
0267 stub GetMenuStringA
0268 stub GetMenuStringW
0269 stdcall GetMessageA(ptr long long long) USER32_GetMessageA
0270 stub GetMessageExtraInfo
0271 stub GetMessagePos
0272 stub GetMessageTime
0273 stub GetMessageW
0274 stub GetNextDlgGroupItem
0275 stub GetNextDlgTabItem
0276 stub GetOpenClipboardWindow
0277 stub GetParent
0278 stub GetPriorityClipboardFormat
0279 stub GetProcessWindowStation
0280 stub GetPropA
0281 stub GetPropW
0282 stub GetQueueStatus
0283 stub GetScrollInfo
0284 stub GetScrollPos
0285 stub GetScrollRange
0286 stub GetShellWindow
0287 stub GetSubMenu
0288 	stdcall GetSysColor(long) GetSysColor
0289 stdcall GetSysColorBrush(long) GetSysColorBrush
0290 stdcall GetSystemMenu(long long) GetSystemMenu
0291 stdcall GetSystemMetrics(long) GetSystemMetrics
0292 stub GetTabbedTextExtentA
0293 stub GetTabbedTextExtentW
0294 stub GetThreadDesktop
0295 stub GetTopWindow
0296 stub GetUpdateRect
0297 stub GetUpdateRgn
0298 stub GetUserObjectInformationA
0299 stub GetUserObjectInformationW
0300 stub GetUserObjectSecurity
0301 stdcall GetWindow(long long) GetWindow
0302 stub GetWindowContextHelpId
0303 stdcall GetWindowDC(long) GetWindowDC
0304 stub GetWindowLongA
0305 stub GetWindowLongW
0306 stub GetWindowPlacement
0307 stub GetWindowRect
0308 stdcall GetWindowTextA(long segptr long) WIN16_GetWindowText
0309 stub GetWindowTextLengthA
0310 stub GetWindowTextLengthW
0311 stub GetWindowTextW
0312 stub GetWindowThreadProcessId
0313 stub GetWindowWord
0314 stub GrayStringA
0315 stub GrayStringW
0316 stub HideCaret
0317 stub HiliteMenuItem
0318 stub ImpersonateDdeClientWindow
0319 stub InSendMessage
0320 stub InflateRect
0321 stub InsertMenuA
0322 stub InsertMenuItemA
0323 stub InsertMenuItemW
0324 stub InsertMenuW
0325 stub InternalGetWindowText
0326 stub IntersectRect
0327 stdcall InvalidateRect(long ptr long)	USER32_InvalidateRect
0328 stub InvalidateRgn
0329 stub InvertRect
0330 stub IsCharAlphaA
0331 stub IsCharAlphaNumericA
0332 stub IsCharAlphaNumericW
0333 stub IsCharAlphaW
0334 stub IsCharLowerA
0335 stub IsCharLowerW
0336 stub IsCharUpperA
0337 stub IsCharUpperW
0338 stub IsChild
0339 stub IsClipboardFormatAvailable
0340 stub IsDialogMessage
0341 stub IsDialogMessageA
0342 stub IsDialogMessageW
0343 stub IsDlgButtonChecked
0344 stdcall IsIconic(long) IsIconic
0345 stub IsMenu
0346 stub IsRectEmpty
0347 stub IsWindow
0348 stub IsWindowEnabled
0349 stub IsWindowUnicode
0350 stub IsWindowVisible
0351 stub IsZoomed
0352 stub KillSystemTimer
0353 stdcall KillTimer(long long) KillTimer
0354 	stdcall LoadAcceleratorsA(long ptr) WIN32_LoadAcceleratorsA
0355 stdcall LoadAcceleratorsW(long ptr)	WIN32_LoadAcceleratorsW
0356 	stdcall LoadBitmapA(long ptr) WIN32_LoadBitmapA
0357 	stdcall LoadBitmapW(long ptr) WIN32_LoadBitmapW
0358 	stdcall LoadCursorA(long ptr) WIN32_LoadCursorA
0359 stub LoadCursorFromFileA
0360 stub LoadCursorFromFileW
0361	stdcall LoadCursorW(long ptr) WIN32_LoadCursorW
0362	stdcall LoadIconA(long ptr) WIN32_LoadIconA
0363 	stdcall LoadIconW(long ptr) WIN32_LoadIconW
0364 stub LoadImageA
0365 stub LoadImageW
0366 stub LoadKeyboardLayoutA
0367 stub LoadKeyboardLayoutW
0368 stub LoadLocalFonts
0369 stdcall LoadMenuA(long ptr) WIN32_LoadMenuA
0370 stdcall LoadMenuIndirectA(long ptr) WIN32_LoadMenuIndirectA
0371 stdcall LoadMenuIndirectW(long ptr) WIN32_LoadMenuIndirectW
0372 stdcall LoadMenuW(long ptr) WIN32_LoadMenuW
0373 stub LoadRemoteFonts
0374 	stdcall LoadStringA(long long ptr long) WIN32_LoadStringA
0375 	stdcall LoadStringW(long long ptr long) WIN32_LoadStringW
0376 stub LockWindowStation
0377 stub LockWindowUpdate
0378 stub LookupIconIdFromDirectory
0379 stub LookupIconIdFromDirectoryEx
0380 stub MBToWCSEx
0381 stub MapDialogRect
0382 stub MapVirtualKeyA
0383 stub MapVirtualKeyExA
0384 stub MapVirtualKeyW
0385 stub MapWindowPoints
0386 stub MenuItemFromPoint
0387 stub MenuWindowProcA
0388 stub MenuWindowProcW
0389 stdcall MessageBeep(long) MessageBeep
0390	stdcall MessageBoxA(long ptr ptr long)	MessageBox
0391 stub MessageBoxExA
0392 stub MessageBoxExW
0393 stub MessageBoxIndirectA
0394 stub MessageBoxIndirectW
0395 stub MessageBoxW
0396 stub ModifyMenuA
0397 stub ModifyMenuW
0398 stub MoveWindow
0399 stub MsgWaitForMultipleObjects
0400 stub OemKeyScan
0401 stub OemToCharA
0402 stub OemToCharBuffA
0403 stub OemToCharBuffW
0404 stub OemToCharW
0405 stub OffsetRect
0406 stub OpenClipboard
0407 stub OpenDesktopA
0408 stub OpenDesktopW
0409 stub OpenIcon
0410 stub OpenInputDesktop
0411 stub OpenWindowStationA
0412 stub OpenWindowStationW
0413 stub PackDDElParam
0414 stub PaintDesktop
0415 stub PeekMessageA
0416 stub PeekMessageW
0417 stub PlaySoundEvent
0418 stdcall PostMessageA(long long long long) PostMessage
0419 stub PostMessageW
0420 stdcall PostQuitMessage(long) PostQuitMessage
0421 stub PostThreadMessageA
0422 stub PostThreadMessageW
0423 stub PtInRect
0424 stub QuerySendMessage
0425 stub RedrawWindow
0426 stdcall RegisterClassA(ptr)	USER32_RegisterClassA
0427 stub RegisterClassExA
0428 stub RegisterClassExW
0429 stub RegisterClassW
0430 stub RegisterClipboardFormatA
0431 stub RegisterClipboardFormatW
0432 stub RegisterHotKey
0433 stub RegisterLogonProcess
0434 stub RegisterSystemThread
0435 stub RegisterTasklist
0436 stub RegisterWindowMessageA
0437 stub RegisterWindowMessageW
0438 stdcall ReleaseCapture() ReleaseCapture
0439 stdcall ReleaseDC(long long) ReleaseDC
0440 stub RemoveMenu
0441 stub RemovePropA
0442 stub RemovePropW
0443 stub ReplyMessage
0444 stub ResetDisplay
0445 stub ReuseDDElParam
0446 stub ScreenToClient
0447 stub ScrollChildren
0448 stub ScrollDC
0449 stub ScrollWindow
0450 stub ScrollWindowEx
0451 stub SendDlgItemMessageA
0452 stub SendDlgItemMessageW
0453 stdcall SendMessageA(long long long long) SendMessage
0454 stub SendMessageCallbackA
0455 stub SendMessageCallbackW
0456 stub SendMessageTimeoutA
0457 stub SendMessageTimeoutW
0458 stub SendMessageW
0459 stub SendNotifyMessageA
0460 stub SendNotifyMessageW
0461 stub ServerSetFunctionPointers
0462 stub SetActiveWindow
0463 stdcall SetCapture(long) SetCapture
0464 stub SetCaretBlinkTime
0465 stub SetCaretPos
0466 stub SetClassLongA
0467 stub SetClassLongW
0468 stub SetClassWord
0469 stub SetClipboardData
0470 stub SetClipboardViewer
0471 	stdcall SetCursor(long) SetCursor
0472 stub SetCursorContents
0473 stub SetCursorPos
0474 stub SetDebugErrorLevel
0475 stub SetDeskWallpaper
0476 stdcall SetDlgItemInt(long long long long) SetDlgItemInt
0477 stub SetDlgItemTextA
0478 stub SetDlgItemTextW
0479 stub SetDoubleClickTime
0480 stub SetFocus
0481 stub SetForegroundWindow
0482 stub SetInternalWindowPos
0483 stub SetKeyboardState
0484 stub SetLastErrorEx
0485 stub SetLogonNotifyWindow
0486 stub SetMenu
0487 stub SetMenuContextHelpId
0488 stub SetMenuDefaultItem
0489 stub SetMenuItemBitmaps
0490 stub SetMenuItemInfoA
0491 stub SetMenuItemInfoW
0492 stub SetMessageExtraInfo
0493 stdcall  SetMessageQueue(long) SetMessageQueue
0494 stub SetParent
0495 stub SetProcessWindowStation
0496 stub SetPropA
0497 stub SetPropW
0498 stub SetRect
0499 stub SetRectEmpty
0500 stub SetScrollInfo
0501 stub SetScrollPos
0502 stub SetScrollRange
0503 stub SetShellWindow
0504 stub SetSysColors
0505 stub SetSysColorsTemp
0506 stub SetSystemCursor
0507 stub SetSystemMenu
0508 stub SetSystemTimer
0509 stub SetThreadDesktop
0510 stdcall SetTimer(long long long long) USER32_SetTimer
0511 stub SetUserObjectInformationA
0512 stub SetUserObjectInformationW
0513 stub SetUserObjectSecurity
0514 stub SetWindowContextHelpId
0515 stub SetWindowFullScreenState
0516 stub SetWindowLongA
0517 stub SetWindowLongW
0518 stub SetWindowPlacement
0519 stub SetWindowPos
0520 stub SetWindowStationUser
0521 stdcall SetWindowTextA(long ptr)	SetWindowText
0522 stub SetWindowTextW
0523 stub SetWindowWord
0524 stub SetWindowsHookA
0525 stub SetWindowsHookExA
0526 stub SetWindowsHookExW
0527 stub SetWindowsHookW
0528 stub ShowCaret
0529 stdcall ShowCursor(long) ShowCursor
0530 stub ShowOwnedPopups
0531 stub ShowScrollBar
0532 stub ShowStartGlass
0533 stdcall ShowWindow(long long) ShowWindow
0534 stub ShowWindowAsync
0535 stub SubtractRect
0536 stub SwapMouseButton
0537 stub SwitchDesktop
0538 stub SwitchToThisWindow
0539 	stdcall SystemParametersInfoA(long long ptr long) SystemParametersInfo
0540 stub SystemParametersInfoW
0541 stub TabbedTextOutA
0542 stub TabbedTextOutW
0543 stub TileChildWindows
0544 stub TileWindows
0545 stub ToAscii
0546 stub ToAsciiEx
0547 stub ToUnicode
0548 stub TrackPopupMenu
0549 stub TrackPopupMenuEx
0550 stub TranslateAccelerator
0551 stdcall TranslateAcceleratorA(long long ptr) TranslateAccelerator
0552 stub TranslateAcceleratorW
0553 stub TranslateCharsetInfo
0554 stub TranslateMDISysAccel
0555 stdcall TranslateMessage(ptr) USER32_TranslateMessage
0556 stub UnhookWindowsHook
0557 stub UnhookWindowsHookEx
0558 stub UnionRect
0559 stub UnloadKeyboardLayout
0560 stub UnlockWindowStation
0561 stub UnpackDDElParam
0562 stub UnregisterClassA
0563 stub UnregisterClassW
0564 stub UnregisterHotKey
0565 stub UpdatePerUserSystemParameters
0566 stdcall UpdateWindow(long) UpdateWindow
0567 stub UserClientDllInitialize
0568 stub UserRealizePalette
0569 stub UserRegisterWowHandlers
0570 stub ValidateRect
0571 stub ValidateRgn
0572 stub VkKeyScanA
0573 stub VkKeyScanExA
0574 stub VkKeyScanExW
0575 stub VkKeyScanW
0576 stub WaitForInputIdle
0577 stub WaitMessage
0578 stdcall WinHelpA(long ptr long long)	WIN32_WinHelpA
0579 stub WinHelpW
0580 stub WindowFromDC
0581 stub WindowFromPoint
0582 stub keybd_event
0583 stub mouse_event
0584 stdcall wsprintfA() USER32_wsprintfA
0585 stub wsprintfW
0586 stub wvsprintfA
0587 stub wvsprintfW
#late additions
0588 stub ChangeDisplaySettingsA
0588 stub ChangeDisplaySettingsW
0588 stub EnumDesktopWindows
0588 stub EnumDisplaySettingsA
0588 stub EnumDisplaySettingsW
0588 stub GetWindowRgn
0588 stub MapVirtualKeyExW
0588 stub RegisterServicesProcess
0588 stub SetWindowRgn
0588 stub ToUnicodeEx

