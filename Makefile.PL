# -*-Perl-*-

#
# Copyright 1998 CDS Networks, Inc., Medford Oregon
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#      This product includes software developed by CDS Networks, Inc.
# 4. The name of CDS Networks, Inc.  may not be used to endorse or promote
#    products derived from this software without specific prior
#    written permission.
#
# THIS SOFTWARE IS PROVIDED BY CDS NETWORKS, INC. ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL CDS NETWORKS, INC. BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

use ExtUtils::MakeMaker;

use strict;

require 5.004;

use DBI;
use DBI::DBD;
use Config;

use vars qw($LINKTYPE $lib_string);
my $dbidir = "$Config{sitearchexp}/auto/DBI";
die unless -e "$dbidir/DBIXS.h";
my $include_string = "-I$dbidir -I/usr/local/include";
my $libdirs = '-L/usr/local/lib -llibdibs';
$lib_string="";

#
# Perl modules are normally built for dynamic linkage, but this makes it much 
# harder to debug.  Change LINKTYPE to static to build a perl that is easier
# to debug.  
# 

$LINKTYPE='dynamic';
# $LINKTYPE='static';

my $lddlflags = $Config{lddlflags};
my $ldflags = $Config{ldflags};
my $cflags = 
    '-Wall '                     . 
#    '-W '                        .
#    '-Wshadow '                  . 
    '-Wmissing-prototypes '      .    
    '-ggdb '                     .
    '-static '                   .
#    '-DTRACE_ALLOCMEMORY_LEAKS ' .
    '';


# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.


WriteMakefile(
    'CC'           => 'gcc',
    'NAME'	   => 'DBD::FreeTDS',
    'VERSION_FROM' => 'FreeTDS.pm', # finds $VERSION
    LIBS	   => [$lib_string],   # e.g., '-lm'
    INC	           => "$include_string", # e.g., '-I/usr/other'
    'OPTIMIZE'     => ' -ggdb ',
    'CCFLAGS'      => $cflags,
    'LDFLAGS'      => $ldflags,
    'LDDLFLAGS'    => $lddlflags,
    'LINKTYPE'     => $LINKTYPE,         # static or dynamic (?)
    'CCCDLFLAGS'   => ' ',
    OBJECT         => '$(O_FILES)'
#    'PM'        => 'FreeTDS.pm'
);



sub MY::postamble {
    return dbd_postamble();
}
