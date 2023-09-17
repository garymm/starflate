#pragma once

#include <string>

namespace starflate {
class Version
{
public:
  static const std::string build_vcs_revision;
  static const std::string build_vcs_status;
  static const std::string full_version_string;
  static const bool isReleaseBuild;
};
}  // namespace starflate
