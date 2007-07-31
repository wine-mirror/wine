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

package c_parser;

use strict;

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw();

use options qw($options);
use output qw($output);

use c_function;
use c_type;

# Defined a couple common regexp tidbits
my $CALL_CONVENTION="__cdecl|__stdcall|" .
                    "__RPC_API|__RPC_STUB|__RPC_USER|" .
		    "CALLBACK|CDECL|NTAPI|PASCAL|RPC_ENTRY|RPC_VAR_ENTRY|" .
		    "VFWAPI|VFWAPIV|WINAPI|WINAPIV|APIENTRY|";


sub parse_c_function($$$$$);
sub parse_c_function_call($$$$$$$$);
sub parse_c_preprocessor($$$$);
sub parse_c_statements($$$$);
sub parse_c_tuple($$$$$$$);
sub parse_c_type($$$$$);
sub parse_c_typedef($$$$);
sub parse_c_variable($$$$$$$);


########################################################################
# new
#
sub new($$) {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    my $file = \${$self->{FILE}};
    my $create_function = \${$self->{CREATE_FUNCTION}};
    my $create_type = \${$self->{CREATE_TYPE}};
    my $found_comment = \${$self->{FOUND_COMMENT}};
    my $found_declaration = \${$self->{FOUND_DECLARATION}};
    my $found_function = \${$self->{FOUND_FUNCTION}};
    my $found_function_call = \${$self->{FOUND_FUNCTION_CALL}};
    my $found_line = \${$self->{FOUND_LINE}};
    my $found_preprocessor = \${$self->{FOUND_PREPROCESSOR}};
    my $found_statement = \${$self->{FOUND_STATEMENT}};
    my $found_type = \${$self->{FOUND_TYPE}};
    my $found_variable = \${$self->{FOUND_VARIABLE}};

    $$file = shift;

    $$create_function = sub { return new c_function; };
    $$create_type = sub { return new c_type; };
    $$found_comment = sub { return 1; };
    $$found_declaration = sub { return 1; };
    $$found_function = sub { return 1; };
    $$found_function_call = sub { return 1; };
    $$found_line = sub { return 1; };
    $$found_preprocessor = sub { return 1; };
    $$found_statement = sub { return 1; };
    $$found_type = sub { return 1; };
    $$found_variable = sub { return 1; };

    return $self;
}

########################################################################
# set_found_comment_callback
#
sub set_found_comment_callback($$) {
    my $self = shift;

    my $found_comment = \${$self->{FOUND_COMMENT}};

    $$found_comment = shift;
}

########################################################################
# set_found_declaration_callback
#
sub set_found_declaration_callback($$) {
    my $self = shift;

    my $found_declaration = \${$self->{FOUND_DECLARATION}};

    $$found_declaration = shift;
}

########################################################################
# set_found_function_callback
#
sub set_found_function_callback($$) {
    my $self = shift;

    my $found_function = \${$self->{FOUND_FUNCTION}};

    $$found_function = shift;
}

########################################################################
# set_found_function_call_callback
#
sub set_found_function_call_callback($$) {
    my $self = shift;

    my $found_function_call = \${$self->{FOUND_FUNCTION_CALL}};

    $$found_function_call = shift;
}

########################################################################
# set_found_line_callback
#
sub set_found_line_callback($$) {
    my $self = shift;

    my $found_line = \${$self->{FOUND_LINE}};

    $$found_line = shift;
}

########################################################################
# set_found_preprocessor_callback
#
sub set_found_preprocessor_callback($$) {
    my $self = shift;

    my $found_preprocessor = \${$self->{FOUND_PREPROCESSOR}};

    $$found_preprocessor = shift;
}

########################################################################
# set_found_statement_callback
#
sub set_found_statement_callback($$) {
    my $self = shift;

    my $found_statement = \${$self->{FOUND_STATEMENT}};

    $$found_statement = shift;
}

########################################################################
# set_found_type_callback
#
sub set_found_type_callback($$) {
    my $self = shift;

    my $found_type = \${$self->{FOUND_TYPE}};

    $$found_type = shift;
}

########################################################################
# set_found_variable_callback
#
sub set_found_variable_callback($$) {
    my $self = shift;

    my $found_variable = \${$self->{FOUND_VARIABLE}};

    $$found_variable = shift;
}


########################################################################
# _format_c_type

