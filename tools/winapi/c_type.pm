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
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
#

package c_type;

use strict;
use warnings 'all';

use output qw($output);

sub _refresh($);

sub new($)
{
    my ($proto) = @_;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless $self, $class;

    return $self;
}

#
# Callback setters
#

sub set_find_align_callback($$)
{
    my ($self, $find_align) = @_;
    $self->{FIND_ALIGN} = $find_align;
}

sub set_find_kind_callback($$)
{
    my ($self, $find_kind) = @_;
    $self->{FIND_KIND} = $find_kind;
}

sub set_find_size_callback($$)
{
    my ($self, $find_size) = @_;
    $self->{FIND_SIZE} = $find_size;
}

sub set_find_count_callback($$)
{
    my ($self, $find_count) = @_;
    $self->{FIND_COUNT} = $find_count;
}


#
# Property setter / getter functions (each does both)
#

sub kind($;$)
{
    my ($self, $kind) = @_;
    if (defined $kind)
    {
        $self->{KIND} = $kind;
	$self->{DIRTY} = 1;
    }
    $self->_refresh() if (!defined $self->{KIND});
    return $self->{KIND};
}

sub _name($;$)
{
    my ($self, $_name) = @_;
    if (defined $_name)
    {
        $self->{_NAME} = $_name;
	$self->{DIRTY} = 1;
    }
    return $self->{_NAME};
}

sub name($;$)
{
    my ($self, $name) = @_;
    if (defined $name)
    {
        $self->{NAME} = $name;
	$self->{DIRTY} = 1;
    }
    return $self->{NAME} if ($self->{NAME});
    return "$self->{KIND} $self->{_NAME}";
}

sub pack($;$)
{
    my ($self, $pack) = @_;
    if (defined $pack)
    {
        $self->{PACK} = $pack;
	$self->{DIRTY} = 1;
    }
    return $self->{PACK};
}

sub align($)
{
    my ($self) = @_;
    $self->_refresh();
    return $self->{ALIGN};
}

sub fields($)
{
    my ($self) = @_;

    my $count = $self->field_count;

    my @fields = ();
    for (my $n = 0; $n < $count; $n++) {
	my $field = 'c_type_field'->new($self, $n);
	push @fields, $field;
    }
    return @fields;
}

sub field_base_sizes($)
{
    my ($self) = @_;
    $self->_refresh();
    return $self->{FIELD_BASE_SIZES};
}

sub field_aligns($)
{
    my ($self) = @_;
    $self->_refresh();
    return $self->{FIELD_ALIGNS};
}

sub field_count($)
{
    my ($self) = @_;
    return scalar @{$self->{FIELD_TYPE_NAMES}};
}

sub field_names($;$)
{
    my ($self, $field_names) = @_;
    if (defined $field_names)
    {
        $self->{FIELD_NAMES} = $field_names;
	$self->{DIRTY} = 1;
    }
    return $self->{FIELD_NAMES};
}

sub field_offsets($)
{
    my ($self) = @_;
    $self->_refresh();
    return $self->{FIELD_OFFSETS};
}

sub field_sizes($)
{
    my ($self) = @_;
    $self->_refresh();
    return $self->{FIELD_SIZES};
}

sub field_type_names($;$)
{
    my ($self, $field_type_names) = @_;
    if (defined $field_type_names)
    {
        $self->{FIELD_TYPE_NAMES} = $field_type_names;
	$self->{DIRTY} = 1;
    }
    return $self->{FIELD_TYPE_NAMES};
}

sub size($)
{
    my ($self) = @_;
    $self->_refresh();
    return $self->{SIZE};
}

