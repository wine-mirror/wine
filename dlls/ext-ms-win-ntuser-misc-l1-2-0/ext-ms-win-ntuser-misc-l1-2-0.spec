@ stdcall AttachThreadInput(long long long) user32.AttachThreadInput
@ stdcall CloseClipboard() user32.CloseClipboard
@ stub CloseGestureInfoHandle
@ stdcall DrawTextExW(long wstr long ptr long ptr) user32.DrawTextExW
@ stdcall DrawTextW(long wstr long ptr long) user32.DrawTextW
@ stdcall EmptyClipboard() user32.EmptyClipboard
@ stdcall EnableScrollBar(long long long) user32.EnableScrollBar
@ stdcall ExitWindowsEx(long long) user32.ExitWindowsEx
@ stdcall GetClipboardData(long) user32.GetClipboardData
@ stdcall GetClipboardFormatNameW(long ptr long) user32.GetClipboardFormatNameW
@ stdcall GetClipboardOwner() user32.GetClipboardOwner
@ stdcall GetGestureConfig(long long long ptr ptr long) user32.GetGestureConfig
@ stdcall GetGestureInfo(long ptr) user32.GetGestureInfo
@ stdcall GetGuiResources(long long) user32.GetGuiResources
@ stdcall GetRawInputData(ptr long ptr ptr long) user32.GetRawInputData
@ stdcall GetRawInputDeviceInfoW(ptr long ptr ptr) user32.GetRawInputDeviceInfoW
@ stdcall GetRawInputDeviceList(ptr ptr long) user32.GetRawInputDeviceList
@ stdcall GetScrollBarInfo(long long ptr) user32.GetScrollBarInfo
@ stdcall GetScrollInfo(long long ptr) user32.GetScrollInfo
@ stdcall GetScrollPos(long long) user32.GetScrollPos
@ stdcall GetTouchInputInfo(long long ptr long) user32.GetTouchInputInfo
@ stdcall GetUserObjectSecurity(long ptr ptr long ptr) user32.GetUserObjectSecurity
@ stub GetWindowFeedbackSetting
@ stdcall IsClipboardFormatAvailable(long) user32.IsClipboardFormatAvailable
@ stdcall IsTouchWindow(long ptr) user32.IsTouchWindow
@ stdcall KillTimer(long long) user32.KillTimer
@ stdcall MessageBeep(long) user32.MessageBeep
@ stdcall OpenClipboard(long) user32.OpenClipboard
@ stdcall RegisterClipboardFormatA(str) user32.RegisterClipboardFormatA
@ stdcall RegisterClipboardFormatW(wstr) user32.RegisterClipboardFormatW
@ stdcall RegisterDeviceNotificationW(long ptr long) user32.RegisterDeviceNotificationW
@ stdcall RegisterRawInputDevices(ptr long long) user32.RegisterRawInputDevices
@ stdcall ScrollWindowEx(long long long ptr ptr long ptr long) user32.ScrollWindowEx
@ stdcall SetClipboardData(long long) user32.SetClipboardData
@ stdcall SetCoalescableTimer(long long long ptr long) user32.SetCoalescableTimer
@ stdcall SetScrollInfo(long long ptr long) user32.SetScrollInfo
@ stdcall SetScrollPos(long long long long) user32.SetScrollPos
@ stdcall SetScrollRange(long long long long long) user32.SetScrollRange
@ stdcall SetTimer(long long long ptr) user32.SetTimer
@ stdcall SetUserObjectSecurity(long ptr ptr) user32.SetUserObjectSecurity
@ stub SetWindowFeedbackSetting
@ stdcall ShowScrollBar(long long long) user32.ShowScrollBar
@ stdcall ShutdownBlockReasonCreate(long wstr) user32.ShutdownBlockReasonCreate
@ stdcall ShutdownBlockReasonDestroy(long) user32.ShutdownBlockReasonDestroy
@ stdcall TabbedTextOutW(long long long wstr long long ptr long) user32.TabbedTextOutW
@ stdcall UnregisterDeviceNotification(long) user32.UnregisterDeviceNotification
@ stdcall WaitForInputIdle(long long) user32.WaitForInputIdle
@ stdcall WinHelpW(long wstr long long) user32.WinHelpW
