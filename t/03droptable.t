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
use Data::Dumper;

use lib (".", "./t/.");
use TestCommon;

$|++;



print "1..4\n";


my $dbh = TestCommon->connect;

print "ok 1\n";

my $sql = "drop table tdstest001";
my $sth;

if ($sth = $dbh->prepare($sql))
{
    print "ok 2\n";
}
else
{
    print "not ok 2\n";
}

if ($sth->execute)
{
    print STDERR "Executed\n";
    print "ok 3\n";
}
else
{
    if ($dbh->errstr =~ /Cannot drop the tabl/)
    {
        print "ok 3\n";
    }
    else
    {
        print STDERR "Error message is  $dbh->errstr\n";
        print STDERR $dbh->errstr . "\n";
        print "not ok 3\n";
    }
}

print Dumper($dbh);

$dbh->disconnect;

print "ok 4\n";