sub _refresh($)
{
    my ($self) = @_;
    return if (!$self->{DIRTY});

    my $pack = $self->pack;
    $pack = 8 if !defined($pack);

    my $max_field_align = 0;

    my $offset = 0;
    my $bitfield_size = 0;
    my $bitfield_bits = 0;

    my $n = 0;
    foreach my $field ($self->fields())
    {
	my $type_name = $field->type_name;

        my $bits;
	my $count;
        if ($type_name =~ s/^(.*?)\s*(?:\[\s*(.*?)\s*\]|:(\d+))?$/$1/)
        {
            $count = $2;
            $bits = $3;
	}
        my $declspec_align;
        if ($type_name =~ s/\s+DECLSPEC_ALIGN\((\d+)\)//)
        {
            $declspec_align=$1;
        }
        my $base_size = $self->{FIND_SIZE}($type_name);
        my $type_size=$base_size;
        if (defined $count)
        {
            $count=$self->{FIND_COUNT}($count) if ($count !~ /^\d+$/);
            if (!defined $count)
            {
                $type_size=undef;
            }
            else
            {
                if (!defined $type_size)
                {
                    print STDERR "$type_name -> type_size=undef, count=$count\n";
                }
                else
                {
                    $type_size *= int($count);
                }
            }
        }
        if ($bitfield_size != 0)
        {
            if (($type_name eq "" and defined $bits and $bits == 0) or
                (defined $type_size and $bitfield_size != $type_size) or
                !defined $bits or
                $bitfield_bits + $bits > 8 * $bitfield_size)
            {
                # This marks the end of the previous bitfield
                $bitfield_size=0;
                $bitfield_bits=0;
            }
            else
            {
                $bitfield_bits+=$bits;
                $n++;
                next;
            }
        }

        $self->{ALIGN} = $self->{FIND_ALIGN}($type_name);
        $self->{ALIGN} = $declspec_align if (defined $declspec_align);

        if (defined $self->{ALIGN})
        {
            $self->{ALIGN} = $pack if ($self->{ALIGN} > $pack);
            $max_field_align = $self->{ALIGN} if ($self->{ALIGN}) > $max_field_align;

            if ($offset % $self->{ALIGN} != 0) {
                $offset = (int($offset / $self->{ALIGN}) + 1) * $self->{ALIGN};
            }
        }

        if ($self->{KIND} !~ /^(?:struct|union)$/)
        {
            $self->{KIND} = $self->{FIND_KIND}($type_name) || "";
        }

        if (!$type_size)
        {
            $self->{ALIGN} = undef;
            $self->{SIZE} = undef;
            return;
        }

        $self->{FIELD_ALIGNS}->[$n] = $self->{ALIGN};
        $self->{FIELD_BASE_SIZES}->[$n] = $base_size;
        $self->{FIELD_OFFSETS}->[$n] = $offset;
        $self->{FIELD_SIZES}->[$n] = $type_size;
        $offset += $type_size;

        if ($bits)
        {
            $bitfield_size=$type_size;
            $bitfield_bits=$bits;
        }
	$n++;
    }

    $self->{ALIGN} = $pack;
    $self->{ALIGN} = $max_field_align if ($max_field_align < $pack);

    $self->{SIZE} = $offset;
    if ($self->{KIND} =~ /^(?:struct|union)$/) {
	if ($self->{SIZE} % $self->{ALIGN} != 0) {
	    $self->{SIZE} = (int($self->{SIZE} / $self->{ALIGN}) + 1) * $self->{ALIGN};
	}
    }

    $self->{DIRTY} = 0;
}

package c_type_field;

sub new($$$)
{
    my ($proto, $type, $number) = @_;
    my $class = ref($proto) || $proto;
    my $self  = {TYPE=> $type,
		 NUMBER => $number
		};
    bless $self, $class;
    return $self;
}

sub align($)
{
    my ($self) = @_;
    return undef unless defined $self->{TYPE}->field_aligns();
    return $self->{TYPE}->field_aligns()->[$self->{NUMBER}];
}

sub base_size($)
{
    my ($self) = @_;
    return undef unless defined $self->{TYPE}->field_base_sizes();
    return $self->{TYPE}->field_base_sizes()->[$self->{NUMBER}];
}

sub name($)
{
    my ($self) = @_;
    return undef unless defined $self->{TYPE}->field_names();
    return $self->{TYPE}->field_names()->[$self->{NUMBER}];
}

sub offset($)
{
    my ($self) = @_;
    return undef unless defined $self->{TYPE}->field_offsets();
    return $self->{TYPE}->field_offsets()->[$self->{NUMBER}];
}

sub size($)
{
    my ($self) = @_;
    return undef unless defined $self->{TYPE}->field_sizes();
    return $self->{TYPE}->field_sizes()->[$self->{NUMBER}];
}

sub type_name($)
{
    my ($self) = @_;
    return $self->{TYPE}->field_type_names()->[$self->{NUMBER}];
}

1;
