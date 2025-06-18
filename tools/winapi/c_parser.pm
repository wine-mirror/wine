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
use warnings 'all';

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
		    "SEC_ENTRY|VFWAPI|VFWAPIV|WINGDIPAPI|WMIAPI|WINAPI|WINAPIV|APIENTRY|";


sub parse_c_function($$$$$);
sub parse_c_function_call($$$$$$$$);
sub parse_c_preprocessor($$$$);
sub parse_c_statements($$$$);
sub parse_c_tuple($$$$$$$);
sub parse_c_type($$$$$);
sub parse_c_typedef($$$$);
sub parse_c_variable($$$$$$$);


sub new($$)
{
    my ($proto, $filename) = @_;
    my $class = ref($proto) || $proto;
    my $self  = {FILE => $filename,
                 CREATE_FUNCTION => sub { return new c_function; },
                 CREATE_TYPE => sub { return new c_type; },
                 FOUND_COMMENT => sub { return 1; },
                 FOUND_DECLARATION => sub { return 1; },
                 FOUND_FUNCTION => sub { return 1; },
                 FOUND_FUNCTION_CALL => sub { return 1; },
                 FOUND_LINE => sub { return 1; },
                 FOUND_PREPROCESSOR => sub { return 1; },
                 FOUND_STATEMENT => sub { return 1; },
                 FOUND_TYPE => sub { return 1; },
                 FOUND_VARIABLE => sub { return 1; }
    };
    bless ($self, $class);
    return $self;
}


#
# Callback setters
#

sub set_found_comment_callback($$)
{
    my ($self, $found_comment) = @_;
    $self->{FOUND_COMMENT} = $found_comment;
}

sub set_found_declaration_callback($$)
{
    my ($self, $found_declaration) = @_;
    $self->{FOUND_DEClARATION} = $found_declaration;
}

    sub set_found_function_callback($$)
{
    my ($self, $found_function) = @_;
    $self->{FOUND_FUNCTION} = $found_function;
}

sub set_found_function_call_callback($$)
{
    my ($self, $found_function_call) = @_;
    $self->{FOUND_FUNCTION_CALL} = $found_function_call;
}

sub set_found_line_callback($$)
{
    my ($self, $found_line) = @_;
    $self->{FOUND_LINE} = $found_line;
}

sub set_found_preprocessor_callback($$)
{
    my ($self, $found_preprocessor) = @_;
    $self->{FOUND_PREPROCESSOR} = $found_preprocessor;
}

sub set_found_statement_callback($$)
{
    my ($self, $found_statement) = @_;
    $self->{FOUND_STATEMENT} = $found_statement;
}

sub set_found_type_callback($$)
{
    my ($self, $found_type) = @_;
    $self->{FOUND_TYPE} = $found_type;
}

sub set_found_variable_callback($$)
{
    my ($self, $found_variable) = @_;
    $self->{FOUND_VARIABLE} = $found_variable;
}


