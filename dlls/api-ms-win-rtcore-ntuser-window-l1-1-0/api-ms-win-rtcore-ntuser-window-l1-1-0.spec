@ stdcall AllowSetForegroundWindow(long) user32.AllowSetForegroundWindow
@ stdcall BeginDeferWindowPos(long) user32.BeginDeferWindowPos
@ stdcall CallWindowProcW(ptr long long long long) user32.CallWindowProcW
@ stdcall ChildWindowFromPoint(long int64) user32.ChildWindowFromPoint
@ stdcall ChildWindowFromPointEx(long int64 long) user32.ChildWindowFromPointEx
@ stdcall ClientToScreen(long ptr) user32.ClientToScreen
@ stdcall CreateWindowExW(long wstr wstr long long long long long long long long ptr) user32.CreateWindowExW
@ stdcall DeferWindowPos(long long long long long long long long) user32.DeferWindowPos
@ stdcall DefWindowProcW(long long long long) user32.DefWindowProcW
@ stdcall DestroyWindow(long) user32.DestroyWindow
@ stdcall DispatchMessageW(ptr) user32.DispatchMessageW
@ stdcall EnableWindow(long long) user32.EnableWindow
@ stdcall EndDeferWindowPos(long) user32.EndDeferWindowPos
@ stdcall EnumChildWindows(long ptr long) user32.EnumChildWindows
@ stdcall EnumPropsExW(long ptr long) user32.EnumPropsExW
@ stdcall EnumPropsW(long ptr) user32.EnumPropsW
@ stdcall EnumWindows(ptr long) user32.EnumWindows
@ stdcall FindWindowExW(long long wstr wstr) user32.FindWindowExW
@ stdcall FindWindowW(wstr wstr) user32.FindWindowW
@ stdcall GetActiveWindow() user32.GetActiveWindow
@ stdcall GetAncestor(long long) user32.GetAncestor
@ stdcall GetClassInfoExW(long wstr ptr) user32.GetClassInfoExW
@ stdcall GetClassInfoW(long wstr ptr) user32.GetClassInfoW
@ stdcall GetClassNameW(long ptr long) user32.GetClassNameW
@ stdcall GetClientRect(long long) user32.GetClientRect
@ stdcall GetCursorPos(ptr) user32.GetCursorPos
@ stdcall GetDesktopWindow() user32.GetDesktopWindow
@ stdcall GetFocus() user32.GetFocus
@ stdcall GetForegroundWindow() user32.GetForegroundWindow
@ stdcall GetMessageExtraInfo() user32.GetMessageExtraInfo
@ stdcall GetMessagePos() user32.GetMessagePos
@ stdcall GetMessageTime() user32.GetMessageTime
@ stdcall GetMessageW(ptr long long long) user32.GetMessageW
@ stdcall GetParent(long) user32.GetParent
@ stdcall GetPropW(long wstr) user32.GetPropW
@ stdcall GetQueueStatus(long) user32.GetQueueStatus
@ stdcall GetTopWindow(long) user32.GetTopWindow
@ stdcall GetWindow(long long) user32.GetWindow
@ stdcall GetWindowLongA(long long) user32.GetWindowLongA
@ stdcall GetWindowLongW(long long) user32.GetWindowLongW
@ stdcall GetWindowRect(long ptr) user32.GetWindowRect
@ stdcall GetWindowTextW(long ptr long) user32.GetWindowTextW
@ stdcall GetWindowThreadProcessId(long ptr) user32.GetWindowThreadProcessId
@ stdcall InSendMessage() user32.InSendMessage
@ stdcall InSendMessageEx(ptr) user32.InSendMessageEx
@ stdcall IsChild(long long) user32.IsChild
@ stdcall IsWindow(long) user32.IsWindow
@ stdcall IsWindowEnabled(long) user32.IsWindowEnabled
@ stdcall IsWindowVisible(long) user32.IsWindowVisible
@ stdcall KillTimer(long long) user32.KillTimer
@ stdcall MoveWindow(long long long long long long) user32.MoveWindow
@ stdcall PeekMessageW(ptr long long long long) user32.PeekMessageW
@ stdcall PostMessageW(long long long long) user32.PostMessageW
@ stdcall PostQuitMessage(long) user32.PostQuitMessage
@ stdcall PostThreadMessageW(long long long long) user32.PostThreadMessageW
@ stdcall RegisterClassExW(ptr) user32.RegisterClassExW
@ stdcall RegisterClassW(ptr) user32.RegisterClassW
@ stdcall RegisterWindowMessageW(wstr) user32.RegisterWindowMessageW
@ stdcall RemovePropW(long wstr) user32.RemovePropW
@ stdcall ScreenToClient(long ptr) user32.ScreenToClient
@ stdcall SendMessageCallbackW(long long long long ptr long) user32.SendMessageCallbackW
@ stdcall SendMessageTimeoutW(long long long long long long ptr) user32.SendMessageTimeoutW
@ stdcall SendMessageW(long long long long) user32.SendMessageW
@ stdcall SendNotifyMessageW(long long long long) user32.SendNotifyMessageW
@ stdcall SetActiveWindow(long) user32.SetActiveWindow
@ stdcall SetCursorPos(long long) user32.SetCursorPos
@ stdcall SetFocus(long) user32.SetFocus
@ stdcall SetForegroundWindow(long) user32.SetForegroundWindow
@ stdcall SetMessageExtraInfo(long) user32.SetMessageExtraInfo
@ stdcall SetParent(long long) user32.SetParent
@ stdcall SetPropW(long wstr long) user32.SetPropW
@ stdcall SetTimer(long long long ptr) user32.SetTimer
@ stdcall SetWindowLongA(long long long) user32.SetWindowLongA
@ stdcall SetWindowLongW(long long long) user32.SetWindowLongW
@ stdcall SetWindowPos(long long long long long long long) user32.SetWindowPos
@ stdcall SetWindowTextW(long wstr) user32.SetWindowTextW
@ stdcall ShowWindow(long long) user32.ShowWindow
@ stdcall TranslateMessage(ptr) user32.TranslateMessage
@ stdcall UnregisterClassW(wstr long) user32.UnregisterClassW
@ stdcall WaitMessage() user32.WaitMessage
@ stdcall WindowFromPoint(int64) user32.WindowFromPoint
