package psapi;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "QueryWorkingSet" => ["long",  ["long", "ptr", "long"]],
    "EmptyWorkingSet" => ["long",  ["long"]],
    "EnumDeviceDrivers" => ["long",  ["ptr", "long", "ptr"]],
    "EnumProcessModules" => ["long",  ["long", "ptr", "long", "ptr"]],
    "EnumProcesses" => ["long",  ["ptr", "long", "ptr"]],
    "GetDeviceDriverBaseNameA" => ["long",  ["ptr", "str", "long"]],
    "GetDeviceDriverBaseNameW" => ["long",  ["ptr", "wstr", "long"]],
    "GetDeviceDriverFileNameA" => ["long",  ["ptr", "str", "long"]],
    "GetDeviceDriverFileNameW" => ["long",  ["ptr", "wstr", "long"]],
    "GetMappedFileNameA" => ["long",  ["long", "ptr", "str", "long"]],
    "GetMappedFileNameW" => ["long",  ["long", "ptr", "wstr", "long"]],
    "GetModuleBaseNameA" => ["long",  ["long", "long", "str", "long"]],
    "GetModuleBaseNameW" => ["long",  ["long", "long", "wstr", "long"]],
    "GetModuleFileNameExA" => ["long",  ["long", "long", "str", "long"]],
    "GetModuleFileNameExW" => ["long",  ["long", "long", "wstr", "long"]],
    "GetModuleInformation" => ["long",  ["long", "long", "ptr", "long"]],
    "GetProcessMemoryInfo" => ["long",  ["long", "ptr", "long"]],
    "GetWsChanges" => ["long",  ["long", "ptr", "long"]],
    "InitializeProcessForWsWatch" => ["long",  ["long"]]
};

&wine::declare("psapi",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
