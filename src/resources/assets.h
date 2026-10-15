#pragma once

#include <cstdint>
#include <span>

namespace corncob::resources {

std::span<uint8_t const> cockpit_asset() noexcept;

} // namespace corncob::resources
