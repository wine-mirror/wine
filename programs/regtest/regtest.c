/*
 * Registry testing program
 *
 * Copyright 1998 Matthew Becker
 *
 * The return codes were generated in an NT40 environment, using lcc-win32
 *
 * NOTES
 *    When compiling under lcc-win32, I get three (3) warning, but they all
 *    seem to be issues with lcc.
 *
 *    If creating a new testing sequence, please try to clean up any
 *    registry changes that are made.
 */

#include <stdio.h>
#include <malloc.h>
#include <windows.h>
#include <winreg.h>
#include <winerror.h>
#include <winnt.h>

#ifndef __GNUC__
#define __FUNCTION__ "<function>"
#endif

/* True this when security is implemented */
#define CHECK_SAM FALSE

#define xERROR(s,d) fprintf(stderr, "%s:#%d(Status=%ld)\n", __FUNCTION__,s,d)

/*
 * NOTES: These individual routines are listed in alphabetical order.
 *
 * They are meant to test error conditions.  Success conditions are
 * tested in the sequences found at the end.
 */

/******************************************************************************
 * TestCloseKey
 */
void TestCloseKey()
{
    long lSts;

    lSts = RegCloseKey((HKEY)2);
    if (lSts != ERROR_INVALID_HANDLE) xERROR(1,lSts);

    lSts = RegCloseKey(HKEY_LOCAL_MACHINE);
    if (lSts != ERROR_SUCCESS) xERROR(2,lSts);

    /* Check twice just for kicks */
    lSts = RegCloseKey(HKEY_LOCAL_MACHINE);
    if (lSts != ERROR_SUCCESS) xERROR(3,lSts);
}

/******************************************************************************
 * TestConnectRegistry
 */
void TestConnectRegistry()
{
    long lSts;
    HKEY hkey;

    lSts = RegConnectRegistry("",(HKEY)2,&hkey);
    if (lSts != ERROR_SUCCESS) xERROR(1,lSts);

    lSts = RegConnectRegistry("",HKEY_LOCAL_MACHINE,&hkey);
    if (lSts != ERROR_SUCCESS) xERROR(2,lSts);

#if TOO_SLOW
    lSts = RegConnectRegistry("\\\\regtest",HKEY_LOCAL_MACHINE,&hkey);
    if (lSts != ERROR_BAD_NETPATH) xERROR(3,lSts);
#endif
}

/******************************************************************************
 * TestCreateKey
 */
void TestCreateKey()
{
    long lSts;
    HKEY hkey;

    lSts = RegCreateKey((HKEY)2,"",&hkey);
    if (lSts != ERROR_BADKEY) xERROR(1,lSts);

    lSts = RegCreateKey(HKEY_LOCAL_MACHINE,"",&hkey);
    if (lSts != ERROR_SUCCESS) xERROR(2,lSts);
    RegCloseKey(hkey);

    lSts = RegCreateKey(HKEY_LOCAL_MACHINE,"\\asdf",&hkey);
    if (lSts != ERROR_BAD_PATHNAME) xERROR(3,lSts);

#if 0
    lSts = RegCreateKey(HKEY_LOCAL_MACHINE,"asdf\\",&hkey);
    if (lSts != ERROR_INVALID_PARAMETER) xERROR(4,lSts);
#endif

    lSts = RegCreateKey(HKEY_LOCAL_MACHINE,"\\asdf\\",&hkey);
    if (lSts != ERROR_BAD_PATHNAME) xERROR(5,lSts);
}

/******************************************************************************
 * TestCreateKeyEx
 */
void TestCreateKeyEx()
{
    long lSts;
    HKEY hkey;
    DWORD dwDisp;

    lSts = RegCreateKeyEx((HKEY)2,"",0,"",0,0,NULL,&hkey,&dwDisp);
    if (lSts != ERROR_INVALID_HANDLE) xERROR(1,lSts);

    lSts = RegCreateKeyEx(HKEY_LOCAL_MACHINE,"regtest",0,"",0,0,NULL,&hkey,
                          &dwDisp);
    if (lSts != ERROR_INVALID_PARAMETER) xERROR(2,lSts);

    lSts = RegCreateKeyEx(HKEY_LOCAL_MACHINE,"regtest",0,"asdf",0,
                          KEY_ALL_ACCESS,NULL,&hkey,&dwDisp);
    if (lSts != ERROR_INVALID_PARAMETER) xERROR(3,lSts);

    lSts = RegCreateKeyEx(HKEY_LOCAL_MACHINE,"regtest",0,"",0,
                          KEY_ALL_ACCESS,NULL,&hkey,&dwDisp);
    if (lSts != ERROR_INVALID_PARAMETER) xERROR(4,lSts);

}

