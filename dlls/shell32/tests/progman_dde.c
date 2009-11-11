/*
 * Unit test of the Program Manager DDE Interfaces
 *
 * Copyright 2009 Mikey Alexander
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* DDE Program Manager Tests
 * - Covers basic CreateGroup, ShowGroup, DeleteGroup, AddItem, and DeleteItem
 *   functionality
 * - Todo: Handle CommonGroupFlag
 *         Handle Difference between administrator and non-administrator calls
 *           as documented
 *         Better AddItem Tests (Lots of parameters to test)
 *         Tests for Invalid Characters in Names / Invalid Parameters
 */

#include <stdio.h>
#include <wine/test.h>
#include <winbase.h>
#include "dde.h"
#include "ddeml.h"
#include "winuser.h"
#include "shlobj.h"

/* Timeout on DdeClientTransaction Call */
#define MS_TIMEOUT_VAL 1000
/* # of times to poll for window creation */
#define PDDE_POLL_NUM 50
/* time to sleep between polls */
#define PDDE_POLL_TIME 200

/* Call Info */
#define DDE_TEST_MISC            0x00010000
#define DDE_TEST_CREATEGROUP     0x00020000
#define DDE_TEST_DELETEGROUP     0x00030000
#define DDE_TEST_SHOWGROUP       0x00040000
#define DDE_TEST_ADDITEM         0x00050000
#define DDE_TEST_DELETEITEM      0x00060000
#define DDE_TEST_COMPOUND        0x00070000
#define DDE_TEST_CALLMASK        0x00ff0000

/* Type of Test (Common, Individual) */
#define DDE_TEST_COMMON            0x01000000
#define DDE_TEST_INDIVIDUAL        0x02000000

#define DDE_TEST_NUMMASK           0x0000ffff

static BOOL (WINAPI *pSHGetSpecialFolderPathA)(HWND, LPSTR, int, BOOL);

static void init_function_pointers(void)
{
    HMODULE hmod;

    hmod = GetModuleHandleA("shell32.dll");
    pSHGetSpecialFolderPathA = (void*)GetProcAddress(hmod, "SHGetSpecialFolderPathA");
}

static HDDEDATA CALLBACK DdeCallback(UINT type, UINT format, HCONV hConv, HSZ hsz1, HSZ hsz2,
                                     HDDEDATA hDDEData, ULONG_PTR data1, ULONG_PTR data2)
{
    trace("Callback: type=%i, format=%i\n", type, format);
    return NULL;
}

/*
 * Encoded String for Error Messages so that inner failures can determine
 * what test is failing.  Format is: [Code:TestNum]
 */
static const char * GetStringFromTestParams(int testParams)
{
    int testNum;
    static char testParamString[64];
    const char *callId;

    testNum = testParams & DDE_TEST_NUMMASK;
    switch (testParams & DDE_TEST_CALLMASK)
    {
    default:
    case DDE_TEST_MISC:
        callId = "MISC";
        break;
    case DDE_TEST_CREATEGROUP:
        callId = "C_G";
        break;
    case DDE_TEST_DELETEGROUP:
        callId = "D_G";
        break;
    case DDE_TEST_SHOWGROUP:
        callId = "S_G";
        break;
    case DDE_TEST_ADDITEM:
        callId = "A_I";
        break;
    case DDE_TEST_DELETEITEM:
        callId = "D_I";
        break;
    case DDE_TEST_COMPOUND:
        callId = "CPD";
        break;
    }

    sprintf(testParamString, "  [%s:%i]", callId, testNum);
    return testParamString;
}

