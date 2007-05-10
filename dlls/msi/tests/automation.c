/*
 * Copyright (C) 2007 Mike McCormack for CodeWeavers
 * Copyright (C) 2007 Misha Koshelev
 *
 * A test program for Microsoft Installer OLE automation functionality.
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

#define COBJMACROS

#include <stdio.h>

#include <windows.h>
#include <msiquery.h>
#include <msidefs.h>
#include <msi.h>
#include <fci.h>

#include "wine/test.h"

static const char *msifile = "winetest.msi";
static const WCHAR szMsifile[] = {'w','i','n','e','t','e','s','t','.','m','s','i',0};
CHAR CURR_DIR[MAX_PATH];
EXCEPINFO excepinfo;

/*
 * OLE automation data
 **/
static const WCHAR szProgId[] = { 'W','i','n','d','o','w','s','I','n','s','t','a','l','l','e','r','.','I','n','s','t','a','l','l','e','r',0 };
static IDispatch *pInstaller;

/* msi database data */

static const CHAR component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                    "s72\tS38\ts72\ti2\tS255\tS72\n"
                                    "Component\tComponent\n"
                                    "Five\t{8CC92E9D-14B2-4CA4-B2AA-B11D02078087}\tNEWDIR\t2\t\tfive.txt\n"
                                    "Four\t{FD37B4EA-7209-45C0-8917-535F35A2F080}\tCABOUTDIR\t2\t\tfour.txt\n"
                                    "One\t{783B242E-E185-4A56-AF86-C09815EC053C}\tMSITESTDIR\t2\t\tone.txt\n"
                                    "Three\t{010B6ADD-B27D-4EDD-9B3D-34C4F7D61684}\tCHANGEDDIR\t2\t\tthree.txt\n"
                                    "Two\t{BF03D1A6-20DA-4A65-82F3-6CAC995915CE}\tFIRSTDIR\t2\t\ttwo.txt\n"
                                    "dangler\t{6091DF25-EF96-45F1-B8E9-A9B1420C7A3C}\tTARGETDIR\t4\t\tregdata\n"
                                    "component\t\tMSITESTDIR\t0\t1\tfile\n"
                                    "service_comp\t\tMSITESTDIR\t0\t1\tservice_file";

static const CHAR directory_dat[] = "Directory\tDirectory_Parent\tDefaultDir\n"
                                    "s72\tS72\tl255\n"
                                    "Directory\tDirectory\n"
                                    "CABOUTDIR\tMSITESTDIR\tcabout\n"
                                    "CHANGEDDIR\tMSITESTDIR\tchanged:second\n"
                                    "FIRSTDIR\tMSITESTDIR\tfirst\n"
                                    "MSITESTDIR\tProgramFilesFolder\tmsitest\n"
                                    "NEWDIR\tCABOUTDIR\tnew\n"
                                    "ProgramFilesFolder\tTARGETDIR\t.\n"
                                    "TARGETDIR\t\tSourceDir";

static const CHAR feature_dat[] = "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
                                  "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
                                  "Feature\tFeature\n"
                                  "Five\t\tFive\tThe Five Feature\t5\t3\tNEWDIR\t0\n"
                                  "Four\t\tFour\tThe Four Feature\t4\t3\tCABOUTDIR\t0\n"
                                  "One\t\tOne\tThe One Feature\t1\t3\tMSITESTDIR\t0\n"
                                  "Three\tOne\tThree\tThe Three Feature\t3\t3\tCHANGEDDIR\t0\n"
                                  "Two\tOne\tTwo\tThe Two Feature\t2\t3\tFIRSTDIR\t0\n"
                                  "feature\t\t\t\t2\t1\tTARGETDIR\t0\n"
                                  "service_feature\t\t\t\t2\t1\tTARGETDIR\t0";

static const CHAR feature_comp_dat[] = "Feature_\tComponent_\n"
                                       "s38\ts72\n"
                                       "FeatureComponents\tFeature_\tComponent_\n"
                                       "Five\tFive\n"
                                       "Four\tFour\n"
                                       "One\tOne\n"
                                       "Three\tThree\n"
                                       "Two\tTwo\n"
                                       "feature\tcomponent\n"
                                       "service_feature\tservice_comp\n";

static const CHAR file_dat[] = "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
                               "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
                               "File\tFile\n"
                               "five.txt\tFive\tfive.txt\t1000\t\t\t16384\t5\n"
                               "four.txt\tFour\tfour.txt\t1000\t\t\t16384\t4\n"
                               "one.txt\tOne\tone.txt\t1000\t\t\t0\t1\n"
                               "three.txt\tThree\tthree.txt\t1000\t\t\t0\t3\n"
                               "two.txt\tTwo\ttwo.txt\t1000\t\t\t0\t2\n"
                               "file\tcomponent\tfilename\t100\t\t\t8192\t1\n"
                               "service_file\tservice_comp\tservice.exe\t100\t\t\t8192\t1";

static const CHAR install_exec_seq_dat[] = "Action\tCondition\tSequence\n"
                                           "s72\tS255\tI2\n"
                                           "InstallExecuteSequence\tAction\n"
                                           "AllocateRegistrySpace\tNOT Installed\t1550\n"
                                           "CostFinalize\t\t1000\n"
                                           "CostInitialize\t\t800\n"
                                           "FileCost\t\t900\n"
                                           "InstallFiles\t\t4000\n"
                                           "InstallServices\t\t5000\n"
                                           "InstallFinalize\t\t6600\n"
                                           "InstallInitialize\t\t1500\n"
                                           "InstallValidate\t\t1400\n"
                                           "LaunchConditions\t\t100\n"
                                           "WriteRegistryValues\tSourceDir And SOURCEDIR\t5000";

static const CHAR media_dat[] = "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
                                "i2\ti4\tL64\tS255\tS32\tS72\n"
                                "Media\tDiskId\n"
                                "1\t3\t\t\tDISK1\t\n"
                                "2\t5\t\tmsitest.cab\tDISK2\t\n";

static const CHAR property_dat[] = "Property\tValue\n"
                                   "s72\tl0\n"
                                   "Property\tProperty\n"
                                   "DefaultUIFont\tDlgFont8\n"
                                   "HASUIRUN\t0\n"
                                   "INSTALLLEVEL\t3\n"
                                   "InstallMode\tTypical\n"
                                   "Manufacturer\tWine\n"
                                   "PIDTemplate\t12345<###-%%%%%%%>@@@@@\n"
                                   "ProductCode\t{F1C3AF50-8B56-4A69-A00C-00773FE42F30}\n"
                                   "ProductID\tnone\n"
                                   "ProductLanguage\t1033\n"
                                   "ProductName\tMSITEST\n"
                                   "ProductVersion\t1.1.1\n"
                                   "PROMPTROLLBACKCOST\tP\n"
                                   "Setup\tSetup\n"
                                   "UpgradeCode\t{CE067E8D-2E1A-4367-B734-4EB2BDAD6565}";

static const CHAR registry_dat[] = "Registry\tRoot\tKey\tName\tValue\tComponent_\n"
                                   "s72\ti2\tl255\tL255\tL0\ts72\n"
                                   "Registry\tRegistry\n"
                                   "Apples\t2\tSOFTWARE\\Wine\\msitest\tName\timaname\tOne\n"
                                   "Oranges\t2\tSOFTWARE\\Wine\\msitest\tnumber\t#314\tTwo\n"
                                   "regdata\t2\tSOFTWARE\\Wine\\msitest\tblah\tbad\tdangler\n"
                                   "OrderTest\t2\tSOFTWARE\\Wine\\msitest\tOrderTestName\tOrderTestValue\tcomponent";

static const CHAR service_install_dat[] = "ServiceInstall\tName\tDisplayName\tServiceType\tStartType\tErrorControl\t"
                                          "LoadOrderGroup\tDependencies\tStartName\tPassword\tArguments\tComponent_\tDescription\n"
                                          "s72\ts255\tL255\ti4\ti4\ti4\tS255\tS255\tS255\tS255\tS255\ts72\tL255\n"
                                          "ServiceInstall\tServiceInstall\n"
                                          "TestService\tTestService\tTestService\t2\t3\t0\t\t\tTestService\t\t\tservice_comp\t\t";

