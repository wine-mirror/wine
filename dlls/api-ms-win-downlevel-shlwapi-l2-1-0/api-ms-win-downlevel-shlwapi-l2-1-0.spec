@ stub IStream_Copy
@ stdcall IStream_Read(ptr ptr long) shcore.IStream_Read
@ stub IStream_ReadStr
@ stdcall IStream_Reset(ptr) shcore.IStream_Reset
@ stdcall IStream_Size(ptr ptr) shcore.IStream_Size
@ stdcall IStream_Write(ptr ptr long) shcore.IStream_Write
@ stub IStream_WriteStr
@ stdcall IUnknown_AtomicRelease(ptr) shcore.IUnknown_AtomicRelease
@ stdcall IUnknown_GetSite(ptr ptr ptr) shcore.IUnknown_GetSite
@ stdcall IUnknown_QueryService(ptr ptr ptr ptr) shcore.IUnknown_QueryService
@ stdcall IUnknown_Set(ptr ptr) shcore.IUnknown_Set
@ stdcall IUnknown_SetSite(ptr ptr) shcore.IUnknown_SetSite
@ stdcall SHAnsiToAnsi(str ptr long) shcore.SHAnsiToAnsi
@ stdcall SHAnsiToUnicode(str ptr long) shcore.SHAnsiToUnicode
@ stdcall SHCopyKeyA(long str long long) shcore.SHCopyKeyA
@ stdcall SHCopyKeyW(long wstr long long) shcore.SHCopyKeyW
@ stdcall SHCreateMemStream(ptr long) shcore.SHCreateMemStream
@ stdcall SHCreateStreamOnFileA(str long ptr) shcore.SHCreateStreamOnFileA
@ stdcall SHCreateStreamOnFileEx(wstr long long long ptr ptr) shcore.SHCreateStreamOnFileEx
@ stdcall SHCreateStreamOnFileW(wstr long ptr) shcore.SHCreateStreamOnFileW
@ stdcall SHCreateThreadRef(ptr ptr) shcore.SHCreateThreadRef
@ stdcall SHDeleteEmptyKeyA(long str) shcore.SHDeleteEmptyKeyA
@ stdcall SHDeleteEmptyKeyW(long wstr) shcore.SHDeleteEmptyKeyW
@ stdcall SHDeleteKeyA(long str) shcore.SHDeleteKeyA
@ stdcall SHDeleteKeyW(long wstr) shcore.SHDeleteKeyW
@ stdcall SHDeleteValueA(long str str) shcore.SHDeleteValueA
@ stdcall SHDeleteValueW(long wstr wstr) shcore.SHDeleteValueW
@ stdcall SHEnumKeyExA(long long str ptr) shcore.SHEnumKeyExA
@ stdcall SHEnumKeyExW(long long wstr ptr) shcore.SHEnumKeyExW
@ stdcall SHEnumValueA(long long str ptr ptr ptr ptr) shcore.SHEnumValueA
@ stdcall SHEnumValueW(long long wstr ptr ptr ptr ptr) shcore.SHEnumValueW
@ stdcall SHGetThreadRef(ptr) shcore.SHGetThreadRef
@ stdcall SHGetValueA(long str str ptr ptr ptr) shcore.SHGetValueA
@ stdcall SHGetValueW(long wstr wstr ptr ptr ptr) shcore.SHGetValueW
@ stdcall SHOpenRegStream2A(long str str long) shcore.SHOpenRegStream2A
@ stdcall SHOpenRegStream2W(long wstr wstr long) shcore.SHOpenRegStream2W
@ stdcall SHOpenRegStreamA(long str str long) shcore.SHOpenRegStreamA
@ stdcall SHOpenRegStreamW(long wstr wstr long) shcore.SHOpenRegStreamW
@ stdcall SHQueryInfoKeyA(long ptr ptr ptr ptr) shcore.SHQueryInfoKeyA
@ stdcall SHQueryInfoKeyW(long ptr ptr ptr ptr) shcore.SHQueryInfoKeyW
@ stdcall SHQueryValueExA(long str ptr ptr ptr ptr) shcore.SHQueryValueExA
@ stdcall SHQueryValueExW(long wstr ptr ptr ptr ptr) shcore.SHQueryValueExW
@ stdcall SHRegDuplicateHKey(long) shcore.SHRegDuplicateHKey
@ stdcall SHRegGetPathA(long str str ptr long) shcore.SHRegGetPathA
@ stdcall SHRegGetPathW(long wstr wstr ptr long) shcore.SHRegGetPathW
@ stdcall SHRegGetValueA(long str str long ptr ptr ptr) shcore.SHRegGetValueA
@ stdcall SHRegGetValueW(long wstr wstr long ptr ptr ptr) shcore.SHRegGetValueW
@ stdcall SHRegSetPathA(long str str str long) shcore.SHRegSetPathA
@ stdcall SHRegSetPathW(long wstr wstr wstr long) shcore.SHRegSetPathW
@ stdcall SHReleaseThreadRef() shcore.SHReleaseThreadRef
@ stdcall SHSetThreadRef(ptr) shcore.SHSetThreadRef
@ stdcall SHSetValueA(long str str long ptr long) shcore.SHSetValueA
@ stdcall SHSetValueW(long wstr wstr long ptr long) shcore.SHSetValueW
@ stdcall SHStrDupW(wstr ptr) shcore.SHStrDupW
@ stdcall SHUnicodeToAnsi(wstr ptr ptr) shcore.SHUnicodeToAnsi
@ stdcall SHUnicodeToUnicode(wstr ptr long) shcore.SHUnicodeToUnicode
