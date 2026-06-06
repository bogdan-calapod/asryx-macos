#ifndef ASRYX_RUNTIME_RUNTIME_HPP
#define ASRYX_RUNTIME_RUNTIME_HPP

#include "config/config.hpp"

#include <string>

namespace runtime {

std::string get_status();
void toggle(const config::Config& cfg);

} // namespace runtime

#endif // ASRYX_RUNTIME_RUNTIME_HPP
