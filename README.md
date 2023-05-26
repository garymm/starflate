# gpu-deflate

Deflate on GPU

## Set up

Currently only supported on Linux x86_64 / AMD64.

* Install `bazel` or `bazelisk`

* Verify that you can build and test:
```
bazel test //...
```
## TODO

* Support building on ARM64. Currently using Bootlin
  toolchain which only exists for x86-64. Probably easiest
  to:
  *  switch to conda environment or docker container
     that installs the toolchain (don't use a bazel
     hermetic toolchain).

* Set up SYCL

## References

* [DEFLATE Compressed Data Format Specification version 1.3](https://tools.ietf.org/html/rfc1951)
* [An Explanation of the Deflate Algorithm](https://zlib.net/feldspar.html)
* [LZ77 Specification](https://www.cs.duke.edu/courses/spring03/cps296.5/papers/ziv_lempel_1977_universal_algorithm.pdf)
