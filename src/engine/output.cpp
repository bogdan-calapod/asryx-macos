#include "constants/constants.hpp"
#include "engine/engine.hpp"
#include "platform/process.hpp"

#include <iostream>
#include <string>

namespace engine {

namespace {

#if defined(__APPLE__)
std::string escape_applescript(const std::string& input)
{
  std::string output;
  output.reserve(input.size());
  for (char ch : input) {
    if (ch == '\\' || ch == '"') {
      output.push_back('\\');
    }
    output.push_back(ch);
  }
  return output;
}
#endif

} // namespace

bool copy_to_clipboard(const std::string& text)
{
#ifdef ASRYX_TESTING
  if (auto hook = testing::copy_to_clipboard_hook()) {
    return hook(text);
  }
#endif

#if defined(__APPLE__)
  if (platform::command_exists("pbcopy")) {
    return platform::run_process_with_stdin({"pbcopy"}, text);
  }

  std::cerr << "Warning: pbcopy is not available to copy transcript.\n";
  return false;
#else
  if (platform::command_exists("wl-copy")) {
    return platform::run_process_with_stdin({"wl-copy"}, text);
  }

  if (platform::command_exists("xclip")) {
    return platform::run_process_with_stdin({"xclip", "-selection", "clipboard"}, text);
  }

  std::cerr << "Warning: Neither wl-copy nor xclip is available to copy transcript.\n";
  return false;
#endif
}

bool send_notification(const std::string& message)
{
#ifdef ASRYX_TESTING
  if (auto hook = testing::notification_hook()) {
    return hook(message);
  }
#endif

#if defined(__APPLE__)
  if (platform::command_exists("osascript")) {
    const std::string script = "display notification \"" + escape_applescript(message) +
                               "\" with title \"" + std::string(constants::app_name) + "\"";
    return platform::run_process_blocking({"osascript", "-e", script});
  }

  return false;
#else
  if (platform::command_exists("notify-send")) {
    return platform::run_process_blocking(
        {"notify-send", std::string(constants::app_name), message});
  }

  return false;
#endif
}

} // namespace engine
