package output;

use strict;

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw();
@EXPORT_OK = qw($output);

use vars qw($output);

$output = '_output'->new;

package _output;

use strict;

my $stdout_isatty = -t STDOUT;
my $stderr_isatty = -t STDERR;

sub new {
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

sub enable_progress {
    my $self = shift;
    my $progress_enabled = \${$self->{PROGRESS_ENABLED}};

    $$progress_enabled = 1;
}

sub disable_progress {
    my $self = shift;
    my $progress_enabled = \${$self->{PROGRESS_ENABLED}};

    $$progress_enabled = 0;
}

sub show_progress {
    my $self = shift;
    my $progress_enabled = \${$self->{PROGRESS_ENABLED}};
    my $progress = \${$self->{PROGRESS}};
    my $last_progress = \${$self->{LAST_PROGRESS}};
    my $progress_count = \${$self->{PROGRESS_COUNT}};

    $$progress_count++;

    if($$progress_enabled) {
	if($$progress_count > 0 && $$progress && $stderr_isatty) {
	    print STDERR $$progress;
	    $$last_progress = $$progress;
	}
    }
}

sub hide_progress  {
    my $self = shift;
    my $progress_enabled = \${$self->{PROGRESS_ENABLED}};
    my $progress = \${$self->{PROGRESS}};
    my $last_progress = \${$self->{LAST_PROGRESS}};
    my $progress_count = \${$self->{PROGRESS_COUNT}};

    $$progress_count--;

    if($$progress_enabled) {
	if($$last_progress && $stderr_isatty) {
	    my $message;
	    for (1..length($$last_progress)) {
		$message .= " ";
	    }
	    print STDERR $message;
	    undef $$last_progress;
	}
    }
}

sub update_progress {
    my $self = shift;
    my $progress_enabled = \${$self->{PROGRESS_ENABLED}};
    my $progress = \${$self->{PROGRESS}};
    my $last_progress = \${$self->{LAST_PROGRESS}};
    
    if($$progress_enabled) {
	my $prefix = "";
	my $suffix = "";
	if($$last_progress) {
	    for (1..length($$last_progress)) {
		$prefix .= "";
	    }
	    
	    my $diff = length($$last_progress)-length($$progress);
	    if($diff > 0) {
		for (1..$diff) {
		    $suffix .= " ";
		}
		for (1..$diff) {
		    $suffix .= "";
		}
	    }
	}
	print STDERR $prefix . $$progress . $suffix;
	$$last_progress = $$progress;
    }
}

sub progress {
    my $self = shift;
    my $progress = \${$self->{PROGRESS}};
    my $last_time = \${$self->{LAST_TIME}};

    $$progress = shift;

    $self->update_progress;
    $$last_time = 0;
}

sub lazy_progress {
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

sub prefix {
    my $self = shift;
    my $prefix = \${$self->{PREFIX}};
    my $prefix_callback = \${$self->{PREFIX_CALLBACK}};

    my $new_prefix = shift;
    if(defined($new_prefix)) {
	$$prefix = $new_prefix;
	$$prefix_callback = undef;
    } else {
	return $$prefix;
    }
}

sub prefix_callback {
    my $self = shift;

    my $prefix = \${$self->{PREFIX}};
    my $prefix_callback = \${$self->{PREFIX_CALLBACK}};

    $$prefix = undef;
    $$prefix_callback = shift;
}

sub write {
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
