package msacm32;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "DriverProc" => ["long",  ["long", "long", "long", "long", "long"]],
    "acmDriverAddA" => ["long",  ["ptr", "long", "long", "long", "long"]],
    "acmDriverAddW" => ["long",  ["ptr", "long", "long", "long", "long"]],
    "acmDriverClose" => ["long",  ["long", "long"]],
    "acmDriverDetailsA" => ["long",  ["long", "ptr", "long"]],
    "acmDriverDetailsW" => ["long",  ["long", "ptr", "long"]],
    "acmDriverEnum" => ["long",  ["ptr", "long", "long"]],
    "acmDriverID" => ["long",  ["long", "ptr", "long"]],
    "acmDriverMessage" => ["long",  ["long", "long", "long", "long"]],
    "acmDriverOpen" => ["long",  ["ptr", "long", "long"]],
    "acmDriverPriority" => ["long",  ["long", "long", "long"]],
    "acmDriverRemove" => ["long",  ["long", "long"]],
    "acmFilterChooseA" => ["long",  ["ptr"]],
    "acmFilterChooseW" => ["long",  ["ptr"]],
    "acmFilterDetailsA" => ["long",  ["long", "ptr", "long"]],
    "acmFilterDetailsW" => ["long",  ["long", "ptr", "long"]],
    "acmFilterEnumA" => ["long",  ["long", "ptr", "ptr", "long", "long"]],
    "acmFilterEnumW" => ["long",  ["long", "ptr", "ptr", "long", "long"]],
    "acmFilterTagDetailsA" => ["long",  ["long", "ptr", "long"]],
    "acmFilterTagDetailsW" => ["long",  ["long", "ptr", "long"]],
    "acmFilterTagEnumA" => ["long",  ["long", "ptr", "ptr", "long", "long"]],
    "acmFilterTagEnumW" => ["long",  ["long", "ptr", "ptr", "long", "long"]],
    "acmFormatChooseA" => ["long",  ["ptr"]],
    "acmFormatChooseW" => ["long",  ["ptr"]],
    "acmFormatDetailsA" => ["long",  ["long", "ptr", "long"]],
    "acmFormatDetailsW" => ["long",  ["long", "ptr", "long"]],
    "acmFormatEnumA" => ["long",  ["long", "ptr", "ptr", "long", "long"]],
    "acmFormatEnumW" => ["long",  ["long", "ptr", "ptr", "long", "long"]],
    "acmFormatSuggest" => ["long",  ["long", "ptr", "ptr", "long", "long"]],
    "acmFormatTagDetailsA" => ["long",  ["long", "ptr", "long"]],
    "acmFormatTagDetailsW" => ["long",  ["long", "ptr", "long"]],
    "acmFormatTagEnumA" => ["long",  ["long", "ptr", "ptr", "long", "long"]],
    "acmFormatTagEnumW" => ["long",  ["long", "ptr", "ptr", "long", "long"]],
    "acmGetVersion" => ["long",  []],
    "acmMetrics" => ["long",  ["long", "long", "ptr"]],
    "acmStreamClose" => ["long",  ["long", "long"]],
    "acmStreamConvert" => ["long",  ["long", "ptr", "long"]],
    "acmStreamMessage" => ["long",  ["long", "long", "long", "long"]],
    "acmStreamOpen" => ["long",  ["ptr", "long", "ptr", "ptr", "ptr", "long", "long", "long"]],
    "acmStreamPrepareHeader" => ["long",  ["long", "ptr", "long"]],
    "acmStreamReset" => ["long",  ["long", "long"]],
    "acmStreamSize" => ["long",  ["long", "long", "ptr", "long"]],
    "acmStreamUnprepareHeader" => ["long",  ["long", "ptr", "long"]]
};

&wine::declare("msacm32",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
