package comdlg32;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "ChooseColorA" => ["long",  ["ptr"]],
    "ChooseColorW" => ["long",  ["ptr"]],
    "ChooseFontA" => ["long",  ["ptr"]],
    "ChooseFontW" => ["long",  ["ptr"]],
    "CommDlgExtendedError" => ["long",  []],
    "FindTextA" => ["long",  ["ptr"]],
    "FindTextW" => ["long",  ["ptr"]],
    "GetFileTitleA" => ["long",  ["str", "str", "long"]],
    "GetFileTitleW" => ["long",  ["wstr", "wstr", "long"]],
    "GetOpenFileNameA" => ["long",  ["ptr"]],
    "GetOpenFileNameW" => ["long",  ["ptr"]],
    "GetSaveFileNameA" => ["long",  ["ptr"]],
    "GetSaveFileNameW" => ["long",  ["ptr"]],
    "PageSetupDlgA" => ["long",  ["ptr"]],
    "PageSetupDlgW" => ["long",  ["ptr"]],
    "PrintDlgA" => ["long",  ["ptr"]],
    "PrintDlgExA" => ["long",  ["ptr"]],
    "PrintDlgExW" => ["long",  ["ptr"]],
    "PrintDlgW" => ["long",  ["ptr"]],
    "ReplaceTextA" => ["long",  ["ptr"]],
    "ReplaceTextW" => ["long",  ["ptr"]]
};

&wine::declare("comdlg32",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
