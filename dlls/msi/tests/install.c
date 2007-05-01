/*
 * Copyright (C) 2006 James Hawkins
 *
 * A test program for installing MSI products.
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

#include <stdio.h>

#include <windows.h>
#include <msiquery.h>
#include <msidefs.h>
#include <msi.h>
#include <fci.h>

#include "wine/test.h"

static const char *msifile = "msitest.msi";
CHAR CURR_DIR[MAX_PATH];
CHAR PROG_FILES_DIR[MAX_PATH];

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
                                  "Three\t\tThree\tThe Three Feature\t3\t3\tCHANGEDDIR\t0\n"
                                  "Two\t\tTwo\tThe Two Feature\t2\t3\tFIRSTDIR\t0\n"
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

/* tables for test_continuouscabs */
static const CHAR cc_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                       "s72\tS38\ts72\ti2\tS255\tS72\n"
                                       "Component\tComponent\n"
                                       "maximus\t\tMSITESTDIR\t0\t1\tmaximus\n"
                                       "augustus\t\tMSITESTDIR\t0\t1\taugustus\n"
                                       "caesar\t\tMSITESTDIR\t0\t1\tcaesar\n";

static const CHAR cc_feature_dat[] = "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
                                     "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
                                     "Feature\tFeature\n"
                                     "feature\t\t\t\t2\t1\tTARGETDIR\t0";

static const CHAR cc_feature_comp_dat[] = "Feature_\tComponent_\n"
                                          "s38\ts72\n"
                                          "FeatureComponents\tFeature_\tComponent_\n"
                                          "feature\tmaximus\n"
                                          "feature\taugustus\n"
                                          "feature\tcaesar";

static const CHAR cc_file_dat[] = "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
                                  "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
                                  "File\tFile\n"
                                  "maximus\tmaximus\tmaximus\t500\t\t\t16384\t1\n"
                                  "augustus\taugustus\taugustus\t50000\t\t\t16384\t2\n"
                                  "caesar\tcaesar\tcaesar\t500\t\t\t16384\t12";

static const CHAR cc_media_dat[] = "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
                                   "i2\ti4\tL64\tS255\tS32\tS72\n"
                                   "Media\tDiskId\n"
                                   "1\t10\t\ttest1.cab\tDISK1\t\n"
                                   "2\t2\t\ttest2.cab\tDISK2\t\n"
                                   "3\t12\t\ttest3.cab\tDISK3\t\n";

static const CHAR co_file_dat[] = "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
                                  "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
                                  "File\tFile\n"
                                  "maximus\tmaximus\tmaximus\t500\t\t\t16384\t1\n"
                                  "augustus\taugustus\taugustus\t50000\t\t\t16384\t2\n"
                                  "caesar\tcaesar\tcaesar\t500\t\t\t16384\t3";

static const CHAR co_media_dat[] = "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
                                   "i2\ti4\tL64\tS255\tS32\tS72\n"
                                   "Media\tDiskId\n"
                                   "1\t10\t\ttest1.cab\tDISK1\t\n"
                                   "2\t2\t\ttest2.cab\tDISK2\t\n"
                                   "3\t3\t\ttest3.cab\tDISK3\t\n";

static const CHAR co2_media_dat[] = "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
                                    "i2\ti4\tL64\tS255\tS32\tS72\n"
                                    "Media\tDiskId\n"
                                    "1\t10\t\ttest1.cab\tDISK1\t\n"
                                    "2\t12\t\ttest3.cab\tDISK3\t\n"
                                    "3\t2\t\ttest2.cab\tDISK2\t\n";

static const CHAR mm_file_dat[] = "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
                                  "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
                                  "File\tFile\n"
                                  "maximus\tmaximus\tmaximus\t500\t\t\t512\t1\n"
                                  "augustus\taugustus\taugustus\t500\t\t\t512\t2\n"
                                  "caesar\tcaesar\tcaesar\t500\t\t\t16384\t3";

static const CHAR mm_media_dat[] = "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
                                   "i2\ti4\tL64\tS255\tS32\tS72\n"
                                   "Media\tDiskId\n"
                                   "1\t3\t\ttest1.cab\tDISK1\t\n";

static const CHAR ss_media_dat[] = "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
                                   "i2\ti4\tL64\tS255\tS32\tS72\n"
                                   "Media\tDiskId\n"
                                   "1\t2\t\ttest1.cab\tDISK1\t\n"
                                   "2\t2\t\ttest2.cab\tDISK2\t\n"
                                   "3\t12\t\ttest3.cab\tDISK3\t\n";

/* tables for test_uiLevelFlags */
static const CHAR ui_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                       "s72\tS38\ts72\ti2\tS255\tS72\n"
                                       "Component\tComponent\n"
                                       "maximus\t\tMSITESTDIR\t0\tHASUIRUN=1\tmaximus\n"
                                       "augustus\t\tMSITESTDIR\t0\t1\taugustus\n"
                                       "caesar\t\tMSITESTDIR\t0\t1\tcaesar\n";

static const CHAR ui_install_ui_seq_dat[] = "Action\tCondition\tSequence\n"
                                           "s72\tS255\tI2\n"
                                           "InstallUISequence\tAction\n"
                                           "SetUIProperty\t\t5\n"
                                           "ExecuteAction\t\t1100\n";

