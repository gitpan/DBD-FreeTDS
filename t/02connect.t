#!/usr/bin/perl -w
#
# This program will create named files from information stored 
# in a sql database.
# 
use strict 'vars';
use strict 'subs';

use Sys::Hostname;
use DBI;
use AutoLoader;
use POSIX;

use lib (".", "./t/.");
use TestCommon;

$|++;

print "1..2\n";

my $dbh = TestCommon->connect;


print "ok 1\n";

$dbh->disconnect;

print "ok 2\n";



