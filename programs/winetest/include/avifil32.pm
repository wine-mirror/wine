package avifil32;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "AVIFileCreateStreamA" => ["long",  ["ptr", "ptr", "ptr"]],
    "AVIFileCreateStreamW" => ["long",  ["ptr", "ptr", "ptr"]],
    "AVIFileExit" => ["void",  []],
    "AVIFileGetStream" => ["long",  ["ptr", "ptr", "long", "long"]],
    "AVIFileInfoA" => ["long",  ["ptr", "ptr", "long"]],
    "AVIFileInfoW" => ["long",  ["ptr", "ptr", "long"]],
    "AVIFileInit" => ["void",  []],
    "AVIFileOpenA" => ["long",  ["ptr", "str", "long", "ptr"]],
    "AVIFileRelease" => ["long",  ["ptr"]],
    "AVIMakeCompressedStream" => ["long",  ["ptr", "ptr", "ptr", "ptr"]],
    "AVIStreamGetFrame" => ["ptr",  ["ptr", "long"]],
    "AVIStreamGetFrameClose" => ["long",  ["ptr"]],
    "AVIStreamGetFrameOpen" => ["ptr",  ["ptr", "ptr"]],
    "AVIStreamInfoA" => ["long",  ["ptr", "ptr", "long"]],
    "AVIStreamInfoW" => ["long",  ["ptr", "ptr", "long"]],
    "AVIStreamLength" => ["long",  ["ptr"]],
    "AVIStreamRead" => ["long",  ["ptr", "long", "long", "ptr", "long", "ptr", "ptr"]],
    "AVIStreamReadData" => ["long",  ["ptr", "long", "ptr", "ptr"]],
    "AVIStreamReadFormat" => ["long",  ["ptr", "long", "ptr", "ptr"]],
    "AVIStreamRelease" => ["long",  ["ptr"]],
    "AVIStreamSetFormat" => ["long",  ["ptr", "long", "ptr", "long"]],
    "AVIStreamStart" => ["long",  ["ptr"]],
    "AVIStreamWrite" => ["long",  ["ptr", "long", "long", "ptr", "long", "long", "ptr", "ptr"]],
    "AVIStreamWriteData" => ["long",  ["ptr", "long", "ptr", "long"]]
};

&wine::declare("avifil32",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