/* Transfer DMLERR's into text readable strings for Error Messages */
#define DMLERR_TO_STR(x) case x: return#x;
static const char * GetStringFromError(UINT err)
{
    const char * retstr;
    switch (err)
    {
    DMLERR_TO_STR(DMLERR_NO_ERROR);
    DMLERR_TO_STR(DMLERR_ADVACKTIMEOUT);
    DMLERR_TO_STR(DMLERR_BUSY);
    DMLERR_TO_STR(DMLERR_DATAACKTIMEOUT);
    DMLERR_TO_STR(DMLERR_DLL_NOT_INITIALIZED);
    DMLERR_TO_STR(DMLERR_DLL_USAGE);
    DMLERR_TO_STR(DMLERR_EXECACKTIMEOUT);
    DMLERR_TO_STR(DMLERR_INVALIDPARAMETER);
    DMLERR_TO_STR(DMLERR_LOW_MEMORY);
    DMLERR_TO_STR(DMLERR_MEMORY_ERROR);
    DMLERR_TO_STR(DMLERR_NOTPROCESSED);
    DMLERR_TO_STR(DMLERR_NO_CONV_ESTABLISHED);
    DMLERR_TO_STR(DMLERR_POKEACKTIMEOUT);
    DMLERR_TO_STR(DMLERR_POSTMSG_FAILED);
    DMLERR_TO_STR(DMLERR_REENTRANCY);
    DMLERR_TO_STR(DMLERR_SERVER_DIED);
    DMLERR_TO_STR(DMLERR_SYS_ERROR);
    DMLERR_TO_STR(DMLERR_UNADVACKTIMEOUT);
    DMLERR_TO_STR(DMLERR_UNFOUND_QUEUE_ID);
    default:
        retstr = "Unknown DML Error";
        break;
    }

    return retstr;
}

/* Helper Function to Transfer DdeGetLastError into a String */
static const char * GetDdeLastErrorStr(DWORD instance)
{
    UINT err = DdeGetLastError(instance);

    return GetStringFromError(err);
}

/* Execute a Dde Command and return the error & result */
/* Note: Progman DDE always returns a pointer to 0x00000001 on a successful result */
static void DdeExecuteCommand(DWORD instance, HCONV hConv, const char *strCmd, HDDEDATA *hData, UINT *err, int testParams)
{
    HDDEDATA command;

    command = DdeCreateDataHandle(instance, (LPBYTE) strCmd, strlen(strCmd)+1, 0, 0L, 0, 0);
    ok (command != NULL, "DdeCreateDataHandle Error %s.%s\n",
        GetDdeLastErrorStr(instance), GetStringFromTestParams(testParams));
    *hData = DdeClientTransaction((void *) command,
                                  -1,
                                  hConv,
                                  0,
                                  0,
                                  XTYP_EXECUTE,
                                  MS_TIMEOUT_VAL,
                                  NULL);

    /* hData is technically a pointer, but for Program Manager,
     * it is NULL (error) or 1 (success)
     * TODO: Check other versions of Windows to verify 1 is returned.
     * While it is unlikely that anyone is actually testing that the result is 1
     * if all versions of windows return 1, Wine should also.
     */
    if (*hData == NULL)
    {
        *err = DdeGetLastError(instance);
    }
    else
    {
        *err = DMLERR_NO_ERROR;
        todo_wine
        {
            ok(*hData == (HDDEDATA) 1, "Expected HDDEDATA Handle == 1, actually %p.%s\n",
               *hData, GetStringFromTestParams(testParams));
        }
    }
}

/*
 * Check if Window is onscreen with the appropriate name.
 *
 * Windows are not created synchronously.  So we do not know
 * when and if the window will be created/shown on screen.
 * This function implements a polling mechanism to determine
 * creation.
 * A more complicated method would be to use SetWindowsHookEx.
 * Since polling worked fine in my testing, no reason to implement
 * the other.  Comments about other methods of determining when
 * window creation happened were not encouraging (not including
 * SetWindowsHookEx).
 */
static void CheckWindowCreated(const char *winName, int closeWindow, int testParams)
{
    HWND window = NULL;
    int i;

    /* Poll for Window Creation */
    for (i = 0; window == NULL && i < PDDE_POLL_NUM; i++)
    {
        Sleep(PDDE_POLL_TIME);
        window = FindWindowA(NULL, winName);
    }
    ok (window != NULL, "Window \"%s\" was not created in %i seconds - assumed failure.%s\n",
        winName, PDDE_POLL_NUM*PDDE_POLL_TIME/1000, GetStringFromTestParams(testParams));

    /* Close Window as desired. */
    if (window != NULL && closeWindow)
    {
        SendMessageA(window, WM_SYSCOMMAND, SC_CLOSE, 0);
    }
}

