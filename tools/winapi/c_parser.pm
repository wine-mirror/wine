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

########################################################################
# new
#
sub new {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    my $file = \${$self->{FILE}};
    my $found_comment = \${$self->{FOUND_COMMENT}};
    my $found_declaration = \${$self->{FOUND_DECLARATION}};
    my $create_function = \${$self->{CREATE_FUNCTION}};
    my $found_function = \${$self->{FOUND_FUNCTION}};
    my $found_function_call = \${$self->{FOUND_FUNCTION_CALL}};
    my $found_preprocessor = \${$self->{FOUND_PREPROCESSOR}};
    my $found_statement = \${$self->{FOUND_STATEMENT}};
    my $found_variable = \${$self->{FOUND_VARIABLE}};

    $$file = shift;

    $$found_comment = sub { return 1; };
    $$found_declaration = sub { return 1; };
    $$create_function = sub { return new c_function; };
    $$found_function = sub { return 1; };
    $$found_function_call = sub { return 1; };
    $$found_preprocessor = sub { return 1; };
    $$found_statement = sub { return 1; };
    $$found_variable = sub { return 1; };

    return $self;
}

########################################################################
# set_found_comment_callback
#
sub set_found_comment_callback {
    my $self = shift;

    my $found_comment = \${$self->{FOUND_COMMENT}};

    $$found_comment = shift;
}

########################################################################
# set_found_declaration_callback
#
sub set_found_declaration_callback {
    my $self = shift;

    my $found_declaration = \${$self->{FOUND_DECLARATION}};

    $$found_declaration = shift;
}

########################################################################
# set_found_function_callback
#
sub set_found_function_callback {
    my $self = shift;

    my $found_function = \${$self->{FOUND_FUNCTION}};

    $$found_function = shift;
}

########################################################################
# set_found_function_call_callback
#
sub set_found_function_call_callback {
    my $self = shift;

    my $found_function_call = \${$self->{FOUND_FUNCTION_CALL}};

    $$found_function_call = shift;
}

########################################################################
# set_found_preprocessor_callback
#
sub set_found_preprocessor_callback {
    my $self = shift;

    my $found_preprocessor = \${$self->{FOUND_PREPROCESSOR}};

    $$found_preprocessor = shift;
}

########################################################################
# set_found_statement_callback
#
sub set_found_statement_callback {
    my $self = shift;

    my $found_statement = \${$self->{FOUND_STATEMENT}};

    $$found_statement = shift;
}

########################################################################
# set_found_variable_callback
#
sub set_found_variable_callback {
    my $self = shift;

    my $found_variable = \${$self->{FOUND_VARIABLE}};

    $$found_variable = shift;
}

########################################################################
# _parse_c

