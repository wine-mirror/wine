/* Unit test suite for Rtl string functions
 *
 * Copyright 2002 Robert Shearman
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * NOTES
 * We use function pointers here as there is no import library for NTDLL on
 * windows.
 */

#include "winbase.h"
#include "wine/test.h"
#include "winnt.h"
#include "winternl.h"

static UNICODE_STRING uni;
static STRING str;

/* Function ptrs for ntdll calls */
static HMODULE hntdll = 0;
static VOID (WINAPI *pRtlInitAnsiString)(PSTRING,LPCSTR);
static VOID (WINAPI *pRtlInitString)(PSTRING, LPCSTR);
/*static VOID (WINAPI *pRtlFreeAnsiString)(PSTRING);*/
/*static VOID (WINAPI *pRtlFreeOemString)(PSTRING);*/
static VOID (WINAPI *pRtlCopyString)(STRING *, const STRING *);
static VOID (WINAPI *pRtlInitUnicodeString)(PUNICODE_STRING,LPCWSTR);
static BOOLEAN (WINAPI *pRtlCreateUnicodeString)(PUNICODE_STRING,LPCWSTR);
/*static BOOLEAN (WINAPI *pRtlCreateUnicodeStringFromAsciiz)(PUNICODE_STRING,LPCSTR);*/
/*static VOID (WINAPI *pRtlFreeUnicodeString)(PUNICODE_STRING);*/
/*static VOID (WINAPI *pRtlCopyUnicodeString)(UNICODE_STRING *, const UNICODE_STRING *);*/
/*static VOID (WINAPI *pRtlEraseUnicodeString)(UNICODE_STRING *);*/
/*static LONG (WINAPI *pRtlCompareString)(const STRING *,const STRING *,BOOLEAN);*/
/*static LONG (WINAPI *pRtlCompareUnicodeString)(const UNICODE_STRING *,const UNICODE_STRING *,BOOLEAN);*/
/*static BOOLEAN (WINAPI *pRtlEqualString)(const STRING *,const STRING *,BOOLEAN);*/
/*static BOOLEAN (WINAPI *pRtlEqualUnicodeString)(const UNICODE_STRING *,const UNICODE_STRING *,BOOLEAN);*/
/*static BOOLEAN (WINAPI *pRtlPrefixString)(const STRING *, const STRING *, BOOLEAN);*/
/*static BOOLEAN (WINAPI *pRtlPrefixUnicodeString)(const UNICODE_STRING *, const UNICODE_STRING *, BOOLEAN);*/
/*static NTSTATUS (WINAPI *pRtlAnsiStringToUnicodeString)(PUNICODE_STRING, PCANSI_STRING, BOOLEAN);*/
/*static NTSTATUS (WINAPI *pRtlOemStringToUnicodeString)(PUNICODE_STRING, const STRING *, BOOLEAN);*/
/*static NTSTATUS (WINAPI *pRtlUnicodeStringToAnsiString)(STRING *, const UNICODE_STRING *, BOOLEAN);*/
/*static NTSTATUS (WINAPI *pRtlUnicodeStringToOemString)(STRING *, const UNICODE_STRING *, BOOLEAN);*/
/*static NTSTATUS (WINAPI *pRtlMultiByteToUnicodeN)(LPWSTR, DWORD, LPDWORD, LPCSTR, DWORD);*/
/*static NTSTATUS (WINAPI *pRtlOemToUnicodeN)(LPWSTR, DWORD, LPDWORD, LPCSTR, DWORD);*/
/*static NTSTATUS (WINAPI *pRtlUpperString)(STRING *, const STRING *);*/
/*static NTSTATUS (WINAPI *pRtlUpcaseUnicodeString)(UNICODE_STRING *, const UNICODE_STRING *, BOOLEAN);*/
/*static NTSTATUS (WINAPI *pRtlUpcaseUnicodeStringToAnsiString)(STRING *, const UNICODE_STRING *, BOOLEAN);*/
/*static NTSTATUS (WINAPI *pRtlUpcaseUnicodeStringToOemString)(STRING *, const UNICODE_STRING *, BOOLEAN);*/
/*static NTSTATUS (WINAPI *pRtlUpcaseUnicodeToMultiByteN)(LPSTR, DWORD, LPDWORD, LPCWSTR, DWORD);*/
/*static NTSTATUS (WINAPI *pRtlUpcaseUnicodeToOemN)(LPSTR, DWORD, LPDWORD, LPCWSTR, DWORD);*/
/*static UINT (WINAPI *pRtlOemToUnicodeSize)(const STRING *);*/
/*static DWORD (WINAPI *pRtlAnsiStringToUnicodeSize)(const STRING *);*/

/* more function pointers here */

/*static DWORD (WINAPI *pRtlIsTextUnicode)(LPVOID, DWORD, DWORD *);*/
static NTSTATUS (WINAPI *pRtlUnicodeStringToInteger)(const UNICODE_STRING *, int, int *);

static void InitFunctionPtrs()
{
  hntdll = LoadLibraryA("ntdll.dll");
  ok(hntdll != 0, "LoadLibrary failed");
  if (hntdll)
  {
	pRtlInitAnsiString = (void *)GetProcAddress(hntdll, "RtlInitAnsiString");
	pRtlInitString = (void *)GetProcAddress(hntdll, "RtlInitString");
	pRtlInitUnicodeString = (void *)GetProcAddress(hntdll, "RtlInitUnicodeString");
	pRtlCopyString = (void *)GetProcAddress(hntdll, "RtlCopyString");
	pRtlCreateUnicodeString = (void *)GetProcAddress(hntdll, "RtlCreateUnicodeString");
	pRtlUnicodeStringToInteger = (void *)GetProcAddress(hntdll, "RtlUnicodeStringToInteger");
  }
}

