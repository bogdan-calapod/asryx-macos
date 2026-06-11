#include "runtime/render.hpp"

#include "constants/constants.hpp"
#include "model/model.hpp"

#include <array>
#include <chrono>
#include <ctime>
#include <iostream>
#include <stdexcept>
#include <string>

namespace runtime::render {

bool only_self_speaker(const std::vector<engine::format::LabeledSegment>& segments)
{
  const std::string self(constants::diarization::self_speaker_token);
  for (const auto& seg : segments) {
    if (seg.speaker != self) {
      return false;
    }
  }
  return !segments.empty();
}

std::string iso8601_now()
{
  const auto now = std::chrono::system_clock::now();
  const std::time_t t = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
  ::gmtime_r(&t, &tm);
  std::array<char, 32> buf{};
  std::strftime(buf.data(), buf.size(), "%Y-%m-%dT%H:%M:%SZ", &tm);
  return std::string(buf.data());
}

std::string render_for_format(const std::vector<engine::format::LabeledSegment>& segments,
                              const engine::format::JsonMeta& meta, const std::string& format)
{
  if (format == constants::config::format_json) {
    return engine::format::render_json(segments, meta);
  }
  if (format == constants::config::format_plain) {
    return engine::format::render_plain(segments);
  }
  // Dialogue rendering with "You: ..." prefixes is only informative when there
  // are multiple speakers. For mic-only / single-speaker recordings (typical
  // dictation use case) we drop the labels and emit plain text.
  if (only_self_speaker(segments)) {
    return engine::format::render_plain(segments);
  }
  return engine::format::render_dialogue(segments);
}

AssembledTranscript assemble_transcript(const std::filesystem::path& runtime_dir,
                                        const config::Config& cfg, const std::string& language,
                                        const std::string& model_path)
{
  using engine::DiarizationSegment;
  using engine::TranscriptSegment;
  using engine::format::LabeledSegment;

  const auto mic_wav = runtime_dir / std::string(constants::runtime::recorder_mic_wav_file);
  const auto sys_wav = runtime_dir / std::string(constants::runtime::recorder_sys_wav_file);

  std::vector<TranscriptSegment> mic_segments =
      engine::transcribe_with_segments(model_path, mic_wav.string(), language);
  std::vector<TranscriptSegment> sys_segments;
  std::vector<DiarizationSegment> diarization;
  bool diarized = false;

  if (std::filesystem::exists(sys_wav) && std::filesystem::file_size(sys_wav) > 44) {
    sys_segments = engine::transcribe_with_segments(model_path, sys_wav.string(), language);
    if (cfg.diarize_enabled) {
      const auto seg_path =
          model::diarize::get_segmentation_model_path(cfg.diarize_segmentation_model);
      const auto emb_path = model::diarize::get_embedding_model_path(cfg.diarize_embedding_model);
      if (!seg_path.empty() && !emb_path.empty() &&
          model::diarize::is_segmentation_model_installed(cfg.diarize_segmentation_model) &&
          model::diarize::is_embedding_model_installed(cfg.diarize_embedding_model))
      {
        try {
          diarization =
              engine::diarize(sys_wav.string(), seg_path, emb_path, cfg.diarize_threshold);
          diarized = true;
        }
        catch (const std::exception& e) {
          std::cerr << "diarize: " << e.what() << " (falling back to unlabeled sys text)\n";
        }
      }
    }
  }

  auto labeled_mic = engine::format::label_mic_segments(mic_segments);
  std::vector<LabeledSegment> labeled_sys;
  if (diarized) {
    labeled_sys = engine::format::label_sys_segments(sys_segments, diarization);
  }
  else if (!sys_segments.empty()) {
    // Diarization disabled or unavailable: tag sys segments as a single
    // generic "speaker_0" bucket so they show up labeled but unattributed.
    std::vector<DiarizationSegment> fallback;
    fallback.push_back({0.0, 1e9, 0});
    labeled_sys = engine::format::label_sys_segments(sys_segments, fallback);
  }

  AssembledTranscript out;
  out.merged = engine::format::merge_by_time(std::move(labeled_mic), std::move(labeled_sys));
  out.diarized = diarized;
  out.meta.language = language.empty() ? cfg.language : language;
  out.meta.started_at_iso8601 = iso8601_now();
  out.meta.duration_seconds = out.merged.empty() ? 0.0 : out.merged.back().t1;
  out.meta.model = cfg.model;
  out.meta.diarization_segmentation_model = diarized ? cfg.diarize_segmentation_model : "";
  out.meta.diarization_embedding_model = diarized ? cfg.diarize_embedding_model : "";
  return out;
}

} // namespace runtime::render
