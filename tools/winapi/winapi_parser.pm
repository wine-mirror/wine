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

package winapi_parser;

use strict;
use warnings 'all';

use output qw($output);
use options qw($options);

# Defined a couple common regexp tidbits
my $CALL_CONVENTION="__cdecl|__stdcall|" .
                    "__RPC_API|__RPC_STUB|__RPC_USER|RPC_ENTRY|" .
		    "RPC_VAR_ENTRY|STDMETHODCALLTYPE|NET_API_FUNCTION|" .
                    "CALLBACK|CDECL|NTAPI|PASCAL|APIENTRY|" .
		    "SEC_ENTRY|VFWAPI|VFWAPIV|WINGDIPAPI|WMIAPI|WINAPI|WINAPIV|";

sub parse_c_file($$) {
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
		if($type =~ /^(?:enum|interface|struct|union)/) {
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
    readmore: while($again || defined(my $line = <IN>)) {
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

        my $prefix="";
        while ($_ ne "")
        {
            if (s/^([^\"\/]+|\"(?:[^\\\"]*|\\.)*\")//)
            {
                $prefix.=$1;
            }
            elsif (/^\/\*/)
            {
                # remove C comments
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
                next readmore;
            }
            elsif (s/^(\/\/.*)$//)
            {
                # remove C++ comments
                &$cplusplus_comment_found_callback($., $1);
                $again = 1;
            }
            elsif (s/^(.)//)
            {
                $prefix.=$1;
            }
        }
        $_=$prefix;

	# remove preprocessor directives
	if(s/^\s*\#/\#/s) {
            if (/^#\s*if\s+0\s*$/ms) {
                # Skip #if 0 ... #endif sections entirely.
                # They are typically used as 'super comments' and may not
                # contain C code. This totally ignores nesting.
                if(s/^(\s*#\s*if\s+0\s*\n.*?\n\s*#\s*endif\s*)\n//s) {
                    my @lines = split(/\n/, $1);
                    $_ = "\n" x $#lines;
                    &$preprocessor_found_callback("if", "0");
                    $again = 1;
                } else {
                    $lookahead = 1;
                }
                next readmore;
            }
	    elsif(/^(\#.*?)\\$/s) {
		$_ = "$1\n";
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
		    if(/^\s*((?:(?:FAR\s*)?\*\s*(?:RESTRICTED_POINTER\s+)?)?
			    (?:volatile\s+)?
			    (?:\w+|WS\(\w+\))\s*
			    (?:\s*,\s*(?:(?:FAR\s*)?\*+\s*(?:RESTRICTED_POINTER\s+)?)?(?:volatile\s+)?(?:\w+|WS\(\w+\)))*\s*);/sx) {
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
	} elsif(/(extern\s+|static\s+)?((interface\s+|struct\s+|union\s+|enum\s+|signed\s+|unsigned\s+)?\w+((\s*\*)+\s*|\s+))
            (($CALL_CONVENTION)\s+)?
            (?:DECLSPEC_HOTPATCH\s+)?
	    (\w+(\(\w+\))?)\s*\((.*?)\)\s*(\{|\;)/sx)
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
	    my @arguments;
	    my $n = 0;
	    while ($arguments =~ s/^((?:[^,\(\)]*|(?:\([^\)]*\))?)+)(?:,|$)// && $1) {
		my $argument = $1;
		push @arguments, $argument;

		my $argument_type = "";
		my $argument_name = "";

		$argument =~ s/^\s*(.*?)\s*$/$1/;
		# print "  " . ($n + 1) . ": '$argument'\n";
		$argument =~ s/^(?:IN OUT|IN|OUT)?\s+//;
		$argument =~ s/^(?:const|CONST|GDIPCONST|volatile)?\s+//;
		if($argument =~ /^\.\.\.$/) {
		    $argument_type = "...";
		    $argument_name = "...";
		} elsif($argument =~ /^
			((?:interface\s+|struct\s+|union\s+|enum\s+|register\s+|(?:signed\s+|unsigned\s+)?
			  (?:short\s+(?=int)|long\s+(?=int))?)?(?:\w+|ElfW\(\w+\)|WS\(\w+\)))\s*
			((?:__RPC_FAR|const|CONST|GDIPCONST|volatile)?\s*(?:\*\s*(?:__RPC_FAR|const|CONST|volatile)?\s*?)*)\s*
			(\w*)\s*(\[\])?(?:\s+OPTIONAL)?$/x)
		{
		    $argument_type = $1;
		    if ($2) {
			$argument_type .= " $2";
		    }
		    if ($4) {
			$argument_type .= "$4";
		    }
		    $argument_name = $3;
		} elsif ($argument =~ /^
			((?:interface\s+|struct\s+|union\s+|enum\s+|register\s+|(?:signed\s+|unsigned\s+)?
			  (?:short\s+(?=int)|long\s+(?=int))?)?\w+)\s*
			((?:const|volatile)?\s*(?:\*\s*(?:const|volatile)?\s*?)*)\s*
			(?:(?:$CALL_CONVENTION)\s+)?
			\(\s*(?:$CALL_CONVENTION)?\s*\*\s*((?:\w+)?)\s*\)\s*
			\(\s*(.*?)\s*\)$/x) 
		{
		    my $return_type = $1;
		    if($2) {
			$return_type .= " $2";
		    }
		    $argument_name = $3;
		    my $arguments = $4;

		    $return_type =~ s/\s+/ /g;
		    $arguments =~ s/\s*,\s*/,/g;
		    
		    $argument_type = "$return_type (*)($arguments)";
		} elsif ($argument =~ /^
			((?:interface\s+|struct\s+|union\s+|enum\s+|register\s+|(?:signed\s+|unsigned\s+)
			  (?:short\s+(?=int)|long\s+(?=int))?)?\w+)\s*
			((?:const|volatile)?\s*(?:\*\s*(?:const|volatile)?\s*?)*)\s*
			(\w+)\s*\[\s*(.*?)\s*\](?:\[\s*(.*?)\s*\])?$/x)
		{
		    my $return_type = $1;
		    if($2) {
			$return_type .= " $2";
		    }
		    $argument_name = $3;

		    $argument_type = "$return_type\[$4\]";

		    if (defined($5)) {
			$argument_type .= "\[$5\]";
		    }

		    # die "$file: $.: syntax error: '$argument_type':'$argument_name'\n";
		} else {
                    # This is either a complex argument type, typically
                    # involving parentheses, or a macro argument. This is rare
                    # so just ignore the 'function' declaration.
		    print STDERR "$file: $.: cannot parse declaration argument (ignoring): '$argument'\n";
                    next readmore;
		}

		$argument_type =~ s/\s*(?:const|volatile)\s*/ /g; # Remove const/volatile
		$argument_type =~ s/([^\*\(\s])\*/$1 \*/g; # Assure whitespace between non-* and *
		$argument_type =~ s/,([^\s])/, $1/g; # Assure whitespace after ,
		$argument_type =~ s/\*\s+\*/\*\*/g; # Remove whitespace between * and *
		$argument_type =~ s/([\(\[])\s+/$1/g; # Remove whitespace after ( and [
		$argument_type =~ s/\s+([\)\]])/$1/g; # Remove whitespace before ] and )
		$argument_type =~ s/\s+/ /; # Remove multiple whitespace
		$argument_type =~ s/^\s*(.*?)\s*$/$1/; # Remove leading and trailing whitespace

		$argument_name =~ s/^\s*(.*?)\s*$/$1/; # Remove leading and trailing whitespace

		$argument_types[$n] = $argument_type;
		$argument_names[$n] = $argument_name;
		# print "  " . ($n + 1) . ": '" . $argument_types[$n] . "', '" . $argument_names[$n] . "'\n";

		$n++;
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
	} elsif(/DEFINE_THISCALL_WRAPPER\((\S*)\)/s) {
	    my @lines = split(/\n/, $&);
	    my $function_line = $. - scalar(@lines) + 1;

	    $_ = $'; $again = 1;

	    &$function_begin($documentation_line, $documentation,
			     $function_line, "", "void", "", "__thiscall_" . $1, \());
	    &$function_end($function_line, "");
        } elsif(/DEFINE_REGS_ENTRYPOINT_\d+\(\s*(\S*)\s*,\s*([^\s,\)]*).*?\)/s) {
	    $_ = $'; $again = 1;
	    $regs_entrypoints{$2} = $1;
	} elsif(/DEFAULT_DEBUG_CHANNEL\s*\((\S+)\)/s) {
	    $_ = $'; $again = 1;
	    unshift @$debug_channels, $1;
	} elsif(/(DEFAULT|DECLARE)_DEBUG_CHANNEL\s*\((\S+)\)/s) {
	    $_ = $'; $again = 1;
	    push @$debug_channels, $1;
	} elsif(/typedef\s+(enum|interface|struct|union)(?:\s+DECLSPEC_ALIGN\(\d+\))?(?:\s+(\w+))?\s*\{/s) {
	    $_ = $'; $again = 1;
	    $level++;
	    my $type = $1;
	    if(defined($2)) {
	       $type .= " $2";
	    }
	    &$type_begin($type);
	} elsif(/typedef\s+
		((?:const\s+|CONST\s+|enum\s+|interface\s+|long\s+|signed\s+|short\s+|struct\s+|union\s+|unsigned\s+|volatile\s+)*?)
		(\w+)
		(?:\s+const|\s+volatile)?
		((?:\s*(?:(?:FAR|__RPC_FAR|TW_HUGE)?\s*)?\*+\s*|\s+)(?:volatile\s+|DECLSPEC_ALIGN\(\d+\)\s+)?\w+\s*(?:\[[^\]]*\])*
		(?:\s*,\s*(?:\s*(?:(?:FAR|__RPC_FAR|TW_HUGE)?\s*)?\*+\s*|\s+)\w+\s*(?:\[[^\]]*\])?)*)
		\s*;/sx)
	{
	    $_ = $'; $again = 1;

	    my $type = "$1 $2";

	    my @names;
	    my @parts = split(/\s*,\s*/, $2);
	    foreach my $part (@parts) {
		if($part =~ /(?:\s*((?:(?:FAR|__RPC_FAR|TW_HUGE)?\s*)?\*+)\s*|\s+)(\w+)\s*(\[[^\]]*\])?/) {
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
		(?:(?:const\s+|enum\s+|interface\s+|long\s+|signed\s+|short\s+|struct\s+|union\s+|unsigned\s+|volatile\s+)*?)
		(\w+(?:\s*\*+\s*)?)\s*
		(?:(\w+)\s*)?
		\((?:(\w+)\s*)?\s*(?:\*\s*(\w+)|_ATL_CATMAPFUNC)\s*\)\s*
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
	    $output->write("$file: $.: could not parse typedef: '$&'\n");
	} elsif(/typedef[^\{]*\{[^\}]*\}[^;];/s) {
	    $_ = $'; $again = 1;
	    $output->write("$file: $.: could not parse multi-line typedef: '$&'\n");
	} elsif(/\'[^\']*\'/s) {
	    $_ = $'; $again = 1;
	} elsif(/\"(?:[^\\\"]*|\\.)*\"/s) {
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
