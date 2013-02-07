#!/usr/bin/perl -w

$N = $ARGV[0]+1;

printf "PEP\nPetriBox\nFORMAT_N2\nPL\n";

$plc = 0;
for ($i = 0; $i < $N; $i++) {
	for ($j = 0; $j < $N; $j++) {
		$plc++;
		printf "%d\"P%d-%d-0\"7\@7\n",$plc,$i,$j;
		$plc++;
		printf "%d\"P%d-%d-1\"7\@7\n",$plc,$i,$j;
	}
}

for ($i = 1; $i < $N; $i++) { $plc++; printf "%d\"init-left-%d\"7\@7M1m1\n",$plc,$i; }
for ($i = 1; $i < $N; $i++) { $plc++; printf "%d\"init-up-%d\"7\@7M1m1\n",$plc,$i; }

printf "TR\n";
for ($i = 1; $i < $N; $i++) {
	transfer("init-left-0",2*$N*$N+$i,place("0,$i",0));
	transfer("init-left-1",2*$N*$N+$i,place("0,$i",1));
	transfer("init-up-0",2*$N*$N+$N-1+$i,place("$i,0",0));
	transfer("init-up-1",2*$N*$N+$N-1+$i,place("$i,0",1));
}

for ($i = 1; $i < $N; $i++) {
	for ($j = 1; $j < $N; $j++) {
		my $b = mp($i,$j);
		my $left = mp($i-1,$j);
		my $up = mp($i,$j-1);
		newtrans("raise-left",$b); ra($left,1); one($b);
		newtrans("raise-up",$b);   ra($up,1);   one($b);
		newtrans("fall",$b);       ra($left,0); ra($up,0); zero($b);
	}
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
	my ($type,$b) = @_;
	$trc++;
	$type .= "-" if $b ne "";
	$b =~ s/,/-/;
	printf "%d\"%s%s\"7\@7\n",$trc,$type,$b;
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

sub transfer {
	my ($title,$p,$q) = @_;
	newtrans($title,"");
	push @consume,"$p>$trc";
	push @produce,"$trc<$q";
}
