#include "engine/engine.hpp"

#include "platform/fs.hpp"
#include "platform/process.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace engine {

pid_t start_recording(const std::string& wav_path, const std::string& err_path)
{
  std::vector<std::string> args;
  if (platform::command_exists("pw-record")) {
    args = {"pw-record", "--format=s16", "--rate=16000", "--channels=1", wav_path};
  }
  else if (platform::command_exists("arecord")) {
    args = {"arecord", "-f", "S16_LE", "-c", "1", "-r", "16000", wav_path};
  }
  else {
    throw std::runtime_error("No recorder tool found (need pw-record or arecord)");
  }

  pid_t pid = platform::spawn_process_background(args, err_path);
  if (pid == -1) {
    throw std::runtime_error("Failed to start recorder process");
  }
  return pid;
}

bool stop_recording(pid_t pid)
{
  if (pid <= 0) {
    return false;
  }
  platform::stop_process(pid, 2); // SIGINT
  platform::wait_process(pid);
  return true;
}

bool run_whisper(const std::string& model_path,
                 const std::string& wav_path,
                 const std::string& out_prefix)
{
  auto whisper_cli_path = platform::get_home_relative_path(".local/bin/whisper-cli");
  if (!std::filesystem::exists(whisper_cli_path)) {
    throw std::runtime_error("whisper-cli is not installed at " + whisper_cli_path.string() +
                             ". Run the top-level install script.");
  }

  std::vector<std::string> args =
      {whisper_cli_path.string(), "-m", model_path, "-f", wav_path, "-of", out_prefix, "-otxt"};
  return platform::run_process_blocking(args);
}

bool copy_to_clipboard(const std::string& text)
{
  if (platform::command_exists("wl-copy")) {
    return platform::run_process_with_stdin({"wl-copy"}, text);
  }
  if (platform::command_exists("xclip")) {
    return platform::run_process_with_stdin({"xclip", "-selection", "clipboard"}, text);
  }
  std::cerr << "Warning: Neither wl-copy nor xclip is available to copy transcript.\n";
  return false;
}

bool send_notification(const std::string& message)
{
  if (platform::command_exists("notify-send")) {
    return platform::run_process_blocking({"notify-send", "asryx", message});
  }
  return false;
}

} // namespace engine
