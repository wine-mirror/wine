package winspool_drv;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "AddFormA" => ["long",  ["long", "long", "ptr"]],
    "AddFormW" => ["long",  ["long", "long", "ptr"]],
    "AddJobA" => ["long",  ["long", "long", "ptr", "long", "ptr"]],
    "AddJobW" => ["long",  ["long", "long", "ptr", "long", "ptr"]],
    "AddMonitorA" => ["long",  ["str", "long", "ptr"]],
    "AddPrinterA" => ["long",  ["str", "long", "ptr"]],
    "AddPrinterDriverA" => ["long",  ["str", "long", "ptr"]],
    "AddPrinterDriverW" => ["long",  ["wstr", "long", "ptr"]],
    "AddPrinterW" => ["long",  ["wstr", "long", "ptr"]],
    "ClosePrinter" => ["long",  ["long"]],
    "DeleteFormA" => ["long",  ["long", "str"]],
    "DeleteFormW" => ["long",  ["long", "wstr"]],
    "DeleteMonitorA" => ["long",  ["str", "str", "str"]],
    "DeletePortA" => ["long",  ["str", "long", "str"]],
    "DeletePrinter" => ["long",  ["long"]],
    "DeletePrinterDriverA" => ["long",  ["str", "str", "str"]],
    "DeviceCapabilities" => ["long",  ["str", "str", "long", "str", "ptr"]],
    "DeviceCapabilitiesA" => ["long",  ["str", "str", "long", "str", "ptr"]],
    "DeviceCapabilitiesW" => ["long",  ["wstr", "wstr", "long", "wstr", "ptr"]],
    "DocumentPropertiesA" => ["long",  ["long", "long", "str", "ptr", "ptr", "long"]],
    "DocumentPropertiesW" => ["long",  ["long", "long", "wstr", "ptr", "ptr", "long"]],
    "EnumJobsA" => ["long",  ["long", "long", "long", "long", "ptr", "long", "ptr", "ptr"]],
    "EnumJobsW" => ["long",  ["long", "long", "long", "long", "ptr", "long", "ptr", "ptr"]],
    "EnumPortsA" => ["long",  ["str", "long", "ptr", "long", "ptr", "ptr"]],
    "EnumPrinterDataExA" => ["long",  ["long", "str", "ptr", "long", "ptr", "ptr"]],
    "EnumPrinterDataExW" => ["long",  ["long", "wstr", "ptr", "long", "ptr", "ptr"]],
    "EnumPrinterDriversA" => ["long",  ["str", "str", "long", "ptr", "long", "ptr", "ptr"]],
    "EnumPrinterDriversW" => ["long",  ["wstr", "wstr", "long", "ptr", "long", "ptr", "ptr"]],
    "EnumPrintersA" => ["long",  ["long", "str", "long", "ptr", "long", "ptr", "ptr"]],
    "EnumPrintersW" => ["long",  ["long", "wstr", "long", "ptr", "long", "ptr", "ptr"]],
    "GetDefaultPrinterA" => ["long",  ["str", "ptr"]],
    "GetDefaultPrinterW" => ["long",  ["wstr", "ptr"]],
    "GetFormA" => ["long",  ["long", "str", "long", "ptr", "long", "ptr"]],
    "GetFormW" => ["long",  ["long", "wstr", "long", "ptr", "long", "ptr"]],
    "GetPrinterA" => ["long",  ["long", "long", "ptr", "long", "ptr"]],
    "GetPrinterDataA" => ["long",  ["long", "str", "ptr", "ptr", "long", "ptr"]],
    "GetPrinterDataExA" => ["long",  ["long", "str", "str", "ptr", "ptr", "long", "ptr"]],
    "GetPrinterDataExW" => ["long",  ["long", "wstr", "wstr", "ptr", "ptr", "long", "ptr"]],
    "GetPrinterDataW" => ["long",  ["long", "wstr", "ptr", "ptr", "long", "ptr"]],
    "GetPrinterDriverA" => ["long",  ["long", "str", "long", "ptr", "long", "ptr"]],
    "GetPrinterDriverDirectoryA" => ["long",  ["str", "str", "long", "ptr", "long", "ptr"]],
    "GetPrinterDriverDirectoryW" => ["long",  ["wstr", "wstr", "long", "ptr", "long", "ptr"]],
    "GetPrinterDriverW" => ["long",  ["long", "wstr", "long", "ptr", "long", "ptr"]],
    "GetPrinterW" => ["long",  ["long", "long", "ptr", "long", "ptr"]],
    "OpenPrinterA" => ["long",  ["str", "ptr", "ptr"]],
    "OpenPrinterW" => ["long",  ["wstr", "ptr", "ptr"]],
    "PrinterProperties" => ["long",  ["long", "long"]],
    "ReadPrinter" => ["long",  ["long", "ptr", "long", "ptr"]],
    "ResetPrinterA" => ["long",  ["long", "ptr"]],
    "ResetPrinterW" => ["long",  ["long", "ptr"]],
    "SetFormA" => ["long",  ["long", "str", "long", "ptr"]],
    "SetFormW" => ["long",  ["long", "wstr", "long", "ptr"]],
    "SetJobA" => ["long",  ["long", "long", "long", "ptr", "long"]],
    "SetJobW" => ["long",  ["long", "long", "long", "ptr", "long"]],
    "SetPrinterA" => ["long",  ["long", "long", "ptr", "long"]],
    "SetPrinterDataA" => ["long",  ["long", "str", "long", "ptr", "long"]],
    "SetPrinterDataExA" => ["long",  ["long", "str", "str", "long", "ptr", "long"]],
    "SetPrinterDataExW" => ["long",  ["long", "wstr", "wstr", "long", "ptr", "long"]],
    "SetPrinterDataW" => ["long",  ["long", "wstr", "long", "ptr", "long"]],
    "SetPrinterW" => ["long",  ["long", "long", "ptr", "long"]],
    "WritePrinter" => ["long",  ["long", "ptr", "long", "ptr"]]
};

&wine::declare("winspool.drv",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
