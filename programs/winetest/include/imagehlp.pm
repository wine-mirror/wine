package imagehlp;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "BindImage" => ["long",  ["str", "str", "str"]],
    "BindImageEx" => ["long",  ["long", "str", "str", "str", "ptr"]],
    "CheckSumMappedFile" => ["ptr",  ["ptr", "long", "ptr", "ptr"]],
    "EnumerateLoadedModules" => ["long",  ["long", "ptr", "ptr"]],
    "FindDebugInfoFile" => ["long",  ["str", "str", "str"]],
    "FindExecutableImage" => ["long",  ["str", "str", "str"]],
    "GetImageConfigInformation" => ["long",  ["ptr", "ptr"]],
    "GetImageUnusedHeaderBytes" => ["long",  ["ptr", "ptr"]],
    "GetTimestampForLoadedLibrary" => ["long",  ["long"]],
    "ImageAddCertificate" => ["long",  ["long", "ptr", "ptr"]],
    "ImageDirectoryEntryToData" => ["ptr",  ["ptr", "long", "long", "ptr"]],
    "ImageEnumerateCertificates" => ["long",  ["long", "long", "ptr", "ptr", "long"]],
    "ImageGetCertificateData" => ["long",  ["long", "long", "ptr", "ptr"]],
    "ImageGetCertificateHeader" => ["long",  ["long", "long", "ptr"]],
    "ImageGetDigestStream" => ["long",  ["long", "long", "ptr", "ptr"]],
    "ImageLoad" => ["ptr",  ["str", "str"]],
    "ImageNtHeader" => ["ptr",  ["ptr"]],
    "ImageRemoveCertificate" => ["long",  ["long", "long"]],
    "ImageRvaToSection" => ["ptr",  ["ptr", "ptr", "long"]],
    "ImageRvaToVa" => ["ptr",  ["ptr", "ptr", "long", "ptr"]],
    "ImageUnload" => ["long",  ["ptr"]],
    "ImagehlpApiVersion" => ["ptr",  ["undef"]],
    "ImagehlpApiVersionEx" => ["ptr",  ["ptr"]],
    "MakeSureDirectoryPathExists" => ["long",  ["str"]],
    "MapAndLoad" => ["long",  ["str", "str", "ptr", "long", "long"]],
    "MapDebugInformation" => ["ptr",  ["long", "str", "str", "long"]],
    "MapFileAndCheckSumA" => ["long",  ["str", "ptr", "ptr"]],
    "MapFileAndCheckSumW" => ["long",  ["wstr", "ptr", "ptr"]],
    "ReBaseImage" => ["long",  ["str", "str", "long", "long", "long", "long", "ptr", "ptr", "ptr", "ptr", "long"]],
    "RemovePrivateCvSymbolic" => ["long",  ["ptr", "ptr", "ptr"]],
    "RemoveRelocations" => ["void",  ["ptr"]],
    "SearchTreeForFile" => ["long",  ["str", "str", "str"]],
    "SetImageConfigInformation" => ["long",  ["ptr", "ptr"]],
    "SplitSymbols" => ["long",  ["str", "str", "str", "long"]],
    "StackWalk" => ["long",  ["long", "long", "long", "ptr", "ptr", "ptr", "ptr", "ptr", "ptr"]],
    "SymCleanup" => ["long",  ["long"]],
    "SymEnumerateModules" => ["long",  ["long", "ptr", "ptr"]],
    "SymEnumerateSymbols" => ["long",  ["long", "long", "ptr", "ptr"]],
    "SymFunctionTableAccess" => ["ptr",  ["long", "long"]],
    "SymGetModuleBase" => ["long",  ["long", "long"]],
    "SymGetModuleInfo" => ["long",  ["long", "long", "ptr"]],
    "SymGetOptions" => ["long",  ["undef"]],
    "SymGetSearchPath" => ["long",  ["long", "str", "long"]],
    "SymGetSymFromAddr" => ["long",  ["long", "long", "ptr", "ptr"]],
    "SymGetSymFromName" => ["long",  ["long", "str", "ptr"]],
    "SymGetSymNext" => ["long",  ["long", "ptr"]],
    "SymGetSymPrev" => ["long",  ["long", "ptr"]],
    "SymInitialize" => ["long",  ["long", "str", "long"]],
    "SymLoadModule" => ["long",  ["long", "long", "str", "str", "long", "long"]],
    "SymRegisterCallback" => ["long",  ["long", "ptr", "ptr"]],
    "SymSetOptions" => ["long",  ["long"]],
    "SymSetSearchPath" => ["long",  ["long", "str"]],
    "SymUnDName" => ["long",  ["ptr", "str", "long"]],
    "SymUnloadModule" => ["long",  ["long", "long"]],
    "TouchFileTimes" => ["long",  ["long", "ptr"]],
    "UnDecorateSymbolName" => ["long",  ["str", "str", "long", "long"]],
    "UnMapAndLoad" => ["long",  ["ptr"]],
    "UnmapDebugInformation" => ["long",  ["ptr"]],
    "UpdateDebugInfoFile" => ["long",  ["str", "str", "str", "ptr"]],
    "UpdateDebugInfoFileEx" => ["long",  ["str", "str", "str", "ptr", "long"]]
};

&wine::declare("imagehlp",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
