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

/* True this when security is implemented */
#define CHECK_SAM FALSE

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
    fprintf(stderr, "Testing RegCloseKey...\n");

    lSts = RegCloseKey((HKEY)2);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegCloseKey(HKEY_LOCAL_MACHINE);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 2:%ld\n",lSts);

    /* Check twice just for kicks */
    lSts = RegCloseKey(HKEY_LOCAL_MACHINE);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 3:%ld\n",lSts);
}

/******************************************************************************
 * TestConnectRegistry
 */
void TestConnectRegistry()
{
    long lSts;
    HKEY hkey;
    fprintf(stderr, "Testing RegConnectRegistry...\n");

    lSts = RegConnectRegistry("",(HKEY)2,&hkey);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegConnectRegistry("",HKEY_LOCAL_MACHINE,&hkey);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 2:%ld\n",lSts);

#if TOO_SLOW
    lSts = RegConnectRegistry("\\\\regtest",HKEY_LOCAL_MACHINE,&hkey);
    if (lSts != ERROR_BAD_NETPATH) fprintf(stderr, " 3:%ld\n",lSts);
#endif
}

/******************************************************************************
 * TestCreateKey
 */
void TestCreateKey()
{
    long lSts;
    HKEY hkey;

    fprintf(stderr, "Testing RegCreateKey...\n");

    lSts = RegCreateKey((HKEY)2,"",&hkey);
    if (lSts != ERROR_BADKEY) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegCreateKey(HKEY_LOCAL_MACHINE,"",&hkey);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 2:%ld\n",lSts);
    RegCloseKey(hkey);

    lSts = RegCreateKey(HKEY_LOCAL_MACHINE,"\\asdf",&hkey);
    if (lSts != ERROR_BAD_PATHNAME) fprintf(stderr, " 3:%ld\n",lSts);

#if 0
    lSts = RegCreateKey(HKEY_LOCAL_MACHINE,"asdf\\",&hkey);
    if (lSts != ERROR_INVALID_PARAMETER) fprintf(stderr, " 4:%ld\n",lSts);
#endif

    lSts = RegCreateKey(HKEY_LOCAL_MACHINE,"\\asdf\\",&hkey);
    if (lSts != ERROR_BAD_PATHNAME) fprintf(stderr, " 5:%ld\n",lSts);
}

/******************************************************************************
 * TestCreateKeyEx
 */
void TestCreateKeyEx()
{
    long lSts;
    HKEY hkey;
    DWORD dwDisp;

    fprintf(stderr, "Testing RegCreateKeyEx...\n");

    lSts = RegCreateKeyEx((HKEY)2,"",0,"",0,0,NULL,&hkey,&dwDisp);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegCreateKeyEx(HKEY_LOCAL_MACHINE,"regtest",0,"",0,0,NULL,&hkey,
                          &dwDisp);
    if (lSts != ERROR_INVALID_PARAMETER) fprintf(stderr, " 2:%ld\n",lSts);

    lSts = RegCreateKeyEx(HKEY_LOCAL_MACHINE,"regtest",0,"asdf",0,
                          KEY_ALL_ACCESS,NULL,&hkey,&dwDisp);
    if (lSts != ERROR_INVALID_PARAMETER) fprintf(stderr, " 3:%ld\n",lSts);

    lSts = RegCreateKeyEx(HKEY_LOCAL_MACHINE,"regtest",0,"",0,
                          KEY_ALL_ACCESS,NULL,&hkey,&dwDisp);
    if (lSts != ERROR_INVALID_PARAMETER) fprintf(stderr, " 4:%ld\n",lSts);

}

/******************************************************************************
 * TestDeleteKey
 */
void TestDeleteKey()
{
    long lSts;
    fprintf(stderr, "Testing RegDeleteKey...\n");

    lSts = RegDeleteKey((HKEY)2, "asdf");
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegDeleteKey(HKEY_CURRENT_USER, "asdf");
    if (lSts != ERROR_FILE_NOT_FOUND) fprintf(stderr, " 2:%ld\n",lSts);

#if CHECK_SAM
    lSts = RegDeleteKey(HKEY_CURRENT_USER, "");
    if (lSts != ERROR_ACCESS_DENIED) fprintf(stderr, " 3:%ld\n",lSts);
#endif
}

/******************************************************************************
 * TestDeleteValue
 */
