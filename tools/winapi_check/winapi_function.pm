package winapi_function;
use base qw(function);

use strict;

use util qw(&normalize_set);
use winapi qw($win16api $win32api @winapis);

########################################################################
# constructor
#

sub new {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    return $self;
}

########################################################################
# winapi
#

sub external_name16 {
    my $self = shift;
    my $internal_name = $self->internal_name;

    return $win16api->function_external_name($internal_name);
}

sub external_names16 {
    my $self = shift;
    my $external_name16 = $self->external_name16;
    
    if(defined($external_name16)) {
	return split(/\s*&\s*/, $external_name16);
    } else {
	return ();
    }
}

sub external_name32 {
    my $self = shift;
    my $internal_name = $self->internal_name;

    return $win32api->function_external_name($internal_name);
}

sub external_names32 {
    my $self = shift;
    my $external_name32 = $self->external_name32;
    
    if(defined($external_name32)) {
	return split(/\s*&\s*/, $external_name32);
    } else {
	return ();
    }
}

sub external_names {
    my $self = shift;

    my @external_names;
    push @external_names, $self->external_names16;
    push @external_names, $self->external_names32;

    return @external_names;
}

sub module16 {
    my $self = shift;
    my $internal_name = $self->internal_name;

    return $win16api->function_internal_module($internal_name);
}

sub modules16 {
    my $self = shift;
    my $module16 = $self->module16;
    
    if(defined($module16)) {
	return split(/\s*&\s*/, $module16);
    } else {
	return ();
    }
}

sub module32 {
    my $self = shift;
    my $internal_name = $self->internal_name;

    return $win32api->function_internal_module($internal_name);
}

sub modules32 {
    my $self = shift;
    my $module32 = $self->module32;
    
    if(defined($module32)) {
	return split(/\s*&\s*/, $module32);
    } else {
	return ();
    }
}

sub module {
    my $self = shift;
    my $module16 = $self->module16;
    my $module32 = $self->module32;

    my $module;
    if(defined($module16) && defined($module32)) {
	$module = "$module16 & $module32";
    } elsif(defined($module16)) {
	$module = $module16;
    } elsif(defined($module32)) {
	$module = $module32;
    } else {
	$module = "";
    }
}

sub modules {
    my $self = shift;

    my @modules;
    push @modules, $self->modules16;
    push @modules, $self->modules32;

    return @modules;
}

sub prefix {
    my $self = shift;
    my $module16 = $self->module16;
    my $module32 = $self->module32;

    my $return_type = $self->return_type;
    my $internal_name = $self->internal_name;
    my $calling_convention = $self->calling_convention;
    my @argument_types = @{$self->argument_types};

    if($#argument_types < 0) {
	@argument_types = ("void");
    }

    my $prefix = "";
    if(defined($module16) && !defined($module32)) {
	$prefix .= normalize_set($module16) . ": ";
    } elsif(!defined($module16) && defined($module32)) {
	$prefix .= normalize_set($module32) . ": ";
    } elsif(defined($module16) && defined($module32)) {
	$prefix .= normalize_set($module16) . " & " . normalize_set($module32) . ": ";
    } else {
	$prefix .= "<>: ";
    }
    $prefix .= "$return_type ";
    $prefix .= "$calling_convention " if $calling_convention;
    $prefix .= "$internal_name(" . join(",", @argument_types) . "): ";

    return $prefix;
}

sub calling_convention16 {
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
    if(/^__cdecl$/) {
	return "cdecl";
    } elsif(/^VFWAPIV|WINAPIV$/) {
	if(!defined($suffix)) { return undef; }
	return "pascal$suffix"; # FIXME: Is this correct?
    } elsif(/^__stdcall|VFWAPI|WINAPI|CALLBACK$/) {
	if(!defined($suffix)) { return undef; }
	return "pascal$suffix";
    } elsif(/^__asm$/) {
	return "asm";
    } else {
	return "cdecl";
    }
}

sub calling_convention32 {
    my $self = shift;

    local $_ = $self->calling_convention;
    if(/^__cdecl$/) {
	return "cdecl";
    } elsif(/^VFWAPIV|WINAPIV$/) {
	return "varargs";
    } elsif(/^__stdcall|VFWAPI|WINAPI|CALLBACK$/) {
	return "stdcall";
    } elsif(/^__asm$/) {
	return "asm";
    } else {
	return "cdecl";
    }
}

sub get_all_module_ordinal16 {
    my $self = shift;
    my $internal_name = $self->internal_name;

    return winapi::get_all_module_internal_ordinal16($internal_name);
}

sub get_all_module_ordinal32 {
    my $self = shift;
    my $internal_name = $self->internal_name;

    return winapi::get_all_module_internal_ordinal32($internal_name);
}

sub get_all_module_ordinal {
    my $self = shift;
    my $internal_name = $self->internal_name;

    return winapi::get_all_module_internal_ordinal($internal_name);
}

sub _return_kind {
    my $self = shift;
    my $winapi = shift;
    my $return_type = $self->return_type;

    return $winapi->translate_argument($return_type);
}

sub return_kind16 {
    my $self = shift; return $self->_return_kind($win16api, @_);
}

sub return_kind32 {
    my $self = shift; return $self->_return_kind($win32api, @_);
}

sub _argument_kinds {   
    my $self = shift;
    my $winapi = shift;
    my @argument_types = @{$self->argument_types};

    my @argument_kinds;
    foreach my $argument_type (@argument_types) {
	my $argument_kind = $winapi->translate_argument($argument_type);

	if(defined($argument_kind) && $argument_kind eq "longlong") {
	    push @argument_kinds, ("long", "long");
	} else {
	    push @argument_kinds, $argument_kind;
	}
    }

    return [@argument_kinds];
}

sub argument_kinds16 {
    my $self = shift; return $self->_argument_kinds($win16api, @_);
}

sub argument_kinds32 {
    my $self = shift; return $self->_argument_kinds($win32api, @_);
}

##############################################################################
# Accounting
#

sub function_called {    
    my $self = shift;
    my $called_function_names = \%{$self->{CALLED_FUNCTION_NAMES}};

    my $name = shift;

    $$called_function_names{$name}++;
}

sub function_called_by { 
   my $self = shift;
   my $called_by_function_names = \%{$self->{CALLED_BY_FUNCTION_NAMES}};

   my $name = shift;

   $$called_by_function_names{$name}++;
}

sub called_function_names {    
    my $self = shift;
    my $called_function_names = \%{$self->{CALLED_FUNCTION_NAMES}};

    return sort(keys(%$called_function_names));
}

sub called_by_function_names {    
    my $self = shift;
    my $called_by_function_names = \%{$self->{CALLED_BY_FUNCTION_NAMES}};

    return sort(keys(%$called_by_function_names));
}


1;
