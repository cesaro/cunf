===================
The Cunf Tool v.1.6
===================

The Cunf Tool is a set of programs for carrying out unfolding-based
verification of Petri nets extended with read arcs, also called contextual
nets, or c-nets.  The package specifically contains the tools:

* cunf: constructs the unfolding of a c-net;
* cna: performs reachability and deadlock analysis using unfoldings
  constructed by cunf;
* Scripts such as ``pep2dot`` or ``grml2pep`` to do format conversion between
  various Petri net formats, unfolding formats, etc.

Cunf is written in C, the sources are in the `<src/>`__ folder. Cna is
written in python, and depends on the `<tools/ptnet/>`__ module; both are
located in the `<tools/>`__ folder.

Cna requires the Minisat solver to be in the ``$PATH``.

.. For your
.. convenience, the source code of Minisat v.2.2.0 was
.. minisat/ in previous versions of this project :)


Downloads and Installing
------------------------

You are encouraged use the latest available release, v1.6.
The code present in the repository is considered experimental and you should
probably not use it.

Precompiled Binaries (include examples)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- Precompiled binaries of the
  `Cunf Tool v1.6 (Mac OSX x86-64)
  <https://cunf.googlecode.com/files/cunf-v1.6_macos_x86-64.zip>`__.
- Precompiled binaries of the
  `Cunf Tool v1.6 (Linux x86-64)
  <https://cunf.googlecode.com/files/cunf-v1.6_linux_x86-64.zip>`__.
- Precompiled binaries of the
  `Cunf Tool v1.6 (Linux i386, 32 bits)
  <https://cunf.googlecode.com/files/cunf-v1.6_linux_i386.zip>`__.

Compiling and Installing from Source Code
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


1. Download the following:
   `Cunf Tool v1.6 (boundle with source code)
   <https://cunf.googlecode.com/files/cunf-v1.6_src.tar.gz>`__.

2. Type the following commands::

    make all
    make dist

   This will put all binaries and libraries into the ``dist/`` folder, from
   where you may copy them to suitable locations in your machine.

3. In particular, make available to Python the module in::

    dist/lib/ptnet

   by copying it to any folder pointed by your installation-dependent default
   module search path, or any folder pointed by the environment variable
   ``PYTHONPATH``.  Cna and other Python scripts won't work without this step.


Full details about the installation are given in section 3 of the
`user's manual
<https://cunf.googlecode.com/files/user-manual-v1.6.pdf>`__.

Documentation
-------------

See
`Cunf Tool user's manual
<https://cunf.googlecode.com/files/user-manual-v1.6.pdf>`__.

Algorithms
----------

The Cunf Tool implements the c-net unfolding procedure proposed by Baldan et
al. in [BCKS08]_.  The algorithms and data structures actually
implemented have been partially described in [RSB11]_, [BBCKRS12]_.
Cunf can only unfold 1-safe c-nets (i.e., no reachable marking puts more
than one token on every place), and for the
time being the tool will blindly assume the input is 1-safe.

Cna, whose name stands for *Contextual Net Analyzer*,
checks for place coverability or deadlock-freedom of a c-net by examining
its unfolding.  The tool reduces these problems to the satisfiability of a
propositional formula that it generates out of the unfolding, and uses
`Minisat <http://minisat.se/>`__
as a back-end to solve the formula.
The algorithms used by Cna has been described in [RS12]_.

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
   <http://www.lsv.ens-cachan.fr/~rodriguez/tools/cunf/rs12.pdf>`__.
   In Proc. of CONCUR'12, vol. 7454 of LNCS, pages 471–485, September 2012.

Similar Tools
-------------

- Stefan Schwoon's
  `Mole <http://www.lsv.ens-cachan.fr/~schwoon/tools/mole/>`__ unfolder.
- Victor Khomenko's
  `Punf <http://homepages.cs.ncl.ac.uk/victor.khomenko/tools/tools.html>`__
  unfolder.
- The `PEP <http://theoretica.informatik.uni-oldenburg.de/~pep/>`__ homepage.

Author and Contact
------------------

The Cunf Tool is developed and maintained by
`César Rodríguez <http://lipn.univ-paris13.fr/~rodriguez/>`__.
Please feel free to contact me with questions or to send feedback.

