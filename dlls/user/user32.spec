name	user32
type	win32
init	USER_Init
rsrc	user32

import	gdi32.dll
import	kernel32.dll

@ stdcall ActivateKeyboardLayout(long long) ActivateKeyboardLayout
@ stdcall AdjustWindowRect(ptr long long) AdjustWindowRect
@ stdcall AdjustWindowRectEx(ptr long long long) AdjustWindowRectEx
@ stdcall AnyPopup() AnyPopup
@ stdcall AppendMenuA(long long long ptr) AppendMenuA
@ stdcall AppendMenuW(long long long ptr) AppendMenuW
@ stdcall ArrangeIconicWindows(long) ArrangeIconicWindows
@ stdcall AttachThreadInput(long long long) AttachThreadInput
@ stdcall BeginDeferWindowPos(long) BeginDeferWindowPos
@ stdcall BeginPaint(long ptr) BeginPaint
@ stdcall BringWindowToTop(long) BringWindowToTop
@ stdcall BroadcastSystemMessage(long ptr long long long) BroadcastSystemMessage
@ stub CalcChildScroll
@ stub CallMsgFilter
@ stdcall CallMsgFilterA(ptr long) CallMsgFilterA
@ stdcall CallMsgFilterW(ptr long) CallMsgFilterW
@ stdcall CallNextHookEx(long long long long) CallNextHookEx
@ stdcall CallWindowProcA(ptr long long long long) CallWindowProcA
@ stdcall CallWindowProcW(ptr long long long long) CallWindowProcW
@ stub CascadeChildWindows
@ stdcall CascadeWindows(long long ptr long ptr) CascadeWindows
@ stdcall ChangeClipboardChain(long long) ChangeClipboardChain
@ stdcall ChangeMenuA(long long ptr long long) ChangeMenuA
@ stdcall ChangeMenuW(long long ptr long long) ChangeMenuW
@ stdcall CharLowerA(str) CharLowerA
@ stdcall CharLowerBuffA(str long) CharLowerBuffA
@ stdcall CharLowerBuffW(wstr long) CharLowerBuffW
@ stdcall CharLowerW(wstr) CharLowerW
@ stdcall CharNextA(str) CharNextA
@ stdcall CharNextExA(long str long) CharNextExA
@ stdcall CharNextExW(long wstr long) CharNextExW
@ stdcall CharNextW(wstr) CharNextW
@ stdcall CharPrevA(str str) CharPrevA
@ stdcall CharPrevExA(long str str long) CharPrevExA
@ stdcall CharPrevExW(long wstr wstr long) CharPrevExW
@ stdcall CharPrevW(wstr wstr) CharPrevW
@ stdcall CharToOemA(str ptr) CharToOemA
@ stdcall CharToOemBuffA(str ptr long) CharToOemBuffA
@ stdcall CharToOemBuffW(wstr ptr long) CharToOemBuffW
@ stdcall CharToOemW(wstr ptr) CharToOemW
@ stdcall CharUpperA(str) CharUpperA
@ stdcall CharUpperBuffA(str long) CharUpperBuffA
@ stdcall CharUpperBuffW(wstr long) CharUpperBuffW
@ stdcall CharUpperW(wstr) CharUpperW
@ stdcall CheckDlgButton(long long long) CheckDlgButton
@ stdcall CheckMenuItem(long long long) CheckMenuItem
@ stdcall CheckMenuRadioItem(long long long long long) CheckMenuRadioItem
@ stdcall CheckRadioButton(long long long long) CheckRadioButton
@ stdcall ChildWindowFromPoint(long long long) ChildWindowFromPoint
@ stdcall ChildWindowFromPointEx(long long long long) ChildWindowFromPointEx
@ stub ClientThreadConnect
@ stdcall ClientToScreen(long ptr) ClientToScreen
@ stdcall ClipCursor(ptr) ClipCursor
@ stdcall CloseClipboard() CloseClipboard
@ stdcall CloseDesktop(long) CloseDesktop
@ stdcall CloseWindow(long) CloseWindow
@ stdcall CloseWindowStation(long) CloseWindowStation			
@ stdcall CopyAcceleratorTableA(long ptr long) CopyAcceleratorTableA
@ stdcall CopyAcceleratorTableW(long ptr long) CopyAcceleratorTableW
@ stdcall CopyIcon(long) CopyIcon
@ stdcall CopyImage(long long long long long) CopyImage
@ stdcall CopyRect(ptr ptr) CopyRect
@ stdcall CountClipboardFormats() CountClipboardFormats
@ stdcall CreateAcceleratorTableA(ptr long) CreateAcceleratorTableA
@ stdcall CreateAcceleratorTableW(ptr long) CreateAcceleratorTableW
@ stdcall CreateCaret(long long long long) CreateCaret
@ stdcall CreateCursor(long long long long long ptr ptr) CreateCursor
@ stdcall CreateDesktopA(str str ptr long long ptr) CreateDesktopA
@ stdcall CreateDesktopW(wstr wstr ptr long long ptr) CreateDesktopW
@ stdcall CreateDialogIndirectParamA(long ptr long ptr long) CreateDialogIndirectParamA
@ stdcall CreateDialogIndirectParamAorW (long ptr long ptr long) CreateDialogIndirectParamAorW
@ stdcall CreateDialogIndirectParamW(long ptr long ptr long) CreateDialogIndirectParamW
@ stdcall CreateDialogParamA(long ptr long ptr long) CreateDialogParamA
@ stdcall CreateDialogParamW(long ptr long ptr long) CreateDialogParamW
@ stdcall CreateIcon(long long long long long ptr ptr) CreateIcon
@ stdcall CreateIconFromResource (ptr long long long) CreateIconFromResource
@ stdcall CreateIconFromResourceEx(ptr long long long long long long) CreateIconFromResourceEx
@ stdcall CreateIconIndirect(ptr) CreateIconIndirect
@ stdcall CreateMDIWindowA(ptr ptr long long long long long long long long) CreateMDIWindowA
@ stdcall CreateMDIWindowW(ptr ptr long long long long long long long long) CreateMDIWindowW
@ stdcall CreateMenu() CreateMenu
@ stdcall CreatePopupMenu() CreatePopupMenu
@ stdcall CreateWindowExA(long str str long long long long long long long long ptr) CreateWindowExA
@ stdcall CreateWindowExW(long wstr wstr long long long long long long long long ptr) CreateWindowExW
@ stub CreateWindowStationA
@ stdcall CreateWindowStationW(wstr long long ptr) CreateWindowStationW
@ stdcall DdeAbandonTransaction(long long long)DdeAbandonTransaction
@ stdcall DdeAccessData(long ptr) DdeAccessData
@ stdcall DdeAddData(long ptr long long) DdeAddData
@ stdcall DdeClientTransaction(ptr long long long long long long ptr) DdeClientTransaction
@ stdcall DdeCmpStringHandles(long long) DdeCmpStringHandles
@ stdcall DdeConnect(long long long ptr) DdeConnect
@ stdcall DdeConnectList(long long long long ptr) DdeConnectList
@ stdcall DdeCreateDataHandle(long ptr long long long long long) DdeCreateDataHandle
@ stdcall DdeCreateStringHandleA(long str long) DdeCreateStringHandleA
@ stdcall DdeCreateStringHandleW(long wstr long) DdeCreateStringHandleW
@ stdcall DdeDisconnect(long) DdeDisconnect
@ stdcall DdeDisconnectList(long) DdeDisconnectList
@ stdcall DdeEnableCallback(long long long) DdeEnableCallback
@ stdcall DdeFreeDataHandle(long) DdeFreeDataHandle
@ stdcall DdeFreeStringHandle(long long) DdeFreeStringHandle
@ stdcall DdeGetData(long ptr long long) DdeGetData
@ stdcall DdeGetLastError(long) DdeGetLastError
@ stub DdeGetQualityOfService
@ stdcall DdeImpersonateClient(long) DdeImpersonateClient
@ stdcall DdeInitializeA(ptr ptr long long) DdeInitializeA
@ stdcall DdeInitializeW(ptr ptr long long) DdeInitializeW
@ stdcall DdeKeepStringHandle(long long) DdeKeepStringHandle
@ stdcall DdeNameService(long long long long) DdeNameService
@ stdcall DdePostAdvise(long long long) DdePostAdvise
@ stdcall DdeQueryConvInfo(long long ptr) DdeQueryConvInfo
@ stdcall DdeQueryNextServer(long long) DdeQueryNextServer
@ stdcall DdeQueryStringA(long long ptr long long) DdeQueryStringA
@ stdcall DdeQueryStringW(long long ptr long long) DdeQueryStringW
@ stdcall DdeReconnect(long) DdeReconnect
@ stdcall DdeSetQualityOfService(long ptr ptr) DdeSetQualityOfService
@ stdcall DdeSetUserHandle (long long long) DdeSetUserHandle
@ stdcall DdeUnaccessData(long) DdeUnaccessData
@ stdcall DdeUninitialize(long) DdeUninitialize
@ stdcall DefDlgProcA(long long long long) DefDlgProcA
@ stdcall DefDlgProcW(long long long long) DefDlgProcW
@ stdcall DefFrameProcA(long long long long long) DefFrameProcA
@ stdcall DefFrameProcW(long long long long long) DefFrameProcW
@ stdcall DefMDIChildProcA(long long long long) DefMDIChildProcA
@ stdcall DefMDIChildProcW(long long long long) DefMDIChildProcW
@ stdcall DefWindowProcA(long long long long) DefWindowProcA
@ stdcall DefWindowProcW(long long long long) DefWindowProcW
@ stdcall DeferWindowPos(long long long long long long long long) DeferWindowPos
@ stdcall DeleteMenu(long long long) DeleteMenu
@ stdcall DestroyAcceleratorTable(long) DestroyAcceleratorTable
@ stdcall DestroyCaret() DestroyCaret
@ stdcall DestroyCursor(long) DestroyCursor
@ stdcall DestroyIcon(long) DestroyIcon
@ stdcall DestroyMenu(long) DestroyMenu
@ stdcall DestroyWindow(long) DestroyWindow
@ stdcall DialogBoxIndirectParamA(long ptr long ptr long) DialogBoxIndirectParamA
@ stdcall DialogBoxIndirectParamAorW(long ptr long ptr long long) DialogBoxIndirectParamAorW
@ stdcall DialogBoxIndirectParamW(long ptr long ptr long) DialogBoxIndirectParamW
@ stdcall DialogBoxParamA(long str long ptr long) DialogBoxParamA
@ stdcall DialogBoxParamW(long wstr long ptr long) DialogBoxParamW
@ stdcall DispatchMessageA(ptr) DispatchMessageA
@ stdcall DispatchMessageW(ptr) DispatchMessageW
@ stdcall DlgDirListA(long ptr long long long) DlgDirListA
@ stdcall DlgDirListComboBoxA(long ptr long long long) DlgDirListComboBoxA
@ stdcall DlgDirListComboBoxW(long ptr long long long) DlgDirListComboBoxW
@ stdcall DlgDirListW(long ptr long long long) DlgDirListW
@ stdcall DlgDirSelectComboBoxExA(long ptr long long) DlgDirSelectComboBoxExA
@ stdcall DlgDirSelectComboBoxExW(long ptr long long) DlgDirSelectComboBoxExW
@ stdcall DlgDirSelectExA(long ptr long long) DlgDirSelectExA
@ stdcall DlgDirSelectExW(long ptr long long) DlgDirSelectExW
@ stdcall DragDetect(long long long) DragDetect
@ stub DragObject
@ stdcall DrawAnimatedRects(long long ptr ptr) DrawAnimatedRects
@ stdcall DrawCaption(long long ptr long) DrawCaption
@ stdcall DrawEdge(long ptr long long) DrawEdge
@ stdcall DrawFocusRect(long ptr) DrawFocusRect
@ stub DrawFrame
@ stdcall DrawFrameControl(long ptr long long) DrawFrameControl
@ stdcall DrawIcon(long long long long) DrawIcon
@ stdcall DrawIconEx(long long long long long long long long long) DrawIconEx
@ stdcall DrawMenuBar(long) DrawMenuBar
@ stdcall DrawStateA(long long ptr long long long long long long long) DrawStateA
@ stdcall DrawStateW(long long ptr long long long long long long long) DrawStateW
@ stdcall DrawTextA(long str long ptr long) DrawTextA
@ stdcall DrawTextExA(long str long ptr long ptr) DrawTextExA
@ stdcall DrawTextExW(long wstr long ptr long ptr) DrawTextExW
@ stdcall DrawTextW(long wstr long ptr long) DrawTextW
@ stdcall EditWndProc(long long long long) EditWndProc
@ stdcall EmptyClipboard() EmptyClipboard
@ stdcall EnableMenuItem(long long long) EnableMenuItem
@ stdcall EnableScrollBar(long long long) EnableScrollBar
@ stdcall EnableWindow(long long) EnableWindow
@ stdcall EndDeferWindowPos(long) EndDeferWindowPos
@ stdcall EndDialog(long long) EndDialog
@ stdcall EndMenu() EndMenu
@ stdcall EndPaint(long ptr) EndPaint
@ stub EndTask
@ stdcall EnumChildWindows(long ptr long) EnumChildWindows
@ stdcall EnumClipboardFormats(long) EnumClipboardFormats
@ stub EnumDesktopsA
@ stub EnumDesktopsW
@ stub EnumDisplayDeviceModesA
@ stub EnumDisplayDeviceModesW
@ stdcall EnumDisplayDevicesA(ptr long ptr long) EnumDisplayDevicesA
@ stdcall EnumDisplayDevicesW(ptr long ptr long) EnumDisplayDevicesW
@ stdcall EnumPropsA(long ptr) EnumPropsA
@ stdcall EnumPropsExA(long ptr long) EnumPropsExA
@ stdcall EnumPropsExW(long ptr long) EnumPropsExW
@ stdcall EnumPropsW(long ptr) EnumPropsW
@ stdcall EnumThreadWindows(long ptr long) EnumThreadWindows
@ stub EnumWindowStationsA
@ stub EnumWindowStationsW
@ stdcall EnumWindows(ptr long) EnumWindows
@ stdcall EqualRect(ptr ptr) EqualRect
@ stdcall ExcludeUpdateRgn(long long) ExcludeUpdateRgn
@ stdcall ExitWindowsEx(long long) ExitWindowsEx
@ stdcall FillRect(long ptr long) FillRect
@ stdcall FindWindowA(str str) FindWindowA
@ stdcall FindWindowExA(long long str str) FindWindowExA
@ stdcall FindWindowExW(long long wstr wstr) FindWindowExW
@ stdcall FindWindowW(wstr wstr) FindWindowW
@ stdcall FlashWindow(long long) FlashWindow
@ stdcall FrameRect(long ptr long) FrameRect
@ stdcall FreeDDElParam(long long) FreeDDElParam
@ stdcall GetActiveWindow() GetActiveWindow
@ stdcall GetAppCompatFlags(long) GetAppCompatFlags
@ stdcall GetAsyncKeyState(long) GetAsyncKeyState
@ stdcall GetCapture() GetCapture
@ stdcall GetCaretBlinkTime() GetCaretBlinkTime
@ stdcall GetCaretPos(ptr) GetCaretPos
@ stdcall GetClassInfoA(long str ptr) GetClassInfoA
@ stdcall GetClassInfoExA(long str ptr) GetClassInfoExA
@ stdcall GetClassInfoExW(long wstr ptr) GetClassInfoExW
@ stdcall GetClassInfoW(long wstr ptr) GetClassInfoW
@ stdcall GetClassLongA(long long) GetClassLongA
@ stdcall GetClassLongW(long long) GetClassLongW
@ stdcall GetClassNameA(long ptr long) GetClassNameA
@ stdcall GetClassNameW(long ptr long) GetClassNameW
@ stdcall GetClassWord(long long) GetClassWord
@ stdcall GetClientRect(long long) GetClientRect
@ stdcall GetClipCursor(ptr) GetClipCursor
@ stdcall GetClipboardData(long) GetClipboardData
@ stdcall GetClipboardFormatNameA(long ptr long) GetClipboardFormatNameA
@ stdcall GetClipboardFormatNameW(long ptr long) GetClipboardFormatNameW
@ stdcall GetClipboardOwner() GetClipboardOwner
@ stdcall GetClipboardViewer() GetClipboardViewer
@ stdcall GetCursor() GetCursor
@ stub GetCursorInfo
@ stdcall GetCursorPos(ptr) GetCursorPos
@ stdcall GetDC(long) GetDC
@ stdcall GetDCEx(long long long) GetDCEx
@ stdcall GetDesktopWindow() GetDesktopWindow
@ stdcall GetDialogBaseUnits() GetDialogBaseUnits
@ stdcall GetDlgCtrlID(long) GetDlgCtrlID
@ stdcall GetDlgItem(long long) GetDlgItem
@ stdcall GetDlgItemInt(long long ptr long) GetDlgItemInt
@ stdcall GetDlgItemTextA(long long ptr long) GetDlgItemTextA
@ stdcall GetDlgItemTextW(long long ptr long) GetDlgItemTextW
@ stdcall GetDoubleClickTime() GetDoubleClickTime
@ stdcall GetFocus() GetFocus
@ stdcall GetForegroundWindow() GetForegroundWindow
@ stdcall GetIconInfo(long ptr) GetIconInfo
@ stub GetInputDesktop
@ stdcall GetInputState() GetInputState
@ stdcall GetInternalWindowPos(long ptr ptr) GetInternalWindowPos
@ stdcall GetKBCodePage() GetKBCodePage
@ stdcall GetKeyNameTextA(long ptr long) GetKeyNameTextA
@ stdcall GetKeyNameTextW(long ptr long) GetKeyNameTextW
@ stdcall GetKeyState(long) GetKeyState
@ stdcall GetKeyboardLayout(long) GetKeyboardLayout
@ stdcall GetKeyboardLayoutList(long ptr) GetKeyboardLayoutList
@ stdcall GetKeyboardLayoutNameA(ptr) GetKeyboardLayoutNameA
@ stdcall GetKeyboardLayoutNameW(ptr) GetKeyboardLayoutNameW
@ stdcall GetKeyboardState(ptr) GetKeyboardState
@ stdcall GetKeyboardType(long) GetKeyboardType
@ stdcall GetLastActivePopup(long) GetLastActivePopup
@ stdcall GetMenu(long) GetMenu
@ stdcall GetMenuCheckMarkDimensions() GetMenuCheckMarkDimensions
@ stdcall GetMenuContextHelpId(long) GetMenuContextHelpId
@ stdcall GetMenuDefaultItem(long long long) GetMenuDefaultItem
@ stub GetMenuIndex
@ stdcall GetMenuItemCount(long) GetMenuItemCount
@ stdcall GetMenuItemID(long long) GetMenuItemID
@ stdcall GetMenuItemInfoA(long long long ptr) GetMenuItemInfoA
@ stdcall GetMenuItemInfoW(long long long ptr) GetMenuItemInfoW
@ stdcall GetMenuItemRect(long long long ptr) GetMenuItemRect
@ stdcall GetMenuState(long long long) GetMenuState
@ stdcall GetMenuStringA(long long ptr long long) GetMenuStringA
@ stdcall GetMenuStringW(long long ptr long long) GetMenuStringW
@ stdcall GetMessageA(ptr long long long) GetMessageA
@ stdcall GetMessageExtraInfo() GetMessageExtraInfo
@ stdcall GetMessagePos() GetMessagePos
@ stdcall GetMessageTime() GetMessageTime
@ stdcall GetMessageW(ptr long long long) GetMessageW
@ stdcall GetNextDlgGroupItem(long long long) GetNextDlgGroupItem
@ stdcall GetNextDlgTabItem(long long long) GetNextDlgTabItem
@ stdcall GetOpenClipboardWindow() GetOpenClipboardWindow
@ stdcall GetParent(long) GetParent
@ stdcall GetPriorityClipboardFormat(ptr long) GetPriorityClipboardFormat
@ stdcall GetProcessWindowStation() GetProcessWindowStation
@ stdcall GetPropA(long str) GetPropA
@ stdcall GetPropW(long wstr) GetPropW
@ stdcall GetQueueStatus(long) GetQueueStatus
@ stdcall GetScrollInfo(long long ptr) GetScrollInfo
@ stdcall GetScrollPos(long long) GetScrollPos
@ stdcall GetScrollRange(long long ptr ptr) GetScrollRange
@ stdcall GetShellWindow() GetShellWindow
@ stdcall GetSubMenu(long long) GetSubMenu
@ stdcall GetSysColor(long) GetSysColor
@ stdcall GetSysColorBrush(long) GetSysColorBrush
@ stdcall GetSystemMenu(long long) GetSystemMenu
@ stdcall GetSystemMetrics(long) GetSystemMetrics
@ stdcall GetTabbedTextExtentA(long str long long ptr) GetTabbedTextExtentA
@ stdcall GetTabbedTextExtentW(long wstr long long ptr) GetTabbedTextExtentW
@ stdcall GetThreadDesktop(long) GetThreadDesktop
@ stdcall GetTopWindow(long) GetTopWindow
@ stdcall GetUpdateRect(long ptr long) GetUpdateRect
@ stdcall GetUpdateRgn(long long long) GetUpdateRgn
@ stdcall GetUserObjectInformationA (long long ptr long ptr) GetUserObjectInformationA
@ stdcall GetUserObjectInformationW (long long ptr long ptr) GetUserObjectInformationW
@ stdcall GetUserObjectSecurity (long ptr ptr long ptr) GetUserObjectSecurity
@ stdcall GetWindow(long long) GetWindow
@ stdcall GetWindowContextHelpId(long) GetWindowContextHelpId
@ stdcall GetWindowDC(long) GetWindowDC
@ stdcall GetWindowLongA(long long) GetWindowLongA
@ stdcall GetWindowLongW(long long) GetWindowLongW
@ stdcall GetWindowPlacement(long ptr) GetWindowPlacement
@ stdcall GetWindowRect(long ptr) GetWindowRect
@ stdcall GetWindowTextA(long ptr long) GetWindowTextA
@ stdcall GetWindowTextLengthA(long) GetWindowTextLengthA
@ stdcall GetWindowTextLengthW(long) GetWindowTextLengthW
@ stdcall GetWindowTextW(long ptr long) GetWindowTextW
@ stdcall GetWindowThreadProcessId(long ptr) GetWindowThreadProcessId
@ stdcall GetWindowWord(long long) GetWindowWord
@ stdcall GrayStringA(long long ptr long long long long long long) GrayStringA
@ stdcall GrayStringW(long long ptr long long long long long long) GrayStringW
@ stdcall HideCaret(long) HideCaret
@ stdcall HiliteMenuItem(long long long long) HiliteMenuItem
@ stub ImpersonateDdeClientWindow
@ stdcall InSendMessage() InSendMessage
@ stdcall InflateRect(ptr long long) InflateRect
@ stdcall InsertMenuA(long long long long ptr) InsertMenuA
@ stdcall InsertMenuItemA(long long long ptr) InsertMenuItemA
@ stdcall InsertMenuItemW(long long long ptr) InsertMenuItemW
@ stdcall InsertMenuW(long long long long ptr) InsertMenuW
@ stdcall InternalGetWindowText(long long long) InternalGetWindowText
@ stdcall IntersectRect(ptr ptr ptr) IntersectRect
@ stdcall InvalidateRect(long ptr long) InvalidateRect
@ stdcall InvalidateRgn(long long long) InvalidateRgn
@ stdcall InvertRect(long ptr) InvertRect
@ stdcall IsCharAlphaA(long) IsCharAlphaA
@ stdcall IsCharAlphaNumericA(long) IsCharAlphaNumericA
@ stdcall IsCharAlphaNumericW(long) IsCharAlphaNumericW
@ stdcall IsCharAlphaW(long) IsCharAlphaW
@ stdcall IsCharLowerA(long) IsCharLowerA
@ stdcall IsCharLowerW(long) IsCharLowerW
@ stdcall IsCharUpperA(long) IsCharUpperA
@ stdcall IsCharUpperW(long) IsCharUpperW
@ stdcall IsChild(long long) IsChild
@ stdcall IsClipboardFormatAvailable(long) IsClipboardFormatAvailable
@ stub IsDialogMessage
@ stdcall IsDialogMessageA(long ptr) IsDialogMessageA
@ stdcall IsDialogMessageW(long ptr) IsDialogMessageW
@ stdcall IsDlgButtonChecked(long long) IsDlgButtonChecked
@ stdcall IsIconic(long) IsIconic
@ stdcall IsMenu(long) IsMenu
@ stdcall IsRectEmpty(ptr) IsRectEmpty
@ stdcall IsWindow(long) IsWindow
@ stdcall IsWindowEnabled(long) IsWindowEnabled
@ stdcall IsWindowUnicode(long) IsWindowUnicode
@ stdcall IsWindowVisible(long) IsWindowVisible
@ stdcall IsZoomed(long) IsZoomed
@ stdcall KillSystemTimer(long long) KillSystemTimer
@ stdcall KillTimer(long long) KillTimer
@ stdcall LoadAcceleratorsA(long str) LoadAcceleratorsA
@ stdcall LoadAcceleratorsW(long wstr) LoadAcceleratorsW
@ stdcall LoadBitmapA(long str) LoadBitmapA
@ stdcall LoadBitmapW(long wstr) LoadBitmapW
@ stdcall LoadCursorA(long str) LoadCursorA
@ stdcall LoadCursorFromFileA(str) LoadCursorFromFileA
@ stdcall LoadCursorFromFileW(wstr) LoadCursorFromFileW
@ stdcall LoadCursorW(long wstr) LoadCursorW
@ stdcall LoadIconA(long str) LoadIconA
@ stdcall LoadIconW(long wstr) LoadIconW
@ stdcall LoadImageA(long str long long long long) LoadImageA
@ stdcall LoadImageW(long wstr long long long long) LoadImageW
@ stdcall LoadKeyboardLayoutA(str long) LoadKeyboardLayoutA
@ stdcall LoadKeyboardLayoutW(wstr long) LoadKeyboardLayoutW
@ stdcall LoadLocalFonts() LoadLocalFonts
@ stdcall LoadMenuA(long str) LoadMenuA
@ stdcall LoadMenuIndirectA(ptr) LoadMenuIndirectA
@ stdcall LoadMenuIndirectW(ptr) LoadMenuIndirectW
@ stdcall LoadMenuW(long wstr) LoadMenuW
@ stub LoadRemoteFonts
@ stdcall LoadStringA(long long ptr long) LoadStringA
@ stdcall LoadStringW(long long ptr long) LoadStringW
@ stub LockWindowStation
@ stdcall LockWindowUpdate(long) LockWindowUpdate
@ stdcall LookupIconIdFromDirectory(ptr long) LookupIconIdFromDirectory
@ stdcall LookupIconIdFromDirectoryEx(ptr long long long long) LookupIconIdFromDirectoryEx
@ stub MBToWCSEx
@ stdcall MapDialogRect(long ptr) MapDialogRect
@ stdcall MapVirtualKeyA(long long) MapVirtualKeyA
@ stdcall MapVirtualKeyExA(long long long) MapVirtualKeyExA
@ stdcall MapVirtualKeyW(long long) MapVirtualKeyA
@ stdcall MapWindowPoints(long long ptr long) MapWindowPoints
@ stdcall MenuItemFromPoint(long long long long) MenuItemFromPoint
@ stub MenuWindowProcA
@ stub MenuWindowProcW
@ stdcall MessageBeep(long) MessageBeep
@ stdcall MessageBoxA(long str str long) MessageBoxA
@ stdcall MessageBoxExA(long str str long long) MessageBoxExA
@ stdcall MessageBoxExW(long wstr wstr long long) MessageBoxExW
@ stdcall MessageBoxIndirectA(ptr) MessageBoxIndirectA
@ stdcall MessageBoxIndirectW(ptr) MessageBoxIndirectW
@ stdcall MessageBoxW(long wstr wstr long) MessageBoxW
@ stdcall ModifyMenuA(long long long long ptr) ModifyMenuA
@ stdcall ModifyMenuW(long long long long ptr) ModifyMenuW
@ stdcall MoveWindow(long long long long long long) MoveWindow
@ stdcall MsgWaitForMultipleObjects(long ptr long long long) MsgWaitForMultipleObjects
@ stdcall OemKeyScan(long) OemKeyScan
@ stdcall OemToCharA(ptr ptr) OemToCharA
@ stdcall OemToCharBuffA(ptr ptr long) OemToCharBuffA
@ stdcall OemToCharBuffW(ptr ptr long) OemToCharBuffW
@ stdcall OemToCharW(ptr ptr) OemToCharW
@ stdcall OffsetRect(ptr long long) OffsetRect
@ stdcall OpenClipboard(long) OpenClipboard
@ stdcall OpenDesktopA(str long long long) OpenDesktopA
@ stub OpenDesktopW
@ stdcall OpenIcon(long) OpenIcon
@ stub OpenInputDesktop
@ stub OpenWindowStationA
@ stub OpenWindowStationW
@ stdcall PackDDElParam(long long long) PackDDElParam
@ stdcall PaintDesktop(long) PaintDesktop
@ stdcall PeekMessageA(ptr long long long long) PeekMessageA
@ stdcall PeekMessageW(ptr long long long long) PeekMessageW
@ stub PlaySoundEvent
@ stdcall PostMessageA(long long long long) PostMessageA
@ stdcall PostMessageW(long long long long) PostMessageW
@ stdcall PostQuitMessage(long) PostQuitMessage
@ stdcall PostThreadMessageA(long long long long) PostThreadMessageA
@ stdcall PostThreadMessageW(long long long long) PostThreadMessageW
@ stdcall PtInRect(ptr long long) PtInRect
@ stub QuerySendMessage
@ stdcall RedrawWindow(long ptr long long) RedrawWindow
@ stdcall RegisterClassA(ptr) RegisterClassA
@ stdcall RegisterClassExA(ptr) RegisterClassExA
@ stdcall RegisterClassExW(ptr) RegisterClassExW
@ stdcall RegisterClassW(ptr) RegisterClassW
@ stdcall RegisterClipboardFormatA(str) RegisterClipboardFormatA
@ stdcall RegisterClipboardFormatW(wstr) RegisterClipboardFormatW
@ stdcall RegisterHotKey(long long long long) RegisterHotKey
@ stdcall RegisterLogonProcess(long long) RegisterLogonProcess
@ stdcall RegisterSystemThread(long long) RegisterSystemThread
@ stdcall RegisterTasklist (long) RegisterTaskList
@ stdcall RegisterWindowMessageA(str) RegisterWindowMessageA
@ stdcall RegisterWindowMessageW(wstr) RegisterWindowMessageW
@ stdcall ReleaseCapture() ReleaseCapture
@ stdcall ReleaseDC(long long) ReleaseDC
@ stdcall RemoveMenu(long long long) RemoveMenu
@ stdcall RemovePropA(long str) RemovePropA
@ stdcall RemovePropW(long wstr) RemovePropW
@ stdcall ReplyMessage(long) ReplyMessage
@ stub ResetDisplay
@ stdcall ReuseDDElParam(long long long long long) ReuseDDElParam
@ stdcall ScreenToClient(long ptr) ScreenToClient
@ stdcall ScrollChildren(long long long long) ScrollChildren
@ stdcall ScrollDC(long long long ptr ptr long ptr) ScrollDC
@ stdcall ScrollWindow(long long long ptr ptr) ScrollWindow
@ stdcall ScrollWindowEx(long long long ptr ptr long ptr long) ScrollWindowEx
@ stdcall SendDlgItemMessageA(long long long long long) SendDlgItemMessageA
@ stdcall SendDlgItemMessageW(long long long long long) SendDlgItemMessageW
@ stdcall SendMessageA(long long long long) SendMessageA
@ stdcall SendMessageCallbackA(long long long long ptr long) SendMessageCallbackA
@ stdcall SendMessageCallbackW(long long long long ptr long) SendMessageCallbackW
@ stdcall SendMessageTimeoutA(long long long long long long ptr) SendMessageTimeoutA
@ stdcall SendMessageTimeoutW(long long long long long long ptr) SendMessageTimeoutW
@ stdcall SendMessageW(long long long long) SendMessageW
@ stdcall SendNotifyMessageA(long long long long) SendNotifyMessageA
@ stdcall SendNotifyMessageW(long long long long) SendNotifyMessageW
@ stub ServerSetFunctionPointers
@ stdcall SetActiveWindow(long) SetActiveWindow
@ stdcall SetCapture(long) SetCapture
@ stdcall SetCaretBlinkTime(long) SetCaretBlinkTime
@ stdcall SetCaretPos(long long) SetCaretPos
@ stdcall SetClassLongA(long long long) SetClassLongA
@ stdcall SetClassLongW(long long long) SetClassLongW
@ stdcall SetClassWord(long long long) SetClassWord
@ stdcall SetClipboardData(long long) SetClipboardData
@ stdcall SetClipboardViewer(long) SetClipboardViewer
@ stdcall SetCursor(long) SetCursor
@ stub SetCursorContents
@ stdcall SetCursorPos(long long) SetCursorPos
@ stdcall SetDebugErrorLevel(long) SetDebugErrorLevel
@ stdcall SetDeskWallPaper(str) SetDeskWallPaper
@ stdcall SetDlgItemInt(long long long long) SetDlgItemInt
@ stdcall SetDlgItemTextA(long long str) SetDlgItemTextA
@ stdcall SetDlgItemTextW(long long wstr) SetDlgItemTextW
@ stdcall SetDoubleClickTime(long) SetDoubleClickTime
@ stdcall SetFocus(long) SetFocus
@ stdcall SetForegroundWindow(long) SetForegroundWindow
@ stdcall SetInternalWindowPos(long long ptr ptr) SetInternalWindowPos
@ stdcall SetKeyboardState(ptr) SetKeyboardState
@ stdcall SetLastErrorEx(long long) SetLastErrorEx
@ stdcall SetLogonNotifyWindow(long long) SetLogonNotifyWindow
@ stdcall SetMenu(long long) SetMenu
@ stdcall SetMenuContextHelpId(long long) SetMenuContextHelpId
@ stdcall SetMenuDefaultItem(long long long) SetMenuDefaultItem
@ stdcall SetMenuItemBitmaps(long long long long long) SetMenuItemBitmaps
@ stdcall SetMenuItemInfoA(long long long ptr) SetMenuItemInfoA
@ stdcall SetMenuItemInfoW(long long long ptr) SetMenuItemInfoW
@ stub SetMessageExtraInfo
@ stdcall SetMessageQueue(long) SetMessageQueue
@ stdcall SetParent(long long) SetParent
@ stdcall SetProcessWindowStation(long) SetProcessWindowStation
@ stdcall SetPropA(long str long) SetPropA
@ stdcall SetPropW(long wstr long) SetPropW
@ stdcall SetRect(ptr long long long long) SetRect
@ stdcall SetRectEmpty(ptr) SetRectEmpty
@ stdcall SetScrollInfo(long long ptr long) SetScrollInfo
@ stdcall SetScrollPos(long long long long) SetScrollPos
@ stdcall SetScrollRange(long long long long long) SetScrollRange
@ stdcall SetShellWindow(long) SetShellWindow
@ stdcall SetSysColors(long ptr ptr) SetSysColors
@ stub SetSysColorsTemp
@ stdcall SetSystemCursor(long long) SetSystemCursor
@ stdcall SetSystemMenu(long long) SetSystemMenu
@ stdcall SetSystemTimer(long long long ptr) SetSystemTimer
@ stdcall SetThreadDesktop(long) SetThreadDesktop
@ stdcall SetTimer(long long long ptr) SetTimer
@ stdcall SetUserObjectInformationA(long long long long) SetUserObjectInformationA
@ stub SetUserObjectInformationW
@ stdcall SetUserObjectSecurity(long ptr ptr) SetUserObjectSecurity
@ stdcall SetWindowContextHelpId(long long) SetWindowContextHelpId
@ stub SetWindowFullScreenState
@ stdcall SetWindowLongA(long long long) SetWindowLongA
@ stdcall SetWindowLongW(long long long) SetWindowLongW
@ stdcall SetWindowPlacement(long ptr) SetWindowPlacement
@ stdcall SetWindowPos(long long long long long long long) SetWindowPos
@ stdcall SetWindowStationUser(long long) SetWindowStationUser
@ stdcall SetWindowTextA(long str) SetWindowTextA
@ stdcall SetWindowTextW(long wstr) SetWindowTextW
@ stdcall SetWindowWord(long long long) SetWindowWord
@ stdcall SetWindowsHookA(long ptr) SetWindowsHookA
@ stdcall SetWindowsHookExA(long long long long) SetWindowsHookExA
@ stdcall SetWindowsHookExW(long long long long) SetWindowsHookExW
@ stdcall SetWindowsHookW(long ptr) SetWindowsHookW
@ stdcall ShowCaret(long) ShowCaret
@ stdcall ShowCursor(long) ShowCursor
@ stdcall ShowOwnedPopups(long long) ShowOwnedPopups
@ stdcall ShowScrollBar(long long long) ShowScrollBar
@ stub ShowStartGlass
@ stdcall ShowWindow(long long) ShowWindow
@ stdcall ShowWindowAsync(long long) ShowWindowAsync
@ stdcall SubtractRect(ptr ptr ptr) SubtractRect
@ stdcall SwapMouseButton(long) SwapMouseButton
@ stub SwitchDesktop
@ stdcall SwitchToThisWindow(long long) SwitchToThisWindow
@ stdcall SystemParametersInfoA(long long ptr long) SystemParametersInfoA
@ stdcall SystemParametersInfoW(long long ptr long) SystemParametersInfoW
@ stdcall TabbedTextOutA(long long long str long long ptr long) TabbedTextOutA
@ stdcall TabbedTextOutW(long long long wstr long long ptr long) TabbedTextOutW
@ stub TileChildWindows
@ stdcall TileWindows(long long ptr long ptr) TileWindows
@ stdcall ToAscii(long long ptr ptr long) ToAscii
@ stdcall ToAsciiEx(long long ptr ptr long long) ToAsciiEx
@ stdcall ToUnicode(long long ptr wstr long long) ToUnicode
@ stdcall TrackPopupMenu(long long long long long long ptr) TrackPopupMenu
@ stdcall TrackPopupMenuEx(long long long long long ptr) TrackPopupMenuEx
@ stdcall TranslateAccelerator(long long ptr) TranslateAccelerator
@ stdcall TranslateAcceleratorA(long long ptr) TranslateAccelerator
@ stdcall TranslateAcceleratorW(long long ptr) TranslateAccelerator
@ stdcall TranslateCharsetInfo(ptr ptr long) TranslateCharsetInfo
@ stdcall TranslateMDISysAccel(long ptr) TranslateMDISysAccel
@ stdcall TranslateMessage(ptr) TranslateMessage
@ stdcall UnhookWindowsHook(long ptr) UnhookWindowsHook
@ stdcall UnhookWindowsHookEx(long) UnhookWindowsHookEx
@ stdcall UnionRect(ptr ptr ptr) UnionRect
@ stub UnloadKeyboardLayout
@ stub UnlockWindowStation
@ stdcall UnpackDDElParam(long long ptr ptr) UnpackDDElParam
@ stdcall UnregisterClassA(str long) UnregisterClassA
@ stdcall UnregisterClassW(wstr long) UnregisterClassW
@ stdcall UnregisterHotKey(long long) UnregisterHotKey
@ stub UpdatePerUserSystemParameters
@ stdcall UpdateWindow(long) UpdateWindow
@ stub UserClientDllInitialize
@ stub UserRealizePalette
@ stub UserRegisterWowHandlers
@ stdcall ValidateRect(long ptr) ValidateRect
@ stdcall ValidateRgn(long long) ValidateRgn
@ stdcall VkKeyScanA(long) VkKeyScanA
@ stdcall VkKeyScanExA(long long) VkKeyScanExA
@ stdcall VkKeyScanExW(long long) VkKeyScanExW
@ stdcall VkKeyScanW(long) VkKeyScanW
@ stdcall WaitForInputIdle(long long) WaitForInputIdle
@ stdcall WaitMessage() WaitMessage
@ stdcall WinHelpA(long str long long) WinHelpA
@ stdcall WinHelpW(long wstr long long) WinHelpW
@ stdcall WindowFromDC(long) WindowFromDC
@ stdcall WindowFromPoint(long long) WindowFromPoint
@ stdcall keybd_event(long long long long) keybd_event
@ stdcall mouse_event(long long long long long) mouse_event
@ varargs wsprintfA(str str) wsprintfA
@ varargs wsprintfW(wstr wstr) wsprintfW
@ stdcall wvsprintfA(ptr str ptr) wvsprintfA
@ stdcall wvsprintfW(ptr wstr ptr) wvsprintfW

