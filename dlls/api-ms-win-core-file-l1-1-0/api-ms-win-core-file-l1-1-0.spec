@ stdcall CompareFileTime(ptr ptr) kernelbase.CompareFileTime
@ stdcall CreateDirectoryA(str ptr) kernelbase.CreateDirectoryA
@ stdcall CreateDirectoryW(wstr ptr) kernelbase.CreateDirectoryW
@ stdcall CreateFileA(str long long ptr long long long) kernelbase.CreateFileA
@ stdcall CreateFileW(wstr long long ptr long long long) kernelbase.CreateFileW
@ stdcall DefineDosDeviceW(long wstr wstr) kernelbase.DefineDosDeviceW
@ stdcall DeleteFileA(str) kernelbase.DeleteFileA
@ stdcall DeleteFileW(wstr) kernelbase.DeleteFileW
@ stdcall DeleteVolumeMountPointW(wstr) kernelbase.DeleteVolumeMountPointW
@ stdcall FileTimeToLocalFileTime(ptr ptr) kernelbase.FileTimeToLocalFileTime
@ stdcall FileTimeToSystemTime(ptr ptr) kernelbase.FileTimeToSystemTime
@ stdcall FindClose(long) kernelbase.FindClose
@ stdcall FindCloseChangeNotification(long) kernelbase.FindCloseChangeNotification
@ stdcall FindFirstChangeNotificationA(str long long) kernelbase.FindFirstChangeNotificationA
@ stdcall FindFirstChangeNotificationW(wstr long long) kernelbase.FindFirstChangeNotificationW
@ stdcall FindFirstFileA(str ptr) kernelbase.FindFirstFileA
@ stdcall FindFirstFileExA(str long ptr long ptr long) kernelbase.FindFirstFileExA
@ stdcall FindFirstFileExW(wstr long ptr long ptr long) kernelbase.FindFirstFileExW
@ stdcall FindFirstFileW(wstr ptr) kernelbase.FindFirstFileW
@ stdcall FindFirstVolumeW(ptr long) kernelbase.FindFirstVolumeW
@ stdcall FindNextChangeNotification(long) kernelbase.FindNextChangeNotification
@ stdcall FindNextFileA(long ptr) kernelbase.FindNextFileA
@ stdcall FindNextFileW(long ptr) kernelbase.FindNextFileW
@ stdcall FindNextVolumeW(long ptr long) kernelbase.FindNextVolumeW
@ stdcall FindVolumeClose(ptr) kernelbase.FindVolumeClose
@ stdcall FlushFileBuffers(long) kernelbase.FlushFileBuffers
@ stdcall GetDiskFreeSpaceA(str ptr ptr ptr ptr) kernelbase.GetDiskFreeSpaceA
@ stdcall GetDiskFreeSpaceExA(str ptr ptr ptr) kernelbase.GetDiskFreeSpaceExA
@ stdcall GetDiskFreeSpaceExW(wstr ptr ptr ptr) kernelbase.GetDiskFreeSpaceExW
@ stdcall GetDiskFreeSpaceW(wstr ptr ptr ptr ptr) kernelbase.GetDiskFreeSpaceW
@ stdcall GetDriveTypeA(str) kernelbase.GetDriveTypeA
@ stdcall GetDriveTypeW(wstr) kernelbase.GetDriveTypeW
@ stdcall GetFileAttributesA(str) kernelbase.GetFileAttributesA
@ stdcall GetFileAttributesExA(str long ptr) kernelbase.GetFileAttributesExA
@ stdcall GetFileAttributesExW(wstr long ptr) kernelbase.GetFileAttributesExW
@ stdcall GetFileAttributesW(wstr) kernelbase.GetFileAttributesW
@ stdcall GetFileInformationByHandle(long ptr) kernelbase.GetFileInformationByHandle
@ stdcall GetFileSize(long ptr) kernelbase.GetFileSize
@ stdcall GetFileSizeEx(long ptr) kernelbase.GetFileSizeEx
@ stdcall GetFileTime(long ptr ptr ptr) kernelbase.GetFileTime
@ stdcall GetFileType(long) kernelbase.GetFileType
@ stdcall GetFinalPathNameByHandleA(long ptr long long) kernelbase.GetFinalPathNameByHandleA
@ stdcall GetFinalPathNameByHandleW(long ptr long long) kernelbase.GetFinalPathNameByHandleW
@ stdcall GetFullPathNameA(str long ptr ptr) kernelbase.GetFullPathNameA
@ stdcall GetFullPathNameW(wstr long ptr ptr) kernelbase.GetFullPathNameW
@ stdcall GetLogicalDriveStringsW(long ptr) kernelbase.GetLogicalDriveStringsW
@ stdcall GetLogicalDrives() kernelbase.GetLogicalDrives
@ stdcall GetLongPathNameA(str ptr long) kernelbase.GetLongPathNameA
@ stdcall GetLongPathNameW(wstr ptr long) kernelbase.GetLongPathNameW
@ stdcall GetShortPathNameW(wstr ptr long) kernelbase.GetShortPathNameW
@ stdcall GetTempFileNameW(wstr wstr long ptr) kernelbase.GetTempFileNameW
@ stdcall GetVolumeInformationByHandleW(ptr ptr long ptr ptr ptr ptr long) kernelbase.GetVolumeInformationByHandleW
@ stdcall GetVolumeInformationW(wstr ptr long ptr ptr ptr ptr long) kernelbase.GetVolumeInformationW
@ stdcall GetVolumePathNameW(wstr ptr long) kernelbase.GetVolumePathNameW
@ stdcall LocalFileTimeToFileTime(ptr ptr) kernelbase.LocalFileTimeToFileTime
@ stdcall LockFile(long long long long long) kernelbase.LockFile
@ stdcall LockFileEx(long long long long long ptr) kernelbase.LockFileEx
@ stdcall QueryDosDeviceW(wstr ptr long) kernelbase.QueryDosDeviceW
@ stdcall ReadFile(long ptr long ptr ptr) kernelbase.ReadFile
@ stdcall ReadFileEx(long ptr long ptr ptr) kernelbase.ReadFileEx
@ stdcall ReadFileScatter(long ptr long ptr ptr) kernelbase.ReadFileScatter
@ stdcall RemoveDirectoryA(str) kernelbase.RemoveDirectoryA
@ stdcall RemoveDirectoryW(wstr) kernelbase.RemoveDirectoryW
@ stdcall SetEndOfFile(long) kernelbase.SetEndOfFile
@ stdcall SetFileAttributesA(str long) kernelbase.SetFileAttributesA
@ stdcall SetFileAttributesW(wstr long) kernelbase.SetFileAttributesW
@ stdcall SetFileInformationByHandle(long long ptr long) kernelbase.SetFileInformationByHandle
@ stdcall SetFilePointer(long long ptr long) kernelbase.SetFilePointer
@ stdcall SetFilePointerEx(long int64 ptr long) kernelbase.SetFilePointerEx
@ stdcall SetFileTime(long ptr ptr ptr) kernelbase.SetFileTime
@ stdcall SetFileValidData(ptr int64) kernelbase.SetFileValidData
@ stdcall UnlockFile(long long long long long) kernelbase.UnlockFile
@ stdcall UnlockFileEx(long long long long ptr) kernelbase.UnlockFileEx
@ stdcall WriteFile(long ptr long ptr ptr) kernelbase.WriteFile
@ stdcall WriteFileEx(long ptr long ptr ptr) kernelbase.WriteFileEx
@ stdcall WriteFileGather(long ptr long ptr ptr) kernelbase.WriteFileGather