sub _format_c_type($$) {
    my $self = shift;

    local $_ = shift;
    s/^\s*(.*?)\s*$/$1/;

    if (/^(\w+(?:\s*\*)*)\s*\(\s*\*\s*\)\s*\(\s*(.*?)\s*\)$/s) {
	my $return_type = $1;
	my @arguments = split(/\s*,\s*/, $2);
	foreach my $argument (@arguments) {
	    if ($argument =~ s/^(\w+(?:\s*\*)*)\s*\w+$/$1/) { 
		$argument =~ s/\s+/ /g;
		$argument =~ s/\s*\*\s*/*/g;
		$argument =~ s/(\*+)$/ $1/;
	    }
	}

	$_ = "$return_type (*)(" . join(", ", @arguments) . ")";
    }
    
    return $_;
}


########################################################################
# _parse_c_warning
#
# FIXME: Use caller (See man perlfunc)

sub _parse_c_warning($$$$$$) {
    my $self = shift;

    local $_ = shift;
    my $line = shift;
    my $column = shift;
    my $context = shift;
    my $message = shift;

    my $file = \${$self->{FILE}};

    $message = "warning" if !$message;

    my $current = "";
    if($_) {
	my @lines = split(/\n/, $_);

	$current .= $lines[0] . "\n" if $lines[0];
        $current .= $lines[1] . "\n" if $lines[1];
    }

    if (0) {
	(my $package, my $filename, my $line) = caller(0);
	$output->write("*** caller ***: $filename:$line\n");
    }

    if($current) {
	$output->write("$$file:$line." . ($column + 1) . ": $context: $message: \\\n$current");
    } else {
	$output->write("$$file:$line." . ($column + 1) . ": $context: $message\n");
    }
}

########################################################################
# _parse_c_error

sub _parse_c_error($$$$$$) {
    my $self = shift;

    local $_ = shift;
    my $line = shift;
    my $column = shift;
    my $context = shift;
    my $message = shift;

    $message = "parse error" if !$message;

    # Why did I do this?
    if($output->prefix) {
	# $output->write("\n");
	$output->prefix("");
    }

    $self->_parse_c_warning($_, $line, $column, $context, $message);

    exit 1;
}

########################################################################
# _update_c_position

sub _update_c_position($$$$) {
    my $self = shift;

    local $_ = shift;
    my $refline = shift;
    my $refcolumn = shift;

    my $line = $$refline;
    my $column = $$refcolumn;

    while($_) {
	if(s/^[^\n\t\'\"]*//s) {
	    $column += length($&);
	}

	if(s/^\'//) {
	    $column++;
	    while(/^./ && !s/^\'//) {
		s/^([^\'\\]*)//s;
		$column += length($1);
		if(s/^\\//) {
		    $column++;
		    if(s/^(.)//s) {
			$column += length($1);
			if($1 eq "0") {
			    s/^(\d{0,3})//s;
			    $column += length($1);
			}
		    }
		}
	    }
	    $column++;
	} elsif(s/^\"//) {
	    $column++;
	    while(/^./ && !s/^\"//) {
		s/^([^\"\\]*)//s;
		$column += length($1);
		if(s/^\\//) {
		    $column++;
		    if(s/^(.)//s) {
			$column += length($1);
			if($1 eq "0") {
			    s/^(\d{0,3})//s;
			    $column += length($1);
			}
		    }
		}
	    }
	    $column++;
	} elsif(s/^\n//) {
	    $line++;
	    $column = 0;
	} elsif(s/^\t//) {
	    $column = $column + 8 - $column % 8;
	}
    }

    $$refline = $line;
    $$refcolumn = $column;
}

########################################################################
# __parse_c_until_one_of

sub __parse_c_until_one_of($$$$$$$) {
    my $self = shift;

    my $characters = shift;
    my $on_same_level = shift;
    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;
    my $match = shift;

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;


    if(!defined($match)) {
	my $blackhole;
	$match = \$blackhole;
    }

    my $level = 0;
    $$match = "";
    while(/^[^$characters]/s || $level > 0) {
	my $submatch = "";

	if ($level > 0) {
	    if(s/^[^\(\)\[\]\{\}\n\t\'\"]*//s) {
		$submatch .= $&;
	    }
	} elsif ($on_same_level) {
	    if(s/^[^$characters\(\)\[\]\{\}\n\t\'\"]*//s) {
		$submatch .= $&;
	    }
	} else {
	    if(s/^[^$characters\n\t\'\"]*//s) {
		$submatch .= $&;
	    }
	}

	if(s/^\'//) {
	    $submatch .= "\'";
	    while(/^./ && !s/^\'//) {
		s/^([^\'\\]*)//s;
		$submatch .= $1;
		if(s/^\\//) {
		    $submatch .= "\\";
		    if(s/^(.)//s) {
			$submatch .= $1;
			if($1 eq "0") {
			    s/^(\d{0,3})//s;
			    $submatch .= $1;
			}
		    }
		}
	    }
	    $submatch .= "\'";

	    $$match .= $submatch;
	    $column += length($submatch);
	} elsif(s/^\"//) {
	    $submatch .= "\"";
	    while(/^./ && !s/^\"//) {
		s/^([^\"\\]*)//s;
		$submatch .= $1;
		if(s/^\\//) {
		    $submatch .= "\\";
		    if(s/^(.)//s) {
			$submatch .= $1;
			if($1 eq "0") {
			    s/^(\d{0,3})//s;
			    $submatch .= $1;
			}
		    }
		}
	    }
	    $submatch .= "\"";

	    $$match .= $submatch;
	    $column += length($submatch);
	} elsif($on_same_level && s/^[\(\[\{]//) {
	    $level++;

	    $submatch .= $&;
	    $$match .= $submatch;
	    $column++;
	} elsif($on_same_level && s/^[\)\]\}]//) {
	    if ($level > 0) {
		$level--;
		
		$submatch .= $&;
		$$match .= $submatch;
		$column++;
	    } else {
		$_ = "$&$_";
		$$match .= $submatch;
		last;
	    }
	} elsif(s/^\n//) {
	    $submatch .= "\n";

	    $$match .= $submatch;
	    $line++;
	    $column = 0;
	} elsif(s/^\t//) {
	    $submatch .= "\t";

	    $$match .= $submatch;
	    $column = $column + 8 - $column % 8;
	} else {
	    $$match .= $submatch;
	    $column += length($submatch);
	}
    }

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;
    return 1;
}

########################################################################
# _parse_c_until_one_of

sub _parse_c_until_one_of($$$$$$) {
    my $self = shift;

    my $characters = shift;
    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;
    my $match = shift;

    return $self->__parse_c_until_one_of($characters, 0, $refcurrent, $refline, $refcolumn, $match);
}

########################################################################
# _parse_c_on_same_level_until_one_of

sub _parse_c_on_same_level_until_one_of($$$$$$) {
    my $self = shift;

    my $characters = shift;
    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;
    my $match = shift;

    return $self->__parse_c_until_one_of($characters, 1, $refcurrent, $refline, $refcolumn, $match);
}

########################################################################
# parse_c_block

sub parse_c_block($$$$$$$) {
    my $self = shift;

    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    my $refstatements = shift;
    my $refstatements_line = shift;
    my $refstatements_column = shift;

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    $self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);

    my $statements;
    if(s/^\{//) {
	$column++;
	$statements = "";
    } else {
	return 0;
    }

    $self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);

    my $statements_line = $line;
    my $statements_column = $column;

    my $plevel = 1;
    while($plevel > 0) {
	my $match;
	$self->_parse_c_until_one_of("\\{\\}", \$_, \$line, \$column, \$match);

	$column++;

	$statements .= $match;
	if(s/^\}//) {
	    $plevel--;
	    if($plevel > 0) {
		$statements .= "}";
	    }
	} elsif(s/^\{//) {
	    $plevel++;
	    $statements .= "{";
	} else {
	    return 0;
	}
    }

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;
    $$refstatements = $statements;
    $$refstatements_line = $statements_line;
    $$refstatements_column = $statements_column;

    return 1;
}

########################################################################
# parse_c_declaration

sub parse_c_declaration($$$$$$$$$$$$) {
    my $self = shift;

    my $found_declaration = \${$self->{FOUND_DECLARATION}};
    my $found_function = \${$self->{FOUND_FUNCTION}};

    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    $self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);

    my $begin_line = $line;
    my $begin_column = $column + 1;

    my $end_line = $begin_line;
    my $end_column = $begin_column;
    $self->_update_c_position($_, \$end_line, \$end_column);

    if(!&$$found_declaration($begin_line, $begin_column, $end_line, $end_column, $_)) {
	return 1;
    }

    # Function
    my $function = shift;

    my $linkage = shift;
    my $calling_convention = shift;
    my $return_type = shift;
    my $name = shift;
    my @arguments = shift;
    my @argument_lines = shift;
    my @argument_columns = shift;

    # Variable
    my $type;

    if(s/^WINE_(?:DEFAULT|DECLARE)_DEBUG_CHANNEL\s*\(\s*(\w+)\s*\)\s*//s) { # FIXME: Wine specific kludge
	$self->_update_c_position($&, \$line, \$column);
    } elsif(s/^__ASM_GLOBAL_FUNC\(\s*(\w+)\s*,\s*//s) { # FIXME: Wine specific kludge
	$self->_update_c_position($&, \$line, \$column);
	$self->_parse_c_until_one_of("\)", \$_, \$line, \$column);
	if(s/\)//) {
	    $column++;
	}
    } elsif(s/^(?:DEFINE_AVIGUID|DEFINE_OLEGUID)\s*(?=\()//s) { # FIXME: Wine specific kludge
	$self->_update_c_position($&, \$line, \$column);

	my @arguments;
	my @argument_lines;
	my @argument_columns;

	if(!$self->parse_c_tuple(\$_, \$line, \$column, \@arguments, \@argument_lines, \@argument_columns)) {
	    return 0;
	}
    } elsif(s/^DEFINE_COMMON_NOTIFICATIONS\(\s*(\w+)\s*,\s*(\w+)\s*\)//s) { # FIXME: Wine specific kludge
	$self->_update_c_position($&, \$line, \$column);
    } elsif(s/^MAKE_FUNCPTR\(\s*(\w+)\s*\)//s) { # FIXME: Wine specific kludge
	$self->_update_c_position($&, \$line, \$column);
    } elsif(s/^START_TEST\(\s*(\w+)\s*\)\s*{//s) { # FIXME: Wine specific kludge
	$self->_update_c_position($&, \$line, \$column);
    } elsif(s/^int\s*_FUNCTION_\s*{//s) { # FIXME: Wine specific kludge
	$self->_update_c_position($&, \$line, \$column);
    } elsif(s/^(?:jump|strong)_alias//s) { # FIXME: GNU C library specific kludge
	$self->_update_c_position($&, \$line, \$column);
    } elsif(s/^(?:__asm__|asm)\s*\(//) {
	$self->_update_c_position($&, \$line, \$column);
    } elsif($self->parse_c_typedef(\$_, \$line, \$column)) {
	# Nothing
    } elsif($self->parse_c_variable(\$_, \$line, \$column, \$linkage, \$type, \$name)) {
	# Nothing
    } elsif($self->parse_c_function(\$_, \$line, \$column, \$function)) {
	if(&$$found_function($function))
	{
	    my $statements = $function->statements;
	    my $statements_line = $function->statements_line;
	    my $statements_column = $function->statements_column;

	    if(defined($statements)) {
		if(!$self->parse_c_statements(\$statements, \$statements_line, \$statements_column)) {
		    return 0;
		}
	    }
	}
    } else {
	$self->_parse_c_error($_, $line, $column, "declaration");
    }

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;

    return 1;
}

########################################################################
# parse_c_declarations

sub parse_c_declarations($$$$) {
    my $self = shift;

    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    return 1;
}

########################################################################
# _parse_c

sub _parse_c($$$$$$) {
    my $self = shift;

    my $pattern = shift;
    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    my $refmatch = shift;

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    my $match;
    if(s/^(?:$pattern)//s) {
	$self->_update_c_position($&, \$line, \$column);
	$match = $&;
    } else {
	return 0;
    }

    $self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;

    $$refmatch = $match;

    return 1;
}

########################################################################
# parse_c_enum

sub parse_c_enum($$$$) {
    my $self = shift;

    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    $self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);

    if (!s/^enum\s+((?:MSVCRT|WS)\(\s*\w+\s*\)|\w+)?\s*\{\s*//s) {
	return 0;
    }
    my $_name = $1 || "";

    $self->_update_c_position($&, \$line, \$column);

    my $name = "";
    
    my $match;
    while ($self->_parse_c_on_same_level_until_one_of(',', \$_, \$line, \$column, \$match)) {
	if ($match) {
	    if ($match !~ /^(\w+)\s*(?:=\s*(.*?)\s*)?$/) {
		$self->_parse_c_error($_, $line, $column, "enum");
	    }
	    my $enum_name = $1;
	    my $enum_value = $2 || "";

	    # $output->write("enum:$_name:$enum_name:$enum_value\n");
	}

	if ($self->_parse_c(',', \$_, \$line, \$column)) {
	    next;
	} elsif ($self->_parse_c('}', \$_, \$line, \$column)) {
	    # FIXME: Kludge
	    my $tuple = "($_)";
	    my $tuple_line = $line;
	    my $tuple_column = $column - 1;
	    
	    my @arguments;
	    my @argument_lines;
		    my @argument_columns;
	    
	    if(!$self->parse_c_tuple(\$tuple, \$tuple_line, \$tuple_column,
				     \@arguments, \@argument_lines, \@argument_columns)) 
	    {
		$self->_parse_c_error($_, $line, $column, "enum");
	    }
	    
	    # FIXME: Kludge
	    if ($#arguments >= 0) {
		$name = $arguments[0];
	    }
	    
	    last;
	} else {
	    $self->_parse_c_error($_, $line, $column, "enum");
	}
    }

    $self->_update_c_position($_, \$line, \$column);

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;
}


########################################################################
# parse_c_expression

sub parse_c_expression($$$$) {
    my $self = shift;

    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    my $found_function_call = \${$self->{FOUND_FUNCTION_CALL}};

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    $self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);

    while($_) {
	if(s/^(.*?)(\w+\s*\()/$2/s) {
	    $self->_update_c_position($1, \$line, \$column);

	    my $begin_line = $line;
	    my $begin_column = $column + 1;

	    my $name;
	    my @arguments;
	    my @argument_lines;
	    my @argument_columns;
	    if(!$self->parse_c_function_call(\$_, \$line, \$column, \$name, \@arguments, \@argument_lines, \@argument_columns)) {
		return 0;
	    }

	    if(&$$found_function_call($begin_line, $begin_column, $line, $column, $name, \@arguments))
	    {
		while(defined(my $argument = shift @arguments) &&
		      defined(my $argument_line = shift @argument_lines) &&
		      defined(my $argument_column = shift @argument_columns))
		{
		    $self->parse_c_expression(\$argument, \$argument_line, \$argument_column);
		}
	    }
	} else {
	    $_ = "";
	}
    }

    $self->_update_c_position($_, \$line, \$column);

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;

    return 1;
}

########################################################################
# parse_c_file

sub parse_c_file($$$$) {
    my $self = shift;

    my $found_comment = \${$self->{FOUND_COMMENT}};
    my $found_line = \${$self->{FOUND_LINE}};

    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    my $declaration = "";
    my $declaration_line = $line;
    my $declaration_column = $column;

    my $previous_line = 0;
    my $previous_column = -1;

    my $preprocessor_condition;
    my $if = 0;
    my $if0 = 0;
    my $extern_c = 0;

    my $blevel = 1;
    my $plevel = 1;
    while($plevel > 0 || $blevel > 0) {
	my $match;
	$self->_parse_c_until_one_of("#/\\(\\)\\[\\]\\{\\};", \$_, \$line, \$column, \$match);

	if($line != $previous_line) {
	    &$$found_line($line);
	} elsif(0 && $column == $previous_column) {
	    $self->_parse_c_error($_, $line, $column, "file", "no progress");
	} else {
	    # &$$found_line("$line.$column");
	}
	$previous_line = $line;
	$previous_column = $column;

	if($match !~ /^\s+$/s && $options->debug) {
	    $self->_parse_c_warning($_, $line, $column, "file", "$plevel $blevel: '$declaration' '$match'");
	}

	if(!$declaration && $match =~ s/^\s+//s) {
	    $self->_update_c_position($&, \$declaration_line, \$declaration_column);
	}

	if(!$if0) {
	    $declaration .= $match;

	    # FIXME: Kludge
	    if ($declaration =~ s/^extern\s*\"C\"//s) {
		if (s/^\{//) {
		    $self->_update_c_position($&, \$line, \$column);
		    $declaration = "";
		    $declaration_line = $line;
		    $declaration_column = $column;

		    $extern_c = 1;
		    next;
		}
	    } elsif ($extern_c && $blevel == 1 && $plevel == 1 && !$declaration) {
		if (s/^\}//) {
		    $self->_update_c_position($&, \$line, \$column);
		    $declaration = "";
		    $declaration_line = $line;
		    $declaration_column = $column;
		    
		    $extern_c = 0;
		    next;
		}
	    } elsif($declaration =~ s/^(?:__DEFINE_(?:GET|SET)_SEG|OUR_GUID_ENTRY)\s*(?=\()//sx) { # FIXME: Wine specific kludge
		my $prefix = $&;
		if ($plevel > 2 || !s/^\)//) {
		    $declaration = "$prefix$declaration";
		} else {
		    $plevel--;
		    $self->_update_c_position($&, \$line, \$column);
		    $declaration .= $&;

		    my @arguments;
		    my @argument_lines;
		    my @argument_columns;

		    if(!$self->parse_c_tuple(\$declaration, \$declaration_line, \$declaration_column,
					     \@arguments, \@argument_lines, \@argument_columns)) 
		    {
			$self->_parse_c_error($declaration, $declaration_line, $declaration_column, "file", "tuple expected");
		    }

		    $declaration = "";
		    $declaration_line = $line;
		    $declaration_column = $column;
		    
		    next;
		}
	    } elsif ($declaration =~ s/^(?:DEFINE_SHLGUID)\s*\(.*?\)//s) {
		$self->_update_c_position($&, \$declaration_line, \$declaration_column);
	    } elsif ($declaration =~ s/^(?:DECL_WINELIB_TYPE_AW|DECLARE_HANDLE(?:16)?|TYPE_MARSHAL)\(\s*(\w+)\s*\)\s*//s) {
		$self->_update_c_position($&, \$declaration_line, \$declaration_column);
	    } elsif ($declaration =~ s/^ICOM_DEFINE\(\s*(\w+)\s*,\s*(\w+)\s*\)\s*//s) {
		$self->_update_c_position($&, \$declaration_line, \$declaration_column);
	    }
	} else {
	    my $blank_lines = 0;

	    local $_ = $match;
	    while(s/^.*?\n//) { $blank_lines++; }

	    if(!$declaration) {
		$declaration_line = $line;
		$declaration_column = $column;
	    } else {
		$declaration .= "\n" x $blank_lines;
	    }

	}

	if(/^[\#\/]/) {
	    my $blank_lines = 0;
	    if(s/^\#\s*//) {
		my $preprocessor_line = $line;
		my $preprocessor_column = $column;

		my $preprocessor = $&;
		while(s/^(.*?)\\\s*\n//) {
		    $blank_lines++;
		    $preprocessor .= "$1\n";
		}
		if(s/^(.*?)(\/\*.*?\*\/)(.*?)\n//) {
		    $_ = "$2\n$_";
		    if(defined($3)) {
			$preprocessor .= "$1$3";
		    } else {
			$preprocessor .= $1;
		    }
		} elsif(s/^(.*?)(\/[\*\/].*?)?\n//) {
		    if(defined($2)) {
			$_ = "$2\n$_";
		    } else {
			$blank_lines++;
		    }
		    $preprocessor .= $1;
		}


		if($preprocessor =~ /^\#\s*if/) {
		    if($preprocessor =~ /^\#\s*if\s*0/) {
			$if0++;
		    } elsif($if0 > 0) {
			$if++;
		    } else {
			if($preprocessor =~ /^\#\s*ifdef\s+WORDS_BIGENDIAN$/) {
			    $preprocessor_condition = "defined(WORD_BIGENDIAN)";
			    # $output->write("'$preprocessor_condition':'$declaration'\n")
			} else {
			    $preprocessor_condition = "";
			}
		    }
		} elsif($preprocessor =~ /^\#\s*else/) {
		    if ($preprocessor_condition ne "") {
			$preprocessor_condition =~ "!$preprocessor_condition";
			$preprocessor_condition =~ s/^!!/!/;
			# $output->write("'$preprocessor_condition':'$declaration'\n")
		    }
		} elsif($preprocessor =~ /^\#\s*endif/) {
		    if($if0 > 0) {
			if($if > 0) {
			    $if--;
			} else {
			    $if0--;
			}
		    } else {
			if ($preprocessor_condition ne "") {
			    # $output->write("'$preprocessor_condition':'$declaration'\n");
			    $preprocessor_condition = "";
			}
		    }
		}

		if(!$self->parse_c_preprocessor(\$preprocessor, \$preprocessor_line, \$preprocessor_column)) {
		     return 0;
		}
	    }

	    if(s/^\/\*.*?\*\///s) {
		&$$found_comment($line, $column + 1, $&);
	        local $_ = $&;
		while(s/^.*?\n//) {
		    $blank_lines++;
		}
		if($_) {
		    $column += length($_);
		}
	    } elsif(s/^\/\/(.*?)\n//) {
		&$$found_comment($line, $column + 1, $&);
		$blank_lines++;
	    } elsif(s/^\///) {
		if(!$if0) {
		    $declaration .= $&;
		    $column++;
		}
	    }

	    $line += $blank_lines;
	    if($blank_lines > 0) {
		$column = 0;
	    }

	    if(!$declaration) {
		$declaration_line = $line;
		$declaration_column = $column;
	    } elsif($blank_lines > 0) {
		$declaration .= "\n" x $blank_lines;
	    }

	    next;
	}

	$column++;

	if($if0) {
	    s/^.//;
	    next;
	}

	if(s/^[\(\[]//) {
	    $plevel++;
	    $declaration .= $&;
	} elsif(s/^\]//) {
	    $plevel--;
	    $declaration .= $&;
	} elsif(s/^\)//) {
	    $plevel--;
	    if($blevel <= 0) {
		$self->_parse_c_error($_, $line, $column, "file", ") without (");
	    }
	    $declaration .= $&;
	    if($plevel == 1 && $declaration =~ /^__ASM_GLOBAL_FUNC/) {
		if(!$self->parse_c_declaration(\$declaration, \$declaration_line, \$declaration_column)) {
		    return 0;
		}
		$self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);
		$declaration = "";
		$declaration_line = $line;
		$declaration_column = $column;
	    }
	} elsif(s/^\{//) {
	    $blevel++;
	    $declaration .= $&;
	} elsif(s/^\}//) {
	    $blevel--;
	    if($blevel <= 0) {
		$self->_parse_c_error($_, $line, $column, "file", "} without {");
	    }

	    $declaration .= $&;

	    if($declaration =~ /^typedef/s ||
	       $declaration =~ /^(?:const\s+|extern\s+|static\s+|volatile\s+)*(?:interface|struct|union)(?:\s+\w+)?\s*\{/s)
	    {
		# Nothing
	    } elsif($plevel == 1 && $blevel == 1) {
		if(!$self->parse_c_declaration(\$declaration, \$declaration_line, \$declaration_column)) {
		    return 0;
		}
		$self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);
		$declaration = "";
		$declaration_line = $line;
		$declaration_column = $column;
	    } elsif($column == 1 && !$extern_c) {
		$self->_parse_c_error("", $line, $column, "file", "inner } ends on column 1");
	    }
	} elsif(s/^;//) {
	    $declaration .= $&;
	    if(0 && $blevel == 1 &&
	       $declaration !~ /^typedef/ &&
	       $declaration !~ /^(?:const\s+|extern\s+|static\s+|volatile\s+)?(?:interface|struct|union)(?:\s+\w+)?\s*\{/s &&
	       $declaration =~ /^(?:\w+(?:\s*\*)*\s+)*(\w+)\s*\(\s*(?:(?:\w+\s*,\s*)*(\w+))?\s*\)\s*(.*?);$/s &&
	       $1 ne "ICOM_VTABLE" && defined($2) && $2 ne "void" && $3) # K&R
	    {
		$self->_parse_c_warning("", $line, $column, "file", "function $1: warning: function has K&R format");
	    } elsif($plevel == 1 && $blevel == 1) {
		$declaration =~ s/\s*;$//;
		if($declaration && !$self->parse_c_declaration(\$declaration, \$declaration_line, \$declaration_column)) {
		    return 0;
		}
		$self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);
		$declaration = "";
		$declaration_line = $line;
		$declaration_column = $column;
	    }
	} elsif(/^\s*$/ && $declaration =~ /^\s*$/ && $match =~ /^\s*$/) {
	    $plevel = 0;
	    $blevel = 0;
	} else {
	    $self->_parse_c_error($_, $line, $column, "file", "parse error: '$declaration' '$match'");
	}
    }

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;

    return 1;
}

########################################################################
# parse_c_function

sub parse_c_function($$$$$) {
    my $self = shift;

    my $file = \${$self->{FILE}};
    my $create_function = \${$self->{CREATE_FUNCTION}};

    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    my $reffunction = shift;

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    my $linkage = "";
    my $calling_convention = "";
    my $return_type;
    my $name;
    my @arguments;
    my @argument_lines;
    my @argument_columns;
    my $statements;
    my $statements_line;
    my $statements_column;

    $self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);

    my $begin_line = $line;
    my $begin_column = $column + 1;

    if($self->_parse_c('__declspec\((?:dllexport|dllimport|naked)\)|INTERNETAPI|RPCRTAPI', \$_, \$line, \$column)) {
	# Nothing
    }

    # $self->_parse_c_warning($_, $line, $column, "function", "");

    my $match;
    while($self->_parse_c('(?:const|inline|extern(?:\s+\"C\")?|EXTERN_C|static|volatile|' .
			  'signed(?=\s+__int(?:8|16|32|64)\b|\s+char\b|\s+int\b|\s+long(?:\s+long)?\b|\s+short\b)|' .
			  'unsigned(?=\s+__int(?:8|16|32|64)\b|\s+char\b|\s+int\b|\s+long(?:\s+long)?\b|\s+short\b)|' .
			  'long(?=\s+double\b|\s+int\b|\s+long\b))(?=\b)',
			  \$_, \$line, \$column, \$match))
    {
	if($match =~ /^(?:extern|static)$/) {
	    if(!$linkage) {
		$linkage = $match;
	    }
	}
    }

    if($self->_parse_c('DECL_GLOBAL_CONSTRUCTOR', \$_, \$line, \$column, \$name)) { # FIXME: Wine specific kludge
	# Nothing
    } elsif($self->_parse_c('WINE_EXCEPTION_FILTER\(\w+\)', \$_, \$line, \$column, \$name)) { # FIXME: Wine specific kludge
	# Nothing
    } else {
	if(!$self->parse_c_type(\$_, \$line, \$column, \$return_type)) {
	    return 0;
	}

	$self->_parse_c('inline|FAR', \$_, \$line, \$column);

	$self->_parse_c($CALL_CONVENTION,
			\$_, \$line, \$column, \$calling_convention);


	# FIXME: ???: Old variant of __attribute((const))
	$self->_parse_c('(?:const|volatile)', \$_, \$line, \$column);

	if(!$self->_parse_c('(?:operator\s*!=|(?:MSVCRT|WS)\(\s*\w+\s*\)|\w+)', \$_, \$line, \$column, \$name)) {
	    return 0;
	}

	my $p = 0;
	if(s/^__P\s*\(//) {
	    $self->_update_c_position($&, \$line, \$column);
	    $p = 1;
	}

	if(!$self->parse_c_tuple(\$_, \$line, \$column, \@arguments, \@argument_lines, \@argument_columns)) {
	    return 0;
	}

	if($p) {
	    if (s/^\)//) {
		$self->_update_c_position($&, \$line, \$column);
	    } else {
		$self->_parse_c_error($_, $line, $column, "function");
	    }
	}
    }


    if($self->_parse_c('__attribute__\s*\(\s*\(\s*(?:constructor|destructor)\s*\)\s*\)', \$_, \$line, \$column)) {
	# Nothing
    }

    my $kar;
    # FIXME: Implement proper handling of K&R C functions
    $self->_parse_c_until_one_of("{", \$_, \$line, \$column, $kar);

    if($kar) {
	$output->write("K&R: $kar\n");
    }

    if($_ && !$self->parse_c_block(\$_, \$line, \$column, \$statements, \$statements_line, \$statements_column)) {
	return 0;
    }

    my $end_line = $line;
    my $end_column = $column;

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;

    my $function = &$$create_function;

    $function->file($$file);
    $function->begin_line($begin_line);
    $function->begin_column($begin_column);
    $function->end_line($end_line);
    $function->end_column($end_column);
    $function->linkage($linkage);
    $function->return_type($return_type);
    $function->calling_convention($calling_convention);
    $function->name($name);
    # if(defined($argument_types)) {
    #     $function->argument_types([@$argument_types]);
    # }
    # if(defined($argument_names)) {
    #     $function->argument_names([@$argument_names]);
    # }
    $function->statements_line($statements_line);
    $function->statements_column($statements_column);
    $function->statements($statements);

    $$reffunction = $function;

    return 1;
}

########################################################################
# parse_c_function_call

sub parse_c_function_call($$$$$$$$) {
    my $self = shift;

    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    my $refname = shift;
    my $refarguments = shift;
    my $refargument_lines = shift;
    my $refargument_columns = shift;

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    my $name;
    my @arguments;
    my @argument_lines;
    my @argument_columns;

    if(s/^(\w+)(\s*)(?=\()//s) {
	$self->_update_c_position($&, \$line, \$column);

	$name = $1;

	if(!$self->parse_c_tuple(\$_, \$line, \$column, \@arguments, \@argument_lines, \@argument_columns)) {
	    return 0;
	}
    } else {
	return 0;
    }

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;

    $$refname = $name;
    @$refarguments = @arguments;
    @$refargument_lines = @argument_lines;
    @$refargument_columns = @argument_columns;

    return 1;
}

########################################################################
# parse_c_preprocessor

sub parse_c_preprocessor($$$$) {
    my $self = shift;

    my $found_preprocessor = \${$self->{FOUND_PREPROCESSOR}};

    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    $self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);

    my $begin_line = $line;
    my $begin_column = $column + 1;

    if(!&$$found_preprocessor($begin_line, $begin_column, "$_")) {
	return 1;
    }

    if(/^\#\s*define\s*(.*?)$/s) {
	$self->_update_c_position($_, \$line, \$column);
    } elsif(/^\#\s*else/s) {
	$self->_update_c_position($_, \$line, \$column);
    } elsif(/^\#\s*endif/s) {
	$self->_update_c_position($_, \$line, \$column);
    } elsif(/^\#\s*(?:if|ifdef|ifndef)?\s*(.*?)$/s) {
	$self->_update_c_position($_, \$line, \$column);
    } elsif(/^\#\s*include\s+(.*?)$/s) {
	$self->_update_c_position($_, \$line, \$column);
    } elsif(/^\#\s*undef\s+(.*?)$/s) {
	$self->_update_c_position($_, \$line, \$column);
    } else {
	$self->_parse_c_error($_, $line, $column, "preprocessor");
    }

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;

    return 1;
}

########################################################################
# parse_c_statement

sub parse_c_statement($$$$) {
    my $self = shift;

    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    my $found_function_call = \${$self->{FOUND_FUNCTION_CALL}};

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    $self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);

    $self->_parse_c('(?:case\s+)?(\w+)\s*:\s*', \$_, \$line, \$column);

    # $output->write("$line.$column: statement: '$_'\n");

    if(/^$/) {
	# Nothing
    } elsif(/^\{/) {
	my $statements;
	my $statements_line;
	my $statements_column;
	if(!$self->parse_c_block(\$_, \$line, \$column, \$statements, \$statements_line, \$statements_column)) {
	    return 0;
	}
	if(!$self->parse_c_statements(\$statements, \$statements_line, \$statements_column)) {
	    return 0;
	}
    } elsif(s/^(for|if|switch|while)\s*(?=\()//) {
	$self->_update_c_position($&, \$line, \$column);

	my $name = $1;

	my @arguments;
	my @argument_lines;
	my @argument_columns;
	if(!$self->parse_c_tuple(\$_, \$line, \$column, \@arguments, \@argument_lines, \@argument_columns)) {
	    return 0;
	}

	$self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);
	if(!$self->parse_c_statement(\$_, \$line, \$column)) {
	    return 0;
	}
	$self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);

	while(defined(my $argument = shift @arguments) &&
	      defined(my $argument_line = shift @argument_lines) &&
	      defined(my $argument_column = shift @argument_columns))
	{
	    $self->parse_c_expression(\$argument, \$argument_line, \$argument_column);
	}
    } elsif(s/^else//) {
	$self->_update_c_position($&, \$line, \$column);
	if(!$self->parse_c_statement(\$_, \$line, \$column)) {
	    return 0;
	}
    } elsif(s/^return//) {
	$self->_update_c_position($&, \$line, \$column);
	$self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);
	if(!$self->parse_c_expression(\$_, \$line, \$column)) {
	    return 0;
	}
    } elsif($self->parse_c_expression(\$_, \$line, \$column)) {
	# Nothing
    } else {
	# $self->_parse_c_error($_, $line, $column, "statement");
    }

    $self->_update_c_position($_, \$line, \$column);

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;

    return 1;
}

########################################################################
# parse_c_statements

sub parse_c_statements($$$$) {
    my $self = shift;

    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    my $found_function_call = \${$self->{FOUND_FUNCTION_CALL}};

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    $self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);

    # $output->write("$line.$column: statements: '$_'\n");

    my $statement = "";
    my $statement_line = $line;
    my $statement_column = $column;

    my $previous_line = -1;
    my $previous_column = -1;

    my $blevel = 1;
    my $plevel = 1;
    while($plevel > 0 || $blevel > 0) {
	my $match;
	$self->_parse_c_until_one_of("\\(\\)\\[\\]\\{\\};", \$_, \$line, \$column, \$match);

	if($previous_line == $line && $previous_column == $column) {
	    $self->_parse_c_error($_, $line, $column, "statements", "no progress");
	}
	$previous_line = $line;
	$previous_column = $column;

	# $output->write("'$match' '$_'\n");

	$statement .= $match;
	$column++;
	if(s/^[\(\[]//) {
	    $plevel++;
	    $statement .= $&;
	} elsif(s/^[\)\]]//) {
	    $plevel--;
	    if($plevel <= 0) {
		$self->_parse_c_error($_, $line, $column, "statements");
	    }
	    $statement .= $&;
	} elsif(s/^\{//) {
	    $blevel++;
	    $statement .= $&;
	} elsif(s/^\}//) {
	    $blevel--;
	    $statement .= $&;
	    if($blevel == 1) {
		if(!$self->parse_c_statement(\$statement, \$statement_line, \$statement_column)) {
		    return 0;
		}
		$self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);
		$statement = "";
		$statement_line = $line;
		$statement_column = $column;
	    }
	} elsif(s/^;//) {
	    if($plevel == 1 && $blevel == 1) {
		if(!$self->parse_c_statement(\$statement, \$statement_line, \$statement_column)) {
		    return 0;
		}

		$self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);
		$statement = "";
		$statement_line = $line;
		$statement_column = $column;
	    } else {
		$statement .= $&;
	    }
	} elsif(/^\s*$/ && $statement =~ /^\s*$/ && $match =~ /^\s*$/) {
	    $plevel = 0;
	    $blevel = 0;
	} else {
	    $self->_parse_c_error($_, $line, $column, "statements");
	}
    }

    $self->_update_c_position($_, \$line, \$column);

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;

    return 1;
}

########################################################################
# parse_c_struct_union

sub parse_c_struct_union($$$$$$$$$) {
    my $self = shift;

    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    my $refkind = shift;
    my $ref_name = shift;
    my $reffield_type_names = shift;
    my $reffield_names = shift;
    my $refnames = shift;

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    my $kind;
    my $_name;
    my @field_type_names = ();
    my @field_names = ();
    my @names = ();

    $self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);

    if (!s/^(interface\s+|struct\s+|union\s+)((?:MSVCRT|WS)\(\s*\w+\s*\)|\w+)?\s*\{\s*//s) {
	return 0;
    }
    $kind = $1;
    $_name = $2 || "";

    $self->_update_c_position($&, \$line, \$column);
    
    $kind =~ s/\s+//g;

    my $match;
    while ($_ && $self->_parse_c_on_same_level_until_one_of(';', \$_, \$line, \$column, \$match))
    {
	my $field_linkage;
	my $field_type_name;
	my $field_name;
	
	if ($self->parse_c_variable(\$match, \$line, \$column, \$field_linkage, \$field_type_name, \$field_name)) {
	    $field_type_name =~ s/\s+/ /g;
	    
	    push @field_type_names, $field_type_name;
	    push @field_names, $field_name;
	    # $output->write("$kind:$_name:$field_type_name:$field_name\n");
	} elsif ($match) {
	    $self->_parse_c_error($_, $line, $column, "typedef $kind: '$match'");
	}
	
	if ($self->_parse_c(';', \$_, \$line, \$column)) {
	    next;
	} elsif ($self->_parse_c('}', \$_, \$line, \$column)) {
	    # FIXME: Kludge
	    my $tuple = "($_)";
	    my $tuple_line = $line;
	    my $tuple_column = $column - 1;
	    
	    my @arguments;
	    my @argument_lines;
	    my @argument_columns;
	    
	    if(!$self->parse_c_tuple(\$tuple, \$tuple_line, \$tuple_column,
				     \@arguments, \@argument_lines, \@argument_columns)) 
	    {
		$self->_parse_c_error($_, $line, $column, "$kind");
	    }

	    foreach my $argument (@arguments) {
		my $name = $argument;

		push @names, $name;
	    }

	    last;
	} else {
	    $self->_parse_c_error($_, $line, $column, "$kind");
	}
    }

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;

    $$refkind = $kind;
    $$ref_name = $_name;
    @$reffield_type_names = @field_type_names;
    @$reffield_names = @field_names;
    @$refnames = @names;

    return 1;
}

########################################################################
# parse_c_tuple

sub parse_c_tuple($$$$$$$) {
    my $self = shift;

    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    # FIXME: Should not write directly
    my $items = shift;
    my $item_lines = shift;
    my $item_columns = shift;

    local $_ = $$refcurrent;

    my $line = $$refline;
    my $column = $$refcolumn;

    my $item;
    if(s/^\(//) {
	$column++;
	$item = "";
    } else {
	return 0;
    }

    my $item_line = $line;
    my $item_column = $column + 1;

    my $plevel = 1;
    while($plevel > 0) {
	my $match;
	$self->_parse_c_until_one_of("\\(,\\)", \$_, \$line, \$column, \$match);

	$column++;

	$item .= $match;
	if(s/^\)//) {
	    $plevel--;
	    if($plevel == 0) {
		push @$item_lines, $item_line;
		push @$item_columns, $item_column;
		push @$items, $item;
		$item = "";
	    } else {
		$item .= ")";
	    }
	} elsif(s/^\(//) {
	    $plevel++;
	    $item .= "(";
	} elsif(s/^,//) {
	    if($plevel == 1) {
		push @$item_lines, $item_line;
		push @$item_columns, $item_column;
		push @$items, $item;
		$self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);
		$item_line = $line;
		$item_column = $column + 1;
		$item = "";
	    } else {
		$item .= ",";
	    }
	} else {
	    return 0;
	}
    }

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;

    return 1;
}

########################################################################
# parse_c_type

sub parse_c_type($$$$$) {
    my $self = shift;

    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    my $reftype = shift;

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    my $type;

    $self->_parse_c("(?:const|volatile)", \$_, \$line, \$column);

    if($self->_parse_c('ICOM_VTABLE\(.*?\)', \$_, \$line, \$column, \$type)) {
	# Nothing
    } elsif($self->_parse_c('(?:enum\s+|interface\s+|struct\s+|union\s+)?(?:(?:MSVCRT|WS)\(\s*\w+\s*\)|\w+)\s*(\*\s*)*',
			    \$_, \$line, \$column, \$type))
    {
	# Nothing
    } else {
	return 0;
    }
    $type =~ s/\s//g;

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;

    $$reftype = $type;

    return 1;
}

########################################################################
# parse_c_typedef

sub parse_c_typedef($$$$) {
    my $self = shift;

    my $create_type = \${$self->{CREATE_TYPE}};
    my $found_type = \${$self->{FOUND_TYPE}};
    my $preprocessor_condition = \${$self->{PREPROCESSOR_CONDITION}};

    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    my $type;

    if (!$self->_parse_c("typedef", \$_, \$line, \$column)) {
	return 0;
    }

    my $finished = 0;
    
    if ($finished) {
	# Nothing
    } elsif ($self->parse_c_enum(\$_, \$line, \$column)) {
	$finished = 1;
    } 

    my $kind;
    my $_name;
    my @field_type_names;
    my @field_names;
    my @names;
    if ($finished) {
	# Nothing
    } elsif ($self->parse_c_struct_union(\$_, \$line, \$column,
					 \$kind, \$_name, \@field_type_names, \@field_names, \@names))
    {
	my $base_name;
        foreach my $name (@names)
        {
            if ($name =~ /^\w+$/)
            {
                $base_name = $name;
                last;
            }
        }
        $base_name="$kind $_name" if (!defined $base_name and defined $_name);
        $base_name=$kind if (!defined $base_name);
	foreach my $name (@names) {
	    if ($name =~ /^\w+$/) {
		my $type = &$$create_type();
		
		$type->kind($kind);
		$type->_name($_name);
		$type->name($name);
		$type->field_type_names([@field_type_names]);
		$type->field_names([@field_names]);

		&$$found_type($type);
	    } elsif ($name =~ /^(\*+)\s*(?:RESTRICTED_POINTER\s+)?(\w+)$/) {
		my $type_name = "$base_name $1";
		$name = $2;

		my $type = &$$create_type();

		$type->kind("");
		$type->name($name);
		$type->field_type_names([$type_name]);
		$type->field_names([""]);

		&$$found_type($type);		
	    } else {
		$self->_parse_c_error($_, $line, $column, "typedef 2");
	    }
	}
	
	$finished = 1;
    }

    my $linkage;
    my $type_name;
    my $name;
    if ($finished) {
	# Nothing
    } elsif ($self->parse_c_variable(\$_, \$line, \$column, \$linkage, \$type_name, \$name)) {
	$type_name =~ s/\s+/ /g;
	
	if(defined($type_name) && defined($name)) {
	    my $type = &$$create_type();
	    
	    if (length($name) == 0) {
		$self->_parse_c_error($_, $line, $column, "typedef");
	    }

	    $type->kind("");
	    $type->name($name);
	    $type->field_type_names([$type_name]);
	    $type->field_names([""]);
	    
	    &$$found_type($type);
	}

	if (0 && $_ && !/^,/) {
	    $self->_parse_c_error($_, $line, $column, "typedef");
	}   
    } else {
	$self->_parse_c_error($_, $line, $column, "typedef");
    }

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;

    return 1;
}

########################################################################
# parse_c_variable

sub parse_c_variable($$$$$$$) {
    my $self = shift;

    my $found_variable = \${$self->{FOUND_VARIABLE}};

    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    my $reflinkage = shift;
    my $reftype = shift;
    my $refname = shift;

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    $self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);

    my $begin_line = $line;
    my $begin_column = $column + 1;

    my $linkage = "";
    my $sign = "";
    my $type = "";
    my $name = "";

    # $self->_parse_c_warning($_, $line, $column, "variable");

    my $match;
    while($self->_parse_c('(?:const|inline|extern(?:\s+\"C\")?|EXTERN_C|static|volatile|' .
			  'signed(?=\s+__int(?:8|16|32|64)\b|\s+char\b|\s+int\b|\s+long(?:\s+long)?\b|\s+short\b)|' .
			  'unsigned(?=\s+__int(?:8|16|32|64)\b|\s+char\b|\s+int\b|\s+long(?:\s+long)?\b|\s+short\b)|' .
			  'long(?=\s+double\b|\s+int\b|\s+long\b))(?=\b)',
			  \$_, \$line, \$column, \$match))
    {
	if ($match =~ /^(?:extern|static)$/) {
	    if (!$linkage) {
		$linkage = $match;
	    } else {
		$self->_parse_c_warning($_, $line, $column, "repeated linkage (ignored): $match");
	    }
	} elsif ($match =~ /^(?:signed|unsigned)$/) {
	    if (!$sign) {
		$sign = "$match ";
	    } else {
		$self->_parse_c_warning($_, $line, $column, "repeated sign (ignored): $match");
	    }
	}
    }

    my $finished = 0;

    if($finished) {
	# Nothing
    } elsif(/^$/) {
	return 0;
    } elsif (s/^(enum\s+|interface\s+|struct\s+|union\s+)((?:MSVCRT|WS)\(\s*\w+\s*\)|\w+)?\s*\{\s*//s) {
	my $kind = $1;
	my $_name = $2;
	$self->_update_c_position($&, \$line, \$column);

	if(defined($_name)) {
	    $type = "$kind $_name { }";
	} else {
	    $type = "$kind { }";
	}

	$finished = 1;
    } elsif(s/^((?:enum\s+|interface\s+|struct\s+|union\s+)?\w+\b(?:\s+DECLSPEC_ALIGN\(.*?\)|\s*(?:const\s*|volatile\s*)?\*)*)\s*(\w+)\s*(\[.*?\]$|:\s*(\d+)$|\{)?//s) {
	$type = "$sign$1";
	$name = $2;

	if (defined($3)) {
	    my $bits = $4;
	    local $_ = $3;
	    if (/^\[/) {
		$type .= $_;
	    } elsif (/^:/) {
		$type .= ":$bits";
	    } elsif (/^\{/) {
		# Nothing
	    }
	}

	$type = $self->_format_c_type($type);

	$finished = 1;
    } elsif(s/^((?:enum\s+|interface\s+|struct\s+|union\s+)?\w+\b(?:\s*\*)*)\s*:\s*(\d+)$//s) {
	$type = "$sign$1:$2";
	$name = "";
	$type = $self->_format_c_type($type);

	$finished = 1;
    } elsif(s/^((?:enum\s+|interface\s+|struct\s+|union\s+)?\w+\b(?:\s*\*)*\s*\(\s*(?:$CALL_CONVENTION)?(?:\s*\*)*)\s*(\w+)\s*(\)\s*\(.*?\))$//s) {
	$type = $self->_format_c_type("$sign$1$3");
	$name = $2;

	$finished = 1;
    } elsif($self->_parse_c('DEFINE_GUID', \$_, \$line, \$column, \$match)) { # Windows specific
	$type = $match;
	$finished = 1;
    } else {
	$self->_parse_c_warning($_, $line, $column, "variable", "'$_'");
	$finished = 1;
    }

    if($finished) {
	# Nothing
    } elsif($self->_parse_c('SEQ_DEFINEBUF', \$_, \$line, \$column, \$match)) { # Linux specific
	$type = $match;
	$finished = 1;
    } elsif($self->_parse_c('DEFINE_REGS_ENTRYPOINT_\w+|DPQ_DECL_\w+|HANDLER_DEF|IX86_ONLY', # Wine specific
			    \$_, \$line, \$column, \$match))
    {
	$type = $match;
	$finished = 1;
    } elsif($self->_parse_c('(?:struct\s+)?ICOM_VTABLE\s*\(\w+\)', \$_, \$line, \$column, \$match)) {
	$type = $match;
	$finished = 1;
    } elsif(s/^(enum|interface|struct|union)(?:\s+(\w+))?\s*\{.*?\}\s*//s) {
	my $kind = $1;
	my $_name = $2;
	$self->_update_c_position($&, \$line, \$column);

	if(defined($_name)) {
	    $type = "struct $_name { }";
	} else {
	    $type = "struct { }";
	}
    } elsif(s/^((?:enum\s+|interface\s+|struct\s+|union\s+)?\w+)\s*(?:\*\s*)*//s) {
	$type = $&;
	$type =~ s/\s//g;
    } else {
	return 0;
    }

    # $output->write("*** $type: '$_'\n");

    # $self->_parse_c_warning($_, $line, $column, "variable2", "");

    if($finished) {
	# Nothing
    } elsif(s/^WINAPI\s*//) {
	$self->_update_c_position($&, \$line, \$column);
    }

    if($finished) {
	# Nothing
    } elsif(s/^(\((?:$CALL_CONVENTION)?\s*\*?\s*(?:$CALL_CONVENTION)?\w+\s*(?:\[[^\]]*\]\s*)*\))\s*\(//) {
	$self->_update_c_position($&, \$line, \$column);

	$name = $1;
	$name =~ s/\s//g;

	$self->_parse_c_until_one_of("\\)", \$_, \$line, \$column);
	if(s/^\)//) { $column++; }
	$self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);

        if(!s/^(?:=\s*|,\s*|$)//) {
	    return 0;
	}
    } elsif(s/^(?:\*\s*)*(?:const\s+|volatile\s+)?(\w+)\s*(?:\[[^\]]*\]\s*)*\s*(?:=\s*|,\s*|$)//) {
	$self->_update_c_position($&, \$line, \$column);

	$name = $1;
	$name =~ s/\s//g;
    } elsif(/^$/) {
	$name = "";
    } else {
	return 0;
    }

    # $output->write("$type: $name: '$_'\n");

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;

    $$reflinkage = $linkage;
    $$reftype = $type;
    $$refname = $name;

    if(&$$found_variable($begin_line, $begin_column, $linkage, $type, $name))
    {
	# Nothing
    }

    return 1;
}

1;
