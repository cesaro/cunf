
- struct rusage.ru_maxrss is implementation dependent in mac os,
  see /usr/include/sys/resource.h, change for something better

- new method Speccheck::verify is untested, as well as _do_partial_verification,
  test it with some small net

- The new version of Cna, in C++, integrated in Cunf, is implemented in the
  classes Speccheck, methods load_spec() and verify(). It only works for
  unfoldings of plain nets, see Cunfsat::encode()

- Document somewhere the format of the spec files!! It's not difficult to guess
  it by looking at the spec_lexer.l

- The DEBUG/INFO/TRACE/PRINT macros are much less optimal than they could be.
  DPU contains an updated version of these.

Changelog:
- v1.6 Installation
- v1.5 CUF03 format
- v1.4 CUF02 format
- v1.3 CUF file format
- v1.2 Asymetric concurrency
- v1.1 Bug free!
- v1.0 Initial feature-complete implementation
