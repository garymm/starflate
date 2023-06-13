"""
Repository rule for determining Bazel directories.
"""

def _local_config_info_impl(rctx):
    rctx.file("BUILD.bazel")

    external = str(rctx.path(".").realpath).removesuffix("/" + rctx.name)

    rctx.file(
        "defs.bzl",
        executable = False,
        content = """BAZEL_EXTERNAL_DIR = "{}"
              """.format(external),
    )

local_config_info = repository_rule(
    implementation = _local_config_info_impl,
    doc = "A repository rule for determining Bazel directories",
)
