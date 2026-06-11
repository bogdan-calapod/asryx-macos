#include "engine/format.hpp"

#include "constants/constants.hpp"

#include <algorithm>
#include <iomanip>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace engine::format {

namespace {

std::string trim_copy(std::string_view s)
{
  size_t start = 0;
  size_t end = s.size();
  while (start < end && std::isspace(static_cast<unsigned char>(s[start])) != 0) {
    ++start;
  }
  while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])) != 0) {
    --end;
  }
  return std::string(s.substr(start, end - start));
}

std::string speaker_token_for(int speaker_id)
{
  return "speaker_" + std::to_string(speaker_id);
}

std::string speaker_display_for(const std::string& token)
{
  if (token == "you") {
    return "You";
  }
  if (token.rfind("speaker_", 0) == 0) {
    // speaker_N -> "Speaker (N+1)" so users see 1-indexed labels.
    try {
      int id = std::stoi(token.substr(std::string_view{"speaker_"}.size()));
      return "Speaker " + std::to_string(id + 1);
    }
    catch (const std::exception&) {
      // fallthrough to raw token
    }
  }
  return token;
}

// Compute overlap of [a0, a1] with [b0, b1]; returns 0 when disjoint.
double overlap_seconds(double a0, double a1, double b0, double b1)
{
  const double lo = std::max(a0, b0);
  const double hi = std::min(a1, b1);
  return hi > lo ? hi - lo : 0.0;
}

void append_json_escaped(std::string& out, std::string_view s)
{
  out.push_back('"');
  for (char ch : s) {
    switch (ch) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\b':
        out += "\\b";
        break;
      case '\f':
        out += "\\f";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (static_cast<unsigned char>(ch) < 0x20U) {
          char buf[8];
          // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
          std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned int>(ch) & 0xFFU);
          out += buf;
        }
        else {
          out.push_back(ch);
        }
        break;
    }
  }
  out.push_back('"');
}

std::string format_number(double value, int precision)
{
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(precision) << value;
  return oss.str();
}

} // namespace

std::vector<LabeledSegment> label_mic_segments(const std::vector<TranscriptSegment>& mic)
{
  std::vector<LabeledSegment> out;
  out.reserve(mic.size());
  for (const auto& seg : mic) {
    std::string text = trim_copy(seg.text);
    if (text.empty()) {
      continue;
    }
    out.push_back(
        {std::string(constants::diarization::self_speaker_token), seg.t0, seg.t1, std::move(text)});
  }
  return out;
}

std::vector<LabeledSegment> label_sys_segments(const std::vector<TranscriptSegment>& sys,
                                               const std::vector<DiarizationSegment>& diarization)
{
  std::vector<LabeledSegment> out;
  out.reserve(sys.size());

  for (const auto& seg : sys) {
    std::string text = trim_copy(seg.text);
    if (text.empty()) {
      continue;
    }

    int best_speaker = -1;
    double best_overlap = 0.0;
    for (const auto& d : diarization) {
      const double o = overlap_seconds(seg.t0, seg.t1, d.t0, d.t1);
      if (o > best_overlap) {
        best_overlap = o;
        best_speaker = d.speaker_id;
      }
    }

    // Conservative VAD gate: if sherpa reported zero speech in this range,
    // the whisper segment is almost certainly a hallucination. Drop it.
    if (best_speaker < 0 || best_overlap <= 0.0) {
      continue;
    }

    out.push_back({speaker_token_for(best_speaker), seg.t0, seg.t1, std::move(text)});
  }

  return out;
}

std::vector<LabeledSegment> merge_by_time(std::vector<LabeledSegment> a,
                                          std::vector<LabeledSegment> b)
{
  std::vector<LabeledSegment> out;
  out.reserve(a.size() + b.size());
  for (auto& seg : a) {
    out.push_back(std::move(seg));
  }
  for (auto& seg : b) {
    out.push_back(std::move(seg));
  }
  std::sort(out.begin(), out.end(),
            [](const LabeledSegment& x, const LabeledSegment& y) { return x.t0 < y.t0; });
  return out;
}

std::string render_dialogue(const std::vector<LabeledSegment>& segments)
{
  std::string out;
  std::string last_speaker;
  for (const auto& seg : segments) {
    std::string text = trim_copy(seg.text);
    if (text.empty()) {
      continue;
    }
    if (seg.speaker != last_speaker) {
      if (!out.empty()) {
        out.push_back('\n');
      }
      out += speaker_display_for(seg.speaker);
      out += ": ";
      last_speaker = seg.speaker;
    }
    else {
      out.push_back(' ');
    }
    out += text;
  }
  return out;
}

std::string render_plain(const std::vector<LabeledSegment>& segments)
{
  std::string out;
  for (const auto& seg : segments) {
    std::string text = trim_copy(seg.text);
    if (text.empty()) {
      continue;
    }
    if (!out.empty() && out.back() != ' ') {
      out.push_back(' ');
    }
    out += text;
  }
  return out;
}

std::string render_json(const std::vector<LabeledSegment>& segments, const JsonMeta& meta)
{
  // We tally num_speakers_detected from the speaker tokens actually present
  // in the output (after filtering) rather than trusting the caller.
  std::set<std::string> distinct_speakers;
  for (const auto& seg : segments) {
    distinct_speakers.insert(seg.speaker);
  }
  const int speakers_in_output = static_cast<int>(distinct_speakers.size());

  std::string out;
  out += "{";
  out += "\"asryx_schema_version\":";
  out += std::to_string(kSchemaVersion);

  out += ",\"meta\":{";
  out += "\"language\":";
  append_json_escaped(out, meta.language);
  out += ",\"started_at\":";
  append_json_escaped(out, meta.started_at_iso8601);
  out += ",\"duration_seconds\":";
  out += format_number(meta.duration_seconds, 3);
  out += ",\"num_speakers_detected\":";
  out += std::to_string(std::max(meta.num_speakers_detected, speakers_in_output));
  out += ",\"model\":";
  append_json_escaped(out, meta.model);
  out += ",\"diarization_segmentation_model\":";
  append_json_escaped(out, meta.diarization_segmentation_model);
  out += ",\"diarization_embedding_model\":";
  append_json_escaped(out, meta.diarization_embedding_model);
  out += "}";

  out += ",\"segments\":[";
  bool first = true;
  for (const auto& seg : segments) {
    if (!first) {
      out += ",";
    }
    first = false;
    out += "{\"speaker\":";
    append_json_escaped(out, seg.speaker);
    out += ",\"t0\":";
    out += format_number(seg.t0, 3);
    out += ",\"t1\":";
    out += format_number(seg.t1, 3);
    out += ",\"text\":";
    append_json_escaped(out, seg.text);
    out += "}";
  }
  out += "]}";

  return out;
}

} // namespace engine::format