/******************************************************************************
 * TestCreateKeyEx
 */
void TestCreateKeyEx1()
{
    long lSts;
    HKEY hkey,hkeyP;
    DWORD dwDisp;
    char keyname[]="regtest1";

    lSts = RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE",0,1,&hkeyP);
    if (lSts != ERROR_SUCCESS) 
      {
	xERROR(1,lSts);
	return;
      }
    lSts = RegCreateKeyEx(hkeyP,keyname,0,0,0,0xf003f,0,&hkey,&dwDisp);
    if (lSts != ERROR_SUCCESS) 
    {
      xERROR(2,lSts);
      RegCloseKey(hkeyP);
      return;
    }
    lSts = RegDeleteKey( hkeyP,keyname);
    if (lSts != ERROR_SUCCESS)  xERROR(3,lSts);
    RegCloseKey(hkeyP);
}

/******************************************************************************
 * TestDeleteKey
 */
void TestDeleteKey()
{
    long lSts;

    lSts = RegDeleteKey((HKEY)2, "asdf");
    if (lSts != ERROR_INVALID_HANDLE) xERROR(1,lSts);

    lSts = RegDeleteKey(HKEY_CURRENT_USER, "asdf");
    if (lSts != ERROR_FILE_NOT_FOUND) xERROR(2,lSts);

#if CHECK_SAM
    lSts = RegDeleteKey(HKEY_CURRENT_USER, "");
    if (lSts != ERROR_ACCESS_DENIED) xERROR(3,lSts);
#endif
}

/******************************************************************************
 * TestDeleteValue
 */
void TestDeleteValue()
{
    long lSts;

    lSts = RegDeleteValue((HKEY)2, "asdf");
    if (lSts != ERROR_INVALID_HANDLE) xERROR(1,lSts);

    lSts = RegDeleteValue(HKEY_CURRENT_USER, "");
    if (lSts != ERROR_FILE_NOT_FOUND) xERROR(2,lSts);

    lSts = RegDeleteValue(HKEY_CURRENT_USER, "asdf");
    if (lSts != ERROR_FILE_NOT_FOUND) xERROR(3,lSts);

    lSts = RegDeleteValue(HKEY_CURRENT_USER, "\\asdf");
    if (lSts != ERROR_FILE_NOT_FOUND) xERROR(4,lSts);
}

/******************************************************************************
 * TestEnumKey
 */
void TestEnumKey()
{
    long lSts;
    char *sVal;
    long lVal;

    lVal = 1;
    sVal = (char *)malloc(lVal * sizeof(char));

    lSts = RegEnumKey((HKEY)2,3,sVal,lVal);
    if (lSts != ERROR_INVALID_HANDLE) xERROR(1,lSts);

    lSts = RegEnumKey(HKEY_CURRENT_USER,-1,sVal,lVal);
    if (lSts != ERROR_NO_MORE_ITEMS) xERROR(2,lSts);

    lSts = RegEnumKey(HKEY_CURRENT_USER,0,sVal,lVal);
    if (lSts != ERROR_MORE_DATA) xERROR(3,lSts);
}

/******************************************************************************
 * TestEnumKeyEx
 */
void TestEnumKeyEx()
{
    long lSts;
    char *sVal;
    char *sClass;
    unsigned long lLen1;
    unsigned long lLen2;
    FILETIME ft;

    lLen1 = 1;
    sVal = (char *)malloc(lLen1 * sizeof(char));
    lLen2 = 1;
    sClass = (char *)malloc(lLen2 * sizeof(char));

    lSts = RegEnumKeyEx((HKEY)2,0,sVal,&lLen1,0,sClass,&lLen2,&ft);
    if (lSts != ERROR_INVALID_HANDLE) xERROR(1,lSts);

    lSts = RegEnumKeyEx(HKEY_LOCAL_MACHINE,0,sVal,&lLen1,0,sClass,&lLen2,&ft);
    if (lSts != ERROR_MORE_DATA) xERROR(2,lSts);

    lSts = RegEnumKeyEx(HKEY_LOCAL_MACHINE,0,sVal,&lLen1,0,sClass,&lLen2,&ft);
    if (lSts != ERROR_MORE_DATA) xERROR(3,lSts);
}

