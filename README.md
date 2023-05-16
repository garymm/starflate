# gpu-deflate

Deflate on GPU

## TODO

* Trying to get code that depends on HIP-CPU to build with bazel.
  Seems we need to depend on Intel's TBB.

* If we decide to not use HIP and just do CUDA:
  build of CUDA code is not currently using the hermetic clang toolchain,
  rather it's relying on the system's nvcc. should
  look at https://github.com/bazel-contrib/rules_cuda/issues/4.


## References

* [DEFLATE Compressed Data Format Specification version 1.3](https://tools.ietf.org/html/rfc1951)
* [An Explanation of the Deflate Algorithm](https://zlib.net/feldspar.html)
* [LZ77 Specification](https://www.cs.duke.edu/courses/spring03/cps296.5/papers/ziv_lempel_1977_universal_algorithm.pdf)
