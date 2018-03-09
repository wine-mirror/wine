@ stdcall AdjustWindowRectEx(ptr long long long) user32.AdjustWindowRectEx
@ stdcall AllowSetForegroundWindow(long) user32.AllowSetForegroundWindow
@ stdcall AnimateWindow(long long long) user32.AnimateWindow
@ stdcall BringWindowToTop(long) user32.BringWindowToTop
@ stdcall CallNextHookEx(long long long long) user32.CallNextHookEx
@ stdcall CreateWindowExA(long str str long long long long long long long long ptr) user32.CreateWindowExA
@ stdcall CreateWindowExW(long wstr wstr long long long long long long long long ptr) user32.CreateWindowExW
@ stdcall DefWindowProcA(long long long long) user32.DefWindowProcA
@ stdcall DefWindowProcW(long long long long) user32.DefWindowProcW
@ stdcall DestroyWindow(long) user32.DestroyWindow
@ stdcall EnumChildWindows(long ptr long) user32.EnumChildWindows
@ stdcall EnumThreadWindows(long ptr long) user32.EnumThreadWindows
@ stdcall EnumWindows(ptr long) user32.EnumWindows
@ stdcall FindWindowA(str str) user32.FindWindowA
@ stdcall FindWindowExW(long long wstr wstr) user32.FindWindowExW
@ stdcall FindWindowW(wstr wstr) user32.FindWindowW
@ stdcall GetAncestor(long long) user32.GetAncestor
@ stdcall GetClientRect(long long) user32.GetClientRect
@ stdcall GetDesktopWindow() user32.GetDesktopWindow
@ stdcall GetForegroundWindow() user32.GetForegroundWindow
@ stdcall GetGUIThreadInfo(long ptr) user32.GetGUIThreadInfo
@ stdcall GetLayeredWindowAttributes(long ptr ptr ptr) user32.GetLayeredWindowAttributes
@ stdcall GetParent(long) user32.GetParent
@ stdcall GetPropW(long wstr) user32.GetPropW
@ stdcall GetShellWindow() user32.GetShellWindow
@ stdcall GetWindow(long long) user32.GetWindow
@ stdcall GetWindowDisplayAffinity(long ptr) user32.GetWindowDisplayAffinity
@ stdcall GetWindowInfo(long ptr) user32.GetWindowInfo
@ stdcall GetWindowPlacement(long ptr) user32.GetWindowPlacement
@ stdcall GetWindowRect(long ptr) user32.GetWindowRect
@ stdcall GetWindowTextLengthW(long) user32.GetWindowTextLengthW
@ stdcall GetWindowTextW(long ptr long) user32.GetWindowTextW
@ stdcall GetWindowThreadProcessId(long ptr) user32.GetWindowThreadProcessId
@ stdcall IsChild(long long) user32.IsChild
@ stdcall IsIconic(long) user32.IsIconic
@ stdcall IsWindow(long) user32.IsWindow
@ stdcall IsWindowUnicode(long) user32.IsWindowUnicode
@ stdcall IsWindowVisible(long) user32.IsWindowVisible
@ stdcall LogicalToPhysicalPoint(long ptr) user32.LogicalToPhysicalPoint
@ stdcall MoveWindow(long long long long long long) user32.MoveWindow
@ stdcall RemovePropW(long wstr) user32.RemovePropW
@ stdcall SetForegroundWindow(long) user32.SetForegroundWindow
@ stdcall SetLayeredWindowAttributes(ptr long long long) user32.SetLayeredWindowAttributes
@ stdcall SetParent(long long) user32.SetParent
@ stdcall SetPropW(long wstr long) user32.SetPropW
@ stdcall SetWindowDisplayAffinity(long long) user32.SetWindowDisplayAffinity
@ stdcall SetWindowPos(long long long long long long long) user32.SetWindowPos
@ stdcall SetWindowTextW(long wstr) user32.SetWindowTextW
@ stdcall SetWindowsHookExW(long long long long) user32.SetWindowsHookExW
@ stdcall ShowWindow(long long) user32.ShowWindow
@ stub SoundSentry
@ stdcall UnhookWindowsHookEx(long) user32.UnhookWindowsHookEx
@ stdcall UpdateLayeredWindow(long long ptr ptr long ptr long ptr long) user32.UpdateLayeredWindow
@ stub WindowFromPhysicalPoint
@ stdcall WindowFromPoint(int64) user32.WindowFromPoint