/* Check for Existence (or non-existence) of a file or group
 *   When testing for existence of a group, groupName is not needed
 */
static void CheckFileExistsInProgramGroups(const char *nameToCheck, int shouldExist, int isGroup,
                                           const char *groupName, int testParams)
{
    char *path;
    int err;
    DWORD attributes;
    int len;

    if (!pSHGetSpecialFolderPathA)
        return;

    path = HeapAlloc(GetProcessHeap(), 0, MAX_PATH);
    if (path != NULL)
    {
        int specialFolder;

        err = FALSE;
        /* Win 9x doesn't support common */
        if (testParams & DDE_TEST_COMMON)
        {
            specialFolder = CSIDL_COMMON_PROGRAMS;
            err = pSHGetSpecialFolderPathA(NULL, path, specialFolder, FALSE);
            /* Win 9x fails, use CSIDL_PROGRAMS (err == FALSE) */
        }
        if (err == FALSE)
        {
            specialFolder = CSIDL_PROGRAMS;
            err = pSHGetSpecialFolderPathA(NULL, path, specialFolder, FALSE);
        }
        len = strlen(path) + strlen(nameToCheck)+1;
        if (groupName != NULL)
        {
            len += strlen(groupName)+1;
        }
        ok (len <= MAX_PATH, "Path Too Long.%s\n", GetStringFromTestParams(testParams));
        if (len <= MAX_PATH)
        {
            if (groupName != NULL)
            {
                strcat(path, "\\");
                strcat(path, groupName);
            }
            strcat(path, "\\");
            strcat(path, nameToCheck);
            attributes = GetFileAttributes(path);
            if (!shouldExist)
            {
                ok (attributes == INVALID_FILE_ATTRIBUTES , "File exists and shouldn't %s.%s\n",
                    path, GetStringFromTestParams(testParams));
            } else {
                if (attributes == INVALID_FILE_ATTRIBUTES)
                {
                    ok (FALSE, "Created File %s doesn't exist.%s\n", path, GetStringFromTestParams(testParams));
                } else if (isGroup) {
                    ok (attributes & FILE_ATTRIBUTE_DIRECTORY, "%s is not a folder (attr=%x).%s\n",
                        path, attributes, GetStringFromTestParams(testParams));
                } else {
                    ok (attributes & FILE_ATTRIBUTE_ARCHIVE, "Created File %s has wrong attributes (%x).%s\n",
                        path, attributes, GetStringFromTestParams(testParams));
                }
            }
        }
        HeapFree(GetProcessHeap(), 0, path);
    }
    else
    {
        ok (FALSE, "Could not Allocate Path Buffer\n");
    }
}

/* Create Group Test.
 *   command and expected_result.
 *   if expected_result is DMLERR_NO_ERROR, test
 *        1. group was created
 *        2. window is open
 */
static void CreateGroupTest(DWORD instance, HCONV hConv, const char *command, UINT expected_result,
                            const char *groupName, int testParams)
{
    HDDEDATA hData;
    UINT error;

    /* Execute Command & Check Result */
    DdeExecuteCommand(instance, hConv, command, &hData, &error, testParams);
    todo_wine
    {
        ok (expected_result == error, "CreateGroup %s: Expected Error %s, received %s.%s\n",
            groupName, GetStringFromError(expected_result), GetStringFromError(error),
            GetStringFromTestParams(testParams));
    }

    /* No Error */
    if (error == DMLERR_NO_ERROR)
    {

        /* Check if Group Now Exists */
        CheckFileExistsInProgramGroups(groupName, TRUE, TRUE, NULL, testParams);
        /* Check if Window is Open (polling) */
        CheckWindowCreated(groupName, TRUE, testParams);
    }
}

/* Show Group Test.
 *   DDE command, expected_result, and the group name to check for existence
 *   if expected_result is DMLERR_NO_ERROR, test
 *        1. window is open
 */
