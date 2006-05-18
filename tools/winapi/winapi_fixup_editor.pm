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

package winapi_fixup_editor;

use strict;

use options qw($options);
use output qw($output);
use winapi qw($win16api $win32api @winapis);

use util;

sub new($$) {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    my $file = \${$self->{FILE}};

    $$file = shift;

    return $self;
}

sub add_trigger($$$) {
    my $self = shift;

    my $triggers = \%{$self->{TRIGGERS}};

    my $line = shift;
    my $action = shift;

    if(!defined($$triggers{$line})) {
	$$triggers{$line} = [];
    }

    push @{$$triggers{$line}}, $action;
}

sub replace($$$$$$) {
    my $self = shift;

    my $begin_line = shift;
    my $begin_column = shift;
    my $end_line = shift;
    my $end_column = shift;
    my $replace = shift;

    my $file = \${$self->{FILE}};

    my $line = $begin_line;
    my $action = {};

    $self->add_trigger($begin_line, {
	type => "replace",
	begin_line => $begin_line,
	begin_column => $begin_column,
	end_line => $end_line,
	end_column => $end_column,
	replace => $replace
    });
}

sub flush($) {
    my $self = shift;

    my $file = \${$self->{FILE}};
    my $triggers = \%{$self->{TRIGGERS}};

    my $editor = sub {
	local *IN = shift;
	local *OUT = shift;

	my $modified = 0;

	my $again = 0;
	my $lookahead = 0;
	my $lookahead_count = 0;
	LINE: while($again || defined(my $current = <IN>)) {
	    if(!$again) {
		chomp $current;

		if($lookahead) {
		    $lookahead = 0;
		    $_ .= "\n" . $current;
		    $lookahead_count++;
		} else {
		    $_ = $current;
		    $lookahead_count = 0;
		}
	    } else {
		$lookahead_count = 0;
		$again = 0;
	    }

	    my $line = $. - $lookahead_count;
	    foreach my $action (@{$$triggers{$line}}) {
    		if($. < $action->{end_line}) {
		    $lookahead = 1;
		    next LINE;
		}

		my $type = $action->{type};
		my $begin_line = $action->{begin_line};
		my $begin_column = $action->{begin_column};
		my $end_line = $action->{end_line};
		my $end_column = $action->{end_column};

		if($type eq "replace") {
		    my $replace = $action->{replace};

		    my @lines = split(/\n/, $_);
		    if($#lines < 0) {
			@lines = ($_);
		    }

		    my $begin = "";
		    my $column = 0;
		    $_ = $lines[0];
		    while($column < $begin_column - 1 && s/^.//) {
			$begin .= $&;
			if($& eq "\t") {
			    $column = $column + 8 - $column % 8;
			} else {
			    $column++;
			}
		    }

		    my $column2 = 0;
		    $_ = $lines[$#lines];
		    while($column2 < $end_column && s/^.//) {
			if($& eq "\t") {
			    $column2 = $column2 + 8 - $column2 % 8;
			} else {
			    $column2++;
			}
		    }
		    my $end = $_;

		    $_ = "$begin$replace$end";
		    if($options->modify) {
			$modified = 1;
		    } else {
			$output->write("$$file:$begin_line.$begin_column-$end_line.$end_column: $replace\n");
		    }
		}
	    }

	    print OUT "$_\n";
	}

	return $modified;
    };

    my $modified = 0;
    if(1) {
	$modified = edit_file($$file, $editor);
    }

    if(!$modified) {
	$self->flush_old;
    }
}

########################################################################
# Hack for backward compabillity
#

my %insert_line;
my %substitute_line;
my %delete_line;

my %spec_file;

sub flush_old($) {
    my $self = shift;

    my $file = ${$self->{FILE}};

    my $editor = sub {
	local *IN = shift;
	local *OUT = shift;

	my $modified = 0;
	while(<IN>) {
	    chomp;

	    my $line;

	    $line = $insert_line{$.};
	    if(defined($line)) {
		if($options->modify) {
		    $_ = "$line$_";
		    $modified = 1;
		} else {
		    my $line2 = $line; chomp($line2);
		    my @line2 = split(/\n/, $line2);
		    if($#line2 > 0) {
			$output->write("$file: $.: insert: \\\n");
			foreach my $line2 (@line2) {
			    $output->write("'$line2'\n");
			}
		    } else {
			$output->write("$file: $.: insert: '$line2'\n");
		    }
		}
	    }

	    my $search = $substitute_line{$.}{search};
	    my $replace = $substitute_line{$.}{replace};

	    if(defined($search) && defined($replace)) {
		my $modified2 = 0;
		if(s/$search/$replace/) {
		    if($options->modify) {
			$modified = 1;
		    }
		    $modified2 = 1;
		}

		if(!$options->modify || !$modified2) {
		    my $search2;
		    my $replace2;
		    if(!$modified2) {
			$search2 = "unmatched search";
			$replace2 = "unmatched replace";
		    } else {
			$search2 = "search";
			$replace2 = "replace";
		    }
		    $output->write("$file: $.: $search2 : '$search'\n");

		    my @replace2 = split(/\n/, $replace);
		    if($#replace2 > 0) {
			$output->write("$file: $.: $replace2: \\\n");
			foreach my $replace2 (@replace2) {
			    $output->write("'$replace2'\n");
			}
		    } else {
			$output->write("$file: $.: $replace2: '$replace'\n");
		    }
		}
	    }

	    $line = $delete_line{$.};
	    if(defined($line)) {
		if(/$line/) {
		    if($options->modify) {
			$modified = 1;
			next;
		    } else {
			$output->write("$file: $.: delete: '$line'\n");
		    }
		} else {
		    $output->write("$file: $.: unmatched delete: '$line'\n");
		}
	    }

	    print OUT "$_\n";
	}

	return $modified;
    };

    my $n = 0;
    while(defined(each %insert_line)) { $n++; }
    while(defined(each %substitute_line)) { $n++; }
    while(defined(each %delete_line)) { $n++; }
    if($n > 0) {
	edit_file($file, $editor);
    }

    foreach my $module (sort(keys(%spec_file))) {
	my $file;
	foreach my $winapi (@winapis) {
	    $file = ($winapi->module_file($module) || $file);
	}

	if(defined($file)) {
	    $file = file_normalize($file);
	}

	my @substitutes = @{$spec_file{$module}};

	my $editor = sub {
	    local *IN = shift;
	    local *OUT = shift;

	    my $modified = 0;
	    while(<IN>) {
		chomp;

		my @substitutes2 = ();
		foreach my $substitute (@substitutes) {
		    my $search = $substitute->{search};
		    my $replace = $substitute->{replace};

		    if(s/$search/$replace/) {
			if($options->modify) {
			    $modified = 1;
			} else {
			    $output->write("$file: search : '$search'\n");
			    $output->write("$file: replace: '$replace'\n");
			}
			next;
		    } else {
			push @substitutes2, $substitute;
		    }
		}
		@substitutes = @substitutes2;

		print OUT "$_\n";
	    }

	    return $modified;
	};

	if(defined($file)) {
	    edit_file($file, $editor);
	} else {
	    $output->write("$module: doesn't have any spec file\n");
	}

	if($#substitutes >= 0) {
	    foreach my $substitute (@substitutes) {
		my $search = $substitute->{search};
		my $replace = $substitute->{replace};

		$output->write("$file: unmatched search : '$search'\n");
		$output->write("$file: unmatched replace: '$replace'\n");
	    }
	}

    }

    %insert_line = ();
    %substitute_line = ();
    %delete_line = ();

    %spec_file = ();
}

sub delete_line($$$) {
    my $self = shift;

    my $line = shift;
    my $pattern = shift;

    $delete_line{$line} = $pattern;
}

sub insert_line($$$) {
    my $self = shift;

    my $line = shift;
    my $insert = shift;

    $insert_line{$line} = $insert;
}

sub substitute_line($$$$) {
    my $self = shift;

    my $line = shift;
    my $search = shift;
    my $replace = shift;

    $substitute_line{$line}{search} = $search;
    $substitute_line{$line}{replace} = $replace;
}

sub replace_spec_file($$$$) {
    my $self = shift;

    my $module = shift;
    my $search = shift;
    my $replace = shift;

    my $substitute = {};
    $substitute->{search} = $search;
    $substitute->{replace} = $replace;

    if(!defined($spec_file{$module})) {
	$spec_file{$module} = [];
    }

    push @{$spec_file{$module}}, $substitute;
}

1;
