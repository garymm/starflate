# gpu-deflate

Deflate on GPU

## Set up

Currently only supported on Linux x86_64 / AMD64.

* Install `bazel` or `bazelisk`

* Verify that you can build and test:
```
bazel test //...
```

* See what [HIP](https://rocm-developer-tools.github.io/HIP/) detects about your computer:

```
bazel run //:print_hip_info
```

If you have the CUDA toolkit installed,
you can verify things are working with CUDA:

```
bazel run --cuda //:print_hip_info
```

## TODO

* Support building on ARM64. Currently using Bootlin
  toolchain which only exists for x86-64. Probably easiest
  to:
  *  wait for libc++ (LLVM's std lib) to implement
     std::execution (AKA PSTL), then we can switch to use
     LLVM toolchain, or
  *  switch to conda environment or docker container
     that installs the toolchain (don't use a bazel
     hermetic toolchain).


## References

* [DEFLATE Compressed Data Format Specification version 1.3](https://tools.ietf.org/html/rfc1951)
* [An Explanation of the Deflate Algorithm](https://zlib.net/feldspar.html)
* [LZ77 Specification](https://www.cs.duke.edu/courses/spring03/cps296.5/papers/ziv_lempel_1977_universal_algorithm.pdf)
