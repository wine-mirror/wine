package winapi;

use strict;

sub new {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    my $output = \${$self->{OUTPUT}};
    my $name = \${$self->{NAME}};

    $$output = shift;
    $$name = shift;
    my $file = shift;
    my $path = shift;

    $file =~ s/^.\/(.*)$/$1/;
    $self->parse_api_file($file);

    my @files = map {
	s/^.\/(.*)$/$1/;
	$_; 
    } split(/\n/, `find $path -name \\*.api`);
  
    foreach my $file (@files) {
	my $module = $file;
	$module =~ s/.*?\/([^\/]*?)\.api$/$1/;
	$self->parse_api_file($file,$module);
    }   

    return $self;
}

sub parse_api_file {
    my $self = shift;
    my $output = \${$self->{OUTPUT}};
    my $allowed_kind = \%{$self->{ALLOWED_KIND}};
    my $allowed_modules = \%{$self->{ALLOWED_MODULES}};
    my $allowed_modules_limited = \%{$self->{ALLOWED_MODULES_LIMITED}};
    my $allowed_modules_unlimited = \%{$self->{ALLOWED_MODULES_UNLIMITED}};
    my $translate_argument = \%{$self->{TRANSLATE_ARGUMENT}};

    my $file = shift;
    my $module = shift;

    my $kind;
    my $forbidden = 0;

    $$output->progress("$file");

    open(IN, "< $file") || die "$file: $!\n";
    $/ = "\n";
    while(<IN>) {
	s/^\s*?(.*?)\s*$/$1/; # remove whitespace at begin and end of line
	s/^(.*?)\s*#.*$/$1/;  # remove comments
	/^$/ && next;         # skip empty lines

	if(s/^%(\S+)\s*//) {
	    $kind = $1;
	    $forbidden = 0;

	    $$allowed_kind{$kind} = 1;
	    if(/^--forbidden/) {
		$forbidden = 1;
	    }
	} elsif(defined($kind)) {
	    my $type = $_;
	    if(!$forbidden) {
		if(defined($module)) {
		    if($$allowed_modules_unlimited{$type}) {
			$$output->write("$file: type ($type) already specificed as an unlimited type\n");
		    } elsif(!$$allowed_modules{$type}{$module}) {
			$$allowed_modules{$type}{$module} = 1;
			$$allowed_modules_limited{$type} = 1;
		    } else {
			$$output->write("$file: type ($type) already specificed\n");
		    }
		} else {
		    $$allowed_modules_unlimited{$type} = 1;
		}
	    } else {
		$$allowed_modules_limited{$type} = 1;
	    }
	    if(defined($$translate_argument{$type}) && $$translate_argument{$type} ne $kind) {
		$$output->write("$file: type ($type) respecified as different kind ($kind != $$translate_argument{$type})\n");
	    } else {
		$$translate_argument{$type} = $kind;
	    }
	} else {
	    $$output->write("$file: file must begin with %<type> statement\n");
	    exit 1;
	}
    }
    close(IN);
}

sub get_spec_file_type {
    my $proto = shift;
    my $class = ref($proto) || $proto;

    my $file = shift;

    my $type;

    open(IN, "< $file") || die "$file: $!\n";
    $/ = "\n";
    while(<IN>) {
	if(/^type\s*(\w+)/) {
	    $type = $1;
	    last;
	}
    }
    close(IN);

    return $type;
}

sub read_spec_files {
    my $proto = shift;
    my $class = ref($proto) || $proto;

    my $path = shift;
    my $win16api = shift;
    my $win32api = shift;

    my @files = map {
	s/^.\/(.*)$/$1/;
	$_; 
    } split(/\n/, `find $path -name \\*.spec`);

    foreach my $file (@files) {
	my $type = 'winapi'->get_spec_file_type($file);
	if($type eq "win16") {
	    $win16api->parse_spec_file($file);
	} elsif($type eq "win32") {
	    $win32api->parse_spec_file($file);
	}
    }
}

sub parse_spec_file {
    my $self = shift;

    my $output = \${$self->{OUTPUT}};
    my $function_arguments = \%{$self->{FUNCTION_ARGUMENTS}};
    my $function_calling_convention = \%{$self->{FUNCTION_CALLING_CONVENTION}};
    my $function_stub = \%{$self->{FUNCTION_STUB}};
    my $function_module = \%{$self->{FUNCTION_MODULE}};


    my $file = shift;

    my %ordinals;
    my $type;
    my $module;

    $$output->progress("$file");

    open(IN, "< $file") || die "$file: $!\n";
    $/ = "\n";
    my $header = 1;
    my $lookahead = 0;
    while($lookahead || defined($_ = <IN>)) {
	$lookahead = 0;
	s/^\s*(.*?)\s*$/$1/;
	s/^(.*?)\s*#.*$/$1/;
	/^$/ && next;

	if($header)  {
	    if(/^name\s*(\S*)/) { $module = $1; }
	    if(/^type\s*(\w+)/) { $type = $1; }
	    if(/^\d+/) { $header = 0 };
	    next;
	} 

	my $ordinal;
	if(/^(\d+)\s+(pascal|pascal16|stdcall|cdecl|register|interrupt|varargs)\s+(\S+)\s*\(\s*(.*?)\s*\)\s*(\S+)$/) {
	    my $calling_convention = $2;
	    my $external_name = $3;
	    my $arguments = $4;
	    my $internal_name = $5;
	   
	    $ordinal = $1;

	    # FIXME: Internal name existing more than once not handled properly
	    $$function_arguments{$internal_name} = $arguments;
	    $$function_calling_convention{$internal_name} = $calling_convention;
	    if(!$$function_module{$internal_name}) {
		$$function_module{$internal_name} = "$module";
	    } elsif($$function_module{$internal_name} !~ /$module/) {
		$$function_module{$internal_name} .= " & $module";
	    }
	} elsif(/^(\d+)\s+stub\s+(\S+)$/) {
	    my $external_name = $2;

	    $ordinal = $1;

	    my $internal_name;
	    if($type eq "win16") {
		$internal_name = $external_name . "16";
	    } else {
		$internal_name = $external_name;
	    }

	    # FIXME: Internal name existing more than once not handled properly
	    $$function_stub{$internal_name} = 1;
	    if(!$$function_module{$internal_name}) {
		$$function_module{$internal_name} = "$module";
	    } elsif($$function_module{$internal_name} !~ /$module/) {
		$$function_module{$internal_name} .= " & $module";
	    }
	} elsif(/^\d+\s+(equate|long|word|extern|forward)/) {
	    # ignore
	} else {
	    my $next_line = <IN>;
	    if($next_line =~ /^\d/) {
		die "$file: $.: syntax error: '$_'\n";
	    } else {
		$_ .= $next_line;
		$lookahead = 1;
	    }
	}
	
	if(defined($ordinal)) {
	    if($ordinals{$ordinal}) {
		$$output->write("$file: ordinal redefined: $_\n");
	    }
	    $ordinals{$ordinal}++;
	}
    }
    close(IN);
}

