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

my $table = "t13orderby";

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
    "  a  varchar(10) not null)   ";

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


my @values = ('a', 'b', 'c');
my $value;
foreach $value ( @values )
{
    $sql = "insert into $table values ('$value') ";
    
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
}    


$sql = "select * from $table order by a desc";
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
                       
        if (! defined($r{"a"}))
        {
            undef $value;
        }
        else
        {
            $value = $r{"a"} . " ";
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
    $sth->finish;
}
$dbh->disconnect;

print "ok 14\n";

