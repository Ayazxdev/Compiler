// Version constants for the compiler toolchain.
#pragma once

#include <string_view>

namespace luna {

constexpr int VERSION_MAJOR = 0;
constexpr int VERSION_MINOR = 1;
constexpr int VERSION_PATCH = 0;
constexpr std::string_view VERSION_STRING = "0.1.0";

constexpr std::string_view BUILD_TAG = "Development";

} // namespace luna
