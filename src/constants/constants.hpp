#ifndef ASRYX_CONSTANTS_CONSTANTS_HPP
#define ASRYX_CONSTANTS_CONSTANTS_HPP

#include <filesystem>
#include <string_view>
#include <vector>

namespace constants {

inline constexpr std::string_view app_name = "asryx";

namespace config {

inline constexpr std::string_view file_name = ".asryx.conf";
inline constexpr std::string_view temp_suffix = ".tmp";
inline constexpr std::string_view model_key = "model";
inline constexpr std::string_view language_key = "language";
inline constexpr std::string_view pipe_to_key = "pipe_to";
inline constexpr std::string_view mic_only_fallback_key = "mic_only_fallback";
inline constexpr std::string_view diarize_enabled_key = "diarize";
inline constexpr std::string_view diarize_segmentation_model_key = "diarize_segmentation_model";
inline constexpr std::string_view diarize_embedding_model_key = "diarize_embedding_model";
inline constexpr std::string_view diarize_threshold_key = "diarize_threshold";
inline constexpr std::string_view clipboard_format_key = "clipboard_format";
inline constexpr std::string_view pipe_to_format_key = "pipe_to_format";
inline constexpr std::string_view default_clipboard_format = "dialogue";
inline constexpr std::string_view default_pipe_to_format = "json";
inline constexpr std::string_view format_dialogue = "dialogue";
inline constexpr std::string_view format_json = "json";
inline constexpr std::string_view format_plain = "plain";
inline constexpr std::string_view default_model = "base.en";
inline constexpr std::string_view default_language = "auto";
inline constexpr std::string_view auto_language = "auto";
inline constexpr std::string_view english_language = "en";

} // namespace config

namespace runtime {

inline constexpr std::string_view dir_name = "asryx";
inline constexpr std::string_view fallback_tmp_root = "/tmp";
inline constexpr std::string_view lock_dir_name = "lock";
inline constexpr std::string_view pid_file_name = "pid";
inline constexpr std::string_view recorder_pid_file = "rec.pid";
inline constexpr std::string_view recorder_mic_wav_file = "rec.mic.wav";
inline constexpr std::string_view recorder_sys_wav_file = "rec.sys.wav";
inline constexpr std::string_view recorder_error_file = "rec.err";
inline constexpr std::string_view error_log_file = "error.log";
inline constexpr std::string_view state_file = "state";
inline constexpr std::string_view idle_state = "idle";
inline constexpr std::string_view recording_state = "recording";
inline constexpr std::string_view transcribing_state = "transcribing";

} // namespace runtime

namespace notifications {

inline constexpr std::string_view transcription_copied = "transcription copied to clipboard.";
inline constexpr std::string_view pipe_copied = "piped and copied to clipboard.";
inline constexpr std::string_view pipe_failed = "copied to clipboard (pipe failed).";

} // namespace notifications

namespace diarization {

// Speaker token used in the JSON schema for microphone-attributed segments.
// The dialogue rendering displays this as "You".
inline constexpr std::string_view self_speaker_token = "you";

// Default sherpa-onnx clustering threshold for meeting use. Higher values
// merge speakers more aggressively (chosen for fewer-but-correct speakers).
inline constexpr float default_threshold = 0.7F;

// Defaults for the two ONNX models asryx fetches.
inline constexpr std::string_view default_segmentation_model = "pyannote-segmentation-3-0";
inline constexpr std::string_view default_embedding_model = "wespeaker-voxceleb-resnet34";

inline constexpr std::string_view diarize_dir_rel = ".local/share/asryx/diarize";

} // namespace diarization

namespace paths {

inline constexpr std::string_view local_bin_dir_rel = ".local/bin";
inline constexpr std::string_view asryx_bin_rel = ".local/bin/asryx";
inline constexpr std::string_view asryx_opt_dir_rel = ".local/opt/asryx";
inline constexpr std::string_view whisper_checkout_rel = ".local/opt/whisper.cpp";
inline constexpr std::string_view data_dir_rel = ".local/share/asryx";
inline constexpr std::string_view cache_dir_rel = ".cache/asryx";
inline constexpr std::string_view whisper_pin_rel = ".local/share/asryx/versions/whisper-cpp-sha";

inline std::filesystem::path config_path(const std::filesystem::path& home)
{
  return home / std::string(config::file_name);
}

inline std::filesystem::path asryx_bin_path(const std::filesystem::path& home)
{
  return home / std::string(asryx_bin_rel);
}

inline std::filesystem::path asryx_opt_dir(const std::filesystem::path& home)
{
  return home / std::string(asryx_opt_dir_rel);
}

inline std::filesystem::path whisper_checkout_dir(const std::filesystem::path& home)
{
  return home / std::string(whisper_checkout_rel);
}

inline std::filesystem::path data_dir(const std::filesystem::path& home)
{
  return home / std::string(data_dir_rel);
}

inline std::filesystem::path cache_dir(const std::filesystem::path& home)
{
  return home / std::string(cache_dir_rel);
}

inline std::vector<std::filesystem::path> owned_home_paths(const std::filesystem::path& home)
{
  return {asryx_bin_path(home), asryx_opt_dir(home), whisper_checkout_dir(home),
          data_dir(home),       cache_dir(home),     config_path(home)};
}

} // namespace paths

} // namespace constants

#endif // ASRYX_CONSTANTS_CONSTANTS_HPP
