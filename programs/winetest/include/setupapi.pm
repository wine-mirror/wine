package setupapi;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "SetupCloseFileQueue" => ["void",  ["long"]],
    "SetupCloseInfFile" => ["void",  ["long"]],
    "SetupCommitFileQueueA" => ["long",  ["long", "long", "ptr", "ptr"]],
    "SetupDefaultQueueCallbackA" => ["long",  ["ptr", "long", "long", "long"]],
    "SetupFindFirstLineA" => ["long",  ["long", "str", "str", "ptr"]],
    "SetupFindNextLine" => ["long",  ["ptr", "ptr"]],
    "SetupGetLineByIndexA" => ["long",  ["long", "str", "long", "ptr"]],
    "SetupGetLineTextA" => ["long",  ["ptr", "long", "str", "str", "str", "long", "ptr"]],
    "SetupGetStringFieldA" => ["long",  ["ptr", "long", "str", "long", "ptr"]],
    "SetupInitDefaultQueueCallback" => ["ptr",  ["long"]],
    "SetupInitDefaultQueueCallbackEx" => ["ptr",  ["long", "long", "long", "long", "ptr"]],
    "SetupInstallFromInfSectionA" => ["long",  ["long", "long", "str", "long", "long", "str", "long", "ptr", "ptr", "long", "ptr"]],
    "SetupIterateCabinetA" => ["long",  ["str", "long", "ptr", "ptr"]],
    "SetupIterateCabinetW" => ["long",  ["str", "long", "ptr", "ptr"]],
    "SetupOpenAppendInfFileA" => ["long",  ["str", "long", "ptr"]],
    "SetupOpenFileQueue" => ["long",  []],
    "SetupOpenInfFileA" => ["long",  ["str", "str", "long", "ptr"]],
    "SetupQueueCopyA" => ["long",  ["long", "str", "str", "str", "str", "str", "str", "str", "long"]],
    "SetupSetDirectoryIdA" => ["long",  ["long", "long", "str"]],
    "SetupTermDefaultQueueCallback" => ["void",  ["ptr"]]
};

&wine::declare("setupapi",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
