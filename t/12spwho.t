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

print "1..9\n";


my $dbh = TestCommon->connect;

print "ok 1\n";

print  "Back from connect\n";


if (!defined($dbh))
{
    my $str = "Can't connect to DB.  " . $DBI::errstr;
    die($str);
}

my $QueryString = "sp_who";
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
my @a;
my $num_found = 0;
while(@a = $sth->fetchrow)
{
    $num_found++;
    foreach $s (@a)
    {
        print "$s  ";
    }
    print "\n";
}

print STDERR "number found is $num_found\n";

print "ok 5\n";

if (! $sth->{more_results})
{
    print "not ok 6\n";
}
else
{
    print "ok 6\n";
}


$num_found = 0;
while(@a = $sth->fetchrow)
{
    foreach $s (@a)
    {
        print "$s ";
    }
    print "\n";
}

if ($sth->{more_results})
{
    print "not ok 7\n";
    exit 1;
}
else
{
    print "ok 7\n";
}
print  "Done fetching rows\n";

if ($sth->finish)
{
    print "ok 8\n";
}
else
{
    print "not ok 8\n";
}

if ($dbh->disconnect)
{
    print "ok 9\n";
}
else
{
    print "not ok 9\n";
}