static void test_RtlInitString(void)
{
	static const char teststring[] = "Some Wild String";
	str.Length = 0;
	str.MaximumLength = 0;
	str.Buffer = (void *)0xdeadbeef;
	pRtlInitString(&str, teststring);
	ok(str.Length == sizeof(teststring) - sizeof(char), "Length uninitialized");
	ok(str.MaximumLength == sizeof(teststring), "MaximumLength uninitialized");
	ok(strcmp(str.Buffer, "Some Wild String") == 0, "Buffer written to");
}

static void test_RtlInitUnicodeString(void)
{
#define STRINGW {'S','o','m','e',' ','W','i','l','d',' ','S','t','r','i','n','g',0}
    static const WCHAR teststring[] = STRINGW;
	static const WCHAR originalstring[] = STRINGW;
#undef STRINGW
	uni.Length = 0;
	uni.MaximumLength = 0;
	uni.Buffer = (void *)0xdeadbeef;
	pRtlInitUnicodeString(&uni, teststring);
	ok(uni.Length == sizeof(teststring) - sizeof(WCHAR), "Length uninitialized");
	ok(uni.MaximumLength == sizeof(teststring), "MaximumLength uninitialized");
	ok(wcscmp(uni.Buffer, originalstring) == 0, "Buffer written to");
}

static void test_RtlCopyString(void)
{
	static const char teststring[] = "Some Wild String";
	static char deststring[] = "                    ";
	STRING deststr;
	pRtlInitString(&str, teststring);
	pRtlInitString(&deststr, deststring);
	pRtlCopyString(&deststr, &str);
	ok(strncmp(str.Buffer, deststring, str.Length) == 0, "String not copied");
}

static void test_RtlUnicodeStringToInteger(void)
{
	int dest = 0;
	int i;
	DWORD result;

	/* keep these next two arrays in sync. or they'll be trouble! */
	static const WCHAR stringwithint[][12] = {
		{'1','0','1','1','1','0','1','1','0','0',0},
		{'1','2','3','4','5','6','7',0},
		{'2','1','4','7','4','8','3','6','4','8',0},
		{'-','2','1','4','7','4','8','3','6','4','8',0},
		{'-','2','1','4',0},
		{' ','\n',' ','\r','2','1','4',0},
		{'+','2','1','4',' ','0',0},
		{' ','2','1','4','.','0','1',0},
		{'f','8','1',0},
		{'0','x','1','2','3','4','5',0},
		{'1','x','3','4',0}
	};
	static const int expectedresults[] = {
		1011101100,
		1234567,
		2147483648,
		2147483648,
		-214,
		214,
		214,
		214,
		0,
		0x12345,
		1
	};
	/* these are for stringwithint[0]: */
	static const int expectedresultsbase[] = {
		748, /* base 2 */
		136610368, /* base 8 */
		1011101100, /* base 10 */
		286265600, /* base 16 */
	};
	
	for (i = 0; i < sizeof(expectedresults) / sizeof(int); i++)
	{
		dest = 0xdeadbeef;
		pRtlInitUnicodeString(&uni, stringwithint[i]);
		result = pRtlUnicodeStringToInteger(&uni, 0, &dest);
		ok(result == 0, "call failed: RtlUnicodeStringToInteger(\"%S\", %d, [out])", uni.Buffer, 0);
		ok(dest == expectedresults[i], "didn't return expected value (test %d): expected: %d, got: %d}", i, expectedresults[i], dest);
	}
	pRtlInitUnicodeString(&uni, stringwithint[0]);
	result = pRtlUnicodeStringToInteger(&uni, 2, &dest);
	ok(result == 0, "call failed: RtlUnicodeStringToInteger(\"%S\", %d, [out])", uni.Buffer, 2);
	ok(dest == expectedresultsbase[0], "didn't return expected value: \"%S\"; expected: %d, got: %d}", uni.Buffer, expectedresultsbase[0], dest);
	result = pRtlUnicodeStringToInteger(&uni, 8, &dest);
	ok(result == 0, "call failed: RtlUnicodeStringToInteger(\"%S\", %d, [out])", uni.Buffer, 8);
	ok(dest == expectedresultsbase[1], "didn't return expected value: \"%S\"; expected: %d, got: %d}", uni.Buffer, expectedresultsbase[1], dest);
	result = pRtlUnicodeStringToInteger(&uni, 10, &dest);
	ok(result == 0, "call failed: RtlUnicodeStringToInteger(\"%S\", %d, [out])", uni.Buffer, 10);
	ok(dest == expectedresultsbase[2], "didn't return expected value: \"%S\"; expected: %d, got: %d}", uni.Buffer, expectedresultsbase[2], dest);
	result = pRtlUnicodeStringToInteger(&uni, 16, &dest);
	ok(result == 0, "call failed: RtlUnicodeStringToInteger(\"%S\", %d, [out])", uni.Buffer, 16);
	ok(dest == expectedresultsbase[3], "didn't return expected value: \"%S\"; expected: %d, got: %d}", uni.Buffer, expectedresultsbase[3], dest);
}

START_TEST(rtlstr)
{
  InitFunctionPtrs();

  if (pRtlInitAnsiString)
  {
    test_RtlInitString();
    test_RtlInitUnicodeString();
    test_RtlCopyString();
    test_RtlUnicodeStringToInteger();
  }
}
