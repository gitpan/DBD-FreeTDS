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


print "1..10\n";


my $dbh = TestCommon->connect;

print "ok 1\n";

print  "Back from connect\n";


if (!defined($dbh))
{
    my $str = "Can't connect to DB.  " . $DBI::errstr;
    die($str);
}

my $sth;
my $QueryString;

$QueryString = "select 'hello' message";
print  "Calling prepare\n";
$sth = $dbh->prepare($QueryString);
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

if ($sth->{"NUM_OF_FIELDS"}==1) 
{       
        print "ok 4\n";
}

print "result set has " . $sth->{"NUM_OF_FIELDS"} . " columns\n";

$QueryString = "select 'hello' message, 'nothing' mu, 1 one";
print  "Calling prepare\n";
$sth = $dbh->prepare($QueryString);
print  "back from  prepare\n";
    
print "ok 5\n";
    
if (!defined($sth))
{
    my $str = $dbh->errstr;
    die("No sth " . $str);
}

print "ok 6\n";

print  "About to exectute()\n";
if (!($sth->execute()))
{
    my $Str = $dbh->errstr;
    die "Couldn't execute" . $Str;
    return undef;
}
print  "Back from execute()\n";
if ($sth->{"NUM_OF_FIELDS"}==3) 
{
    print "ok 7\n";
}

print "ok 8\n";

print "result set has " . $sth->{"NUM_OF_FIELDS"} . " columns\n";

print "ok 9\n";

print  "closing the database handles\n"; 

defined($sth) && $sth->finish;
defined($dbh) && $dbh->disconnect;

print "ok 10\n";

print  "Finished running foo.pl\n";

