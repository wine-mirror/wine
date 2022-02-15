@ stdcall AttachConsole(long) kernelbase.AttachConsole
@ stdcall CreateConsoleScreenBuffer(long long ptr long ptr) kernelbase.CreateConsoleScreenBuffer
@ stdcall FillConsoleOutputAttribute(long long long long ptr) kernelbase.FillConsoleOutputAttribute
@ stdcall FillConsoleOutputCharacterA(long long long long ptr) kernelbase.FillConsoleOutputCharacterA
@ stdcall FillConsoleOutputCharacterW(long long long long ptr) kernelbase.FillConsoleOutputCharacterW
@ stdcall FlushConsoleInputBuffer(long) kernelbase.FlushConsoleInputBuffer
@ stdcall FreeConsole() kernelbase.FreeConsole
@ stdcall GenerateConsoleCtrlEvent(long long) kernelbase.GenerateConsoleCtrlEvent
@ stdcall GetConsoleCursorInfo(long ptr) kernelbase.GetConsoleCursorInfo
@ stdcall GetConsoleScreenBufferInfo(long ptr) kernelbase.GetConsoleScreenBufferInfo
@ stdcall GetConsoleScreenBufferInfoEx(long ptr) kernelbase.GetConsoleScreenBufferInfoEx
@ stdcall GetConsoleTitleW(ptr long) kernelbase.GetConsoleTitleW
@ stdcall GetLargestConsoleWindowSize(long) kernelbase.GetLargestConsoleWindowSize
@ stdcall PeekConsoleInputW(ptr ptr long ptr) kernelbase.PeekConsoleInputW
@ stdcall ReadConsoleOutputA(long ptr long long ptr) kernelbase.ReadConsoleOutputA
@ stdcall ReadConsoleOutputAttribute(long ptr long long ptr) kernelbase.ReadConsoleOutputAttribute
@ stdcall ReadConsoleOutputCharacterA(long ptr long long ptr) kernelbase.ReadConsoleOutputCharacterA
@ stdcall ReadConsoleOutputCharacterW(long ptr long long ptr) kernelbase.ReadConsoleOutputCharacterW
@ stdcall ReadConsoleOutputW(long ptr long long ptr) kernelbase.ReadConsoleOutputW
@ stdcall ScrollConsoleScreenBufferA(long ptr ptr ptr ptr) kernelbase.ScrollConsoleScreenBufferA
@ stdcall ScrollConsoleScreenBufferW(long ptr ptr ptr ptr) kernelbase.ScrollConsoleScreenBufferW
@ stdcall SetConsoleActiveScreenBuffer(long) kernelbase.SetConsoleActiveScreenBuffer
@ stdcall SetConsoleCP(long) kernelbase.SetConsoleCP
@ stdcall SetConsoleCursorInfo(long ptr) kernelbase.SetConsoleCursorInfo
@ stdcall SetConsoleCursorPosition(long long) kernelbase.SetConsoleCursorPosition
@ stdcall SetConsoleOutputCP(long) kernelbase.SetConsoleOutputCP
@ stdcall SetConsoleScreenBufferInfoEx(long ptr) kernelbase.SetConsoleScreenBufferInfoEx
@ stdcall SetConsoleScreenBufferSize(long long) kernelbase.SetConsoleScreenBufferSize
@ stdcall SetConsoleTextAttribute(long long) kernelbase.SetConsoleTextAttribute
@ stdcall SetConsoleTitleW(wstr) kernelbase.SetConsoleTitleW
@ stdcall SetConsoleWindowInfo(long long ptr) kernelbase.SetConsoleWindowInfo
@ stdcall WriteConsoleInputA(long ptr long ptr) kernelbase.WriteConsoleInputA
@ stdcall WriteConsoleInputW(long ptr long ptr) kernelbase.WriteConsoleInputW
@ stdcall WriteConsoleOutputA(long ptr long long ptr) kernelbase.WriteConsoleOutputA
@ stdcall WriteConsoleOutputAttribute(long ptr long long ptr) kernelbase.WriteConsoleOutputAttribute
@ stdcall WriteConsoleOutputCharacterA(long ptr long long ptr) kernelbase.WriteConsoleOutputCharacterA
@ stdcall WriteConsoleOutputCharacterW(long ptr long long ptr) kernelbase.WriteConsoleOutputCharacterW
@ stdcall WriteConsoleOutputW(long ptr long long ptr) kernelbase.WriteConsoleOutputW
