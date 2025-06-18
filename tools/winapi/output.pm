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

package output;

use strict;
use warnings 'all';

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw($output);

use vars qw($output);

$output = '_output'->new;

package _output;

use strict;
use warnings 'all';

my $stdout_isatty = -t STDOUT;
my $stderr_isatty = -t STDERR;

sub new($) {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    my $progress_enabled = \${$self->{PROGRESS_ENABLED}};
    my $progress = \${$self->{PROGRESS}};
    my $last_progress = \${$self->{LAST_PROGRESS}};
    my $last_time = \${$self->{LAST_TIME}};
    my $progress_count = \${$self->{PROGRESS_COUNT}};
    my $prefix = \${$self->{PREFIX}};
    my $prefix_callback = \${$self->{PREFIX_CALLBACK}};

    $$progress_enabled = 1;
    $$progress = "";
    $$last_progress = "";
    $$last_time = 0;
    $$progress_count = 0;
    $$prefix = undef;
    $$prefix_callback = undef;

    return $self;
}

sub DESTROY {
    my $self = shift;

    $self->hide_progress;
}

sub enable_progress($) {
    my $self = shift;
    my $progress_enabled = \${$self->{PROGRESS_ENABLED}};

    $$progress_enabled = 1;
}

sub disable_progress($) {
    my $self = shift;
    my $progress_enabled = \${$self->{PROGRESS_ENABLED}};

    $$progress_enabled = 0;
}

sub show_progress($) {
    my $self = shift;
    my $progress_enabled = \${$self->{PROGRESS_ENABLED}};
    my $progress = ${$self->{PROGRESS}};
    my $last_progress = \${$self->{LAST_PROGRESS}};
    my $progress_count = \${$self->{PROGRESS_COUNT}};

    $$progress_count++;

    if($$progress_enabled) {
	if($$progress_count > 0 && $$progress && $stderr_isatty) {
            # If progress has more than $columns characters the xterm will
            # scroll to the next line and our ^H characters will fail to
            # erase it.
            my $columns=$ENV{COLUMNS} || 80;
            $progress = substr $progress,0,($columns-1);
	    print STDERR $progress;
	    $$last_progress = $progress;
	}
    }
}

sub hide_progress($)  {
    my $self = shift;
    my $progress_enabled = \${$self->{PROGRESS_ENABLED}};
    my $progress = \${$self->{PROGRESS}};
    my $last_progress = \${$self->{LAST_PROGRESS}};
    my $progress_count = \${$self->{PROGRESS_COUNT}};

    $$progress_count--;

    if($$progress_enabled) {
	if($$last_progress && $stderr_isatty) {
	    my $message=" " x length($$last_progress);
	    print STDERR $message;
	    undef $$last_progress;
	}
    }
}

sub update_progress($) {
    my $self = shift;
    my $progress_enabled = \${$self->{PROGRESS_ENABLED}};
    my $progress = ${$self->{PROGRESS}};
    my $last_progress = \${$self->{LAST_PROGRESS}};

    if($$progress_enabled) {
        # If progress has more than $columns characters the xterm will
        # scroll to the next line and our ^H characters will fail to
        # erase it.
        my $columns=$ENV{COLUMNS} || 80;
        $progress = substr $progress,0,($columns-1);

	my $prefix = "";
	my $suffix = "";
	if($$last_progress) {
            $prefix = "" x length($$last_progress);

	    my $diff = length($$last_progress)-length($progress);
	    if($diff > 0) {
                $suffix = (" " x $diff) . ("" x $diff);
	    }
	}
	print STDERR $prefix, $progress, $suffix;
	$$last_progress = $progress;
    }
}

sub progress($$) {
    my $self = shift;
    my $progress = \${$self->{PROGRESS}};
    my $last_time = \${$self->{LAST_TIME}};

    my $new_progress = shift;
    if(defined($new_progress)) {
	if(!defined($$progress) || $new_progress ne $$progress) {
	    $$progress = $new_progress;

	    $self->update_progress;
	    $$last_time = 0;
	}
    } else {
	return $$progress;
    }
}

sub lazy_progress($$) {
    my $self = shift;
    my $progress = \${$self->{PROGRESS}};
    my $last_time = \${$self->{LAST_TIME}};

    $$progress = shift;

    my $time = time();
    if($time - $$last_time > 0) {
	$self->update_progress;
    	$$last_time = $time;
    }
}

sub prefix($$) {
    my $self = shift;
    my $prefix = \${$self->{PREFIX}};
    my $prefix_callback = \${$self->{PREFIX_CALLBACK}};

    my $new_prefix = shift;
    if(defined($new_prefix)) {
	if(!defined($$prefix) || $new_prefix ne $$prefix) {
	    $$prefix = $new_prefix;
	    $$prefix_callback = undef;
	}
    } else {
	return $$prefix;
    }
}

sub prefix_callback($) {
    my $self = shift;

    my $prefix = \${$self->{PREFIX}};
    my $prefix_callback = \${$self->{PREFIX_CALLBACK}};

    $$prefix = undef;
    $$prefix_callback = shift;
}

sub write($$) {
    my $self = shift;

    my $message = shift;

    my $prefix = \${$self->{PREFIX}};
    my $prefix_callback = \${$self->{PREFIX_CALLBACK}};

    $self->hide_progress if $stdout_isatty;
    if(defined($$prefix)) {
	print $$prefix . $message;
    } elsif(defined($$prefix_callback)) {
	print &{$$prefix_callback}() . $message;
    } else {
	print $message;
    }
    $self->show_progress if $stdout_isatty;
}

1;
