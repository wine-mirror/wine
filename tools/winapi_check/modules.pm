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
    my $spec_file2module = \%{$self->{SPEC_FILE2MODULE}};

    $$options = shift;
    $$output = shift;
    my $wine_dir = shift;
    my $current_dir = shift;
    my $file_type = shift;    
    my $module_file = shift;

    $module_file =~ s/^\.\///;

    my @all_spec_files = map {
	s/^.\/(.*)$/$1/;
	if(&$file_type($_) eq "library") {
	    $_;
	} else {
	    ();
	}
    } split(/\n/, `find $wine_dir -name \\*.spec`);

    my %all_spec_files;
    foreach my $file (@all_spec_files) {
	$all_spec_files{$file}++ ;
    }

    if($$options->progress) {
	$$output->progress("modules.dat");
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
	   
	    if(!-f "$wine_dir/$spec_file") {
		$$output->write("modules.dat: $spec_file: file ($spec_file) doesn't exist or is no file\n");
	    } 

	    if($wine_dir eq ".") {
		$all_spec_files{$spec_file}--;
	    } else {
		$all_spec_files{"$wine_dir/$spec_file"}--;
	    }
	    $$spec_files{""}{$spec_file}++; # FIXME: Kludge
	    next;
	} else {
	    $allowed_dir = $1;
	}
	$$spec_files{$allowed_dir}{$spec_file}++;

	if(!-d "$wine_dir/$allowed_dir") {
	    $$output->write("modules.dat: $spec_file: directory ($allowed_dir) doesn't exist or is no directory\n");
	} 
    }
    close(IN);

    foreach my $spec_file (sort(keys(%all_spec_files))) {
	if($all_spec_files{$spec_file} > 0) {
	    $$output->write("modules.dat: $spec_file: exists but is not specified\n");
	}
    }

    return $self;
}

sub spec_file_module {
    my $self = shift;

    my $spec_file2module = \%{$self->{SPEC_FILE2MODULE}};
    my $module2spec_file = \%{$self->{MODULE2SPEC_FILE}};

    my $spec_file = shift;
    $spec_file =~ s/^\.\///;

    my $module = shift;
  
    $$spec_file2module{$spec_file}{$module}++;
    $$module2spec_file{$module}{$spec_file}++;
}

sub is_allowed_module_in_file {
    my $self = shift;

    my $spec_files = \%{$self->{SPEC_FILES}};
    my $spec_file2module = \%{$self->{SPEC_FILE2MODULE}};

    my $module = shift;
    my $file = shift;
    $file =~ s/^\.\///;

    my $dir = $file;
    $dir =~ s/\/[^\/]*$//;

    foreach my $spec_file (sort(keys(%{$$spec_files{$dir}}))) {
	if($$spec_file2module{$spec_file}{$module}) {
	    return 1;
	}
    }

    return 0;
}

sub allowed_modules_in_file {
    my $self = shift;

    my $spec_files = \%{$self->{SPEC_FILES}};
    my $spec_file2module = \%{$self->{SPEC_FILE2MODULE}};

    my $file = shift;
    $file =~ s/^\.\///;

    my $dir = $file;
    $dir =~ s/\/[^\/]*$//;

    my %allowed_modules = ();
    foreach my $spec_file (sort(keys(%{$$spec_files{$dir}}))) {
	foreach my $module (sort(keys(%{$$spec_file2module{$spec_file}}))) {
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

sub found_module_in_dir {
    my $self = shift;

    my $module = shift;
    my $dir = shift;

    my $used_module_dirs = \%{$self->{USED_MODULE_DIRS}};

    $$used_module_dirs{$module}{$dir}++;
}

sub global_report {
    my $self = shift;

    my $output = \${$self->{OUTPUT}};
    my $spec_files = \%{$self->{SPEC_FILES}};
    my $spec_file2module = \%{$self->{SPEC_FILE2MODULE}};
    my $used_module_dirs = \%{$self->{USED_MODULE_DIRS}};

    my @messages;
    foreach my $dir (sort(keys(%$spec_files))) {
	if($dir eq "") { next; }
	foreach my $spec_file (sort(keys(%{$$spec_files{$dir}}))) {
	    foreach my $module (sort(keys(%{$$spec_file2module{$spec_file}}))) {
		if(!$$used_module_dirs{$module}{$dir}) {
		    push @messages, "modules.dat: $spec_file: directory ($dir) is not used\n";
		}
	    }
	}
    }

    foreach my $message (sort(@messages)) {
	$$output->write($message);
    }
}

1;
