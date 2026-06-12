#include "engine/engine.hpp"

#include "config/config.hpp"
#include "constants/constants.hpp"
#include "engine/wav.hpp"
#include "platform/process.hpp"

#if defined(__APPLE__)
#  include "platform/audio.hpp"
#endif

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <vector>

#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wshadow"
#endif

#include <whisper.h>

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

namespace engine {

namespace {

#ifdef ASRYX_TESTING
#  define ASRYX_TEST_HOOK(name, ...)                                                               \
    if (auto hook = testing::name()) {                                                             \
      return hook(__VA_ARGS__);                                                                    \
    }
#else
#  define ASRYX_TEST_HOOK(name, ...)
#endif

bool wait_until_recorder_exits(pid_t pid)
{
  for (int attempt = 0; attempt < 100; ++attempt) {
    int status = 0;
    const pid_t result = waitpid(pid, &status, WNOHANG);
    if (result == pid) {
      return true;
    }

    if (result == -1) {
      if (errno != ECHILD) {
        return false;
      }

      if (!platform::is_process_running(pid)) {
        return true;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  return false;
}

int thread_count()
{
  const auto detected = std::thread::hardware_concurrency();
  if (detected == 0) {
    return 4;
  }

  return static_cast<int>(std::min(4U, detected));
}

const char* whisper_language(whisper_context* ctx, const std::string& language)
{
  if (whisper_is_multilingual(ctx) == 0) {
    return constants::config::english_language.data();
  }

  if (language.empty() || language == constants::config::auto_language) {
    return nullptr;
  }

  return language.c_str();
}

struct WhisperDeleter
{
  void operator()(whisper_context* ctx) const
  {
    if (ctx != nullptr) {
      whisper_free(ctx);
    }
  }
};

} // namespace

pid_t start_recording(const std::string& mic_wav_path, const std::string& sys_wav_path,
                      const std::string& err_path)
{
  ASRYX_TEST_HOOK(start_recording_hook, mic_wav_path, sys_wav_path, err_path);

#if defined(__APPLE__)
  const auto cfg = config::load_config();
  auto mode = platform::audio::CaptureMode::All;
  // Triggers the native "asryx would like to record this computer's screen"
  // dialog on first invocation. Returns the current permission state, which
  // is typically false on first run even when the user is about to grant.
  if (!platform::audio::request_screen_recording_permission()) {
    if (!cfg.mic_only_fallback) {
      platform::audio::open_screen_recording_settings();
      throw std::runtime_error(
          "asryx needs Screen Recording permission. Grant access in the dialog (or "
          "in System Settings > Privacy & Security > Screen Recording) and run "
          "asryx again. Set mic_only_fallback=true in ~/.asryx.conf to capture mic "
          "only when permission is missing.");
    }
    mode = platform::audio::CaptureMode::Mic;
  }
  // sys_wav_path is forwarded unconditionally; the child writes to it only in
  // CaptureMode::All. Mic-only fallback discards it via the empty-string check
  // in spawn_capture.
  const std::string sys_arg = (mode == platform::audio::CaptureMode::All) ? sys_wav_path : "";
  pid_t pid = platform::audio::spawn_capture(mode, mic_wav_path, sys_arg, err_path);
#else
  // Linux only has microphone capture today; sys_wav_path is ignored.
  (void)sys_wav_path;
  std::vector<std::string> args;
  if (platform::command_exists("pw-record")) {
    args = {"pw-record", "--format=s16", "--rate=16000", "--channels=1", mic_wav_path};
  }
  else if (platform::command_exists("arecord")) {
    args = {"arecord", "-q", "-t", "wav", "-f", "S16_LE", "-c", "1", "-r", "16000", mic_wav_path};
  }
  else {
    throw std::runtime_error("No recorder tool found (need pw-record or arecord)");
  }
  pid_t pid = platform::spawn_process_background(args, err_path);
#endif
  if (pid == -1) {
    throw std::runtime_error("Failed to start recorder process");
  }
  return pid;
}

bool stop_recording(pid_t pid)
{
  ASRYX_TEST_HOOK(stop_recording_hook, pid);

  if (pid <= 0) {
    return false;
  }

  platform::stop_process(pid, SIGINT);
  if (wait_until_recorder_exits(pid)) {
    return true;
  }

  platform::stop_process(pid, SIGTERM);
  if (wait_until_recorder_exits(pid)) {
    return true;
  }

  platform::stop_process(pid, SIGKILL);
  return wait_until_recorder_exits(pid);
}

namespace {

void run_whisper_full(whisper_context* ctx, const std::vector<float>& samples,
                      const std::string& language, bool want_timestamps)
{
  const char* language_arg = whisper_language(ctx, language);

  whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
  params.n_threads = thread_count();
  params.print_progress = false;
  params.print_realtime = false;
  params.print_timestamps = false;
  params.no_timestamps = !want_timestamps;
  params.suppress_blank = true;
  params.suppress_nst = true;
  params.language = language_arg;
  params.detect_language = false;

  // Hallucination suppression for silent / near-silent audio (whisper's
  // infamous "Hello what's up" / "Thanks for watching" confabulations).
  // no_speech_thold: segments whose no-speech probability exceeds this are
  // treated as silence (0.6 default -> 0.8, matching stock OpenAI whisper).
  // logprob_thold: segments whose mean token log-probability falls below
  // this are considered unreliable (-1.0 default -> -0.5, stricter).
  // temperature_inc enables the fallback re-decode loop; without it the two
  // thresholds above are checked but never acted upon.
  params.no_speech_thold = 0.8F;
  params.logprob_thold = -0.5F;
  params.temperature_inc = 0.2F;

  if (whisper_full(ctx, params, samples.data(), static_cast<int>(samples.size())) != 0) {
    throw std::runtime_error("whisper transcription failed");
  }
}

std::unique_ptr<whisper_context, WhisperDeleter> load_whisper_context(const std::string& model_path)
{
  if (!std::filesystem::exists(model_path)) {
    throw std::runtime_error("model file does not exist: " + model_path);
  }
  whisper_context_params context_params = whisper_context_default_params();
  std::unique_ptr<whisper_context, WhisperDeleter> ctx(
      whisper_init_from_file_with_params(model_path.c_str(), context_params));
  if (ctx == nullptr) {
    throw std::runtime_error("failed to initialize whisper model: " + model_path);
  }
  return ctx;
}

} // namespace

std::string transcribe(const std::string& model_path, const std::string& wav_path,
                       const std::string& language)
{
  ASRYX_TEST_HOOK(transcribe_hook, model_path, wav_path, language);

  auto samples = read_pcm16_wav(wav_path);
  auto ctx = load_whisper_context(model_path);
  run_whisper_full(ctx.get(), samples, language, /*want_timestamps=*/false);

  std::string output;
  const int segments = whisper_full_n_segments(ctx.get());
  for (int i = 0; i < segments; ++i) {
    const char* text = whisper_full_get_segment_text(ctx.get(), i);
    if (text != nullptr) {
      output += text;
    }
  }
  return output;
}

std::vector<TranscriptSegment> transcribe_with_segments(const std::string& model_path,
                                                        const std::string& wav_path,
                                                        const std::string& language)
{
  ASRYX_TEST_HOOK(transcribe_with_segments_hook, model_path, wav_path, language);

  if (!std::filesystem::exists(wav_path)) {
    return {};
  }

  auto samples = read_pcm16_wav(wav_path);
  if (samples.empty()) {
    return {};
  }

  auto ctx = load_whisper_context(model_path);
  run_whisper_full(ctx.get(), samples, language, /*want_timestamps=*/true);

  std::vector<TranscriptSegment> output;
  const int n = whisper_full_n_segments(ctx.get());
  output.reserve(static_cast<size_t>(n));

  for (int i = 0; i < n; ++i) {
    const char* text = whisper_full_get_segment_text(ctx.get(), i);
    if (text == nullptr) {
      continue;
    }
    TranscriptSegment seg;
    // whisper returns t0/t1 in centiseconds.
    seg.t0 = static_cast<double>(whisper_full_get_segment_t0(ctx.get(), i)) / 100.0;
    seg.t1 = static_cast<double>(whisper_full_get_segment_t1(ctx.get(), i)) / 100.0;
    seg.text = text;
    output.push_back(std::move(seg));
  }
  return output;
}

} // namespace engine