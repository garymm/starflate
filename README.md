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

### TODO

* Implement Huffman decompression with C++ std lib.
* Set up bazel build of OpenSYCL with OpenMP.
* Port Huffman decompression to SYCL.
* Implement LZ77 with C++ std lib.
* Port LZ77 to SYCL.
* Implement Deflate decompression.
* Try building it for a GPU.

## References

* [DEFLATE Compressed Data Format Specification version 1.3](https://tools.ietf.org/html/rfc1951)
* [An Explanation of the Deflate Algorithm](https://zlib.net/feldspar.html)
* [LZ77 Specification](https://www.cs.duke.edu/courses/spring03/cps296.5/papers/ziv_lempel_1977_universal_algorithm.pdf)