static const CHAR service_control_dat[] = "ServiceControl\tName\tEvent\tArguments\tWait\tComponent_\n"
                                          "s72\tl255\ti2\tL255\tI2\ts72\n"
                                          "ServiceControl\tServiceControl\n"
                                          "ServiceControl\tTestService\t8\t\t0\tservice_comp";

typedef struct _msi_table
{
    const CHAR *filename;
    const CHAR *data;
    int size;
} msi_table;

#define ADD_TABLE(x) {#x".idt", x##_dat, sizeof(x##_dat)}

static const msi_table tables[] =
{
    ADD_TABLE(component),
    ADD_TABLE(directory),
    ADD_TABLE(feature),
    ADD_TABLE(feature_comp),
    ADD_TABLE(file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(media),
    ADD_TABLE(property),
    ADD_TABLE(registry),
    ADD_TABLE(service_install),
    ADD_TABLE(service_control)
};

/*
 * Database Helpers
 */

static void write_file(const CHAR *filename, const char *data, int data_size)
{
    DWORD size;

    HANDLE hf = CreateFile(filename, GENERIC_WRITE, 0, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    WriteFile(hf, data, data_size, &size, NULL);
    CloseHandle(hf);
}

static void write_msi_summary_info(MSIHANDLE db)
{
    MSIHANDLE summary;
    UINT r;

    r = MsiGetSummaryInformationA(db, NULL, 4, &summary);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSummaryInfoSetPropertyA(summary, PID_TEMPLATE, VT_LPSTR, 0, NULL, ";1033");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSummaryInfoSetPropertyA(summary, PID_REVNUMBER, VT_LPSTR, 0, NULL,
                                   "{004757CA-5092-49c2-AD20-28E1CE0DF5F2}");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSummaryInfoSetPropertyA(summary, PID_PAGECOUNT, VT_I4, 100, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSummaryInfoSetPropertyA(summary, PID_WORDCOUNT, VT_I4, 0, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    /* write the summary changes back to the stream */
    r = MsiSummaryInfoPersist(summary);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    MsiCloseHandle(summary);
}

static void create_database(const CHAR *name, const msi_table *tables, int num_tables)
{
    MSIHANDLE db;
    UINT r;
    int j;

    r = MsiOpenDatabaseA(name, MSIDBOPEN_CREATE, &db);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    /* import the tables into the database */
    for (j = 0; j < num_tables; j++)
    {
        const msi_table *table = &tables[j];

        write_file(table->filename, table->data, (table->size - 1) * sizeof(char));

        r = MsiDatabaseImportA(db, CURR_DIR, table->filename);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

        DeleteFileA(table->filename);
    }

    write_msi_summary_info(db);

    r = MsiDatabaseCommit(db);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    MsiCloseHandle(db);
}

/*
 * Automation helpers and tests
 */

/* ok-like statement which takes two unicode strings as arguments */
static CHAR string1[MAX_PATH], string2[MAX_PATH];

#define ok_w2(format, szString1, szString2) \
\
    if (lstrcmpW(szString1, szString2) != 0) \
    { \
        WideCharToMultiByte(CP_ACP, 0, szString1, -1, string1, MAX_PATH, NULL, NULL); \
        WideCharToMultiByte(CP_ACP, 0, szString2, -1, string2, MAX_PATH, NULL, NULL); \
        ok(0, format, string1, string2); \
    }

/* exception checker */
static WCHAR szSource[] = {'M','s','i',' ','A','P','I',' ','E','r','r','o','r',0};

#define ok_exception(hr, szDescription)           \
    if (hr == DISP_E_EXCEPTION) \
    { \
        /* Compare wtype, source, and destination */                    \
        ok(excepinfo.wCode == 1000, "Exception info was %d, expected 1000\n", excepinfo.wCode); \
\
        ok(excepinfo.bstrSource != NULL, "Exception source was NULL\n"); \
        if (excepinfo.bstrSource)                                       \
            ok_w2("Exception source was \"%s\" but expected to be \"%s\"\n", excepinfo.bstrSource, szSource); \
\
        ok(excepinfo.bstrDescription != NULL, "Exception description was NULL\n"); \
        if (excepinfo.bstrDescription) \
            ok_w2("Exception description was \"%s\" but expected to be \"%s\"\n", excepinfo.bstrDescription, szDescription); \
    }

static DISPID get_dispid( IDispatch *disp, const char *name )
{
    LPOLESTR str;
    UINT len;
    DISPID id = -1;
    HRESULT r;

    len = MultiByteToWideChar(CP_ACP, 0, name, -1, NULL, 0 );
    str = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR) );
    if (str)
    {
        len = MultiByteToWideChar(CP_ACP, 0, name, -1, str, len );
        r = IDispatch_GetIDsOfNames( disp, &IID_NULL, &str, 1, 0, &id );
        HeapFree(GetProcessHeap(), 0, str);
        if (r != S_OK)
            return -1;
    }

    return id;
}

static void test_dispid(void)
{
    todo_wine {
    ok( get_dispid( pInstaller, "CreateRecord" ) == 1, "dispid wrong\n");
    }
    ok( get_dispid( pInstaller, "OpenPackage" ) == 2, "dispid wrong\n");
    todo_wine {
    ok( get_dispid( pInstaller, "OpenProduct" ) == 3, "dispid wrong\n");
    ok( get_dispid( pInstaller, "OpenDatabase" ) == 4, "dispid wrong\n");
    ok( get_dispid( pInstaller, "SummaryInformation" ) == 5, "dispid wrong\n");
    ok( get_dispid( pInstaller, "UILevel" ) == 6, "dispid wrong\n");
    ok( get_dispid( pInstaller, "EnableLog" ) == 7, "dispid wrong\n");
    ok( get_dispid( pInstaller, "InstallProduct" ) == 8, "dispid wrong\n");
    ok( get_dispid( pInstaller, "Version" ) == 9, "dispid wrong\n");
    ok( get_dispid( pInstaller, "LastErrorRecord" ) == 10, "dispid wrong\n");
    ok( get_dispid( pInstaller, "RegistryValue" ) == 11, "dispid wrong\n");
    ok( get_dispid( pInstaller, "Environment" ) == 12, "dispid wrong\n");
    ok( get_dispid( pInstaller, "FileAttributes" ) == 13, "dispid wrong\n");

    ok( get_dispid( pInstaller, "FileSize" ) == 15, "dispid wrong\n");
    ok( get_dispid( pInstaller, "FileVersion" ) == 16, "dispid wrong\n");
    }
    ok( get_dispid( pInstaller, "ProductState" ) == 17, "dispid wrong\n");
    todo_wine {
    ok( get_dispid( pInstaller, "ProductInfo" ) == 18, "dispid wrong\n");
    ok( get_dispid( pInstaller, "ConfigureProduct" ) == 19, "dispid wrong\n");
    ok( get_dispid( pInstaller, "ReinstallProduct" ) == 20 , "dispid wrong\n");
    ok( get_dispid( pInstaller, "CollectUserInfo" ) == 21, "dispid wrong\n");
    ok( get_dispid( pInstaller, "ApplyPatch" ) == 22, "dispid wrong\n");
    ok( get_dispid( pInstaller, "FeatureParent" ) == 23, "dispid wrong\n");
    ok( get_dispid( pInstaller, "FeatureState" ) == 24, "dispid wrong\n");
    ok( get_dispid( pInstaller, "UseFeature" ) == 25, "dispid wrong\n");
    ok( get_dispid( pInstaller, "FeatureUsageCount" ) == 26, "dispid wrong\n");
    ok( get_dispid( pInstaller, "FeatureUsageDate" ) == 27, "dispid wrong\n");
    ok( get_dispid( pInstaller, "ConfigureFeature" ) == 28, "dispid wrong\n");
    ok( get_dispid( pInstaller, "ReinstallFeature" ) == 29, "dispid wrong\n");
    ok( get_dispid( pInstaller, "ProvideComponent" ) == 30, "dispid wrong\n");
    ok( get_dispid( pInstaller, "ComponentPath" ) == 31, "dispid wrong\n");
    ok( get_dispid( pInstaller, "ProvideQualifiedComponent" ) == 32, "dispid wrong\n");
    ok( get_dispid( pInstaller, "QualifierDescription" ) == 33, "dispid wrong\n");
    ok( get_dispid( pInstaller, "ComponentQualifiers" ) == 34, "dispid wrong\n");
    }
    ok( get_dispid( pInstaller, "Products" ) == 35, "dispid wrong\n");
    todo_wine {
    ok( get_dispid( pInstaller, "Features" ) == 36, "dispid wrong\n");
    ok( get_dispid( pInstaller, "Components" ) == 37, "dispid wrong\n");
    ok( get_dispid( pInstaller, "ComponentClients" ) == 38, "dispid wrong\n");
    ok( get_dispid( pInstaller, "Patches" ) == 39, "dispid wrong\n");
    ok( get_dispid( pInstaller, "RelatedProducts" ) == 40, "dispid wrong\n");
    ok( get_dispid( pInstaller, "PatchInfo" ) == 41, "dispid wrong\n");
    ok( get_dispid( pInstaller, "PatchTransforms" ) == 42, "dispid wrong\n");
    ok( get_dispid( pInstaller, "AddSource" ) == 43, "dispid wrong\n");
    ok( get_dispid( pInstaller, "ClearSourceList" ) == 44, "dispid wrong\n");
    ok( get_dispid( pInstaller, "ForceSourceListResolution" ) == 45, "dispid wrong\n");
    ok( get_dispid( pInstaller, "ShortcutTarget" ) == 46, "dispid wrong\n");
    ok( get_dispid( pInstaller, "FileHash" ) == 47, "dispid wrong\n");
    ok( get_dispid( pInstaller, "FileSignatureInfo" ) == 48, "dispid wrong\n");
    ok( get_dispid( pInstaller, "RemovePatches" ) == 49, "dispid wrong\n");

    ok( get_dispid( pInstaller, "ApplyMultiplePatches" ) == 51, "dispid wrong\n");
    ok( get_dispid( pInstaller, "ProductsEx" ) ==  52, "dispid wrong\n");

    ok( get_dispid( pInstaller, "PatchesEx" ) == 55, "dispid wrong\n");

    ok( get_dispid( pInstaller, "ExtractPatchXMLData" ) == 57, "dispid wrong\n");
    }

    /* MSDN claims the following functions exist but IDispatch->GetIDsOfNames disagrees */
    if (0)
    {
        get_dispid( pInstaller, "ProductElevated" );
        get_dispid( pInstaller, "ProductInfoFromScript" );
        get_dispid( pInstaller, "ProvideAssembly" );
        get_dispid( pInstaller, "CreateAdvertiseScript" );
        get_dispid( pInstaller, "AdvertiseProduct" );
        get_dispid( pInstaller, "PatchFiles" );
    }
}

/* Test basic IDispatch functions */
static void test_dispatch(void)
{
    static WCHAR szOpenPackage[] = { 'O','p','e','n','P','a','c','k','a','g','e',0 };
    static WCHAR szOpenPackageException[] = {'O','p','e','n','P','a','c','k','a','g','e',',','P','a','c','k','a','g','e','P','a','t','h',',','O','p','t','i','o','n','s',0};
    HRESULT hr;
    DISPID dispid;
    OLECHAR *name;
    VARIANT varresult;
    VARIANTARG vararg[2];
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};

    /* Test getting ID of a function name that does not exist */
    name = (WCHAR *)szMsifile;
    hr = IDispatch_GetIDsOfNames(pInstaller, &IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &dispid);
    ok(hr == DISP_E_UNKNOWNNAME, "IDispatch::GetIDsOfNames returned 0x%08x\n", hr);

    /* Test invoking this function */
    hr = IDispatch_Invoke(pInstaller, dispid, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, NULL, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "IDispatch::Invoke returned 0x%08x\n", hr);

    /* Test getting ID of a function name that does exist */
    name = (WCHAR *)szOpenPackage;
    hr = IDispatch_GetIDsOfNames(pInstaller, &IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &dispid);
    ok(SUCCEEDED(hr), "IDispatch::GetIDsOfNames returned 0x%08x\n", hr);

    /* Test invoking this function (without parameters passed) */
    if (0) /* All of these crash MSI on Windows XP */
    {
        hr = IDispatch_Invoke(pInstaller, dispid, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, NULL, NULL, NULL, NULL);
        hr = IDispatch_Invoke(pInstaller, dispid, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, NULL, NULL, &excepinfo, NULL);
        VariantInit(&varresult);
        hr = IDispatch_Invoke(pInstaller, dispid, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, NULL, &varresult, &excepinfo, NULL);
    }

    /* Try with NULL params */
    hr = IDispatch_Invoke(pInstaller, dispid, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, &dispparams, &varresult, &excepinfo, NULL);
    todo_wine ok(hr == DISP_E_TYPEMISMATCH, "IDispatch::Invoke returned 0x%08x\n", hr);

    /* Try one empty parameter */
    dispparams.rgvarg = vararg;
    dispparams.cArgs = 1;
    VariantInit(&vararg[0]);
    hr = IDispatch_Invoke(pInstaller, dispid, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, &dispparams, &varresult, &excepinfo, NULL);
    todo_wine ok(hr == DISP_E_TYPEMISMATCH, "IDispatch::Invoke returned 0x%08x\n", hr);

    /* Try one parameter, function requires two */
    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BSTR;
    V_BSTR(&vararg[0]) = SysAllocString(szMsifile);
    hr = IDispatch_Invoke(pInstaller, dispid, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, &dispparams, &varresult, &excepinfo, NULL);
    VariantClear(&vararg[0]);

    ok(hr == DISP_E_EXCEPTION, "IDispatch::Invoke returned 0x%08x\n", hr);
    ok_exception(hr, szOpenPackageException);
}

/* invocation helper function */
static HRESULT invoke(IDispatch *pDispatch, LPCSTR szName, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, VARTYPE vtResult)
{
    OLECHAR *name = NULL;
    DISPID dispid;
    HRESULT hr;
    int i;
    UINT len;

    memset(pVarResult, 0, sizeof(VARIANT));
    VariantInit(pVarResult);

    len = MultiByteToWideChar(CP_ACP, 0, szName, -1, NULL, 0 );
    name = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR) );
    if (!name) return E_FAIL;
    len = MultiByteToWideChar(CP_ACP, 0, szName, -1, name, len );
    hr = IDispatch_GetIDsOfNames(pDispatch, &IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &dispid);
    HeapFree(GetProcessHeap(), 0, name);
    ok(SUCCEEDED(hr), "IDispatch::GetIDsOfNames returned 0x%08x\n", hr);
    if (!SUCCEEDED(hr)) return hr;

    memset(&excepinfo, 0, sizeof(excepinfo));
    hr = IDispatch_Invoke(pDispatch, dispid, &IID_NULL, LOCALE_NEUTRAL, wFlags, pDispParams, pVarResult, &excepinfo, NULL);

    if (SUCCEEDED(hr))
    {
        ok(V_VT(pVarResult) == vtResult, "Variant result type is %d, expected %d\n", V_VT(pVarResult), vtResult);
        if (vtResult != VT_EMPTY)
        {
            hr = VariantChangeTypeEx(pVarResult, pVarResult, LOCALE_NEUTRAL, 0, vtResult);
            ok(SUCCEEDED(hr), "VariantChangeTypeEx returned 0x%08x\n", hr);
        }
    }

    for (i=0; i<pDispParams->cArgs; i++)
        VariantClear(&pDispParams->rgvarg[i]);

    return hr;
}

