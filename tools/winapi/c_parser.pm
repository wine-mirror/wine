package c_parser;

use strict;

use options qw($options);
use output qw($output);

sub _update_c_position {
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

sub parse_c {
    my $pattern = shift;
    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    if(s/$pattern//) {
	_update_c_position($&, \$line, \$column);
    } else {
	return 0;
    }

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;

    return 1;
}

sub parse_c_until_one_of {
    my $characters = shift;
    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;
    my $match = shift;

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    if(!defined($line) || !defined($column)) {
	$output->write("error: \$characters = '$characters' \$_ = '$_'\n");
	exit 1;
    }

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

sub parse_c_block {
    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;
    my $refstatements = shift;
    my $refstatements_line = shift;
    my $refstatements_column = shift;

    local $_ = $$refcurrent;
    my $line = $$refline;
    my $column = $$refcolumn;

    my $statements;
    if(s/^\{//) {
	$column++;
	$statements = "";
    } else {
	return 0;
    }

    parse_c_until_one_of("\\S", \$_, \$line, \$column);

    my $statements_line = $line;
    my $statements_column = $column;

    my $plevel = 1;
    while($plevel > 0) {
	my $match;
	parse_c_until_one_of("\\{\\}", \$_, \$line, \$column, \$match);

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

sub parse_c_expression {
    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;
    my $found_function_call_callback = shift;

    my $line = $$refline;
    my $column = $$refcolumn;

    local $_ = $$refcurrent;

    parse_c_until_one_of("\\S", \$_, \$line, \$column);

    if(s/^(.*?)(\w+)(\s*)\(//s) {
	my $begin_line = $line;
	my $begin_column = $column + length($1) + 1;

	$line = $begin_line;
	$column = $begin_column + length("$2$3") - 1;

	my $name = $2;

	$_ = "($'";

	# $output->write("$name: $line.$column: '$_'\n");

	my @arguments;
	my @argument_lines;
	my @argument_columns;
	if(!parse_c_tuple(\$_, \$line, \$column, \@arguments, \@argument_lines, \@argument_columns)) {
	    return 0;
	}

	if($name =~ /^sizeof$/) {
	    # Nothing
	} else {
	    &$found_function_call_callback($begin_line, $begin_column, $line, $column, 
					   $name, \@arguments);
	}

	while(defined(my $argument = shift @arguments) &&
	      defined(my $argument_line = shift @argument_lines) &&
	      defined(my $argument_column = shift @argument_columns))
	{
	    parse_c_expression(\$argument, \$argument_line, \$argument_column, $found_function_call_callback);
	}
    } elsif(s/^return//) {
	$column += length($&);
	parse_c_until_one_of("\\S", \$_, \$line, \$column);
	if(!parse_c_expression(\$_, \$line, \$column, $found_function_call_callback)) {
	    return 0;
	}
    } else {
	return 0;
    }

    _update_c_position($_, \$line, \$column);

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;

    return 1;
}

sub parse_c_statement {
    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;
    my $found_function_call_callback = shift;

    my $line = $$refline;
    my $column = $$refcolumn;

    local $_ = $$refcurrent;

    parse_c_until_one_of("\\S", \$_, \$line, \$column);

    if(s/^(?:case\s+)?(\w+)\s*://) {
	$column += length($&);
	parse_c_until_one_of("\\S", \$_, \$line, \$column);
    }

    # $output->write("$line.$column: '$_'\n");

    if(/^$/) {
	# Nothing
    } elsif(/^\{/) {
	my $statements;
	my $statements_line;
	my $statements_column;
	if(!parse_c_block(\$_, \$line, \$column, \$statements, \$statements_line, \$statements_column)) {
	    return 0;
	}
	if(!parse_c_statements(\$statements, \$statements_line, \$statements_column, $found_function_call_callback)) {
	    return 0;
	}
    } elsif(/^(for|if|switch|while)(\s*)\(/) {
	$column += length("$1$2");
	my $name = $1;

	$_ = "($'";

	my @arguments;
	my @argument_lines;
	my @argument_columns;
	if(!parse_c_tuple(\$_, \$line, \$column, \@arguments, \@argument_lines, \@argument_columns)) {
	    return 0;
	}

	parse_c_until_one_of("\\S", \$_, \$line, \$column);
	if(!parse_c_statement(\$_, \$line, \$column, $found_function_call_callback)) {
	    return 0;
	}
	parse_c_until_one_of("\\S", \$_, \$line, \$column);

	while(defined(my $argument = shift @arguments) &&
	      defined(my $argument_line = shift @argument_lines) &&
	      defined(my $argument_column = shift @argument_columns))
	{
	    parse_c_expression(\$argument, \$argument_line, \$argument_column, $found_function_call_callback);
	}
    } elsif(s/^else//) {
	$column += length($&);
	if(!parse_c_statement(\$_, \$line, \$column, $found_function_call_callback)) {
	    return 0;
	}
    } elsif(parse_c_expression(\$_, \$line, \$column, $found_function_call_callback)) {
	# Nothing
    } else {
	# $output->write("error '$_'\n");
	# exit 1;
    }

    _update_c_position($_, \$line, \$column);

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;

    return 1;
}

sub parse_c_statements {
    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;
    my $found_function_call_callback = shift;

    my $line = $$refline;
    my $column = $$refcolumn;

    local $_ = $$refcurrent;

    parse_c_until_one_of("\\S", \$_, \$line, \$column);
    my $statement = "";
    my $statement_line = $line;
    my $statement_column = $column;

    my $blevel = 1;
    my $plevel = 1;
    while($plevel > 0 || $blevel > 0) {
	my $match;
	parse_c_until_one_of("\\(\\)\\[\\]\\{\\};", \$_, \$line, \$column, \$match);

	# $output->write("'$match' '$_'\n");

	$column++;
	$statement .= $match;
	if(s/^[\(\[]//) {
	    $plevel++;
	    $statement .= $&;
	} elsif(s/^[\)\]]//) {
	    $plevel--;
	    if($plevel <= 0) {
		$output->write("error $plevel: '$statement' '$match' '$_'\n");
		exit 1;
	    }
	    $statement .= $&;
	} elsif(s/^\{//) {
	    $blevel++;
	    $statement .= $&;
	} elsif(s/^\}//) {
	    $blevel--;
	    $statement .= $&;
	    if($blevel == 1) {
		if(!parse_c_statement(\$statement, \$statement_line, \$statement_column, $found_function_call_callback)) {
		    return 0;
		}
		parse_c_until_one_of("\\S", \$_, \$line, \$column);
		$statement = "";
		$statement_line = $line;
		$statement_column = $column;
	    }
	} elsif(s/^;//) {
	    if($plevel == 1 && $blevel == 1) {
		if(!parse_c_statement(\$statement, \$statement_line, \$statement_column, $found_function_call_callback)) {
		    return 0;
		}

		parse_c_until_one_of("\\S", \$_, \$line, \$column);
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
	    $output->write("error $plevel: '$statement' '$match' '$_'\n");
	    exit 1;
	}
    }

    _update_c_position($_, \$line, \$column);

    $$refcurrent = $_;
    $$refline = $line;
    $$refcolumn = $column;

    return 1;
}

sub parse_c_tuple {
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
	parse_c_until_one_of("\\(,\\)", \$_, \$line, \$column, \$match);

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
		parse_c_until_one_of("\\S", \$_, \$line, \$column);
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

1;
