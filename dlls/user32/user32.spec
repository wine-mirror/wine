1502 stub @
# 1503 stub GetPointerFrameArrivalTimes

1550 stub @
1551 stub @
1552 stub @
# 1553 stub DwmGetDxRgn
1554 stub @

2001 stub @
2002 stub @

2005 stub @

2010 stub @

# 2503 stub DelegateInput
# 2504 stub UndelegateInput
# 2505 stub HandleDelegatedInput
2506 stub @
2507 stub @
2508 stub @
2509 stub @
2510 stub @
2511 stub @
2512 stub @
2513 stub @
2514 stub @
2515 stub @
2516 stub @
2517 stub @
2518 stub @
2519 stub @
2520 stub @
# 2521 stub GetProcessUIContextInformation
2522 stub @
2523 stub @
2524 stub @
2525 stub @
2526 stub @
2527 stub @
# 2528 stub IsThreadMessageQueueAttached
2529 stub @
2530 stub @
2531 stub @
2532 stub @
2533 stub @
2534 stub @
2535 stub @
2536 stub @
2537 stub @
2538 stub @
2539 stub @
2540 stub @
2541 stub @
# 2542 stub SetCoveredWindowStates

2544 stub @
2545 stub @
2546 stdcall -noname RegisterTouchPadCapable(long) NtUserRegisterTouchPadCapable

2548 stub @
2549 stub @
2550 stub @
# 2551 stub ReportInertia
2552 stub @
2553 stub @
2554 stub @
2555 stub @
2556 stub @
2557 stub @
2558 stub @
2559 stub @
2560 stub @
2561 stdcall -noname EnableMouseInPointerForThread() NtUserEnableMouseInPointerForThread

2563 stub @
2564 stub @
2565 stub @
2566 stub @
2567 stub @
2568 stub @
2569 stub @
2570 stub @
# 2571 stub SetCoreWindow
2572 stub @
2573 stub @
2574 stub @
2575 stub @
2576 stub @

2578 stub @
2579 stub @

2581 stub @
2582 stub @

2584 stub @
2585 stub @
2586 stub @
2587 stub @
2588 stub @
2589 stub @
2590 stub @
2591 stub @
2592 stub @
2593 stub @
2594 stub @
2595 stub @

2597 stub @
2598 stub @
2599 stub @
2600 stub @

2606 stub @

2608 stub @
2609 stub @
2610 stub @
2611 stub @
2612 stub @
2613 stub @
2614 stub @
2615 stub @
2616 stub @
2617 stub @
2618 stub @
2619 stub @
2620 stub @
2621 stub @
2622 stub @

2627 stub @

2633 stub @
2634 stub @
2635 stub @
2636 stub @
2637 stub @
2638 stub @
2639 stub @
2640 stub @
2641 stub @
2642 stub @
2643 stub @
2644 stub @
2645 stub @

2647 stub @
2648 stub @
2649 stub @
2650 stub @
2651 stub @
2652 stub @
2653 stub @

2656 stub @
2657 stub @
2658 stub @
2659 stub @

2661 stub @
# 2662 stub RIMRegisterForInputEx
# 2663 stub RIMOnAsyncPnpWorkNotification
# 2664 stub ShellMigrateWindow
# 2665 stub SetAdditionalForegroundBoostProcesses
# 2666 stub RegisterForTooltipDismissNotification
# 2667 stub RegisterForCustomDockTargets
# 2668 stub GetClipboardMetadata
2669 stub @
2670 stub @
# 2671 stub ShellRegisterHotKey
# 2672 stub SetUserObjectCapability
# 2673 stub SetWindowMessageCapability
# 2674 stub ShellForegroundBoostProcess
# 2675 stub SuppressWindowActions
# 2676 stub GetSuppressedWindowActions

# 2680 stub DwmWindowNotificationsEnabled
# 2681 stub ApplyWindowAction
# 2682 stub RegisterCloakedNotification
# 2683 stub GetCurrentMonitorTopologyId
# 2684 stub SuppressWindowDisplayChange
# 2685 stub IsWindowDisplayChangeSuppressed
# 2686 stub InternalStartMoveSize
# 2687 stub RaiseLowerShellWindow
# 2688 stub RegisterPrecisionTouchpadThread
# 2689 stub RegisterPrecisionTouchpadWindow
# 2690 stub CreateSyntheticPointerDevice2
# 2691 stub GetPointerTouchpadInfo
# 2692 stub GetPointerTouchpadInfoHistory
# 2693 stub GetPointerFrameTouchpadInfo
# 2694 stub GetPointerFrameTouchpadInfoHistory
# 2695 stub ReportPointerDeviceInertia

2700 stub @

2703 stub @
2704 stub @
2705 stub @
2706 stub @
2707 stub @
2708 stub @
2709 stub @
2710 stub @
2711 stub @
2712 stub @
2713 stub @
2714 stub @
2715 stub @
2716 stub @

# 2800 stub ShellHandwritingDelegateInput
# 2801 stub ShellHandwritingUndelegateInput
# 2802 stub ShellHandwritingHandleDelegatedInput
# 2804 stub EnableWindowShellWindowManagementBehavior