/* Object_Property helper functions */

static HRESULT Installer_CreateRecord(int count, IDispatch **pRecord)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_I4;
    V_I4(&vararg[0]) = count;

    hr = invoke(pInstaller, "CreateRecord", DISPATCH_METHOD, &dispparams, &varresult, VT_DISPATCH);
    *pRecord = V_DISPATCH(&varresult);
    return hr;
}

static HRESULT Installer_RegistryValue(HKEY hkey, LPCWSTR szKey, VARIANT vValue, VARIANT *pVarResult, VARTYPE vtExpect)
{
    VARIANTARG vararg[3];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};

    VariantInit(&vararg[2]);
    V_VT(&vararg[2]) = VT_I4;
    V_I4(&vararg[2]) = (int)hkey;
    VariantInit(&vararg[1]);
    V_VT(&vararg[1]) = VT_BSTR;
    V_BSTR(&vararg[1]) = SysAllocString(szKey);
    VariantInit(&vararg[0]);
    VariantCopy(&vararg[0], &vValue);
    VariantClear(&vValue);

    return invoke(pInstaller, "RegistryValue", DISPATCH_METHOD, &dispparams, pVarResult, vtExpect);
}

static HRESULT Installer_RegistryValueE(HKEY hkey, LPCWSTR szKey, BOOL *pBool)
{
    VARIANT varresult;
    VARIANTARG vararg;
    HRESULT hr;

    VariantInit(&vararg);
    V_VT(&vararg) = VT_EMPTY;
    hr = Installer_RegistryValue(hkey, szKey, vararg, &varresult, VT_BOOL);
    *pBool = V_BOOL(&varresult);
    VariantClear(&varresult);
    return hr;
}

