# starflate

Deflate decompression in C++23.

This started with the goal of implenting deflate decompression on a GPU, but
it turns out that is basically impossible to parallelize. Compression is possible
to parallelize, but I have not yet implemented it.


## Set up

Should work on Linux x86_64 and MacOS.

* Install [bazelisk](https://github.com/bazelbuild/bazelisk/blob/master/README.md).

* Verify that you can build and test:
```sh
bazel test //...
```

### VS Code

Install the recommended extensions. Type `@recommended` in the extensions search, or see .vscode/extensions.json.

For the Bazel extension to auto-format .bazel files, you need to install [buildifier](https://github.com/bazelbuild/buildtools/releases).
On MacOS, just `brew install buildifier`.

### Auto-completion

Create a compilation database:

```sh
bazel build //... && bazel run @hedron_compile_commands//:refresh_all && bazel build //...
```

Then configure [clangd](https://clangd.llvm.org/).
If you're using VS Code, the .vscode/settings does this for you,
and .vscode/extensions.json already recommends installing the clangd extension.

Otherwise, copy the clangd args from the [.vscode/settings.json](.vscode/settings.json).

## Status

[![CI](https://github.com/garymm/starflate/actions/workflows/check.yml/badge.svg)](https://github.com/garymm/starflate/actions/workflows/check.yml) [![codecov](https://codecov.io/gh/garymm/starflate/graph/badge.svg?token=PGIMUPMNIF)](https://codecov.io/gh/garymm/starflate)


## References

* [DEFLATE Compressed Data Format Specification version 1.3](https://tools.ietf.org/html/rfc1951)
* [Simple-DEFLATE-decompressor](https://github.com/nayuki/Simple-DEFLATE-decompressor)
* [pyflate](https://github.com/garymm/pyflate)
* [An Explanation of the Deflate Algorithm](https://zlib.net/feldspar.html)
* [Parallel lossless compression using GPUs](https://on-demand.gputechconf.com/gtc/2014/presentations/S4459-parallel-lossless-compression-using-gpus.pdf)
* [GPU implementations of deflate encoding and decoding](https://doi.org/10.1002/cpe.7454)
