#!/usr/bin/perl -w

use Digest::MD5 qw(md5_hex);

if (@ARGV != 2) {
	print "unfiso.pl DOTFILE1 DOTFILE2\n";
	exit;
}
($file1,$file2) = ($ARGV[0],$ARGV[1]);

read_unfolding($file1,"c","e");
@work = ();
label_all($file1);
@c1 = sort {$uq{$a} cmp $uq{$b}} @conds;
@e1 = sort {$uq{$a} cmp $uq{$b}} @evs;

read_unfolding($file2,"d","f");
@work = ();
label_all($file2);
@c2 = sort {$uq{$a} cmp $uq{$b}} @conds;
@e2 = sort {$uq{$a} cmp $uq{$b}} @evs;

compare_labels("condition",\@c1,\@c2);
compare_labels("event",\@e1,\@e2);

print STDERR "Nets are isomorphic!\n";

exit 0;

sub compare_labels {
	my ($type,$t1,$t2) = @_;
	my @i1 = @$t1;
	my @i2 = @$t2;

	while ($#i1 >= 0 && $#i2 >= 0) {
		my $n1 = shift @i1;
		my $n2 = shift @i2;
		if ($uq{$n1} lt $uq{$n2}) {
			error("$type ".substr($n1,1)." in $file1".
				" has no correspondence in $file2");
		} elsif ($uq{$n1} gt $uq{$n2}) {
			error("$type ".substr($n2,1)." in $file2".
				" has no correspondence in $file1");
		}
	}
	if ($#i1 >= 0) {
		my $n1 = shift @i1;
		error("$type ".substr($n1,1)." in $file1".
			" has no correspondence in $file2");
	}
	if ($#i2 >= 0) {
		my $n2 = shift @i2;
		error("$type ".substr($n2,1)." in $file2".
			" has no correspondence in $file1");
	}
}

sub label_all {
	my ($file) = @_;
	for my $c (@conds) {
		next if $indeg{$c};
		label_condition($file,$c,"");
	}
	while ($#work >= 0) {
		my $e = shift @work;
		label_event($file,$e);
	}
	for my $c (@conds) {
		error("condition ".substr($c,1)." in $file unreachable")
			unless $uq{$c};
	}
	for my $e (@evs) {
		error("event ".substr($e,1)." in $file unreachable")
			unless $uq{$e};
	}
}

sub label_condition {
	my ($file,$c,$ev) = @_;
	$uq{$c} = "[$ev]".$label{$c};
	#print "labelling ".substr($c,1)." in $file with ".$uq{$c}."\n";
	return unless $out{$c};
	for my $e (split(/#/,$out{$c})) {
		push @work,$e unless --$indeg{$e};
	}
}

sub label_event {
	my ($file,$e) = @_;
	my @preset = sort {$label{$a} cmp $label{$b}} split(/#/,$in{$e});
	my @preuq = map {$uq{$_}} @preset;
	$uq{$e} = "(".join(",",@preuq).")".$label{$e};
	#print "labelling ".substr($e,1)." in $file with ".$uq{$e}."\n";
	my @postset = split(/#/,$out{$e});
	my $elab = md5_hex($uq{$e});
	for my $c (@postset) {
		label_condition($file,$c,$elab);
	}
}

sub error {
	my ($msg) = @_;
	print STDERR "$msg\n";
	exit 1;
}

sub read_unfolding {
	my ($file,$co,$ev) = @_;
	%hco = ();
	%hev = ();
	%in = ();
	%out = ();
	%indeg = ();
	open FD,$file;
	while (<FD>) {
		if (/arrowhead=none/) {
			print STDERR "Found read arcs in '$file'; cannot " .
					"handle this!\n";
			exit 1;
		}
		if (/^\s*([cp]\d*)\s*->\s*([et]\d*);/) {
			$hco{$co.$1} = "";
			$hev{$ev.$2} = "";
			$indeg{$ev.$2}++;
			$out{$co.$1} .= "#$ev$2";
			$in{$ev.$2} .= "#$co$1";
			#print "file $file edge p>t $1 > $2\n";
		}
		if (/^\s*([et]\d*)\s*->\s*([cp]\d*);/) {
			$hev{$ev.$1} = "";
			$hco{$co.$2} = "";
			$indeg{$co.$2}++;
			$out{$ev.$1} .= "#$co$2";
			$in{$co.$2} .= "#$ev$1";
			#print "file $file edge t>p $1 > $2\n";
		}
		if (/^\s*([cp]\d*)\s*.label="([^ :"]*)/) {
			$hco{$co.$1} = "";
			$label{$co.$1} = $2;
			#print "file $file p $1 label $2\n";
		}
		if (/^\s*([et]\d*)\s*.label="([^ :"]*)/) {
			$hev{$ev.$1} = "";
			$label{$ev.$1} = $2;
			#print "file $file t $1 label $2\n";
		}
	}
	close FD;
	@conds = keys %hco;
	@evs = keys %hev;
	for (keys %out) { $out{$_} = substr($out{$_},1); }
	for (keys %in) { $in{$_} = substr($in{$_},1); }
	$cnr = @conds;
	$enr = @evs;
	print STDERR "Net at '$file';  $enr transitions/events, $cnr " .
			"places/conditions\n";
}

