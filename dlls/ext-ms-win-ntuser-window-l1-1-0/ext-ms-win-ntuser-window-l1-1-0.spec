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
@ stdcall FindWindowW(wstr wstr) user32.FindWindowW
@ stdcall GetClientRect(long long) user32.GetClientRect
@ stdcall GetDesktopWindow() user32.GetDesktopWindow
@ stdcall GetForegroundWindow() user32.GetForegroundWindow
@ stdcall GetParent(long) user32.GetParent
@ stdcall GetPropW(long wstr) user32.GetPropW
@ stdcall GetShellWindow() user32.GetShellWindow
@ stdcall GetWindow(long long) user32.GetWindow
@ stdcall GetWindowInfo(long ptr) user32.GetWindowInfo
@ stdcall GetWindowPlacement(long ptr) user32.GetWindowPlacement
@ stdcall GetWindowRect(long ptr) user32.GetWindowRect
@ stdcall GetWindowTextW(long ptr long) user32.GetWindowTextW
@ stdcall GetWindowThreadProcessId(long ptr) user32.GetWindowThreadProcessId
@ stdcall IsChild(long long) user32.IsChild
@ stdcall IsIconic(long) user32.IsIconic
@ stdcall IsWindow(long) user32.IsWindow
@ stdcall RemovePropW(long wstr) user32.RemovePropW
@ stdcall SetForegroundWindow(long) user32.SetForegroundWindow
@ stdcall SetParent(long long) user32.SetParent
@ stdcall SetPropW(long wstr long) user32.SetPropW
@ stdcall SetWindowPos(long long long long long long long) user32.SetWindowPos
@ stdcall SetWindowsHookExW(long long long long) user32.SetWindowsHookExW
@ stdcall SetWindowTextW(long wstr) user32.SetWindowTextW
@ stdcall ShowWindow(long long) user32.ShowWindow
@ stub SoundSentry
@ stdcall UnhookWindowsHookEx(long) user32.UnhookWindowsHookEx
