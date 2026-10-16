#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace corncob::platform {

std::filesystem::path default_save_directory();
std::vector<uint8_t> read_binary(std::filesystem::path const &path);
void write_binary(std::filesystem::path const &path, std::span<uint8_t const> bytes);

} // namespace corncob::platform
