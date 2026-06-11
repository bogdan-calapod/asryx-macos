#ifndef ASRYX_PLATFORM_AUDIO_HPP
#define ASRYX_PLATFORM_AUDIO_HPP

#include <string>
#include <sys/types.h>

namespace platform::audio {

enum class CaptureMode
{
  // record only the default microphone via AVAudioEngine
  Mic,
  // record default mic + system audio via ScreenCaptureKit, mixed in-process
  All,
};

// True if the running process has Screen Recording permission. Fast, no prompt.
bool has_screen_recording_permission();

// Opens System Settings > Privacy & Security > Screen Recording.
// Non-blocking. Best effort.
void open_screen_recording_settings();

// Re-exec asryx as a child capture process. Returns the child pid (>0) on
// success, or -1 on failure. The child writes 16 kHz mono signed-16 WAV to
// wav_path. err_path receives the child's stderr.
// SIGINT to the child finalizes the WAV header and exits cleanly.
pid_t spawn_capture(CaptureMode mode, const std::string& wav_path, const std::string& err_path);

// Entry point for the re-exec'd child process. Blocks until SIGINT/SIGTERM.
// Returns the process exit code.
int run_capture_child(CaptureMode mode, const std::string& wav_path);

} // namespace platform::audio

#endif // ASRYX_PLATFORM_AUDIO_HPP
