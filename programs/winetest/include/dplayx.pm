package dplayx;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "DirectPlayCreate" => ["long",  ["ptr", "ptr", "ptr"]],
    "DirectPlayEnumerateA" => ["long",  ["ptr", "ptr"]],
    "DirectPlayEnumerateW" => ["long",  ["ptr", "ptr"]],
    "DirectPlayLobbyCreateA" => ["long",  ["ptr", "ptr", "ptr", "ptr", "long"]],
    "DirectPlayLobbyCreateW" => ["long",  ["ptr", "ptr", "ptr", "ptr", "long"]],
    "DirectPlayEnumerate" => ["long",  ["ptr", "ptr"]],
    "DllCanUnloadNow" => ["long",  []],
    "DllGetClassObject" => ["long",  ["ptr", "ptr", "ptr"]]
};

&wine::declare("dplayx",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
