package rasapi32;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "RasDeleteEntryA" => ["long",  ["str", "str"]],
    "RasDeleteEntryW" => ["long",  ["wstr", "wstr"]],
    "RasEnumAutodialAddressesA" => ["long",  ["ptr", "ptr", "ptr"]],
    "RasEnumAutodialAddressesW" => ["long",  ["ptr", "ptr", "ptr"]],
    "RasEnumDevicesA" => ["long",  ["ptr", "ptr", "ptr"]],
    "RasEnumDevicesW" => ["long",  ["ptr", "ptr", "ptr"]],
    "RasGetAutodialAddressA" => ["long",  ["str", "ptr", "ptr", "ptr", "ptr"]],
    "RasGetAutodialAddressW" => ["long",  ["wstr", "ptr", "ptr", "ptr", "ptr"]],
    "RasGetAutodialEnableA" => ["long",  ["long", "ptr"]],
    "RasGetAutodialEnableW" => ["long",  ["long", "ptr"]],
    "RasGetAutodialParamA" => ["long",  ["long", "ptr", "ptr"]],
    "RasGetAutodialParamW" => ["long",  ["long", "ptr", "ptr"]],
    "RasSetAutodialAddressA" => ["long",  ["str", "long", "ptr", "long", "long"]],
    "RasSetAutodialAddressW" => ["long",  ["wstr", "long", "ptr", "long", "long"]],
    "RasSetAutodialEnableA" => ["long",  ["long", "long"]],
    "RasSetAutodialEnableW" => ["long",  ["long", "long"]],
    "RasSetAutodialParamA" => ["long",  ["long", "ptr", "long"]],
    "RasSetAutodialParamW" => ["long",  ["long", "ptr", "long"]],
    "RasEnumConnectionsA" => ["long",  ["ptr", "ptr", "ptr"]],
    "RasEnumEntriesA" => ["long",  ["str", "str", "ptr", "ptr", "ptr"]],
    "RasGetEntryDialParamsA" => ["long",  ["str", "ptr", "ptr"]],
    "RasHangUpA" => ["long",  ["long"]]
};

&wine::declare("rasapi32",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
