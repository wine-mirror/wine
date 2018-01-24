@ stub CopyFile2
@ stdcall CopyFileExW(wstr wstr ptr ptr ptr long) kernel32.CopyFileExW
@ stdcall CopyFileW(wstr wstr long) kernel32.CopyFileW
@ stdcall CreateDirectoryExW(wstr wstr ptr) kernel32.CreateDirectoryExW
@ stdcall CreateHardLinkA(str str ptr) kernel32.CreateHardLinkA
@ stdcall CreateHardLinkW(wstr wstr ptr) kernel32.CreateHardLinkW
@ stdcall CreateSymbolicLinkW(wstr wstr long) kernel32.CreateSymbolicLinkW
@ stdcall GetFileInformationByHandleEx(long long ptr long) kernel32.GetFileInformationByHandleEx
@ stdcall MoveFileExW(wstr wstr long) kernel32.MoveFileExW
@ stdcall MoveFileWithProgressW(wstr wstr ptr ptr long) kernel32.MoveFileWithProgressW
@ stdcall OpenFileById(long ptr long long ptr long) kernel32.OpenFileById
@ stdcall ReadDirectoryChangesW(long ptr long long long ptr ptr ptr) kernel32.ReadDirectoryChangesW
@ stdcall ReOpenFile(ptr long long long) kernel32.ReOpenFile
@ stdcall ReplaceFileW(wstr wstr wstr long ptr ptr) kernel32.ReplaceFileW
