#!/usr/bin/perl

use POSIX;
use File::Find;

my $rootdir = '';
$output = '';
%hash = ();

#Language Files
create_langfile('../../Miranda-IM', '../../Miranda-IM/docs/translations/langpack_english.txt', 'English (UK)', '0809', 'Miranda IM Development Team', 'info@miranda-im.org');

sub create_langfile {
    $rootdir = shift(@_);
    $outfile = shift(@_);
    $lang = shift(@_);
    $locale = shift(@_);
    $author = shift(@_);
    $email = shift(@_);
    %hash = ();
    $output = "";
    find({ wanted => \&csearch, preprocess => \&pre_dir }, $rootdir);
    find({ wanted => \&rcsearch, preprocess => \&pre_dir }, $rootdir);
    open(WRITE, "> $outfile") or die;
    print WRITE "Miranda Language Pack Version 1\n";
    print WRITE "Locale: $locale\n";
    print WRITE "Authors: $author\n";
    print WRITE "Author-email: $email\n";
    print WRITE "Last-Modified-Using: Miranda IM 0.3.3\n";
    print WRITE "Plugins-included:\n\n";
    print WRITE $output;
    close(WRITE);
}

sub pre_dir {
  @files = grep { not /^\.\.?$/ } @_;
  return sort @files;
}

sub append_str {
    $str = shift(@_);
    $found = shift(@_);
    $str = substr($str, 1, length($str) - 2);
    if (length($str) gt 0) {
        if (!$hash{$str}) {
            if ($found eq 0) {
                $output .= "; ".$file."\n";
            }
            $output .= ";[".$str."]\n";
            $hash{$str} = 1;
            return 1;
        }
    }
    return 0;
}

sub csearch {
    if ( -f $_ and $_ =~ m/\.c\Z/) {
        $found = 0;
        $file = $_;
        print "Processing $_... ";
	    open(READ, "< $_") or return;
        $all = "";
	    while ($lines = <READ>) {
            $all = $all.$lines;
        }
	    close(READ);
        $_ = $all;
        while (/Translate\s*\(\s*(\".*?\")\s*\)/g) {
            $found += append_str($1, $found);
        }
        if ($found gt 0) {
            $output .= "\n";
        }
        print "found $found strings\n";
    }
}

sub rcsearch {
    if ( -f $_ and $_ =~ m/\.rc\Z/) {
        $found = 0;
        $file = $_;
        print "Processing $_... ";
	    open(READ, "< $_") or return;
        $all = "";
	    while ($lines = <READ>) {
            $all = $all.$lines;
        }
	    close(READ);
        $_ = $all;
        while (/\s*CONTROL\s*(\".*?\")/g) {
            $found += append_str($1, $found);
        }
        while (/\s*DEFPUSHBUTTON\s*(\".*?\")/g) {
            $found += append_str($1, $found);
        }
        while (/\s*PUSHBUTTON\s*(\".*?\")/g) {
            $found += append_str($1, $found);
        }
        while (/\s*LTEXT\s*(\".*?\")/g) {
            $found += append_str($1, $found);
        }
        while (/\s*RTEXT\s*(\".*?\")/g) {
            $found += append_str($1, $found);
        }
        while (/\s*GROUPBOX\s*(\".*?\")/g) {
            $found += append_str($1, $found);
        }
        while (/\s*CAPTION\s*(\".*?\")/g) {
            $found += append_str($1, $found);
        }
        if ($found gt 0) {
            $output .= "\n";
        }
        print "found $found strings\n";
    }
}