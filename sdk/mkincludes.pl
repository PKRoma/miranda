#!/usr/bin/perl

use POSIX;
use File::Copy;
use File::Find;
use Win32::AbsPath;

$rootdir = '../../MirandaIM/';
$outdir = Win32::AbsPath::Fix('../include/');
$now = localtime(time);

copyreqs();
find(\&filesearch, $rootdir);

sub copyreqs {
    print "Copying win2k.h...\n";
    copy("../../MirandaIM/win2k.h","$outdir\\win2k.h");
    print "Copying m_icq.h...\n";
    copy("../../Protocols/IcqOscar8/m_icq.h","$outdir\\m_icq.h");
    print "Copying m_fuse.h...\n";
    copy("../../Tools/fuse/m_fuse.h","$outdir\\m_fuse.h");
    print "Copying newpluginapi.h...\n";
    copy("../../MirandaIM/random/plugins/newpluginapi.h","$outdir\\newpluginapi.h");
    print "Copying statusmodes.h...\n";
    copy("../../MirandaIM/ui/contactlist/statusmodes.h","$outdir\\statusmodes.h");
}

sub filesearch {
    if ( -f $_ and $_ =~ m/\Am_/ and $_ =~ m/\.h\Z/) {
        print "Copying $_...\n";
        copy($_,"$outdir\\$_");
	    open(READ, $_) or return;
	    my @lines = <READ>;
	    close(READ);
        open(WRITE, ">$outdir\\$_") or return;

	    for my $line (@lines) {
            if ( $line =~ m/\#include/ and $line =~ m/m_options.h/ ) {
                print WRITE "\#include \"m_options.h\"\n";
            }
            else {
                print WRITE "$line";
            }
	    }
        close(WRITE);
    }
}
