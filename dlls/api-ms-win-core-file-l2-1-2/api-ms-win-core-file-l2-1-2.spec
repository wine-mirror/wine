@ stub CopyFile2
@ stdcall CopyFileExW(wstr wstr ptr ptr ptr long) kernelbase.CopyFileExW
@ stdcall CopyFileW(wstr wstr long) kernelbase.CopyFileW
@ stdcall CreateDirectoryExW(wstr wstr ptr) kernelbase.CreateDirectoryExW
@ stdcall CreateHardLinkA(str str ptr) kernelbase.CreateHardLinkA
@ stdcall CreateHardLinkW(wstr wstr ptr) kernelbase.CreateHardLinkW
@ stdcall CreateSymbolicLinkW(wstr wstr long) kernelbase.CreateSymbolicLinkW
@ stdcall GetFileInformationByHandleEx(long long ptr long) kernelbase.GetFileInformationByHandleEx
@ stdcall MoveFileExW(wstr wstr long) kernelbase.MoveFileExW
@ stdcall MoveFileWithProgressW(wstr wstr ptr ptr long) kernelbase.MoveFileWithProgressW
@ stdcall OpenFileById(long ptr long long ptr long) kernelbase.OpenFileById
@ stdcall ReadDirectoryChangesW(long ptr long long long ptr ptr ptr) kernelbase.ReadDirectoryChangesW
@ stdcall ReOpenFile(ptr long long long) kernelbase.ReOpenFile
@ stdcall ReplaceFileW(wstr wstr wstr long ptr ptr) kernelbase.ReplaceFileW
