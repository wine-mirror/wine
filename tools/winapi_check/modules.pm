package modules;

use strict;

sub new {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    my $options = \${$self->{OPTIONS}};
    my $output = \${$self->{OUTPUT}};
    my $spec_files = \%{$self->{SPEC_FILES}};
    my $modules = \%{$self->{MODULES}};

    $$options = shift;
    $$output = shift;
    my $wine_dir = shift;
    my $current_dir = shift;
    my $module_file = shift;

    $module_file =~ s/^\.\///;

    if($$options->progress) {
	$$output->progress("$module_file");
    }

    my $allowed_dir;
    my $spec_file;

    open(IN, "< $module_file");
    local $/ = "\n";
    while(<IN>) {
	s/^\s*?(.*?)\s*$/$1/; # remove whitespace at begining and end of line
	s/^(.*?)\s*#.*$/$1/;  # remove comments
	/^$/ && next;         # skip empty lines

	if(/^%\s+(.*?)$/) {
	    $spec_file = $1;
	    next;
	} else {
	    $allowed_dir = $1;
	}
	$$spec_files{$allowed_dir}{$spec_file}++;

	if(!-d "$wine_dir/$allowed_dir") {
	    $$output->write("$module_file: $spec_file: directory ($allowed_dir) doesn't exist or is no directory\n");
	} 
    }
    close(IN);

    return $self;
}

sub spec_file_module {
    my $self = shift;

    my $modules = \%{$self->{MODULES}};

    my $spec_file = shift;
    $spec_file =~ s/^\.\///;

    my $module = shift;
  
    $$modules{$spec_file}{$module}++;
}

sub is_allowed_module_in_file {
    my $self = shift;

    my $spec_files = \%{$self->{SPEC_FILES}};
    my $modules = \%{$self->{MODULES}};

    my $module = shift;
    my $file = shift;
    $file =~ s/^\.\///;

    my $dir = $file;
    $dir =~ s/\/[^\/]*$//;

    foreach my $spec_file (sort(keys(%{$$spec_files{$dir}}))) {
	if($$modules{$spec_file}{$module}) {
	    return 1;
	}
    }

    return 0;
}

sub allowed_modules_in_file {
    my $self = shift;

    my $spec_files = \%{$self->{SPEC_FILES}};
    my $modules = \%{$self->{MODULES}};

    my $file = shift;
    $file =~ s/^\.\///;

    my $dir = $file;
    $dir =~ s/\/[^\/]*$//;

    my %allowed_modules = ();
    foreach my $spec_file (sort(keys(%{$$spec_files{$dir}}))) {
	foreach my $module (sort(keys(%{$$modules{$spec_file}}))) {
	    $allowed_modules{$module}++;
	}
    }

    return join(" & ", sort(keys(%allowed_modules)));
}

sub allowed_spec_files {
    my $self = shift;

    my $options = \${$self->{OPTIONS}};
    my $output = \${$self->{OUTPUT}};
    my $spec_files = \%{$self->{SPEC_FILES}};

    my $wine_dir = shift;
    my $current_dir = shift;

    my @dirs = map {
	s/^\.\/(.*)$/$1/;
	if(/^\.$/) {
	    $current_dir;
	} else {
	    if($current_dir ne ".") {
		"$current_dir/$_";
	    } else {
		$_;
	    }
	}
    } split(/\n/, `find . -type d ! -name CVS`);

    my %allowed_spec_files = ();
    foreach my $dir (sort(@dirs)) {
	foreach my $spec_file (sort(keys(%{$$spec_files{$dir}}))) {
	    $allowed_spec_files{$spec_file}++; 
	}
    }

    return sort(keys(%allowed_spec_files));
}

1;