/******************************************************************************
 * TestEnumValue
 */
void TestEnumValue()
{
    long lSts;
    char *sVal;
    unsigned long lVal;
    unsigned long lType;
    unsigned long lLen1;
    char *bVal;

    lVal = 1;
    sVal = (char *)malloc(lVal * sizeof(char));
    lLen1 = 1;
    bVal = (char *)malloc(lLen1 * sizeof(char));

    lSts = RegEnumValue((HKEY)2,-1,sVal,&lVal,0,&lType,NULL,&lLen1);
    if (lSts != ERROR_INVALID_HANDLE) xERROR(1,lSts);

    lSts = RegEnumValue(HKEY_LOCAL_MACHINE,-1,sVal,&lVal,0,&lType,NULL,&lLen1);
    if (lSts != ERROR_NO_MORE_ITEMS) xERROR(2,lSts);

    lSts = RegEnumValue(HKEY_LOCAL_MACHINE,0,sVal,&lVal,0,&lType,NULL,&lLen1);
    if (lSts != ERROR_SUCCESS) xERROR(3,lSts);

    lSts = RegEnumValue(HKEY_LOCAL_MACHINE,0,sVal,&lVal,0,NULL,NULL,&lLen1);
    if (lSts != ERROR_SUCCESS) xERROR(4,lSts);

    lSts = RegEnumValue(HKEY_LOCAL_MACHINE,1,sVal,&lVal,0,&lType,bVal,&lLen1);
    if (lSts != ERROR_NO_MORE_ITEMS) xERROR(5,lSts);
}

/******************************************************************************
 * TestFlushKey
 */
void TestFlushKey()
{
    long lSts;

    lSts = RegFlushKey((HKEY)2);
    if (lSts != ERROR_INVALID_HANDLE) xERROR(1,lSts);

    lSts = RegFlushKey(HKEY_LOCAL_MACHINE);
    if (lSts != ERROR_SUCCESS) xERROR(2,lSts);
}

/******************************************************************************
 * TestGetKeySecurity
 */
void TestGetKeySecurity()
{
    long lSts;
    SECURITY_INFORMATION si;
    SECURITY_DESCRIPTOR sd;
    unsigned long lLen;

    lLen = sizeof(sd);
    si = 0;
    lSts = RegGetKeySecurity((HKEY)2,si,&sd,&lLen);
    if (lSts != ERROR_INVALID_HANDLE) xERROR(1,lSts);

    lSts = RegGetKeySecurity(HKEY_LOCAL_MACHINE,si,&sd,&lLen);
    if (lSts != ERROR_INSUFFICIENT_BUFFER) xERROR(2,lSts);

    si = GROUP_SECURITY_INFORMATION;
    lSts = RegGetKeySecurity(HKEY_LOCAL_MACHINE,si,&sd,&lLen);
    if (lSts != ERROR_SUCCESS) xERROR(3,lSts);
}

/******************************************************************************
 * TestLoadKey
 */
void TestLoadKey()
{
    long lSts;

    lSts = RegLoadKey((HKEY)2,"","");
    if (lSts != ERROR_INVALID_PARAMETER) xERROR(1,lSts);

    lSts = RegLoadKey(HKEY_CURRENT_USER,"","");
    if (lSts != ERROR_INVALID_PARAMETER) xERROR(2,lSts);

    lSts = RegLoadKey(HKEY_CURRENT_USER,"regtest","");
    if (lSts != ERROR_INVALID_PARAMETER) xERROR(3,lSts);

    lSts = RegLoadKey(HKEY_CURRENT_USER,"\\regtest","");
    if (lSts != ERROR_INVALID_PARAMETER) xERROR(4,lSts);

#if CHECK_SAM
    lSts = RegLoadKey(HKEY_CURRENT_USER,"regtest","regtest.dat");
    if (lSts != ERROR_PRIVILEGE_NOT_HELD) xERROR(5,lSts);

    lSts = RegLoadKey(HKEY_CURRENT_USER,"\\regtest","regtest.dat");
    if (lSts != ERROR_PRIVILEGE_NOT_HELD) xERROR(6,lSts);
#endif
}

