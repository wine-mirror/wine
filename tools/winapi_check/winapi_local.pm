package winapi_local;

use strict;

sub check_function {
    my $options = shift;
    my $output = shift;
    my $return_type = shift;
    my $calling_convention = shift;
    my $name = shift;
    my $refargument_types = shift;
    my @argument_types = @$refargument_types;
    my $winapi = shift;

    my $module = $winapi->function_module($name);
       
    if($winapi->name eq "win16") {
	my $name16 = $name;
	$name16 =~ s/16$//;
	if($name16 ne $name && $winapi->function_stub($name16)) {
	    if($options->implemented) {
		&$output("function implemented but declared as stub in .spec file");
	    }
	    return;
	} elsif($winapi->function_stub($name)) {
	    if($options->implemented_win32) {
		&$output("32-bit variant of function implemented but declared as stub in .spec file");
	    }
	    return;
	}
    } elsif($winapi->function_stub($name)) {
	if($options->implemented) {
	    &$output("function implemented but declared as stub in .spec file");
	}
	return;
    }

    my $forbidden_return_type = 0;
    my $implemented_return_kind;
    $winapi->type_used_in_module($return_type,$module);
    if(!defined($implemented_return_kind = $winapi->translate_argument($return_type))) {
	if($return_type ne "") {
	    &$output("no translation defined: " . $return_type);
	}
    } elsif(!$winapi->is_allowed_kind($implemented_return_kind) || !$winapi->allowed_type_in_module($return_type,$module)) {
	$forbidden_return_type = 1;
	if($options->report_argument_forbidden($return_type)) {
	    &$output("forbidden return type: $return_type ($implemented_return_kind)");
	}
    }
    
    my $segmented = 0;
    if($implemented_return_kind =~ /^segptr|segstr$/) {
	$segmented = 1;
    }

    my $implemented_calling_convention;
    if($winapi->name eq "win16") {
	if($calling_convention =~ /^__cdecl$/) {
	    $implemented_calling_convention = "cdecl";
	} elsif($calling_convention =~ /^VFWAPIV|WINAPIV$/) {
	    $implemented_calling_convention = "varargs";
	} elsif($calling_convention = ~ /^__stdcall|VFWAPI|WINAPI$/) {
	    if($implemented_return_kind =~ /^s_word|word|void$/) {
		$implemented_calling_convention = "pascal16";
	    } else {
		$implemented_calling_convention = "pascal";
	    }
	}
    } elsif($winapi->name eq "win32") {
	if($calling_convention =~ /^__cdecl$/) {
	    $implemented_calling_convention = "cdecl";
	} elsif($calling_convention =~ /^VFWAPIV|WINAPIV$/) {
	    $implemented_calling_convention = "varargs";
	} elsif($calling_convention =~ /^__stdcall|VFWAPI|WINAPI$/) {
	    $implemented_calling_convention = "stdcall";
	} else {
	    $implemented_calling_convention = "<default>";
	}
    }

    my $declared_calling_convention = $winapi->function_calling_convention($name);
    my @declared_argument_kinds = split(/\s+/, $winapi->function_arguments($name));

    if($declared_calling_convention =~ /^register|interrupt$/) {
	push @declared_argument_kinds, "ptr";
    }
   
    if($declared_calling_convention =~ /^register|interupt$/ && 
         (($winapi->name eq "win32" && $implemented_calling_convention eq "stdcall") ||
         (($winapi->name eq "win16" && $implemented_calling_convention =~ /^pascal/))))
    {
	# correct
    } elsif($implemented_calling_convention ne $declared_calling_convention && 
       !($declared_calling_convention =~ /^pascal/ && $forbidden_return_type) &&
       !($implemented_calling_convention =~ /^cdecl|varargs$/ && $declared_calling_convention =~ /^cdecl|varargs$/))
    {
	if($options->calling_convention) {
	    &$output("calling convention mismatch: $implemented_calling_convention != $declared_calling_convention");
	}
    }

    if($declared_calling_convention eq "varargs") {
	if($#argument_types != -1 && $argument_types[$#argument_types] eq "...") {
	    pop @argument_types;
	} else {
	    &$output("function not implemented as vararg");
	}
    } elsif($#argument_types != -1 && $argument_types[$#argument_types] eq "...") {
	&$output("function not declared as vararg");
    }

    if($#argument_types != -1 && $argument_types[$#argument_types] eq "CONTEXT *" &&
       $name !~ /^(Get|Set)ThreadContext$/)
    {
	$#argument_types--;
    }
    
    if($name =~ /^CRTDLL__ftol|CRTDLL__CIpow$/) {
	# ignore
    } else {
	my $n = 0;
	my @argument_kinds = map {
	    my $type = $_;
	    my $kind = "unknown";
	    $winapi->type_used_in_module($type,$module);
	    if(!defined($kind = $winapi->translate_argument($type))) {
		&$output("no translation defined: " . $type);
	    } elsif(!$winapi->is_allowed_kind($kind) ||
		    !$winapi->allowed_type_in_module($type, $module)) {
		if($options->report_argument_forbidden($type)) {
		    &$output("forbidden argument " . ($n + 1) . " type (" . $type . ")");
		}
	    }
	    if(defined($kind) && $kind eq "longlong") {
		$n+=2;
		("long", "long");
	    } else {
		$n++;
		$kind;
	    }
	} @argument_types;

	for my $n (0..$#argument_kinds) {
	    if(!defined($argument_kinds[$n]) || !defined($declared_argument_kinds[$n])) { next; }

	    if($argument_kinds[$n] =~ /^segptr|segstr$/ ||
	       $declared_argument_kinds[$n] =~ /^segptr|segstr$/)
	    {
		$segmented = 1;
	    }

	    if($argument_kinds[$n] ne $declared_argument_kinds[$n]) {
		if($options->report_argument_kind($argument_kinds[$n]) ||
		   $options->report_argument_kind($declared_argument_kinds[$n]))
		{
		    &$output("argument " . ($n + 1) . " type mismatch: " .
			     $argument_types[$n] . " ($argument_kinds[$n]) != " . $declared_argument_kinds[$n]);
		}
	    }
	}
        if($#argument_kinds != $#declared_argument_kinds) {
	    if($options->argument_count) {
		&$output("argument count differs: " . ($#argument_types + 1) . " != " . ($#declared_argument_kinds + 1));
	    }
	}

    }

    if($segmented && $options->shared_segmented && $winapi->is_shared_function($name)) {
	&$output("function using segmented pointers shared between Win16 och Win32");
    }
}

sub check_file {
    my $options = shift;
    my $output = shift;
    my $file = shift;
    my $functions = shift;

    if($options->cross_call) {
	my @names = sort(keys(%$functions));
	for my $name (@names) {
	    my @called_names = $$functions{$name}->called_function_names;
	    my @called_by_names = $$functions{$name}->called_by_function_names;
	    my $module = $$functions{$name}->module;
	    my $module16 = $$functions{$name}->module16;
	    my $module32 = $$functions{$name}->module32;

	    if($#called_names >= 0 && (defined($module16) || defined($module32)) ) {	
		for my $called_name (@called_names) {
		    my $called_module16 = $$functions{$called_name}->module16;
		    my $called_module32 = $$functions{$called_name}->module32;
		    if(defined($module32) &&
		       defined($called_module16) && !defined($called_module32) &&
		       $name ne $called_name) 
		    {
			$output->write("$file: $module: $name: illegal call to $called_name (Win16)\n");
		    }
		}
	    }
	}
    }
}

1;

