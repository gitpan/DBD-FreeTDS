#!/usr/bin/perl -w
#
# This program checks to see if the DBI driver will handles 
# columns defined as  "timestamp not null".  It really only tests 
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

my $table = "t007mytimestamp";

my $sql = "drop table $table";

my $sth;

{
    
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
    print "About to call finish\n";
    $sth->finish;
    $sth = undef;
    print "Back from finish\n";
}

$sql = 
    "create table $table ( " .
    "  mytimestamp  timestamp not null)             ";

{
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
}


$sql = "insert into $table values (null) ";

{
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
    $sth->finish;
}

$sql = "insert into $table values (null) ";

{
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
    $sth->finish;
}

$sql = "select mytimestamp, convert(int, mytimestamp) converted from $table";
{
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
    my $found_converted   = 0;
    my $found_mytimestamp = 0;
    my $value;
    while (defined ($rs = $sth->fetchrow_hashref))
    {
        my %r = %{$rs};
        my $k;
        
        foreach $k (keys %r)
        {
            if ($k eq "mytimestamp")
            {
                if (! $found_mytimestamp)
                {
                    
                }
                $found_mytimestamp = 1;
            }
            if ($k eq "converted")
            {
                if (! $found_converted)
                {
                    
                }
                $found_converted = 1;
            }
        }
        
        
        if (! defined($r{"mytimestamp"}))
        {
            undef $value;
        }
        else
        {
            $value = $r{"mytimestamp"} . " ";
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
        
    }
    if ($found_converted)
    {
        print "ok 12\n";
    }
    if ($found_mytimestamp)
    {
        print "ok 13\n";
    }
    
    $sth->finish;
}
$dbh->disconnect;

print "ok 14\n";

