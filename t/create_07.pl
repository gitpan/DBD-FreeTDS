#!/usr/bin/perl


my $template = `cat create_07.template`;


open in, "<create_07.data" or die;

while(<in>)
{
    chomp;
    split /!/;
    next if ($_[0] eq '#');

    open fh, ">07" . $_[0] . ".t" or die;

    $tmp = $template;

    $tmp =~ s/__NAME__/$_[0]/g;
    $tmp =~ s/__TYPE__/$_[1]/g;
    $tmp =~ s/__V1__/$_[2]/g;
    $tmp =~ s/__V2__/$_[3]/g;
    if ($_[2] eq 'null')
    {
        $tmp =~ s/__CMP_V1__/! defined (\$value)/g;
    }
    elsif ($_[2] =~ /^\'/) 
    {
        $tmp =~ s/__CMP_V1__/defined \$value && \$value eq $_[2]/g;
    }
    else 
    {
        $tmp =~ s/__CMP_V1__/defined \$value && \$value ==  $_[2]/g;
    }

    if ($_[3] eq 'null')
    {
        $tmp =~ s/__CMP_V2__/! defined (\$value)/g;
    }
    elsif ($_[3] =~ /^\'/) 
    {
        $tmp =~ s/__CMP_V2__/defined \$value && \$value eq $_[3]/g;
    }
    else 
    {
        $tmp =~ s/__CMP_V2__/defined \$value && \$value ==  $_[3]/g;
    }
    print fh $tmp or die;
    close fh;
}
