/*
 * AILinux Wine functional Win32/Win64 smoke suite
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winreg.h>
#include <objbase.h>
#include <bcrypt.h>
#include <winsock2.h>
#include <stdio.h>
#include <string.h>

static int passed = 0;
static int failed = 0;
static void result(const char *name, int ok)
{
    printf("%s %s\n", ok ? "PASS" : "FAIL", name);
    if (ok) ++passed; else ++failed;
}

static void test_file(void)
{
    wchar_t dir[MAX_PATH], file[MAX_PATH];
    const char payload[] = "ailinux-wine";
    char returned[sizeof(payload)] = {0};
    DWORD done = 0;
    HANDLE h;
    int good = 0;
    if (!GetTempPathW(MAX_PATH, dir) || !GetTempFileNameW(dir, L"ALW", 0, file))
    {
        result("file roundtrip", 0);
        return;
    }
    h = CreateFileW(file, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE)
    {
        good = WriteFile(h, payload, sizeof(payload), &done, NULL) &&
               done == sizeof(payload) &&
               SetFilePointer(h, 0, NULL, FILE_BEGIN) == 0 &&
               ReadFile(h, returned, sizeof(returned), &done, NULL) &&
               done == sizeof(returned) &&
               memcmp(payload, returned, sizeof(payload)) == 0;
        CloseHandle(h);
    }
    DeleteFileW(file);
    result("file roundtrip", good);
}

static void test_registry(void)
{
    HKEY key = NULL;
    const wchar_t *path = L"Software\\AILinuxWineAlphaProbe";
    DWORD expected = 0xA11E2026, actual = 0, actual_size = sizeof(actual), type = 0;
    LONG rc = RegCreateKeyExW(HKEY_CURRENT_USER, path, 0, NULL, 0,
                              KEY_READ | KEY_WRITE, NULL, &key, NULL);
    int good = 0;
    if (rc == ERROR_SUCCESS)
    {
        good = RegSetValueExW(key, L"AlphaProbe", 0, REG_DWORD,
                             (const BYTE *)&expected, sizeof(expected)) == ERROR_SUCCESS &&
               RegQueryValueExW(key, L"AlphaProbe", NULL, &type,
                                (BYTE *)&actual, &actual_size) == ERROR_SUCCESS &&
               type == REG_DWORD && actual == expected;
        RegCloseKey(key);
        RegDeleteKeyW(HKEY_CURRENT_USER, path);
    }
    result("registry read/write", good);
}

static void test_com(void)
{
    GUID guid, parsed;
    wchar_t text[64] = {0};
    HRESULT init = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    HRESULT rc = CoCreateGuid(&guid);
    int good = (init == S_OK || init == S_FALSE) && SUCCEEDED(rc) &&
               StringFromGUID2(&guid, text, 64) > 0 &&
               SUCCEEDED(CLSIDFromString(text, &parsed)) &&
               !memcmp(&guid, &parsed, sizeof(GUID));
    if (init == S_OK || init == S_FALSE) CoUninitialize();
    result("COM GUID create/parse", good);
}

static void test_winsock(void)
{
    WSADATA data;
    SOCKET sock = INVALID_SOCKET;
    int good = 0;
    if (WSAStartup(MAKEWORD(2, 2), &data) == 0)
    {
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        good = sock != INVALID_SOCKET;
        if (good) closesocket(sock);
        WSACleanup();
    }
    result("Winsock TCP socket", good);
}

static void test_crypto(void)
{
    UCHAR bytes[32] = {0};
    int good = BCryptGenRandom(NULL, bytes, sizeof(bytes),
                               BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
    int nonzero = 0;
    unsigned int i;
    for (i = 0; i < sizeof(bytes); ++i) nonzero |= bytes[i];
    result("BCrypt random", good && nonzero);
}

static void test_sync(void)
{
    HANDLE event = CreateEventW(NULL, TRUE, FALSE, NULL);
    int good = event && SetEvent(event) &&
               WaitForSingleObject(event, 1000) == WAIT_OBJECT_0;
    if (event) CloseHandle(event);
    result("kernel event synchronization", good);
}

int main(void)
{
    printf("AILinux Wine functional PE test, architecture=%u-bit\n",
           (unsigned)(sizeof(void *) * 8));
    test_file();
    test_registry();
    test_com();
    test_winsock();
    test_crypto();
    test_sync();
    printf("SUMMARY pass=%d fail=%d\n", passed, failed);
    return failed ? 1 : 0;
}
