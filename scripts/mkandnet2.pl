#!/usr/bin/perl -w

# Asynchronous boolean circuit consisting in a square grid of NxN AND gates.
#
# The inputs of every gate are the outputs of the its north and west gates.
# The initial state of all signals is low except for the signals at the
# top and left boundaries of the grid, which are non-deterministally simulated
# to be high or low.  This circuit represents thus the entire state-space of a
# real circuit of the same topology.  Rising and falling edges (transitions)
# are simulated.
#
# The output is a contextual net.

$N = $ARGV[0]+1;

printf "PEP\nPetriBox\nFORMAT_N2\nPL\n";

$plc = 0;
for ($i = 0; $i < $N; $i++) {
	for ($j = 0; $j < $N; $j++) {
		$one = ($i == 0) || ($j == 0);
		$plc++;
		printf "%d\"off/%dx%d\"7\@7%s\n",$plc,$i,$j,$one? "" : "M1m1";
		$plc++;
		printf "%d\"on/%dx%d\"7\@7%s\n",$plc,$i,$j,$one? "M1m1" : "";
	}
}

printf "TR\n";
for ($i = 1; $i < $N; $i++) {
	for ($j = 1; $j < $N; $j++) {
		my $b = mp($i,$j);
		my $left = mp($i-1,$j);
		my $up = mp($i,$j-1);
		newtrans("raise", $b);
		ra($left,1); ra($up,1); one($b);
newtrans("fall", $b);
ra($left,0); ra($up,0); zero($b);
	}
}

for ($i = 0; $i < $N; $i++) {
	$trc++;
	printf "%d\"raise/0-%d\"8\@8\n", $trc, $i;
	$low = place ("0,$i", 0);
	$high = place ("0,$i", 1);
	push @consume, "$low>$trc";
	push @produce, "$trc<$high";

	$trc++;
	printf "%d\"fall/0-%d\"8\@8\n", $trc, $i;
	push @consume, "$high>$trc";
	push @produce, "$trc<$low";

	if ($i == 0) { next; }
	$trc++;
	printf "%d\"raise/%d-0\"8\@8\n", $trc, $i;
	$low = place ("$i,0", 0);
	$high = place ("$i,0", 1);
	push @consume, "$low>$trc";
	push @produce, "$trc<$high";

	$trc++;
	printf "%d\"fall/%d-0\"8\@8\n", $trc, $i;
	push @consume, "$high>$trc";
	push @produce, "$trc<$low";
}

printf "TP\n"; for $t (@produce) { print "$t\n"; }
#for $t (@readarc) { $x = $t; $x =~ s/-/</; print "$x\n"; }

printf "PT\n"; for $t (@consume) { print "$t\n"; }
#for $t (@readarc) { $t =~ s/(\d*)-(\d*)/$2>$1/; print "$t\n"; }

printf "RA\n"; for $t (@readarc) { $x = $t; $x =~ s/-/</; print "$x\n"; }

exit 0;

sub ra {
	my ($x,$v) = @_;
	my $p = place($x,$v);
	push @readarc,"$trc-$p";
}

sub one {
	my ($b) = @_;
	my $p = place($b,0);
	my $q = place($b,1);
	push @consume,"$p>$trc";
	push @produce,"$trc<$q";
}

sub zero {
	my ($b) = @_;
	my $p = place($b,1);
	my $q = place($b,0);
	push @consume,"$p>$trc";
	push @produce,"$trc<$q";
}

sub newtrans {
	my ($name,$b) = @_;
	$trc++;
	$b =~ s/,/-/;
	printf "%d\"%s/%s\"9\@9\n",$trc,$name,$b;
}

sub place {
	my ($b,$v) = @_;
	my ($i,$j) = split(/,/,$b);
	return $i*2*$N + $j*2 + $v + 1;
}

sub mp {
	my ($i,$j) = @_;
	return "$i,$j";
}
