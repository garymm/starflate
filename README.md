# gpu-deflate

Deflate on GPU

## Set up

* Install `bazel` or `bazelisk`

* Verify that you have pulled in an appropriate toolchain
```
bazel test //...
```

* Get *HIP*

```
bazel run //:print_hip_info
```

## TODO

* Get code to build and run using HIP and CUDA. Currently HIP-CPU works.
  CUDA alone used to work before I switched to using GCC from conda.
  Should try to get that working first.


## References

* [DEFLATE Compressed Data Format Specification version 1.3](https://tools.ietf.org/html/rfc1951)
* [An Explanation of the Deflate Algorithm](https://zlib.net/feldspar.html)
* [LZ77 Specification](https://www.cs.duke.edu/courses/spring03/cps296.5/papers/ziv_lempel_1977_universal_algorithm.pdf)
