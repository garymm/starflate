#include "version/version.hpp"

/**
 * This file is *MAGIC*
 * When we compile if from Bazel, we do magical substitutions to some variables defined in CAPS in this file.
 * See `version/Build` for up-to-date list of substitutions.
 *
 *  This file takes them and packages them to a API that is more pleasant to work with.
 */
using namespace std;

namespace gpu_deflate {
// NOTE: bazel ignores build flags for this.
// this means that: we can't use c++14\c++11 features here
// and this code has no debugger support


#ifdef NDEBUG
constexpr bool is_release_build = true;
#else
constexpr bool is_release_build = false;
#endif

constexpr bool build_vcs_modified = STABLE_VCS_MODIFIED;
constexpr string_view build_vcs_revision = STABLE_VCS_REVISION;

const bool Version::isReleaseBuild = is_release_build;

string makeVCSStatus() {
    return build_vcs_modified ? "-dirty" : "";
}
const string Version::build_vcs_status = makeVCSStatus();

string makeVCSRevision() {
    return string(build_vcs_revision);
}
const string Version::build_vcs_revision = makeVCSRevision();

const string Version::full_version_string = (Version::isReleaseBuild ? ""s : "(non-release) "s) +
                                            Version::build_vcs_revision + Version::build_vcs_status;
}
