#!/usr/bin/perl -w
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


print "1..12\n";


my $dbh = TestCommon->connect;

print "ok 1\n";

print  "Back from connect\n";


if (!defined($dbh))
{
    my $str = "Can't connect to DB.  " . $DBI::errstr;
    die($str);
}


my $QueryString = "";
print  "Calling prepare\n";
my $rv = $dbh->do("create table t014 (mychar char(10) not null)");
print "ok 2\n";
if (!defined($rv))
{
    die "$DBH::errstr";
}
print "ok 3\n";


$rv = $dbh->do("delete from t014");
print "ok 4\n";
if (!defined($rv))
{
    die "$DBH::errstr";
}
print "ok 5\n";


$rv = $dbh->do("insert into t014 values ('abc')");
print "ok 6\n";
if (!defined($rv))
{
    die "$DBH::errstr";
}
print "ok 7\n";


my $sth;
$QueryString = "select * from t014";
print  "Calling prepare\n";
$sth = $dbh->prepare($QueryString);
print  "back from  prepare\n";
    
print "ok 8\n";

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

print "ok 9\n";

my %rs;

my ($i, $s);
while(($s) = $sth->fetchrow())
{
    $s =~ s/ *$//;
    if ($s eq 'abc')
    {
        print "ok 10\n";
    }
}
print  "Done fetching rows\n";

print  "closing the database handles\n"; 

defined($sth) && $sth->finish;
print "ok 11\n";
defined($dbh) && $dbh->disconnect;

print "ok 12\n";

print  "Finished running foo.pl\n";

