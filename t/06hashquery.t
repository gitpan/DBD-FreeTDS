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


print "1..7\n";


my $dbh = TestCommon->connect;

print "ok 1\n";

my $sql;
my $sth;


$sql = "select 'Dear Sirs' as greeting, 'Sincerely' as closing";
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
    print "ok 3\n";
}
else
{
    print STDERR "Error message is  $dbh->errstr\n";
    print STDERR $dbh->errstr . "\n";
}

my $k;
my $rs;
my $i;
while (defined ($rs = $sth->fetchrow_hashref))
{
    my %r = %{$rs};
    $i = 0;
    foreach $k (keys %r)
    {
        if ($i==0 
            && $k eq "greeting" 
            && $r{$k} eq "Dear Sirs")
        {
            print "ok 4\n";
        }
        elsif ($i==1 
            && $k eq "closing" 
            && $r{$k} eq "Sincerely")
        {
            print "ok 5\n";
        }
        else
        {
            print "fail";
            exit (1);
        }

        print "$k -> " . $r{$k} . "\n";
        $i++;
    }
}
print "ok 6\n";


$dbh->disconnect;

print "ok 7\n";