static const CHAR ui_custom_action_dat[] = "Action\tType\tSource\tTarget\tISComments\n"
                                           "s72\ti2\tS64\tS0\tS255\n"
                                           "CustomAction\tAction\n"
                                           "SetUIProperty\t51\tHASUIRUN\t1\t\n";

static const CHAR rof_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                        "s72\tS38\ts72\ti2\tS255\tS72\n"
                                        "Component\tComponent\n"
                                        "maximus\t\tMSITESTDIR\t0\t1\tmaximus\n";

static const CHAR rof_feature_dat[] = "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
                                      "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
                                      "Feature\tFeature\n"
                                      "feature\t\t\t\t2\t1\tTARGETDIR\t0";

static const CHAR rof_feature_comp_dat[] = "Feature_\tComponent_\n"
                                           "s38\ts72\n"
                                           "FeatureComponents\tFeature_\tComponent_\n"
                                           "feature\tmaximus";

static const CHAR rof_file_dat[] = "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
                                   "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
                                   "File\tFile\n"
                                   "maximus\tmaximus\tmaximus\t500\t\t\t8192\t1";

static const CHAR rof_media_dat[] = "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
                                    "i2\ti4\tL64\tS255\tS32\tS72\n"
                                    "Media\tDiskId\n"
                                    "1\t1\t\t\tDISK1\t\n";

static const CHAR sdp_install_exec_seq_dat[] = "Action\tCondition\tSequence\n"
                                               "s72\tS255\tI2\n"
                                               "InstallExecuteSequence\tAction\n"
                                               "AllocateRegistrySpace\tNOT Installed\t1550\n"
                                               "CostFinalize\t\t1000\n"
                                               "CostInitialize\t\t800\n"
                                               "FileCost\t\t900\n"
                                               "InstallFiles\t\t4000\n"
                                               "InstallFinalize\t\t6600\n"
                                               "InstallInitialize\t\t1500\n"
                                               "InstallValidate\t\t1400\n"
                                               "LaunchConditions\t\t100\n"
                                               "SetDirProperty\t\t950";

static const CHAR sdp_custom_action_dat[] = "Action\tType\tSource\tTarget\tISComments\n"
                                            "s72\ti2\tS64\tS0\tS255\n"
                                            "CustomAction\tAction\n"
                                            "SetDirProperty\t51\tMSITESTDIR\t[CommonFilesFolder]msitest\\\t\n";

static const CHAR cie_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                        "s72\tS38\ts72\ti2\tS255\tS72\n"
                                        "Component\tComponent\n"
                                        "maximus\t\tMSITESTDIR\t0\t1\tmaximus\n"
                                        "augustus\t\tMSITESTDIR\t0\t1\taugustus\n"
                                        "caesar\t\tMSITESTDIR\t0\t1\tcaesar\n"
                                        "gaius\t\tMSITESTDIR\t0\t1\tgaius\n";

static const CHAR cie_feature_comp_dat[] = "Feature_\tComponent_\n"
                                           "s38\ts72\n"
                                           "FeatureComponents\tFeature_\tComponent_\n"
                                           "feature\tmaximus\n"
                                           "feature\taugustus\n"
                                           "feature\tcaesar\n"
                                           "feature\tgaius";

static const CHAR cie_file_dat[] = "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
                                   "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
                                   "File\tFile\n"
                                   "maximus\tmaximus\tmaximus\t500\t\t\t16384\t1\n"
                                   "augustus\taugustus\taugustus\t50000\t\t\t16384\t2\n"
                                   "caesar\tcaesar\tcaesar\t500\t\t\t16384\t12\n"
                                   "gaius\tgaius\tgaius\t500\t\t\t8192\t11";

static const CHAR cie_media_dat[] = "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
                                    "i2\ti4\tL64\tS255\tS32\tS72\n"
                                    "Media\tDiskId\n"
                                    "1\t1\t\ttest1.cab\tDISK1\t\n"
                                    "2\t2\t\ttest2.cab\tDISK2\t\n"
                                    "3\t12\t\ttest3.cab\tDISK3\t\n";

static const CHAR ci_install_exec_seq_dat[] = "Action\tCondition\tSequence\n"
                                              "s72\tS255\tI2\n"
                                              "InstallExecuteSequence\tAction\n"
                                              "CostFinalize\t\t1000\n"
                                              "CostInitialize\t\t800\n"
                                              "FileCost\t\t900\n"
                                              "InstallFiles\t\t4000\n"
                                              "InstallServices\t\t5000\n"
                                              "InstallFinalize\t\t6600\n"
                                              "InstallInitialize\t\t1500\n"
                                              "RunInstall\t\t1600\n"
                                              "InstallValidate\t\t1400\n"
                                              "LaunchConditions\t\t100";

static const CHAR ci_custom_action_dat[] = "Action\tType\tSource\tTarget\tISComments\n"
                                            "s72\ti2\tS64\tS0\tS255\n"
                                            "CustomAction\tAction\n"
                                            "RunInstall\t23\tmsitest\\concurrent.msi\tMYPROP=[UILevel]\t\n";

static const CHAR ci_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                       "s72\tS38\ts72\ti2\tS255\tS72\n"
                                       "Component\tComponent\n"
                                       "maximus\t\tMSITESTDIR\t0\tUILevel=5\tmaximus\n";