sub name {
    my $self = shift;
    my $name = \${$self->{NAME}};

    return $$name;
}

sub is_allowed_kind {
    my $self = shift;
    my $allowed_kind = \%{$self->{ALLOWED_KIND}};

    my $kind = shift;
    if(defined($kind)) {
	return $$allowed_kind{$kind};
    } else {
	return 0;
    }
}

sub is_limited_type {
    my $self = shift;
    my $allowed_modules_limited = \%{$self->{ALLOWED_MODULES_LIMITED}};

    my $type = shift;

    return $$allowed_modules_limited{$type};
}

sub allowed_type_in_module {
    my $self = shift;
    my $allowed_modules = \%{$self->{ALLOWED_MODULES}};
    my $allowed_modules_limited = \%{$self->{ALLOWED_MODULES_LIMITED}};

    my $type = shift;
    my $module = shift;

    return !$$allowed_modules_limited{$type} || $$allowed_modules{$type}{$module};
}

sub type_used_in_module {
    my $self = shift;
    my $used_modules = \%{$self->{USED_MODULES}};

    my $type = shift;
    my $module = shift;

    $$used_modules{$type}{$module} = 1;
    
    return ();
}

sub types_not_used {
    my $self = shift;
    my $used_modules = \%{$self->{USED_MODULES}};
    my $allowed_modules = \%{$self->{ALLOWED_MODULES}};

    my $not_used;
    foreach my $type (sort(keys(%$allowed_modules))) {
	foreach my $module (sort(keys(%{$$allowed_modules{$type}}))) {
	    if(!$$used_modules{$type}{$module}) {
		$$not_used{$module}{$type} = 1;
	    }
	}
    }
    return $not_used;
}

sub translate_argument {
    my $self = shift;
    my $translate_argument = \%{$self->{TRANSLATE_ARGUMENT}};

    my $argument = shift;

    return $$translate_argument{$argument};
}

sub all_declared_types {
    my $self = shift;
    my $translate_argument = \%{$self->{TRANSLATE_ARGUMENT}};

    return sort(keys(%$translate_argument));
}

sub found_type {
    my $self = shift;
    my $type_found = \%{$self->{TYPE_FOUND}};

    my $name = shift;

    $$type_found{$name}++;
}

sub type_found {
    my $self = shift;
    my $type_found= \%{$self->{TYPE_FOUND}};

    my $name = shift;

    return $$type_found{$name};
}

sub all_functions {
    my $self = shift;
    my $function_calling_convention = \%{$self->{FUNCTION_CALLING_CONVENTION}};

    return sort(keys(%$function_calling_convention));
}

sub all_functions_found {
    my $self = shift;
    my $function_found = \%{$self->{FUNCTION_FOUND}};

    return sort(keys(%$function_found));
}

sub function_calling_convention {
    my $self = shift;
    my $function_calling_convention = \%{$self->{FUNCTION_CALLING_CONVENTION}};

    my $name = shift;

    return $$function_calling_convention{$name};
}

sub is_function {
    my $self = shift;
    my $function_calling_convention = \%{$self->{FUNCTION_CALLING_CONVENTION}};

    my $name = shift;

    return $$function_calling_convention{$name};
}

sub is_shared_function {
    my $self = shift;
    my $function_shared = \%{$self->{FUNCTION_SHARED}};

    my $name = shift;

    return $$function_shared{$name};
}

sub found_shared_function {
    my $self = shift;
    my $function_shared = \%{$self->{FUNCTION_SHARED}};

    my $name = shift;

    $$function_shared{$name} = 1;
}

sub function_arguments {
    my $self = shift;
    my $function_arguments = \%{$self->{FUNCTION_ARGUMENTS}};

    my $name = shift;

    return $$function_arguments{$name};
}

sub function_module {
    my $self = shift;
    my $function_module = \%{$self->{FUNCTION_MODULE}};

    my $name = shift;

    return $$function_module{$name};
}

sub function_stub {
    my $self = shift;
    my $function_stub = \%{$self->{FUNCTION_STUB}};

    my $name = shift;

    return $$function_stub{$name};
}

sub found_function {
    my $self = shift;
    my $function_found = \%{$self->{FUNCTION_FOUND}};

    my $name = shift;

    $$function_found{$name}++;
}

sub function_found {
    my $self = shift;
    my $function_found = \%{$self->{FUNCTION_FOUND}};

    my $name = shift;

    return $$function_found{$name};
}

1;
