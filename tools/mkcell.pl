#!/usr/bin/perl -w

$N = $ARGV[0];

printf "PEP\nPetriBox\nFORMAT_N2\nPL\n";

$plc = 0;
for ($i = 0; $i < $N; $i++) {
	for ($j = 0; $j < $N; $j++) {
		$marked = int(rand(2));
		$plc++;
		printf "%d\"P%d-%d-0\"%s\n",$plc,$i,$j,$marked? "" : "M1m1";
		$plc++;
		printf "%d\"P%d-%d-1\"%s\n",$plc,$i,$j,$marked? "M1m1" : "";
	}
}

printf "TR\n";
for ($i = 0; $i < $N; $i++) {
	for ($j = 0; $j < $N; $j++) {
		if (($i == 0 || $i == $N-1) && ($j == 0 || $j == $N-1))
			{ corner($i,$j); }
		elsif ($i == 0 || $i == $N-1 || $j == 0 || $j == $N-1)
			{ edge($i,$j); }
		else { inner($i,$j); }
	}
}

printf "TP\n"; for $t (@produce) { print "$t\n"; }
for $t (@readarc) { $x = $t; $x =~ s/-/</; print "$x\n"; }

printf "PT\n"; for $t (@consume) { print "$t\n"; }
for $t (@readarc) { $t =~ s/(\d*)-(\d*)/$2>$1/; print "$t\n"; }

exit 0;

sub corner {
	my ($i,$j) = @_;
	my $b = mp($i,$j);
	my $x = mp($i, ($j > 0)? $j-1 : 1);
	my $y = mp(($i > 0)? $i-1 : 1, $j);

	newtrans($b,"1(xy)"); ra($x,1); ra($y,1); one($b);
	newtrans($b,"0(x)"); ra($x,0); zero($b);
	newtrans($b,"0(y)"); ra($y,0); zero($b);
}

sub edge {
	my ($i,$j) = @_;
	my $b = mp($i,$j);
	my $x = mp($i, ($j > 0)? $j-1 : 1);
	my $y = mp(($i > 0)? $i-1 : 1, $j);
	my $z;
	if ($i == 0 || $i == $N-1) { $z = mp($i,$j+1); }
				else { $z = mp($i+1,$j) };

	newtrans($b,"1(xy)"); ra($x,1); ra($y,1); one($b);
	newtrans($b,"1(xz)"); ra($x,1); ra($z,1); one($b);
	newtrans($b,"1(yz)"); ra($y,1); ra($z,1); one($b);

	newtrans($b,"0(xy)"); ra($x,0); ra($y,0); zero($b);
	newtrans($b,"0(xz)"); ra($x,0); ra($z,0); zero($b);
	newtrans($b,"0(yz)"); ra($y,0); ra($z,0); zero($b);
}

sub inner {
	my ($i,$j) = @_;
	my $b = mp($i,$j);
	my $x = mp($i-1,$j);
	my $y = mp($i,$j+1);
	my $z = mp($i+1,$j);
	my $w = mp($i,$j-1);

	newtrans($b,"1(xyz)"); ra($x,1); ra($y,1); ra($z,1); one($b);
	newtrans($b,"1(yzw)"); ra($y,1); ra($z,1); ra($w,1); one($b);
	newtrans($b,"1(zwx)"); ra($z,1); ra($w,1); ra($x,1); one($b);
	newtrans($b,"1(wxy)"); ra($w,1); ra($x,1); ra($y,1); one($b);

	newtrans($b,"0(xy)"); ra($x,0); ra($y,0); zero($b);
	newtrans($b,"0(xz)"); ra($x,0); ra($z,0); zero($b);
	newtrans($b,"0(xw)"); ra($x,0); ra($w,0); zero($b);
	newtrans($b,"0(yz)"); ra($y,0); ra($z,0); zero($b);
	newtrans($b,"0(yw)"); ra($y,0); ra($w,0); zero($b);
	newtrans($b,"0(zw)"); ra($z,0); ra($w,0); zero($b);
}

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
	my ($b,$s) = @_;
	$trc++;
	$b =~ s/,/-/;
	printf "%d\"%s->%s\"\n",$trc,$b,$s;
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
