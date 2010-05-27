#!/usr/bin/perl -w

# The parser assumes the format produced by César's unfolder!

use Digest::MD5 qw(md5_base64);

@work = ();
%mtab = ();
read_unfolding($ARGV[0]);
find_initial_marking();
while ($#work >= 0) {
	$mark = shift @work;
	process($mark);
}

exit 0;

sub find_initial_marking {
	my @co = ();
	for my $c (@conds) {
		push @co,($c) unless $indeg{$c};
	}
	my $cmark = join("#",@co);
	my $pmark = join("#",sort(map {$label{$_}} @co));
	push @work,($cmark);
	$mtab{$pmark} = "";
}

sub process {
	my ($ocmark) = @_;
	my @oco = split(/#/,$ocmark);
	my %i = %indeg;
	my @fire = ();
	my %marked = ();

	print join(" ",sort(map {$label{$_}} @oco))."\n";

	for my $c (@oco) {
		$marked{$c} = "";
		next unless defined($out{$c});
		for my $e (split(/#/,$out{$c})) {
			push @fire,($e) unless --$i{$e};
		}
	}

	for my $t (@fire) {
		my %nmarked = %marked;
		for my $c (split(/#/,$in{$t})) {
			delete $nmarked{$c};
		}
		for my $c (split(/#/,$out{$t})) {
			$nmarked{$c} = "";
		}
		my @nco = keys %nmarked;
		my $pmark = join("#",sort(map {$label{$_}} @nco));
		next if defined($mtab{$pmark});
		$mtab{$pmark} = "";
		my $cmark = join("#",@nco);
		push @work,($cmark);
	}
}

sub error {
	my ($msg) = @_;
	print STDERR "$msg\n";
	exit 1;
}

sub read_unfolding {
	my ($file) = @_;
	%hco = ();
	%in = ();
	%out = ();
	%indeg = ();
	open FD,$file;
	while (<FD>) {
		next if /\be0\b/;
		if (/(c\d*).*->.*?(e\d*)/) {
			my ($co,$ev) = ($1,$2);
			$hco{$co} = "";
			$indeg{$ev}++;
			$out{$co} .= "#$ev";
			$in{$ev} .= "#$co" unless /arrowhead=none/;
		}
		if (/(e\d*).*->.*(c\d*)/) {
			my ($ev,$co) = ($1,$2);
			$hco{$co} = "";
			$indeg{$co}++;
			$out{$ev} .= "#$co";
			$in{$co} .= "#$ev";
		}
		if (/(c\d*).*label=".*?:(.*?)"/) {
			$hco{$1} = "";
			$label{$1} = $2;
		}
		if (/(e\d*).*label=".*?:(.*?)"/) {
			$label{$1} = $2;
		}
	}
	close FD;
	@conds = keys %hco;
	for (keys %out) { $out{$_} = substr($out{$_},1); }
	for (keys %in) { $in{$_} = substr($in{$_},1); }
	for (keys %cont) { $cont{$_} = substr($cont{$_},1); }
}
