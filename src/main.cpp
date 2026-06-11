#include "app/app.hpp"

#include <string>
#include <vector>

#if defined(__APPLE__)
#  include "platform/audio.hpp"
#endif

int main(int argc, char* argv[])
{
  std::vector<std::string> args;
  if (argc > 1) {
    args.reserve(static_cast<size_t>(argc - 1));
  }
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }
#if defined(__APPLE__)
  if (args.size() == 4 && args[0] == "--internal-capture") {
    const auto& mode_str = args[1];
    const auto& mic_wav_path = args[2];
    const auto& sys_wav_path = args[3];
    platform::audio::CaptureMode mode =
        (mode_str == "all") ? platform::audio::CaptureMode::All : platform::audio::CaptureMode::Mic;
    return platform::audio::run_capture_child(mode, mic_wav_path, sys_wav_path);
  }
#endif

  return app::run(args);
}
