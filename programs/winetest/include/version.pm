package version;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "GetFileVersionInfoA" => ["long",  ["str", "long", "long", "ptr"]],
    "GetFileVersionInfoSizeA" => ["long",  ["str", "ptr"]],
    "GetFileVersionInfoSizeW" => ["long",  ["wstr", "ptr"]],
    "GetFileVersionInfoW" => ["long",  ["wstr", "long", "long", "ptr"]],
    "VerFindFileA" => ["long",  ["long", "str", "str", "str", "str", "ptr", "str", "ptr"]],
    "VerFindFileW" => ["long",  ["long", "wstr", "wstr", "wstr", "wstr", "ptr", "wstr", "ptr"]],
    "VerInstallFileA" => ["long",  ["long", "str", "str", "str", "str", "str", "str", "ptr"]],
    "VerInstallFileW" => ["long",  ["long", "wstr", "wstr", "wstr", "wstr", "wstr", "wstr", "ptr"]],
    "VerQueryValueA" => ["long",  ["ptr", "str", "ptr", "ptr"]],
    "VerQueryValueW" => ["long",  ["ptr", "wstr", "ptr", "ptr"]]
};

&wine::declare("version",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
