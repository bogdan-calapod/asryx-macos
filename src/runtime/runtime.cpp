#include "runtime/runtime.hpp"

#include "config/config.hpp"
#include "engine/engine.hpp"
#include "model/model.hpp"
#include "platform/fs.hpp"
#include "platform/process.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <unistd.h>

namespace runtime {

namespace {

std::filesystem::path lock_dir_for(const std::filesystem::path& runtime_dir)
{
  return runtime_dir / "lock";
}

bool read_pid_file(const std::filesystem::path& path, pid_t& pid)
{
  std::ifstream file(path);
  return static_cast<bool>(file >> pid);
}

bool acquire_lock(const std::filesystem::path& runtime_dir)
{
  std::filesystem::create_directories(runtime_dir);
  auto lock_dir = lock_dir_for(runtime_dir);

  std::error_code ec;
  if (std::filesystem::create_directory(lock_dir, ec)) {
    std::ofstream(lock_dir / "pid") << getpid() << "\n";
    return true;
  }

  pid_t pid = 0;
  if (!read_pid_file(lock_dir / "pid", pid) || !platform::is_process_running(pid)) {
    platform::safe_delete_directory(lock_dir);
    if (std::filesystem::create_directory(lock_dir, ec)) {
      std::ofstream(lock_dir / "pid") << getpid() << "\n";
      return true;
    }
  }
  return false;
}

void release_lock(const std::filesystem::path& runtime_dir)
{
  platform::safe_delete_directory(lock_dir_for(runtime_dir));
}

void clean_stale_payload(const std::filesystem::path& runtime_dir)
{
  platform::safe_delete_file(runtime_dir / "rec.pid");
  platform::safe_delete_file(runtime_dir / "rec.wav");
  platform::safe_delete_file(runtime_dir / "rec.err");
  platform::safe_delete_file(runtime_dir / "state");
  platform::safe_delete_file(runtime_dir / "out.txt");
}

std::string read_state_file(const std::filesystem::path& runtime_dir)
{
  auto state_file = runtime_dir / "state";
  if (!std::filesystem::exists(state_file)) {
    return "";
  }
  std::ifstream sf(state_file);
  std::string state;
  if (sf >> state) {
    return state;
  }
  return "";
}

} // namespace

std::string get_status()
{
  auto runtime_dir = platform::get_runtime_directory();
  auto state = read_state_file(runtime_dir);
  if (state == "transcribing") {
    return state;
  }

  auto rec_pid_path = runtime_dir / "rec.pid";
  pid_t pid = 0;
  if (read_pid_file(rec_pid_path, pid)) {
    if (platform::is_process_running(pid)) {
      return state.empty() ? "recording" : state;
    }
  }

  return "idle";
}

void toggle()
{
  auto runtime_dir = platform::get_runtime_directory();
  if (!acquire_lock(runtime_dir)) {
    // Exit quietly if lock is held by concurrent toggle
    return;
  }

  try {
    auto rec_pid_path = runtime_dir / "rec.pid";
    bool is_recording = false;
    pid_t rec_pid = 0;

    if (read_pid_file(rec_pid_path, rec_pid) && platform::is_process_running(rec_pid)) {
      is_recording = true;
    }

    if (!is_recording) {
      // First call: Start recording
      clean_stale_payload(runtime_dir);

      auto cfg = config::load_config();
      auto model_path = model::get_model_path(cfg.model);
      if (!std::filesystem::exists(model_path)) {
        std::cerr << "error: model '" << cfg.model
                  << "' is not installed.\nInstall it with: asryx --model install " << cfg.model
                  << "\n";
        release_lock(runtime_dir);
        std::exit(1);
      }

      auto wav_path = runtime_dir / "rec.wav";
      auto err_path = runtime_dir / "rec.err";

      pid_t pid = engine::start_recording(wav_path.string(), err_path.string());
      {
        std::ofstream pfile(rec_pid_path);
        pfile << pid << "\n";
      }
      {
        std::ofstream sfile(runtime_dir / "state");
        sfile << "recording\n";
      }

      engine::send_notification("recording…");
      release_lock(runtime_dir);
    }
    else {
      // Second call: Stop, Transcribe, Copy, Notify, Clean
      {
        std::ofstream sfile(runtime_dir / "state");
        sfile << "transcribing\n";
      }

      engine::stop_recording(rec_pid);

      auto cfg = config::load_config();
      auto model_path = model::get_model_path(cfg.model);
      auto wav_path = runtime_dir / "rec.wav";
      auto out_prefix = runtime_dir / "out";
      auto out_txt = runtime_dir / "out.txt";

      bool success = engine::run_whisper(model_path, wav_path.string(), out_prefix.string());
      if (success && std::filesystem::exists(out_txt)) {
        std::ifstream tf(out_txt);
        std::string transcript((std::istreambuf_iterator<char>(tf)),
                               std::istreambuf_iterator<char>());
        // Trim whitespace
        while (!transcript.empty() && std::isspace(static_cast<unsigned char>(transcript.back()))) {
          transcript.pop_back();
        }
        size_t start = 0;
        while (start < transcript.size() &&
               std::isspace(static_cast<unsigned char>(transcript[start])))
        {
          start++;
        }
        transcript = transcript.substr(start);

        engine::copy_to_clipboard(transcript);
        engine::send_notification("Copied to clipboard");
      }
      else {
        engine::send_notification("Transcription failed");
      }

      clean_stale_payload(runtime_dir);
      release_lock(runtime_dir);
    }
  }
  catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
    clean_stale_payload(runtime_dir);
    release_lock(runtime_dir);
    std::exit(1);
  }
}

} // namespace runtime
