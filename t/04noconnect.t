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

$|++;

my $RDBMS    = "FreeTDS";
my $DBHost   = "mail.cdsnet.net"; # let's hope mail never has an SQL Server
my $DBName   = "jdbctest";
my $Username = "testuser";
my $Password = "password";


print "1..1\n";


my $dbh = DBI->connect("DBI:$RDBMS:$DBName:$DBHost", 
                       $Username, $Password, 
                       {PrintError => 0});

if (! defined($dbh))
{
    print "Error is: " . $DBI::errstr . "\n";
    print "ok 1\n";
}

