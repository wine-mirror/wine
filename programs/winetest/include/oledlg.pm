package oledlg;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "OleUIAddVerbMenuA" => ["long",  ["ptr", "str", "long", "long", "long", "long", "long", "long", "ptr"]],
    "OleUICanConvertOrActivateAs" => ["long",  ["ptr", "long", "long"]],
    "OleUIInsertObjectA" => ["long",  ["ptr"]],
    "OleUIPasteSpecialA" => ["long",  ["ptr"]],
    "OleUIEditLinksA" => ["long",  ["ptr"]],
    "OleUIChangeIconA" => ["long",  ["ptr"]],
    "OleUIConvertA" => ["long",  ["ptr"]],
    "OleUIBusyA" => ["long",  ["ptr"]],
    "OleUIUpdateLinksA" => ["long",  ["ptr", "long", "str", "long"]],
    "OleUIObjectPropertiesA" => ["long",  ["ptr"]],
    "OleUIChangeSourceA" => ["long",  ["ptr"]],
    "OleUIAddVerbMenuW" => ["long",  ["ptr", "wstr", "long", "long", "long", "long", "long", "long", "ptr"]],
    "OleUIBusyW" => ["long",  ["ptr"]],
    "OleUIChangeIconW" => ["long",  ["ptr"]],
    "OleUIChangeSourceW" => ["long",  ["ptr"]],
    "OleUIConvertW" => ["long",  ["ptr"]],
    "OleUIEditLinksW" => ["long",  ["ptr"]],
    "OleUIInsertObjectW" => ["long",  ["ptr"]],
    "OleUIObjectPropertiesW" => ["long",  ["ptr"]],
    "OleUIPasteSpecialW" => ["long",  ["ptr"]],
    "OleUIUpdateLinksW" => ["long",  ["ptr", "long", "wstr", "long"]]
};

&wine::declare("oledlg",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
