@ stdcall CompareFileTime(ptr ptr) kernel32.CompareFileTime
@ stdcall CreateDirectoryA(str ptr) kernel32.CreateDirectoryA
@ stdcall CreateDirectoryW(wstr ptr) kernel32.CreateDirectoryW
@ stdcall CreateFile2(wstr long long long ptr) kernel32.CreateFile2
@ stdcall CreateFileA(str long long ptr long long long) kernel32.CreateFileA
@ stdcall CreateFileW(wstr long long ptr long long long) kernel32.CreateFileW
@ stdcall DefineDosDeviceW(long wstr wstr) kernel32.DefineDosDeviceW
@ stdcall DeleteFileA(str) kernel32.DeleteFileA
@ stdcall DeleteFileW(wstr) kernel32.DeleteFileW
@ stdcall DeleteVolumeMountPointW(wstr) kernel32.DeleteVolumeMountPointW
@ stdcall FileTimeToLocalFileTime(ptr ptr) kernel32.FileTimeToLocalFileTime
@ stdcall FindClose(long) kernel32.FindClose
@ stdcall FindCloseChangeNotification(long) kernel32.FindCloseChangeNotification
@ stdcall FindFirstChangeNotificationA(str long long) kernel32.FindFirstChangeNotificationA
@ stdcall FindFirstChangeNotificationW(wstr long long) kernel32.FindFirstChangeNotificationW
@ stdcall FindFirstFileA(str ptr) kernel32.FindFirstFileA
@ stdcall FindFirstFileExA(str long ptr long ptr long) kernel32.FindFirstFileExA
@ stdcall FindFirstFileExW(wstr long ptr long ptr long) kernel32.FindFirstFileExW
@ stdcall FindFirstFileW(wstr ptr) kernel32.FindFirstFileW
@ stdcall FindFirstVolumeW(ptr long) kernel32.FindFirstVolumeW
@ stdcall FindNextChangeNotification(long) kernel32.FindNextChangeNotification
@ stdcall FindNextFileA(long ptr) kernel32.FindNextFileA
@ stdcall FindNextFileW(long ptr) kernel32.FindNextFileW
@ stdcall FindNextVolumeW(long ptr long) kernel32.FindNextVolumeW
@ stdcall FindVolumeClose(ptr) kernel32.FindVolumeClose
@ stdcall FlushFileBuffers(long) kernel32.FlushFileBuffers
@ stdcall GetDiskFreeSpaceA(str ptr ptr ptr ptr) kernel32.GetDiskFreeSpaceA
@ stdcall GetDiskFreeSpaceExA(str ptr ptr ptr) kernel32.GetDiskFreeSpaceExA
@ stdcall GetDiskFreeSpaceExW(wstr ptr ptr ptr) kernel32.GetDiskFreeSpaceExW
@ stdcall GetDiskFreeSpaceW(wstr ptr ptr ptr ptr) kernel32.GetDiskFreeSpaceW
@ stdcall GetDriveTypeA(str) kernel32.GetDriveTypeA
@ stdcall GetDriveTypeW(wstr) kernel32.GetDriveTypeW
@ stdcall GetFileAttributesA(str) kernel32.GetFileAttributesA
@ stdcall GetFileAttributesExA(str long ptr) kernel32.GetFileAttributesExA
@ stdcall GetFileAttributesExW(wstr long ptr) kernel32.GetFileAttributesExW
@ stdcall GetFileAttributesW(wstr) kernel32.GetFileAttributesW
@ stdcall GetFileInformationByHandle(long ptr) kernel32.GetFileInformationByHandle
@ stdcall GetFileSize(long ptr) kernel32.GetFileSize
@ stdcall GetFileSizeEx(long ptr) kernel32.GetFileSizeEx
@ stdcall GetFileTime(long ptr ptr ptr) kernel32.GetFileTime
@ stdcall GetFileType(long) kernel32.GetFileType
@ stdcall GetFinalPathNameByHandleA(long ptr long long) kernel32.GetFinalPathNameByHandleA
@ stdcall GetFinalPathNameByHandleW(long ptr long long) kernel32.GetFinalPathNameByHandleW
@ stdcall GetFullPathNameA(str long ptr ptr) kernel32.GetFullPathNameA
@ stdcall GetFullPathNameW(wstr long ptr ptr) kernel32.GetFullPathNameW
@ stdcall GetLogicalDriveStringsW(long ptr) kernel32.GetLogicalDriveStringsW
@ stdcall GetLogicalDrives() kernel32.GetLogicalDrives
@ stdcall GetLongPathNameA(str long long) kernel32.GetLongPathNameA
@ stdcall GetLongPathNameW(wstr long long) kernel32.GetLongPathNameW
@ stdcall GetShortPathNameW(wstr ptr long) kernel32.GetShortPathNameW
@ stdcall GetTempFileNameW(wstr wstr long ptr) kernel32.GetTempFileNameW
@ stdcall GetTempPathW(long ptr) kernel32.GetTempPathW
@ stub GetVolumeInformationByHandleW
@ stdcall GetVolumeInformationW(wstr ptr long ptr ptr ptr ptr long) kernel32.GetVolumeInformationW
@ stdcall GetVolumeNameForVolumeMountPointW(wstr ptr long) kernel32.GetVolumeNameForVolumeMountPointW
@ stdcall GetVolumePathNameW(wstr ptr long) kernel32.GetVolumePathNameW
@ stdcall GetVolumePathNamesForVolumeNameW(wstr ptr long ptr) kernel32.GetVolumePathNamesForVolumeNameW
@ stdcall LocalFileTimeToFileTime(ptr ptr) kernel32.LocalFileTimeToFileTime
@ stdcall LockFile(long long long long long) kernel32.LockFile
@ stdcall LockFileEx(long long long long long ptr) kernel32.LockFileEx
@ stdcall QueryDosDeviceW(wstr ptr long) kernel32.QueryDosDeviceW
@ stdcall ReadFile(long ptr long ptr ptr) kernel32.ReadFile
@ stdcall ReadFileEx(long ptr long ptr ptr) kernel32.ReadFileEx
@ stdcall ReadFileScatter(long ptr long ptr ptr) kernel32.ReadFileScatter
@ stdcall RemoveDirectoryA(str) kernel32.RemoveDirectoryA
@ stdcall RemoveDirectoryW(wstr) kernel32.RemoveDirectoryW
@ stdcall SetEndOfFile(long) kernel32.SetEndOfFile
@ stdcall SetFileAttributesA(str long) kernel32.SetFileAttributesA
@ stdcall SetFileAttributesW(wstr long) kernel32.SetFileAttributesW
@ stdcall SetFileInformationByHandle(long long ptr long) kernel32.SetFileInformationByHandle
@ stdcall SetFilePointer(long long ptr long) kernel32.SetFilePointer
@ stdcall SetFilePointerEx(long int64 ptr long) kernel32.SetFilePointerEx
@ stdcall SetFileTime(long ptr ptr ptr) kernel32.SetFileTime
@ stdcall SetFileValidData(ptr int64) kernel32.SetFileValidData
@ stdcall UnlockFile(long long long long long) kernel32.UnlockFile
@ stdcall UnlockFileEx(long long long long ptr) kernel32.UnlockFileEx
@ stdcall WriteFile(long ptr long ptr ptr) kernel32.WriteFile
@ stdcall WriteFileEx(long ptr long ptr ptr) kernel32.WriteFileEx
@ stdcall WriteFileGather(long ptr long ptr ptr) kernel32.WriteFileGather
