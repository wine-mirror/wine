@ stdcall CancelIo(long) kernel32.CancelIo
@ stdcall CancelIoEx(long ptr) kernel32.CancelIoEx
@ stdcall CancelSynchronousIo(long) kernel32.CancelSynchronousIo
@ stdcall CreateIoCompletionPort(long long long long) kernel32.CreateIoCompletionPort
@ stdcall DeviceIoControl(long long ptr long ptr long ptr ptr) kernel32.DeviceIoControl
@ stdcall GetOverlappedResult(long ptr ptr long) kernel32.GetOverlappedResult
@ stdcall GetOverlappedResultEx(long ptr ptr long long) kernel32.GetOverlappedResultEx
@ stdcall GetQueuedCompletionStatus(long ptr ptr ptr long) kernel32.GetQueuedCompletionStatus
@ stdcall GetQueuedCompletionStatusEx(ptr ptr long ptr long long) kernel32.GetQueuedCompletionStatusEx
@ stdcall PostQueuedCompletionStatus(long long ptr ptr) kernel32.PostQueuedCompletionStatus
