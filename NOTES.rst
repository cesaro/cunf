
- struct rusage.ru_maxrss is implementation dependent in mac os,
  see /usr/include/sys/resource.h, change for something better

- new method Speccheck::verify is untested, as well as _do_partial_verification,
  test it with some small net

Changelog:
- v1.6 Installation
- v1.5 CUF03 format
- v1.4 CUF02 format
- v1.3 CUF file format
- v1.2 Asymetric concurrency
- v1.1 Bug free!
- v1.0 Initial feature-complete implementation