########################################################################
# _format_c_type
sub _format_c_type($$)
{
    my ($self, $type) = @_;

    $type =~ s/^\s*(.*?)\s*$/$1/;

    if ($type =~ /^(\w+(?:\s*\*)*)\s*\(\s*\*\s*\)\s*\(\s*(.*?)\s*\)$/s) {
	my $return_type = $1;
	my @arguments = split(/\s*,\s*/, $2);
	foreach my $argument (@arguments) {
	    if ($argument =~ s/^(\w+(?:\s*\*)*)\s*\w+$/$1/) { 
		$argument =~ s/\s+/ /g;
		$argument =~ s/\s*\*\s*/*/g;
		$argument =~ s/(\*+)$/ $1/;
	    }
	}

	$type = "$return_type (*)(" . join(", ", @arguments) . ")";
    }

    return $type;
}


########################################################################
# _parse_c_warning
#
# FIXME: Use caller (See man perlfunc)
sub _parse_c_warning($$$$$$)
{
    my ($self, $curlines, $line, $column, $context, $message) = @_;

    $message = "warning" if !$message;

    my $current = "";
    if ($curlines) {
	my @lines = split(/\n/, $curlines);

	$current .= $lines[0] . "\n" if $lines[0];
        $current .= $lines[1] . "\n" if $lines[1];
    }

    if($current) {
	$output->write("$self->{FILE}:$line." . ($column + 1) . ": $context: $message: \\\n$current");
    } else {
	$output->write("$self->{FILE}:$line." . ($column + 1) . ": $context: $message\n");
    }
}

########################################################################
# _parse_c_error

sub _parse_c_error($$$$$$)
{
    my ($self, $curlines, $line, $column, $context, $message) = @_;

    $message = "parse error" if !$message;

    # Why did I do this?
    if($output->prefix) {
	# $output->write("\n");
	$output->prefix("");
    }

    $self->_parse_c_warning($curlines, $line, $column, $context, $message);

    exit 1;
}

########################################################################
# _update_c_position

sub _update_c_position($$$$)
{
    my ($self, $source, $refline, $refcolumn) = @_;
    my $line = $$refline;
    my $column = $$refcolumn;

    while ($source)
    {
	if ($source =~ s/^[^\n\t\'\"]*//s)
        {
	    $column += length($&);
	}

	if ($source =~ s/^\'//)
        {
	    $column++;
	    while ($source =~ /^./ && $source !~ s/^\'//)
            {
		$source =~ s/^([^\'\\]*)//s;
		$column += length($1);
		if ($source =~ s/^\\//)
                {
		    $column++;
		    if ($source =~ s/^(.)//s)
                    {
			$column += length($1);
			if ($1 eq "0")
                        {
			    $source =~ s/^(\d{0,3})//s;
			    $column += length($1);
			}
		    }
		}
	    }
	    $column++;
	}
        elsif ($source =~ s/^\"//)
        {
	    $column++;
	    while ($source =~ /^./ && $source !~ s/^\"//)
            {
		$source =~ s/^([^\"\\]*)//s;
		$column += length($1);
		if ($source =~ s/^\\//)
                {
		    $column++;
		    if ($source =~ s/^(.)//s)
                    {
			$column += length($1);
			if ($1 eq "0")
                        {
			    $source =~ s/^(\d{0,3})//s;
			    $column += length($1);
			}
		    }
		}
	    }
	    $column++;
	}
        elsif ($source =~ s/^\n//)
        {
	    $line++;
	    $column = 0;
	}
        elsif ($source =~ s/^\t//)
        {
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

sub _parse_c_until_one_of($$$$$$)
{
    my ($self, $characters, $refcurrent, $refline, $refcolumn, $match) = @_;
    return $self->__parse_c_until_one_of($characters, 0, $refcurrent, $refline, $refcolumn, $match);
}

sub _parse_c_on_same_level_until_one_of($$$$$$)
{
    my ($self, $characters, $refcurrent, $refline, $refcolumn, $match) = @_;
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

sub parse_c_declaration($$$$)
{
    my ($self, $refcurrent, $refline, $refcolumn) = @_;

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    $self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);

    my $begin_line = $line;
    my $begin_column = $column + 1;

    my $end_line = $begin_line;
    my $end_column = $begin_column;
    $self->_update_c_position($_, \$end_line, \$end_column);

    if(!$self->{FOUND_DECLARATION}($begin_line, $begin_column, $end_line, $end_column, $_)) {
	return 1;
    }

    # Function
    my $function;

    # Variable
    my ($linkage, $type, $name);

    if(s/^WINE_(?:DEFAULT|DECLARE)_DEBUG_CHANNEL\s*\(\s*(\w+)\s*\)\s*//s) { # FIXME: Wine specific kludge
	$self->_update_c_position($&, \$line, \$column);
    } elsif(s/^__ASM_GLOBAL_FUNC\(\s*(\w+)\s*,\s*//s) { # FIXME: Wine specific kludge
	$self->_update_c_position($&, \$line, \$column);
	$self->_parse_c_until_one_of("\)", \$_, \$line, \$column);
	if(s/\)//) {
	    $column++;
	}
    } elsif(s/^__ASM_STDCALL_FUNC\(\s*(\w+)\s*,\s*\d+\s*,\s*//s) { # FIXME: Wine specific kludge
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
	if($self->{FOUND_FUNCTION}($function))
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

sub _parse_c($$$$$$)
{
    my ($self, $pattern, $refcurrent, $refline, $refcolumn, $refmatch) = @_;

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

sub parse_c_enum($$$$)
{
    my ($self, $refcurrent, $refline, $refcolumn) = @_;

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

sub parse_c_expression($$$$)
{
    my ($self, $refcurrent, $refline, $refcolumn) = @_;

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

	    if($self->{FOUND_FUNCTION_CALL}($begin_line, $begin_column, $line, $column, $name, \@arguments))
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

sub parse_c_file($$$$)
{
    my ($self, $refcurrent, $refline, $refcolumn) = @_;

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    my $declaration = "";
    my $declaration_line = $line;
    my $declaration_column = $column;

    my $previous_line = 0;
    my $previous_column = -1;

    my $preprocessor_condition = "";
    my $if = 0;
    my $if0 = 0;
    my $extern_c = 0;

    my $blevel = 1;
    my $plevel = 1;
    while($plevel > 0 || $blevel > 0) {
	my $match;
	$self->_parse_c_until_one_of("#/\\(\\)\\[\\]\\{\\};", \$_, \$line, \$column, \$match);

	if($line != $previous_line) {
	    $self->{FOUND_LINE}($line);
	} else {
	    # $self->{FOUND_LINE}("$line.$column");
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
	    } elsif ($declaration =~ s/^(?:DECL_WINELIB_TYPE_AW|DECLARE_HANDLE(?:16)?|TYPE_MARSHAL)\(\s*(\w+)\s*\)\s*//s) {
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
		$self->{FOUND_COMMENT}($line, $column + 1, $&);
	        local $_ = $&;
		while(s/^.*?\n//) {
		    $blank_lines++;
		}
		if($_) {
		    $column += length($_);
		}
	    } elsif(s/^\/\/(.*?)\n//) {
		$self->{FOUND_COMMENT}($line, $column + 1, $&);
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
	    if($plevel <= 0) {
		$self->_parse_c_error($_, $line, $column, "file", ") without (");
	    }
	    $declaration .= $&;
	    if($plevel == 1 && $declaration =~ /^(__ASM_GLOBAL_FUNC|__ASM_STDCALL_FUNC)/) {
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
		$self->_parse_c_warning("", $line, $column, "file", "inner } ends on column 1");
	    }
	} elsif(s/^;//) {
	    $declaration .= $&;
	    if($plevel == 1 && $blevel == 1) {
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

sub parse_c_function($$$$$)
{
    my ($self, $refcurrent, $refline, $refcolumn, $reffunction) = @_;

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

    my $function = $self->{CREATE_FUNCTION}();

    $function->file($self->{FILE});
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

sub parse_c_function_call($$$$$$$$)
{
    my ($self, $refcurrent, $refline, $refcolumn, $refname, $refarguments, $refargument_lines, $refargument_columns) = @_;

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


sub parse_c_preprocessor($$$$)
{
    my ($self, $refcurrent, $refline, $refcolumn) = @_;

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    $self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);

    my $begin_line = $line;
    my $begin_column = $column + 1;

    if(!$self->{FOUND_PREPROCESSOR}($begin_line, $begin_column, "$_")) {
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

sub parse_c_statement($$$$)
{
    my ($self, $refcurrent, $refline, $refcolumn) = @_;

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

sub parse_c_statements($$$$)
{
    my ($self, $refcurrent, $refline, $refcolumn) = @_;

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

sub parse_c_struct_union($$$$$$$$$)
{
    my ($self, $refcurrent, $refline, $refcolumn, $refkind, $ref_name, $reffield_type_names, $reffield_names, $refnames) = @_;

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    my $kind;
    my $_name;
    my @field_type_names = ();
    my @field_names = ();
    my @names = ();

    $self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);

    if (!s/^(interface|struct|union)(\s+((?:MSVCRT|WS)\(\s*\w+\s*\)|\w+))?\s*\{\s*//s) {
	return 0;
    }
    $kind = $1;
    $_name = $3 || "";

    $self->_update_c_position($&, \$line, \$column);

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

sub parse_c_tuple($$$$$$$)
{
    my ($self, $refcurrent, $refline, $refcolumn,
        # FIXME: Should not write directly
        $items, $item_lines, $item_columns) = @_;

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

sub parse_c_type($$$$$)
{
    my ($self, $refcurrent, $refline, $refcolumn, $reftype) = @_;

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

sub parse_c_typedef($$$$)
{
    my ($self, $refcurrent, $refline, $refcolumn) = @_;

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    if (!$self->_parse_c("typedef", \$_, \$line, \$column)) {
	return 0;
    }

    my ($kind, $name, @field_type_names, @field_names, @names);
    my ($linkage, $type_name);
    if ($self->parse_c_enum(\$_, \$line, \$column))
{
        # Nothing to do
    }
    elsif ($self->parse_c_struct_union(\$_, \$line, \$column,
					 \$kind, \$name, \@field_type_names, \@field_names, \@names))
    {
	my $base_name;
        foreach my $_name (@names)
        {
            if ($_name =~ /^\w+$/)
            {
                $base_name = $_name;
                last;
            }
        }
        $base_name="$kind $name" if (!defined $base_name and defined $name);
        $base_name=$kind if (!defined $base_name);
	foreach my $_name (@names) {
	    if ($_name =~ /^\w+$/) {
		my $type = $self->{CREATE_TYPE}();
		
		$type->kind($kind);
		$type->_name($name);
		$type->name($_name);
		$type->field_type_names([@field_type_names]);
		$type->field_names([@field_names]);

		$self->{FOUND_TYPE}($type);
	    } elsif ($_name =~ /^(\*+)\s*(?:RESTRICTED_POINTER\s+)?(\w+)$/) {
		my $type_name = "$base_name $1";
		$_name = $2;

		my $type = $self->{CREATE_TYPE}();

		$type->kind("");
		$type->name($_name);
		$type->field_type_names([$type_name]);
		$type->field_names([""]);

		$self->{FOUND_TYPE}($type);
	    } else {
		$self->_parse_c_error($_, $line, $column, "typedef 2");
	    }
	}
    }
    elsif ($self->parse_c_variable(\$_, \$line, \$column, \$linkage, \$type_name, \$name))
    {
	$type_name =~ s/\s+/ /g;
	
	if(defined($type_name) && defined($name)) {
	    my $type = $self->{CREATE_TYPE}();
	    
	    if (length($name) == 0) {
		$self->_parse_c_error($_, $line, $column, "typedef");
	    }

	    $type->kind("");
	    $type->name($name);
	    $type->field_type_names([$type_name]);
	    $type->field_names([""]);
	    
	    $self->{FOUND_TYPE}($type);
	}
    } else {
	$self->_parse_c_error($_, $line, $column, "typedef");
    }

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;

    return 1;
}

sub parse_c_variable($$$$$$$)
{
    my ($self, $refcurrent, $refline, $refcolumn, $reflinkage, $reftype, $refname) = @_;

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

    return 0 if(/^$/);

  finished: while (1)
    {
        if (s/^(enum\s+|interface\s+|struct\s+|union\s+)((?:MSVCRT|WS)\(\s*\w+\s*\)|\w+)?\s*\{\s*//s) {
            my $kind = $1;
            my $_name = $2;
            $self->_update_c_position($&, \$line, \$column);

            if(defined($_name)) {
                $type = "$kind $_name { }";
            } else {
                $type = "$kind { }";
            }

            last finished;
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

            last finished;
        } elsif(s/^((?:enum\s+|interface\s+|struct\s+|union\s+)?\w+\b(?:\s*\*)*)\s*:\s*(\d+)$//s) {
            $type = "$sign$1:$2";
            $name = "";
            $type = $self->_format_c_type($type);

            last finished;
        } elsif(s/^((?:enum\s+|interface\s+|struct\s+|union\s+)?\w+\b(?:\s*\*)*\s*\(\s*(?:$CALL_CONVENTION)?(?:\s+DECLSPEC_[A-Z]+)?(?:\s*\*)*)\s*(\w+)\s*(\)\s*\(.*?\))$//s) {
            $type = $self->_format_c_type("$sign$1$3");
            $name = $2;

            last finished;
        } elsif($self->_parse_c('DEFINE_GUID', \$_, \$line, \$column, \$match)) { # Windows specific
            $type = $match;
            last finished;
        } else {
            $self->_parse_c_warning($_, $line, $column, "variable", "'$_'");
            last finished;
        }

        if($self->_parse_c('SEQ_DEFINEBUF', \$_, \$line, \$column, \$match)) { # Linux specific
            $type = $match;
            last finished;
        } elsif($self->_parse_c('DEFINE_REGS_ENTRYPOINT_\w+|DPQ_DECL_\w+|HANDLER_DEF|IX86_ONLY', # Wine specific
                                \$_, \$line, \$column, \$match))
        {
            $type = $match;
            last finished;
        } elsif($self->_parse_c('(?:struct\s+)?ICOM_VTABLE\s*\(\w+\)', \$_, \$line, \$column, \$match)) {
            $type = $match;
            last finished;
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

        if(s/^WINAPI\s*//) {
            $self->_update_c_position($&, \$line, \$column);
        }

        if(s/^(\((?:$CALL_CONVENTION)?\s*\*?\s*(?:$CALL_CONVENTION)?\w+\s*(?:\[[^\]]*\]\s*)*\))\s*\(//) {
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
        last finished;
    }

    # $output->write("$type: $name: '$_'\n");

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;

    $$reflinkage = $linkage;
    $$reftype = $type;
    $$refname = $name;

    $self->{FOUND_VARIABLE}($begin_line, $begin_column, $linkage, $type, $name);

    return 1;
}

1;
