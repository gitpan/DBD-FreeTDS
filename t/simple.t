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


print "1..8\n";


my $dbh = TestCommon->connect;

print "ok 1\n";

print  "Back from connect\n";


if (!defined($dbh))
{
    my $str = "Can't connect to DB.  " . $DBI::errstr;
    die($str);
}

my $QueryString = "select i, s from t0020 where i<10";
print  "Calling prepare\n";
my $sth = $dbh->prepare($QueryString);
print  "back from  prepare\n";
    
print "ok 2\n";
    
if (!defined($sth))
{
    my $str = $dbh->errstr;
    die("No sth " . $str);
}

print "ok 3\n";

print  "About to exectute()\n";
if (!($sth->execute()))
{
    my $Str = $dbh->errstr;
    die "Couldn't execute" . $Str;
    return undef;
}
print  "Back from execute()\n";

print "ok 4\n";

my $i;
my $s;
while(($i, $s) = $sth->fetchrow)
{
    print "Result $i  |$s|\n";
}
print  "Done fetching rows\n";

print "ok 5\n";


$QueryString = "select i, s from t0020 where i>20 and i<30";
print  "Calling prepare\n";
$sth = $dbh->prepare($QueryString);
print  "back from  prepare\n";
    
print "ok 6\n";

if (!defined($sth))
{
    my $str = $dbh->errstr;
    die("No sth " . $str);
}

print  "About to exectute()\n";
if (!($sth->execute()))
{
    my $Str = $dbh->errstr;
    die "Couldn't execute" . $Str;
    return undef;
}
print  "Back from execute()\n";

print "ok 7\n";

my %rs;

while(($i, $s) = $sth->fetchrow())
{
    print "Result " . $i . " |" . $s . "|\n";
}
print  "Done fetching rows\n";

print  "closing the database handles\n"; 

defined($sth) && $sth->finish;
defined($dbh) && $dbh->disconnect;

print "ok 8\n";

print  "Finished running foo.pl\n";

