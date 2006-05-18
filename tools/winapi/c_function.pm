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

package c_function;

use strict;

sub new($) {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    return $self;
}

sub file($$) {
    my $self = shift;
    my $file = \${$self->{FILE}};

    local $_ = shift;

    if(defined($_)) { $$file = $_; }

    return $$file;
}

sub begin_line($$) {
    my $self = shift;
    my $begin_line = \${$self->{BEGIN_LINE}};

    local $_ = shift;

    if(defined($_)) { $$begin_line = $_; }

    return $$begin_line;
}

sub begin_column($$) {
    my $self = shift;
    my $begin_column = \${$self->{BEGIN_COLUMN}};

    local $_ = shift;

    if(defined($_)) { $$begin_column = $_; }

    return $$begin_column;
}

sub end_line($$) {
    my $self = shift;
    my $end_line = \${$self->{END_LINE}};

    local $_ = shift;

    if(defined($_)) { $$end_line = $_; }

    return $$end_line;
}

sub end_column($$) {
    my $self = shift;
    my $end_column = \${$self->{END_COLUMN}};

    local $_ = shift;

    if(defined($_)) { $$end_column = $_; }

    return $$end_column;
}

sub linkage($$) {
    my $self = shift;
    my $linkage = \${$self->{LINKAGE}};

    local $_ = shift;

    if(defined($_)) { $$linkage = $_; }

    return $$linkage;
}

sub return_type($$) {
    my $self = shift;
    my $return_type = \${$self->{RETURN_TYPE}};

    local $_ = shift;

    if(defined($_)) { $$return_type = $_; }

    return $$return_type;
}

sub calling_convention($$) {
    my $self = shift;
    my $calling_convention = \${$self->{CALLING_CONVENTION}};

    local $_ = shift;

    if(defined($_)) { $$calling_convention = $_; }

    return $$calling_convention;
}

sub name($$) {
    my $self = shift;
    my $name = \${$self->{NAME}};

    local $_ = shift;

    if(defined($_)) { $$name = $_; }

    return $$name;
}

sub argument_types($$) {
    my $self = shift;
    my $argument_types = \${$self->{ARGUMENT_TYPES}};

    local $_ = shift;

    if(defined($_)) { $$argument_types = $_; }

    return $$argument_types;
}

sub argument_names($$) {
    my $self = shift;
    my $argument_names = \${$self->{ARGUMENT_NAMES}};

    local $_ = shift;

    if(defined($_)) { $$argument_names = $_; }

    return $$argument_names;
}

sub statements_line($$) {
    my $self = shift;
    my $statements_line = \${$self->{STATEMENTS_LINE}};

    local $_ = shift;

    if(defined($_)) { $$statements_line = $_; }

    return $$statements_line;
}

sub statements_column($$) {
    my $self = shift;
    my $statements_column = \${$self->{STATEMENTS_COLUMN}};

    local $_ = shift;

    if(defined($_)) { $$statements_column = $_; }

    return $$statements_column;
}

sub statements($$) {
    my $self = shift;
    my $statements = \${$self->{STATEMENTS}};

    local $_ = shift;

    if(defined($_)) { $$statements = $_; }

    return $$statements;
}

1;
