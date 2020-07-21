#
# Copyright 1999, 2000, 2001 Patrik Stridvall
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
#

package winapi_function;

use strict;
use warnings 'all';

use base qw(function);

use config qw($current_dir $wine_dir);
use util qw(normalize_set);

my $import = 0;
use vars qw($modules $win16api $win32api @winapis);

########################################################################
# constructor
#

sub new($) {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    if (!$import) {
	require modules;
	import modules qw($modules);
	
	require winapi;
	import winapi qw($win16api $win32api @winapis);

	$import = 1;
    }
    return $self;
}

########################################################################
# is_win
#

sub is_win16($) { my $self = shift; return defined($self->_module($win16api, @_)); }
sub is_win32($) { my $self = shift; return defined($self->_module($win32api, @_)); }

########################################################################
# external_name
#

sub _external_name($$) {
    my $self = shift;
    my $winapi = shift;

    my $file = $self->file;
    my $internal_name = $self->internal_name;

    my $external_name = $winapi->function_external_name($internal_name);
    my $module = $winapi->function_internal_module($internal_name);

    if(!defined($external_name) && !defined($module)) {
	return undef;
    }

    my @external_names = split(/\s*&\s*/, $external_name);
    my @modules = split(/\s*&\s*/, $module);

    my @external_names2;
    while(defined(my $external_name = shift @external_names) &&
	  defined(my $module = shift @modules))
    {
	if($modules->is_allowed_module_in_file($module, "$current_dir/$file")) {
	    push @external_names2, $external_name;
	}
    }

    return join(" & ", @external_names2);
}

sub _external_names($$) {
    my $self = shift;
    my $winapi = shift;

    my $external_name = $self->_external_name($winapi);

    if(defined($external_name)) {
	return split(/\s*&\s*/, $external_name);
    } else {
	return ();
    }
}

sub external_name($) {
    my $self = shift;

    foreach my $winapi (@winapis) {
	my $external_name = $self->_external_name($winapi, @_);

	if(defined($external_name)) {
	    return $external_name;
	}
    }

    return undef;
}

sub external_name16($) { my $self = shift; return $self->_external_name($win16api, @_); }
sub external_name32($) { my $self = shift; return $self->_external_name($win32api, @_); }

sub external_names16($) { my $self = shift; return $self->_external_names($win16api, @_); }
sub external_names32($) { my $self = shift; return $self->_external_names($win32api, @_); }

sub external_names($) { my $self = shift; return ($self->external_names16, $self->external_names32); }

########################################################################
# module
#

sub _module($$) {
    my $self = shift;
    my $winapi = shift;

    my $file = $self->file;
    my $internal_name = $self->internal_name;

    my $module = $winapi->function_internal_module($internal_name);
    if(!defined($module)) {
	return undef;
    }

    if(!defined($file)) {
	return undef;
    }

    my @modules;
    foreach my $module (split(/\s*&\s*/, $module)) {
	if($modules->is_allowed_module_in_file($module, "$current_dir/$file")) {
	    push @modules, $module;
	}
    }

    return join(" & ", @modules);
}

sub _modules($$) {
    my $self = shift;
    my $winapi = shift;

    my $module = $self->_module($winapi);

    if(defined($module)) {
	return split(/\s*&\s*/, $module);
    } else {
	return ();
    }
}

sub module16($) { my $self = shift; return $self->_module($win16api, @_); }
sub module32($) { my $self = shift; return $self->_module($win32api, @_); }

sub module($) { my $self = shift; return join (" & ", $self->modules); }

sub modules16($) { my $self = shift; return $self->_modules($win16api, @_); }
sub modules32($) { my $self = shift; return $self->_modules($win32api, @_); }

sub modules($) { my $self = shift; return ($self->modules16, $self->modules32); }

########################################################################
# ordinal
#

sub _ordinal($$) {
    my $self = shift;
    my $winapi = shift;

    my $file = $self->file;
    my $internal_name = $self->internal_name;

    my $ordinal = $winapi->function_internal_ordinal($internal_name);
    my $module = $winapi->function_internal_module($internal_name);

    if(!defined($ordinal) && !defined($module)) {
	return undef;
    }

    my @ordinals = split(/\s*&\s*/, $ordinal);
    my @modules = split(/\s*&\s*/, $module);

    my @ordinals2;
    while(defined(my $ordinal = shift @ordinals) &&
	  defined(my $module = shift @modules))
    {
	if($modules->is_allowed_module_in_file($module, "$current_dir/$file")) {
	    push @ordinals2, $ordinal;
	}
    }

    return join(" & ", @ordinals2);
}

sub _ordinals($$) {
    my $self = shift;
    my $winapi = shift;

    my $ordinal = $self->_ordinal($winapi);

    if(defined($ordinal)) {
	return split(/\s*&\s*/, $ordinal);
    } else {
	return ();
    }
}

sub ordinal16($) { my $self = shift; return $self->_ordinal($win16api, @_); }
sub ordinal32($) { my $self = shift; return $self->_ordinal($win32api, @_); }

sub ordinal($) { my $self = shift; return join (" & ", $self->ordinals); }

sub ordinals16($) { my $self = shift; return $self->_ordinals($win16api, @_); }
sub ordinals32($) { my $self = shift; return $self->_ordinals($win32api, @_); }

sub ordinals($) { my $self = shift; return ($self->ordinals16, $self->ordinals32); }

########################################################################
# prefix
#

