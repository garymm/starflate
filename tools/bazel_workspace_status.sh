#!/usr/bin/env bash

# See https://bazel.build/docs/user-manual?hl=en#workspace-status

set -o errexit
set -o pipefail
set -o nounset

function hasLocalChanges() {
  if [[ -n $(git status --porcelain) ]]; then
    echo "true"
  else
    echo "false"
  fi
}

function revisionDesc() {
  if [[ "${2}" == "true" ]]; then
    echo "${1}-dirty"
  else
    echo ${1}
  fi
}

# These aren't STABLE_ because they might change without the code changing.
echo BUILD_SCM_USER $(git config user.name) \<$(git config user.email)\>

readonly rev=$(git rev-parse HEAD)
readonly has_changes=$(hasLocalChanges)

echo STABLE_VCS_REVISION ${rev}
echo STABLE_VCS_MODIFIED ${has_changes}
# %cI = committer date, strict ISO 8601 format
echo STABLE_VCS_TIME $(git show --no-patch --format=%cI ${rev})