void TestDeleteValue()
{
    long lSts;
    fprintf(stderr, "Testing RegDeleteValue...\n");

    lSts = RegDeleteValue((HKEY)2, "asdf");
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegDeleteValue(HKEY_CURRENT_USER, "");
    if (lSts != ERROR_FILE_NOT_FOUND) fprintf(stderr, " 2:%ld\n",lSts);

    lSts = RegDeleteValue(HKEY_CURRENT_USER, "asdf");
    if (lSts != ERROR_FILE_NOT_FOUND) fprintf(stderr, " 3:%ld\n",lSts);

    lSts = RegDeleteValue(HKEY_CURRENT_USER, "\\asdf");
    if (lSts != ERROR_FILE_NOT_FOUND) fprintf(stderr, " 4:%ld\n",lSts);
}

/******************************************************************************
 * TestEnumKey
 */
void TestEnumKey()
{
    long lSts;
    char *sVal;
    long lVal;

    fprintf(stderr, "Testing RegEnumKey...\n");
    lVal = 1;
    sVal = (char *)malloc(lVal * sizeof(char));

    lSts = RegEnumKey((HKEY)2,3,sVal,lVal);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegEnumKey(HKEY_CURRENT_USER,-1,sVal,lVal);
    if (lSts != ERROR_NO_MORE_ITEMS) fprintf(stderr, " 2:%ld\n",lSts);

    lSts = RegEnumKey(HKEY_CURRENT_USER,0,sVal,lVal);
    if (lSts != ERROR_MORE_DATA) fprintf(stderr, " 3:%ld\n",lSts);
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

    fprintf(stderr, "Testing RegEnumKeyEx...\n");
    lLen1 = 1;
    sVal = (char *)malloc(lLen1 * sizeof(char));
    lLen2 = 1;
    sClass = (char *)malloc(lLen2 * sizeof(char));

    lSts = RegEnumKeyEx((HKEY)2,0,sVal,&lLen1,0,sClass,&lLen2,&ft);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegEnumKeyEx(HKEY_LOCAL_MACHINE,0,sVal,&lLen1,0,sClass,&lLen2,&ft);
    if (lSts != ERROR_MORE_DATA) fprintf(stderr, " 2:%ld\n",lSts);

    lSts = RegEnumKeyEx(HKEY_LOCAL_MACHINE,0,sVal,&lLen1,0,sClass,&lLen2,&ft);
    if (lSts != ERROR_MORE_DATA) fprintf(stderr, " 3:%ld\n",lSts);
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

    fprintf(stderr, "Testing RegEnumValue...\n");
    lVal = 1;
    sVal = (char *)malloc(lVal * sizeof(char));
    lLen1 = 1;
    bVal = (char *)malloc(lLen1 * sizeof(char));

    lSts = RegEnumValue((HKEY)2,-1,sVal,&lVal,0,&lType,NULL,&lLen1);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegEnumValue(HKEY_LOCAL_MACHINE,-1,sVal,&lVal,0,&lType,NULL,&lLen1);
    if (lSts != ERROR_NO_MORE_ITEMS) fprintf(stderr, " 2:%ld\n",lSts);

    lSts = RegEnumValue(HKEY_LOCAL_MACHINE,0,sVal,&lVal,0,&lType,NULL,&lLen1);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 3:%ld\n",lSts);

    lSts = RegEnumValue(HKEY_LOCAL_MACHINE,1,sVal,&lVal,0,&lType,bVal,&lLen1);
    if (lSts != ERROR_NO_MORE_ITEMS) fprintf(stderr, " 4:%ld\n",lSts);
}

/******************************************************************************
 * TestFlushKey
 */
void TestFlushKey()
{
    long lSts;
    fprintf(stderr, "Testing RegFlushKey...\n");

    lSts = RegFlushKey((HKEY)2);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegFlushKey(HKEY_LOCAL_MACHINE);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 2:%ld\n",lSts);
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

    fprintf(stderr, "Testing RegGetKeySecurity...\n");
    lLen = sizeof(sd);

    lSts = RegGetKeySecurity((HKEY)2,si,&sd,&lLen);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegGetKeySecurity(HKEY_LOCAL_MACHINE,si,&sd,&lLen);
    if (lSts != ERROR_INSUFFICIENT_BUFFER) fprintf(stderr, " 2:%ld\n",lSts);

    si = GROUP_SECURITY_INFORMATION;
    lSts = RegGetKeySecurity(HKEY_LOCAL_MACHINE,si,&sd,&lLen);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 3:%ld\n",lSts);
}

