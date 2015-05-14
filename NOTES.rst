
- struct rusage.ru_maxrss is implementation dependent in mac os,
  see /usr/include/sys/resource.h, change for something better

- new method Speccheck::verify is untested, as well as _do_partial_verification,
  test it with some small net