#late additions
@ stdcall ChangeDisplaySettingsA(ptr long) ChangeDisplaySettingsA
@ stub ChangeDisplaySettingsW
@ stdcall EnumDesktopWindows(long ptr ptr) EnumDesktopWindows
@ stdcall EnumDisplaySettingsA(str long ptr) EnumDisplaySettingsA
@ stdcall EnumDisplaySettingsW(wstr long ptr ) EnumDisplaySettingsW
@ stdcall GetWindowRgn(long long) GetWindowRgn
@ stub MapVirtualKeyExW
@ stub RegisterServicesProcess
@ stdcall SetWindowRgn(long long long) SetWindowRgn
@ stub ToUnicodeEx
@ stdcall DrawCaptionTempA(long long ptr long long str long) DrawCaptionTempA
@ stub RegisterNetworkCapabilities
@ stub WNDPROC_CALLBACK
@ stdcall DrawCaptionTempW(long long ptr long long wstr long) DrawCaptionTempW
@ stub IsHungAppWindow
@ stdcall ChangeDisplaySettingsExA(str ptr long long ptr) ChangeDisplaySettingsExA
@ stub ChangeDisplaySettingsExW
@ stdcall SetWindowText(long str) SetWindowTextA
@ stdcall GetMonitorInfoA(long ptr) GetMonitorInfoA
@ stdcall GetMonitorInfoW(long ptr) GetMonitorInfoW
@ stdcall MonitorFromWindow(long long) MonitorFromWindow
@ stdcall MonitorFromRect(ptr long) MonitorFromRect
@ stdcall MonitorFromPoint(long long long) MonitorFromPoint
@ stdcall EnumDisplayMonitors(long ptr ptr long) EnumDisplayMonitors
@ stdcall PrivateExtractIconExA (long long long long long) PrivateExtractIconExA
@ stdcall PrivateExtractIconExW (long long long long long) PrivateExtractIconExW
@ stdcall PrivateExtractIconsW (long long long long long long long long) PrivateExtractIconsW
@ stdcall RegisterShellHookWindow (long) RegisterShellHookWindow
@ stdcall DeregisterShellHookWindow (long) DeregisterShellHookWindow
@ stdcall SetShellWindowEx (long long) SetShellWindowEx
@ stdcall SetProgmanWindow (long) SetProgmanWindow
@ stdcall GetTaskmanWindow () GetTaskmanWindow
@ stdcall SetTaskmanWindow (long) SetTaskmanWindow
@ stdcall GetProgmanWindow () GetProgmanWindow
@ stdcall UserSignalProc(long long long long) UserSignalProc

# win98
@ stdcall GetMenuInfo(long ptr)GetMenuInfo
@ stdcall SetMenuInfo(long ptr)SetMenuInfo
@ stdcall GetProcessDefaultLayout(ptr) GetProcessDefaultLayout
@ stdcall SetProcessDefaultLayout(long) SetProcessDefaultLayout
@ stdcall RegisterDeviceNotificationA(long ptr long) RegisterDeviceNotificationA
@ stub    RegisterDeviceNotificationW
@ stub    UnregisterDeviceNotification