/******************************************************************************
 * TestNotifyChangeKeyValue
 */
void TestNotifyChangeKeyValue()
{
    long lSts;
    HANDLE hEvent;

    hEvent = (HANDLE)0;

    lSts = RegNotifyChangeKeyValue((HKEY)2, TRUE, REG_NOTIFY_CHANGE_NAME, 0, 0);
    if (lSts != ERROR_INVALID_HANDLE) xERROR(1,lSts);

    lSts = RegNotifyChangeKeyValue(HKEY_CURRENT_USER, TRUE, REG_NOTIFY_CHANGE_NAME, 0, 1);
    if (lSts != ERROR_INVALID_PARAMETER) xERROR(2,lSts);

    hEvent = (HANDLE)HKEY_CURRENT_USER;
    lSts = RegNotifyChangeKeyValue(HKEY_CURRENT_USER, TRUE, REG_NOTIFY_CHANGE_NAME, hEvent, 1);
    if (lSts != ERROR_INVALID_HANDLE) xERROR(3,lSts);
}

/******************************************************************************
 * TestOpenKey
 */
void TestOpenKey()
{
    long lSts;
    HKEY hkey;

    lSts = RegOpenKey((HKEY)72, "",&hkey);
    if (lSts != ERROR_SUCCESS) xERROR(1,lSts);
    RegCloseKey(hkey);

    lSts = RegOpenKey((HKEY)2, "regtest",&hkey);
    if (lSts != ERROR_INVALID_HANDLE) xERROR(2,lSts);

    lSts = RegOpenKey(HKEY_CURRENT_USER, "regtest",&hkey);
    if (lSts != ERROR_FILE_NOT_FOUND) xERROR(3,lSts);

    lSts = RegOpenKey(HKEY_CURRENT_USER, "\\regtest",&hkey);
    if (lSts != ERROR_BAD_PATHNAME) xERROR(4,lSts);
}

/******************************************************************************
 * TestOpenKeyEx
 */
void TestOpenKeyEx()
{
    long lSts;
    HKEY hkey;

    lSts = RegOpenKeyEx((HKEY)2,"",0,KEY_ALL_ACCESS,&hkey);
    if (lSts != ERROR_INVALID_HANDLE) xERROR(1,lSts);

    lSts = RegOpenKeyEx(HKEY_CURRENT_USER,"\\regtest",0,KEY_ALL_ACCESS,&hkey);
    if (lSts != ERROR_BAD_PATHNAME) xERROR(2,lSts);

    lSts = RegOpenKeyEx(HKEY_CURRENT_USER,"regtest",0,0,&hkey);
    if (lSts != ERROR_FILE_NOT_FOUND) xERROR(3,lSts);

    lSts = RegOpenKeyEx(HKEY_CURRENT_USER,"regtest\\",0,0,&hkey);
    if (lSts != ERROR_FILE_NOT_FOUND) xERROR(4,lSts);
}

/******************************************************************************
 * TestQueryInfoKey
 */
void TestQueryInfoKey()
{
    long lSts;
    char *sClass;
    unsigned long lClass;
    unsigned long lSubKeys;
    unsigned long lMaxSubLen;
    unsigned long lMaxClassLen;
    unsigned long lValues;
    unsigned long lMaxValNameLen;
    unsigned long lMaxValLen;
    unsigned long lSecDescLen;
    FILETIME ft;

    lClass = 1;
    sClass = (char *)malloc(lClass * sizeof(char));

    lSts = RegQueryInfoKey((HKEY)2,sClass,&lClass,0,&lSubKeys,&lMaxSubLen,
                           &lMaxClassLen,&lValues,&lMaxValNameLen,&lMaxValLen,
                           &lSecDescLen, &ft);
    if (lSts != ERROR_INVALID_HANDLE) xERROR(1,lSts);

    lSts = RegQueryInfoKey(HKEY_CURRENT_USER,sClass,&lClass,0,&lSubKeys,
                           &lMaxSubLen,&lMaxClassLen,&lValues,&lMaxValNameLen,
                           &lMaxValLen,&lSecDescLen, &ft);
    if (lSts != ERROR_SUCCESS) xERROR(2,lSts);
}

/******************************************************************************
 * TestQueryValue
 */
