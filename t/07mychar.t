#!/usr/bin/perl -w
#
# This file was automatically generated from create_07.template.
# Any modifications you make to this file will be lost.
#
# This program checks to see if the DBI driver will handles 
# columns defined as  "char(10) not null".  It really only tests 
# that the datatype is implemented and should not be considered 
# a stressful test of the driver.
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


print "1..14\n";


my $dbh = TestCommon->connect;

print "ok 1\n";

my $table = "t007mychar";

my $sql = "drop table $table";
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
    print "ok 3\n";
}
else
{
    print STDERR "Error message is  $dbh->errstr\n";
    print STDERR $dbh->errstr . "\n";
    if ($dbh->errstr =~ /Cannot drop the tabl/)
    {
        print "ok 3\n";
    }
    else
    {
        print "not ok 3\n";
    }
}

$sql = 
    "create table $table ( " .
    "  mychar  char(10) not null)             ";

if ($sth = $dbh->prepare($sql))
{
    print "ok 4\n";
}
else
{
    print "not ok 4\n";
}
    
if ($sth->execute)
{
    print "ok 5\n";
}
else
{
    print STDERR "Error message is  $dbh->errstr\n";
    print STDERR $dbh->errstr . "\n";
}

$sql = "insert into $table values ('hello') ";

if ($sth = $dbh->prepare($sql))
{
    print "ok 6\n";
}
else
{
    print "not ok 6\n";
}

if ($sth->execute)
{
    print "ok 7\n";
}
else
{
    print STDERR "Error message is  $dbh->errstr\n";
    print STDERR $dbh->errstr . "\n";
}


$sql = "insert into $table values ('goodbye') ";

if ($sth = $dbh->prepare($sql))
{
    print "ok 8\n";
}
else
{
    print "not ok 8\n";
}

if ($sth->execute)
{
    print "ok 9\n";
}
else
{
    print STDERR "Error message is  $dbh->errstr\n";
    print STDERR $dbh->errstr . "\n";
}

$sql = "select * from $table";
if ($sth = $dbh->prepare($sql))
{
    print "ok 10\n";
}
else
{
    print "not ok 10\n";
}

if ($sth->execute)
{
    print "ok 11\n";
}
else
{
    print STDERR "Error message is  $dbh->errstr\n";
    print STDERR $dbh->errstr . "\n";
}

my $rs;

my $got_v1    = 0;
my $got_v2    = 0;
my $found_key = 0;
my $value;
while (defined ($rs = $sth->fetchrow_hashref))
{
    my %r = %{$rs};
    my $k;

    foreach $k (keys %r)
    {
        if ($k ne "mychar")
        {
            die "Unknown column name $k";
        }
        else
        {
            if (! $found_key)
            {
                print "ok 12\n";
            }
            $found_key = 1;
        }
    }


    if (! defined($r{"mychar"}))
    {
        undef $value;
    }
    else
    {
        $value = $r{"mychar"} . " ";
        $value =~ s/ *$//;
    }

    if (defined $value)
    {
        print "The value is |" . $value . "|\n";
    }
    else
    {
        print "The value is null\n";
    }

    if (defined $value && $value eq 'hello')
    {
        if ($got_v1)
        {
            die "read 'hello' twice" ;
        }
        $got_v1 = 1;
    }
    elsif (defined $value && $value eq 'goodbye')
    {
        if ($got_v2)
        {
            die "read 'goodbye' twice" ;
        }
        $got_v2 = 1;
    }
    else
    {
        print STDERR "Read unexpected value " . $value . "\n";
    }
}
if ($got_v1 && $got_v2)
{
    print "ok 13\n";
}

$dbh->disconnect;

print "ok 14\n";

