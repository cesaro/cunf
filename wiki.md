# Overview #

The Cunf Tool is a set of programs for carrying out unfolding-based
[verification](http://en.wikipedia.org/wiki/Formal_verification)
of [Petri nets](http://en.wikipedia.org/wiki/Petri_net)
extended with read arcs, also called contextual nets, or c-nets.
The package specifically contains the tools:

  * cunf: constructs the unfolding of a c-net;
  * cna: performs reachability and deadlock analysis using unfoldings constructed by cunf;
  * Scripts such as pep2dot or grml2pep to do format conversion between various Petri net formats, unfolding formats, etc.

Cunf is written in C. Cna is
written in Python, and depends on the
[ptnet](http://code.google.com/p/cunf/source/browse/tools/ptnet) module;
Cna requires the
[Minisat solver](http://minisat.se/) to be in the `$PATH`.
For your convenience, a copy of Minisat is distributed both in the bundles
of precompiled binaries and with the sources.


---

# Petri Nets and Unfoldings #

[Petri nets](http://en.wikipedia.org/wiki/Petri_net) are a modelling language for concurrent systems.
The reader unfamiliar to the topic could perhaps start with (Mur89).

Contextual nets are Petri nets where, in addition to the ordinary
_arrows_ between places and transitions, one may find
_read arcs_.
These allow transitions to verify that
tokens exist in a place before firing, but don't consume them
when firing.
Transitions can then be thought of reading a
_context_ required to fire, hence the name.

The unfolding of a c-net is another well-defined c-net of
acyclic structure that fully represents the _behaviour_ (reachable
markings) of the first.
A c-net unfolding is at most as big as the reachablity graph of the
c-net.
Because unfoldings represent behaviour by partial orders rathen than by
interleavings,
for highly concurrent c-nets, unfoldings are often much
(exponentially) smaller, which makes for natural interest in them
for the verification of concurrent systems.

Got interested?
Why you don't continue reading the introduction of the
[Cunf Tool user's manual](http://cunf.googlecode.com/files/user-manual-v1.6.pdf)?


---

# Algorithms #

The Cunf Tool implements the c-net unfolding procedure proposed by Baldan et
al. in (BCKS08).  The algorithms and data structures actually
implemented have been partially described in (RSB11,BBCKRS12).
Cunf can only unfold 1-safe c-nets (i.e., no reachable marking puts more
than one token on every place), and for the
time being the tool will blindly assume the input is 1-safe.

Cna, whose name stands for _Contextual Net Analyzer_,
checks for place coverability or deadlock-freedom of a c-net by examining
its unfolding.  The tool reduces these problems to the satisfiability of a
propositional formula that it generates out of the unfolding, and uses
[Minisat](http://minisat.se/)
as a back-end to solve the formula.
The algorithms used by Cna has been described in (RS12).


---

# Downloads and Installing #

Section 3 of the [manual](http://cunf.googlecode.com/files/user-manual-v1.6.pdf) tells how to install the Cunf Tool on your machine.
For the hurried user, the
[README](https://code.google.com/p/cunf/source/browse/README.rst) included in
the sources contains quick instructions for installation.

## Precompiled Binaries (include examples) ##

  * Precompiled binaries of [the Cunf Tool v1.6 for Mac OSX x86-64 (64 bit)](https://cunf.googlecode.com/files/cunf-v1.6_macos_x86-64.zip).
  * Precompiled binaries of [the Cunf Tool v1.6 for Linux x86-64 (64 bit)](https://cunf.googlecode.com/files/cunf-v1.6_linux_x86-64.zip).
  * Precompiled binaries of [the Cunf Tool v1.6 for Linux i386 (32 bit)](https://cunf.googlecode.com/files/cunf-v1.6_linux_i386.zip).

## Bundle with Source code ##

  * [Source code of the Cunf Tool v1.6](https://cunf.googlecode.com/files/cunf-v1.6_src.tar.gz).

## Development ##

You can additionally
[browse the latest source code](https://code.google.com/p/cunf/source/browse/)
online; or get a copy of it and start hacking, type in your terminal:

```
git clone https://code.google.com/p/cunf/
```


---

# Documentation #

  * The [Cunf Tool v1.6 User's Manual](http://cunf.googlecode.com/files/user-manual-v1.6.pdf).


---

# Authors and Contact #

The Cunf Tool is developed and maintained by
[César Rodríguez](http://www.lsv.ens-cachan.fr/~rodriguez/).


---

# See also #

  * Stefan Schwoon's [Mole](http://www.lsv.ens-cachan.fr/~schwoon/tools/mole/) unfolder.
  * Victor Khomenko's [Punf](http://homepages.cs.ncl.ac.uk/victor.khomenko/tools/tools.html) unfolder.
  * The [PEP](http://theoretica.informatik.uni-oldenburg.de/~pep/) homepage.


---

# References #

  * (BBCKRS12) Paolo Baldan, Alessandro Bruni, Andrea Corradini, Barbara König, César Rodríguez, and Stefan Schwoon.  [Efficient Unfolding of Contextual Petri Nets](http://www.lsv.ens-cachan.fr/Publis/PAPERS/PDF/bbckrs-tcs12.pdf). Theoretical Computer Science 449, 2 – 22 (2012)

  * (BCKS08) Paolo Baldan, Andrea Corradini, Barbara König, and Stefan Schwoon.  [McMillan's Complete Prefix for Contextual Nets](http://dx.doi.org/10.1007/978-3-540-89287-8_12).  In Transactions on Petri Nets and Other Models of Concurrency I, pages 199-220, 2008.  Springer-Verlag.

  * (RSB11) César Rodríguez, Stefan Schwoon, and Paolo Baldan.  [Efficient contextual unfolding](http://www.lsv.ens-cachan.fr/Publis/PAPERS/PDF/RSB-concur11.pdf).  In Proc. of CONCUR'11, volume 6901 of LNCS.  Springer, 2011.

  * (RS12) César Rodríguez and Stefan Schwoon. [Verification of Petri Nets with Read Arcs](http://www.lsv.ens-cachan.fr/~rodriguez/tools/cunf/rs12.pdf). In Proc. of CONCUR'12, vol. 7454 of LNCS, pages 471–485, September 2012.

  * (Mur89) Tadao Murata.  Petri Nets: Properties, Analysis and Applications. In Proc. of the IEEE, vol. 77, no. 4, April 1989
