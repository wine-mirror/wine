#
# Copyright 2002 Patrik Stridvall
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

package c_type;

use strict;

sub new {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    return $self;
}

sub kind {
    my $self = shift;
    my $kind = \${$self->{KIND}};

    local $_ = shift;

    if(defined($_)) { $$kind = $_; }

    return $$kind;
}

sub _name {
    my $self = shift;
    my $_name = \${$self->{_NAME}};

    local $_ = shift;

    if(defined($_)) { $$_name = $_; }

    return $$_name;
}

sub name {
    my $self = shift;
    my $name = \${$self->{NAME}};

    local $_ = shift;

    if(defined($_)) { $$name = $_; }

    if($$name) {
	return $$name;
    } else {
	my $kind = \${$self->{KIND}};
	my $_name = \${$self->{_NAME}};

	return "$$kind $$_name";
    }
}

sub fields {
    my $self = shift;

    my @field_types = @{$self->field_types};
    my @field_names = @{$self->field_names};

    my $count = scalar(@field_types);
    
    my @fields = ();
    for (my $n = 0; $n < $count; $n++) {
	push @fields, [$field_types[$n], $field_names[$n]];
    }
    return @fields;
}

sub field_names {
    my $self = shift;
    my $field_names = \${$self->{FIELD_NAMES}};

    local $_ = shift;

    if(defined($_)) { $$field_names = $_; }

    return $$field_names;
}

sub field_types {
    my $self = shift;
    my $field_types = \${$self->{FIELD_TYPES}};

    local $_ = shift;

    if(defined($_)) { $$field_types = $_; }

    return $$field_types;
}

1;
