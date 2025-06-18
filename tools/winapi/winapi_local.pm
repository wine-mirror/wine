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

package winapi_local;

use strict;
use warnings 'all';

use nativeapi qw($nativeapi);
use options qw($options);
use output qw($output);
use winapi qw($win16api $win32api @winapis);

sub _check_function($$$$$$) {
    my $return_type = shift;
    my $calling_convention = shift;
    my $external_name = shift;
    my $internal_name = shift;
    my $refargument_types = shift;
    my @argument_types = @$refargument_types;
    my $winapi = shift;

    my $module = $winapi->function_internal_module($internal_name);

    if($winapi->name eq "win16") {
	if($winapi->is_function_stub_in_module($module, $internal_name)) {
	    if($options->implemented) {
		$output->write("function implemented but declared as stub in .spec file\n");
	    }
	    return;
	} elsif($winapi->is_function_stub_in_module($module, $internal_name)) {
	    if($options->implemented_win32) {
		$output->write("32-bit variant of function implemented but declared as stub in .spec file\n");
	    }
	    return;
	}
    } elsif($winapi->is_function_stub_in_module($module, $internal_name)) {
	if($options->implemented) {
	    $output->write("function implemented but declared as stub in .spec file\n");
	}
	return;
    }

    my $forbidden_return_type = 0;
    my $implemented_return_kind;
    $winapi->type_used_in_module($return_type,$module);
    if(!defined($implemented_return_kind = $winapi->translate_argument($return_type))) {
	$winapi->declare_argument($return_type, "unknown");
	if($return_type ne "") {
	    $output->write("no win*.api translation defined: " . $return_type . "\n");
	}
    } elsif(!$winapi->is_allowed_kind($implemented_return_kind) ||
	    !$winapi->is_allowed_type_in_module($return_type, $module))
    {
	$forbidden_return_type = 1;
	$winapi->allow_kind($implemented_return_kind);
	$winapi->allow_type_in_module($return_type, $module);
	if($options->report_argument_forbidden($return_type)) {
	    $output->write("return type is forbidden: $return_type ($implemented_return_kind)\n");
	}
    }

    my $segmented = 0;
    if(defined($implemented_return_kind) && $implemented_return_kind =~ /^seg[sp]tr$/) {
	$segmented = 1;
    }

    my $implemented_calling_convention;
    if($winapi->name eq "win16") {
	if($calling_convention eq "__cdecl") {
	    $implemented_calling_convention = "cdecl";
	} elsif($calling_convention =~ /^(?:VFWAPIV|WINAPIV)$/) {
	    $implemented_calling_convention = "varargs";
	} elsif($calling_convention =~ /^(?:__stdcall|__RPC_STUB|__RPC_USER|NET_API_FUNCTION|RPC_ENTRY|SEC_ENTRY|VFWAPI|WINGDIPAPI|WMIAPI|WINAPI|CALLBACK)$/) {
	    if(defined($implemented_return_kind) && $implemented_return_kind =~ /^(?:s_word|word|void)$/) {
		$implemented_calling_convention = "pascal16";
	    } else {
		$implemented_calling_convention = "pascal";
	    }
	} elsif($calling_convention eq "__asm") {
    	    $implemented_calling_convention = "asm";
	} else {
    	    $implemented_calling_convention = "cdecl";
	}
    } elsif($winapi->name eq "win32") {
	if($calling_convention eq "__cdecl") {
	    $implemented_calling_convention = "cdecl";
	} elsif($calling_convention =~ /^(?:VFWAPIV|WINAPIV)$/) {
	    $implemented_calling_convention = "varargs";
	} elsif($calling_convention =~ /^(?:__stdcall|__RPC_STUB|__RPC_USER|APIENTRY|NET_API_FUNCTION|RPC_ENTRY|SEC_ENTRY|VFWAPI|WINGDIPAPI|WMIAPI|WINAPI|CALLBACK)$/) {
	    if(defined($implemented_return_kind) && $implemented_return_kind eq "longlong") {
		$implemented_calling_convention = "stdcall"; # FIXME: Check entry flags
	    } else {
		$implemented_calling_convention = "stdcall";
	    }
	} elsif($calling_convention eq "__asm") {
    	    $implemented_calling_convention = "asm";
	} else {
	    $implemented_calling_convention = "cdecl";
	}
    }

    my $declared_calling_convention = $winapi->function_internal_calling_convention($internal_name) || "";
    my @declared_argument_kinds = split(/\s+/, $winapi->function_internal_arguments($internal_name));

    my $declared_register = ($declared_calling_convention =~ / -register\b/);
    my $declared_i386 = ($declared_calling_convention =~ /(?:^pascal| -i386)\b/);
    $declared_calling_convention =~ s/ .*$//;

    if(!$declared_register &&
       $implemented_calling_convention ne $declared_calling_convention &&
       $implemented_calling_convention ne "asm" &&
       !($declared_calling_convention =~ /^pascal/ && $forbidden_return_type) &&
       !($implemented_calling_convention =~ /^(?:cdecl|varargs)$/ && $declared_calling_convention =~ /^(?:cdecl|varargs)$/))
    {
	if($options->calling_convention && (
            ($options->calling_convention_win16 && $winapi->name eq "win16") ||
            ($options->calling_convention_win32 && $winapi->name eq "win32")) &&
	    !$nativeapi->is_function($internal_name))
        {
	    $output->write("calling convention mismatch: $implemented_calling_convention != $declared_calling_convention\n");
	}
    }

    if($declared_calling_convention eq "varargs") {
	if ($#argument_types != -1 &&
            (($winapi->name eq "win32" && $argument_types[$#argument_types] eq "...") ||
            ($winapi->name eq "win16" && $argument_types[$#argument_types] eq "VA_LIST16")))
	{
	    pop @argument_types;
	} else {
	    $output->write("function not implemented as varargs\n");
	}
    } elsif ($#argument_types != -1 &&
        (($winapi->name eq "win32" && $argument_types[$#argument_types] eq "...") ||
        ($winapi->name eq "win16" && $argument_types[$#argument_types] eq "VA_LIST16")))
    {
	if($#argument_types == 0) {
	    pop @argument_types;
	} else {
	    $output->write("function not declared as varargs\n");
	}
    }

    if($internal_name =~ /^(?:NTDLL__ftol|NTDLL__CIpow)$/) { # FIXME: Kludge
	# ignore
    } else {
	my $n = 0;
	my @argument_kinds = map {
	    my $type = $_;
	    my $kind = "unknown";
	    $winapi->type_used_in_module($type,$module);
	    if($type eq "CONTEXT *") {
		$kind = "context";
	    } elsif($type eq "CONTEXT86 *") {
		$kind = "context86";
	    } elsif(!defined($kind = $winapi->translate_argument($type))) {
		$winapi->declare_argument($type, "unknown");
		$output->write("no win*.api translation defined: " . $type . "\n");
	    } elsif(!$winapi->is_allowed_kind($kind) ||
		    !$winapi->is_allowed_type_in_module($type, $module))
	    {
		$winapi->allow_kind($kind);
		$winapi->allow_type_in_module($type, $module);
		if($options->report_argument_forbidden($type)) {
		    $output->write("argument " . ($n + 1) . " type is forbidden: " . $type . " (" . $kind . ")\n");
		}
	    }

	    # FIXME: Kludge
	    if(defined($kind) && $kind eq "struct16") {
		$n+=2;
		("double", "double");
	    } elsif(defined($kind) && $kind eq "longlong") {
		$n+=1;
		"longlong";
	    } else {
		$n++;
		$kind;
	    }
	} @argument_types;

	if ($declared_register)
        {
            if (!$declared_i386 &&
                $argument_kinds[$#argument_kinds] ne "context") {
                $output->write("function declared as register, but CONTEXT * is not last argument\n");
            } elsif ($declared_i386 &&
                     $argument_kinds[$#argument_kinds] ne "context86") {
                $output->write("function declared as register, but CONTEXT86 * is not last argument\n");
            }
	}

	for my $n (0..$#argument_kinds) {
	    if(!defined($argument_kinds[$n]) || !defined($declared_argument_kinds[$n])) { next; }

	    if($argument_kinds[$n] =~ /^seg[ps]tr$/ ||
	       $declared_argument_kinds[$n] =~ /^seg[ps]tr$/)
	    {
		$segmented = 1;
	    }

	    # FIXME: Kludge
	    if(!defined($argument_types[$n])) {
		$argument_types[$n] = "";
	    }

	    if($argument_kinds[$n] =~ /^context(?:86)?$/) {
		# Nothing
	    } elsif(!$winapi->is_allowed_kind($argument_kinds[$n]) ||
	       !$winapi->is_allowed_type_in_module($argument_types[$n], $module))
	    {
		$winapi->allow_kind($argument_kinds[$n]);
		$winapi->allow_type_in_module($argument_types[$n],, $module);
		if($options->report_argument_forbidden($argument_types[$n])) {
		    $output->write("argument " . ($n + 1) . " type is forbidden: " .
				   "$argument_types[$n] ($argument_kinds[$n])\n");
		}
	    } elsif($argument_kinds[$n] ne $declared_argument_kinds[$n] &&
                   !($argument_kinds[$n] eq "longlong" && $declared_argument_kinds[$n] eq "double")) {
		if($options->report_argument_kind($argument_kinds[$n]) ||
		   $options->report_argument_kind($declared_argument_kinds[$n]))
		{
		    $output->write("argument " . ($n + 1) . " type mismatch: " .
			     $argument_types[$n] . " ($argument_kinds[$n]) != " .
			     $declared_argument_kinds[$n] . "\n");
		}
	    }
	}

        if ($options->argument_count &&
            $implemented_calling_convention ne "asm")
	{
	    if ($#argument_kinds != $#declared_argument_kinds and
                $#argument_types != $#declared_argument_kinds) {
		$output->write("argument count differs: " .
		    ($#argument_kinds + 1) . " != " .
		    ($#declared_argument_kinds + 1) . "\n");
	    } elsif ($#argument_kinds != $#declared_argument_kinds or
                     $#argument_types != $#declared_argument_kinds) {
		$output->write("argument count differs: " .
		    ($#argument_kinds + 1) . "/" . ($#argument_types + 1) .
		     " != " . ($#declared_argument_kinds + 1) .
                     " (long vs. long long problem?)\n");
	    }
	}

    }

    if($segmented && $options->shared_segmented && $winapi->is_shared_internal_function($internal_name)) {
	$output->write("function using segmented pointers shared between Win16 and Win32\n");
    }
}

sub check_function($) {
    my $function = shift;

    my $return_type = $function->return_type;
    my $calling_convention = $function->calling_convention;
    my $calling_convention16 = $function->calling_convention16;
    my $calling_convention32 = $function->calling_convention32;
    my $internal_name = $function->internal_name;
    my $external_name16 = $function->external_name16;
    my $external_name32 = $function->external_name32;
    my $module16 = $function->module16;
    my $module32 = $function->module32;
    my $refargument_types = $function->argument_types;

    if(!defined($refargument_types)) {
	return;
    }

    if($options->win16 && $options->report_module($module16)) {
	_check_function($return_type,
			$calling_convention, $external_name16,
			$internal_name, $refargument_types,
			$win16api);
    }

    if($options->win32 && $options->report_module($module32)) {
	_check_function($return_type,
			$calling_convention, $external_name32,
			$internal_name, $refargument_types,
			$win32api);
    }
}

sub _check_statements($$$) {
    my $winapi = shift;
    my $functions = shift;
    my $function = shift;

    my $module = $function->module;
    my $internal_name = $function->internal_name;

    my $first_debug_message = 1;
    local $_ = $function->statements;
    while(defined($_)) {
	if(s/(\w+)\s*(?:\(\s*(\w+)\s*\))?\s*\(\s*((?:\"[^\"]*\"|\([^\)]*\)|[^\)])*?)\s*\)//) {
	    my $called_name = $1;
	    my $channel = $2;
	    my $called_arguments = $3;
	    if($called_name =~ /^(?:if|for|while|switch|sizeof)$/) {
		# Nothing
	    } elsif($called_name =~ /^(?:ERR|FIXME|MSG|TRACE|WARN)$/) {
		if($first_debug_message && $called_name =~ /^(?:FIXME|TRACE)$/) {
		    $first_debug_message = 0;
		    if($called_arguments =~ /^\"\((.*?)\)(.*?)\"\s*,\s*(.*?)$/) {
			my $formatting = $1;
			my $extra = $2;
			my $arguments = $3;

			my $format;
			my $argument;
			my $n = 0;
			while($formatting && ($formatting =~ s/^([^,]*),?//, $format = $1, $format =~ s/^\s*(.*?)\s*$/$1/) &&
			      $arguments && ($arguments =~ s/^([^,]*),?//, $argument = $1, $argument =~ s/^\s*(.*?)\s*$/$1/))
			{
			    my $type = @{$function->argument_types}[$n];
			    my $name = @{$function->argument_names}[$n];

			    $n++;

			    if(!defined($type)) { last; }

			    $format =~ s/^\w+\s*[:=]?\s*//;
			    $format =~ s/\s*\{[^\{\}]*\}$//;
			    $format =~ s/\s*\[[^\[\]]*\]$//;
			    $format =~ s/^\'(.*?)\'$/$1/;
			    $format =~ s/^\\\"(.*?)\\\"$/$1/;

			    if($options->debug_messages) {
				if($argument !~ /$name/) {
				    $output->write("$called_name: argument $n is wrong ($name != '$argument')\n");
				} elsif(!$winapi->is_allowed_type_format($module, $type, $format)) {
				    $output->write("$called_name: argument $n ($type $name) has illegal format ($format)\n");
				}
			    }
			}

			if($options->debug_messages) {
			    my $count = $#{$function->argument_types} + 1;
			    if($n != $count) {
				$output->write("$called_name: argument count mismatch ($n != $count)\n");
			    }
			}
		    }
		}
	    } elsif($options->cross_call) {
		# $output->write("$internal_name: called $called_name\n");
		$$functions{$internal_name}->function_called($called_name);
		if(!defined($$functions{$called_name})) {
		    my $called_function = 'winapi_function'->new;

		    $called_function->internal_name($called_name);

		    $$functions{$called_name} = $called_function;	
		}
		$$functions{$called_name}->function_called_by($internal_name);
	    }
	} else {
	    undef $_;
	}
    }
}

sub check_statements($$) {
    my $functions = shift;
    my $function = shift;

    my $module16 = $function->module16;
    my $module32 = $function->module32;

    if($options->win16 && $options->report_module($module16)) {
	_check_statements($win16api, $functions, $function);
    }

    if($options->win32 && $options->report_module($module32)) {
	_check_statements($win32api, $functions, $function);
    }
}

sub check_file($$) {
    my $file = shift;
    my $functions = shift;

    if($options->cross_call) {
	my @names = sort(keys(%$functions));
	for my $name (@names) {
	    my $function = $$functions{$name};

	    my @called_names = $function->called_function_names;
	    my @called_by_names = $function->called_by_function_names;
	    my $module = $function->module;

	    if($options->cross_call_win32_win16) {
		my $module16 = $function->module16;
		my $module32 = $function->module32;

		if($#called_names >= 0 && (defined($module16) || defined($module32)) ) {
		    for my $called_name (@called_names) {
			my $called_function = $$functions{$called_name};

			my $called_module16 = $called_function->module16;
			my $called_module32 = $called_function->module32;
			if(defined($module32) &&
			   defined($called_module16) && !defined($called_module32) &&
			   $name ne $called_name)
			{
			    $output->write("$file: $module: $name: illegal call to $called_name (Win32 -> Win16)\n");
			}
		    }
		}
	    }

	    if($options->cross_call_unicode_ascii) {
		if($name =~ /(?<!A)W$/) {
		    for my $called_name (@called_names) {
			if($called_name =~ /A$/) {
			    $output->write("$file: $module: $name: illegal call to $called_name (Unicode -> ASCII)\n");
			}
		    }
		}
	    }
	}
    }
}

1;