static HRESULT Installer_RegistryValueW(HKEY hkey, LPCWSTR szKey, LPCWSTR szValue, LPWSTR szString)
{
    VARIANT varresult;
    VARIANTARG vararg;
    HRESULT hr;

    VariantInit(&vararg);
    V_VT(&vararg) = VT_BSTR;
    V_BSTR(&vararg) = SysAllocString(szValue);

    hr = Installer_RegistryValue(hkey, szKey, vararg, &varresult, VT_BSTR);
    lstrcpyW(szString, V_BSTR(&varresult));
    VariantClear(&varresult);
    return hr;
}

static HRESULT Installer_RegistryValueI(HKEY hkey, LPCWSTR szKey, int iValue, LPWSTR szString, VARTYPE vtResult)
{
    VARIANT varresult;
    VARIANTARG vararg;
    HRESULT hr;

    VariantInit(&vararg);
    V_VT(&vararg) = VT_I4;
    V_I4(&vararg) = iValue;

    hr = Installer_RegistryValue(hkey, szKey, vararg, &varresult, vtResult);
    if (vtResult == VT_BSTR) lstrcpyW(szString, V_BSTR(&varresult));
    VariantClear(&varresult);
    return hr;
}

static HRESULT Installer_OpenPackage(LPCWSTR szPackagePath, int options, IDispatch **pSession)
{
    VARIANT varresult;
    VARIANTARG vararg[2];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[1]);
    V_VT(&vararg[1]) = VT_BSTR;
    V_BSTR(&vararg[1]) = SysAllocString(szPackagePath);
    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_I4;
    V_I4(&vararg[0]) = options;

    hr = invoke(pInstaller, "OpenPackage", DISPATCH_METHOD, &dispparams, &varresult, VT_DISPATCH);
    *pSession = V_DISPATCH(&varresult);
    return hr;
}

static HRESULT Installer_ProductState(LPCWSTR szProduct, int *pInstallState)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BSTR;
    V_BSTR(&vararg[0]) = SysAllocString(szProduct);

    hr = invoke(pInstaller, "ProductState", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_I4);
    *pInstallState = V_I4(&varresult);
    VariantClear(&varresult);
    return hr;
}

static HRESULT Installer_Products(IDispatch **pStringList)
{
    VARIANT varresult;
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    HRESULT hr;

    hr = invoke(pInstaller, "Products", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_DISPATCH);
    *pStringList = V_DISPATCH(&varresult);
    return hr;
}

static HRESULT Installer_VersionGet(LPCWSTR szVersion)
{
    VARIANT varresult;
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    HRESULT hr;

    hr = invoke(pInstaller, "Version", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_BSTR);
    lstrcpyW((WCHAR *)szVersion, V_BSTR(&varresult));
    VariantClear(&varresult);
    return hr;
}

static HRESULT Session_Installer(IDispatch *pSession, IDispatch **pInst)
{
    VARIANT varresult;
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    HRESULT hr;

    hr = invoke(pSession, "Installer", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_DISPATCH);
    *pInst = V_DISPATCH(&varresult);
    return hr;
}

static HRESULT Session_PropertyGet(IDispatch *pSession, LPCWSTR szName, LPCWSTR szReturn)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BSTR;
    V_BSTR(&vararg[0]) = SysAllocString(szName);

    hr = invoke(pSession, "Property", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_BSTR);
    lstrcpyW((WCHAR *)szReturn, V_BSTR(&varresult));
    VariantClear(&varresult);
    return hr;
}

static HRESULT Session_PropertyPut(IDispatch *pSession, LPCWSTR szName, LPCWSTR szValue)
{
    VARIANT varresult;
    VARIANTARG vararg[2];
    DISPID dispid = DISPID_PROPERTYPUT;
    DISPPARAMS dispparams = {vararg, &dispid, sizeof(vararg)/sizeof(VARIANTARG), 1};

    VariantInit(&vararg[1]);
    V_VT(&vararg[1]) = VT_BSTR;
    V_BSTR(&vararg[1]) = SysAllocString(szName);
    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BSTR;
    V_BSTR(&vararg[0]) = SysAllocString(szValue);

    return invoke(pSession, "Property", DISPATCH_PROPERTYPUT, &dispparams, &varresult, VT_EMPTY);
}

static HRESULT Session_LanguageGet(IDispatch *pSession, UINT *pLangId)
{
    VARIANT varresult;
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    HRESULT hr;

    hr = invoke(pSession, "Language", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_I4);
    *pLangId = V_I4(&varresult);
    VariantClear(&varresult);
    return hr;
}

static HRESULT Session_ModeGet(IDispatch *pSession, int iFlag, BOOL *pMode)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_I4;
    V_I4(&vararg[0]) = iFlag;

    hr = invoke(pSession, "Mode", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_BOOL);
    *pMode = V_BOOL(&varresult);
    VariantClear(&varresult);
    return hr;
}

static HRESULT Session_ModePut(IDispatch *pSession, int iFlag, BOOL bMode)
{
    VARIANT varresult;
    VARIANTARG vararg[2];
    DISPID dispid = DISPID_PROPERTYPUT;
    DISPPARAMS dispparams = {vararg, &dispid, sizeof(vararg)/sizeof(VARIANTARG), 1};

    VariantInit(&vararg[1]);
    V_VT(&vararg[1]) = VT_I4;
    V_I4(&vararg[1]) = iFlag;
    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BOOL;
    V_BOOL(&vararg[0]) = bMode;

    return invoke(pSession, "Mode", DISPATCH_PROPERTYPUT, &dispparams, &varresult, VT_EMPTY);
}

static HRESULT Session_Database(IDispatch *pSession, IDispatch **pDatabase)
{
    VARIANT varresult;
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    HRESULT hr;

    hr = invoke(pSession, "Database", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_DISPATCH);
    *pDatabase = V_DISPATCH(&varresult);
    return hr;
}

static HRESULT Session_DoAction(IDispatch *pSession, LPCWSTR szAction, int *iReturn)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BSTR;
    V_BSTR(&vararg[0]) = SysAllocString(szAction);

    hr = invoke(pSession, "DoAction", DISPATCH_METHOD, &dispparams, &varresult, VT_I4);
    *iReturn = V_I4(&varresult);
    VariantClear(&varresult);
    return hr;
}

static HRESULT Session_SetInstallLevel(IDispatch *pSession, long iInstallLevel)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_I4;
    V_I4(&vararg[0]) = iInstallLevel;

    return invoke(pSession, "SetInstallLevel", DISPATCH_METHOD, &dispparams, &varresult, VT_EMPTY);
}