static void ShowGroupTest(DWORD instance, HCONV hConv, const char *command, UINT expected_result,
                          const char *groupName, int closeAfterShowing, int testParams)
{
    HDDEDATA hData;
    UINT error;

    DdeExecuteCommand(instance, hConv, command, &hData, &error, testParams);
/* todo_wine...  Is expected to fail, wine stubbed functions DO fail */
/* TODO REMOVE THIS CODE!!! */
    if (expected_result == DMLERR_NOTPROCESSED)
    {
        ok (expected_result == error, "ShowGroup %s: Expected Error %s, received %s.%s\n",
            groupName, GetStringFromError(expected_result), GetStringFromError(error),
            GetStringFromTestParams(testParams));
    } else {
        todo_wine
        {
            ok (expected_result == error, "ShowGroup %s: Expected Error %s, received %s.%s\n",
                groupName, GetStringFromError(expected_result), GetStringFromError(error),
                GetStringFromTestParams(testParams));
        }
    }

    if (error == DMLERR_NO_ERROR)
    {
        /* Check if Window is Open (polling) */
        CheckWindowCreated(groupName, closeAfterShowing, testParams);
    }
}

/* Delete Group Test.
 *   DDE command, expected_result, and the group name to check for existence
 *   if expected_result is DMLERR_NO_ERROR, test
 *        1. group does not exist
 */
static void DeleteGroupTest(DWORD instance, HCONV hConv, const char *command, UINT expected_result,
                            const char *groupName, int testParams)
{
    HDDEDATA hData;
    UINT error;

    DdeExecuteCommand(instance, hConv, command, &hData, &error, testParams);
    todo_wine
    {
        ok (expected_result == error, "DeleteGroup %s: Expected Error %s, received %s.%s\n",
            groupName, GetStringFromError(expected_result), GetStringFromError(error),
            GetStringFromTestParams(testParams));
    }

    if (error == DMLERR_NO_ERROR)
    {
        /* Check that Group does not exist */
        CheckFileExistsInProgramGroups(groupName, FALSE, TRUE, NULL, testParams);
    }
}

/* Add Item Test
 *   DDE command, expected result, and group and file name where it should exist.
 *   checks to make sure error code matches expected error code
 *   checks to make sure item exists if successful
 */
static void AddItemTest(DWORD instance, HCONV hConv, const char *command, UINT expected_result,
                        const char *fileName, const char *groupName, int testParams)
{
    HDDEDATA hData;
    UINT error;

    DdeExecuteCommand(instance, hConv, command, &hData, &error, testParams);
    todo_wine
    {
        ok (expected_result == error, "AddItem %s: Expected Error %s, received %s.%s\n",
            fileName, GetStringFromError(expected_result), GetStringFromError(error),
            GetStringFromTestParams(testParams));
    }

    if (error == DMLERR_NO_ERROR)
    {
        /* Check that File exists */
        CheckFileExistsInProgramGroups(fileName, TRUE, FALSE, groupName, testParams);
    }
}

/* Delete Item Test.
 *   DDE command, expected result, and group and file name where it should exist.
 *   checks to make sure error code matches expected error code
 *   checks to make sure item does not exist if successful
 */
static void DeleteItemTest(DWORD instance, HCONV hConv, const char *command, UINT expected_result,
                           const char *fileName, const char *groupName, int testParams)
{
    HDDEDATA hData;
    UINT error;

    DdeExecuteCommand(instance, hConv, command, &hData, &error, testParams);
    todo_wine
    {
        ok (expected_result == error, "DeleteItem %s: Expected Error %s, received %s.%s\n",
            fileName, GetStringFromError(expected_result), GetStringFromError(error),
            GetStringFromTestParams(testParams));
    }

    if (error == DMLERR_NO_ERROR)
    {
        /* Check that File does not exist */
        CheckFileExistsInProgramGroups(fileName, FALSE, FALSE, groupName, testParams);
    }
}

/* Compound Command Test.
 *   not really generic, assumes command of the form:
 *          [CreateGroup ...][AddItem ...][AddItem ...]
 *   All samples I've seen using Compound were of this form (CreateGroup,
 *   AddItems) so this covers minimum expected functionality.
 */
