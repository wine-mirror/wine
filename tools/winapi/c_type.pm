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

use output qw($output);

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

sub pack {
    my $self = shift;
    my $pack = \${$self->{PACK}};
    
    local $_ = shift;

    if(defined($_)) { $$pack = $_; }

    return $$pack;
}

sub fields {
    my $self = shift;

    my $find_size = shift;

    if (defined($find_size)) {
	$self->_refresh($find_size);
    }

    my $count = $self->field_count;

    my @fields = ();
    for (my $n = 0; $n < $count; $n++) {
	my $field = 'c_type_field'->new($self, $n);
	push @fields, $field;
    }
    return @fields;
}

sub field_aligns {
    my $self = shift;
    my $field_aligns = \${$self->{FIELD_ALIGNS}};

    my $find_size = shift;

    if (defined($find_size)) {
	$self->_refresh($find_size);
    }

    return $$field_aligns;
}

sub field_count {
    my $self = shift;
    my $field_type_names = \${$self->{FIELD_TYPE_NAMES}};

    my @field_type_names = @{$$field_type_names}; 
    my $count = scalar(@field_type_names);

    return $count;
}

sub field_names {
    my $self = shift;
    my $field_names = \${$self->{FIELD_NAMES}};

    local $_ = shift;

    if(defined($_)) { $$field_names = $_; }

    return $$field_names;
}

sub field_offsets {
    my $self = shift;
    my $field_offsets = \${$self->{FIELD_OFFSETS}};

    my $find_size = shift;

    if (defined($find_size)) {
	$self->_refresh($find_size);
    }

    return $$field_offsets;
}

sub field_sizes {
    my $self = shift;
    my $field_sizes = \${$self->{FIELD_SIZES}};

    my $find_size = shift;

    if (defined($find_size)) {
	$self->_refresh($find_size);
    }

    return $$field_sizes;
}

sub field_type_names {
    my $self = shift;
    my $field_type_names = \${$self->{FIELD_TYPE_NAMES}};

    local $_ = shift;

    if(defined($_)) { $$field_type_names = $_; }

    return $$field_type_names;
}

sub size {
    my $self = shift;

    my $size = \${$self->{SIZE}};

    my $find_size = shift;
    if (defined($find_size)) {
	$self->_refresh($find_size);
    }

    return $$size;
}

sub _refresh {
    my $self = shift;

    my $field_aligns = \${$self->{FIELD_ALIGNS}};
    my $field_offsets = \${$self->{FIELD_OFFSETS}};
    my $field_sizes = \${$self->{FIELD_SIZES}};
    my $size = \${$self->{SIZE}};

    my $find_size = shift;

    my $pack = $self->pack;

    my $offset = 0;
    my $offset_bits = 0;

    my $n = 0;
    foreach my $field ($self->fields) {
	my $type_name = $field->type_name;
	my $size = &$find_size($type_name);


	my $base_type_name = $type_name;
	if ($base_type_name =~ s/^(.*?)\s*(?:\[\s*(.*?)\s*\]|:(\d+))?$/$1/) {
	    my $count = $2;
	    my $bits = $3;
	}
	my $base_size = &$find_size($base_type_name);

	my $align = 0;
	$align = $base_size % 4 if defined($base_size);
	$align = 4 if !$align;

	if (!defined($size)) {
	    $$size = undef;
	    return;
	} elsif ($size >= 0) {
	    if ($offset_bits) {
		$offset += $pack * int(($offset_bits + 8 * $pack - 1 ) / (8 * $pack));
		$offset_bits = 0;
	    }

	    $$$field_aligns[$n] = $align;
	    $$$field_offsets[$n] = $offset;
	    $$$field_sizes[$n] = $size;

	    $offset += $size;
	} else {
	    $$$field_aligns[$n] = $align;
	    $$$field_offsets[$n] = $offset;
	    $$$field_sizes[$n] = $size;

	    $offset_bits += -$size;
	}

	$n++;
    }

    $$size = $offset;
    if (1) {
	if ($$size % $pack != 0) {
	    $$size = (int($$size / $pack) + 1) * $pack;
	}
    }
}

package c_type_field;

sub new {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    my $type = \${$self->{TYPE}};
    my $number = \${$self->{NUMBER}};

    $$type = shift;
    $$number = shift;

    return $self;
}

sub align {
    my $self = shift;
    my $type = \${$self->{TYPE}};
    my $number = \${$self->{NUMBER}};

    my $field_aligns = $$type->field_aligns;

    return $$field_aligns[$$number];
}

sub name {
    my $self = shift;
    my $type = \${$self->{TYPE}};
    my $number = \${$self->{NUMBER}};

    my $field_names = $$type->field_names;

    return $$field_names[$$number];
}

sub offset {
    my $self = shift;
    my $type = \${$self->{TYPE}};
    my $number = \${$self->{NUMBER}};

    my $field_offsets = $$type->field_offsets;

    return $$field_offsets[$$number];
}

sub size {
    my $self = shift;
    my $type = \${$self->{TYPE}};
    my $number = \${$self->{NUMBER}};

    my $field_sizes = $$type->field_sizes;

    return $$field_sizes[$$number];
}

sub type_name {
    my $self = shift;
    my $type = \${$self->{TYPE}};
    my $number = \${$self->{NUMBER}};

    my $field_type_names = $$type->field_type_names;

    return $$field_type_names[$$number];
}

1;
