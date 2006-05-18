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

package winapi_c_parser;

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
use function;
use winapi_function;
use winapi_parser;

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
# parse_c_file

sub parse_c_file($$$$) {
    my $self = shift;

    my $file = \${$self->{FILE}};

    my $found_comment = \${$self->{FOUND_COMMENT}};
    my $found_declaration = \${$self->{FOUND_DECLARATION}};
    my $create_function = \${$self->{CREATE_FUNCTION}};
    my $found_function = \${$self->{FOUND_FUNCTION}};
    my $found_function_call = \${$self->{FOUND_FUNCTION_CALL}};
    my $found_line = \${$self->{FOUND_LINE}};
    my $found_preprocessor = \${$self->{FOUND_PREPROCESSOR}};
    my $found_statement = \${$self->{FOUND_STATEMENT}};
    my $found_variable = \${$self->{FOUND_VARIABLE}};

    my $refcurrent = shift;
    my $refline = shift;
    my $refcolumn = shift;

    my $_create_function = sub {
	return 'function'->new;
    };

    my $_create_type = sub {
	return 'type'->new;
    };

    my $_found_function = sub {
	my $old_function = shift;

	my $function = new c_function;

	$function->file($old_function->file);
	
	$function->begin_line($old_function->function_line);
	$function->begin_column(0);
	$function->end_line($old_function->function_line);
	$function->end_column(0);

	$function->linkage($old_function->linkage);
	$function->return_type($old_function->return_type);
	$function->calling_convention($old_function->calling_convention);
	$function->name($old_function->internal_name);

	if(defined($old_function->argument_types)) {
	    $function->argument_types([@{$old_function->argument_types}]);
	}
	if(defined($old_function->argument_names)) {
	    $function->argument_names([@{$old_function->argument_names}]);
	}

	$function->statements_line($old_function->statements_line);
	$function->statements_column(0);
	$function->statements($old_function->statements);
	
	&$$found_function($function);
    };

    my $_found_preprocessor = sub {
	my $directive = shift;
	my $argument = shift;

	my $begin_line = 0;
	my $begin_column = 0;
	my $preprocessor = "#$directive $argument";

	&$$found_preprocessor($begin_line, $begin_column, $preprocessor);
    };

    my $_found_type = sub {
	my $type = shift;
    };

    winapi_parser::parse_c_file($$file, {
	# c_comment_found => $found_c_comment,
	# cplusplus_comment_found => $found_cplusplus_comment,
	function_create => $_create_function,
	function_found => $_found_function,
	preprocessor_found => $_found_preprocessor,
	type_create => $_create_type,
	type_found => $_found_type,
    });
}

1;