static void CompoundCommandTest(DWORD instance, HCONV hConv, const char *command, UINT expected_result,
                                const char *groupName, const char *fileName1,
                                const char *fileName2, int testParams)
{
    HDDEDATA hData;
    UINT error;

    DdeExecuteCommand(instance, hConv, command, &hData, &error, testParams);
    todo_wine
    {
        ok (expected_result == error, "Compound String %s: Expected Error %s, received %s.%s\n",
            command, GetStringFromError(expected_result), GetStringFromError(error),
            GetStringFromTestParams(testParams));
    }

    if (error == DMLERR_NO_ERROR)
    {
        /* Check that File exists */
        CheckFileExistsInProgramGroups(groupName, TRUE, TRUE, NULL, testParams);
        CheckWindowCreated(groupName, FALSE, testParams);
        CheckFileExistsInProgramGroups(fileName1, TRUE, FALSE, groupName, testParams);
        CheckFileExistsInProgramGroups(fileName2, TRUE, FALSE, groupName, testParams);
    }
}

/* 1st set of tests */
static int DdeTestProgman(DWORD instance, HCONV hConv)
{
    HDDEDATA hData;
    UINT error;
    int testnum;

    testnum = 1;
    /* Invalid Command */
    DdeExecuteCommand(instance, hConv, "[InvalidCommand()]", &hData, &error, DDE_TEST_MISC|testnum++);
    ok (error == DMLERR_NOTPROCESSED, "InvalidCommand(), expected error %s, received %s.\n",
        GetStringFromError(DMLERR_NOTPROCESSED), GetStringFromError(error));

    /* CreateGroup Tests (including AddItem, DeleteItem) */
    CreateGroupTest(instance, hConv, "[CreateGroup(Group1)]", DMLERR_NO_ERROR, "Group1", DDE_TEST_COMMON|DDE_TEST_CREATEGROUP|testnum++);
    AddItemTest(instance, hConv, "[AddItem(c:\\f1g1,f1g1Name)]", DMLERR_NO_ERROR, "f1g1Name.lnk", "Group1", DDE_TEST_COMMON|DDE_TEST_ADDITEM|testnum++);
    AddItemTest(instance, hConv, "[AddItem(c:\\f2g1,f2g1Name)]", DMLERR_NO_ERROR, "f2g1Name.lnk", "Group1", DDE_TEST_COMMON|DDE_TEST_ADDITEM|testnum++);
    DeleteItemTest(instance, hConv, "[DeleteItem(f2g1Name)]", DMLERR_NO_ERROR, "f2g1Name.lnk", "Group1", DDE_TEST_COMMON|DDE_TEST_DELETEITEM|testnum++);
    AddItemTest(instance, hConv, "[AddItem(c:\\f3g1,f3g1Name)]", DMLERR_NO_ERROR, "f3g1Name.lnk", "Group1", DDE_TEST_COMMON|DDE_TEST_ADDITEM|testnum++);
    CreateGroupTest(instance, hConv, "[CreateGroup(Group2)]", DMLERR_NO_ERROR, "Group2", DDE_TEST_COMMON|DDE_TEST_CREATEGROUP|testnum++);
    /* Create Group that already exists - same instance */
    CreateGroupTest(instance, hConv, "[CreateGroup(Group1)]", DMLERR_NO_ERROR, "Group1", DDE_TEST_COMMON|DDE_TEST_CREATEGROUP|testnum++);

    /* ShowGroup Tests */
    ShowGroupTest(instance, hConv, "[ShowGroup(Group1)]", DMLERR_NOTPROCESSED, "Startup", TRUE, DDE_TEST_SHOWGROUP|testnum++);
    DeleteItemTest(instance, hConv, "[DeleteItem(f3g1Name)]", DMLERR_NO_ERROR, "f3g1Name.lnk", "Group1", DDE_TEST_COMMON|DDE_TEST_DELETEITEM|testnum++);
    ShowGroupTest(instance, hConv, "[ShowGroup(Startup,0)]", DMLERR_NO_ERROR, "Startup", TRUE, DDE_TEST_SHOWGROUP|testnum++);
    ShowGroupTest(instance, hConv, "[ShowGroup(Group1,0)]", DMLERR_NO_ERROR, "Group1", FALSE, DDE_TEST_SHOWGROUP|testnum++);

    /* DeleteGroup Test - Note that Window is Open for this test */
    DeleteGroupTest(instance, hConv, "[DeleteGroup(Group1)]", DMLERR_NO_ERROR, "Group1", DDE_TEST_DELETEGROUP|testnum++);

    /* Compound Execute String Command */
    CompoundCommandTest(instance, hConv, "[CreateGroup(Group3)][AddItem(c:\\f1g3,f1g3Name)][AddItem(c:\\f2g3,f2g3Name)]", DMLERR_NO_ERROR, "Group3", "f1g3Name.lnk", "f2g3Name.lnk", DDE_TEST_COMMON|DDE_TEST_COMPOUND|testnum++);
    DeleteGroupTest(instance, hConv, "[DeleteGroup(Group3)]", DMLERR_NO_ERROR, "Group3", DDE_TEST_DELETEGROUP|testnum++);

    /* Full Parameters of Add Item */
    /* AddItem(CmdLine[,Name[,IconPath[,IconIndex[,xPos,yPos[,DefDir[,HotKey[,fMinimize[fSeparateSpace]]]]]]]) */

    return testnum;
}

