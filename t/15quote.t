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


print "1..6\n";

my $dbh = TestCommon->connect;
my $str;
my $qstr;

print "ok 1\n";

print  "Back from connect\n";


if (!defined($dbh))
{
    my $str = "Can't connect to DB.  " . $DBI::errstr;
    die($str);
}
print "ok 2\n";

($dbh->quote("abc") eq q{'abc'}) or print 'not ';
print "ok 3";

$str  = "apostrophe'";
$qstr = "'apostrophe'''";
print "|" . $str . "| => |" . $dbh->quote($str) . "| == |" . $qstr . "|\n";
($dbh->quote($str) eq $qstr) or print "not ";
print "ok 4\n";

$str  = 'abc"def';
$qstr = "'abc\"def'";
print "|" . $str . "| => |" . $dbh->quote($str) . "| == |" . $qstr . "|\n";
($dbh->quote($str) eq $qstr) or print "not ";
print "ok 5\n";




$dbh->disconnect;
print "ok 6\n";
