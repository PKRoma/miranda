#!/usr/bin/perl

use POSIX;

if(@ARGV < 2)
{
	print "trans <to translate> <old translation> [<next old translation> ...]\n";
	exit;
}

my $key;
my %trans;
my $totrans = shift @ARGV;
while(@ARGV)
{
	my $oldtrans = shift @ARGV;
	open OT, "<$oldtrans" or die "Error: Cannot open translation";
	while(<OT>)
	{
		chop;
		if(m/^\[(.*)\]/)
		{ $key = $1; next; }
		elsif($key and !$trans{$key} and !m/^;/)
		{ $trans{$key} = $_; $key = 0; }
	}
}

open TT, "<$totrans" or die "Error: Cannot open file to translate";
while(<TT>)
{
	if(m/^;\[(.*)\]/ and $trans{$1})
	{
		print "[$1]\n";
		print $trans{$1};
		print "\n";
	}
	else
	{ print; }
}

close TT;
close OT;
