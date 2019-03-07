@ stdcall RegCloseKey(long) advapi32.RegCloseKey
@ stdcall RegCopyTreeW(long wstr long) advapi32.RegCopyTreeW
@ stdcall RegCreateKeyExA(long str long ptr long long ptr ptr ptr) advapi32.RegCreateKeyExA
@ stdcall RegCreateKeyExW(long wstr long ptr long long ptr ptr ptr) advapi32.RegCreateKeyExW
@ stdcall RegDeleteKeyExA(long str long long) advapi32.RegDeleteKeyExA
@ stdcall RegDeleteKeyExW(long wstr long long) advapi32.RegDeleteKeyExW
@ stdcall RegDeleteTreeA(long str) advapi32.RegDeleteTreeA
@ stdcall RegDeleteTreeW(long wstr) advapi32.RegDeleteTreeW
@ stdcall RegDeleteValueA(long str) advapi32.RegDeleteValueA
@ stdcall RegDeleteValueW(long wstr) advapi32.RegDeleteValueW
@ stub RegDisablePredefinedCacheEx
@ stdcall RegEnumKeyExA(long long ptr ptr ptr ptr ptr ptr) advapi32.RegEnumKeyExA
@ stdcall RegEnumKeyExW(long long ptr ptr ptr ptr ptr ptr) advapi32.RegEnumKeyExW
@ stdcall RegEnumValueA(long long ptr ptr ptr ptr ptr ptr) advapi32.RegEnumValueA
@ stdcall RegEnumValueW(long long ptr ptr ptr ptr ptr ptr) advapi32.RegEnumValueW
@ stdcall RegFlushKey(long) advapi32.RegFlushKey
@ stdcall RegGetKeySecurity(long long ptr ptr) advapi32.RegGetKeySecurity
@ stdcall RegGetValueA(long str str long ptr ptr ptr) advapi32.RegGetValueA
@ stdcall RegGetValueW(long wstr wstr long ptr ptr ptr) advapi32.RegGetValueW
@ stdcall RegLoadAppKeyA(str ptr long long long) advapi32.RegLoadAppKeyA
@ stdcall RegLoadAppKeyW(wstr ptr long long long) advapi32.RegLoadAppKeyW
@ stdcall RegLoadKeyA(long str str) advapi32.RegLoadKeyA
@ stdcall RegLoadKeyW(long wstr wstr) advapi32.RegLoadKeyW
@ stdcall RegLoadMUIStringA(long str str long ptr long str) advapi32.RegLoadMUIStringA
@ stdcall RegLoadMUIStringW(long wstr wstr long ptr long wstr) advapi32.RegLoadMUIStringW
@ stdcall RegNotifyChangeKeyValue(long long long long long) advapi32.RegNotifyChangeKeyValue
@ stdcall RegOpenCurrentUser(long ptr) advapi32.RegOpenCurrentUser
@ stdcall RegOpenKeyExA(long str long long ptr) advapi32.RegOpenKeyExA
@ stdcall RegOpenKeyExW(long wstr long long ptr) advapi32.RegOpenKeyExW
@ stdcall RegOpenUserClassesRoot(ptr long long ptr) advapi32.RegOpenUserClassesRoot
@ stdcall RegQueryInfoKeyA(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) advapi32.RegQueryInfoKeyA
@ stdcall RegQueryInfoKeyW(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) advapi32.RegQueryInfoKeyW
@ stdcall RegQueryValueExA(long str ptr ptr ptr ptr) advapi32.RegQueryValueExA
@ stdcall RegQueryValueExW(long wstr ptr ptr ptr ptr) advapi32.RegQueryValueExW
@ stdcall RegRestoreKeyA(long str long) advapi32.RegRestoreKeyA
@ stdcall RegRestoreKeyW(long wstr long) advapi32.RegRestoreKeyW
@ stdcall RegSaveKeyExA(long str ptr long) advapi32.RegSaveKeyExA
@ stdcall RegSaveKeyExW(long wstr ptr long) advapi32.RegSaveKeyExW
@ stdcall RegSetKeySecurity(long long ptr) advapi32.RegSetKeySecurity
@ stdcall RegSetValueExA(long str long long ptr long) advapi32.RegSetValueExA
@ stdcall RegSetValueExW(long wstr long long ptr long) advapi32.RegSetValueExW
@ stdcall RegUnLoadKeyA(long str) advapi32.RegUnLoadKeyA
@ stdcall RegUnLoadKeyW(long wstr) advapi32.RegUnLoadKeyW