void TestQueryValue()
{
    long lSts;
    long lLen;
    char *sVal;

    sVal = (char *)malloc(80 * sizeof(char));
    lLen = strlen(sVal);

    lSts = RegQueryValue((HKEY)2,"",NULL,&lLen);
    if (lSts != ERROR_INVALID_HANDLE) xERROR(1,lSts);

    lSts = RegQueryValue(HKEY_CURRENT_USER,"",NULL,&lLen);
    if (lSts != ERROR_SUCCESS) xERROR(2,lSts);

    lSts = RegQueryValue(HKEY_CURRENT_USER,"\\regtest",NULL,&lLen);
    if (lSts != ERROR_BAD_PATHNAME) xERROR(3,lSts);

    lSts = RegQueryValue(HKEY_CURRENT_USER,"",sVal,&lLen);
    if (lSts != ERROR_SUCCESS) xERROR(4,lSts);
}

/******************************************************************************
 * TestQueryValueEx
 */
void TestQueryValueEx()
{
    char *sVal;
    long lSts;
    unsigned long lType;
    unsigned long lLen;

    lLen = 80;
    sVal = (char *)malloc(lLen * sizeof(char));

    lSts = RegQueryValueEx((HKEY)2,"",0,&lType,sVal,&lLen);
    if (lSts != ERROR_INVALID_HANDLE) xERROR(1,lSts);

    lSts = RegQueryValueEx(HKEY_CURRENT_USER,"",(LPDWORD)1,&lType,sVal,&lLen);
    if (lSts != ERROR_INVALID_PARAMETER) xERROR(2,lSts);

    lSts = RegQueryValueEx(HKEY_LOCAL_MACHINE,"",0,&lType,sVal,&lLen);
    if (lSts != ERROR_SUCCESS) xERROR(3,lSts);
}

/******************************************************************************
 * TestReplaceKey
 */
void TestReplaceKey()
{
    long lSts;

    lSts = RegReplaceKey((HKEY)2,"","","");
    if (lSts != ERROR_INVALID_HANDLE) xERROR(1,lSts);

#if CHECK_SAM
    lSts = RegReplaceKey(HKEY_LOCAL_MACHINE,"","","");
    if (lSts != ERROR_ACCESS_DENIED) xERROR(2,lSts);

    lSts = RegReplaceKey(HKEY_LOCAL_MACHINE,"Software","","");
    if (lSts != ERROR_ACCESS_DENIED) xERROR(3,lSts);
#endif
}

/******************************************************************************
 * TestRestoreKey
 */
void TestRestoreKey()
{
    long lSts;

    lSts = RegRestoreKey((HKEY)2,"",0);
    if (lSts != ERROR_INVALID_PARAMETER) xERROR(1,lSts);

    lSts = RegRestoreKey(HKEY_LOCAL_MACHINE,"",0);
    if (lSts != ERROR_INVALID_PARAMETER) xERROR(2,lSts);

    lSts = RegRestoreKey(HKEY_LOCAL_MACHINE,"a.a",0);
    if (lSts != ERROR_FILE_NOT_FOUND) xERROR(3,lSts);
}

/******************************************************************************
 * TestSaveKey
 */
void TestSaveKey()
{
    long lSts;

    lSts = RegSaveKey((HKEY)2,"",NULL);
    if (lSts != ERROR_INVALID_PARAMETER) xERROR(1,lSts);

    lSts = RegSaveKey(HKEY_LOCAL_MACHINE,"",NULL);
    if (lSts != ERROR_INVALID_PARAMETER) xERROR(2,lSts);

#if CHECK_SAM
    lSts = RegSaveKey(HKEY_LOCAL_MACHINE,"a.a",NULL);
    if (lSts != ERROR_PRIVILEGE_NOT_HELD) xERROR(3,lSts);
#endif
}

/******************************************************************************
 * TestSetKeySecurity
 */
void TestSetKeySecurity()
{
    long lSts;
    SECURITY_DESCRIPTOR sd;

    lSts = RegSetKeySecurity((HKEY)2,0,NULL);
    if (lSts != ERROR_INVALID_PARAMETER) xERROR(1,lSts);

    lSts = RegSetKeySecurity(HKEY_LOCAL_MACHINE,0,NULL);
    if (lSts != ERROR_INVALID_PARAMETER) xERROR(2,lSts);

    lSts = RegSetKeySecurity(HKEY_LOCAL_MACHINE,OWNER_SECURITY_INFORMATION,NULL);
    if (lSts != ERROR_INVALID_PARAMETER) xERROR(3,lSts);

    lSts = RegSetKeySecurity(HKEY_LOCAL_MACHINE,OWNER_SECURITY_INFORMATION,&sd);
    if (lSts != ERROR_INVALID_PARAMETER) xERROR(4,lSts);
}

