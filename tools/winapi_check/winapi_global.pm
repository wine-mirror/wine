package winapi_global;

use strict;
 
sub check {
    my $options = shift;
    my $output = shift;
    my $winapi = shift;
    my $nativeapi = shift;

    my $winver = $winapi->name;

    if($options->argument) {
	foreach my $type ($winapi->all_declared_types) {
	    if(!$winapi->type_found($type) && !$winapi->is_limited_type($type) && $type ne "CONTEXT86 *") {
		$output->write("*.c: $winver: ");
		$output->write("type ($type) not used\n");
	    }
	}
    }

    if($options->declared) {
	foreach my $name ($winapi->all_functions) {
	    if(!$winapi->function_found($name) && !$nativeapi->is_function($name)) {
		my $module = $winapi->function_module($name);
		$output->write("*.c: $module: $name: ");
		$output->write("function declared but not implemented: " . $winapi->function_arguments($name) . "\n");
	    }
	}
    }

    if($options->argument_forbidden) {
	my $not_used = $winapi->types_not_used;

	foreach my $module (sort(keys(%$not_used))) {
	    foreach my $type (sort(keys(%{$$not_used{$module}}))) {
		$output->write("*.c: $module: type $type not used\n");
	    }
	}	
    }
}

1;

