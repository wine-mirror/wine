package winapi;

use strict;

sub new {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    my $allowed_kind = \%{$self->{ALLOWED_KIND}};
    my $allowed_modules = \%{$self->{ALLOWED_MODULES}};
    my $allowed_modules_limited = \%{$self->{ALLOWED_MODULES_LIMITED}};
    my $translate_argument = \%{$self->{TRANSLATE_ARGUMENT}};

    $self->{NAME} = shift;
    my $file = shift;

    my @modules;
    my $kind;
    my $forbidden = 0;

    open(IN, "< $file") || die "$file: $!\n";
    $/ = "\n";
    while(<IN>) {
	s/^\s*?(.*?)\s*$/$1/; # remove whitespace at begin and end of line
	s/^(.*?)\s*#.*$/$1/;  # remove comments
	/^$/ && next;         # skip empty lines

	if(s/^%(\S+)\s*//) {
	    $kind = $1;
	    @modules = ();
	    $forbidden = 0;

	    $$allowed_kind{$kind} = 1;
	    if(/^--module=(\S*)/) {
		@modules = split(/,/, $1);
	    } elsif(/^--forbidden/) {
		$forbidden = 1;
	    }
	} elsif(defined($kind)) {
	    my $type = $_;
	    if(!$forbidden) {
		for my $module (@modules) {
		    $$allowed_modules_limited{$type} = 1;
		    $$allowed_modules{$type}{$module} = 1;
		}
	    } else {
		$$allowed_modules_limited{$type} = 1;
	    }
	    $$translate_argument{$type} = $kind;
	} else {
	    print "$file: file must begin with %<type> statement\n";
	    exit 1;
	}
    }
    close(IN);

    return $self;
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

    my $win16api = shift;
    my $win32api = shift;

    foreach my $file (split(/\n/, `find . -name \\*.spec`)) {
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
    my $function_arguments = \%{$self->{FUNCTION_ARGUMENTS}};
    my $function_calling_convention = \%{$self->{FUNCTION_CALLING_CONVENTION}};
    my $function_stub = \%{$self->{FUNCTION_STUB}};
    my $function_module = \%{$self->{FUNCTION_MODULE}};

    my $file = shift;
    
    my $type;
    my $module;

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
	    if(/^\d+/) { $header = 0 };
	    next;
	} 

	if(/^\d+\s+(pascal|pascal16|stdcall|cdecl|register|interrupt|varargs)\s+(\S+)\s*\(\s*(.*?)\s*\)\s*(\S+)$/) {
	    my $calling_convention = $1;
	    my $external_name = $2;
	    my $arguments = $3;
	    my $internal_name = $4;

	    # FIXME: Internal name existing more than once not handled properly
	    $$function_arguments{$internal_name} = $arguments;
	    $$function_calling_convention{$internal_name} = $calling_convention;
	    $$function_module{$internal_name} = $module;
	} elsif(/^\d+\s+stub\s+(\S+)$/) {
	    my $external_name = $1;
	    $$function_stub{$external_name} = 1;
	    $$function_module{$external_name} = $module;
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
    }
    close(IN);
}

sub name {
    my $self = shift;
    return $self->{NAME};
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

sub allowed_type_in_module {
    my $self = shift;
    my $allowed_modules = \%{$self->{ALLOWED_MODULES}};
    my $allowed_modules_limited = \%{$self->{ALLOWED_MODULES_LIMITED}};

    my $type = shift;
    my $module = shift;

    return !$$allowed_modules_limited{$type} || $$allowed_modules{$type}{$module};
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
    my $function_found = \$self->{FUNCTION_FOUND};

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

    if($self->is_function($name)) { 
	return $$function_module{$name};
    } else {
	return undef;
    }
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
