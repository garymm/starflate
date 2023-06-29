# gpu-deflate

Deflate on GPU

## Set up

Should work on Linux and MacOS.

* Install `bazel` or `bazelisk`

* Verify that you can build and test:
```
bazel test //...
```

### VS Code

Install the recommended extensions. See .vscode/extensions.json.

### Auto-completion

Create a compilation database:

```sh
bazel build //...
bazel run @hedron_compile_commands//:refresh_all
```

Then configure [clangd](https://clangd.llvm.org/).
If you're using VS Code, the .vscode/settings does this for you,
and .vscode/extensions.json already recommends installing the clangd extension.

Otherwise, set these clangd args:

```
--header-insertion=never
--compile-commands-dir=${workspaceFolder}/
--query-driver=**
```


## Status

### Done

* Build Huffman code tables from given symbol frequencies.
* Huffman decoding with C++ std lib.

### TODO

* Get SYCL building with bazel. Already have OpenSYCL building for CPU only [here](https://github.com/garymm/xpu).
  Would be nicer to use [intel's LLVM](https://github.com/intel/llvm) which supports lots of GPUs.
* (maybe?) Implement LZ77 with C++ std lib.
* Implement Deflate decompression with C++ std lib.
* Port Deflate to SYCL.
* Benchmark it on CPU.
* Build system work to get it to run on GPU.
* Benchmark it on GPU.

## References

* [DEFLATE Compressed Data Format Specification version 1.3](https://tools.ietf.org/html/rfc1951)
* [pyflate](https://github.com/garymm/pyflate)
* [An Explanation of the Deflate Algorithm](https://zlib.net/feldspar.html)
* [LZ77 Specification](https://www.cs.duke.edu/courses/spring03/cps296.5/papers/ziv_lempel_1977_universal_algorithm.pdf)
