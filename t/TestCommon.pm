{
    package TestCommon;
    
    use DBI;

    @EXPORT = qw(DRIVERNAME SERVER PORT USER PASSWORD);
    
    $DRIVERNAME = "FreeTDS";
    $SERVER     = "testms";
    $PORT       = 1433;
    $DATABASE   = "jdbctest";
    $USER       = "testuser";
    $PASSWORD   = "password";
    
    sub connect
    {
        return DBI->connect("DBI:$DRIVERNAME:database=$DATABASE;host=$SERVER;port=$PORT;", 
                            $USER, $PASSWORD);
    }
}

1;