static const CHAR ci2_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                        "s72\tS38\ts72\ti2\tS255\tS72\n"
                                        "Component\tComponent\n"
                                        "augustus\t\tMSITESTDIR\t0\tUILevel=3 AND MYPROP=5\taugustus\n";

static const CHAR ci2_feature_comp_dat[] = "Feature_\tComponent_\n"
                                           "s38\ts72\n"
                                           "FeatureComponents\tFeature_\tComponent_\n"
                                           "feature\taugustus";

static const CHAR ci2_file_dat[] = "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
                                   "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
                                   "File\tFile\n"
                                   "augustus\taugustus\taugustus\t500\t\t\t8192\t1";

static const CHAR spf_custom_action_dat[] = "Action\tType\tSource\tTarget\tISComments\n"
                                            "s72\ti2\tS64\tS0\tS255\n"
                                            "CustomAction\tAction\n"
                                            "SetFolderProp\t51\tMSITESTDIR\t[ProgramFilesFolder]\\msitest\\added\t\n";

static const CHAR spf_install_exec_seq_dat[] = "Action\tCondition\tSequence\n"
                                               "s72\tS255\tI2\n"
                                               "InstallExecuteSequence\tAction\n"
                                               "CostFinalize\t\t1000\n"
                                               "CostInitialize\t\t800\n"
                                               "FileCost\t\t900\n"
                                               "SetFolderProp\t\t950\n"
                                               "InstallFiles\t\t4000\n"
                                               "InstallServices\t\t5000\n"
                                               "InstallFinalize\t\t6600\n"
                                               "InstallInitialize\t\t1500\n"
                                               "InstallValidate\t\t1400\n"
                                               "LaunchConditions\t\t100";

static const CHAR spf_install_ui_seq_dat[] = "Action\tCondition\tSequence\n"
                                             "s72\tS255\tI2\n"
                                             "InstallUISequence\tAction\n"
                                             "CostInitialize\t\t800\n"
                                             "FileCost\t\t900\n"
                                             "CostFinalize\t\t1000\n"
                                             "ExecuteAction\t\t1100\n";

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

