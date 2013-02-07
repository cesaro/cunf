#!/usr/bin/perl -w

srand $$;
$K = $ARGV[0];

printf "PEP\nPetriBox\nFORMAT_N2\nPL\n";

$plc = 0;
$c = $plc;
for ($i = 1; $i <= $K; $i++) {
	$a[$i] = ++$plc; printf "$plc\"a$i\"M1m1\n";
	$b[$i] = ++$plc; printf "$plc\"b$i\"\n";
	$c[$i] = ++$plc; printf "$plc\"c$i\"\n";
}
$p = ++$plc; printf "%d\"p\"M1m1\n",$plc;

printf "TR\n";
for ($i = 1; $i <= $K; $i++) {
	newtrans("t$i");
	in($a[$i]); in($p); out($b[$i]);
	newtrans("u$i");
	in($b[$i]); out($p); out($c[$i]);
#	newtrans("v$i");
#	in($c[$i]); out($a[$i]);
}

printf "TP\n"; for $t (@produce) { print "$t\n"; }
for $t (@readarc) { $x = $t; $x =~ s/-/</; print "$x\n"; }

printf "PT\n"; for $t (@consume) { print "$t\n"; }
for $t (@readarc) { $t =~ s/(\d*)-(\d*)/$2>$1/; print "$t\n"; }

#printf "RA\n"; for $t (@readarc) { $x = $t; $x =~ s/-/</; print "$x\n"; }

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
