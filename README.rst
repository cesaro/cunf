===================
The Cunf Tool v.1.6
===================

The Cunf Tool is a set of programs for carrying out unfolding-based
verification of Petri nets extended with read arcs, also called contextual
nets, or c-nets.  The package specifically contains the tools:

* cunf: constructs the unfolding of a c-net;
* cna: performs reachability and deadlock analysis using unfoldings
  constructed by cunf;
* Scripts such as pep2dot or grml2pep to do format conversion between
  various Petri net formats, unfolding formats, etc.

Cunf is written in C, the sources are in src/ and /include. Cna is
written in python, and depends on the `<tools/ptnet/>`__ module; both are
located in the `<tools/>`__ folder.

Cna requires the Minisat solver to be in the $PATH.  For your
convenience, the source code of Minisat v.2.2.0 is present in the
minisat/ folder, and the main Makefile will compile it for you.

Quick Installation
------------------

Full details about the installation are given in section 3 of the manual:

https://cunf.googlecode.com/files/user-manual-v1.6.pdf

The installation puts all binaries and libraries into the "dist/"
folder, from where you may copy them to suitable locations in your
machine.

Type the following commands::

  make all
  make dist

After that, make available to Python the module

dist/lib/ptnet,

by copying it to any folder pointed by your installation-dependent
default module search path, or any folder pointed by the environment
variable PYTHONPATH.  Cna and other Python scripts won't work without this
step.

Documentation
-------------

See https://cunf.googlecode.com/files/user-manual-v1.6.pdf

Development
-----------

The Cunf Tool is hosted at https://github.com/cesaro/cunf/
You can get the latest source code typing in your terminal::

  git clone https://github.com/cesaro/cunf/

Author and Contact
==================

The Cunf Tool is developed and maintained by
`César Rodríguez <http://lipn.univ-paris13.fr/~rodriguez/>`__.
Please feel free to contact me in case of questions or to send feedback.

