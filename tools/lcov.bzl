"""
Rule for generating a coverage report
"""

load("@bazel_skylib//rules:write_file.bzl", "write_file")
load("@local_config_info//:defs.bzl", "BAZEL_BIN")

def lcov(
        name,
        instrumented_targets = [],
        test_targets = [],
        coverage_opts = [
            "--combined_report=lcov",
            "--experimental_generate_llvm_lcov",
            "--test_output=errors",
            # https://github.com/bazelbuild/bazel/issues/13919
            "--test_env=COVERAGE_GCOV_OPTIONS=-b",
        ],
        lcov_tool = "lcov",
        lcov_opts = []):
    """
    Generate an lcov reports from the output of Bazel's coverage command.

    Args:
      name: string
        Name for `lcov` rule.
      instrumented_targets: string_list
        List of targets for which to determine coverage.
      test_targets: string_list
        List of targets used to determine coverage.
      coverage_opts: string_list
        Options passed to Bazel's coverage command.
      lcov_tool: string
        lcov tool to use. See `@lcov//:bin/` for a complete list
      lcov_opts: string_list
        Options passed to the lcov tool to generate a report.

    `instrumented_targets` and `test_targets` are specified as strings which
    are passed to the underlying coverage command, and may include wildcards.

    If `lcov_tool` is set to `genhtml`, the output directory is set to `$(bazel
    info output_path)/lcov_html` and the target will attempt to open the html
    page.

    Example:

      lcov(
        name = "lcov_list",
        instrumented_targets = ["//:skytest"],
        test_targets = ["//test/..."],
        lcov_opts = ["--rc lcov_branch_coverage=1", "--list"],
      )

      bazel run :lcov_list

      lcov(
        name = "lcov_html",
        instrumented_targets = ["//:skytest"],
        test_targets = ["//test/..."],
        lcov_tool = "genhtml",
      )

      bazel run :lcov_html
    """
    instrumented = [
        "--instrumentation_filter={}".format(target)
        for target in instrumented_targets
    ]

    coverage_command = " ".join(
        ["{bazel} coverage {bazel_common_opts}"] +
        coverage_opts + instrumented + test_targets,
    )

    open_command = ""

    if lcov_tool == "genhtml":
        output_dir = "\"${{output_path}}/lcov_html\""
        lcov_opts = ["--output {}".format(output_dir)] + lcov_opts
        open_command = "xdg-open {}/index.html".format(output_dir)

    lcov_command = " ".join(["\"${{lcov_tool}}\""] + lcov_opts)

    content = [
        "#!/usr/bin/env bash",
        "set -euo pipefail",
        "",
        "lcov_tool=\"$(pwd)/${{1}}\"",
        "",
        "cd $BUILD_WORKSPACE_DIRECTORY",
        "output_path=\"$({bazel} info {bazel_common_opts} output_path)\"",
        "coverage_report=\"${{output_path}}/_coverage/_coverage_report.dat\"",
        "",
        coverage_command + " || true",
        "",
        lcov_command + " \"${{coverage_report}}\"",
        open_command,
    ]

    content = [
        line.format(
            bazel = BAZEL_BIN,
            bazel_common_opts = "--color=yes --curses=yes",
        )
        for line in content
    ]

    write_file(
        name = "gen_" + name,
        out = name + ".sh",
        content = content,
        is_executable = True,
        tags = ["manual"],
        visibility = ["//visibility:private"],
    )

    native.sh_binary(
        name = name,
        srcs = [name + ".sh"],
        data = ["@lcov//:bin/{}".format(lcov_tool)],
        args = ["$(rootpath @lcov//:bin/{})".format(lcov_tool)],
    )
