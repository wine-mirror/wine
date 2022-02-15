@ stdcall RegCloseKey(long) kernelbase.RegCloseKey
@ stdcall RegCopyTreeW(long wstr long) kernelbase.RegCopyTreeW
@ stdcall RegCreateKeyExA(long str long ptr long long ptr ptr ptr) kernelbase.RegCreateKeyExA
@ stdcall RegCreateKeyExW(long wstr long ptr long long ptr ptr ptr) kernelbase.RegCreateKeyExW
@ stdcall RegDeleteKeyExA(long str long long) kernelbase.RegDeleteKeyExA
@ stdcall RegDeleteKeyExW(long wstr long long) kernelbase.RegDeleteKeyExW
@ stdcall RegDeleteTreeA(long str) kernelbase.RegDeleteTreeA
@ stdcall RegDeleteTreeW(long wstr) kernelbase.RegDeleteTreeW
@ stdcall RegDeleteValueA(long str) kernelbase.RegDeleteValueA
@ stdcall RegDeleteValueW(long wstr) kernelbase.RegDeleteValueW
@ stub RegDisablePredefinedCacheEx
@ stdcall RegEnumKeyExA(long long ptr ptr ptr ptr ptr ptr) kernelbase.RegEnumKeyExA
@ stdcall RegEnumKeyExW(long long ptr ptr ptr ptr ptr ptr) kernelbase.RegEnumKeyExW
@ stdcall RegEnumValueA(long long ptr ptr ptr ptr ptr ptr) kernelbase.RegEnumValueA
@ stdcall RegEnumValueW(long long ptr ptr ptr ptr ptr ptr) kernelbase.RegEnumValueW
@ stdcall RegFlushKey(long) kernelbase.RegFlushKey
@ stdcall RegGetKeySecurity(long long ptr ptr) kernelbase.RegGetKeySecurity
@ stdcall RegGetValueA(long str str long ptr ptr ptr) kernelbase.RegGetValueA
@ stdcall RegGetValueW(long wstr wstr long ptr ptr ptr) kernelbase.RegGetValueW
@ stdcall RegLoadAppKeyA(str ptr long long long) kernelbase.RegLoadAppKeyA
@ stdcall RegLoadAppKeyW(wstr ptr long long long) kernelbase.RegLoadAppKeyW
@ stdcall RegLoadKeyA(long str str) kernelbase.RegLoadKeyA
@ stdcall RegLoadKeyW(long wstr wstr) kernelbase.RegLoadKeyW
@ stdcall RegLoadMUIStringA(long str str long ptr long str) kernelbase.RegLoadMUIStringA
@ stdcall RegLoadMUIStringW(long wstr wstr long ptr long wstr) kernelbase.RegLoadMUIStringW
@ stdcall RegNotifyChangeKeyValue(long long long long long) kernelbase.RegNotifyChangeKeyValue
@ stdcall RegOpenCurrentUser(long ptr) kernelbase.RegOpenCurrentUser
@ stdcall RegOpenKeyExA(long str long long ptr) kernelbase.RegOpenKeyExA
@ stdcall RegOpenKeyExW(long wstr long long ptr) kernelbase.RegOpenKeyExW
@ stdcall RegOpenUserClassesRoot(ptr long long ptr) kernelbase.RegOpenUserClassesRoot
@ stdcall RegQueryInfoKeyA(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) kernelbase.RegQueryInfoKeyA
@ stdcall RegQueryInfoKeyW(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) kernelbase.RegQueryInfoKeyW
@ stdcall RegQueryValueExA(long str ptr ptr ptr ptr) kernelbase.RegQueryValueExA
@ stdcall RegQueryValueExW(long wstr ptr ptr ptr ptr) kernelbase.RegQueryValueExW
@ stdcall RegRestoreKeyA(long str long) kernelbase.RegRestoreKeyA
@ stdcall RegRestoreKeyW(long wstr long) kernelbase.RegRestoreKeyW
@ stdcall RegSaveKeyExA(long str ptr long) kernelbase.RegSaveKeyExA
@ stdcall RegSaveKeyExW(long wstr ptr long) kernelbase.RegSaveKeyExW
@ stdcall RegSetKeySecurity(long long ptr) kernelbase.RegSetKeySecurity
@ stdcall RegSetValueExA(long str long long ptr long) kernelbase.RegSetValueExA
@ stdcall RegSetValueExW(long wstr long long ptr long) kernelbase.RegSetValueExW
@ stdcall RegUnLoadKeyA(long str) kernelbase.RegUnLoadKeyA
@ stdcall RegUnLoadKeyW(long wstr) kernelbase.RegUnLoadKeyW
