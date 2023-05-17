# gpu-deflate

Deflate on GPU

## Set up

* Install conda and mamba
* `mamba env create -f environment.yml`
* Activate the environment.
    * If using direnv:
      `echo 'layout anaconda gpu-deflate' > .envrc`
      `direnv allow`
    * Else: `conda activate gpu-deflate`
* Test you can build: `bazel build //...`.

## Notes

* Using gcc from conda rather than hermetic LLVM toolchain managed by bazel
  because HIP-CPU requires std::execution, which is not implemented in LLVM 16.
  If / when LLVM implements it we can switch to hermetic LLVM toolchain.

## TODO

* Get code to build and run using HIP and CUDA. Currently HIP-CPU works.
  CUDA alone used to work before I switched to using GCC from conda.
  Should try to get that working first.


## References

* [DEFLATE Compressed Data Format Specification version 1.3](https://tools.ietf.org/html/rfc1951)
* [An Explanation of the Deflate Algorithm](https://zlib.net/feldspar.html)
* [LZ77 Specification](https://www.cs.duke.edu/courses/spring03/cps296.5/papers/ziv_lempel_1977_universal_algorithm.pdf)
