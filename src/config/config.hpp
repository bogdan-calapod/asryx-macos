#ifndef ASRYX_CONFIG_CONFIG_HPP
#define ASRYX_CONFIG_CONFIG_HPP

#include "constants/constants.hpp"

#include <string>

namespace config {

struct Config
{
  std::string model = std::string(constants::config::default_model);
  std::string language = std::string(constants::config::default_language);
  std::string pipe_to;
  bool mic_only_fallback = false;
  // Speaker diarization. Defaults match the recommended sherpa-onnx pipeline.
  bool diarize_enabled = true;
  std::string diarize_segmentation_model =
      std::string(constants::diarization::default_segmentation_model);
  std::string diarize_embedding_model =
      std::string(constants::diarization::default_embedding_model);
  float diarize_threshold = constants::diarization::default_threshold;
  // Output format selection. "dialogue" is human-readable; "json" emits the
  // structured schema (asryx_schema_version + meta + segments).
  std::string clipboard_format = std::string(constants::config::default_clipboard_format);
  std::string pipe_to_format = std::string(constants::config::default_pipe_to_format);
};

Config load_config();
void save_config(const Config& cfg);

} // namespace config

#endif // ASRYX_CONFIG_CONFIG_HPP
