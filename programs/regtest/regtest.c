/*
 * Registry testing program
 *
 * The return codes were generated in an NT40 environment, using lcc-win32
 *
 * Copyright 1998 by Matthew Becker (mbecker@glasscity.net)
 *
*/

#include <stdio.h>
#include <malloc.h>
#include <windows.h>
#include <winreg.h>
#include <winerror.h>

/*
 * NOTES: These individual routines are listed in alphabetical order.
 */

/******************************************************************************
 * TestCloseKey
 */
void TestCloseKey()
{
    long lSts;
    HKEY hkey;
    fprintf(stderr, "Testing RegCloseKey...\n");

    hkey = (HKEY)0;
    lSts = RegCloseKey(hkey);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);

    hkey = (HKEY)-2;
    lSts = RegCloseKey(hkey);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 2:%ld\n",lSts);

    lSts = RegCloseKey(HKEY_LOCAL_MACHINE);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 3:%ld\n",lSts);

    lSts = RegCloseKey(HKEY_LOCAL_MACHINE);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 4:%ld\n",lSts);
}

/******************************************************************************
 * TestConnectRegistry
 */
void TestConnectRegistry()
{
    long lSts;
    HKEY hkey;
    fprintf(stderr, "Testing RegConnectRegistry...\n");

    lSts = RegConnectRegistry("",0,&hkey);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegConnectRegistry("",HKEY_LOCAL_MACHINE,&hkey);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 2:%ld\n",lSts);
}

/******************************************************************************
 * TestCreateKey
 */
void TestCreateKey()
{
    HKEY hKeyIn;
    long lSts;
    HKEY hkey;

    fprintf(stderr, "Testing RegCreateKey...\n");

    hKeyIn = (HKEY)1;
    lSts = RegCreateKey(hKeyIn,"",&hkey);
    if (lSts != ERROR_BADKEY) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegCreateKey(HKEY_LOCAL_MACHINE,"\\asdf",&hkey);
    if (lSts != ERROR_BAD_PATHNAME) fprintf(stderr, " 2:%ld\n",lSts);
}

/******************************************************************************
 * TestCreateKeyEx
 */
void TestCreateKeyEx()
{
    HKEY hKeyIn;
    long lSts;
    HKEY hkey;
    DWORD dwDisp;

    fprintf(stderr, "Testing RegCreateKeyEx...\n");

    hKeyIn = (HKEY)1;
    lSts = RegCreateKeyEx(hKeyIn,"",0,"",0,0,NULL,&hkey,&dwDisp);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegCreateKeyEx(hKeyIn,"regtest",0,"",0,0,NULL,&hkey,&dwDisp);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 2:%ld\n",lSts);

    lSts = RegCreateKeyEx(hKeyIn,"regtest",0,"asdf",0,KEY_ALL_ACCESS,NULL,&hkey,&dwDisp);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 3:%ld\n",lSts);
}

/******************************************************************************
 * TestDeleteKey
 */
void TestDeleteKey()
{
    long lSts;
    HKEY hkey;
    fprintf(stderr, "Testing RegDeleteKey...\n");

    hkey = (HKEY)0;
    lSts = RegDeleteKey(hkey, "asdf");
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegDeleteKey(HKEY_CURRENT_USER, "asdf");
    if (lSts != ERROR_FILE_NOT_FOUND) fprintf(stderr, " 2:%ld\n",lSts);
}

/******************************************************************************
 * TestDeleteValue
 */
void TestDeleteValue()
{
    long lSts;
    HKEY hkey;
    fprintf(stderr, "Testing RegDeleteValue...\n");

    hkey = (HKEY)0;
    lSts = RegDeleteValue(hkey, "asdf");
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegDeleteValue(HKEY_CURRENT_USER, "asdf");
    if (lSts != ERROR_FILE_NOT_FOUND) fprintf(stderr, " 2:%ld\n",lSts);
}

/******************************************************************************
 * TestEnumKey
 */
void TestEnumKey()
{
    long lSts;
    char *sVal;
    long lVal;
    HKEY hkey;

    fprintf(stderr, "Testing RegEnumKey...\n");
    sVal = (char *)malloc(1 * sizeof(char));
    lVal = sizeof(sVal);

    hkey = (HKEY)0;
    lSts = RegEnumKey(hkey,3,sVal,lVal);
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
    sVal = (char *)malloc(80 * sizeof(char));
    lLen1= sizeof(sVal);
    sClass = (char *)malloc(10 * sizeof(char));
    lLen2 = sizeof(sClass);

    lSts = RegEnumKeyEx(0,0,sVal,&lLen1,0,sClass,&lLen2,&ft);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);
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
    HKEY hkey;

    fprintf(stderr, "Testing RegEnumValue...\n");
    sVal = (char *)malloc(80 * sizeof(char));
    bVal = (char *)malloc(80 * sizeof(char));
    lVal = sizeof(sVal);
    lLen1 = sizeof(bVal);

    hkey = (HKEY)0;
    lSts = RegEnumValue(hkey,-1,sVal,&lVal,0,&lType,NULL,&lLen1);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegEnumValue(HKEY_LOCAL_MACHINE,-1,sVal,&lVal,0,&lType,NULL,&lLen1);
    if (lSts != ERROR_NO_MORE_ITEMS) fprintf(stderr, " 2:%ld\n",lSts);

    lSts = RegEnumValue(HKEY_LOCAL_MACHINE,0,sVal,&lVal,0,&lType,NULL,&lLen1);
    if (lSts != ERROR_NO_MORE_ITEMS) fprintf(stderr, " 3:%ld\n",lSts);

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

    lSts = RegFlushKey(0);
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
    HKEY hkey;

    fprintf(stderr, "Testing RegGetKeySecurity...\n");
    lLen = sizeof(sd);

    hkey = (HKEY)0;
    lSts = RegGetKeySecurity(hkey,si,&sd,&lLen);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegGetKeySecurity(HKEY_LOCAL_MACHINE,si,&sd,&lLen);
    if (lSts != ERROR_INSUFFICIENT_BUFFER) fprintf(stderr, " 2:%ld\n",lSts);