sub _parse_c {
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
# _parse_c_error

sub _parse_c_error {
    my $self = shift;

    my $file = \${$self->{FILE}};

    local $_ = shift;
    my $line = shift;
    my $column = shift;
    my $context = shift;

    my @lines = split(/\n/, $_);

    my $current = "\n";
    $current .= $lines[0] . "\n" || "";
    $current .= $lines[1] . "\n" || "";

    if($output->prefix) {
	$output->write("\n");
	$output->prefix("");
    }
    $output->write("$$file:$line." . ($column + 1) . ": $context: parse error: \\$current");

    exit 1;
}

########################################################################
# _parse_c_output

sub _parse_c_output {
    my $self = shift;

    local $_ = shift;
    my $line = shift;
    my $column = shift;
    my $message = shift;

    my @lines = split(/\n/, $_);

    my $current = "\n";
    $current .= $lines[0] . "\n" || "";
    $current .= $lines[1] . "\n" || "";

    $output->write("$line." . ($column + 1) . ": $message: \\$current");
}

########################################################################
# _parse_c_until_one_of

sub _parse_c_until_one_of {
    my $self = shift;

    my $characters = shift;
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

    $$match = "";
    while(/^[^$characters]/s) {
	my $submatch = "";

	if(s/^[^$characters\n\t\'\"]*//s) {
	    $submatch .= $&;
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
# _update_c_position

sub _update_c_position {
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
# parse_c_block

sub parse_c_block {
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

sub parse_c_declaration {
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

    # $self->_parse_c_output($_, $line, $column, "declaration");

    if(0) {
	# Nothing
    } elsif(s/^(?:DEFAULT|DECLARE)_DEBUG_CHANNEL\s*\(\s*(\w+)\s*\)\s*//s) { # FIXME: Wine specific kludge
	$self->_update_c_position($&, \$line, \$column);
    } elsif(s/^extern\s*\"(.*?)\"\s*//s) {
	$self->_update_c_position($&, \$line, \$column);
	my $declarations;
	my $declarations_line;
	my $declarations_column;
	if(!$self->parse_c_block(\$_, \$line, \$column, \$declarations, \$declarations_line, \$declarations_column)) {
	    return 0;
	}
	if(!$self->parse_c_declarations(\$declarations, \$declarations_line, \$declarations_column)) {
	    return 0;
	}
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
    } elsif($self->parse_c_typedef(\$_, \$line, \$column)) {
	# Nothing
    } elsif($self->parse_c_variable(\$_, \$line, \$column, \$linkage, \$type, \$name)) {
	# Nothing
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

sub parse_c_declarations {
    my $self = shift;

    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    return 1;
}

########################################################################
# parse_c_expression

sub parse_c_expression {
    my $self = shift;

    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    my $found_function_call = \${$self->{FOUND_FUNCTION_CALL}};

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    $self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);

    if(s/^(.*?)(\w+\s*\()/$2/s) {
	$column += length($1);

	my $begin_line = $line;
	my $begin_column = $column + 1;

	my $name;
	my @arguments;
	my @argument_lines;
	my @argument_columns;
	if(!$self->parse_c_function_call(\$_, \$line, \$column, \$name, \@arguments, \@argument_lines, \@argument_columns)) {
	    return 0;
	}

	if($name =~ /^sizeof$/ || 
	   &$$found_function_call($begin_line, $begin_column, $line, $column, $name, \@arguments))
	{
	    while(defined(my $argument = shift @arguments) &&
		  defined(my $argument_line = shift @argument_lines) &&
		  defined(my $argument_column = shift @argument_columns))
	    {
		$self->parse_c_expression(\$argument, \$argument_line, \$argument_column);
	    }
	}
    } elsif(s/^return//) {
	$column += length($&);
	$self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);
	if(!$self->parse_c_expression(\$_, \$line, \$column)) {
	    return 0;
	}
    } else {
	return 0;
    }

    $self->_update_c_position($_, \$line, \$column);

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;

    return 1;
}

########################################################################
# parse_c_file

sub parse_c_file {
    my $self = shift;

    my $found_comment = \${$self->{FOUND_COMMENT}};

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

    my $blevel = 1;
    my $plevel = 1;
    while($plevel > 0 || $blevel > 0) {
	my $match;
	$self->_parse_c_until_one_of("#/\\(\\)\\[\\]\\{\\};", \$_, \$line, \$column, \$match);

	if($line == $previous_line && $column == $previous_column) {
	    # $self->_parse_c_error($_, $line, $column, "file: no progress");
	}
	$previous_line = $line;
	$previous_column = $column;

	# $self->_parse_c_output($_, $line, $column, "'$match'");

	if(!$declaration && $match =~ s/^\s+//s) {
	    $self->_update_c_position($&, \$declaration_line, \$declaration_column);
	}
	$declaration .= $match;

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
		if(s/^(.*?)(\/[\*\/].*)?\n//) {
		    if(defined($2)) {
			$_ = "$2\n$_";
		    } else {
			$blank_lines++;
		    }
		    $preprocessor .= $1;
		}

		if(!$self->parse_c_preprocessor(\$preprocessor, \$preprocessor_line, \$preprocessor_column)) {
		    return 0;
		}
	    } 

	    if(s/^\/\*(.*?)\*\///s) {
		&$$found_comment($line, $column + 1, "/*$1*/");
		my @lines = split(/\n/, $1);
		if($#lines > 0) {
		    $blank_lines += $#lines;
		} else {
		    $column += length($1);
		}
	    } elsif(s/^\/\/(.*?)\n//) {
		&$$found_comment($line, $column + 1, "//$1");
		$blank_lines++;
	    } elsif(s/^\///) {
		$declaration .= $&;
	    }

	    $line += $blank_lines;
	    if($blank_lines > 0) {
		$column = 0;
	    }

	    if(!$declaration) {
		$declaration_line = $line;
		$declaration_column = $column;
	    } else {
		$declaration .= "\n" x $blank_lines;
	    }

	    next;
	} 

	$column++;
	if(s/^[\(\[]//) {
	    $plevel++;
	    $declaration .= $&;
	} elsif(s/^[\)\]]//) {
	    $plevel--;
	    $declaration .= $&;
	} elsif(s/^\{//) {
	    $blevel++;
	    $declaration .= $&;
	} elsif(s/^\}//) {
	    $blevel--;
	    $declaration .= $&;
	    if($plevel == 1 && $blevel == 1 && $declaration !~ /^typedef/) {
		if(!$self->parse_c_declaration(\$declaration, \$declaration_line, \$declaration_column)) {
		    return 0;
		}
		$self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);
		$declaration = "";
		$declaration_line = $line;
		$declaration_column = $column;
	    }
	} elsif(s/^;//) {
	    if($plevel == 1 && $blevel == 1) {
		if($declaration && !$self->parse_c_declaration(\$declaration, \$declaration_line, \$declaration_column)) {
		    return 0;
		}
		$self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);
		$declaration = "";
		$declaration_line = $line;
		$declaration_column = $column;
	    } else {
		$declaration .= $&;
	    }
	} elsif(/^\s*$/ && $declaration =~ /^\s*$/ && $match =~ /^\s*$/) {
	    $plevel = 0;
	    $blevel = 0;
	} else {
	    $self->_parse_c_error($_, $line, $column, "file");
	}
    }

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;

    return 1;
}

########################################################################
# parse_c_function

sub parse_c_function {
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

    $self->_parse_c("inline", \$_, \$line, \$column);
    $self->_parse_c("extern|static", \$_, \$line, \$column, \$linkage);
    $self->_parse_c("inline", \$_, \$line, \$column);
    if(!$self->parse_c_type(\$_, \$line, \$column, \$return_type)) {
	return 0;
    }

    $self->_parse_c("__cdecl|__stdcall|CDECL|VFWAPIV|VFWAPI|WINAPIV|WINAPI|CALLBACK", 
	     \$_, \$line, \$column, \$calling_convention);
    if(!$self->_parse_c("\\w+", \$_, \$line, \$column, \$name)) { 
	return 0; 
    }
    if(!$self->parse_c_tuple(\$_, \$line, \$column, \@arguments, \@argument_lines, \@argument_columns)) {
	return 0; 
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

sub parse_c_function_call {
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

    if(s/^(\w+)(\s*)\(/\(/s) {
	$column += length("$1$2");

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

sub parse_c_preprocessor {
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
    
    if(0) {
	# Nothing
    } elsif(/^\#\s*define\s+(.*?)$/s) {
	$self->_update_c_position($_, \$line, \$column);
    } elsif(/^\#\s*else/s) {
	$self->_update_c_position($_, \$line, \$column);
    } elsif(/^\#\s*endif/s) {
	$self->_update_c_position($_, \$line, \$column);
    } elsif(/^\#\s*(?:if|ifdef|ifndef)?\s+(.*?)$/s) {
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

sub parse_c_statement {
    my $self = shift;

    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    my $found_function_call = \${$self->{FOUND_FUNCTION_CALL}};

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    $self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);

    if(s/^(?:case\s+)?(\w+)\s*://) {
	$column += length($&);
	$self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);
    }

    # $output->write("$line.$column: '$_'\n");

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
    } elsif(/^(for|if|switch|while)(\s*)\(/) {
	$column += length("$1$2");
	my $name = $1;

	$_ = "($'";

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
	$column += length($&);
	if(!$self->parse_c_statement(\$_, \$line, \$column)) {
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

sub parse_c_statements {
    my $self = shift;

    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    my $found_function_call = \${$self->{FOUND_FUNCTION_CALL}};

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    $self->_parse_c_until_one_of("\\S", \$_, \$line, \$column);

    my $statement = "";
    my $statement_line = $line;
    my $statement_column = $column;

    my $blevel = 1;
    my $plevel = 1;
    while($plevel > 0 || $blevel > 0) {
	my $match;
	$self->_parse_c_until_one_of("\\(\\)\\[\\]\\{\\};", \$_, \$line, \$column, \$match);

	# $output->write("'$match' '$_'\n");

	$column++;
	$statement .= $match;
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
# parse_c_tuple

sub parse_c_tuple {
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

sub parse_c_type {
    my $self = shift;

    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    my $reftype = shift;

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    my $type;

    $self->_parse_c("const", \$_, \$line, \$column);


    if(0) {
	# Nothing
    } elsif($self->_parse_c('ICOM_VTABLE\(.*?\)', \$_, \$line, \$column, \$type)) {
		# Nothing
    } elsif($self->_parse_c('\w+\s*(\*\s*)*', \$_, \$line, \$column, \$type)) {
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

sub parse_c_typedef {
    my $self = shift;

    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    my $reftype = shift;

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    my $type;

    if(!$self->_parse_c("typedef", \$_, \$line, \$column)) {
	return 0;
    }

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;

    $$reftype = $type;

    return 1;
}

########################################################################
# parse_c_variable

sub parse_c_variable {
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
    my $type;
    my $name;

    $self->_parse_c("extern|static", \$_, \$line, \$column, \$linkage);
    if(!$self->parse_c_type(\$_, \$line, \$column, \$type)) { return 0; }
    if(!$self->_parse_c("\\w+", \$_, \$line, \$column, \$name)) { return 0; }

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
