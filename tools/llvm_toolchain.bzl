"""
Provides a wrapper around `llvm_toolchain` that uses a package provided sysroot
instead of the system installed libraries.
"""

load(
    "@bazel_toolchain//toolchain:deps.bzl",
    _llvm_toolchain_dependencies = "bazel_toolchain_dependencies",
)
load(
    "@bazel_toolchain//toolchain:rules.bzl",
    _llvm_toolchain = "llvm_toolchain",
    _llvm_toolchain_files = "llvm",
)
load("@local_config_info//:defs.bzl", "BAZEL_EXTERNAL_DIR")

#  Rename to (hopefully) be less confusing
llvm_toolchain_dependencies = _llvm_toolchain_dependencies

def _patch_dynamic_linker_impl(rctx):
    label = Label(rctx.attr.base_workspace + ":BUILD.bazel")
    base_workspace = str(rctx.path(label).realpath).removesuffix("/BUILD.bazel")
    this_workspace = str(rctx.path("."))

    files = rctx.execute(
        ["ls", base_workspace],
    ).stdout.replace("WORKSPACE", "").replace("bin", "").strip().split("\n")

    for f in files:
        source = base_workspace + "/" + f
        dest = this_workspace + "/" + f
        rctx.execute(["ln", "-s", source, dest])

    rctx.execute(["cp", "-r", base_workspace + "/bin", "real-bin"])

    for tool in rctx.execute(["ls", "real-bin"]).stdout.strip().split("\n"):
        rctx.file(
            "bin/" + tool,
            content = """#!/bin/bash
            exec "{dynamic_linker}" --library-path "{paths}" "{real_tool}" "$@"
            """.format(
                dynamic_linker = rctx.attr.dynamic_linker,
                paths = ":".join(rctx.attr.library_paths),
                real_tool = this_workspace + "/real-bin/" + tool,
            ),
        )

_patch_dynamic_linker = repository_rule(
    implementation = _patch_dynamic_linker_impl,
    attrs = {
        "dynamic_linker": attr.string(
            mandatory = True,
            doc = """
            Absolute path to an alternate dynamic linker.
            """,
        ),
        "library_paths": attr.string_list(
            mandatory = True,
            doc = """
            Absolute paths for the library search directories. These are used
            along with the dynamic linker.
            """,
        ),
        "base_workspace": attr.string(
            mandatory = True,
            doc = """
            Base workspace containing binaries to patch.
            """,
        ),
    },
    doc = """
    Copies the contents of `base_workspace` but replaces all files in the `bin`
    directory with wrapper scripts. These wrapper scripts use the specified
    `dynamic_linker`, allowing runtime use of libraries in `library_paths`.
    """,
)

def _extend_linux_x86_64_key(arg_dict, values):
    existing = arg_dict.get("linux-x86_64", arg_dict[""])
    arg_dict["linux-x86_64"] = existing + values
    return arg_dict

def llvm_toolchain(linux_x86_64_sysroot = "", **kwargs):
    # buildifier: disable=function-docstring-args
    """
    Wraps `llvm_toolchain`, forcing use of a user-provided sysroot on linux-x86_64.

    Args:
      linux_x86_64_sysroot: string
        label of a sysroot

    for other args, see
    https://github.com/grailbio/bazel-toolchain/blob/master/toolchain/rules.bzl
    """

    if not linux_x86_64_sysroot:
        fail("""
        This rule requires the sysroot to be specified for linux-x86_64
        platforms. If this not desired, use `llvm_toolchain` from
        `@bazel_toolchain` directly.
        """)

    linux_x86_64_sysroot_pkg = linux_x86_64_sysroot
    linux_x86_64_sysroot_path = (
        BAZEL_EXTERNAL_DIR +
        linux_x86_64_sysroot_pkg.replace("@", "/").replace("//", "/")
    )

    if "sysroot" in kwargs or "toolchain_roots" in kwargs:
        fail("""
        `sysroot` and `toolchain_roots` need to be overriden by
        `llvm_toolchain`.
        """)

    _llvm_toolchain_files(
        name = kwargs["name"] + "_files",
        llvm_version = kwargs["llvm_version"],
    )

    _patch_dynamic_linker(
        name = kwargs["name"] + "_patched_files",
        dynamic_linker = linux_x86_64_sysroot_path + "/lib/ld-linux-x86-64.so.2",
        library_paths = [
            "{sysroot}/{path}".format(
                sysroot = linux_x86_64_sysroot_path,
                path = path,
            )
            for path in ["lib", "usr/lib"]
        ],
        base_workspace = "@{name}_files//".format(name = kwargs["name"]),
    )

    kwargs["sysroot"] = {
        "linux-x86_64": linux_x86_64_sysroot_pkg,
    }

    kwargs["toolchain_roots"] = {
        "": "@{files}//".format(
            files = kwargs["name"] + "_files",
        ),
        "linux-x86_64": "@{patched_files}//".format(
            patched_files = kwargs["name"] + "_patched_files",
        ),
    }

    # https://github.com/bazelbuild/bazel/issues/4605
    patched_files_abspath = (
        BAZEL_EXTERNAL_DIR + "/" + kwargs["name"] + "_patched_files"
    )

    kwargs["link_libs"] = _extend_linux_x86_64_key(
        kwargs.get("link_libs", {"": []}),
        [
            # libraries from llvm are statically linked and we don't want to
            # dynamically link duplicates from sysroot
            "-nostdlib++",
            "-fuse-ld={patched_files_abspath}/bin/ld.lld".format(
                patched_files_abspath = patched_files_abspath,
            ),
        ] + [
            opt.format(sysroot = linux_x86_64_sysroot_path)
            for opt in [
                "-Wl,--rpath={sysroot}/lib",
                "-Wl,--rpath={sysroot}/usr/lib",
                "-Wl,--rpath={sysroot}/../lib64",
                "-Wl,--dynamic-linker={sysroot}/lib/ld-linux-x86-64.so.2",
            ]
        ],
    )

    major_version = kwargs["llvm_version"].split(".")[0]
    kwargs["cxx_builtin_include_directories"] = _extend_linux_x86_64_key(
        kwargs.get("cxx_builtin_include_directories", {"": []}),
        [
            "{patched_files_abspath}/{include_dir}".format(
                patched_files_abspath = patched_files_abspath,
                include_dir = include_dir,
            )
            for include_dir in [
                "/include/c++/v1",
                "/include/x86_64-unknown-linux-gnu/c++/v1",
                "/lib/clang/{}/include".format(major_version),
                "/lib/clang/{}/share".format(major_version),
            ]
        ] + [
            # The clang-tidy aspect won't find include directories in the sysroot unless we
            # specify the abspath.
            #
            # These *must* be found after C++ standard library headers:
            # https://github.com/llvm/llvm-project/commit/8cedff10a18d8eba9190a645626fa6a509c1f139
            "{sysroot}/{include_dir}".format(
                sysroot = linux_x86_64_sysroot_path,
                include_dir = include_dir,
            )
            for include_dir in [
                "include",
                "usr/include",
                "usr/local/include",
            ]
        ],
    )

    _llvm_toolchain(**kwargs)