static HRESULT Session_FeatureCurrentState(IDispatch *pSession, LPCWSTR szName, int *pState)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BSTR;
    V_BSTR(&vararg[0]) = SysAllocString(szName);

    hr = invoke(pSession, "FeatureCurrentState", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_I4);
    *pState = V_I4(&varresult);
    VariantClear(&varresult);
    return hr;
}

static HRESULT Session_FeatureRequestStateGet(IDispatch *pSession, LPCWSTR szName, int *pState)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BSTR;
    V_BSTR(&vararg[0]) = SysAllocString(szName);

    hr = invoke(pSession, "FeatureRequestState", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_I4);
    *pState = V_I4(&varresult);
    VariantClear(&varresult);
    return hr;
}

static HRESULT Session_FeatureRequestStatePut(IDispatch *pSession, LPCWSTR szName, int iState)
{
    VARIANT varresult;
    VARIANTARG vararg[2];
    DISPID dispid = DISPID_PROPERTYPUT;
    DISPPARAMS dispparams = {vararg, &dispid, sizeof(vararg)/sizeof(VARIANTARG), 1};

    VariantInit(&vararg[1]);
    V_VT(&vararg[1]) = VT_BSTR;
    V_BSTR(&vararg[1]) = SysAllocString(szName);
    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_I4;
    V_I4(&vararg[0]) = iState;

    return invoke(pSession, "FeatureRequestState", DISPATCH_PROPERTYPUT, &dispparams, &varresult, VT_EMPTY);
}

static HRESULT Database_OpenView(IDispatch *pDatabase, LPCWSTR szSql, IDispatch **pView)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BSTR;
    V_BSTR(&vararg[0]) = SysAllocString(szSql);

    hr = invoke(pDatabase, "OpenView", DISPATCH_METHOD, &dispparams, &varresult, VT_DISPATCH);
    *pView = V_DISPATCH(&varresult);
    return hr;
}

static HRESULT View_Execute(IDispatch *pView, IDispatch *pRecord)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_DISPATCH;
    V_DISPATCH(&vararg[0]) = pRecord;

    return invoke(pView, "Execute", DISPATCH_METHOD, &dispparams, &varresult, VT_EMPTY);
}

static HRESULT View_Fetch(IDispatch *pView, IDispatch **ppRecord)
{
    VARIANT varresult;
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    HRESULT hr = invoke(pView, "Fetch", DISPATCH_METHOD, &dispparams, &varresult, VT_DISPATCH);
    *ppRecord = V_DISPATCH(&varresult);
    return hr;
}

static HRESULT View_Close(IDispatch *pView)
{
    VARIANT varresult;
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    return invoke(pView, "Close", DISPATCH_METHOD, &dispparams, &varresult, VT_EMPTY);
}

static HRESULT Record_FieldCountGet(IDispatch *pRecord, int *pFieldCount)
{
    VARIANT varresult;
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    HRESULT hr = invoke(pRecord, "FieldCount", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_I4);
    *pFieldCount = V_I4(&varresult);
    VariantClear(&varresult);
    return hr;
}

static HRESULT Record_StringDataGet(IDispatch *pRecord, int iField, LPCWSTR szString)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_I4;
    V_I4(&vararg[0]) = iField;

    hr = invoke(pRecord, "StringData", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_BSTR);
    lstrcpyW((WCHAR *)szString, V_BSTR(&varresult));
    VariantClear(&varresult);
    return hr;
}

static HRESULT Record_StringDataPut(IDispatch *pRecord, int iField, LPCWSTR szString)
{
    VARIANT varresult;
    VARIANTARG vararg[2];
    DISPID dispid = DISPID_PROPERTYPUT;
    DISPPARAMS dispparams = {vararg, &dispid, sizeof(vararg)/sizeof(VARIANTARG), 1};

    VariantInit(&vararg[1]);
    V_VT(&vararg[1]) = VT_I4;
    V_I4(&vararg[1]) = iField;
    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BSTR;
    V_BSTR(&vararg[0]) = SysAllocString(szString);

    return invoke(pRecord, "StringData", DISPATCH_PROPERTYPUT, &dispparams, &varresult, VT_BSTR);
}

static HRESULT StringList_Item(IDispatch *pStringList, int iIndex, LPWSTR szString)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_I4;
    V_I4(&vararg[0]) = iIndex;

    hr = invoke(pStringList, "Item", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_BSTR);
    lstrcpyW(szString, V_BSTR(&varresult));
    VariantClear(&varresult);
    return hr;
}

static HRESULT StringList_Count(IDispatch *pStringList, int *pCount)
{
    VARIANT varresult;
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    HRESULT hr = invoke(pStringList, "Count", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_I4);
    *pCount = V_I4(&varresult);
    VariantClear(&varresult);
    return hr;
}

/* Test the various objects */

static void test_Database(IDispatch *pDatabase)
{
    static WCHAR szSql[] = { 'S','E','L','E','C','T',' ','`','F','e','a','t','u','r','e','`',' ','F','R','O','M',' ','`','F','e','a','t','u','r','e','`',' ','W','H','E','R','E',' ','`','F','e','a','t','u','r','e','_','P','a','r','e','n','t','`','=','\'','O','n','e','\'',0 };
    static WCHAR szThree[] = { 'T','h','r','e','e',0 };
    static WCHAR szTwo[] = { 'T','w','o',0 };
    static WCHAR szStringDataField[] = { 'S','t','r','i','n','g','D','a','t','a',',','F','i','e','l','d',0 };
    IDispatch *pView = NULL;
    HRESULT hr;

    hr = Database_OpenView(pDatabase, szSql, &pView);
    ok(SUCCEEDED(hr), "Database_OpenView failed, hresult 0x%08x\n", hr);
    if (SUCCEEDED(hr))
    {
        IDispatch *pRecord = NULL;
        WCHAR szString[MAX_PATH];

        /* View::Execute */
        hr = View_Execute(pView, NULL);
        ok(SUCCEEDED(hr), "View_Execute failed, hresult 0x%08x\n", hr);

        /* View::Fetch */
        hr = View_Fetch(pView, &pRecord);
        ok(SUCCEEDED(hr), "View_Fetch failed, hresult 0x%08x\n", hr);
        ok(pRecord != NULL, "View_Fetch should not have returned NULL record\n");
        if (pRecord)
        {
            /* Record::StringDataGet */
            memset(szString, 0, sizeof(szString));
            hr = Record_StringDataGet(pRecord, 1, szString);
            ok(SUCCEEDED(hr), "Record_StringDataGet failed, hresult 0x%08x\n", hr);
            ok_w2("Record_StringDataGet result was %s but expected %s\n", szString, szThree);

            /* Record::StringDataPut with incorrect index */
            hr = Record_StringDataPut(pRecord, -1, szString);
            ok(hr == DISP_E_EXCEPTION, "Record_StringDataPut failed, hresult 0x%08x\n", hr);
            ok_exception(hr, szStringDataField);

            IDispatch_Release(pRecord);
        }

        /* View::Fetch */
        hr = View_Fetch(pView, &pRecord);
        ok(SUCCEEDED(hr), "View_Fetch failed, hresult 0x%08x\n", hr);
        ok(pRecord != NULL, "View_Fetch should not have returned NULL record\n");
        if (pRecord)
        {
            /* Record::StringDataGet */
            memset(szString, 0, sizeof(szString));
            hr = Record_StringDataGet(pRecord, 1, szString);
            ok(SUCCEEDED(hr), "Record_StringDataGet failed, hresult 0x%08x\n", hr);
            ok_w2("Record_StringDataGet result was %s but expected %s\n", szString, szTwo);

            IDispatch_Release(pRecord);
        }

        /* View::Fetch */
        hr = View_Fetch(pView, &pRecord);
        ok(SUCCEEDED(hr), "View_Fetch failed, hresult 0x%08x\n", hr);
        ok(pRecord == NULL, "View_Fetch should have returned NULL record\n");
        if (pRecord)
            IDispatch_Release(pRecord);

        /* View::Close */
        hr = View_Close(pView);
        ok(SUCCEEDED(hr), "View_Close failed, hresult 0x%08x\n", hr);

        IDispatch_Release(pView);
    }
}

