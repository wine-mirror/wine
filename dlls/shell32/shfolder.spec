name shfolder
type win32
import advapi32

@ stdcall SHGetFolderPathA(long long long long ptr)SHGetFolderPathA
@ stdcall SHGetFolderPathW(long long long long ptr)SHGetFolderPathW