/******************************************************************************
 * TestLoadKey
 */
void TestLoadKey()
{
    long lSts;
    fprintf(stderr, "Testing RegLoadKey...\n");

    lSts = RegLoadKey((HKEY)2,"","");
    if (lSts != ERROR_INVALID_PARAMETER) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegLoadKey(HKEY_CURRENT_USER,"","");
    if (lSts != ERROR_INVALID_PARAMETER) fprintf(stderr, " 2:%ld\n",lSts);

    lSts = RegLoadKey(HKEY_CURRENT_USER,"regtest","");
    if (lSts != ERROR_INVALID_PARAMETER) fprintf(stderr, " 3:%ld\n",lSts);

    lSts = RegLoadKey(HKEY_CURRENT_USER,"\\regtest","");
    if (lSts != ERROR_INVALID_PARAMETER) fprintf(stderr, " 4:%ld\n",lSts);

#if CHECK_SAM
    lSts = RegLoadKey(HKEY_CURRENT_USER,"regtest","regtest.dat");
    if (lSts != ERROR_PRIVILEGE_NOT_HELD) fprintf(stderr, " 5:%ld\n",lSts);

    lSts = RegLoadKey(HKEY_CURRENT_USER,"\\regtest","regtest.dat");
    if (lSts != ERROR_PRIVILEGE_NOT_HELD) fprintf(stderr, " 6:%ld\n",lSts);
#endif
}

/******************************************************************************
 * TestNotifyChangeKeyValue
 */
void TestNotifyChangeKeyValue()
{
    long lSts;
    HANDLE hEvent;

    fprintf(stderr, "Testing RegNotifyChangeKeyValue...\n");
    hEvent = (HANDLE)0;

    lSts = RegNotifyChangeKeyValue((HKEY)2, TRUE, REG_NOTIFY_CHANGE_NAME, 0, 0);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegNotifyChangeKeyValue(HKEY_CURRENT_USER, TRUE, REG_NOTIFY_CHANGE_NAME, 0, 1);
    if (lSts != ERROR_INVALID_PARAMETER) fprintf(stderr, " 2:%ld\n",lSts);

    hEvent = (HANDLE)HKEY_CURRENT_USER;
    lSts = RegNotifyChangeKeyValue(HKEY_CURRENT_USER, TRUE, REG_NOTIFY_CHANGE_NAME, hEvent, 1);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 3:%ld\n",lSts);
}

/******************************************************************************
 * TestOpenKey
 */
void TestOpenKey()
{
    long lSts;
    HKEY hkey;
    fprintf(stderr, "Testing RegOpenKey...\n");

    lSts = RegOpenKey((HKEY)72, "",&hkey);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 1:%ld\n",lSts);
    RegCloseKey(hkey);

    lSts = RegOpenKey((HKEY)2, "regtest",&hkey);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 2:%ld\n",lSts);

    lSts = RegOpenKey(HKEY_CURRENT_USER, "regtest",&hkey);
    if (lSts != ERROR_FILE_NOT_FOUND) fprintf(stderr, " 3:%ld\n",lSts);

    lSts = RegOpenKey(HKEY_CURRENT_USER, "\\regtest",&hkey);
    if (lSts != ERROR_BAD_PATHNAME) fprintf(stderr, " 4:%ld\n",lSts);
}

/******************************************************************************
 * TestOpenKeyEx
 */
void TestOpenKeyEx()
{
    long lSts;
    HKEY hkey;
    fprintf(stderr, "Testing RegOpenKeyEx...\n");

    lSts = RegOpenKeyEx((HKEY)2,"",0,KEY_ALL_ACCESS,&hkey);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegOpenKeyEx(HKEY_CURRENT_USER,"\\regtest",0,KEY_ALL_ACCESS,&hkey);
    if (lSts != ERROR_BAD_PATHNAME) fprintf(stderr, " 2:%ld\n",lSts);

    lSts = RegOpenKeyEx(HKEY_CURRENT_USER,"regtest",0,0,&hkey);
    if (lSts != ERROR_FILE_NOT_FOUND) fprintf(stderr, " 3:%ld\n",lSts);

    lSts = RegOpenKeyEx(HKEY_CURRENT_USER,"regtest\\",0,0,&hkey);
    if (lSts != ERROR_FILE_NOT_FOUND) fprintf(stderr, " 3:%ld\n",lSts);
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
    fprintf(stderr, "Testing RegQueryInfoKey...\n");

    lClass = 1;
    sClass = (char *)malloc(lClass * sizeof(char));

    lSts = RegQueryInfoKey((HKEY)2,sClass,&lClass,0,&lSubKeys,&lMaxSubLen,
                           &lMaxClassLen,&lValues,&lMaxValNameLen,&lMaxValLen,
                           &lSecDescLen, &ft);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegQueryInfoKey(HKEY_CURRENT_USER,sClass,&lClass,0,&lSubKeys,
                           &lMaxSubLen,&lMaxClassLen,&lValues,&lMaxValNameLen,
                           &lMaxValLen,&lSecDescLen, &ft);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 2:%ld\n",lSts);
}