static void test_Session(IDispatch *pSession)
{
    static WCHAR szProductName[] = { 'P','r','o','d','u','c','t','N','a','m','e',0 };
    static WCHAR szMSITEST[] = { 'M','S','I','T','E','S','T',0 };
    static WCHAR szOne[] = { 'O','n','e',0 };
    static WCHAR szCostInitialize[] = { 'C','o','s','t','I','n','i','t','i','a','l','i','z','e',0 };
    static WCHAR szEmpty[] = { 0 };
    static WCHAR szEquals[] = { '=',0 };
    static WCHAR szPropertyName[] = { 'P','r','o','p','e','r','t','y',',','N','a','m','e',0 };
    WCHAR stringw[MAX_PATH];
    CHAR string[MAX_PATH];
    UINT len;
    BOOL bool;
    int myint;
    IDispatch *pDatabase = NULL, *pInst = NULL;
    HRESULT hr;

    /* Session::Installer */
    hr = Session_Installer(pSession, &pInst);
    ok(SUCCEEDED(hr), "Session_Installer failed, hresult 0x%08x\n", hr);
    ok(pInst != NULL, "Session_Installer returned NULL IDispatch pointer\n");
    ok(pInst == pInstaller, "Session_Installer does not match Installer instance from CoCreateInstance\n");

    /* Session::Property, get */
    memset(stringw, 0, sizeof(stringw));
    hr = Session_PropertyGet(pSession, szProductName, stringw);
    ok(SUCCEEDED(hr), "Session_PropertyGet failed, hresult 0x%08x\n", hr);
    if (lstrcmpW(stringw, szMSITEST) != 0)
    {
        len = WideCharToMultiByte(CP_ACP, 0, stringw, -1, string, MAX_PATH, NULL, NULL);
        ok(len, "WideCharToMultiByteChar returned error %d\n", GetLastError());
        ok(0, "Property \"ProductName\" expected to be \"MSITEST\" but was \"%s\"\n", string);
    }

    /* Session::Property, put */
    hr = Session_PropertyPut(pSession, szProductName, szProductName);
    ok(SUCCEEDED(hr), "Session_PropertyPut failed, hresult 0x%08x\n", hr);
    memset(stringw, 0, sizeof(stringw));
    hr = Session_PropertyGet(pSession, szProductName, stringw);
    ok(SUCCEEDED(hr), "Session_PropertyGet failed, hresult 0x%08x\n", hr);
    if (lstrcmpW(stringw, szProductName) != 0)
    {
        len = WideCharToMultiByte(CP_ACP, 0, stringw, -1, string, MAX_PATH, NULL, NULL);
        ok(len, "WideCharToMultiByteChar returned error %d\n", GetLastError());
        ok(0, "Property \"ProductName\" expected to be \"ProductName\" but was \"%s\"\n", string);
    }

    /* Try putting a property using empty property identifier */
    hr = Session_PropertyPut(pSession, szEmpty, szProductName);
    ok(hr == DISP_E_EXCEPTION, "Session_PropertyPut failed, hresult 0x%08x\n", hr);
    ok_exception(hr, szPropertyName);

    /* Try putting a property using illegal property identifier */
    hr = Session_PropertyPut(pSession, szEquals, szProductName);
    ok(SUCCEEDED(hr), "Session_PropertyPut failed, hresult 0x%08x\n", hr);

    /* Session::Language, get */
    hr = Session_LanguageGet(pSession, &len);
    ok(SUCCEEDED(hr), "Session_LanguageGet failed, hresult 0x%08x\n", hr);
    /* Not sure how to check the language is correct */

    /* Session::Mode, get */
    hr = Session_ModeGet(pSession, MSIRUNMODE_REBOOTATEND, &bool);
    ok(SUCCEEDED(hr), "Session_ModeGet failed, hresult 0x%08x\n", hr);
    todo_wine ok(!bool, "Reboot at end session mode is %d\n", bool);

    /* Session::Mode, put */
    hr = Session_ModePut(pSession, MSIRUNMODE_REBOOTATEND, TRUE);
    todo_wine ok(SUCCEEDED(hr), "Session_ModePut failed, hresult 0x%08x\n", hr);
    hr = Session_ModeGet(pSession, MSIRUNMODE_REBOOTATEND, &bool);
    ok(SUCCEEDED(hr), "Session_ModeGet failed, hresult 0x%08x\n", hr);
    ok(bool, "Reboot at end session mode is %d, expected 1\n", bool);
    hr = Session_ModePut(pSession, MSIRUNMODE_REBOOTATEND, FALSE);  /* set it again so we don't reboot */
    todo_wine ok(SUCCEEDED(hr), "Session_ModePut failed, hresult 0x%08x\n", hr);

    /* Session::Database, get */
    hr = Session_Database(pSession, &pDatabase);
    ok(SUCCEEDED(hr), "Session_Database failed, hresult 0x%08x\n", hr);
    if (SUCCEEDED(hr))
    {
        test_Database(pDatabase);
        IDispatch_Release(pDatabase);
    }

    /* Session::DoAction(CostInitialize) must occur before the next statements */
    hr = Session_DoAction(pSession, szCostInitialize, &myint);
    ok(SUCCEEDED(hr), "Session_DoAction failed, hresult 0x%08x\n", hr);
    ok(myint == IDOK, "DoAction(CostInitialize) returned %d, %d expected\n", myint, IDOK);

    /* Session::SetInstallLevel */
    hr = Session_SetInstallLevel(pSession, INSTALLLEVEL_MINIMUM);
    ok(SUCCEEDED(hr), "Session_SetInstallLevel failed, hresult 0x%08x\n", hr);

    /* Session::FeatureCurrentState, get */
    hr = Session_FeatureCurrentState(pSession, szOne, &myint);
    ok(SUCCEEDED(hr), "Session_FeatureCurrentState failed, hresult 0x%08x\n", hr);
    ok(myint == INSTALLSTATE_UNKNOWN, "Feature current state was %d but expected %d\n", myint, INSTALLSTATE_UNKNOWN);

    /* Session::FeatureRequestState, put */
    hr = Session_FeatureRequestStatePut(pSession, szOne, INSTALLSTATE_ADVERTISED);
    ok(SUCCEEDED(hr), "Session_FeatureRequestStatePut failed, hresult 0x%08x\n", hr);
    hr = Session_FeatureRequestStateGet(pSession, szOne, &myint);
    ok(SUCCEEDED(hr), "Session_FeatureRequestStateGet failed, hresult 0x%08x\n", hr);
    ok(myint == INSTALLSTATE_ADVERTISED, "Feature request state was %d but expected %d\n", myint, INSTALLSTATE_ADVERTISED);
}

/* delete key and all its subkeys */
static DWORD delete_key( HKEY hkey )
{
    char name[MAX_PATH];
    DWORD ret;

    while (!(ret = RegEnumKeyA(hkey, 0, name, sizeof(name))))
    {
        HKEY tmp;
        if (!(ret = RegOpenKeyExA( hkey, name, 0, KEY_ENUMERATE_SUB_KEYS, &tmp )))
        {
            ret = delete_key( tmp );
            RegCloseKey( tmp );
        }
        if (ret) break;
    }
    if (ret != ERROR_NO_MORE_ITEMS) return ret;
    RegDeleteKeyA( hkey, "" );
    return 0;
}