#if 0
    si = GROUP_SECURITY_INFORMATION;
#endif
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

    lSts = RegLoadKey(0,"","");
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegLoadKey(HKEY_CURRENT_USER,"","");
    if (lSts != ERROR_INVALID_PARAMETER) fprintf(stderr, " 2:%ld\n",lSts);

    lSts = RegLoadKey(HKEY_CURRENT_USER,"regtest","");
    if (lSts != ERROR_INVALID_PARAMETER) fprintf(stderr, " 3:%ld\n",lSts);

    lSts = RegLoadKey(HKEY_CURRENT_USER,"regtest","regtest.dat");
    if (lSts != ERROR_PRIVILEGE_NOT_HELD) fprintf(stderr, " 4:%ld\n",lSts);
}

/******************************************************************************
 * TestNotifyChangeKeyValue
 */
void TestNotifyChangeKeyValue()
{
    long lSts;
    HANDLE hEvent;
    HKEY hkey;

    fprintf(stderr, "Testing RegNotifyChangeKeyValue...\n");
    hEvent = 0;

    hkey = (HKEY)2;
    lSts = RegNotifyChangeKeyValue(hkey, TRUE, REG_NOTIFY_CHANGE_NAME, 0, 0);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegNotifyChangeKeyValue(HKEY_CURRENT_USER, TRUE, REG_NOTIFY_CHANGE_NAME, 0, 1);
    if (lSts != ERROR_INVALID_PARAMETER) fprintf(stderr, " 2:%ld\n",lSts);

    lSts = RegNotifyChangeKeyValue(HKEY_CURRENT_USER, TRUE, REG_NOTIFY_CHANGE_NAME, hEvent, 1);
    if (lSts != ERROR_INVALID_PARAMETER) fprintf(stderr, " 3:%ld\n",lSts);
}

/******************************************************************************
 * TestOpenKey
 */
void TestOpenKey()
{
    long lSts;
    HKEY hkey;
    fprintf(stderr, "Testing RegOpenKey...\n");

    lSts = RegOpenKey(0, "",&hkey);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegOpenKey(0, "regtest",&hkey);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 2:%ld\n",lSts);

    lSts = RegOpenKey(HKEY_CURRENT_USER, "regtest1",&hkey);
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
    HKEY hkeyIn;
    HKEY hkey;
    fprintf(stderr, "Testing RegOpenKeyEx...\n");

    hkeyIn = (HKEY)0;
    lSts = RegOpenKeyEx(hkeyIn,"",0,KEY_ALL_ACCESS,&hkey);
    if (lSts != ERROR_INVALID_HANDLE) fprintf(stderr, " 1:%ld\n",lSts);

    lSts = RegOpenKeyEx(HKEY_CURRENT_USER,"\\regtest",0,KEY_ALL_ACCESS,&hkey);
    if (lSts != ERROR_BAD_PATHNAME) fprintf(stderr, " 2:%ld\n",lSts);
}

/******************************************************************************
 * TestQueryInfoKey
 */
void TestQueryInfoKey()
{
    fprintf(stderr, "Testing RegQueryInfoKey...\n");
}

/******************************************************************************
 * TestQueryValue
 */
void TestQueryValue()
{
    long lSts;
    HKEY hkey;
    long lLen;
    char *sVal;

    fprintf(stderr, "Testing RegQueryValue...\n");
    hkey = (HKEY)0;
    sVal = (char *)malloc(80 * sizeof(char));
    lLen = strlen(sVal);

    lSts = RegQueryValue(hkey,"",NULL,&lLen);
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
    fprintf(stderr, "Testing RegQueryValueEx...\n");
}

/******************************************************************************
 * TestReplaceKey
 */
void TestReplaceKey()
{
    fprintf(stderr, "Testing RegReplaceKey...\n");
}

/******************************************************************************
 * TestRestoreKey
 */
void TestRestoreKey()
{
    fprintf(stderr, "Testing RegRestoreKey...\n");
}

void TestSequence1()
{
    HKEY hkey;
    long lSts;

    fprintf(stderr, "Testing Sequence1...\n");

    lSts = RegCreateKey(HKEY_CURRENT_USER,"regtest",&hkey);
    if (lSts != ERROR_SUCCESS) fprintf(stderr, " 1:%ld\n",lSts);
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
/*
    TestSaveKey();
    TestSetKeySecurity();
    TestSetValue();
    TestSetValueEx();
    TestUnloadKey();
*/

    /* Now we have some sequence testing */
    TestSequence1();

    return 0;
}

