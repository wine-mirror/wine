@ stdcall AllocConsole() kernelbase.AllocConsole
@ stdcall AttachConsole(long) kernelbase.AttachConsole
@ stdcall FreeConsole() kernelbase.FreeConsole
@ stdcall GetConsoleCP() kernelbase.GetConsoleCP
@ stdcall GetConsoleMode(long ptr) kernelbase.GetConsoleMode
@ stdcall GetConsoleOutputCP() kernelbase.GetConsoleOutputCP
@ stdcall GetNumberOfConsoleInputEvents(long ptr) kernelbase.GetNumberOfConsoleInputEvents
@ stdcall PeekConsoleInputA(ptr ptr long ptr) kernelbase.PeekConsoleInputA
@ stdcall PeekConsoleInputW(ptr ptr long ptr) kernelbase.PeekConsoleInputW
@ stdcall ReadConsoleA(long ptr long ptr ptr) kernelbase.ReadConsoleA
@ stdcall ReadConsoleInputA(long ptr long ptr) kernelbase.ReadConsoleInputA
@ stdcall ReadConsoleInputW(long ptr long ptr) kernelbase.ReadConsoleInputW
@ stdcall ReadConsoleW(long ptr long ptr ptr) kernelbase.ReadConsoleW
@ stdcall SetConsoleCtrlHandler(ptr long) kernelbase.SetConsoleCtrlHandler
@ stdcall SetConsoleMode(long long) kernelbase.SetConsoleMode
@ stdcall WriteConsoleA(long ptr long ptr ptr) kernelbase.WriteConsoleA
@ stdcall WriteConsoleW(long ptr long ptr ptr) kernelbase.WriteConsoleW
