=====================================
Cunf: An Unfolder for Contextual Nets
=====================================

Cunf is a set of research tools to carrying out unfolding-based
`formal verification`_ of `Petri nets`_ extended with `read arcs`_, also called
contextual nets, or c-nets.  The package specifically contains the tools:

- ``cunf``: constructs the unfolding of a c-net;
- ``cna``: performs reachability and deadlock analysis using unfoldings
  constructed by cunf;
- Scripts such as ``pep2dot`` or ``grml2pep`` to do format conversion between
  various Petri net formats, unfolding formats, etc.
- ``ptnet``: a small Python module (see `<scripts/ptnet/>`__) suitable to
  programmatically generate and manage Petri net models.

.. _formal verification: https://en.wikipedia.org/wiki/Formal_verification
.. _Petri nets: https://en.wikipedia.org/wiki/Petri_net
.. _read arcs: http://www.lsv.fr/~rodrigue/att/thesis-final.pdf


Installation
============

From Precompiled Binaries
-------------------------


1. Install dependencies. In Ubuntu, type::

    sudo apt-get install minisat python2.7 python-networkx

   In other operating systems, please ensure that the executable ``minisat`` is
   in your ``PATH``, and that Python 2.7 and the Python package ``networkx`` are
   available in your system.

2. Check the `latest release`_ to find the link to the ``.tar.gz`` file with
   precompiled binaries that you want to download (currently only available for
   Linux x86_64). Download it and uncompress it::

    wget https://github.com/cesaro/cunf/releases/download/v?.?.?/cunf-x86_64-v1.6.1.tar.gz
    tar xzvf cunf-x86_64-v1.6.1.tar.gz 
    cd cunf-x86_64-v1.6.1

3. All binaries are in the ``bin/`` folder, you can directly run them from
   there. For instance, you can do deadlock detection on one of the examples::

    ./bin/cunf -i examples/dijkstra/dij04.ll_net -s out.unf
    ./bin/cna  -d out.unf

   The second command should print::
   
    NO , the net is deadlock-free

   together with other debugging information.

From Source Code
----------------

Please note that development takes place in the ``master`` branch of this
repository. If you want a stable version of the tool you should download and
compile the sources of the `latest release`_ available.

1. Ensure that you have all the necessary dependencies. In Ubuntu, install the
   following packages::

    sudo apt-get install libc-dev coreutils git make
    sudo apt-get install clang flex bison cpp minisat python3.5 python3-networkx


2. Check out the latest release::

    git clone git@github.com:cesaro/cunf.git
    cd cunf
    git checkout v1.6.1

2. Compile the source code::

    make all
    make dist

   This will put all binaries and libraries into the ``dist/`` folder, from
   where you may directly run them or copy them to suitable locations in your
   machine.

3. To test the insallation, do deadlock detection on one of the examples::

    ./dist/bin/cunf -i dist/examples/dijkstra/dij04.ll_net -s out.unf
    ./dist/bin/cna  -d out.unf 

   The second command should print::

    NO , the net is deadlock-free

   together with other debugging information.


.. _latest release: https://github.com/cesaro/cunf/releases/latest

Documentation
=============

The Cunf user's manual (available with the `latest release`_) contains all
documentation available, including a guided tutorial of the tool.

Algorithms
----------

Cunf implements the contextual net unfolding algorithm proposed by Baldan et al.
in [BCKS08]_.  The algorithms and data structures actually implemented have been
partially described in [RSB11]_, [BBCKRS12]_.  Cunf can only unfold 1-safe
c-nets (i.e., no reachable marking puts more than one token on every place), and
for the time being the tool will blindly assume the input is 1-safe.

Cna, whose name stands for *Contextual Net Analyzer*, checks for place
coverability or deadlock-freedom of a c-net by examining its unfolding.  The
tool reduces these problems to the satisfiability of a propositional formula
that it generates out of the unfolding, and uses
`Minisat <http://minisat.se/>`__
as a back-end to solve the formula.  The algorithms used by Cna has been
described in [RS12]_.

.. [BBCKRS12]
   Paolo Baldan, Alessandro Bruni, Andrea Corradini, Barbara König, César
   Rodríguez, and Stefan Schwoon.
   `Efficient Unfolding of Contextual Petri Nets
   <http://www.lsv.ens-cachan.fr/Publis/PAPERS/PDF/bbckrs-tcs12.pdf>`__.
   Theoretical Computer Science 449, 2 – 22 (2012).

.. [BCKS08]
   Paolo Baldan, Andrea Corradini, Barbara König, and Stefan Schwoon.
   `McMillan's Complete Prefix for Contextual Nets
   <http://dx.doi.org/10.1007/978-3-540-89287-8_12>`__.
   In Transactions on Petri Nets and Other Models of Concurrency I, p. 199-220,
   2008.  Springer-Verlag.

.. [RSB11]
   César Rodríguez, Stefan Schwoon, and Paolo Baldan.
   `Efficient contextual unfolding
   <http://www.lsv.ens-cachan.fr/Publis/PAPERS/PDF/RSB-concur11.pdf>`__.
   In Proc. of CONCUR'11, volume 6901 of LNCS.  Springer, 2011.

.. [RS12]
   César Rodríguez and Stefan Schwoon.
   `Verification of Petri Nets with Read Arcs
   <http://www.lsv.ens-cachan.fr/Publis/PAPERS/PDF/RS-concur12.pdf>`__.
   In Proc. of CONCUR'12, vol. 7454 of LNCS, pages 471–485, September 2012.

Related Tools
=============

- Stefan Schwoon's
  `Mole <http://www.lsv.ens-cachan.fr/~schwoon/tools/mole/>`__ unfolder.
- Victor Khomenko's
  `Punf <http://homepages.cs.ncl.ac.uk/victor.khomenko/tools/tools.html>`__
  unfolder.
- The `PEP <http://peptool.sourceforge.net/>`__ homepage.
- `DPU <https://github.com/cesaro/dpu>`__, an unfolder for multithreaded C
  programs.

Author and Contact
------------------

The Cunf Tool is developed and maintained by
`César Rodríguez <http://lipn.univ-paris13.fr/~rodriguez/>`__.
Please feel free to contact me with questions or to send feedback.

