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
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

package winapi_parser;

use strict;

use output qw($output);
use options qw($options);

sub parse_c_file {
    my $file = shift;
    my $callbacks = shift;

    my $empty_callback = sub { };

    my $c_comment_found_callback = $$callbacks{c_comment_found} || $empty_callback;
    my $cplusplus_comment_found_callback = $$callbacks{cplusplus_comment_found} || $empty_callback;
    my $function_create_callback = $$callbacks{function_create} || $empty_callback;
    my $function_found_callback = $$callbacks{function_found} || $empty_callback;
    my $type_create_callback = $$callbacks{type_create} || $empty_callback;
    my $type_found_callback = $$callbacks{type_found} || $empty_callback;
    my $preprocessor_found_callback = $$callbacks{preprocessor_found} || $empty_callback;

    # global
    my $debug_channels = [];

    my $in_function = 0;
    my $function_begin;
    my $function_end;
    {
	my $documentation_line;
	my $documentation;
	my $function_line;
	my $linkage;
	my $return_type;
	my $calling_convention;
	my $internal_name = "";
	my $argument_types;
	my $argument_names;
	my $argument_documentations;
	my $statements_line;
	my $statements;

	$function_begin = sub {
	    $documentation_line = shift;
	    $documentation = shift;
	    $function_line = shift;
	    $linkage = shift;
	    $return_type= shift;
	    $calling_convention = shift;
	    $internal_name = shift;
	    $argument_types = shift;
	    $argument_names = shift;
	    $argument_documentations = shift;

	    if(defined($argument_names) && defined($argument_types) &&
	       $#$argument_names == -1)
	    {
		foreach my $n (0..$#$argument_types) {
		    push @$argument_names, "";
		}
	    }

	    if(defined($argument_documentations) &&
	       $#$argument_documentations == -1)
	    {
		foreach my $n (0..$#$argument_documentations) {
		    push @$argument_documentations, "";
		}
	    }

	    $in_function = 1;
	};

	$function_end = sub {
	    $statements_line = shift;
	    $statements = shift;

	    my $function = &$function_create_callback();

	    if(!defined($documentation_line)) {
		$documentation_line = 0;
	    }

	    $function->file($file);
	    $function->debug_channels([@$debug_channels]);
	    $function->documentation_line($documentation_line);
	    $function->documentation($documentation);
	    $function->function_line($function_line);
	    $function->linkage($linkage);
	    $function->return_type($return_type);
	    $function->calling_convention($calling_convention);
	    $function->internal_name($internal_name);
	    if(defined($argument_types)) {
		$function->argument_types([@$argument_types]);
	    }
	    if(defined($argument_names)) {
		$function->argument_names([@$argument_names]);
	    }
	    if(defined($argument_documentations)) {
		$function->argument_documentations([@$argument_documentations]);
	    }
	    $function->statements_line($statements_line);
	    $function->statements($statements);

	    &$function_found_callback($function);

	    $in_function = 0;
	};
    }

    my $in_type = 0;
    my $type_begin;
    my $type_end;
    {
	my $type;

	$type_begin = sub {
	    $type = shift;
	    $in_type = 1;
	};

	$type_end = sub {
	    my $names = shift;

	    foreach my $name (@$names) {
		if($type =~ /^(?:struct|enum)/) {
		    # $output->write("typedef $type {\n");
		    # $output->write("} $name;\n");
		} else {
		    # $output->write("typedef $type $name;\n");
		}
	    }
	    $in_type = 0;
	};
    }

    my %regs_entrypoints;
    my @comment_lines = ();
    my @comments = ();
    my $statements_line;
    my $statements;
    my $level = 0;
    my $extern_c = 0;
    my $again = 0;
    my $lookahead = 0;
    my $lookahead_count = 0;

    print STDERR "Processing file '$file' ... " if $options->verbose;
    open(IN, "< $file") || die "<internal>: $file: $!\n";
    local $_ = "";
    while($again || defined(my $line = <IN>)) {
	$_ = "" if !defined($_);
	if(!$again) {
	    chomp $line;

	    if($lookahead) {
		$lookahead = 0;
		$_ .= "\n" . $line;
		$lookahead_count++;
	    } else {
		$_ = $line;
		$lookahead_count = 0;
	    }
	    $output->write(" $level($lookahead_count): $line\n") if $options->debug >= 2;
	    $output->write("*** $_\n") if $options->debug >= 3;
	} else {
	    $lookahead_count = 0;
	    $again = 0;
	}

	# CVS merge conflicts in file?
	if(/^(<<<<<<<|=======|>>>>>>>)/) {
	    $output->write("$file: merge conflicts in file\n");
	    last;
	}

	# remove C comments
	if(s/^([^\"\/]*?(?:\"[^\"]*?\"[^\"]*?)*?)(?=\/\*)//s) {
	    my $prefix = $1;
	    if(s/^(\/\*.*?\*\/)//s) {
		my @lines = split(/\n/, $1);
		push @comment_lines, $.;
		push @comments, $1;
		&$c_comment_found_callback($. - $#lines, $., $1);
		if($#lines <= 0) {
		    $_ = "$prefix $_";
		} else {
		    $_ = $prefix . ("\n" x $#lines) . $_;
		}
		$again = 1;
	    } else {
		$_ = "$prefix$_";
		$lookahead = 1;
	    }
	    next;
	}

	# remove C++ comments
	while(s/^([^\"\/]*?(?:\"[^\"]*?\"[^\"]*?)*?)(\/\/.*?)$/$1/s) {
	    &$cplusplus_comment_found_callback($., $2);
	    $again = 1;
	}
	if($again) { next; }

	# remove preprocessor directives
	if(s/^\s*\#/\#/s) {
	    if(/^\#.*?\\$/s) {
		$lookahead = 1;
		next;
	    } elsif(s/^\#\s*(\w+)((?:\s+(.*?))?\s*)$//s) {
		my @lines = split(/\n/, $2);
		if($#lines > 0) {
		    $_ = "\n" x $#lines;
		}
		if(defined($3)) {
		    &$preprocessor_found_callback($1, $3);
		} else {
		    &$preprocessor_found_callback($1, "");
		}
		$again = 1;
		next;
	    }
	}

	# Remove extern "C"
	if(s/^\s*extern\s+"C"\s+\{//m) {
	    $extern_c = 1;
	    $again = 1;
	    next;
	}

	my $documentation_line;
	my $documentation;
	my @argument_documentations = ();
	{
	    my $n = $#comments;
	    while($n >= 0 && ($comments[$n] !~ /^\/\*\*/ ||
			      $comments[$n] =~ /^\/\*\*+\/$/))
	    {
		$n--;
	    }

	    if(defined($comments[$n]) && $n >= 0) {
		my @lines = split(/\n/, $comments[$n]);

		$documentation_line = $comment_lines[$n] - scalar(@lines) + 1;
		$documentation = $comments[$n];

		for(my $m=$n+1; $m <= $#comments; $m++) {
		    if($comments[$m] =~ /^\/\*\*+\/$/ ||
		       $comments[$m] =~ /^\/\*\s*(?:\!)?defined/) # FIXME: Kludge
		    {
			@argument_documentations = ();
			next;
		    }
		    push @argument_documentations, $comments[$m];
		}
	    } else {
		$documentation = "";
	    }
	}

	if($level > 0)
	{
	    my $line = "";
	    while(/^[^\{\}]/) {
		s/^([^\{\}\'\"]*)//s;
		$line .= $1;
	        if(s/^\'//) {
		    $line .= "\'";
		    while(/^./ && !s/^\'//) {
			s/^([^\'\\]*)//s;
			$line .= $1;
			if(s/^\\//) {
			    $line .= "\\";
			    if(s/^(.)//s) {
				$line .= $1;
				if($1 eq "0") {
				    s/^(\d{0,3})//s;
				    $line .= $1;
				}
			    }
			}
		    }
		    $line .= "\'";
		} elsif(s/^\"//) {
		    $line .= "\"";
		    while(/^./ && !s/^\"//) {
			s/^([^\"\\]*)//s;
			$line .= $1;
			if(s/^\\//) {
			    $line .= "\\";
			    if(s/^(.)//s) {
				$line .= $1;
				if($1 eq "0") {
				    s/^(\d{0,3})//s;
				    $line .= $1;
				}
			    }
			}
		    }
		    $line .= "\"";
		}
	    }

	    if(s/^\{//) {
		$_ = $'; $again = 1;
		$line .= "{";
		print "+1: \{$_\n" if $options->debug >= 2;
		$level++;
		$statements .= $line;
	    } elsif(s/^\}//) {
		$_ = $'; $again = 1;
		$line .= "}" if $level > 1;
		print "-1: \}$_\n" if $options->debug >= 2;
		$level--;
		if($level == -1 && $extern_c) {
		    $extern_c = 0;
		    $level = 0;
		}
		$statements .= $line;
	    } else {
		$statements .= "$line\n";
	    }

	    if($level == 0) {
		if($in_function) {
		    &$function_end($statements_line, $statements);
		    $statements = undef;
		} elsif($in_type) {
		    if(/^\s*(?:WINE_PACKED\s+)?((?:\*\s*)?\w+\s*(?:\s*,\s*(?:\*+\s*)?\w+)*\s*);/s) {
			my @parts = split(/\s*,\s*/, $1);
			&$type_end([@parts]);
		    } elsif(/;/s) {
			die "$file: $.: syntax error: '$_'\n";
		    } else {
			$lookahead = 1;
		    }
		}
	    }
	    next;
	} elsif(/(extern\s+|static\s+)?((struct\s+|union\s+|enum\s+|signed\s+|unsigned\s+)?\w+((\s*\*)+\s*|\s+))
            ((__cdecl|__stdcall|CDECL|VFWAPIV|VFWAPI|WINAPIV|WINAPI|CALLBACK)\s+)?
	    (\w+(\(\w+\))?)\s*\(([^\)]*)\)\s*(\{|\;)/sx)
        {
	    my @lines = split(/\n/, $&);
	    my $function_line = $. - scalar(@lines) + 1;

	    $_ = $'; $again = 1;

	    if($11 eq "{")  {
		$level++;
	    }

	    my $linkage = $1;
	    my $return_type = $2;
	    my $calling_convention = $7;
	    my $name = $8;
	    my $arguments = $10;

	    if(!defined($linkage)) {
		$linkage = "";
	    }

	    if(!defined($calling_convention)) {
		$calling_convention = "";
	    }

	    $linkage =~ s/\s*$//;

	    $return_type =~ s/\s*$//;
	    $return_type =~ s/\s*\*\s*/*/g;
	    $return_type =~ s/(\*+)/ $1/g;

	    if($regs_entrypoints{$name}) {
		$name = $regs_entrypoints{$name};
	    }

	    $arguments =~ y/\t\n/  /;
	    $arguments =~ s/^\s*(.*?)\s*$/$1/;
	    if($arguments eq "") { $arguments = "..." }

	    my @argument_types;
	    my @argument_names;
	    my @arguments = split(/,/, $arguments);
	    foreach my $n (0..$#arguments) {
		my $argument_type = "";
		my $argument_name = "";
		my $argument = $arguments[$n];
		$argument =~ s/^\s*(.*?)\s*$/$1/;
		# print "  " . ($n + 1) . ": '$argument'\n";
		$argument =~ s/^(IN OUT(?=\s)|IN(?=\s)|OUT(?=\s)|\s*)\s*//;
		$argument =~ s/^(const(?=\s)|CONST(?=\s)|\s*)\s*//;
		if($argument =~ /^\.\.\.$/) {
		    $argument_type = "...";
		    $argument_name = "...";
		} elsif($argument =~ /^
			((?:struct\s+|union\s+|enum\s+|(?:signed\s+|unsigned\s+)
			  (?:short\s+(?=int)|long\s+(?=int))?)?\w+)\s*
			((?:const)?\s*(?:\*\s*?)*)\s*
			(?:WINE_UNUSED\s+)?(\w*)\s*(?:\[\]|\s+OPTIONAL)?/x)
		{
		    $argument_type = "$1";
		    if($2 ne "") {
			$argument_type .= " $2";
		    }
		    $argument_name = $3;

		    $argument_type =~ s/\s*const\s*/ /;
		    $argument_type =~ s/^\s*(.*?)\s*$/$1/;

		    $argument_name =~ s/^\s*(.*?)\s*$/$1/;
		} else {
		    die "$file: $.: syntax error: '$argument'\n";
		}
		$argument_types[$n] = $argument_type;
		$argument_names[$n] = $argument_name;
		# print "  " . ($n + 1) . ": '" . $argument_types[$n] . "', '" . $argument_names[$n] . "'\n";
	    }
	    if($#argument_types == 0 && $argument_types[0] =~ /^void$/i) {
		$#argument_types = -1;
		$#argument_names = -1;
	    }

	    if($options->debug) {
		print "$file: $return_type $calling_convention $name(" . join(",", @arguments) . ")\n";
	    }

	    &$function_begin($documentation_line, $documentation,
			     $function_line, $linkage, $return_type, $calling_convention, $name,
			     \@argument_types,\@argument_names,\@argument_documentations);
	    if($level == 0) {
		&$function_end(undef, undef);
	    }
	    $statements_line = $.;
	    $statements = "";
	} elsif(/__ASM_GLOBAL_FUNC\(\s*(.*?)\s*,/s) {
	    my @lines = split(/\n/, $&);
	    my $function_line = $. - scalar(@lines) + 1;

	    $_ = $'; $again = 1;

	    &$function_begin($documentation_line, $documentation,
			     $function_line, "", "void", "__asm", $1);
	    &$function_end($., "");
	} elsif(/WAVEIN_SHORTCUT_0\s*\(\s*(.*?)\s*,\s*(.*?)\s*\)/s) {
	    my @lines = split(/\n/, $&);
	    my $function_line = $. - scalar(@lines) + 1;

	    $_ = $'; $again = 1;
	    my @arguments16 = ("HWAVEIN16");
	    my @arguments32 = ("HWAVEIN");
	    &$function_begin($documentation_line, $documentation,
			     $function_line,  "", "UINT16", "WINAPI", "waveIn" . $1 . "16", \@arguments16);
	    &$function_end($., "");
	    &$function_begin($documentation_line, $documentation,
			     $function_line, "", "UINT", "WINAPI", "waveIn" . $1, \@arguments32);
	    &$function_end($., "");
	} elsif(/WAVEOUT_SHORTCUT_0\s*\(\s*(.*?)\s*,\s*(.*?)\s*\)/s) {
	    my @lines = split(/\n/, $&);
	    my $function_line = $. - scalar(@lines) + 1;

	    $_ = $'; $again = 1;

	    my @arguments16 = ("HWAVEOUT16");
	    my @arguments32 = ("HWAVEOUT");
	    &$function_begin($documentation_line, $documentation,
			     $function_line, "", "UINT16", "WINAPI", "waveOut" . $1 . "16", \@arguments16);
	    &$function_end($., "");
	    &$function_begin($documentation_line, $documentation,
			     $function_line, "", "UINT", "WINAPI", "waveOut" . $1, \@arguments32);
	    &$function_end($., "");
	} elsif(/WAVEOUT_SHORTCUT_(1|2)\s*\(\s*(.*?)\s*,\s*(.*?)\s*,\s*(.*?)\s*\)/s) {
	    my @lines = split(/\n/, $&);
	    my $function_line = $. - scalar(@lines) + 1;

	    $_ = $'; $again = 1;

	    if($1 eq "1") {
		my @arguments16 = ("HWAVEOUT16", $4);
		my @arguments32 = ("HWAVEOUT", $4);
		&$function_begin($documentation_line, $documentation,
				 $function_line, "", "UINT16", "WINAPI", "waveOut" . $2 . "16", \@arguments16);
		&$function_end($., "");
		&$function_begin($documentation_line, $documentation,
				 $function_line, "", "UINT", "WINAPI", "waveOut" . $2, \@arguments32);
		&$function_end($., "");
	    } elsif($1 eq 2) {
		my @arguments16 = ("UINT16", $4);
		my @arguments32 = ("UINT", $4);
		&$function_begin($documentation_line, $documentation,
				 $function_line, "", "UINT16", "WINAPI", "waveOut". $2 . "16", \@arguments16);
		&$function_end($., "");
		&$function_begin($documentation_line, $documentation,
				 $function_line, "", "UINT", "WINAPI", "waveOut" . $2, \@arguments32);
		&$function_end($., "");
	    }
        } elsif(/DEFINE_REGS_ENTRYPOINT_\d+\(\s*(\S*)\s*,\s*([^\s,\)]*).*?\)/s) {
	    $_ = $'; $again = 1;
	    $regs_entrypoints{$2} = $1;
	} elsif(/DEFAULT_DEBUG_CHANNEL\s*\((\S+)\)/s) {
	    $_ = $'; $again = 1;
	    unshift @$debug_channels, $1;
	} elsif(/(DEFAULT|DECLARE)_DEBUG_CHANNEL\s*\((\S+)\)/s) {
	    $_ = $'; $again = 1;
	    push @$debug_channels, $1;
	} elsif(/typedef\s+(enum|struct|union)(?:\s+(\w+))?\s*\{/s) {
	    $_ = $'; $again = 1;
	    $level++;
	    my $type = $1;
	    if(defined($2)) {
	       $type .= " $2";
	    }
	    &$type_begin($type);
	} elsif(/typedef\s+
		((?:const\s+|enum\s+|long\s+|signed\s+|short\s+|struct\s+|union\s+|unsigned\s+)*?)
		(\w+)
		(?:\s+const)?
		((?:\s*\*+\s*|\s+)\w+\s*(?:\[[^\]]*\])?
		(?:\s*,\s*(?:\s*\*+\s*|\s+)\w+\s*(?:\[[^\]]*\])?)*)
		\s*;/sx)
	{
	    $_ = $'; $again = 1;

	    my $type = "$1 $2";

	    my @names;
	    my @parts = split(/\s*,\s*/, $2);
	    foreach my $part (@parts) {
		if($part =~ /(?:\s*(\*+)\s*|\s+)(\w+)\s*(\[[^\]]*\])?/) {
		    my $name = $2;
		    if(defined($1)) {
			$name = "$1$2";
		    }
		    if(defined($3)) {
			$name .= $3;
		    }
		    push @names, $name;
		}
	    }
	    &$type_begin($type);
	    &$type_end([@names]);
	} elsif(/typedef\s+
		(?:(?:const\s+|enum\s+|long\s+|signed\s+|short\s+|struct\s+|union\s+|unsigned\s+)*?)
		(\w+(?:\s*\*+\s*)?)\s+
		(?:(\w+)\s*)?
		\((?:(\w+)\s*)?\s*\*\s*(\w+)\s*\)\s*
		(?:\(([^\)]*)\)|\[([^\]]*)\])\s*;/sx)
	{
	    $_ = $'; $again = 1;
	    my $type;
	    if(defined($2) || defined($3)) {
		my $cc = $2 || $3;
		if(defined($5)) {
		    $type = "$1 ($cc *)($5)";
		} else {
		    $type = "$1 ($cc *)[$6]";
		}
	    } else {
		if(defined($5)) {
		    $type = "$1 (*)($5)";
		} else {
		    $type = "$1 (*)[$6]";
		}
	    }
	    my $name = $4;
	    &$type_begin($type);
	    &$type_end([$name]);
	} elsif(/typedef[^\{;]*;/s) {
	    $_ = $'; $again = 1;
	    $output->write("$file: $.: can't parse: '$&'\n");
	} elsif(/typedef[^\{]*\{[^\}]*\}[^;];/s) {
	    $_ = $'; $again = 1;
	    $output->write("$file: $.: can't parse: '$&'\n");
	} elsif(/\'[^\']*\'/s) {
	    $_ = $'; $again = 1;
	} elsif(/\"[^\"]*\"/s) {
	    $_ = $'; $again = 1;
	} elsif(/;/s) {
	    $_ = $'; $again = 1;
	} elsif(/extern\s+"C"\s+{/s) {
	    $_ = $'; $again = 1;
        } elsif(/\{/s) {
            $_ = $'; $again = 1;
            print "+1: $_\n" if $options->debug >= 2;
            $level++;
        } else {
	    $lookahead = 1;
	}
    }
    close(IN);
    print STDERR "done\n" if $options->verbose;
    $output->write("$file: not at toplevel at end of file\n") unless $level == 0;
}

1;
