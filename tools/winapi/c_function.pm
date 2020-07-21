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
use warnings 'all';

sub new($)
{
    my ($proto) = @_;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    return $self;
}


#
# Property setter / getter functions (each does both)
#

sub file($;$)
{
    my ($self, $filename) = @_;
    $self->{file} = $filename if (defined $filename);
    return $self->{file};
}

sub begin_line($$)
{
    my ($self, $begin_line) = @_;
    $self->{begin_line} = $begin_line if (defined $begin_line);
    return $self->{begin_line};
}

sub begin_column($;$)
{
    my ($self, $begin_column) = @_;
    $self->{begin_column} = $begin_column if (defined $begin_column);
    return $self->{begin_column};
}

sub end_line($;$)
{
    my ($self, $end_line) = @_;
    $self->{end_line} = $end_line if (defined $end_line);
    return $self->{end_line};
}

sub end_column($;$)
{
    my ($self, $end_column) = @_;
    $self->{end_column} = $end_column if (defined $end_column);
    return $self->{end_column};
}

sub linkage($;$)
{
    my ($self, $linkage) = @_;
    $self->{linkage} = $linkage if (defined $linkage);
    return $self->{linkage};
}

sub return_type($;$)
{
     my ($self, $return_type) = @_;
    $self->{return_type} = $return_type if (defined $return_type);
    return $self->{return_type};
}

sub calling_convention($;$)
{
    my ($self, $calling_convention) = @_;
    $self->{calling_convention} = $calling_convention if (defined $calling_convention);
    return $self->{calling_convention};
}

sub name($;$)
{
    my ($self, $name) = @_;
    $self->{name} = $name if (defined $name);
    return $self->{name};
}

sub argument_types($;$)
{
    my ($self, $argument_types) = @_;
    $self->{argument_types} = $argument_types if (defined $argument_types);
    return $self->{argument_types};
}

sub argument_names($;$)
{
    my ($self, $argument_names) = @_;
    $self->{argument_names} = $argument_names if (defined $argument_names);
    return $self->{argument_names};
}

sub statements_line($;$)
{
    my ($self, $statements_line) = @_;
    $self->{statements_line} = $statements_line if (defined $statements_line);
    return $self->{statements_line};
}

sub statements_column($;$)
{
    my ($self, $statements_column) = @_;
    $self->{statements_column} = $statements_column if (defined $statements_column);
    return $self->{statements_column};
}

sub statements($;$)
{
    my ($self, $statements) = @_;
    $self->{statements} = $statements if (defined $statements);
    return $self->{statements};
}

1;
