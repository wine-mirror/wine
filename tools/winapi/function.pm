package function;

use strict;

sub new {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    bless ($self, $class);

    return $self;
}

sub file {
    my $self = shift;
    my $file = \${$self->{FILE}};

    local $_ = shift;

    if(defined($_)) { $$file = $_; }
    
    return $$file;
}

sub debug_channels {
    my $self = shift;
    my $debug_channels = \${$self->{DEBUG_CHANNELS}};

    local $_ = shift;

    if(defined($_)) { $$debug_channels = $_; }
    
    return $$debug_channels;
}

sub documentation_line {
    my $self = shift;
    my $documentation_line = \${$self->{DOCUMENTATION_LINE}};

    local $_ = shift;

    if(defined($_)) { $$documentation_line = $_; }
    
    return $$documentation_line;
}

sub documentation {
    my $self = shift;
    my $documentation = \${$self->{DOCUMENTATION}};

    local $_ = shift;

    if(defined($_)) { $$documentation = $_; }
    
    return $$documentation;
}

sub function_line {
    my $self = shift;
    my $function_line = \${$self->{FUNCTION_LINE}};

    local $_ = shift;

    if(defined($_)) { $$function_line = $_; }
    
    return $$function_line;
}

sub linkage {
    my $self = shift;
    my $linkage = \${$self->{LINKAGE}};

    local $_ = shift;

    if(defined($_)) { $$linkage = $_; }
    
    return $$linkage;
}

sub return_type {
    my $self = shift;
    my $return_type = \${$self->{RETURN_TYPE}};

    local $_ = shift;

    if(defined($_)) { $$return_type = $_; }
    
    return $$return_type;
}

sub calling_convention {
    my $self = shift;
    my $calling_convention = \${$self->{CALLING_CONVENTION}};

    local $_ = shift;

    if(defined($_)) { $$calling_convention = $_; }
    
    return $$calling_convention;
}

sub internal_name {
    my $self = shift;
    my $internal_name = \${$self->{INTERNAL_NAME}};

    local $_ = shift;

    if(defined($_)) { $$internal_name = $_; }
    
    return $$internal_name;
}

sub argument_types {
    my $self = shift;
    my $argument_types = \${$self->{ARGUMENT_TYPES}};

    local $_ = shift;

    if(defined($_)) { $$argument_types = $_; }
    
    return $$argument_types;
}

sub argument_names {
    my $self = shift;
    my $argument_names = \${$self->{ARGUMENT_NAMES}};

    local $_ = shift;

    if(defined($_)) { $$argument_names = $_; }
    
    return $$argument_names;
}

sub argument_documentations {
    my $self = shift;
    my $argument_documentations = \${$self->{ARGUMENT_DOCUMENTATIONS}};

    local $_ = shift;

    if(defined($_)) { $$argument_documentations = $_; }
    
    return $$argument_documentations;
}

sub statements_line {
    my $self = shift;
    my $statements_line = \${$self->{STATEMENTS_LINE}};

    local $_ = shift;

    if(defined($_)) { $$statements_line = $_; }
    
    return $$statements_line;
}

sub statements {
    my $self = shift;
    my $statements = \${$self->{STATEMENTS}};

    local $_ = shift;

    if(defined($_)) { $$statements = $_; }
    
    return $$statements;
}

1;
