package avifil32;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "AVIFileAddRef" => ["long",  ["ptr"]],
    "AVIFileCreateStreamA" => ["long",  ["ptr", "ptr", "ptr"]],
    "AVIFileCreateStreamW" => ["long",  ["ptr", "ptr", "ptr"]],
    "AVIFileEndRecord" => ["long",  ["ptr"]],
    "AVIFileExit" => ["void",  []],
    "AVIFileGetStream" => ["long",  ["ptr", "ptr", "long", "long"]],
    "AVIFileInfo" => ["long",  ["ptr", "ptr", "long"]],
    "AVIFileInfoA" => ["long",  ["ptr", "ptr", "long"]],
    "AVIFileInfoW" => ["long",  ["ptr", "ptr", "long"]],
    "AVIFileInit" => ["void",  []],
    "AVIFileOpenA" => ["long",  ["ptr", "str", "long", "ptr"]],
    "AVIFileOpenW" => ["long",  ["ptr", "wstr", "long", "ptr"]],
    "AVIFileReadData" => ["long",  ["ptr", "long", "ptr", "ptr"]],
    "AVIFileRelease" => ["long",  ["ptr"]],
    "AVIFileWriteData" => ["long",  ["ptr", "long", "ptr", "long"]],
    "AVIMakeCompressedStream" => ["long",  ["ptr", "ptr", "ptr", "ptr"]],
    "AVIStreamAddRef" => ["long",  ["ptr"]],
    "AVIStreamBeginStreaming" => ["long",  ["ptr", "long", "long", "long"]],
    "AVIStreamCreate" => ["long",  ["ptr", "long", "long", "ptr"]],
    "AVIStreamEndStreaming" => ["long",  ["ptr"]],
    "AVIStreamFindSample" => ["long",  ["ptr", "long", "long"]],
    "AVIStreamGetFrame" => ["ptr",  ["ptr", "long"]],
    "AVIStreamGetFrameClose" => ["long",  ["ptr"]],
    "AVIStreamGetFrameOpen" => ["ptr",  ["ptr", "ptr"]],
    "AVIStreamInfo" => ["long",  ["ptr", "ptr", "long"]],
    "AVIStreamInfoA" => ["long",  ["ptr", "ptr", "long"]],
    "AVIStreamInfoW" => ["long",  ["ptr", "ptr", "long"]],
    "AVIStreamLength" => ["long",  ["ptr"]],
    "AVIStreamOpenFromFileA" => ["long",  ["ptr", "str", "long", "long", "long", "ptr"]],
    "AVIStreamOpenFromFileW" => ["long",  ["ptr", "wstr", "long", "long", "long", "ptr"]],
    "AVIStreamRead" => ["long",  ["ptr", "long", "long", "ptr", "long", "ptr", "ptr"]],
    "AVIStreamReadData" => ["long",  ["ptr", "long", "ptr", "ptr"]],
    "AVIStreamReadFormat" => ["long",  ["ptr", "long", "ptr", "ptr"]],
    "AVIStreamRelease" => ["long",  ["ptr"]],
    "AVIStreamSampleToTime" => ["long",  ["ptr", "long"]],
    "AVIStreamSetFormat" => ["long",  ["ptr", "long", "ptr", "long"]],
    "AVIStreamStart" => ["long",  ["ptr"]],
    "AVIStreamTimeToSample" => ["long",  ["ptr", "long"]],
    "AVIStreamWrite" => ["long",  ["ptr", "long", "long", "ptr", "long", "long", "ptr", "ptr"]],
    "AVIStreamWriteData" => ["long",  ["ptr", "long", "ptr", "long"]],
    "DllCanUnloadNow" => ["long",  []],
    "DllGetClassObject" => ["long",  ["ptr", "ptr", "ptr"]]
};

&wine::declare("avifil32",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
