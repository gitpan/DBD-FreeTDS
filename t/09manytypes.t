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


print "1..11\n";


my $dbh = TestCommon->connect;

print "ok 1\n";

my $sql = "drop table tdstest007";
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
    "create table tdstest007 (                             " .
#      " mybinary                   binary(5) not null,       " .
#      " myvarbinary                varbinary(4) not null,    " .
    " mychar                     char(10) not null,        " .
    " myvarchar                  varchar(8) not null,      " .
    " mydatetime                 datetime not null,        " .
    " mysmalldatetime            smalldatetime not null,   " .
    " mydecimal10_3              decimal(10,3) not null,   " .
    " mynumeric5_4               numeric (5,4) not null,   " .
    " myfloat6                   float(6) not null,        " .
    " myfloat14                  float(6) not null,        " .
    " myreal                     real not null,            " .
    " myint                      int not null,             " .
    " mysmallint                 smallint not null,        " .
    " mytinyint                  tinyint not null,         " .
    " mymoney                    money not null,           " .
    " mysmallmoney               smallmoney not null,      " .
    " mybit                      bit not null,             " .
#    " mytimestamp                timestamp not null,       " .
#    " mytext                     text not null,            " .
#      " myimage                    image not null,           " .
#      " mynullbinary               binary(3) null,           " .
#      " mynullvarbinary            varbinary(6) null,        " .
    " mynullchar                 char(10) null,            " .
    " mynullvarchar              varchar(40) null,         " .
    " mynulldatetime             datetime null,            " .
    " mynullsmalldatetime        smalldatetime null,       " .
    " mynulldecimal10_3          decimal(10,3) null,       " .
    " mynullnumeric15_10         numeric(15,10) null,      " .
    " mynullfloat6               float(6) null,            " .
    " mynullfloat14              float(14) null,           " .
    " mynullreal                 real null,                " .
    " mynullint                  int null,                 " .
    " mynullsmallint             smallint null,            " .
    " mynulltinyint              tinyint null,             " .
    " mynullmoney                money null,               " .
    " mynullsmallmoney           smallmoney null          " .
#      " mynulltext                 text null,                " . 
#      " mynullimage                image null               " .
    " )";

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
    print STDERR "Executed\n";
    print "ok 5\n";
}
else
{
    print STDERR "Error message is  $dbh->errstr\n";
    print STDERR $dbh->errstr . "\n";
}

$sql =
    "insert into tdstest007       (  " .
#      "    mybinary,                      " . 
#      "    myvarbinary,                   " . 
    "    mychar,                        " . 
    "    myvarchar,                     " . 
    "    mydatetime,                    " . 
    "    mysmalldatetime,               " . 
    "    mydecimal10_3,                 " . 
    "    mynumeric5_4,                  " . 
    "    myfloat6,                      " . 
    "    myfloat14,                     " . 
    "    myreal,                        " . 
    "    myint,                         " . 
    "    mysmallint,                    " . 
    "    mytinyint,                     " . 
    "    mymoney,                       " . 
    "    mysmallmoney,                  " . 
    "    mybit,                         " . 
#    "    mytimestamp,                   " . 
#    "    mytext,                        " . 
#    "    myimage,                       " . 
#    "    mynullbinary,                  " . 
#    "    mynullvarbinary,               " . 
    "    mynullchar,                    " . 
    "    mynullvarchar,                 " . 
    "    mynulldatetime,                " . 
    "    mynullsmalldatetime,           " . 
    "    mynulldecimal10_3,             " . 
    "    mynullnumeric15_10,            " . 
    "    mynullfloat6,                  " . 
    "    mynullfloat14,                 " . 
    "    mynullreal,                    " . 
    "    mynullint,                     " . 
    "    mynullsmallint,                " . 
    "    mynulltinyint,                 " . 
    "    mynullmoney,                   " . 
    "    mynullsmallmoney               " . 
#    "    mynulltext,                    " . 
#    "    mynullimage                   " . 
    "    )                              " . 
    "values (                           " .
#      "    0x1213141516,                  " . # mybinary,
#      "    0x1718191A,                    " . # myvarbinary
    "    '1234567890',                  " . # mychar
    "    '12345678',                    " . # myvarchar
    "    '19991015 21:29:59.01',        " . # mydatetime
    "    '19991015 20:45',              " . # mysmalldatetime
    "    1234567.089,                   " . # mydecimal10_3
    "    1.2345,                        " . # mynumeric5_4
    "    65.4321,                       " . # myfloat6
    "    1.123456789,                   " . # myfloat14
    "    987654321.0,                   " . # myreal
    "    4097,                          " . # myint
    "    4094,                          " . # mysmallint
    "    200,                           " . # mytinyint
    "    19.95,                         " . # mymoney
    "    9.97,                          " . # mysmallmoney
    "    1,                             " . # mybit
#    "    null,                          " . # mytimestamp
#      "    'abcdefg',                     " . # mytext
#      "    0x0AAABB,                      " . # myimage
#      "    0x123456,                      " . # mynullbinary
#      "    0xAB,                          " . # mynullvarbinary
    "    'z',                           " . # mynullchar
    "    'zyx',                         " . # mynullvarchar
    "    '1976-07-04 12:00:00.04',      " . # mynulldatetime
    "    '2000-02-29 13:46',            " . # mynullsmalldatetime
    "     1.23,                         " . # mynulldecimal10_3
    "     7.1234567891,                 " . # mynullnumeric15_10
    "     987654,                       " . # mynullfloat6
    "     0,                            " . # mynullfloat14
    "     -1.1,                         " . # mynullreal
    "     -10,                          " . # mynullint
    "     126,                          " . # mynullsmallint
    "     7,                            " . # mynulltinyint
    "     -19999.00,                    " . # mynullmoney
    "     -9.97                         " . # mynullsmallmoney
#      "     '1234',                       " . # mynulltext
#      "     0x1200340056                  " . # mynullimage)
    "    )                              " . 
    "";

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
    print STDERR "Executed\n";
    print "ok 7\n";
}
else
{
    print STDERR "Error message is  $dbh->errstr\n";
    print STDERR $dbh->errstr . "\n";
}

$sql = "select * from tdstest007";
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
    print STDERR "Executed\n";
    print "ok 9\n";
}
else
{
    print STDERR "Error message is  $dbh->errstr\n";
    print STDERR $dbh->errstr . "\n";
}

my $rs;

while (defined ($rs = $sth->fetchrow_hashref))
{
    print "Got a row\n";
}
print "ok 10";

$dbh->disconnect;

print "ok 11\n";