/* 2nd set of tests - 2nd connection */
static void DdeTestProgman2(DWORD instance, HCONV hConv, int testnum)
{
    /* Create Group that already exists on a separate connection */
    CreateGroupTest(instance, hConv, "[CreateGroup(Group2)]", DMLERR_NO_ERROR, "Group2", DDE_TEST_COMMON|DDE_TEST_CREATEGROUP|testnum++);
    DeleteGroupTest(instance, hConv, "[DeleteGroup(Group2)]", DMLERR_NO_ERROR, "Group2", DDE_TEST_COMMON|DDE_TEST_DELETEGROUP|testnum++);
}

START_TEST(progman_dde)
{
    DWORD instance = 0;
    UINT err;
    HSZ hszProgman;
    HCONV hConv;
    int testnum;

    init_function_pointers();

    /* Only report this once */
    if (!pSHGetSpecialFolderPathA)
        win_skip("SHGetSpecialFolderPathA is not available\n");

    /* Initialize DDE Instance */
    err = DdeInitialize(&instance, DdeCallback, APPCMD_CLIENTONLY, 0);
    ok (err == DMLERR_NO_ERROR, "DdeInitialize Error %s\n", GetStringFromError(err));

    /* Create Connection */
    hszProgman = DdeCreateStringHandle(instance, "PROGMAN", CP_WINANSI);
    ok (hszProgman != NULL, "DdeCreateStringHandle Error %s\n", GetDdeLastErrorStr(instance));
    hConv = DdeConnect(instance, hszProgman, hszProgman, NULL);
    ok (hConv != NULL, "DdeConnect Error %s\n", GetDdeLastErrorStr(instance));
    ok (DdeFreeStringHandle(instance, hszProgman), "DdeFreeStringHandle failure\n");

    /* Run Tests */
    testnum = DdeTestProgman(instance, hConv);

    /* Cleanup & Exit */
    ok (DdeDisconnect(hConv), "DdeDisonnect Error %s\n", GetDdeLastErrorStr(instance));
    ok (DdeUninitialize(instance), "DdeUninitialize failed\n");

    /* 2nd Instance (Followup Tests) */
    /* Initialize DDE Instance */
    instance = 0;
    err = DdeInitialize(&instance, DdeCallback, APPCMD_CLIENTONLY, 0);
    ok (err == DMLERR_NO_ERROR, "DdeInitialize Error %s\n", GetStringFromError(err));

    /* Create Connection */
    hszProgman = DdeCreateStringHandle(instance, "PROGMAN", CP_WINANSI);
    ok (hszProgman != NULL, "DdeCreateStringHandle Error %s\n", GetDdeLastErrorStr(instance));
    hConv = DdeConnect(instance, hszProgman, hszProgman, NULL);
    ok (hConv != NULL, "DdeConnect Error %s\n", GetDdeLastErrorStr(instance));
    ok (DdeFreeStringHandle(instance, hszProgman), "DdeFreeStringHandle failure\n");

    /* Run Tests */
    DdeTestProgman2(instance, hConv, testnum);

    /* Cleanup & Exit */
    ok (DdeDisconnect(hConv), "DdeDisonnect Error %s\n", GetDdeLastErrorStr(instance));
    ok (DdeUninitialize(instance), "DdeUninitialize failed\n");
}
