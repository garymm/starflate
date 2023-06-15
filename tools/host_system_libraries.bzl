"""
Repository rule used to determine if a library is installed in the host system.
"""

def _host_system_libraries_impl(rctx):
    rctx.file("BUILD.bazel", executable = False)

    system_libs = rctx.execute(["ldconfig", "-p"]).stdout

    found = [
        '"' + lib + '"'
        for lib in rctx.attr.find
        if lib in system_libs
    ]

    rctx.file(
        "defs.bzl",
        executable = False,
        content = "HOST_SYSTEM_LIBRARIES = [{}]".format(", ".join(found)),
    )

host_system_libraries = repository_rule(
    implementation = _host_system_libraries_impl,
    attrs = {
        "find": attr.string_list(
            mandatory = True,
            doc = """
            List of libraries to detect. Each library is added to `HOST_SYSTEM_LIBRARIES`
            if found.
            """,
        ),
    },
    doc = """
    "A repository rule for checking if system libraries are installed on the
    host platform. Creates a workspace with a `defs.bzl` file containing a
    `HOST_SYSTEM_LIBRARIES` list.
    """,
)
