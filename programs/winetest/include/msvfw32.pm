package msvfw32;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "VideoForWindowsVersion" => ["long",  []],
    "DrawDibBegin" => ["long",  ["long", "long", "long", "long", "ptr", "long", "long", "long"]],
    "DrawDibClose" => ["long",  ["long"]],
    "DrawDibDraw" => ["long",  ["long", "long", "long", "long", "long", "long", "ptr", "ptr", "long", "long", "long", "long", "long"]],
    "DrawDibEnd" => ["long",  ["long"]],
    "DrawDibGetPalette" => ["long",  ["long"]],
    "DrawDibOpen" => ["long",  []],
    "DrawDibRealize" => ["long",  ["long", "long", "long"]],
    "DrawDibSetPalette" => ["long",  ["long", "long"]],
    "DrawDibStart" => ["long",  ["long", "long"]],
    "DrawDibStop" => ["long",  ["long"]],
    "ICClose" => ["long",  ["long"]],
    "ICGetDisplayFormat" => ["long",  ["long", "ptr", "ptr", "long", "long", "long"]],
    "ICGetInfo" => ["long",  ["long", "ptr", "long"]],
    "ICImageCompress" => ["long",  ["long", "long", "ptr", "ptr", "ptr", "long", "ptr"]],
    "ICImageDecompress" => ["long",  ["long", "long", "ptr", "ptr", "ptr"]],
    "ICInfo" => ["long",  ["long", "long", "ptr"]],
    "ICLocate" => ["long",  ["long", "long", "ptr", "ptr", "long"]],
    "ICOpenFunction" => ["long",  ["long", "long", "long", "ptr"]],
    "ICSendMessage" => ["long",  ["long", "long", "long", "long"]],
    "MCIWndRegisterClass" => ["long",  ["long"]]
};

&wine::declare("msvfw32",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
