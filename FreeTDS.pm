# -*-Perl-*-

#
# Copyright 1998 CDS Networks, Inc., Medford Oregon
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#      This product includes software developed by CDS Networks, Inc.
# 4. The name of CDS Networks, Inc.  may not be used to endorse or promote
#    products derived from this software without specific prior
#    written permission.
#
# THIS SOFTWARE IS PROVIDED BY CDS NETWORKS, INC. ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL CDS NETWORKS, INC. BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#


{    
    package DBD::FreeTDS;

    use DBI();
    use DynaLoader();


    use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
    
    require Exporter;
    require DynaLoader;
    require AutoLoader;
    
    @ISA = qw(Exporter DynaLoader);
    # Items to export into callers namespace by default. Note: do not export
    # names by default without a very good reason. Use EXPORT_OK instead.
    # Do not simply export all your public functions/methods/constants.
    @EXPORT = qw(
                 
                 );
    $VERSION = '0.02';

    my $CVS_VERSION = '$Id: FreeTDS.pm,v 1.10 1999/02/17 16:56:07 cts Exp $ ';

    bootstrap DBD::FreeTDS $VERSION;

    $drh = undef;	# holds driver handle once initialised
    $err = 0;		# The $DBI::err value
    $errstr = '';
    $sqlstate = "00000";

    sub driver {
	return $drh if $drh;
	my($class, $attr) = @_;
	$class .= "::dr";
	($drh) = DBI::_new_drh($class, {
	    'Name' => 'FreeTDS',
	    'Version' => $VERSION,
	    'Err'     => \$DBD::FreeTDS::err,
	    'Errstr'  => \$DBD::FreeTDS::errstr,
	    'State'   => \$DBD::FreeTDS::sqlstate,
	    'Attribution' => 'FreeTDS DBD by Craig Spannring',
	    });
	$drh;
    }
    1;
}

{   package DBD::FreeTDS::dr; # ====== DRIVER ======
    use strict;
    
    sub connect { 
        my($drh, $datasource, $user, $auth, $attr)= @_;

        my $host   = $ENV{DSQUERY};
        my $port   = 1433;
        my $dbname;
        


        my $this = DBI::_new_dbh($drh, {
	    'Name'         => $datasource,
	    'User'         => $user,	
	    'CURRENT_USER' => $user,
        });

        $this->STORE("port", "" . $port);
        
        #
        # $datasource should be of the form-
        #     database=jdbctest;host=testms;port=1433;

        my $attribute_pair;
        foreach $attribute_pair (split(/;/, $datasource))
        {
            my ($key, $value) = split(/=/, $attribute_pair, 2);
            if (defined($value))
            {
                $this->STORE($key, $value);
                if ($key eq 'database')
                {
                    $dbname = $value;
                }
            }
            else
            {
                # XXX need to return an error here
            }
        }
        
        if (!defined($dbname))
        {
            return undef;
        }
 

      DBD::FreeTDS::db::_login($this, $dbname, $user, $auth) or return undef;
        
	$this;
    }
}



{   package DBD::FreeTDS::db; # ====== DATABASE ======
    use strict;
    
    sub prepare {
	my($dbh, $statement, @attribs)= @_;
        
	my $sth = DBI::_new_sth($dbh, {
	    'Statement' => $statement,
        });
        
        
        if (DBD::FreeTDS::st::_prepare($sth, $statement, @attribs))
        {
            return $sth;
        }
        else
        {
            print STDERR "Failed from inside of DBD::FreeTDS::db::prepare.  |$statement|\n";
            return undef;
        }
    }

    sub tables {
        die "Not Implemented";
    }

    sub do {
        my($dbh, $statement, $attr, @bind_values) = @_;
        my $sth;

        if (! defined($sth = $dbh->prepare($statement)))
        {
            print STDERR "Couldn't prepare the statement\n";
            return undef;
        }
        if (! $sth->execute(@bind_values))
        {
            print STDERR "Couldn't execute the statement\n";
            return undef;
        }   
        my $tmp1;
        my $tmp2;
        do
        {
            while($tmp1 = $sth->fetchrow_arrayref)
            {
                # XXX Should we warn the user if the statement 
                # returned results?
                # nop
            }
        } while ($sth->{more_results});
        my $rows = $sth->rows;
#        print STDERR "Calling finish from inside do.  Statement was |$statement|\n";
        $sth->finish;
#        print STDERR "Back from finish from inside do\n";
        ($rows == 0) ? "0E0" : $rows;
    }

    sub table_info 
    {
        die "Not Implemented";
    }

    sub ping 
    {
        die "Not Implemented";
    }
}


{   package DBD::FreeTDS::st; # ====== STATEMENT ======
    use strict;


}

1;

__END__


=head1 NAME

DBD::FreeTDS - FreeTDS SQLServer and Sybase database driver for the DBI module

=head1 SYNOPSIS

 use DBI;;
 $dsn = "DBI:FreeTDS:database=$DATABASE;host=$DBHOST;port=$PORT";
 $dbh = DBI->connect($dsn, $user, $password);

=head1 DESCRIPTION

This is still in the early stages of development.

=head2 Multiple result sets

Sybase and SQLServer have the ability to return multiple result sets
from a single SQL query.  Since DBI does not indicate what should be 
done with multiple result sets I have taken an approach similar to 
DBD::Sybase by Michael Peppler.  If a query might return multiple result
sets you whould fetch in the normal fashion.  When you hit the end of the 
result set you should consult the statement handle attribute 'more_results'.

For example-

   do {
     while (@rs = $sth->fetchrow) {
         process the data;
     }
   } while ($sth->{more_results});


=head2 Compute columns

Compute columns are not handle yet.


=head1 AUTHOR

Craig Spannring

=head1 SEE ALSO

DBI.
perl(1).

=head1

=head1 COPYRIGHT

 Copyright 1998, 1999 CDS Networks, Inc., Medford Oregon
 
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.
 3. All advertising materials mentioning features or use of this
    software must display the following acknowledgement:
      This product includes software developed by CDS Networks, Inc.

 4. The name of CDS Networks, Inc.  may not be used to endorse or
    promote products derived from this software without specific
    prior written permission.

 
 THIS SOFTWARE IS PROVIDED BY CDS NETWORKS, INC. ``AS IS'' AND ANY
 EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CDS NETWORKS, INC. BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.



=cut
