package dsound;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "DirectSoundCreate" => ["long",  ["ptr", "ptr", "ptr"]],
    "DirectSoundEnumerateA" => ["long",  ["ptr", "ptr"]],
    "DirectSoundEnumerateW" => ["long",  ["ptr", "ptr"]],
    "DllCanUnloadNow" => ["long",  []],
    "DllGetClassObject" => ["long",  ["ptr", "ptr", "ptr"]],
    "DirectSoundCaptureCreate" => ["long",  ["ptr", "ptr", "ptr"]],
    "DirectSoundCaptureEnumerateA" => ["long",  ["ptr", "ptr"]],
    "DirectSoundCaptureEnumerateW" => ["long",  ["ptr", "ptr"]]
};

&wine::declare("dsound",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
