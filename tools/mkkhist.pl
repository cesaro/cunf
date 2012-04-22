#!/usr/bin/perl -w

# A transition t consumes the place s, produces the place t, and reads c_i for
# 1 <= i <= k.  For 1 <= i <= k, transition r_i consumes p_i, produces q_i
# and reads c_i, and transition m_i consumes c_i and produces d_i.

# The output is a contextual net.  Ask Stefan why this family

srand $$;
$K = $ARGV[0];

printf "PEP\nPetriBox\nFORMAT_N2\nPL\n";

$plc = 0;
$c = $plc;
for ($i = 1; $i <= $K; $i++) {
	$plc++; printf "%d\"c%d\"M1m1\n",$plc,$i;
}
$d = $plc;
for ($i = 1; $i <= $K; $i++) {
	$plc++; printf "%d\"d%d\"\n",$plc,$i;
}
$p = $plc;
for ($i = 1; $i <= $K; $i++) {
	$plc++; printf "%d\"p%d\"M1m1\n",$plc,$i;
}
$q = $plc;
for ($i = 1; $i <= $K; $i++) {
	$plc++; printf "%d\"q%d\"\n",$plc,$i;
}
$plc++; printf "%d\"s\"M1m1\n",$plc; $s = $plc;
$plc++; printf "%d\"t\"\n",$plc;     $t = $plc;

printf "TR\n";
newtrans("t");
in($s); out($t);
for ($i = 1; $i <= $K; $i++) { ra($c+$i); }

for ($i = 1; $i <= $K; $i++) {
	newtrans(sprintf("r%d",$i));
	in($p+$i); out($q+$i); ra($c+$i);
	newtrans(sprintf("m%d",$i));
	in($c+$i); out($d+$i);
}

printf "TP\n"; for $t (@produce) { print "$t\n"; }
#for $t (@readarc) { $x = $t; $x =~ s/-/</; print "$x\n"; }

printf "PT\n"; for $t (@consume) { print "$t\n"; }
#for $t (@readarc) { $t =~ s/(\d*)-(\d*)/$2>$1/; print "$t\n"; }

printf "RA\n"; for $t (@readarc) { $x = $t; $x =~ s/-/</; print "$x\n"; }

exit 0;

sub ra {
	my $p = shift;
	push @readarc,"$trc-$p";
}

sub out {
	my $p = shift;
	push @produce,"$trc<$p";
}

sub in {
	my $p = shift;
	push @consume,"$p>$trc";
}

sub newtrans {
	my ($s) = @_;
	$trc++;
	printf "%d\"%s\"\n",$trc,$s;
}
