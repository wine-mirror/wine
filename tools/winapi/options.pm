package options;

use strict;

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw(&parse_comma_list);
@EXPORT_OK = qw();

my $output = "output";

sub parse_comma_list {
    my $prefix = shift;
    my $value = shift;
    if(defined($prefix) && $prefix eq "no") {
	return { active => 0, filter => 0, hash => {} };
    } elsif(defined($value)) {
	my %names;
	for my $name (split /,/, $value) {
	    $names{$name} = 1;
	}
	return { active => 1, filter => 1, hash => \%names };
    } else {
	return { active => 1, filter => 0, hash => {} };
    }
}

my $_options;

sub new {
    my $self = shift;
    $_options = _options->new(@_);
    return $_options;
}

sub AUTOLOAD {
    my $self = shift;

    my $name = $options::AUTOLOAD;
    $name =~ s/^.*::(.[^:]*)$/$1/;

    return $_options->$name(@_);
}

package _options;

use strict;

sub new {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    my $options_long = \%{$self->{OPTIONS_LONG}};
    my $options_short = \%{$self->{OPTIONS_SHORT}};
    my $options_usage = \${$self->{OPTIONS_USAGE}};

    my $refoptions_long = shift;
    my $refoptions_short = shift;
    $$options_usage = shift;

    %$options_long = %{$refoptions_long};
    %$options_short = %{$refoptions_short};

    $self->options_set("default");

    my $c_files = \@{$self->{C_FILES}};
    my $h_files = \@{$self->{H_FILES}};
    my @files;

    while(defined($_ = shift @ARGV)) {
	if(/^--(all|none)$/) {
	    $self->options_set("$1");
	    next;
	} elsif(/^-([^=]*)(=(.*))?$/) {
	    my $name;
	    my $value;
	    if(defined($2)) {
		$name = $1;
		$value = $3;
            } else {
		$name = $1;
	    }
	    
	    if($name =~ /^([^-].*)$/) {
		$name = $$options_short{$1};
	    } else {
		$name =~ s/^-(.*)$/$1/;
	    }
	    	   
	    my $prefix;
	    if(defined($name) && $name =~ /^no-(.*)$/) {
		$name = $1;
		$prefix = "no";
		if(defined($value)) {
		    $output->write("options with prefix 'no' can't take parameters\n");

		    return undef;
		}
	    }

	    my $option;
	    if(defined($name)) {
		$option = $$options_long{$name};
	    }

	    if(defined($option)) {
		my $key = $$option{key};
		my $parser = $$option{parser};
		my $refvalue = \${$self->{$key}};
		my @parents = ();
		
		if(defined($$option{parent})) {
		    if(ref($$option{parent}) eq "ARRAY") {
			@parents = @{$$option{parent}};
		    } else {
			@parents = $$option{parent};
		    }
		}

		if(defined($parser)) { 
		    $$refvalue = &$parser($prefix,$value);
		} else {
		    if(defined($value)) {
			$$refvalue = $value;
		    } elsif(!defined($prefix)) {
			$$refvalue = 1;
		    } else {
			$$refvalue = 0;
		    }
		}

		if((ref($$refvalue) eq "HASH" && $$refvalue->{active}) || $$refvalue) {
		    while($#parents >= 0) {
			my @old_parents = @parents;
			@parents = ();
			foreach my $parent (@old_parents) {
			    my $parentkey = $$options_long{$parent}{key};
			    my $refparentvalue = \${$self->{$parentkey}};
			    
			    $$refparentvalue = 1;

			    if(defined($$options_long{$parent}{parent})) {
				if(ref($$options_long{$parent}{parent}) eq "ARRAY") {
				    push @parents, @{$$options_long{$parent}{parent}};
				} else {
				    push @parents, $$options_long{$parent}{parent};
				}
			    }
			}
		    }
		}
		next;
	    }
	}
	
	if(/^-(.*)$/) {
	    $output->write("unknown option: $_\n"); 
	    $output->write($$options_usage);
	    exit 1;
	} else {
	    if(!-e $_) {
		$output->write("$_: no such file or directory\n");
		exit 1;
	    }

	    push @files, $_;
	}
    }

    if($self->help) {
	$output->write($$options_usage);
	$self->show_help;
	exit 0;
    }

    my @paths = ();
    my @c_files = ();
    my @h_files = ();
    foreach my $file (@files) {
	if($file =~ /\.c$/) {
	    push @c_files, $file;
	} elsif($file =~ /\.h$/) {
	    push @h_files, $file;
	} else {
	    push @paths, $file;
	}
    }

    if($#c_files == -1 && $#h_files == -1 && $#paths == -1)
    {
        @paths = ".";
    }

    if($#paths != -1 || $#c_files != -1) {
	my $c_command = "find " . join(" ", @paths, @c_files) . " -name \\*.c";
	my %found;
	@$c_files = sort(map {
	    s/^\.\/(.*)$/$1/;
	    if(defined($found{$_}) || /glue\.c|spec\.c$/) {
		();
	    } else {
		$found{$_}++;
		$_;
	    }
	} split(/\n/, `$c_command`));
    }

    if($#h_files != -1) {
	my $h_command = "find " . join(" ", @h_files) . " -name \\*.h";
	my %found;

	@$h_files = sort(map {
	    s/^\.\/(.*)$/$1/;
	    if(defined($found{$_})) {
		();
	    } else {
		$found{$_}++;
		$_;
	    }
	} split(/\n/, `$h_command`));
    }

    return $self;
}

sub DESTROY {
}

sub options_set {
    my $self = shift;

    my $options_long = \%{$self->{OPTIONS_LONG}};
    my $options_short = \%{$self->{OPTIONS_SHORT}};

    local $_ = shift;
    for my $name (sort(keys(%$options_long))) {
        my $option = $$options_long{$name};
	my $key = uc($name);
	$key =~ tr/-/_/;
	$$option{key} = $key;
	my $refvalue = \${$self->{$key}};

	if(/^default$/) {
	    $$refvalue = $$option{default};
	} elsif(/^all$/) {
	    if($name !~ /^help|debug|verbose|module$/) {
		if(ref($$refvalue) ne "HASH") {
		    $$refvalue = 1;
		} else {
		    $$refvalue = { active => 1, filter => 0, hash => {} };
		}
	    }
	} elsif(/^none$/) {
	    if($name !~ /^help|debug|verbose|module$/) {
		if(ref($$refvalue) ne "HASH") {
		    $$refvalue = 0;
		} else {
		    $$refvalue = { active => 0, filter => 0, hash => {} };
		}
	    }
	}
    }
}

sub show_help {
    my $self = shift;

    my $options_long = \%{$self->{OPTIONS_LONG}};
    my $options_short = \%{$self->{OPTIONS_SHORT}};

    my $maxname = 0;
    for my $name (sort(keys(%$options_long))) {
	if(length($name) > $maxname) {
	    $maxname = length($name);
	}
    }

    for my $name (sort(keys(%$options_long))) {
	my $option = $$options_long{$name};
        my $description = $$option{description};
	my $default = $$option{default};
	my $current = ${$self->{$$option{key}}};

	my $value = $current;
	
	my $command;
	if(ref($value) ne "HASH") {
	    if($value) {
		$command = "--no-$name";
	    } else {
		$command = "--$name";
	    }
	} else {
	    if($value->{active}) {
		$command = "--[no-]$name\[=<value>]";
	    } else {
		$command = "--$name\[=<value>]";
	    }
	}

	$output->write($command);
	for (0..(($maxname - length($name) + 17) - (length($command) - length($name) + 1))) { $output->write(" "); }
	if(ref($value) ne "HASH") {
	    if($value) {
		$output->write("Disable ");
	    } else {
		$output->write("Enable ");
	    }    
	} else {
	    if($value->{active}) {
		$output->write("(Disable) ");
	    } else {
	        $output->write("Enable ");
	    }
	}
        if($default == $current) {
	    $output->write("$description (default)\n");
	} else {
	    $output->write("$description\n");
	}    
    }
}

sub AUTOLOAD {
    my $self = shift;

    my $name = $_options::AUTOLOAD;
    $name =~ s/^.*::(.[^:]*)$/\U$1/;

    my $refvalue = $self->{$name};
    if(!defined($refvalue)) {
	die "<internal>: options.pm: member $name does not exists\n"; 
    }

    if(ref($$refvalue) ne "HASH") {
	return $$refvalue;
    } else {
	return $$refvalue->{active};
    }
}

sub c_files { my $self = shift; return @{$self->{C_FILES}}; }

sub h_files { my $self = shift; return @{$self->{H_FILES}}; }

1;
