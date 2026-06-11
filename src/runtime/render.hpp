#ifndef ASRYX_RUNTIME_RENDER_HPP
#define ASRYX_RUNTIME_RENDER_HPP

#include "config/config.hpp"
#include "engine/diarize.hpp"
#include "engine/engine.hpp"
#include "engine/format.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace runtime::render {

// Bundles everything stop_and_transcribe produces from the two WAV files.
// Empty `merged` means no usable speech anywhere.
struct AssembledTranscript
{
  std::vector<engine::format::LabeledSegment> merged;
  engine::format::JsonMeta meta;
  bool diarized = false;
};

// Run both whisper passes (mic + sys), optionally run sherpa-onnx diarization
// on the sys stream, label and merge the segments. Encapsulates all the
// per-stream decision logic and meta construction so stop_and_transcribe
// stays narrow.
AssembledTranscript assemble_transcript(const std::filesystem::path& runtime_dir,
                                        const config::Config& cfg, const std::string& language,
                                        const std::string& model_path);

// True when every segment is attributed to the microphone speaker ("you").
// In that case the runtime renders the output without speaker prefixes so
// pure-dictation use cases don't get a "You:" prefix on every line.
bool only_self_speaker(const std::vector<engine::format::LabeledSegment>& segments);

// UTC ISO-8601 timestamp for the current wall clock, suitable for the JSON
// "started_at" meta field.
std::string iso8601_now();

// Pick the right renderer for the given format string. Supported values are
// "dialogue" (default), "plain", and "json". Falls back to dialogue for any
// unrecognized value, and to plain when only_self_speaker(segments).
std::string render_for_format(const std::vector<engine::format::LabeledSegment>& segments,
                              const engine::format::JsonMeta& meta, const std::string& format);

} // namespace runtime::render

#endif // ASRYX_RUNTIME_RENDER_HPP