/******************************************************************************
 * TestSetValue
 */
void TestSetValue()
{
#if MAKE_NT_CRASH
    long lSts;
#endif

#if MAKE_NT_CRASH
    lSts = RegSetValue((HKEY)2,"",0,NULL,0);
    if (lSts != ERROR_INVALID_PARAMETER) xERROR(1,lSts);
#endif

#if MAKE_NT_CRASH
    lSts = RegSetValue((HKEY)2,"regtest",0,NULL,0);
    if (lSts != ERROR_INVALID_PARAMETER) xERROR(2,lSts);
#endif

#if MAKE_NT_CRASH
    lSts = RegSetValue((HKEY)2,"regtest",REG_SZ,NULL,0);
    if (lSts != ERROR_INVALID_HANDLE) xERROR(3,lSts);
#endif

#if MAKE_NT_CRASH
    lSts = RegSetValue(HKEY_LOCAL_MACHINE,"regtest",REG_SZ,NULL,0);
    if (lSts != ERROR_INVALID_HANDLE) xERROR(4,lSts);
#endif
}

/******************************************************************************
 * TestSetValueEx
 */
void TestSetValueEx()
{
    long lSts;

    lSts = RegSetValueEx((HKEY)2,"",0,0,NULL,0);
    if (lSts != ERROR_INVALID_HANDLE) xERROR(1,lSts);

    lSts = RegSetValueEx(HKEY_LOCAL_MACHINE,"",0,0,NULL,0);
    if (lSts != ERROR_SUCCESS) xERROR(2,lSts);
}

/******************************************************************************
 * TestUnLoadKey
 */
void TestUnLoadKey()
{
#if CHECK_SAM
    long lSts;
#endif

#if CHECK_SAM
    lSts = RegUnLoadKey((HKEY)2,"");
    if (lSts != ERROR_PRIVILEGE_NOT_HELD) xERROR(1,lSts);

    lSts = RegUnLoadKey(HKEY_LOCAL_MACHINE,"");
    if (lSts != ERROR_PRIVILEGE_NOT_HELD) xERROR(2,lSts);

    lSts = RegUnLoadKey(HKEY_LOCAL_MACHINE,"\\regtest");
    if (lSts != ERROR_PRIVILEGE_NOT_HELD) xERROR(3,lSts);
#endif
}

/******************************************************************************
 * TestSequence1
 */
void TestSequence1()
{
    HKEY hkey;
    long lSts;

    lSts = RegCreateKey(HKEY_CURRENT_USER,"regtest",&hkey);
    if (lSts != ERROR_SUCCESS) xERROR(1,lSts);

/*    fprintf(stderr, " hkey=0x%x\n", hkey); */

    lSts = RegDeleteKey(HKEY_CURRENT_USER, "regtest");
    if (lSts != ERROR_SUCCESS) xERROR(2,lSts);
    lSts = RegCloseKey(hkey);
    if (lSts != ERROR_SUCCESS) xERROR(3,lSts);
}


int PASCAL WinMain (HINSTANCE inst, HINSTANCE prev, LPSTR cmdline, int show)
{

    /* These can be in any order */
    TestCloseKey();
    TestConnectRegistry();
    TestCreateKey();
    TestCreateKeyEx();
    TestDeleteKey();
    TestDeleteValue();
    TestEnumKey();
    TestEnumKeyEx();
    TestEnumValue();
    TestFlushKey();
    TestGetKeySecurity();
    TestLoadKey();
    TestNotifyChangeKeyValue();
    TestOpenKey();
    TestOpenKeyEx();
    TestQueryInfoKey();
    TestQueryValue();
    TestQueryValueEx();
    TestReplaceKey();
    TestRestoreKey();
    TestSaveKey();
    TestSetKeySecurity();
    TestSetValue();
    TestSetValueEx();
    TestUnLoadKey();
    TestCreateKeyEx1();

    /* Now we have some sequence testing */
    TestSequence1();

    return 0;
}

