use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    'MAKEFILE'  => 'Makefile.perl',
    'NAME'	=> 'wine',
    'VERSION_FROM' => 'wine.pm', # finds $VERSION
);