/******************************************************************************
 * TestQueryValue
 */
void TestQueryValue()
{
    long lSts;
    long lLen;
    char *sVal;

    fprintf(stderr, "Testing RegQueryValue...\n");
    sVal = (char *)malloc(80 * sizeof(char));
    lLen = strlen(sVal);

    lSts = RegQueryValue((HKEY)2,"",NULL,&lLen);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegQueryValue(HKEY_CURRENT_USER,"",NULL,&lLen);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 2:%ld\n",lSts);

    lSts = RegQueryValue(HKEY_CURRENT_USER,"\\regtest",NULL,&lLen);
    if (lSts != ERROR_BAD_PATHNAME) fprintf(stderr, " 3:%ld\n",lSts);

    lSts = RegQueryValue(HKEY_CURRENT_USER,"",sVal,&lLen);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 4:%ld\n",lSts);
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

    fprintf(stderr, "Testing RegQueryValueEx...\n");
    lLen = 80;
    sVal = (char *)malloc(lLen * sizeof(char));

    lSts = RegQueryValueEx((HKEY)2,"",0,&lType,sVal,&lLen);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegQueryValueEx(HKEY_CURRENT_USER,"",(LPDWORD)1,&lType,sVal,&lLen);
    if (lSts != ERROR_INVALID_PARAMETER) fprintf(stderr, " 2:%ld\n",lSts);

    lSts = RegQueryValueEx(HKEY_LOCAL_MACHINE,"",0,&lType,sVal,&lLen);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 3:%ld\n",lSts);
}

/******************************************************************************
 * TestReplaceKey
 */
void TestReplaceKey()
{
    long lSts;

    fprintf(stderr, "Testing RegReplaceKey...\n");

    lSts = RegReplaceKey((HKEY)2,"","","");
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);

#if CHECK_SAM
    lSts = RegReplaceKey(HKEY_LOCAL_MACHINE,"","","");
    if (lSts != ERROR_ACCESS_DENIED) fprintf(stderr, " 2:%ld\n",lSts);

    lSts = RegReplaceKey(HKEY_LOCAL_MACHINE,"Software","","");
    if (lSts != ERROR_ACCESS_DENIED) fprintf(stderr, " 3:%ld\n",lSts);
#endif
}

/******************************************************************************
 * TestRestoreKey
 */
void TestRestoreKey()
{
    long lSts;
    fprintf(stderr, "Testing RegRestoreKey...\n");

    lSts = RegRestoreKey((HKEY)2,"",0);
    if (lSts != ERROR_INVALID_PARAMETER) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegRestoreKey(HKEY_LOCAL_MACHINE,"",0);
    if (lSts != ERROR_INVALID_PARAMETER) fprintf(stderr, " 2:%ld\n",lSts);

    lSts = RegRestoreKey(HKEY_LOCAL_MACHINE,"a.a",0);
    if (lSts != ERROR_FILE_NOT_FOUND) fprintf(stderr, " 3:%ld\n",lSts);
}

/******************************************************************************
 * TestSaveKey
 */
void TestSaveKey()
{
    long lSts;
    fprintf(stderr, "Testing RegSaveKey...\n");

    lSts = RegSaveKey((HKEY)2,"",NULL);
    if (lSts != ERROR_INVALID_PARAMETER) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegSaveKey(HKEY_LOCAL_MACHINE,"",NULL);
    if (lSts != ERROR_INVALID_PARAMETER) fprintf(stderr, " 2:%ld\n",lSts);

#if CHECK_SAM
    lSts = RegSaveKey(HKEY_LOCAL_MACHINE,"a.a",NULL);
    if (lSts != ERROR_PRIVILEGE_NOT_HELD) fprintf(stderr, " 3:%ld\n",lSts);
#endif
}

