package nativeapi;

use strict;

sub new {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    my $options = \${$self->{OPTIONS}};
    my $output = \${$self->{OUTPUT}};
    my $functions = \%{$self->{FUNCTIONS}};
    my $conditionals = \%{$self->{CONDITIONALS}};
    my $conditional_headers = \%{$self->{CONDITIONAL_HEADERS}};
    my $conditional_functions = \%{$self->{CONDITIONAL_FUNCTIONS}};

    $$options = shift;
    $$output = shift;
    my $api_file = shift;
    my $configure_in_file = shift;
    my $config_h_in_file = shift;

    $api_file =~ s/^\.\///;
    $configure_in_file =~ s/^\.\///;
    $config_h_in_file =~ s/^\.\///;

    if($$options->progress) {
	$$output->progress("$api_file");
    }

    open(IN, "< $api_file");
    local $/ = "\n";
    while(<IN>) {
	s/^\s*?(.*?)\s*$/$1/; # remove whitespace at begin and end of line
	s/^(.*?)\s*#.*$/$1/;  # remove comments
	/^$/ && next;         # skip empty lines   
	
	$$functions{$_}++;
    }
    close(IN);

    if($$options->progress) {
	$$output->progress("$configure_in_file");
    }

    my $again = 0;
    open(IN, "< $configure_in_file");   
    local $/ = "\n";
    while($again || (defined($_ = <IN>))) {
	$again = 0;
	chomp;
	if(/^(.*?)\\$/) {
	    my $current = $1;
	    my $next = <IN>;
	    if(defined($next)) {
		# remove trailing whitespace
		$current =~ s/\s+$//;

		# remove leading whitespace
		$next =~ s/^\s+//;

		$_ = $current . " " . $next;

		$again = 1;
		next;
	    }
	}

	# remove leading and trailing whitespace
	s/^\s*(.*?)\s*$/$1/;

	# skip emty lines
	if(/^$/) { next; }

	# skip comments
	if(/^dnl/) { next; }

	if(/^AC_CHECK_HEADERS\(\s*([^,\)]*)(?:,|\))?/) {
	    foreach my $name (split(/\s+/, $1)) {
		$$conditional_headers{$name}++;
	    }
	} elsif(/^AC_CHECK_FUNCS\(\s*([^,\)]*)(?:,|\))?/) {
	    foreach my $name (split(/\s+/, $1)) {
		$$conditional_functions{$name}++;
	    }
	} elsif(/^AC_FUNC_ALLOCA/) {
	    $$conditional_headers{"alloca.h"}++;
	}

    }
    close(IN);

    if($$options->progress) {
	$$output->progress("$config_h_in_file");
    }

    open(IN, "< $config_h_in_file");
    local $/ = "\n";
    while(<IN>) {
	# remove leading and trailing whitespace
	s/^\s*(.*?)\s*$/$1/;

	# skip emty lines
	if(/^$/) { next; }

	if(/^\#undef\s+(\S+)$/) {
	    $$conditionals{$1}++;
	}
    }
    close(IN);

    return $self;
}

sub is_function {
    my $self = shift;
    my $functions = \%{$self->{FUNCTIONS}};

    my $name = shift;

    return $$functions{$name};
}

sub is_conditional {
    my $self = shift;
    my $conditionals = \%{$self->{CONDITIONALS}};

    my $name = shift;

    return $$conditionals{$name};
}

sub found_conditional {
    my $self = shift;
    my $conditional_found = \%{$self->{CONDITIONAL_FOUND}};

    my $name = shift;

    $$conditional_found{$name}++;
}

sub is_conditional_header {
    my $self = shift;
    my $conditional_headers = \%{$self->{CONDITIONAL_HEADERS}};

    my $name = shift;

    return $$conditional_headers{$name};
}

sub is_conditional_function {
    my $self = shift;
    my $conditional_functions = \%{$self->{CONDITIONAL_FUNCTIONS}};

    my $name = shift;

    return $$conditional_functions{$name};
}

sub global_report {
    my $self = shift;

    my $output = \${$self->{OUTPUT}};
    my $conditional_found = \%{$self->{CONDITIONAL_FOUND}};
    my $conditionals = \%{$self->{CONDITIONALS}};

    my @messages;
    foreach my $name (sort(keys(%$conditionals))) {
	if($name =~ /^const|inline|size_t$/) { next; }

	if(0 && !$$conditional_found{$name}) {
	    push @messages, "config.h.in: conditional $name not used\n";
	}
    }

    foreach my $message (sort(@messages)) {
	$$output->write($message);
    }
}

1;
