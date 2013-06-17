#!/usr/bin/perl

use XML::Parser;

$net = first_child(@{new XML::Parser(Style => 'Tree')->parse(STDIN)}[1],"net");
print "PEP\nPetriBox\nFORMAT_N2\nPL\n";

$c = 0;
for (children($net,"place")) {
	$name = attrib($_,"id");
	$pl{$name} = ++$c;
	$marked = first_child(first_child(first_child($_,
			"initialMarking"),"value"),0);
	print "\"$name\"".($marked? "M1" : "")."\n";
}

$c = 0;
print "TR\n";
for (children($net,"transition")) {
	$name = attrib($_,"id");
	$tr{$name} = ++$c;
	print "\"$name\"\n";
}

for (children($net,"arc")) {
	($src,$tgt) = (attrib($_,"source"),attrib($_,"target"));
	$type = first_child($_,"type");
	if ($type && attrib($type,"value") eq "inhibitor") {
		push @ra,($tr{$tgt}."<".$pl{$src});
	} elsif (defined($pl{$src})) {
		push @pt,($pl{$src}.">".$tr{$tgt});
	} else {
		push @tp,($tr{$src}."<".$pl{$tgt});
	}
}
print "TP\n".join("\n",@tp)."\n";
print "PT\n".join("\n",@pt)."\n";
print "RA\n".join("\n",@ra)."\n";

exit 0;

# find all children of an element with a certain tag

sub children {
	my @list = @{$_[0]};
	my $tag = $_[1];
	my $i;
	my @result = ();

	for ($i = 1; $i < $#list; $i += 2)
		{ push @result,($list[$i+1]) if $list[$i] eq $tag; }
	return @result;
}

# find first child of an element with a certain tag, or return undef

sub first_child {
	my @list = @{$_[0]};
	my $tag = $_[1];
	my $i = 1;

	while ($i++ < $#list) { return $list[$i] if $list[$i-1] eq $tag; }
	return undef;
}

# find attribute value of an element, possibly undef

sub attrib {
	my %h = %{@{$_[0]}[0]};
	return $h{$_[1]};
}