/******************************************************************************
 * TestSetKeySecurity
 */
void TestSetKeySecurity()
{
    long lSts;
    SECURITY_DESCRIPTOR sd;
    fprintf(stderr, "Testing RegSetKeySecurity...\n");

    lSts = RegSetKeySecurity((HKEY)2,0,NULL);
    if (lSts != ERROR_INVALID_PARAMETER) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegSetKeySecurity(HKEY_LOCAL_MACHINE,0,NULL);
    if (lSts != ERROR_INVALID_PARAMETER) fprintf(stderr, " 2:%ld\n",lSts);

    lSts = RegSetKeySecurity(HKEY_LOCAL_MACHINE,OWNER_SECURITY_INFORMATION,NULL);
    if (lSts != ERROR_INVALID_PARAMETER) fprintf(stderr, " 3:%ld\n",lSts);

    lSts = RegSetKeySecurity(HKEY_LOCAL_MACHINE,OWNER_SECURITY_INFORMATION,&sd);
    if (lSts != ERROR_INVALID_PARAMETER) fprintf(stderr, " 4:%ld\n",lSts);
}

/******************************************************************************
 * TestSetValue
 */
void TestSetValue()
{
    long lSts;
    fprintf(stderr, "Testing RegSetValue...\n");

#if MAKE_NT_CRASH
    lSts = RegSetValue((HKEY)2,"",0,NULL,0);
    if (lSts != ERROR_INVALID_PARAMETER) fprintf(stderr, " 1:%ld\n",lSts);
#endif

#if MAKE_NT_CRASH
    lSts = RegSetValue((HKEY)2,"regtest",0,NULL,0);
    if (lSts != ERROR_INVALID_PARAMETER) fprintf(stderr, " 2:%ld\n",lSts);
#endif

#if MAKE_NT_CRASH
    lSts = RegSetValue((HKEY)2,"regtest",REG_SZ,NULL,0);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 3:%ld\n",lSts);
#endif

#if MAKE_NT_CRASH
    lSts = RegSetValue(HKEY_LOCAL_MACHINE,"regtest",REG_SZ,NULL,0);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 4:%ld\n",lSts);
#endif
}

/******************************************************************************
 * TestSetValueEx
 */
void TestSetValueEx()
{
    long lSts;
    fprintf(stderr, "Testing RegSetValueEx...\n");

    lSts = RegSetValueEx((HKEY)2,"",0,0,NULL,0);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegSetValueEx(HKEY_LOCAL_MACHINE,"",0,0,NULL,0);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 2:%ld\n",lSts);
}

/******************************************************************************
 * TestUnLoadKey
 */
void TestUnLoadKey()
{
    long lSts;
    fprintf(stderr, "Testing RegUnloadKey...\n");

#if CHECK_SAM
    lSts = RegUnLoadKey((HKEY)2,"");
    if (lSts != ERROR_PRIVILEGE_NOT_HELD) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegUnLoadKey(HKEY_LOCAL_MACHINE,"");
    if (lSts != ERROR_PRIVILEGE_NOT_HELD) fprintf(stderr, " 2:%ld\n",lSts);

    lSts = RegUnLoadKey(HKEY_LOCAL_MACHINE,"\\regtest");
    if (lSts != ERROR_PRIVILEGE_NOT_HELD) fprintf(stderr, " 3:%ld\n",lSts);
#endif
}

/******************************************************************************
 * TestSequence1
 */
void TestSequence1()
{
    HKEY hkey;
    long lSts;

    fprintf(stderr, "Testing Sequence1...\n");

    lSts = RegCreateKey(HKEY_CURRENT_USER,"regtest",&hkey);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 1:%ld\n",lSts);

    fprintf(stderr, " hkey=0x%x\n", hkey);

    lSts = RegDeleteKey(HKEY_CURRENT_USER, "regtest");
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 2:%ld\n",lSts);
    lSts = RegCloseKey(hkey);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 3:%ld\n",lSts);
}


int PASCAL WinMain (HANDLE inst, HANDLE prev, LPSTR cmdline, int show)
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

    /* Now we have some sequence testing */
    TestSequence1();

    return 0;
}

