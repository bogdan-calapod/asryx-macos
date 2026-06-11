#ifndef ASRYX_ENGINE_FORMAT_HPP
#define ASRYX_ENGINE_FORMAT_HPP

#include "engine/diarize.hpp"
#include "engine/engine.hpp"

#include <string>
#include <vector>

namespace engine::format {

// Stable JSON schema produced by asryx for downstream pipelines.
inline constexpr int kSchemaVersion = 1;

// A labeled segment ready for rendering. The speaker token is one of:
//   - "you"          (microphone-attributed)
//   - "speaker_N"    (sherpa cluster N, zero-indexed in the raw data, rendered
//                     as 1-indexed in dialogue form)
struct LabeledSegment
{
  std::string speaker;
  double t0 = 0.0;
  double t1 = 0.0;
  std::string text;
};

// Tag a list of mic transcript segments uniformly as "you".
std::vector<LabeledSegment> label_mic_segments(const std::vector<TranscriptSegment>& mic);

// For each system-audio transcript segment, assign the sherpa speaker whose
// time range maximally overlaps it. Drops segments with zero overlap (the
// conservative voice-activity gate from the locked plan: if sherpa reports
// no speech in a time range, we assume whisper hallucinated on silence and
// discard the text).
std::vector<LabeledSegment> label_sys_segments(const std::vector<TranscriptSegment>& sys,
                                               const std::vector<DiarizationSegment>& diarization);

// Concatenate two labeled lists and sort by t0.
std::vector<LabeledSegment> merge_by_time(std::vector<LabeledSegment> a,
                                          std::vector<LabeledSegment> b);

// "You: ...\nSpeaker 1: ...\n..." human-readable form (one line per turn).
// Adjacent segments from the same speaker are joined into a single line.
std::string render_dialogue(const std::vector<LabeledSegment>& segments);

// Plain text (concatenation of all segment text, no speaker labels). Used as
// a fallback when there's only one stream or when speaker labelling is
// disabled.
std::string render_plain(const std::vector<LabeledSegment>& segments);

// JSON-encoded payload. Schema is documented in README; key fields are
// asryx_schema_version, meta, segments.
struct JsonMeta
{
  std::string language;
  std::string started_at_iso8601;
  double duration_seconds = 0.0;
  int num_speakers_detected = 0;
  std::string model;
  std::string diarization_segmentation_model;
  std::string diarization_embedding_model;
};

std::string render_json(const std::vector<LabeledSegment>& segments, const JsonMeta& meta);

} // namespace engine::format

#endif // ASRYX_ENGINE_FORMAT_HPP
