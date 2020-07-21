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

package options;

use strict;
use warnings 'all';

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw($options parse_comma_list parse_value);

use vars qw($options);

use output qw($output);

sub parse_comma_list($$) {
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

sub parse_value($$) {
    my $prefix = shift;
    my $value = shift;

    return $value;
}

package _options;

use strict;
use warnings 'all';

use output qw($output);

sub options_set($$);

sub new($$$$) {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    my $options_long = \%{$self->{_OPTIONS_LONG}};
    my $options_short = \%{$self->{_OPTIONS_SHORT}};
    my $options_usage = \${$self->{_OPTIONS_USAGE}};

    my $refoptions_long = shift;
    my $refoptions_short = shift;
    $$options_usage = shift;

    %$options_long = %{$refoptions_long};
    %$options_short = %{$refoptions_short};

    $self->options_set("default");

    my $arguments = \@{$self->{_ARGUMENTS}};
    @$arguments = ();

    my $end_of_options = 0;
    while(defined($_ = shift @ARGV)) {
	if(/^--$/) {
	    $end_of_options = 1;
	    next;
	} elsif($end_of_options) {
	    # Nothing
	} elsif(/^--(all|none)$/) {
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
		    if(!defined($value)) {
			$value = shift @ARGV;
		    }
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

	if(!$end_of_options && /^-(.*)$/) {
	    $output->write("unknown option: $_\n");
	    $output->write($$options_usage);
	    exit 1;
	} else {
	    push @$arguments, $_;
	}
    }

    if($self->help) {
	$output->write($$options_usage);
	$self->show_help;
	exit 0;
    }

    return $self;
}

sub DESTROY {
}

sub parse_files($) {
    my $self = shift;

    my $arguments = \@{$self->{_ARGUMENTS}};
    my $directories = \@{$self->{_DIRECTORIES}};
    my $c_files = \@{$self->{_C_FILES}};
    my $h_files = \@{$self->{_H_FILES}};

    my $error = 0;
    my @files = ();
    foreach (@$arguments) {
	if(!-e $_) {
	    $output->write("$_: no such file or directory\n");
	    $error = 1;
	} else {
	    push @files, $_;
	}
    }
    if($error) {
	exit 1;
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

    if($#c_files == -1 && $#h_files == -1 && $#paths == -1 && -d ".git")
    {
	@$c_files = sort split /\0/, `git ls-files -z \\*.c`;
	@$h_files = sort split /\0/, `git ls-files -z \\*.h`;
    }
    else
    {
        if($#c_files == -1 && $#h_files == -1 && $#paths == -1)
        {
            @paths = ".";
        }

        if($#paths != -1 || $#c_files != -1) {
            my $c_command = "find " . join(" ", @paths, @c_files) . " -name \\*.c";
            my %found;
            @$c_files = sort(map {
                s/^\.\/(.*)$/$1/;
                if(defined($found{$_})) {
                    ();
                } else {
                    $found{$_}++;
                    $_;
                }
                             } split(/\n/, `$c_command`));
        }

        if($#paths != -1 || $#h_files != -1) {
            my $h_command = "find " . join(" ", @paths, @h_files) . " -name \\*.h";
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
    }

    my %dirs;
    foreach my $file (@$c_files, @$h_files) {
	my $dir = $file;
	$dir =~ s%/?[^/]+$%%;
	if(!$dir) { $dir = "."; }
	$dirs{$dir}++
    }

    @$directories = sort(keys(%dirs));
}

sub options_set($$) {
    my $self = shift;

    my $options_long = \%{$self->{_OPTIONS_LONG}};
    my $options_short = \%{$self->{_OPTIONS_SHORT}};

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
	    if($name !~ /^(?:help|debug|verbose|module)$/) {
		if(ref($$refvalue) ne "HASH") {
		    $$refvalue = 1;
		} else {
		    $$refvalue = { active => 1, filter => 0, hash => {} };
		}
	    }
	} elsif(/^none$/) {
	    if($name !~ /^(?:help|debug|verbose|module)$/) {
		if(ref($$refvalue) ne "HASH") {
		    $$refvalue = 0;
		} else {
		    $$refvalue = { active => 0, filter => 0, hash => {} };
		}
	    }
	}
    }
}

sub show_help($) {
    my $self = shift;

    my $options_long = \%{$self->{_OPTIONS_LONG}};
    my $options_short = \%{$self->{_OPTIONS_SHORT}};

    my $maxname = 0;
    for my $name (sort(keys(%$options_long))) {
	if(length($name) > $maxname) {
	    $maxname = length($name);
	}
    }

    for my $name (sort(keys(%$options_long))) {
	my $option = $$options_long{$name};
        my $description = $$option{description};
	my $parser = $$option{parser};
	my $current = ${$self->{$$option{key}}};

	my $value = $current;

	my $command;
	if(!defined $parser) {
	    if($value) {
		$command = "--no-$name";
	    } else {
		$command = "--$name";
	    }
	} else {
	    if(ref($value) eq "HASH" && $value->{active}) {
		$command = "--[no-]$name\[=<value>]";
	    } else {
		$command = "--$name\[=<value>]";
	    }
	}

	$output->write($command);
	$output->write(" " x (($maxname - length($name) + 17) - (length($command) - length($name) + 1)));
	if(!defined $parser) {
	    if($value) {
		$output->write("Disable ");
	    } else {
		$output->write("Enable ");
	    }
	} else {
	    if(ref($value) eq "HASH")
            {
                if ($value->{active}) {
                    $output->write("(Disable) ");
                } else {
                    $output->write("Enable ");
                }
            }
	}
        $output->write("$description\n");
    }
}

sub AUTOLOAD {
    my $self = shift;

    my $name = $_options::AUTOLOAD;
    $name =~ s/^.*::(.[^:]*)$/\U$1/;

    my $refvalue = $self->{$name};
    if(!defined($refvalue)) {
	die "<internal>: options.pm: member $name does not exist\n";
    }

    if(ref($$refvalue) ne "HASH") {
	return $$refvalue;
    } else {
	return $$refvalue->{active};
    }
}

sub arguments($) {
    my $self = shift;

    my $arguments = \@{$self->{_ARGUMENTS}};

    return @$arguments;
}

sub c_files($) {
    my $self = shift;

    my $c_files = \@{$self->{_C_FILES}};

    if(!@$c_files) {
	$self->parse_files;
    }

    return @$c_files;
}

sub h_files($) {
    my $self = shift;

    my $h_files = \@{$self->{_H_FILES}};

    if(!@$h_files) {
	$self->parse_files;
    }

    return @$h_files;
}

sub directories($) {
    my $self = shift;

    my $directories = \@{$self->{_DIRECTORIES}};

    if(!@$directories) {
	$self->parse_files;
    }

    return @$directories;
}

1;
