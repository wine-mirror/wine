package lz32;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "CopyLZFile" => ["long",  ["long", "long"]],
    "GetExpandedNameA" => ["long",  ["str", "str"]],
    "GetExpandedNameW" => ["long",  ["wstr", "wstr"]],
    "LZClose" => ["void",  ["long"]],
    "LZCopy" => ["long",  ["long", "long"]],
    "LZDone" => ["void",  []],
    "LZInit" => ["long",  ["long"]],
    "LZOpenFileA" => ["long",  ["str", "ptr", "long"]],
    "LZOpenFileW" => ["long",  ["wstr", "ptr", "long"]],
    "LZRead" => ["long",  ["long", "ptr", "long"]],
    "LZSeek" => ["long",  ["long", "long", "long"]],
    "LZStart" => ["long",  []]
};

&wine::declare("lz32",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
