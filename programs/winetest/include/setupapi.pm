package setupapi;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "SetupCloseFileQueue" => ["long",  ["long"]],
    "SetupCloseInfFile" => ["void",  ["long"]],
    "SetupCommitFileQueueA" => ["long",  ["long", "long", "ptr", "ptr"]],
    "SetupCommitFileQueueW" => ["long",  ["long", "long", "ptr", "ptr"]],
    "SetupDefaultQueueCallbackA" => ["long",  ["ptr", "long", "long", "long"]],
    "SetupDefaultQueueCallbackW" => ["long",  ["ptr", "long", "long", "long"]],
    "SetupDiDestroyDeviceInfoList" => ["long",  ["long"]],
    "SetupDiEnumDeviceInfo" => ["long",  ["long", "long", "ptr"]],
    "SetupDiEnumDeviceInterfaces" => ["long",  ["long", "ptr", "ptr", "long", "ptr"]],
    "SetupDiGetClassDevsA" => ["long",  ["ptr", "str", "long", "long"]],
    "SetupDiGetDeviceInterfaceDetailA" => ["long",  ["long", "ptr", "ptr", "long", "ptr", "ptr"]],
    "SetupDiGetDeviceInterfaceDetailW" => ["long",  ["long", "ptr", "ptr", "long", "ptr", "ptr"]],
    "SetupDiGetDeviceRegistryPropertyA" => ["long",  ["long", "ptr", "long", "ptr", "ptr", "long", "ptr"]],
    "SetupFindFirstLineA" => ["long",  ["long", "str", "str", "ptr"]],
    "SetupFindFirstLineW" => ["long",  ["long", "str", "str", "ptr"]],
    "SetupFindNextLine" => ["long",  ["ptr", "ptr"]],
    "SetupFindNextMatchLineA" => ["long",  ["ptr", "str", "ptr"]],
    "SetupFindNextMatchLineW" => ["long",  ["ptr", "str", "ptr"]],
    "SetupGetBinaryField" => ["long",  ["ptr", "long", "ptr", "long", "ptr"]],
    "SetupGetFieldCount" => ["long",  ["ptr"]],
    "SetupGetFileQueueCount" => ["long",  ["long", "long", "ptr"]],
    "SetupGetFileQueueFlags" => ["long",  ["long", "ptr"]],
    "SetupGetIntField" => ["long",  ["ptr", "long", "ptr"]],
    "SetupGetLineByIndexA" => ["long",  ["long", "str", "long", "ptr"]],
    "SetupGetLineByIndexW" => ["long",  ["long", "str", "long", "ptr"]],
    "SetupGetLineCountA" => ["long",  ["long", "str"]],
    "SetupGetLineCountW" => ["long",  ["long", "str"]],
    "SetupGetLineTextA" => ["long",  ["ptr", "long", "str", "str", "ptr", "long", "ptr"]],
    "SetupGetLineTextW" => ["long",  ["ptr", "long", "str", "str", "str", "long", "ptr"]],
    "SetupGetMultiSzFieldA" => ["long",  ["ptr", "long", "ptr", "long", "ptr"]],
    "SetupGetMultiSzFieldW" => ["long",  ["ptr", "long", "str", "long", "ptr"]],
    "SetupGetStringFieldA" => ["long",  ["ptr", "long", "ptr", "long", "ptr"]],
    "SetupGetStringFieldW" => ["long",  ["ptr", "long", "str", "long", "ptr"]],
    "SetupInitDefaultQueueCallback" => ["ptr",  ["long"]],
    "SetupInitDefaultQueueCallbackEx" => ["ptr",  ["long", "long", "long", "long", "ptr"]],
    "SetupInstallFilesFromInfSectionA" => ["long",  ["long", "long", "long", "str", "str", "long"]],
    "SetupInstallFilesFromInfSectionW" => ["long",  ["long", "long", "long", "str", "str", "long"]],
    "SetupInstallFromInfSectionA" => ["long",  ["long", "long", "str", "long", "long", "str", "long", "ptr", "ptr", "long", "ptr"]],
    "SetupInstallFromInfSectionW" => ["long",  ["long", "long", "str", "long", "long", "str", "long", "ptr", "ptr", "long", "ptr"]],
    "SetupIterateCabinetA" => ["long",  ["str", "long", "ptr", "ptr"]],
    "SetupIterateCabinetW" => ["long",  ["str", "long", "ptr", "ptr"]],
    "SetupOpenAppendInfFileA" => ["long",  ["str", "long", "ptr"]],
    "SetupOpenAppendInfFileW" => ["long",  ["str", "long", "ptr"]],
    "SetupOpenFileQueue" => ["long",  []],
    "SetupOpenInfFileA" => ["long",  ["str", "str", "long", "ptr"]],
    "SetupOpenInfFileW" => ["long",  ["str", "str", "long", "ptr"]],
    "SetupQueueCopyA" => ["long",  ["long", "str", "str", "str", "str", "str", "str", "str", "long"]],
    "SetupQueueCopyIndirectA" => ["long",  ["ptr"]],
    "SetupQueueCopyIndirectW" => ["long",  ["ptr"]],
    "SetupQueueCopySectionA" => ["long",  ["long", "str", "long", "long", "str", "long"]],
    "SetupQueueCopySectionW" => ["long",  ["long", "str", "long", "long", "str", "long"]],
    "SetupQueueCopyW" => ["long",  ["long", "str", "str", "str", "str", "str", "str", "str", "long"]],
    "SetupQueueDefaultCopyA" => ["long",  ["long", "long", "str", "str", "str", "long"]],
    "SetupQueueDefaultCopyW" => ["long",  ["long", "long", "str", "str", "str", "long"]],
    "SetupQueueDeleteA" => ["long",  ["long", "str", "str"]],
    "SetupQueueDeleteSectionA" => ["long",  ["long", "long", "long", "str"]],
    "SetupQueueDeleteSectionW" => ["long",  ["long", "long", "long", "str"]],
    "SetupQueueDeleteW" => ["long",  ["long", "str", "str"]],
    "SetupQueueRenameA" => ["long",  ["long", "str", "str", "str", "str"]],
    "SetupQueueRenameSectionA" => ["long",  ["long", "long", "long", "str"]],
    "SetupQueueRenameSectionW" => ["long",  ["long", "long", "long", "str"]],
    "SetupQueueRenameW" => ["long",  ["long", "str", "str", "str", "str"]],
    "SetupScanFileQueueA" => ["long",  ["long", "long", "long", "ptr", "ptr", "ptr"]],
    "SetupScanFileQueueW" => ["long",  ["long", "long", "long", "ptr", "ptr", "ptr"]],
    "SetupSetDirectoryIdA" => ["long",  ["long", "long", "str"]],
    "SetupSetDirectoryIdW" => ["long",  ["long", "long", "str"]],
    "SetupSetFileQueueFlags" => ["long",  ["long", "long", "long"]],
    "SetupTermDefaultQueueCallback" => ["void",  ["ptr"]]
};

&wine::declare("setupapi",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