static const msi_table cc_tables[] =
{
    ADD_TABLE(cc_component),
    ADD_TABLE(directory),
    ADD_TABLE(cc_feature),
    ADD_TABLE(cc_feature_comp),
    ADD_TABLE(cc_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(cc_media),
    ADD_TABLE(property),
};

static const msi_table co_tables[] =
{
    ADD_TABLE(cc_component),
    ADD_TABLE(directory),
    ADD_TABLE(cc_feature),
    ADD_TABLE(cc_feature_comp),
    ADD_TABLE(co_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(co_media),
    ADD_TABLE(property),
};

static const msi_table co2_tables[] =
{
    ADD_TABLE(cc_component),
    ADD_TABLE(directory),
    ADD_TABLE(cc_feature),
    ADD_TABLE(cc_feature_comp),
    ADD_TABLE(cc_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(co2_media),
    ADD_TABLE(property),
};

static const msi_table mm_tables[] =
{
    ADD_TABLE(cc_component),
    ADD_TABLE(directory),
    ADD_TABLE(cc_feature),
    ADD_TABLE(cc_feature_comp),
    ADD_TABLE(mm_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(mm_media),
    ADD_TABLE(property),
};

static const msi_table ss_tables[] =
{
    ADD_TABLE(cc_component),
    ADD_TABLE(directory),
    ADD_TABLE(cc_feature),
    ADD_TABLE(cc_feature_comp),
    ADD_TABLE(cc_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(ss_media),
    ADD_TABLE(property),
};

static const msi_table ui_tables[] =
{
    ADD_TABLE(ui_component),
    ADD_TABLE(directory),
    ADD_TABLE(cc_feature),
    ADD_TABLE(cc_feature_comp),
    ADD_TABLE(cc_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(ui_install_ui_seq),
    ADD_TABLE(ui_custom_action),
    ADD_TABLE(cc_media),
    ADD_TABLE(property),
};

static const msi_table rof_tables[] =
{
    ADD_TABLE(rof_component),
    ADD_TABLE(directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(rof_feature_comp),
    ADD_TABLE(rof_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
};

static const msi_table sdp_tables[] =
{
    ADD_TABLE(rof_component),
    ADD_TABLE(directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(rof_feature_comp),
    ADD_TABLE(rof_file),
    ADD_TABLE(sdp_install_exec_seq),
    ADD_TABLE(sdp_custom_action),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
};

static const msi_table cie_tables[] =
{
    ADD_TABLE(cie_component),
    ADD_TABLE(directory),
    ADD_TABLE(cc_feature),
    ADD_TABLE(cie_feature_comp),
    ADD_TABLE(cie_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(cie_media),
    ADD_TABLE(property),
};

static const msi_table ci_tables[] =
{
    ADD_TABLE(ci_component),
    ADD_TABLE(directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(rof_feature_comp),
    ADD_TABLE(rof_file),
    ADD_TABLE(ci_install_exec_seq),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
    ADD_TABLE(ci_custom_action),
};

static const msi_table ci2_tables[] =
{
    ADD_TABLE(ci2_component),
    ADD_TABLE(directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(ci2_feature_comp),
    ADD_TABLE(ci2_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
};

static const msi_table spf_tables[] =
{
    ADD_TABLE(ci_component),
    ADD_TABLE(directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(rof_feature_comp),
    ADD_TABLE(rof_file),
    ADD_TABLE(spf_install_exec_seq),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
    ADD_TABLE(spf_custom_action),
    ADD_TABLE(spf_install_ui_seq),
};

/* cabinet definitions */

/* make the max size large so there is only one cab file */
#define MEDIA_SIZE          0x7FFFFFFF
#define FOLDER_THRESHOLD    900000

/* the FCI callbacks */

static void *mem_alloc(ULONG cb)
{
    return HeapAlloc(GetProcessHeap(), 0, cb);
}

static void mem_free(void *memory)
{
    HeapFree(GetProcessHeap(), 0, memory);
}

static BOOL get_next_cabinet(PCCAB pccab, ULONG  cbPrevCab, void *pv)
{
    sprintf(pccab->szCab, pv, pccab->iCab);
    return TRUE;
}

static long progress(UINT typeStatus, ULONG cb1, ULONG cb2, void *pv)
{
    return 0;
}

static int file_placed(PCCAB pccab, char *pszFile, long cbFile,
                       BOOL fContinuation, void *pv)
{
    return 0;
}

static INT_PTR fci_open(char *pszFile, int oflag, int pmode, int *err, void *pv)
{
    HANDLE handle;
    DWORD dwAccess = 0;
    DWORD dwShareMode = 0;
    DWORD dwCreateDisposition = OPEN_EXISTING;
    
    dwAccess = GENERIC_READ | GENERIC_WRITE;
    /* FILE_SHARE_DELETE is not supported by Windows Me/98/95 */
    dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;

    if (GetFileAttributesA(pszFile) != INVALID_FILE_ATTRIBUTES)
        dwCreateDisposition = OPEN_EXISTING;
    else
        dwCreateDisposition = CREATE_NEW;

    handle = CreateFileA(pszFile, dwAccess, dwShareMode, NULL,
                         dwCreateDisposition, 0, NULL);

    ok(handle != INVALID_HANDLE_VALUE, "Failed to CreateFile %s\n", pszFile);

    return (INT_PTR)handle;
}

static UINT fci_read(INT_PTR hf, void *memory, UINT cb, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    DWORD dwRead;
    BOOL res;
    
    res = ReadFile(handle, memory, cb, &dwRead, NULL);
    ok(res, "Failed to ReadFile\n");

    return dwRead;
}

static UINT fci_write(INT_PTR hf, void *memory, UINT cb, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    DWORD dwWritten;
    BOOL res;

    res = WriteFile(handle, memory, cb, &dwWritten, NULL);
    ok(res, "Failed to WriteFile\n");

    return dwWritten;
}

static int fci_close(INT_PTR hf, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    ok(CloseHandle(handle), "Failed to CloseHandle\n");

    return 0;
}

static long fci_seek(INT_PTR hf, long dist, int seektype, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    DWORD ret;
    
    ret = SetFilePointer(handle, dist, NULL, seektype);
    ok(ret != INVALID_SET_FILE_POINTER, "Failed to SetFilePointer\n");

    return ret;
}

static int fci_delete(char *pszFile, int *err, void *pv)
{
    BOOL ret = DeleteFileA(pszFile);
    ok(ret, "Failed to DeleteFile %s\n", pszFile);

    return 0;
}

static BOOL check_record(MSIHANDLE rec, UINT field, LPCSTR val)
{
    CHAR buffer[0x20];
    UINT r;
    DWORD sz;

    sz = sizeof buffer;
    r = MsiRecordGetString(rec, field, buffer, &sz);
    return (r == ERROR_SUCCESS ) && !strcmp(val, buffer);
}

static BOOL get_temp_file(char *pszTempName, int cbTempName, void *pv)
{
    LPSTR tempname;

    tempname = HeapAlloc(GetProcessHeap(), 0, MAX_PATH);
    GetTempFileNameA(".", "xx", 0, tempname);

    if (tempname && (strlen(tempname) < (unsigned)cbTempName))
    {
        lstrcpyA(pszTempName, tempname);
        HeapFree(GetProcessHeap(), 0, tempname);
        return TRUE;
    }

    HeapFree(GetProcessHeap(), 0, tempname);

    return FALSE;
}

static INT_PTR get_open_info(char *pszName, USHORT *pdate, USHORT *ptime,
                             USHORT *pattribs, int *err, void *pv)
{
    BY_HANDLE_FILE_INFORMATION finfo;
    FILETIME filetime;
    HANDLE handle;
    DWORD attrs;
    BOOL res;

    handle = CreateFile(pszName, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    ok(handle != INVALID_HANDLE_VALUE, "Failed to CreateFile %s\n", pszName);

    res = GetFileInformationByHandle(handle, &finfo);
    ok(res, "Expected GetFileInformationByHandle to succeed\n");
   
    FileTimeToLocalFileTime(&finfo.ftLastWriteTime, &filetime);
    FileTimeToDosDateTime(&filetime, pdate, ptime);

    attrs = GetFileAttributes(pszName);
    ok(attrs != INVALID_FILE_ATTRIBUTES, "Failed to GetFileAttributes\n");

    return (INT_PTR)handle;
}

static BOOL add_file(HFCI hfci, const char *file, TCOMP compress)
{
    char path[MAX_PATH];
    char filename[MAX_PATH];

    lstrcpyA(path, CURR_DIR);
    lstrcatA(path, "\\");
    lstrcatA(path, file);

    lstrcpyA(filename, file);

    return FCIAddFile(hfci, path, filename, FALSE, get_next_cabinet,
                      progress, get_open_info, compress);
}

static void set_cab_parameters(PCCAB pCabParams, const CHAR *name, DWORD max_size)
{
    ZeroMemory(pCabParams, sizeof(CCAB));

    pCabParams->cb = max_size;
    pCabParams->cbFolderThresh = FOLDER_THRESHOLD;
    pCabParams->setID = 0xbeef;
    pCabParams->iCab = 1;
    lstrcpyA(pCabParams->szCabPath, CURR_DIR);
    lstrcatA(pCabParams->szCabPath, "\\");
    lstrcpyA(pCabParams->szCab, name);
}

static void create_cab_file(const CHAR *name, DWORD max_size, const CHAR *files)
{
    CCAB cabParams;
    LPCSTR ptr;
    HFCI hfci;
    ERF erf;
    BOOL res;

    set_cab_parameters(&cabParams, name, max_size);

    hfci = FCICreate(&erf, file_placed, mem_alloc, mem_free, fci_open,
                      fci_read, fci_write, fci_close, fci_seek, fci_delete,
                      get_temp_file, &cabParams, NULL);

    ok(hfci != NULL, "Failed to create an FCI context\n");

    ptr = files;
    while (*ptr)
    {
        res = add_file(hfci, ptr, tcompTYPE_MSZIP);
        ok(res, "Failed to add file: %s\n", ptr);
        ptr += lstrlen(ptr) + 1;
    }

    res = FCIFlushCabinet(hfci, FALSE, get_next_cabinet, progress);
    ok(res, "Failed to flush the cabinet\n");

    res = FCIDestroy(hfci);
    ok(res, "Failed to destroy the cabinet\n");
}

static BOOL get_program_files_dir(LPSTR buf)
{
    HKEY hkey;
    DWORD type = REG_EXPAND_SZ, size;

    if (RegOpenKey(HKEY_LOCAL_MACHINE,
                   "Software\\Microsoft\\Windows\\CurrentVersion", &hkey))
        return FALSE;

    size = MAX_PATH;
    if (RegQueryValueEx(hkey, "ProgramFilesDir", 0, &type, (LPBYTE)buf, &size))
        return FALSE;

    RegCloseKey(hkey);
    return TRUE;
}

static void create_file(const CHAR *name, DWORD size)
{
    HANDLE file;
    DWORD written, left;

    file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Failure to open file %s\n", name);
    WriteFile(file, name, strlen(name), &written, NULL);
    WriteFile(file, "\n", strlen("\n"), &written, NULL);

    left = size - lstrlen(name) - 1;

    SetFilePointer(file, left, NULL, FILE_CURRENT);
    SetEndOfFile(file);
    
    CloseHandle(file);
}

static void create_test_files(void)
{
    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\one.txt", 100);
    CreateDirectoryA("msitest\\first", NULL);
    create_file("msitest\\first\\two.txt", 100);
    CreateDirectoryA("msitest\\second", NULL);
    create_file("msitest\\second\\three.txt", 100);

    create_file("four.txt", 100);
    create_file("five.txt", 100);
    create_cab_file("msitest.cab", MEDIA_SIZE, "four.txt\0five.txt\0");

    create_file("msitest\\filename", 100);
    create_file("msitest\\service.exe", 100);

    DeleteFileA("four.txt");
    DeleteFileA("five.txt");
}

static BOOL delete_pf(const CHAR *rel_path, BOOL is_file)
{
    CHAR path[MAX_PATH];

    lstrcpyA(path, PROG_FILES_DIR);
    lstrcatA(path, "\\");
    lstrcatA(path, rel_path);

    if (is_file)
        return DeleteFileA(path);
    else
        return RemoveDirectoryA(path);
}

static void delete_test_files(void)
{
    DeleteFileA("msitest.msi");
    DeleteFileA("msitest.cab");
    DeleteFileA("msitest\\second\\three.txt");
    DeleteFileA("msitest\\first\\two.txt");
    DeleteFileA("msitest\\one.txt");
    DeleteFileA("msitest\\service.exe");
    DeleteFileA("msitest\\filename");
    RemoveDirectoryA("msitest\\second");
    RemoveDirectoryA("msitest\\first");
    RemoveDirectoryA("msitest");
}

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

static void check_service_is_installed(void)
{
    SC_HANDLE scm, service;
    BOOL res;

    scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    ok(scm != NULL, "Failed to open the SC Manager\n");

    service = OpenService(scm, "TestService", SC_MANAGER_ALL_ACCESS);
    ok(service != NULL, "Failed to open TestService\n");

    res = DeleteService(service);
    ok(res, "Failed to delete TestService\n");
}

static void test_MsiInstallProduct(void)
{
    UINT r;
    CHAR path[MAX_PATH];
    LONG res;
    HKEY hkey;
    DWORD num, size, type;

    create_test_files();
    create_database(msifile, tables, sizeof(tables) / sizeof(msi_table));

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    ok(delete_pf("msitest\\cabout\\new\\five.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\cabout\\new", FALSE), "File not installed\n");
    ok(delete_pf("msitest\\cabout\\four.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\cabout", FALSE), "File not installed\n");
    ok(delete_pf("msitest\\changed\\three.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\changed", FALSE), "File not installed\n");
    ok(delete_pf("msitest\\first\\two.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\first", FALSE), "File not installed\n");
    ok(delete_pf("msitest\\one.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\filename", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\service.exe", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    res = RegOpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Wine\\msitest", &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    size = MAX_PATH;
    type = REG_SZ;
    res = RegQueryValueExA(hkey, "Name", NULL, &type, (LPBYTE)path, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(path, "imaname"), "Expected imaname, got %s\n", path);

    size = MAX_PATH;
    type = REG_SZ;
    res = RegQueryValueExA(hkey, "blah", NULL, &type, (LPBYTE)path, &size);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);

    size = sizeof(num);
    type = REG_DWORD;
    res = RegQueryValueExA(hkey, "number", NULL, &type, (LPBYTE)&num, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(num == 314, "Expected 314, got %d\n", num);

    size = MAX_PATH;
    type = REG_SZ;
    res = RegQueryValueExA(hkey, "OrderTestName", NULL, &type, (LPBYTE)path, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(path, "OrderTestValue"), "Expected imaname, got %s\n", path);

    check_service_is_installed();

    RegDeleteKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Wine\\msitest");

    delete_test_files();
}

static void test_MsiSetComponentState(void)
{
    INSTALLSTATE installed, action;
    MSIHANDLE package;
    char path[MAX_PATH];
    UINT r;

    create_database(msifile, tables, sizeof(tables) / sizeof(msi_table));

    CoInitialize(NULL);

    lstrcpy(path, CURR_DIR);
    lstrcat(path, "\\");
    lstrcat(path, msifile);

    r = MsiOpenPackage(path, &package);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiDoAction(package, "CostInitialize");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiDoAction(package, "FileCost");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiDoAction(package, "CostFinalize");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiGetComponentState(package, "dangler", &installed, &action);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(installed == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", installed);
    ok(action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    r = MsiSetComponentState(package, "dangler", INSTALLSTATE_SOURCE);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    MsiCloseHandle(package);
    CoUninitialize();

    DeleteFileA(msifile);
}

static void test_packagecoltypes(void)
{
    MSIHANDLE hdb, view, rec;
    char path[MAX_PATH];
    LPCSTR query;
    UINT r, count;

    create_database(msifile, tables, sizeof(tables) / sizeof(msi_table));

    CoInitialize(NULL);

    lstrcpy(path, CURR_DIR);
    lstrcat(path, "\\");
    lstrcat(path, msifile);

    r = MsiOpenDatabase(path, MSIDBOPEN_READONLY, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    query = "SELECT * FROM `Media`";
    r = MsiDatabaseOpenView( hdb, query, &view );
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");

    r = MsiViewGetColumnInfo( view, MSICOLINFO_NAMES, &rec );
    count = MsiRecordGetFieldCount( rec );
    ok(r == ERROR_SUCCESS, "MsiViewGetColumnInfo failed\n");
    ok(count == 6, "Expected 6, got %d\n", count);
    ok(check_record(rec, 1, "DiskId"), "wrong column label\n");
    ok(check_record(rec, 2, "LastSequence"), "wrong column label\n");
    ok(check_record(rec, 3, "DiskPrompt"), "wrong column label\n");
    ok(check_record(rec, 4, "Cabinet"), "wrong column label\n");
    ok(check_record(rec, 5, "VolumeLabel"), "wrong column label\n");
    ok(check_record(rec, 6, "Source"), "wrong column label\n");
    MsiCloseHandle(rec);

    r = MsiViewGetColumnInfo( view, MSICOLINFO_TYPES, &rec );
    count = MsiRecordGetFieldCount( rec );
    ok(r == ERROR_SUCCESS, "MsiViewGetColumnInfo failed\n");
    ok(count == 6, "Expected 6, got %d\n", count);
    ok(check_record(rec, 1, "i2"), "wrong column label\n");
    ok(check_record(rec, 2, "i4"), "wrong column label\n");
    ok(check_record(rec, 3, "L64"), "wrong column label\n");
    ok(check_record(rec, 4, "S255"), "wrong column label\n");
    ok(check_record(rec, 5, "S32"), "wrong column label\n");
    ok(check_record(rec, 6, "S72"), "wrong column label\n");

    MsiCloseHandle(rec);
    MsiCloseHandle(view);
    MsiCloseHandle(hdb);
    DeleteFile(msifile);
}

static void create_cc_test_files(void)
{
    CCAB cabParams;
    HFCI hfci;
    ERF erf;
    static CHAR cab_context[] = "test%d.cab";
    BOOL res;

    create_file("maximus", 500);
    create_file("augustus", 50000);
    create_file("caesar", 500);

    set_cab_parameters(&cabParams, "test1.cab", 200);

    hfci = FCICreate(&erf, file_placed, mem_alloc, mem_free, fci_open,
                      fci_read, fci_write, fci_close, fci_seek, fci_delete,
                      get_temp_file, &cabParams, cab_context);
    ok(hfci != NULL, "Failed to create an FCI context\n");

    res = add_file(hfci, "maximus", tcompTYPE_MSZIP);
    ok(res, "Failed to add file maximus\n");

    res = add_file(hfci, "augustus", tcompTYPE_MSZIP);
    ok(res, "Failed to add file augustus\n");

    res = FCIFlushCabinet(hfci, FALSE, get_next_cabinet, progress);
    ok(res, "Failed to flush the cabinet\n");

    res = FCIDestroy(hfci);
    ok(res, "Failed to destroy the cabinet\n");

    create_cab_file("test3.cab", MEDIA_SIZE, "caesar\0");

    DeleteFile("maximus");
    DeleteFile("augustus");
    DeleteFile("caesar");
}

static void delete_cab_files(void)
{
    SHFILEOPSTRUCT shfl;
    CHAR path[MAX_PATH];

    lstrcpyA(path, CURR_DIR);
    lstrcatA(path, "\\*.cab");
    path[strlen(path) + 1] = '\0';

    shfl.hwnd = NULL;
    shfl.wFunc = FO_DELETE;
    shfl.pFrom = (LPCSTR)path;
    shfl.pTo = NULL;
    shfl.fFlags = FOF_FILESONLY | FOF_NOCONFIRMATION | FOF_NORECURSION | FOF_SILENT;

    SHFileOperation(&shfl);
}

static void test_continuouscabs(void)
{
    UINT r;

    create_cc_test_files();
    create_database(msifile, cc_tables, sizeof(cc_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
        ok(delete_pf("msitest\\augustus", TRUE), "File not installed\n");
        ok(delete_pf("msitest\\caesar", TRUE), "File not installed\n");
    }
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    delete_cab_files();
    DeleteFile(msifile);
}

static void test_caborder(void)
{
    UINT r;

    create_file("imperator", 100);
    create_file("maximus", 500);
    create_file("augustus", 50000);
    create_file("caesar", 500);

    create_database(msifile, cc_tables, sizeof(cc_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    create_cab_file("test1.cab", MEDIA_SIZE, "maximus\0");
    create_cab_file("test2.cab", MEDIA_SIZE, "augustus\0");
    create_cab_file("test3.cab", MEDIA_SIZE, "caesar\0");

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_INSTALL_FAILURE, "Expected ERROR_INSTALL_FAILURE, got %u\n", r);
    ok(!delete_pf("msitest\\augustus", TRUE), "File is installed\n");
    ok(!delete_pf("msitest\\caesar", TRUE), "File is installed\n");
    todo_wine
    {
        ok(!delete_pf("msitest\\maximus", TRUE), "File is installed\n");
        ok(!delete_pf("msitest", FALSE), "File is installed\n");
    }

    delete_cab_files();

    create_cab_file("test1.cab", MEDIA_SIZE, "imperator\0");
    create_cab_file("test2.cab", MEDIA_SIZE, "maximus\0augustus\0");
    create_cab_file("test3.cab", MEDIA_SIZE, "caesar\0");

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_INSTALL_FAILURE, "Expected ERROR_INSTALL_FAILURE, got %u\n", r);
    ok(!delete_pf("msitest\\maximus", TRUE), "File is installed\n");
    ok(!delete_pf("msitest\\augustus", TRUE), "File is installed\n");
    ok(!delete_pf("msitest\\caesar", TRUE), "File is installed\n");
    todo_wine
    {
        ok(!delete_pf("msitest", FALSE), "File is installed\n");
    }

    delete_cab_files();
    DeleteFile(msifile);

    create_cc_test_files();
    create_database(msifile, co_tables, sizeof(co_tables) / sizeof(msi_table));

    r = MsiInstallProductA(msifile, NULL);
    ok(!delete_pf("msitest\\augustus", TRUE), "File is installed\n");
    ok(!delete_pf("msitest\\caesar", TRUE), "File is installed\n");
    ok(!delete_pf("msitest", FALSE), "File is installed\n");
    todo_wine
    {
        ok(!delete_pf("msitest\\maximus", TRUE), "File is installed\n");
        ok(r == ERROR_INSTALL_FAILURE, "Expected ERROR_INSTALL_FAILURE, got %u\n", r);
    }

    delete_cab_files();
    DeleteFile(msifile);

    create_cc_test_files();
    create_database(msifile, co2_tables, sizeof(co2_tables) / sizeof(msi_table));

    r = MsiInstallProductA(msifile, NULL);
    ok(!delete_pf("msitest\\augustus", TRUE), "File is installed\n");
    ok(!delete_pf("msitest\\caesar", TRUE), "File is installed\n");
    todo_wine
    {
        ok(r == ERROR_INSTALL_FAILURE, "Expected ERROR_INSTALL_FAILURE, got %u\n", r);
        ok(!delete_pf("msitest\\maximus", TRUE), "File is installed\n");
        ok(!delete_pf("msitest", FALSE), "File is installed\n");
    }

    delete_cab_files();
    DeleteFile("imperator");
    DeleteFile("maximus");
    DeleteFile("augustus");
    DeleteFile("caesar");
    DeleteFile(msifile);
}

static void test_mixedmedia(void)
{
    UINT r;

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);
    create_file("msitest\\augustus", 500);
    create_file("caesar", 500);

    create_database(msifile, mm_tables, sizeof(mm_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    create_cab_file("test1.cab", MEDIA_SIZE, "caesar\0");

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\augustus", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\caesar", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    /* Delete the files in the temp (current) folder */
    DeleteFile("msitest\\maximus");
    DeleteFile("msitest\\augustus");
    RemoveDirectory("msitest");
    DeleteFile("caesar");
    DeleteFile("test1.cab");
    DeleteFile(msifile);
}

static void test_samesequence(void)
{
    UINT r;

    create_cc_test_files();
    create_database(msifile, ss_tables, sizeof(ss_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
        ok(delete_pf("msitest\\augustus", TRUE), "File not installed\n");
        ok(delete_pf("msitest\\caesar", TRUE), "File not installed\n");
    }
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    delete_cab_files();
    DeleteFile(msifile);
}

static void test_uiLevelFlags(void)
{
    UINT r;

    create_cc_test_files();
    create_database(msifile, ui_tables, sizeof(ui_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE | INSTALLUILEVEL_SOURCERESONLY, NULL);

    r = MsiInstallProductA(msifile, NULL);
    ok(!delete_pf("msitest\\maximus", TRUE), "UI install occurred, but execute-only was requested.\n");
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
        ok(delete_pf("msitest\\caesar", TRUE), "File not installed\n");
    }
    ok(delete_pf("msitest\\augustus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    delete_cab_files();
    DeleteFile(msifile);
}

static BOOL file_matches(LPSTR path)
{
    CHAR buf[MAX_PATH];
    HANDLE file;
    DWORD size;

    file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL, OPEN_EXISTING, 0, NULL);

    ZeroMemory(buf, MAX_PATH);
    ReadFile(file, buf, 15, &size, NULL);
    CloseHandle(file);

    return !lstrcmp(buf, "msitest\\maximus");
}

static void test_readonlyfile(void)
{
    UINT r;
    DWORD size;
    HANDLE file;
    CHAR path[MAX_PATH];

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);
    create_database(msifile, rof_tables, sizeof(rof_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    lstrcpy(path, PROG_FILES_DIR);
    lstrcat(path, "\\msitest");
    CreateDirectory(path, NULL);

    lstrcat(path, "\\maximus");
    file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL, CREATE_NEW, FILE_ATTRIBUTE_READONLY, NULL);

    WriteFile(file, "readonlyfile", 20, &size, NULL);
    CloseHandle(file);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(file_matches(path), "Expected file to be overwritten\n");
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    /* Delete the files in the temp (current) folder */
    DeleteFile("msitest\\maximus");
    RemoveDirectory("msitest");
    DeleteFile(msifile);
}

static void test_setdirproperty(void)
{
    UINT r;

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);
    create_database(msifile, sdp_tables, sizeof(sdp_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("Common Files\\msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("Common Files\\msitest", FALSE), "File not installed\n");

    DeleteFile(msifile);
}

static void test_cabisextracted(void)
{
    UINT r;

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\gaius", 500);
    create_file("maximus", 500);
    create_file("augustus", 500);
    create_file("caesar", 500);

    create_cab_file("test1.cab", MEDIA_SIZE, "maximus\0");
    create_cab_file("test2.cab", MEDIA_SIZE, "augustus\0");
    create_cab_file("test3.cab", MEDIA_SIZE, "caesar\0");

    create_database(msifile, cie_tables, sizeof(cie_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\augustus", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\caesar", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\gaius", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    delete_cab_files();
    DeleteFile(msifile);
}

static void test_concurrentinstall(void)
{
    UINT r;
    CHAR path[MAX_PATH];

    CreateDirectoryA("msitest", NULL);
    CreateDirectoryA("msitest\\msitest", NULL);
    create_file("msitest\\maximus", 500);
    create_file("msitest\\msitest\\augustus", 500);

    create_database(msifile, ci_tables, sizeof(ci_tables) / sizeof(msi_table));

    lstrcpyA(path, CURR_DIR);
    lstrcatA(path, "\\msitest\\concurrent.msi");
    create_database(path, ci2_tables, sizeof(ci2_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_FULL, NULL);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\augustus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    DeleteFile(msifile);
}

static void test_setpropertyfolder(void)
{
    UINT r;

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);

    create_database(msifile, spf_tables, sizeof(spf_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_FULL, NULL);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    delete_pf("msitest\\maximus", TRUE); /* FIXME: remove when the tests pass */
    todo_wine
    {
        ok(delete_pf("msitest\\added\\maximus", TRUE), "File not installed\n");
        ok(delete_pf("msitest\\added", FALSE), "File not installed\n");
    }
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    DeleteFile(msifile);
}

START_TEST(install)
{
    DWORD len;
    char temp_path[MAX_PATH], prev_path[MAX_PATH];

    GetCurrentDirectoryA(MAX_PATH, prev_path);
    GetTempPath(MAX_PATH, temp_path);
    SetCurrentDirectoryA(temp_path);

    lstrcpyA(CURR_DIR, temp_path);
    len = lstrlenA(CURR_DIR);

    if(len && (CURR_DIR[len - 1] == '\\'))
        CURR_DIR[len - 1] = 0;

    get_program_files_dir(PROG_FILES_DIR);

    test_MsiInstallProduct();
    test_MsiSetComponentState();
    test_packagecoltypes();
    test_continuouscabs();
    test_caborder();
    test_mixedmedia();
    test_samesequence();
    test_uiLevelFlags();
    test_readonlyfile();
    test_setdirproperty();
    test_cabisextracted();
    test_concurrentinstall();
    test_setpropertyfolder();

    SetCurrentDirectoryA(prev_path);
}