static void test_Installer_RegistryValue(void)
{
    static const DWORD qw[2] = { 0x12345678, 0x87654321 };
    static const WCHAR szKey[] = { 'S','o','f','t','w','a','r','e','\\','W','i','n','e','\\','T','e','s','t',0 };
    static const WCHAR szOne[] = { 'O','n','e',0 };
    static const WCHAR szTwo[] = { 'T','w','o',0 };
    static const WCHAR szThree[] = { 'T','h','r','e','e',0 };
    static const WCHAR szREG_BINARY[] = { '(','R','E','G','_','B','I','N','A','R','Y',')',0 };
    static const WCHAR szFour[] = { 'F','o','u','r',0 };
    static const WCHAR szExpand[] = { '%','M','S','I','T','E','S','T','%',0 };
    static const WCHAR szFive[] = { 'F','i','v','e',0,'H','i',0,0 };
    static const WCHAR szFiveHi[] = { 'F','i','v','e','\n','H','i',0 };
    static const WCHAR szSix[] = { 'S','i','x',0 };
    static const WCHAR szREG_[] = { '(','R','E','G','_',']',0 };
    static const WCHAR szSeven[] = { 'S','e','v','e','n',0 };
    static const WCHAR szEight[] = { 'E','i','g','h','t',0 };
    static const WCHAR szBlank[] = { 0 };
    VARIANT varresult;
    VARIANTARG vararg;
    WCHAR szString[MAX_PATH];
    HKEY hkey, hkey_sub;
    HRESULT hr;
    BOOL bRet;

    /* Delete keys */
    if (!RegOpenKeyW( HKEY_CURRENT_USER, szKey, &hkey )) delete_key( hkey );

    /* Does our key exist? Shouldn't; check with all three possible value parameter types */
    todo_wine {
        hr = Installer_RegistryValueE(HKEY_CURRENT_USER, szKey, &bRet);
        ok(SUCCEEDED(hr), "Installer_RegistryValueE failed, hresult 0x%08x\n", hr);
        if (SUCCEEDED(hr))
            ok(!bRet, "Registry key expected to not exist, but Installer_RegistryValue claims it does\n");

        memset(szString, 0, sizeof(szString));
        hr = Installer_RegistryValueW(HKEY_CURRENT_USER, szKey, NULL, szString);
        ok(hr == DISP_E_BADINDEX, "Installer_RegistryValueW failed, hresult 0x%08x\n", hr);

        memset(szString, 0, sizeof(szString));
        hr = Installer_RegistryValueI(HKEY_CURRENT_USER, szKey, 0, szString, VT_BSTR);
        ok(hr == DISP_E_BADINDEX, "Installer_RegistryValueI failed, hresult 0x%08x\n", hr);
    }

    /* Create key */
    ok(!RegCreateKeyW( HKEY_CURRENT_USER, szKey, &hkey ), "RegCreateKeyW failed\n");

    ok(!RegSetValueExW(hkey,szOne,0,REG_SZ, (const BYTE *)szOne, sizeof(szOne)),
        "RegSetValueExW failed\n");
    ok(!RegSetValueExW(hkey,szTwo,0,REG_DWORD, (const BYTE *)qw, 4),
        "RegSetValueExW failed\n");
    ok(!RegSetValueExW(hkey,szThree,0,REG_BINARY, (const BYTE *)qw, 4),
        "RegSetValueExW failed\n");
    ok(SetEnvironmentVariableA("MSITEST", "Four"), "SetEnvironmentVariableA failed %d\n", GetLastError());
    ok(!RegSetValueExW(hkey,szFour,0,REG_EXPAND_SZ, (const BYTE *)szExpand, sizeof(szExpand)),
        "RegSetValueExW failed\n");
    ok(!RegSetValueExW(hkey,szFive,0,REG_MULTI_SZ, (const BYTE *)szFive, sizeof(szFive)),
        "RegSetValueExW failed\n");
    ok(!RegSetValueExW(hkey,szSix,0,REG_QWORD, (const BYTE *)qw, 8),
        "RegSetValueExW failed\n");
    ok(!RegSetValueExW(hkey,szSeven,0,REG_NONE, (const BYTE *)NULL, 0),
        "RegSetValueExW failed\n");

    ok(!RegSetValueExW(hkey,NULL,0,REG_SZ, (const BYTE *)szOne, sizeof(szOne)),
        "RegSetValueExW failed\n");

    ok(!RegCreateKeyW( hkey, szEight, &hkey_sub ), "RegCreateKeyW failed\n");

    todo_wine {
        /* Does our key exist? It should, and make sure we retrieve the correct default value */
        bRet = FALSE;
        hr = Installer_RegistryValueE(HKEY_CURRENT_USER, szKey, &bRet);
        ok(SUCCEEDED(hr), "Installer_RegistryValueE failed, hresult 0x%08x\n", hr);
        if (SUCCEEDED(hr))
            ok(bRet, "Registry key expected to exist, but Installer_RegistryValue claims it does not\n");

        memset(szString, 0, sizeof(szString));
        hr = Installer_RegistryValueW(HKEY_CURRENT_USER, szKey, NULL, szString);
        ok(SUCCEEDED(hr), "Installer_RegistryValueW failed, hresult 0x%08x\n", hr);
        ok_w2("Default registry value \"%s\" does not match expected \"%s\"\n", szString, szOne);

        /* Ask for the value of a non-existent key */
        memset(szString, 0, sizeof(szString));
        hr = Installer_RegistryValueW(HKEY_CURRENT_USER, szKey, szExpand, szString);
        ok(hr == DISP_E_BADINDEX, "Installer_RegistryValueW failed, hresult 0x%08x\n", hr);

        /* Get values of keys */
        memset(szString, 0, sizeof(szString));
        hr = Installer_RegistryValueW(HKEY_CURRENT_USER, szKey, szOne, szString);
        ok(SUCCEEDED(hr), "Installer_RegistryValueW failed, hresult 0x%08x\n", hr);
        ok_w2("Registry value \"%s\" does not match expected \"%s\"\n", szString, szOne);

        VariantInit(&vararg);
        V_VT(&vararg) = VT_BSTR;
        V_BSTR(&vararg) = SysAllocString(szTwo);
        hr = Installer_RegistryValue(HKEY_CURRENT_USER, szKey, vararg, &varresult, VT_I4);
        ok(SUCCEEDED(hr), "Installer_RegistryValue failed, hresult 0x%08x\n", hr);
        ok(V_I4(&varresult) == 305419896, "Registry value %d does not match expected value\n", V_I4(&varresult));
        VariantClear(&varresult);

        memset(szString, 0, sizeof(szString));
        hr = Installer_RegistryValueW(HKEY_CURRENT_USER, szKey, szThree, szString);
        ok(SUCCEEDED(hr), "Installer_RegistryValueW failed, hresult 0x%08x\n", hr);
        ok_w2("Registry value \"%s\" does not match expected \"%s\"\n", szString, szREG_BINARY);

        memset(szString, 0, sizeof(szString));
        hr = Installer_RegistryValueW(HKEY_CURRENT_USER, szKey, szFour, szString);
        ok(SUCCEEDED(hr), "Installer_RegistryValueW failed, hresult 0x%08x\n", hr);
        ok_w2("Registry value \"%s\" does not match expected \"%s\"\n", szString, szFour);

        memset(szString, 0, sizeof(szString));
        hr = Installer_RegistryValueW(HKEY_CURRENT_USER, szKey, szFive, szString);
        ok(SUCCEEDED(hr), "Installer_RegistryValueW failed, hresult 0x%08x\n", hr);
        ok_w2("Registry value \"%s\" does not match expected \"%s\"\n", szString, szFiveHi);

        memset(szString, 0, sizeof(szString));
        hr = Installer_RegistryValueW(HKEY_CURRENT_USER, szKey, szSix, szString);
        ok(SUCCEEDED(hr), "Installer_RegistryValueW failed, hresult 0x%08x\n", hr);
        ok_w2("Registry value \"%s\" does not match expected \"%s\"\n", szString, szREG_);

        VariantInit(&vararg);
        V_VT(&vararg) = VT_BSTR;
        V_BSTR(&vararg) = SysAllocString(szSeven);
        hr = Installer_RegistryValue(HKEY_CURRENT_USER, szKey, vararg, &varresult, VT_EMPTY);
        ok(SUCCEEDED(hr), "Installer_RegistryValue failed, hresult 0x%08x\n", hr);

        /* Get string class name for the key */
        memset(szString, 0, sizeof(szString));
        hr = Installer_RegistryValueI(HKEY_CURRENT_USER, szKey, 0, szString, VT_BSTR);
        ok(SUCCEEDED(hr), "Installer_RegistryValueI failed, hresult 0x%08x\n", hr);
        ok_w2("Registry name \"%s\" does not match expected \"%s\"\n", szString, szBlank);

        /* Get name of a value by positive number (RegEnumValue like), valid index */
        memset(szString, 0, sizeof(szString));
        hr = Installer_RegistryValueI(HKEY_CURRENT_USER, szKey, 2, szString, VT_BSTR);
        ok(SUCCEEDED(hr), "Installer_RegistryValueI failed, hresult 0x%08x\n", hr);
        ok_w2("Registry name \"%s\" does not match expected \"%s\"\n", szString, szTwo);

        /* Get name of a value by positive number (RegEnumValue like), invalid index */
        memset(szString, 0, sizeof(szString));
        hr = Installer_RegistryValueI(HKEY_CURRENT_USER, szKey, 10, szString, VT_EMPTY);
        ok(SUCCEEDED(hr), "Installer_RegistryValueI failed, hresult 0x%08x\n", hr);

        /* Get name of a subkey by negative number (RegEnumValue like), valid index */
        memset(szString, 0, sizeof(szString));
        hr = Installer_RegistryValueI(HKEY_CURRENT_USER, szKey, -1, szString, VT_BSTR);
        ok(SUCCEEDED(hr), "Installer_RegistryValueI failed, hresult 0x%08x\n", hr);
        ok_w2("Registry name \"%s\" does not match expected \"%s\"\n", szString, szEight);

        /* Get name of a subkey by negative number (RegEnumValue like), invalid index */
        memset(szString, 0, sizeof(szString));
        hr = Installer_RegistryValueI(HKEY_CURRENT_USER, szKey, -10, szString, VT_EMPTY);
        ok(SUCCEEDED(hr), "Installer_RegistryValueI failed, hresult 0x%08x\n", hr);
    }

    /* clean up */
    delete_key(hkey);
}

