"""
Provides a wrapper around `bazel_toolchain_dependencies` which also obtains the
abspath to Bazel's external directory for use by the `llvm_toolchain` wrapper.
"""

load("@bazel_toolchain//toolchain:deps.bzl", "bazel_toolchain_dependencies")
load("//tools:local_config_info.bzl", "local_config_info")

#  Rename to (hopefully) be less confusing
def llvm_toolchain_dependencies():
    bazel_toolchain_dependencies()
    local_config_info(name = "local_config_info")
