package imm32;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "ImmAssociateContext" => ["long",  ["long", "long"]],
    "ImmConfigureIMEA" => ["long",  ["long", "long", "long", "ptr"]],
    "ImmConfigureIMEW" => ["long",  ["long", "long", "long", "ptr"]],
    "ImmCreateContext" => ["long",  []],
    "ImmCreateIMCC" => ["long",  ["long"]],
    "ImmCreateSoftKeyboard" => ["long",  ["long", "long", "long", "long"]],
    "ImmDestroyContext" => ["long",  ["long"]],
    "ImmDestroyIMCC" => ["long",  ["long"]],
    "ImmDestroySoftKeyboard" => ["long",  ["long"]],
    "ImmEnumRegisterWordA" => ["long",  ["long", "ptr", "str", "long", "str", "ptr"]],
    "ImmEnumRegisterWordW" => ["long",  ["long", "ptr", "wstr", "long", "wstr", "ptr"]],
    "ImmEscapeA" => ["long",  ["long", "long", "long", "ptr"]],
    "ImmEscapeW" => ["long",  ["long", "long", "long", "ptr"]],
    "ImmGenerateMessage" => ["long",  ["long"]],
    "ImmGetCandidateListA" => ["long",  ["long", "long", "ptr", "long"]],
    "ImmGetCandidateListCountA" => ["long",  ["long", "ptr"]],
    "ImmGetCandidateListCountW" => ["long",  ["long", "ptr"]],
    "ImmGetCandidateListW" => ["long",  ["long", "long", "ptr", "long"]],
    "ImmGetCandidateWindow" => ["long",  ["long", "long", "ptr"]],
    "ImmGetCompositionFontA" => ["long",  ["long", "ptr"]],
    "ImmGetCompositionFontW" => ["long",  ["long", "ptr"]],
    "ImmGetCompositionStringA" => ["long",  ["long", "long", "ptr", "long"]],
    "ImmGetCompositionStringW" => ["long",  ["long", "long", "ptr", "long"]],
    "ImmGetCompositionWindow" => ["long",  ["long", "ptr"]],
    "ImmGetContext" => ["long",  ["long"]],
    "ImmGetConversionListA" => ["long",  ["long", "long", "str", "ptr", "long", "long"]],
    "ImmGetConversionListW" => ["long",  ["long", "long", "wstr", "ptr", "long", "long"]],
    "ImmGetConversionStatus" => ["long",  ["long", "ptr", "ptr"]],
    "ImmGetDefaultIMEWnd" => ["long",  ["long"]],
    "ImmGetDescriptionA" => ["long",  ["long", "str", "long"]],
    "ImmGetDescriptionW" => ["long",  ["long", "wstr", "long"]],
    "ImmGetGuideLineA" => ["long",  ["long", "long", "str", "long"]],
    "ImmGetGuideLineW" => ["long",  ["long", "long", "wstr", "long"]],
    "ImmGetHotKey" => ["long",  ["long", "ptr", "ptr", "ptr"]],
    "ImmGetIMCCLockCount" => ["long",  ["long"]],
    "ImmGetIMCCSize" => ["long",  ["long"]],
    "ImmGetIMCLockCount" => ["long",  ["long"]],
    "ImmGetIMEFileNameA" => ["long",  ["long", "str", "long"]],
    "ImmGetIMEFileNameW" => ["long",  ["long", "wstr", "long"]],
    "ImmGetOpenStatus" => ["long",  ["long"]],
    "ImmGetProperty" => ["long",  ["long", "long"]],
    "ImmGetRegisterWordStyleA" => ["long",  ["long", "long", "ptr"]],
    "ImmGetRegisterWordStyleW" => ["long",  ["long", "long", "ptr"]],
    "ImmGetStatusWindowPos" => ["long",  ["long", "ptr"]],
    "ImmGetVirtualKey" => ["long",  ["long"]],
    "ImmInstallIMEA" => ["long",  ["str", "str"]],
    "ImmInstallIMEW" => ["long",  ["wstr", "wstr"]],
    "ImmIsIME" => ["long",  ["long"]],
    "ImmIsUIMessageA" => ["long",  ["long", "long", "long", "long"]],
    "ImmIsUIMessageW" => ["long",  ["long", "long", "long", "long"]],
    "ImmLockIMC" => ["ptr",  ["long"]],
    "ImmLockIMCC" => ["ptr",  ["long"]],
    "ImmNotifyIME" => ["long",  ["long", "long", "long", "long"]],
    "ImmReSizeIMCC" => ["long",  ["long", "long"]],
    "ImmRegisterWordA" => ["long",  ["long", "str", "long", "str"]],
    "ImmRegisterWordW" => ["long",  ["long", "wstr", "long", "wstr"]],
    "ImmReleaseContext" => ["long",  ["long", "long"]],
    "ImmSetCandidateWindow" => ["long",  ["long", "ptr"]],
    "ImmSetCompositionFontA" => ["long",  ["long", "ptr"]],
    "ImmSetCompositionFontW" => ["long",  ["long", "ptr"]],
    "ImmSetCompositionStringA" => ["long",  ["long", "long", "ptr", "long", "ptr", "long"]],
    "ImmSetCompositionStringW" => ["long",  ["long", "long", "ptr", "long", "ptr", "long"]],
    "ImmSetCompositionWindow" => ["long",  ["long", "ptr"]],
    "ImmSetConversionStatus" => ["long",  ["long", "long", "long"]],
    "ImmSetHotKey" => ["long",  ["long", "long", "long", "long"]],
    "ImmSetOpenStatus" => ["long",  ["long", "long"]],
    "ImmSetStatusWindowPos" => ["long",  ["long", "ptr"]],
    "ImmShowSoftKeyboard" => ["long",  ["long", "long"]],
    "ImmSimulateHotKey" => ["long",  ["long", "long"]],
    "ImmUnlockIMC" => ["long",  ["long"]],
    "ImmUnlockIMCC" => ["long",  ["long"]],
    "ImmUnregisterWordA" => ["long",  ["long", "str", "long", "str"]],
    "ImmUnregisterWordW" => ["long",  ["long", "wstr", "long", "wstr"]]
};

&wine::declare("imm32",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