static void test_Installer(void)
{
    static WCHAR szProductCode[] = { '{','F','1','C','3','A','F','5','0','-','8','B','5','6','-','4','A','6','9','-','A','0','0','C','-','0','0','7','7','3','F','E','4','2','F','3','0','}',0 };
    static WCHAR szBackslash[] = { '\\',0 };
    WCHAR szPath[MAX_PATH];
    HRESULT hr;
    UINT len;
    IDispatch *pSession = NULL, *pRecord = NULL, *pStringList = NULL;
    int iState;

    if (!pInstaller) return;

    /* Installer::CreateRecord */
    todo_wine {
        hr = Installer_CreateRecord(1, &pRecord);
        ok(SUCCEEDED(hr), "Installer_CreateRecord failed, hresult 0x%08x\n", hr);
        ok(pRecord != NULL, "Installer_CreateRecord should not have returned NULL record\n");
    }
    if (pRecord)
    {
        int iFieldCount = 0;

        /* Record::FieldCountGet */
        hr = Record_FieldCountGet(pRecord, &iFieldCount);
        ok(SUCCEEDED(hr), "Record_FiledCountGet failed, hresult 0x%08x\n", hr);
        ok(iFieldCount == 1, "Record_FieldCountGet result was %d but expected 1\n", iFieldCount);

        IDispatch_Release(pRecord);
    }

    /* Prepare package */
    create_database(msifile, tables, sizeof(tables) / sizeof(msi_table));

    len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, CURR_DIR, -1, szPath, MAX_PATH);
    ok(len, "MultiByteToWideChar returned error %d\n", GetLastError());
    if (!len) return;

    lstrcatW(szPath, szBackslash);
    lstrcatW(szPath, szMsifile);

    /* Installer::OpenPackage */
    hr = Installer_OpenPackage(szPath, 0, &pSession);
    ok(SUCCEEDED(hr), "Installer_OpenPackage failed, hresult 0x%08x\n", hr);
    if (SUCCEEDED(hr))
    {
        test_Session(pSession);
        IDispatch_Release(pSession);
    }

    DeleteFileA(msifile);

    /* Installer::RegistryValue */
    test_Installer_RegistryValue();

    /* Installer::Products */
    hr = Installer_Products(&pStringList);
    ok(SUCCEEDED(hr), "Installer_Products failed, hresult 0x%08x\n", hr);
    if (SUCCEEDED(hr))
    {
        int iCount = 0, idx;

        /* StringList::Count */
        hr = StringList_Count(pStringList, &iCount);
        ok(SUCCEEDED(hr), "StringList_Count failed, hresult 0x%08x\n", hr);

        for (idx=0; idx<iCount; idx++)
        {
            /* StringList::Item */
            memset(szPath, 0, sizeof(szPath));
            hr = StringList_Item(pStringList, idx, szPath);
            ok(SUCCEEDED(hr), "StringList_Item failed (idx %d, count %d), hresult 0x%08x\n", idx, iCount, hr);

            if (SUCCEEDED(hr))
            {
                /* Installer::ProductState */
                hr = Installer_ProductState(szPath, &iState);
                ok(SUCCEEDED(hr), "Installer_ProductState failed, hresult 0x%08x\n", hr);
                if (SUCCEEDED(hr))
                    ok(iState == INSTALLSTATE_DEFAULT || iState == INSTALLSTATE_ADVERTISED, "Installer_ProductState returned %d, expected %d or %d\n", iState, INSTALLSTATE_DEFAULT, INSTALLSTATE_ADVERTISED);
            }
        }

        /* StringList::Item using an invalid index */
        memset(szPath, 0, sizeof(szPath));
        hr = StringList_Item(pStringList, iCount, szPath);
        ok(hr == DISP_E_BADINDEX, "StringList_Item for an invalid index did not return DISP_E_BADINDEX, hresult 0x%08x\n", hr);

        IDispatch_Release(pStringList);
    }

    /* Installer::ProductState for our product code, which should not be installed */
    hr = Installer_ProductState(szProductCode, &iState);
    ok(SUCCEEDED(hr), "Installer_ProductState failed, hresult 0x%08x\n", hr);
    if (SUCCEEDED(hr))
        ok(iState == INSTALLSTATE_UNKNOWN, "Installer_ProductState returned %d, expected %d\n", iState, INSTALLSTATE_UNKNOWN);

    /* Installer::Version */
    todo_wine {
        memset(szPath, 0, sizeof(szPath));
        hr = Installer_VersionGet(szPath);
        ok(SUCCEEDED(hr), "Installer_VersionGet failed, hresult 0x%08x\n", hr);
    }
}

START_TEST(automation)
{
    DWORD len;
    char temp_path[MAX_PATH], prev_path[MAX_PATH];
    HRESULT hr;
    CLSID clsid;
    IUnknown *pUnk;

    GetCurrentDirectoryA(MAX_PATH, prev_path);
    GetTempPath(MAX_PATH, temp_path);
    SetCurrentDirectoryA(temp_path);

    lstrcpyA(CURR_DIR, temp_path);
    len = lstrlenA(CURR_DIR);

    if(len && (CURR_DIR[len - 1] == '\\'))
        CURR_DIR[len - 1] = 0;

    hr = OleInitialize(NULL);
    ok (SUCCEEDED(hr), "OleInitialize returned 0x%08x\n", hr);
    hr = CLSIDFromProgID(szProgId, &clsid);
    ok (SUCCEEDED(hr), "CLSIDFromProgID returned 0x%08x\n", hr);
    hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&pUnk);
    ok(SUCCEEDED(hr), "CoCreateInstance returned 0x%08x\n", hr);

    if (pUnk)
    {
        hr = IUnknown_QueryInterface(pUnk, &IID_IDispatch, (void **)&pInstaller);
        ok (SUCCEEDED(hr), "IUnknown::QueryInterface returned 0x%08x\n", hr);

        test_dispid();
        test_dispatch();
        test_Installer();

        hr = IUnknown_Release(pUnk);
        ok (SUCCEEDED(hr), "IUnknown::Release returned 0x%08x\n", hr);
    }

    OleUninitialize();

    SetCurrentDirectoryA(prev_path);
}
