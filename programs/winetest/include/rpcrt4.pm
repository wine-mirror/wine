package rpcrt4;

use strict;

require Exporter;

use wine;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

my $module_declarations = {
    "NdrDllRegisterProxy" => ["long",  ["long", "ptr", "ptr"]],
    "RpcBindingFromStringBindingA" => ["long",  ["str", "ptr"]],
    "RpcBindingFromStringBindingW" => ["long",  ["wstr", "ptr"]],
    "RpcServerListen" => ["long",  ["long", "long", "long"]],
    "RpcServerRegisterAuthInfoA" => ["long",  ["str", "long", "ptr", "ptr"]],
    "RpcServerRegisterAuthInfoW" => ["long",  ["wstr", "long", "ptr", "ptr"]],
    "RpcServerRegisterIf" => ["long",  ["long", "ptr", "ptr"]],
    "RpcServerRegisterIf2" => ["long",  ["long", "ptr", "ptr", "long", "long", "long", "ptr"]],
    "RpcServerRegisterIfEx" => ["long",  ["long", "ptr", "ptr", "long", "long", "ptr"]],
    "RpcServerUseProtseqEpA" => ["long",  ["str", "long", "str", "ptr"]],
    "RpcServerUseProtseqEpExA" => ["long",  ["str", "long", "str", "ptr", "ptr"]],
    "RpcServerUseProtseqEpExW" => ["long",  ["wstr", "long", "wstr", "ptr", "ptr"]],
    "RpcServerUseProtseqEpW" => ["long",  ["wstr", "long", "wstr", "ptr"]],
    "RpcStringBindingComposeA" => ["long",  ["str", "str", "str", "str", "str", "ptr"]],
    "RpcStringBindingComposeW" => ["long",  ["wstr", "wstr", "wstr", "wstr", "wstr", "ptr"]],
    "RpcStringFreeA" => ["long",  ["ptr"]],
    "UuidCreate" => ["long",  ["ptr"]],
    "UuidCreateSequential" => ["long",  ["ptr"]],
    "UuidHash" => ["ptr",  ["ptr", "ptr"]],
    "UuidToStringA" => ["long",  ["ptr", "ptr"]]
};

&wine::declare("rpcrt4",%$module_declarations);
push @EXPORT, map { "&" . $_; } sort(keys(%$module_declarations));
1;
