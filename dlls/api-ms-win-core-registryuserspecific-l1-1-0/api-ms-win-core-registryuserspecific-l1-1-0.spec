@ stdcall SHRegCloseUSKey(ptr) kernelbase.SHRegCloseUSKey
@ stdcall SHRegCreateUSKeyA(str long long ptr long) kernelbase.SHRegCreateUSKeyA
@ stdcall SHRegCreateUSKeyW(wstr long long ptr long) kernelbase.SHRegCreateUSKeyW
@ stdcall SHRegDeleteEmptyUSKeyA(long str long) kernelbase.SHRegDeleteEmptyUSKeyA
@ stdcall SHRegDeleteEmptyUSKeyW(long wstr long) kernelbase.SHRegDeleteEmptyUSKeyW
@ stdcall SHRegDeleteUSValueA(long str long) kernelbase.SHRegDeleteUSValueA
@ stdcall SHRegDeleteUSValueW(long wstr long) kernelbase.SHRegDeleteUSValueW
@ stdcall SHRegEnumUSKeyA(long long str ptr long) kernelbase.SHRegEnumUSKeyA
@ stdcall SHRegEnumUSKeyW(long long wstr ptr long) kernelbase.SHRegEnumUSKeyW
@ stdcall SHRegEnumUSValueA(long long ptr ptr ptr ptr ptr long) kernelbase.SHRegEnumUSValueA
@ stdcall SHRegEnumUSValueW(long long ptr ptr ptr ptr ptr long) kernelbase.SHRegEnumUSValueW
@ stdcall SHRegGetBoolUSValueA(str str long long) kernelbase.SHRegGetBoolUSValueA
@ stdcall SHRegGetBoolUSValueW(wstr wstr long long) kernelbase.SHRegGetBoolUSValueW
@ stdcall SHRegGetUSValueA(str str ptr ptr ptr long ptr long) kernelbase.SHRegGetUSValueA
@ stdcall SHRegGetUSValueW(wstr wstr ptr ptr ptr long ptr long) kernelbase.SHRegGetUSValueW
@ stdcall SHRegOpenUSKeyA(str long long ptr long) kernelbase.SHRegOpenUSKeyA
@ stdcall SHRegOpenUSKeyW(wstr long long ptr long) kernelbase.SHRegOpenUSKeyW
@ stdcall SHRegQueryInfoUSKeyA(long ptr ptr ptr ptr long) kernelbase.SHRegQueryInfoUSKeyA
@ stdcall SHRegQueryInfoUSKeyW(long ptr ptr ptr ptr long) kernelbase.SHRegQueryInfoUSKeyW
@ stdcall SHRegQueryUSValueA(long str ptr ptr ptr long ptr long) kernelbase.SHRegQueryUSValueA
@ stdcall SHRegQueryUSValueW(long wstr ptr ptr ptr long ptr long) kernelbase.SHRegQueryUSValueW
@ stdcall SHRegSetUSValueA(str str long ptr long long) kernelbase.SHRegSetUSValueA
@ stdcall SHRegSetUSValueW(wstr wstr long ptr long long) kernelbase.SHRegSetUSValueW
@ stdcall SHRegWriteUSValueA(long str long ptr long long) kernelbase.SHRegWriteUSValueA
@ stdcall SHRegWriteUSValueW(long wstr long ptr long long) kernelbase.SHRegWriteUSValueW
