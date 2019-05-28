@ stdcall GetClassInfoA(long str ptr) user32.GetClassInfoA
@ stdcall GetClassInfoExA(long str ptr) user32.GetClassInfoExA
@ stdcall GetClassInfoExW(long wstr ptr) user32.GetClassInfoExW
@ stdcall GetClassInfoW(long wstr ptr) user32.GetClassInfoW
@ stdcall GetClassNameA(long ptr long) user32.GetClassNameA
@ stdcall GetClassNameW(long ptr long) user32.GetClassNameW
@ stdcall GetWindowLongA(long long) user32.GetWindowLongA
@ stdcall -arch=win64 GetWindowLongPtrW(long long) user32.GetWindowLongPtrW
@ stdcall GetWindowLongW(long long) user32.GetWindowLongW
@ stdcall RegisterClassA(ptr) user32.RegisterClassA
@ stdcall RegisterClassExW(ptr) user32.RegisterClassExW
@ stdcall RegisterClassW(ptr) user32.RegisterClassW
@ stdcall SetWindowLongA(long long long) user32.SetWindowLongA
@ stdcall -arch=win64 SetWindowLongPtrW(long long long) user32.SetWindowLongPtrW
@ stdcall SetWindowLongW(long long long) user32.SetWindowLongW
@ stdcall UnregisterClassA(str long) user32.UnregisterClassA
@ stdcall UnregisterClassW(wstr long) user32.UnregisterClassW