sub prefix($) {
    my $self = shift;
    my $module16 = $self->module16;
    my $module32 = $self->module32;

    my $file = $self->file;
    my $function_line = $self->function_line;
    my $return_type = $self->return_type;
    my $internal_name = $self->internal_name;
    my $calling_convention = $self->calling_convention;

    my $refargument_types = $self->argument_types;
    my @argument_types = ();
    if(defined($refargument_types)) {
	@argument_types = @$refargument_types;
	if($#argument_types < 0) {
	    @argument_types = ("void");
	}
    }

    my $prefix = "";

    my @modules = ();
    my %used;
    foreach my $module ($self->modules) {
	if($used{$module}) { next; }
	push @modules, $module;
	$used{$module}++;
    }
    $prefix .= "$file:";
    if(defined($function_line)) {
	$prefix .= "$function_line: ";
    } else {
	$prefix .= "<>: ";
    }
    if($#modules >= 0) {
	$prefix .= join(" & ", @modules) . ": ";
    } else {
	$prefix .= "<>: ";
    }
    $prefix .= "$return_type ";
    $prefix .= "$calling_convention " if $calling_convention;
    $prefix .= "$internal_name(" . join(",", @argument_types) . "): ";

    return $prefix;
}

########################################################################
# calling_convention
#

sub calling_convention16($) {
    my $self = shift;
    my $return_kind16 = $self->return_kind16;

    my $suffix;
    if(!defined($return_kind16)) {
	$suffix = undef;
    } elsif($return_kind16 =~ /^(?:void|s_word|word)$/) {
	$suffix = "16";
    } elsif($return_kind16 =~ /^(?:long|ptr|segptr|segstr|str|wstr)$/) {
	$suffix = "";
    } else {
	$suffix = undef;
    }

    local $_ = $self->calling_convention;
    if($_ eq "__cdecl") {
	return "cdecl";
    } elsif(/^(?:VFWAPIV|WINAPIV)$/) {
	if(!defined($suffix)) { return undef; }
	return "pascal$suffix"; # FIXME: Is this correct?
    } elsif(/^(?:__stdcall|__RPC_STUB|__RPC_USER|NET_API_FUNCTION|RPC_ENTRY|SEC_ENTRY|VFWAPI|WINGDIPAPI|WMIAPI|WINAPI|CALLBACK)$/) {
	if(!defined($suffix)) { return undef; }
	return "pascal$suffix";
    } elsif($_ eq "__asm") {
	return "asm";
    } else {
	return "cdecl";
    }
}

sub calling_convention32($) {
    my $self = shift;

    local $_ = $self->calling_convention;
    if($_ eq "__cdecl") {
	return "cdecl";
    } elsif(/^(?:VFWAPIV|WINAPIV)$/) {
	return "varargs";
    } elsif(/^(?:__stdcall|__RPC_STUB|__RPC_USER|NET_API_FUNCTION|RPC_ENTRY|SEC_ENTRY|VFWAPI|WINGDIPAPI|WMIAPI|WINAPI|CALLBACK)$/) {
	return "stdcall";
    } elsif($_ eq "__asm") {
	return "asm";
    } else {
	return "cdecl";
    }
}

sub get_all_module_ordinal16($) {
    my $self = shift;
    my $internal_name = $self->internal_name;

    return winapi::get_all_module_internal_ordinal16($internal_name);
}

sub get_all_module_ordinal32($) {
    my $self = shift;
    my $internal_name = $self->internal_name;

    return winapi::get_all_module_internal_ordinal32($internal_name);
}

sub get_all_module_ordinal($) {
    my $self = shift;
    my $internal_name = $self->internal_name;

    return winapi::get_all_module_internal_ordinal($internal_name);
}

sub _return_kind($$) {
    my $self = shift;
    my $winapi = shift;
    my $return_type = $self->return_type;

    return $winapi->translate_argument($return_type);
}

sub return_kind16($) {
    my $self = shift; return $self->_return_kind($win16api, @_);
}

sub return_kind32($) {
    my $self = shift; return $self->_return_kind($win32api, @_);
}

sub _argument_kinds($$) {
    my $self = shift;
    my $winapi = shift;
    my $refargument_types = $self->argument_types;

    if(!defined($refargument_types)) {
	return undef;
    }

    my @argument_kinds;
    foreach my $argument_type (@$refargument_types) {
	my $argument_kind = $winapi->translate_argument($argument_type);

	if(defined($argument_kind) && $argument_kind eq "longlong") {
	    push @argument_kinds, "double";
	} else {
	    push @argument_kinds, $argument_kind;
	}
    }

    return [@argument_kinds];
}

sub argument_kinds16($) {
    my $self = shift; return $self->_argument_kinds($win16api, @_);
}

sub argument_kinds32($) {
    my $self = shift; return $self->_argument_kinds($win32api, @_);
}

##############################################################################
# Accounting
#

sub function_called($$) {
    my $self = shift;
    my $called_function_names = \%{$self->{CALLED_FUNCTION_NAMES}};

    my $name = shift;

    $$called_function_names{$name}++;
}

sub function_called_by($$) {
   my $self = shift;
   my $called_by_function_names = \%{$self->{CALLED_BY_FUNCTION_NAMES}};

   my $name = shift;

   $$called_by_function_names{$name}++;
}

sub called_function_names($) {
    my $self = shift;
    my $called_function_names = \%{$self->{CALLED_FUNCTION_NAMES}};

    return sort(keys(%$called_function_names));
}

sub called_by_function_names($) {
    my $self = shift;
    my $called_by_function_names = \%{$self->{CALLED_BY_FUNCTION_NAMES}};

    return sort(keys(%$called_by_function_names));
}


1;