@ stdcall ActivateKeyboardLayout(long long) NtUserActivateKeyboardLayout
@ stdcall AddClipboardFormatListener(long) NtUserAddClipboardFormatListener
# @ stub AddVisualIdentifier
@ stdcall AdjustWindowRect(ptr long long)
@ stdcall AdjustWindowRectEx(ptr long long long)
@ stdcall AdjustWindowRectExForDpi(ptr long long long long)
@ stdcall AlignRects(ptr long long long)
# @ stub AllowForegroundActivation
@ stdcall AllowSetForegroundWindow(long)
@ stdcall AnimateWindow(long long long)
@ stdcall AnyPopup()
@ stdcall AppendMenuA(long long long ptr)
@ stdcall AppendMenuW(long long long ptr)
@ stdcall AreDpiAwarenessContextsEqual(long long)
@ stdcall ArrangeIconicWindows(long) NtUserArrangeIconicWindows
@ stdcall AttachThreadInput(long long long) NtUserAttachThreadInput
@ stdcall BeginDeferWindowPos(long)
@ stdcall BeginPaint(long ptr) NtUserBeginPaint
@ stdcall BlockInput(long)
@ stdcall BringWindowToTop(long)
@ stdcall BroadcastSystemMessage(long ptr long long long) BroadcastSystemMessageA
@ stdcall BroadcastSystemMessageA(long ptr long long long)
@ stdcall BroadcastSystemMessageExA(long ptr long long long ptr)
@ stdcall BroadcastSystemMessageExW(long ptr long long long ptr)
@ stdcall BroadcastSystemMessageW(long ptr long long long)
# @ stub BuildReasonArray
@ stdcall CalcChildScroll(long long)
@ stdcall CalcMenuBar(long long long long ptr) CalcMenuBar
# @ stub CalculatePopupWindowPosition
@ stdcall CallMsgFilter(ptr long) NtUserCallMsgFilter
@ stdcall CallMsgFilterA(ptr long) NtUserCallMsgFilter
@ stdcall CallMsgFilterW(ptr long) NtUserCallMsgFilter
@ stdcall CallNextHookEx(long long long long) NtUserCallNextHookEx
@ stdcall CallWindowProcA(ptr long long long long)
@ stdcall CallWindowProcW(ptr long long long long)
# @ stub CancelShutdown
@ stdcall CascadeChildWindows(long long)
@ stdcall CascadeWindows(long long ptr long ptr)
@ stdcall ChangeClipboardChain(long long) NtUserChangeClipboardChain
@ stdcall ChangeDisplaySettingsA(ptr long)
@ stdcall ChangeDisplaySettingsExA(str ptr long long ptr)
@ stdcall ChangeDisplaySettingsExW(wstr ptr long long ptr)
@ stdcall ChangeDisplaySettingsW(ptr long)
@ stdcall ChangeMenuA(long long ptr long long)
@ stdcall ChangeMenuW(long long ptr long long)
@ stdcall ChangeWindowMessageFilter(long long)
@ stdcall ChangeWindowMessageFilterEx(long long long ptr)
@ stdcall -import CharLowerA(str)
@ stdcall -import CharLowerBuffA(str long)
@ stdcall -import CharLowerBuffW(wstr long)
@ stdcall -import CharLowerW(wstr)
@ stdcall -import CharNextA(str)
@ stdcall -import CharNextExA(long str long)
@ stdcall CharNextExW(long wstr long)
@ stdcall -import CharNextW(wstr)
@ stdcall -import CharPrevA(str str)
@ stdcall -import CharPrevExA(long str str long)
@ stdcall CharPrevExW(long wstr wstr long)
@ stdcall -import CharPrevW(wstr wstr)
@ stdcall CharToOemA(str ptr)
@ stdcall CharToOemBuffA(str ptr long)
@ stdcall CharToOemBuffW(wstr ptr long)
@ stdcall CharToOemW(wstr ptr)
@ stdcall -import CharUpperA(str)
@ stdcall -import CharUpperBuffA(str long)
@ stdcall -import CharUpperBuffW(wstr long)
@ stdcall -import CharUpperW(wstr)
# @ stub CheckBannedOneCoreTransformApi
# @ stub CheckDBCSEnabledExt
@ stdcall CheckDlgButton(long long long)
@ stdcall CheckMenuItem(long long long) NtUserCheckMenuItem
@ stdcall CheckMenuRadioItem(long long long long long)
# @ stub CheckProcessForClipboardAccess
# @ stub CheckProcessSession
@ stdcall CheckRadioButton(long long long long)
# @ stub CheckWindowThreadDesktop
@ stdcall ChildWindowFromPoint(long int64)
@ stdcall ChildWindowFromPointEx(long int64 long)
@ stub CliImmSetHotKey
@ stub ClientThreadConnect
@ stub ClientThreadSetup
@ stdcall ClientToScreen(long ptr)
@ stdcall -import ClipCursor(ptr) NtUserClipCursor
@ stdcall CloseClipboard() NtUserCloseClipboard
@ stdcall CloseDesktop(long) NtUserCloseDesktop
@ stdcall CloseGestureInfoHandle(long)
@ stdcall CloseTouchInputHandle(long)
@ stdcall CloseWindow(long)
@ stdcall CloseWindowStation(long) NtUserCloseWindowStation
# @ stub ConsoleControl
# @ stub ControlMagnification
@ stdcall CopyAcceleratorTableA(long ptr long)
@ stdcall CopyAcceleratorTableW(long ptr long) NtUserCopyAcceleratorTable
@ stdcall CopyIcon(long)
@ stdcall CopyImage(long long long long long)
@ stdcall CopyRect(ptr ptr)
@ stdcall CountClipboardFormats() NtUserCountClipboardFormats
@ stdcall CreateAcceleratorTableA(ptr long)
@ stdcall CreateAcceleratorTableW(ptr long) NtUserCreateAcceleratorTable
@ stdcall CreateCaret(long long long long) NtUserCreateCaret
@ stdcall CreateCursor(long long long long long ptr ptr)
# @ stub CreateDCompositionHwndTarget
@ stdcall CreateDesktopA(str str ptr long long ptr)
# @ stub CreateDesktopExA
# @ stub CreateDesktopExW
@ stdcall CreateDesktopW(wstr wstr ptr long long ptr)
@ stdcall CreateDialogIndirectParamA(long ptr long ptr long)
@ stdcall CreateDialogIndirectParamAorW(long ptr long ptr long long)
@ stdcall CreateDialogIndirectParamW(long ptr long ptr long)
@ stdcall CreateDialogParamA(long str long ptr long)
@ stdcall CreateDialogParamW(long wstr long ptr long)
@ stdcall CreateIcon(long long long long long ptr ptr)
@ stdcall CreateIconFromResource(ptr long long long)
@ stdcall CreateIconFromResourceEx(ptr long long long long long long)
@ stdcall CreateIconIndirect(ptr)
@ stdcall CreateMDIWindowA(str str long long long long long long long long)
@ stdcall CreateMDIWindowW(wstr wstr long long long long long long long long)
@ stdcall CreateMenu() NtUserCreateMenu
@ stdcall CreatePopupMenu() NtUserCreatePopupMenu
@ stdcall CreateSyntheticPointerDevice(long long long)
# @ stub CreateSystemThreads
@ stdcall CreateWindowExA(long str str long long long long long long long long ptr)
@ stdcall CreateWindowExW(long wstr wstr long long long long long long long long ptr)
# @ stub CreateWindowInBand
# @ stub CreateWindowInBandEx
# @ stub CreateWindowIndirect
@ stdcall CreateWindowStationA(str long long ptr)
@ stdcall CreateWindowStationW(wstr long long ptr)
# @ stub CsrBroadcastSystemMessageExW
# @ stub CtxInitUser32
@ stdcall DdeAbandonTransaction(long long long)
@ stdcall DdeAccessData(long ptr)
@ stdcall DdeAddData(long ptr long long)
@ stdcall DdeClientTransaction(ptr long long long long long long ptr)
@ stdcall DdeCmpStringHandles(long long)
@ stdcall DdeConnect(long long long ptr)
@ stdcall DdeConnectList(long long long long ptr)
@ stdcall DdeCreateDataHandle(long ptr long long long long long)
@ stdcall DdeCreateStringHandleA(long str long)
@ stdcall DdeCreateStringHandleW(long wstr long)
@ stdcall DdeDisconnect(long)
@ stdcall DdeDisconnectList(long)
@ stdcall DdeEnableCallback(long long long)
@ stdcall DdeFreeDataHandle(long)
@ stdcall DdeFreeStringHandle(long long)
@ stdcall DdeGetData(long ptr long long)
@ stdcall DdeGetLastError(long)
@ stub DdeGetQualityOfService
@ stdcall DdeImpersonateClient(long)
@ stdcall DdeInitializeA(ptr ptr long long)
@ stdcall DdeInitializeW(ptr ptr long long)
@ stdcall DdeKeepStringHandle(long long)
@ stdcall DdeNameService(long long long long)
@ stdcall DdePostAdvise(long long long)
@ stdcall DdeQueryConvInfo(long long ptr)
@ stdcall DdeQueryNextServer(long long)
@ stdcall DdeQueryStringA(long long ptr long long)
@ stdcall DdeQueryStringW(long long ptr long long)
@ stdcall DdeReconnect(long)
@ stdcall DdeSetQualityOfService(long ptr ptr)
@ stdcall DdeSetUserHandle(long long long)
@ stdcall DdeUnaccessData(long)
@ stdcall DdeUninitialize(long)
@ stdcall DefDlgProcA(long long long long) NTDLL.NtdllDialogWndProc_A
@ stdcall DefDlgProcW(long long long long) NTDLL.NtdllDialogWndProc_W
@ stdcall DefFrameProcA(long long long long long)
@ stdcall DefFrameProcW(long long long long long)
@ stdcall DefMDIChildProcA(long long long long)
@ stdcall DefMDIChildProcW(long long long long)
@ stdcall DefRawInputProc(ptr long long)
@ stdcall DefWindowProcA(long long long long) NTDLL.NtdllDefWindowProc_A
@ stdcall DefWindowProcW(long long long long) NTDLL.NtdllDefWindowProc_W
@ stdcall DeferWindowPos(long long long long long long long long)
# @ stub DeferWindowPosAndBand
@ stdcall DeleteMenu(long long long) NtUserDeleteMenu
@ stdcall DeregisterShellHookWindow(long)
@ stdcall DestroyAcceleratorTable(long) NtUserDestroyAcceleratorTable
@ stdcall DestroyCaret() NtUserDestroyCaret
@ stdcall DestroyCursor(long)
# @ stub DestroyDCompositionHwndTarget
@ stdcall DestroyIcon(long)
@ stdcall DestroyMenu(long) NtUserDestroyMenu
# @ stub DestroyReasons
# @ stub DestroySyntheticPointerDevice
@ stdcall DestroyWindow(long) NtUserDestroyWindow
@ stdcall DialogBoxIndirectParamA(long ptr long ptr long)
@ stdcall DialogBoxIndirectParamAorW(long ptr long ptr long long)
@ stdcall DialogBoxIndirectParamW(long ptr long ptr long)
@ stdcall DialogBoxParamA(long str long ptr long)
@ stdcall DialogBoxParamW(long wstr long ptr long)
@ stdcall DisableProcessWindowsGhosting()
@ stdcall DispatchMessageA(ptr)
@ stdcall DispatchMessageW(ptr)
@ stdcall DisplayConfigGetDeviceInfo(ptr)
@ stdcall DisplayConfigSetDeviceInfo(ptr)
# @ stub DisplayExitWindowsWarnings
@ stdcall DlgDirListA(long str long long long)
@ stdcall DlgDirListComboBoxA(long ptr long long long)
@ stdcall DlgDirListComboBoxW(long ptr long long long)
@ stdcall DlgDirListW(long wstr long long long)
@ stdcall DlgDirSelectComboBoxExA(long ptr long long)
@ stdcall DlgDirSelectComboBoxExW(long ptr long long)
@ stdcall DlgDirSelectExA(long ptr long long)
@ stdcall DlgDirSelectExW(long ptr long long)
# @ stub DoSoundConnect
# @ stub DoSoundDisconnect
@ stdcall DragDetect(long int64)
@ stdcall DragObject(long long long long long) NtUserDragObject
@ stdcall DrawAnimatedRects(long long ptr ptr)
@ stdcall DrawCaption(long long ptr long)
@ stdcall DrawCaptionTempA(long long ptr long long str long)
@ stdcall DrawCaptionTempW(long long ptr long long wstr long) NtUserDrawCaptionTemp
@ stdcall DrawEdge(long ptr long long)
@ stdcall DrawFocusRect(long ptr)
@ stub DrawFrame
@ stdcall DrawFrameControl(long ptr long long)
@ stdcall DrawIcon(long long long long)
@ stdcall DrawIconEx(long long long long long long long long long) NtUserDrawIconEx
@ stdcall DrawMenuBar(long) NtUserDrawMenuBar
@ stdcall DrawMenuBarTemp(long long ptr long long) NtUserDrawMenuBarTemp
@ stdcall DrawStateA(long long ptr long long long long long long long)
@ stdcall DrawStateW(long long ptr long long long long long long long)
@ stdcall DrawTextA(long str long ptr long)
@ stdcall DrawTextExA(long str long ptr long ptr)
@ stdcall DrawTextExW(long wstr long ptr long ptr)
@ stdcall DrawTextW(long wstr long ptr long)
# @ stub DwmGetDxSharedSurface
# @ stub DwmGetRemoteSessionOcclusionEvent
# @ stub DwmGetRemoteSessionOcclusionState
# @ stub DwmKernelShutdown
# @ stub DwmKernelStartup
# @ stub DwmLockScreenUpdates
# @ stub DwmValidateWindow
@ stdcall EditWndProc(long long long long) EditWndProcA
@ stdcall EmptyClipboard() NtUserEmptyClipboard
@ stdcall EnableMenuItem(long long long) NtUserEnableMenuItem
@ stdcall EnableMouseInPointer(long) NtUserEnableMouseInPointer
@ stdcall EnableNonClientDpiScaling(long)
# @ stub EnableOneCoreTransformMode
@ stdcall -import EnableScrollBar(long long long) NtUserEnableScrollBar
# @ stub EnableSessionForMMCSS
@ stdcall EnableWindow(long long) NtUserEnableWindow
@ stdcall EndDeferWindowPos(long)
# @ stub EndDeferWindowPosEx
@ stdcall EndDialog(long long)
@ stdcall EndMenu() NtUserEndMenu
@ stdcall EndPaint(long ptr) NtUserEndPaint
@ stub EndTask
# @ stub EnterReaderModeHelper
@ stdcall EnumChildWindows(long ptr long)
@ stdcall EnumClipboardFormats(long) NtUserEnumClipboardFormats
@ stdcall EnumDesktopWindows(long ptr ptr)
@ stdcall EnumDesktopsA(ptr ptr long)
@ stdcall EnumDesktopsW(ptr ptr long)
@ stub EnumDisplayDeviceModesA
@ stub EnumDisplayDeviceModesW
@ stdcall EnumDisplayDevicesA(str long ptr long)
@ stdcall EnumDisplayDevicesW(wstr long ptr long)
@ stdcall EnumDisplayMonitors(long ptr ptr long) NtUserEnumDisplayMonitors
@ stdcall EnumDisplaySettingsA(str long ptr)
@ stdcall EnumDisplaySettingsExA(str long ptr long)
@ stdcall EnumDisplaySettingsExW(wstr long ptr long)
@ stdcall EnumDisplaySettingsW(wstr long ptr )
@ stdcall EnumPropsA(long ptr)
@ stdcall EnumPropsExA(long ptr long)
@ stdcall EnumPropsExW(long ptr long)
@ stdcall EnumPropsW(long ptr)
@ stdcall EnumThreadWindows(long ptr long)
@ stdcall EnumWindowStationsA(ptr long)
@ stdcall EnumWindowStationsW(ptr long)
@ stdcall EnumWindows(ptr long)
@ stdcall EqualRect(ptr ptr)
# @ stub EvaluateProximityToPolygon
@ stdcall EvaluateProximityToRect(ptr ptr ptr)
@ stdcall ExcludeUpdateRgn(long long) NtUserExcludeUpdateRgn
@ stdcall ExitWindowsEx(long long)
@ stdcall FillRect(long ptr long)
@ stdcall FindWindowA(str str)
@ stdcall FindWindowExA(long long str str)
@ stdcall FindWindowExW(long long wstr wstr)
@ stdcall FindWindowW(wstr wstr)
@ stdcall FlashWindow(long long)
@ stdcall FlashWindowEx(ptr) NtUserFlashWindowEx
@ stdcall FrameRect(long ptr long)
@ stdcall FreeDDElParam(long long)
# @ stub FrostCrashedWindow
@ stdcall GetActiveWindow()
@ stdcall GetAltTabInfo(long long ptr ptr long) GetAltTabInfoA
@ stdcall GetAltTabInfoA(long long ptr ptr long)
@ stdcall GetAltTabInfoW(long long ptr ptr long)
@ stdcall GetAncestor(long long) NtUserGetAncestor
@ stdcall GetAppCompatFlags(long)
@ stdcall GetAppCompatFlags2(long)
@ stdcall -import GetAsyncKeyState(long) NtUserGetAsyncKeyState
@ stdcall GetAutoRotationState(ptr)
@ stdcall GetAwarenessFromDpiAwarenessContext(long)
# @ stub GetCIMSSM
@ stdcall GetCapture()
@ stdcall GetCaretBlinkTime() NtUserGetCaretBlinkTime
@ stdcall GetCaretPos(ptr) NtUserGetCaretPos
@ stdcall GetClassInfoA(long str ptr)
@ stdcall GetClassInfoExA(long str ptr)
@ stdcall GetClassInfoExW(long wstr ptr)
@ stdcall GetClassInfoW(long wstr ptr)
@ stdcall GetClassLongA(long long)
@ stdcall -arch=win64 GetClassLongPtrA(long long)
@ stdcall -arch=win64 GetClassLongPtrW(long long)
@ stdcall GetClassLongW(long long)
@ stdcall GetClassNameA(long ptr long)
@ stdcall GetClassNameW(long ptr long)
@ stdcall GetClassWord(long long)
@ stdcall GetClientRect(long long)
@ stdcall GetClipCursor(ptr) NtUserGetClipCursor
# @ stub GetClipboardAccessToken
@ stdcall GetClipboardData(long)
@ stdcall GetClipboardFormatNameA(long ptr long)
@ stdcall GetClipboardFormatNameW(long ptr long) NtUserGetClipboardFormatName
@ stdcall GetClipboardOwner() NtUserGetClipboardOwner
@ stdcall GetClipboardSequenceNumber() NtUserGetClipboardSequenceNumber
@ stdcall GetClipboardViewer() NtUserGetClipboardViewer
@ stdcall GetComboBoxInfo(long ptr)
@ stdcall GetCurrentInputMessageSource(ptr) NtUserGetCurrentInputMessageSource
@ stdcall GetCursor() NtUserGetCursor
@ stdcall GetCursorFrameInfo(long long long ptr ptr)
@ stdcall GetCursorInfo(ptr) NtUserGetCursorInfo
@ stdcall GetCursorPos(ptr)
@ stdcall GetDC(long) NtUserGetDC
@ stdcall GetDCEx(long long long) NtUserGetDCEx
# @ stub GetDCompositionHwndBitmap
# @ stub GetDesktopID
@ stdcall GetDesktopWindow()
@ stdcall GetDialogBaseUnits()
# @ stub GetDialogControlDpiChangeBehavior
# @ stub GetDialogDpiChangeBehavior
@ stdcall GetDisplayAutoRotationPreferences(ptr)
@ stdcall GetDisplayConfigBufferSizes(long ptr ptr) NtUserGetDisplayConfigBufferSizes
@ stdcall GetDlgCtrlID(long)
@ stdcall GetDlgItem(long long)
@ stdcall GetDlgItemInt(long long ptr long)
@ stdcall GetDlgItemTextA(long long ptr long)
@ stdcall GetDlgItemTextW(long long ptr long)
@ stdcall GetDoubleClickTime() NtUserGetDoubleClickTime
@ stdcall GetDpiAwarenessContextForProcess(ptr)
@ stdcall GetDpiForMonitorInternal(long long ptr ptr) NtUserGetDpiForMonitor
@ stdcall GetDpiForSystem()
@ stdcall GetDpiForWindow(long)
@ stdcall GetDpiFromDpiAwarenessContext(long)
# @ stub GetExtendedPointerDeviceProperty
@ stdcall GetFocus()
@ stdcall GetForegroundWindow() NtUserGetForegroundWindow
@ stdcall GetGUIThreadInfo(long ptr) NtUserGetGUIThreadInfo
@ stdcall GetGestureConfig(long long long ptr ptr long)
@ stdcall GetGestureExtraArgs(long long ptr)
@ stdcall GetGestureInfo(long ptr)
@ stdcall GetGuiResources(long long)
@ stdcall GetIconInfo(long ptr)
@ stdcall GetIconInfoExA(long ptr)
@ stdcall GetIconInfoExW(long ptr)
@ stub GetInputDesktop
# @ stub GetInputLocaleInfo
@ stdcall GetInputState()
@ stdcall GetInternalWindowPos(long ptr ptr) NtUserGetInternalWindowPos
@ stdcall GetKBCodePage()
@ stdcall GetKeyNameTextA(long ptr long)
@ stdcall GetKeyNameTextW(long ptr long) NtUserGetKeyNameText
@ stdcall -import GetKeyState(long) NtUserGetKeyState
@ stdcall GetKeyboardLayout(long) NtUserGetKeyboardLayout
@ stdcall GetKeyboardLayoutList(long ptr) NtUserGetKeyboardLayoutList
@ stdcall GetKeyboardLayoutNameA(ptr)
@ stdcall GetKeyboardLayoutNameW(ptr) NtUserGetKeyboardLayoutName
@ stdcall -import GetKeyboardState(ptr) NtUserGetKeyboardState
@ stdcall GetKeyboardType(long)
@ stdcall GetLastActivePopup(long)
@ stdcall GetLastInputInfo(ptr)
@ stdcall GetLayeredWindowAttributes(long ptr ptr ptr) NtUserGetLayeredWindowAttributes
@ stdcall GetListBoxInfo(long)
# @ stub GetMagnificationDesktopColorEffect
# @ stub GetMagnificationDesktopMagnification
# @ stub GetMagnificationDesktopSamplingMode
# @ stub GetMagnificationLensCtxInformation
@ stdcall GetMenu(long)
@ stdcall GetMenuBarInfo(long long long ptr) NtUserGetMenuBarInfo
@ stdcall GetMenuCheckMarkDimensions()
@ stdcall GetMenuContextHelpId(long)
@ stdcall GetMenuDefaultItem(long long long)
@ stub GetMenuIndex
@ stdcall GetMenuInfo(long ptr)
@ stdcall GetMenuItemCount(long)
@ stdcall GetMenuItemID(long long)
@ stdcall GetMenuItemInfoA(long long long ptr)
@ stdcall GetMenuItemInfoW(long long long ptr)
@ stdcall GetMenuItemRect(long long long ptr) NtUserGetMenuItemRect
@ stdcall GetMenuState(long long long)
@ stdcall GetMenuStringA(long long ptr long long)
@ stdcall GetMenuStringW(long long ptr long long)
@ stdcall GetMessageA(ptr long long long)
@ stdcall GetMessageExtraInfo()
@ stdcall GetMessagePos()
@ stdcall GetMessageTime()
@ stdcall GetMessageW(ptr long long long)
@ stdcall GetMonitorInfoA(long ptr)
@ stdcall GetMonitorInfoW(long ptr)
@ stdcall GetMouseMovePointsEx(long ptr ptr long long) NtUserGetMouseMovePointsEx
@ stdcall GetNextDlgGroupItem(long long long)
@ stdcall GetNextDlgTabItem(long long long)
@ stdcall GetOpenClipboardWindow() NtUserGetOpenClipboardWindow
@ stdcall GetParent(long)
@ stdcall GetPhysicalCursorPos(ptr)
# @ stub GetPointerCursorId
# @ stub GetPointerDevice
# @ stub GetPointerDeviceCursors
# @ stub GetPointerDeviceInputSpace
# @ stub GetPointerDeviceOrientation
# @ stub GetPointerDeviceProperties
# @ stub GetPointerDeviceRects
@ stdcall GetPointerDevices(ptr ptr)
# @ stub GetPointerFrameInfo
# @ stub GetPointerFrameInfoHistory
# @ stub GetPointerFramePenInfo
# @ stub GetPointerFramePenInfoHistory
# @ stub GetPointerFrameTimes
# @ stub GetPointerFrameTouchInfo
# @ stub GetPointerFrameTouchInfoHistory
@ stdcall GetPointerInfo(long ptr)
# @ stub GetPointerInfoHistory
# @ stub GetPointerInputTransform
# @ stub GetPointerPenInfo
# @ stub GetPointerPenInfoHistory
@ stdcall GetPointerTouchInfo(long ptr)
@ stdcall GetPointerTouchInfoHistory(long ptr ptr)
@ stdcall GetPointerType(long ptr)
@ stdcall GetPriorityClipboardFormat(ptr long) NtUserGetPriorityClipboardFormat
@ stdcall GetProcessDefaultLayout(ptr)
@ stdcall GetProcessDpiAwarenessInternal(long ptr)
@ stdcall GetProcessWindowStation() NtUserGetProcessWindowStation
@ stdcall GetProgmanWindow()
@ stdcall GetPropA(long str)
@ stdcall GetPropW(long wstr)
@ stdcall GetQueueStatus(long) NtUserGetQueueStatus
@ stdcall GetRawInputBuffer(ptr ptr long) NtUserGetRawInputBuffer
@ stdcall GetRawInputData(ptr long ptr ptr long) NtUserGetRawInputData
@ stdcall GetRawInputDeviceInfoA(ptr long ptr ptr)
@ stdcall GetRawInputDeviceInfoW(ptr long ptr ptr) NtUserGetRawInputDeviceInfo
@ stdcall GetRawInputDeviceList(ptr ptr long) NtUserGetRawInputDeviceList
# @ stub GetRawPointerDeviceData
# @ stub GetReasonTitleFromReasonCode
@ stdcall GetRegisteredRawInputDevices(ptr ptr long) NtUserGetRegisteredRawInputDevices
@ stdcall GetScrollBarInfo(long long ptr) NtUserGetScrollBarInfo
@ stdcall GetScrollInfo(long long ptr)
@ stdcall GetScrollPos(long long)
@ stdcall GetScrollRange(long long ptr ptr)
# @ stub GetSendMessageReceiver
# @ stub GetShellChangeNotifyWindow
@ stdcall GetShellWindow()
@ stdcall GetSubMenu(long long)
@ stdcall GetSysColor(long)
@ stdcall GetSysColorBrush(long)
@ stdcall GetSystemDpiForProcess(long) NtUserGetSystemDpiForProcess
@ stdcall GetSystemMenu(long long) NtUserGetSystemMenu
@ stdcall GetSystemMetrics(long)
@ stdcall GetSystemMetricsForDpi(long long)
@ stdcall GetTabbedTextExtentA(long str long long ptr)
@ stdcall GetTabbedTextExtentW(long wstr long long ptr)
@ stdcall GetTaskmanWindow()
@ stdcall GetThreadDesktop(long) NtUserGetThreadDesktop
@ stdcall GetThreadDpiAwarenessContext()
@ stdcall GetThreadDpiHostingBehavior()
@ stdcall GetTitleBarInfo(long ptr) NtUserGetTitleBarInfo
# @ stub GetTopLevelWindow
@ stdcall GetTopWindow(long)
@ stdcall GetTouchInputInfo(long long ptr long)
# @ stub GetUnpredictedMessagePos
@ stdcall GetUpdateRect(long ptr long) NtUserGetUpdateRect
@ stdcall GetUpdateRgn(long long long) NtUserGetUpdateRgn
@ stdcall GetUpdatedClipboardFormats(ptr long ptr) NtUserGetUpdatedClipboardFormats
@ stdcall GetUserObjectInformationA(long long ptr long ptr)
@ stdcall GetUserObjectInformationW(long long ptr long ptr) NtUserGetObjectInformation
@ stdcall GetUserObjectSecurity(long ptr ptr long ptr)
# @ stub GetWinStationInfo
@ stdcall GetWindow(long long)
# @ stub GetWindowBand
# @ stub GetWindowCompositionAttribute
# @ stub GetWindowCompositionInfo
@ stdcall GetWindowContextHelpId(long) NtUserGetWindowContextHelpId
@ stdcall GetWindowDC(long) NtUserGetWindowDC
@ stdcall GetWindowDisplayAffinity(long ptr)
@ stdcall GetWindowDpiAwarenessContext(long)
@ stdcall GetWindowDpiHostingBehavior(long)
# @ stub GetWindowFeedbackSetting
@ stdcall GetWindowInfo(long ptr)
@ stdcall GetWindowLongA(long long)
@ stdcall -arch=win64 GetWindowLongPtrA(long long)
@ stdcall -arch=win64 GetWindowLongPtrW(long long)
@ stdcall GetWindowLongW(long long)
# @ stub GetWindowMinimizeRect
@ stdcall GetWindowModuleFileName(long ptr long) GetWindowModuleFileNameA
@ stdcall GetWindowModuleFileNameA(long ptr long)
@ stdcall GetWindowModuleFileNameW(long ptr long)
@ stdcall GetWindowPlacement(long ptr) NtUserGetWindowPlacement
# @ stub GetWindowProcessHandle
@ stdcall GetWindowRect(long ptr)
@ stdcall GetWindowRgn(long long)
@ stdcall GetWindowRgnBox(long ptr)
# @ stub GetWindowRgnEx
@ stdcall GetWindowTextA(long ptr long)
@ stdcall GetWindowTextLengthA(long)
@ stdcall GetWindowTextLengthW(long)
@ stdcall GetWindowTextW(long ptr long)
@ stdcall GetWindowThreadProcessId(long ptr)
@ stdcall GetWindowWord(long long)
# @ stub GhostWindowFromHungWindow
@ stdcall GrayStringA(long long ptr long long long long long long)
@ stdcall GrayStringW(long long ptr long long long long long long)
@ stdcall HideCaret(long) NtUserHideCaret
@ stdcall HiliteMenuItem(long long long long) NtUserHiliteMenuItem
# @ stub HungWindowFromGhostWindow
# @ stub IMPGetIMEA
# @ stub IMPGetIMEW
# @ stub IMPQueryIMEA
# @ stub IMPQueryIMEW
# @ stub IMPSetIMEA
# @ stub IMPSetIMEW
@ stdcall ImpersonateDdeClientWindow(long long)
@ stdcall InSendMessage()
@ stdcall InSendMessageEx(ptr)
@ stdcall InflateRect(ptr long long)
# @ stub InheritWindowMonitor
# @ stub InitDManipHook
# @ stub InitializeGenericHidInjection
# @ stub InitializeInputDeviceInjection
# @ stub InitializeLpkHooks
# @ stub InitializePointerDeviceInjection
# @ stub InitializePointerDeviceInjectionEx
# @ stub InitializeTouchInjection
# @ stub InjectDeviceInput
# @ stub InjectGenericHidInput
# @ stub InjectKeyboardInput
# @ stub InjectMouseInput
# @ stub InjectPointerInput
# @ stub InjectSyntheticPointerInput
# @ stub InjectTouchInput
# @ stub InputSpaceRegionFromPoint
@ stdcall InsertMenuA(long long long long ptr)
@ stdcall InsertMenuItemA(long long long ptr)
@ stdcall InsertMenuItemW(long long long ptr)
@ stdcall InsertMenuW(long long long long ptr)
@ stdcall InternalGetWindowIcon(ptr long) NtUserInternalGetWindowIcon
@ stdcall InternalGetWindowText(long ptr long) NtUserInternalGetWindowText
@ stdcall IntersectRect(ptr ptr ptr)
@ stdcall InvalidateRect(long ptr long) NtUserInvalidateRect
@ stdcall InvalidateRgn(long long long) NtUserInvalidateRgn
@ stdcall InvertRect(long ptr)
@ stdcall -import IsCharAlphaA(long)
@ stdcall -import IsCharAlphaNumericA(long)
@ stdcall -import IsCharAlphaNumericW(long)
@ stdcall -import IsCharAlphaW(long)
@ stdcall -import IsCharLowerA(long)
@ stdcall -import IsCharLowerW(long)
@ stdcall -import IsCharUpperA(long)
@ stdcall -import IsCharUpperW(long)
@ stdcall IsChild(long long)
@ stdcall IsClipboardFormatAvailable(long) NtUserIsClipboardFormatAvailable
@ stdcall IsDialogMessage(long ptr) IsDialogMessageA
@ stdcall IsDialogMessageA(long ptr)
@ stdcall IsDialogMessageW(long ptr)
@ stdcall IsDlgButtonChecked(long long)
@ stdcall IsGUIThread(long)
@ stdcall IsHungAppWindow(long)
@ stdcall IsIconic(long)
# @ stub IsImmersiveProcess
# @ stub IsInDesktopWindowBand
@ stdcall IsMenu(long)
@ stdcall IsMouseInPointerEnabled() NtUserIsMouseInPointerEnabled
# @ stub SetThreadCursorCreationScaling
# @ stub IsOneCoreTransformMode
@ stdcall IsProcessDPIAware()
@ stdcall IsRectEmpty(ptr)
# @ stub IsSETEnabled
# @ stub IsServerSideWindow
# @ stub IsThreadDesktopComposited
# @ stub IsThreadTSFEventAware
# @ stub IsTopLevelWindow
@ stdcall IsTouchWindow(long ptr)
@ stdcall IsValidDpiAwarenessContext(long)
@ stdcall IsWinEventHookInstalled(long)
@ stdcall IsWindow(long)
# @ stub IsWindowArranged
@ stdcall IsWindowEnabled(long)
# @ stub IsWindowInDestroy
@ stdcall IsWindowRedirectedForPrint(long)
@ stdcall IsWindowUnicode(long)
@ stdcall IsWindowVisible(long)
# @ stub IsWow64Message
@ stdcall IsZoomed(long)
@ stdcall KillSystemTimer(long long)
@ stdcall KillTimer(long long) NtUserKillTimer
@ stdcall LoadAcceleratorsA(long str)
@ stdcall LoadAcceleratorsW(long wstr)
@ stdcall LoadBitmapA(long str)
@ stdcall LoadBitmapW(long wstr)
@ stdcall LoadCursorA(long str)
@ stdcall LoadCursorFromFileA(str)
@ stdcall LoadCursorFromFileW(wstr)
@ stdcall LoadCursorW(long wstr)
@ stdcall LoadIconA(long str)
@ stdcall LoadIconW(long wstr)
@ stdcall LoadImageA(long str long long long long)
@ stdcall LoadImageW(long wstr long long long long)
@ stdcall LoadKeyboardLayoutA(str long)
@ stdcall LoadKeyboardLayoutEx(long wstr long)
@ stdcall LoadKeyboardLayoutW(wstr long)
@ stdcall LoadLocalFonts()
@ stdcall LoadMenuA(long str)
@ stdcall LoadMenuIndirectA(ptr)
@ stdcall LoadMenuIndirectW(ptr)
@ stdcall LoadMenuW(long wstr)
@ stub LoadRemoteFonts
@ stdcall LoadStringA(long long ptr long)
@ stdcall LoadStringW(long long ptr long)
@ stdcall LockSetForegroundWindow(long)
@ stub LockWindowStation
@ stdcall LockWindowUpdate(long) NtUserLockWindowUpdate
@ stdcall LockWorkStation()
@ stdcall LogicalToPhysicalPoint(long ptr)
@ stdcall LogicalToPhysicalPointForPerMonitorDPI(long ptr) NtUserLogicalToPerMonitorDPIPhysicalPoint
@ stdcall LookupIconIdFromDirectory(ptr long)
@ stdcall LookupIconIdFromDirectoryEx(ptr long long long long)
@ stub MBToWCSEx
# @ stub MBToWCSExt
# @ stub MB_GetString
# @ stub MITGetCursorUpdateHandle
# @ stub MITSetLastInputRecipient
# @ stub MITSynthesizeTouchInput
# @ stub MakeThreadTSFEventAware
@ stdcall MapDialogRect(long ptr)
# @ stub MapPointsByVisualIdentifier
@ stdcall MapVirtualKeyA(long long)
@ stdcall MapVirtualKeyExA(long long long)
@ stdcall MapVirtualKeyExW(long long long) NtUserMapVirtualKeyEx
@ stdcall MapVirtualKeyW(long long)
# @ stub MapVisualRelativePoints
@ stdcall MapWindowPoints(long long ptr long)
@ stdcall MenuItemFromPoint(long long int64)
@ stub MenuWindowProcA
@ stub MenuWindowProcW
@ stdcall MessageBeep(long) NtUserMessageBeep
@ stdcall MessageBoxA(long str str long)
@ stdcall MessageBoxExA(long str str long long)
@ stdcall MessageBoxExW(long wstr wstr long long)
@ stdcall MessageBoxIndirectA(ptr)
@ stdcall MessageBoxIndirectW(ptr)
@ stdcall MessageBoxTimeoutA(long str str long long long)
@ stdcall MessageBoxTimeoutW(long wstr wstr long long long)
@ stdcall MessageBoxW(long wstr wstr long)
@ stdcall ModifyMenuA(long long long long ptr)
@ stdcall ModifyMenuW(long long long long ptr)
@ stdcall MonitorFromPoint(int64 long)
@ stdcall MonitorFromRect(ptr long)
@ stdcall MonitorFromWindow(long long)
@ stdcall MoveWindow(long long long long long long) NtUserMoveWindow
@ stdcall MsgWaitForMultipleObjects(long ptr long long long)
@ stdcall MsgWaitForMultipleObjectsEx(long ptr long long long) NtUserMsgWaitForMultipleObjectsEx
# @ stub NotifyOverlayWindow
@ stdcall NotifyWinEvent(long long long long) NtUserNotifyWinEvent
@ stdcall OemKeyScan(long)
@ stdcall OemToCharA(str ptr)
@ stdcall OemToCharBuffA(ptr ptr long)
@ stdcall OemToCharBuffW(ptr ptr long)
@ stdcall OemToCharW(str ptr)
@ stdcall OffsetRect(ptr long long)
@ stdcall OpenClipboard(long)
@ stdcall OpenDesktopA(str long long long)
@ stdcall OpenDesktopW(wstr long long long)
@ stdcall OpenIcon(long)
@ stdcall OpenInputDesktop(long long long) NtUserOpenInputDesktop
# @ stub OpenThreadDesktop
@ stdcall OpenWindowStationA(str long long)
@ stdcall OpenWindowStationW(wstr long long)
@ stdcall PackDDElParam(long long long)
@ stdcall PackTouchHitTestingProximityEvaluation(ptr ptr)
@ stdcall PaintDesktop(long)
# @ stub PaintMenuBar
# @ stub PaintMonitor
@ stdcall PeekMessageA(ptr long long long long)
@ stdcall PeekMessageW(ptr long long long long)
@ stdcall PhysicalToLogicalPoint(long ptr)
@ stdcall PhysicalToLogicalPointForPerMonitorDPI(long ptr) NtUserPerMonitorDPIPhysicalToLogicalPoint
@ stub PlaySoundEvent
@ stdcall PostMessageA(long long long long)
@ stdcall PostMessageW(long long long long)
@ stdcall PostQuitMessage(long) NtUserPostQuitMessage
@ stdcall PostThreadMessageA(long long long long)
@ stdcall PostThreadMessageW(long long long long) NtUserPostThreadMessage
@ stdcall PrintWindow(long long long) NtUserPrintWindow
@ stdcall PrivateExtractIconExA(str long ptr ptr long)
@ stdcall PrivateExtractIconExW(wstr long ptr ptr long)
@ stdcall PrivateExtractIconsA(str long long long ptr ptr long long)
@ stdcall PrivateExtractIconsW(wstr long long long ptr ptr long long)
# @ stub PrivateRegisterICSProc
@ stdcall PtInRect(ptr int64)
# @ stub QueryBSDRWindow
@ stdcall QueryDisplayConfig(long ptr ptr ptr ptr ptr) NtUserQueryDisplayConfig
@ stub QuerySendMessage
# @ stub RIMAddInputObserver
# @ stub RIMAreSiblingDevices
# @ stub RIMDeviceIoControl
# @ stub RIMEnableMonitorMappingForDevice
# @ stub RIMFreeInputBuffer
# @ stub RIMGetDevicePreparsedData
# @ stub RIMGetDevicePreparsedDataLockfree
# @ stub RIMGetDeviceProperties
# @ stub RIMGetDevicePropertiesLockfree
# @ stub RIMGetPhysicalDeviceRect
# @ stub RIMGetSourceProcessId
# @ stub RIMObserveNextInput
# @ stub RIMOnPnpNotification
# @ stub RIMOnTimerNotification
# @ stub RIMQueryDevicePath
# @ stub RIMReadInput
# @ stub RIMRegisterForInput
# @ stub RIMRemoveInputObserver
# @ stub RIMSetExtendedDeviceProperty
# @ stub RIMSetTestModeStatus
# @ stub RIMUnregisterForInput
# @ stub RIMUpdateInputObserverRegistration
@ stdcall RealChildWindowFromPoint(long int64)
@ stdcall RealGetWindowClass(long ptr long) RealGetWindowClassA
@ stdcall RealGetWindowClassA(long ptr long)
@ stdcall RealGetWindowClassW(long ptr long)
# @ stub ReasonCodeNeedsBugID
# @ stub ReasonCodeNeedsComment
# @ stub RecordShutdownReason
@ stdcall RedrawWindow(long ptr long long) NtUserRedrawWindow
# @ stub RegisterBSDRWindow
@ stdcall RegisterClassA(ptr)
@ stdcall RegisterClassExA(ptr)
@ stdcall RegisterClassExW(ptr)
@ stdcall RegisterClassW(ptr)
@ stdcall RegisterClipboardFormatA(str)
@ stdcall RegisterClipboardFormatW(wstr)
# @ stub RegisterDManipHook
@ stdcall RegisterDeviceNotificationA(long ptr long)
@ stdcall RegisterDeviceNotificationW(long ptr long)
# @ stub RegisterErrorReportingDialog
# @ stub RegisterFrostWindow
# @ stub RegisterGhostWindow
@ stdcall RegisterHotKey(long long long long) NtUserRegisterHotKey
@ stdcall RegisterLogonProcess(long long)
# @ stub RegisterMessagePumpHook
@ stub RegisterNetworkCapabilities
@ stdcall RegisterPointerDeviceNotifications(long long)
# @ stub RegisterPointerInputTarget
# @ stub RegisterPointerInputTargetEx
@ stdcall RegisterPowerSettingNotification(long ptr long)
@ stdcall -import RegisterRawInputDevices(ptr long long) NtUserRegisterRawInputDevices
@ stdcall RegisterServicesProcess(long)
# @ stub RegisterSessionPort
@ stdcall RegisterShellHookWindow(long)
@ stdcall RegisterSuspendResumeNotification(long long)
@ stdcall RegisterSystemThread(long long)
@ stdcall RegisterTasklist(long)
@ stdcall RegisterTouchHitTestingWindow(long long)
@ stdcall RegisterTouchWindow(long long)
@ stdcall RegisterUserApiHook(ptr ptr)
@ stdcall RegisterWindowMessageA(str)
@ stdcall RegisterWindowMessageW(wstr)
@ stdcall ReleaseCapture() NtUserReleaseCapture
@ stdcall ReleaseDC(long long) NtUserReleaseDC
# @ stub ReleaseDwmHitTestWaiters
@ stdcall RemoveClipboardFormatListener(long) NtUserRemoveClipboardFormatListener
# @ stub RemoveInjectionDevice
@ stdcall RemoveMenu(long long long) NtUserRemoveMenu
@ stdcall RemovePropA(long str)
@ stdcall RemovePropW(long wstr)
# @ stub RemoveThreadTSFEventAwareness
# @ stub RemoveVisualIdentifier
@ stdcall ReplyMessage(long) NtUserReplyMessage
@ stub ResetDisplay
# @ stub ResolveDesktopForWOW
@ stdcall ReuseDDElParam(long long long long long)
@ stdcall ScreenToClient(long ptr)
@ stdcall ScrollChildren(long long long long)
@ stdcall ScrollDC(long long long ptr ptr long ptr) NtUserScrollDC
@ stdcall ScrollWindow(long long long ptr ptr)
@ stdcall ScrollWindowEx(long long long ptr ptr long ptr long) NtUserScrollWindowEx
@ stdcall SendDlgItemMessageA(long long long long long)
@ stdcall SendDlgItemMessageW(long long long long long)
@ stdcall SendIMEMessageExA(long long)
@ stdcall SendIMEMessageExW(long long)
@ stdcall SendInput(long ptr long) NtUserSendInput
@ stdcall SendMessageA(long long long long)
@ stdcall SendMessageCallbackA(long long long long ptr long)
@ stdcall SendMessageCallbackW(long long long long ptr long)
@ stdcall SendMessageTimeoutA(long long long long long long ptr)
@ stdcall SendMessageTimeoutW(long long long long long long ptr)
@ stdcall SendMessageW(long long long long)
@ stdcall SendNotifyMessageA(long long long long)
@ stdcall SendNotifyMessageW(long long long long)
@ stub ServerSetFunctionPointers
@ stdcall SetActiveWindow(long) NtUserSetActiveWindow
@ stdcall -import SetCapture(long) NtUserSetCapture
@ stdcall SetCaretBlinkTime(long) NtUserSetCaretBlinkTime
@ stdcall SetCaretPos(long long) NtUserSetCaretPos
@ stdcall SetClassLongA(long long long)
@ stdcall -arch=win64 SetClassLongPtrA(long long long)
@ stdcall -arch=win64 SetClassLongPtrW(long long long)
@ stdcall SetClassLongW(long long long)
@ stdcall SetClassWord(long long long) NtUserSetClassWord
@ stdcall SetClipboardData(long long)
@ stdcall SetClipboardViewer(long) NtUserSetClipboardViewer
@ stdcall SetCoalescableTimer(long long long ptr long) NtUserSetTimer
@ stdcall -import SetCursor(long) NtUserSetCursor
@ stub SetCursorContents
@ stdcall -import SetCursorPos(long long) NtUserSetCursorPos
@ stdcall SetDebugErrorLevel(long)
@ stdcall SetDeskWallpaper(str)
# @ stub SetDesktopColorTransform
# @ stub SetDialogControlDpiChangeBehavior
# @ stub SetDialogDpiChangeBehavior
@ stdcall SetDisplayAutoRotationPreferences(long)
@ stdcall SetDisplayConfig(long ptr long ptr long)
@ stdcall SetDlgItemInt(long long long long)
@ stdcall SetDlgItemTextA(long long str)
@ stdcall SetDlgItemTextW(long long wstr)
@ stdcall SetDoubleClickTime(long)
# @ stub SetFeatureReportResponse
@ stdcall SetFocus(long) NtUserSetFocus
# @ stub SetForegroundRedirectionForActivationObject
@ stdcall SetForegroundWindow(long)
# @ stub SetFullscreenMagnifierOffsetsDWMUpdated
@ stdcall SetGestureConfig(ptr long long ptr long)
@ stdcall SetInternalWindowPos(long long ptr ptr) NtUserSetInternalWindowPos
@ stdcall SetKeyboardState(ptr) NtUserSetKeyboardState
@ stdcall SetLastErrorEx(long long)
@ stdcall SetLayeredWindowAttributes(ptr long long long) NtUserSetLayeredWindowAttributes
@ stdcall SetLogonNotifyWindow(long long)
# @ stub SetMagnificationDesktopColorEffect
# @ stub SetMagnificationDesktopMagnification
# @ stub SetMagnificationDesktopMagnifierOffsetsDWMUpdated
# @ stub SetMagnificationDesktopSamplingMode
# @ stub SetMagnificationLensCtxInformation
@ stdcall SetMenu(long long) NtUserSetMenu
@ stdcall SetMenuContextHelpId(long long) NtUserSetMenuContextHelpId
@ stdcall SetMenuDefaultItem(long long long) NtUserSetMenuDefaultItem
@ stdcall SetMenuInfo(long ptr)
@ stdcall SetMenuItemBitmaps(long long long long long)
@ stdcall SetMenuItemInfoA(long long long ptr)
@ stdcall SetMenuItemInfoW(long long long ptr)
@ stdcall SetMessageExtraInfo(long)
@ stdcall SetMessageQueue(long)
# @ stub SetMirrorRendering
@ stdcall SetParent(long long) NtUserSetParent
@ stdcall SetPhysicalCursorPos(long long)
# @ stub SetPointerDeviceInputSpace
@ stdcall SetProcessDPIAware()
@ stdcall SetProcessDefaultLayout(long) NtUserSetProcessDefaultLayout
@ stdcall SetProcessDpiAwarenessContext(long)
@ stdcall SetProcessDpiAwarenessInternal(long)
# @ stub SetProcessLaunchForegroundPolicy
# @ stub SetProcessRestrictionExemption
@ stdcall SetProcessWindowStation(long) NtUserSetProcessWindowStation
@ stdcall SetProgmanWindow(long) NtUserSetProgmanWindow
@ stdcall SetPropA(long str long)
@ stdcall SetPropW(long wstr long)
@ stdcall SetRect(ptr long long long long)
@ stdcall SetRectEmpty(ptr)
@ stdcall -import SetScrollInfo(long long ptr long) NtUserSetScrollInfo
@ stdcall SetScrollPos(long long long long)
@ stdcall SetScrollRange(long long long long long)
# @ stub SetShellChangeNotifyWindow
@ stdcall SetShellWindow(long)
@ stdcall SetShellWindowEx(long long) NtUserSetShellWindowEx
@ stdcall SetSysColors(long ptr ptr) NtUserSetSysColors
@ stdcall SetSysColorsTemp(ptr ptr long)
@ stdcall SetSystemCursor(long long)
@ stdcall SetSystemMenu(long long) NtUserSetSystemMenu
@ stdcall SetSystemTimer(long long long ptr)
@ stdcall SetTaskmanWindow(long) NtUserSetTaskmanWindow
@ stdcall SetThreadDesktop(long) NtUserSetThreadDesktop
@ stdcall SetThreadDpiAwarenessContext(ptr)
@ stdcall SetThreadDpiHostingBehavior(long)
# @ stub SetThreadInputBlocked
@ stdcall SetTimer(long long long ptr)
@ stdcall SetUserObjectInformationA(long long ptr long)
@ stdcall SetUserObjectInformationW(long long ptr long) NtUserSetObjectInformation
@ stdcall SetUserObjectSecurity(long ptr ptr)
@ stdcall SetWinEventHook(long long long ptr long long long)
# @ stub SetWindowBand
@ stdcall SetWindowCompositionAttribute(ptr ptr)
# @ stub SetWindowCompositionTransition
@ stdcall SetWindowContextHelpId(long long) NtUserSetWindowContextHelpId
@ stdcall SetWindowDisplayAffinity(long long)
# @ stub SetWindowFeedbackSetting
@ stdcall SetWindowLongA(long long long)
@ stdcall -arch=win64 SetWindowLongPtrA(long long long)
@ stdcall -arch=win64 SetWindowLongPtrW(long long long)
@ stdcall SetWindowLongW(long long long)
@ stdcall SetWindowPlacement(long ptr) NtUserSetWindowPlacement
@ stdcall SetWindowPos(long long long long long long long) NtUserSetWindowPos
@ stdcall SetWindowRgn(long long long) NtUserSetWindowRgn
# @ stub SetWindowRgnEx
@ stdcall SetWindowStationUser(long long)
@ stdcall SetWindowTextA(long str)
@ stdcall SetWindowTextW(long wstr)
@ stdcall SetWindowWord(long long long) NtUserSetWindowWord
@ stdcall SetWindowsHookA(long ptr)
@ stdcall SetWindowsHookExA(long long long long)
# @ stub SetWindowsHookExAW
@ stdcall SetWindowsHookExW(long long long long)
@ stdcall SetWindowsHookW(long ptr)
# @ stub ShellSetWindowPos
@ stdcall ShowCaret(long) NtUserShowCaret
@ stdcall -import ShowCursor(long) NtUserShowCursor
@ stdcall ShowOwnedPopups(long long) NtUserShowOwnedPopups
@ stdcall ShowScrollBar(long long long) NtUserShowScrollBar
@ stub ShowStartGlass
# @ stub ShowSystemCursor
@ stdcall ShowWindow(long long) NtUserShowWindow
@ stdcall ShowWindowAsync(long long) NtUserShowWindowAsync
@ stdcall ShutdownBlockReasonCreate(long wstr)
@ stdcall ShutdownBlockReasonDestroy(long)
# @ stub ShutdownBlockReasonQuery
# @ stub SignalRedirectionStartComplete
# @ stub SkipPointerFrameMessages
# @ stub SoftModalMessageBox
# @ stub SoundSentry
@ stdcall SubtractRect(ptr ptr ptr)
@ stdcall SwapMouseButton(long)
@ stdcall SwitchDesktop(long) NtUserSwitchDesktop
# @ stub SwitchDesktopWithFade
@ stdcall SwitchToThisWindow(long long)
@ stdcall SystemParametersInfoA(long long ptr long)
@ stdcall SystemParametersInfoForDpi(long long ptr long long)
@ stdcall SystemParametersInfoW(long long ptr long)
@ stdcall TabbedTextOutA(long long long str long long ptr long)
@ stdcall TabbedTextOutW(long long long wstr long long ptr long)
@ stdcall TileChildWindows(long long)
@ stdcall TileWindows(long long ptr long ptr)
@ stdcall ToAscii(long long ptr ptr long)
@ stdcall ToAsciiEx(long long ptr ptr long long)
@ stdcall ToUnicode(long long ptr ptr long long)
@ stdcall ToUnicodeEx(long long ptr ptr long long long) NtUserToUnicodeEx
@ stdcall TrackMouseEvent(ptr) NtUserTrackMouseEvent
@ stdcall TrackPopupMenu(long long long long long long ptr)
@ stdcall TrackPopupMenuEx(long long long long long ptr) NtUserTrackPopupMenuEx
@ stdcall TranslateAccelerator(long long ptr) TranslateAcceleratorA
@ stdcall TranslateAcceleratorA(long long ptr)
@ stdcall TranslateAcceleratorW(long long ptr) NtUserTranslateAccelerator
@ stdcall TranslateMDISysAccel(long ptr)
@ stdcall TranslateMessage(ptr)
# @ stub TranslateMessageEx
@ stdcall UnhookWinEvent(long) NtUserUnhookWinEvent
@ stdcall UnhookWindowsHook(long ptr) NtUserUnhookWindowsHook
@ stdcall UnhookWindowsHookEx(long) NtUserUnhookWindowsHookEx
@ stdcall UnionRect(ptr ptr ptr)
@ stdcall UnloadKeyboardLayout(long)
@ stub UnlockWindowStation
@ stdcall UnpackDDElParam(long long ptr ptr)
@ stdcall UnregisterClassA(str long)
@ stdcall UnregisterClassW(wstr long)
@ stdcall UnregisterDeviceNotification(long)
@ stdcall UnregisterHotKey(long long) NtUserUnregisterHotKey
# @ stub UnregisterMessagePumpHook
# @ stub UnregisterPointerInputTarget
# @ stub UnregisterPointerInputTargetEx
@ stdcall UnregisterPowerSettingNotification(ptr)
# @ stub UnregisterSessionPort
@ stdcall UnregisterSuspendResumeNotification(ptr)
@ stdcall UnregisterTouchWindow(long)
@ stdcall UnregisterUserApiHook()
# @ stub UpdateDefaultDesktopThumbnail
@ stdcall UpdateLayeredWindow(long long ptr ptr long ptr long ptr long)
@ stdcall UpdateLayeredWindowIndirect(long ptr)
@ stub UpdatePerUserSystemParameters
@ stdcall UpdateWindow(long)
# @ stub UpdateWindowInputSinkHints
@ stdcall User32InitializeImmEntryTable(ptr)
@ stdcall UserClientDllInitialize(long long ptr) DllMain
@ stdcall UserHandleGrantAccess(ptr ptr long)
# @ stub UserLpkPSMTextOut
# @ stub UserLpkTabbedTextOut
@ stdcall UserRealizePalette(long) NtUserRealizePalette
@ stdcall UserRegisterWowHandlers(ptr ptr)
@ stdcall UserSignalProc(long long long long)
# @ stub VRipOutput
# @ stub VTagOutput
@ stdcall ValidateRect(long ptr) NtUserValidateRect
@ stdcall ValidateRgn(long long) NtUserValidateRgn
@ stdcall VkKeyScanA(long)
@ stdcall VkKeyScanExA(long long)
@ stdcall VkKeyScanExW(long long) NtUserVkKeyScanEx
@ stdcall VkKeyScanW(long)
@ stub WCSToMBEx
@ stdcall WINNLSEnableIME(long long)
@ stdcall WINNLSGetEnableStatus(long)
@ stdcall WINNLSGetIMEHotkey(long)
@ stub WNDPROC_CALLBACK
@ stdcall WaitForInputIdle(long long)
# @ stub WaitForRedirectionStartComplete
@ stdcall WaitMessage() NtUserWaitMessage
@ stdcall WinHelpA(long str long long)
@ stdcall WinHelpW(long wstr long long)
@ stdcall WindowFromDC(long) NtUserWindowFromDC
@ stdcall WindowFromPhysicalPoint(int64)
@ stdcall WindowFromPoint(int64)
# @ stub _UserTestTokenForInteractive
# @ extern gSharedInfo
# @ extern gapfnScSendMessage
@ stdcall keybd_event(long long long long)
@ stdcall mouse_event(long long long long long)
@ varargs wsprintfA(str str)
@ varargs wsprintfW(wstr wstr)
@ stdcall wvsprintfA(ptr str ptr)
@ stdcall wvsprintfW(ptr wstr ptr)
