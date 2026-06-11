#include "engine/diarize.hpp"

#include "engine/wav.hpp"
#include "sherpa-onnx/c-api/c-api.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace engine {

#ifdef ASRYX_TESTING
namespace testing {

namespace {

DiarizeHook& slot()
{
  static DiarizeHook value = nullptr;
  return value;
}

} // namespace

void set_diarize_hook(DiarizeHook hook)
{
  slot() = hook;
}
DiarizeHook diarize_hook()
{
  return slot();
}

} // namespace testing
#endif

namespace {

constexpr uint32_t kRequiredSampleRate = 16000;

// Convert PCM16 mono samples in [-1.0, 1.0] floats. The engine's wav reader
// already does this for whisper; we reuse it here.
std::vector<float> load_wav_for_sherpa(const std::string& wav_path)
{
  if (!std::filesystem::exists(wav_path)) {
    throw std::runtime_error("diarize: wav file not found: " + wav_path);
  }
  // read_pcm16_wav validates the format (16 kHz mono signed-16). Sherpa
  // expects the same.
  return read_pcm16_wav(wav_path);
}

void validate_model_paths(const std::string& segmentation, const std::string& embedding)
{
  if (segmentation.empty() || !std::filesystem::exists(segmentation)) {
    throw std::runtime_error("diarize: segmentation model not found: " + segmentation);
  }
  if (embedding.empty() || !std::filesystem::exists(embedding)) {
    throw std::runtime_error("diarize: embedding model not found: " + embedding);
  }
}

} // namespace

std::vector<DiarizationSegment> diarize(const std::string& wav_path,
                                        const std::string& segmentation_model_path,
                                        const std::string& embedding_model_path, float threshold)
{
#ifdef ASRYX_TESTING
  if (auto hook = testing::diarize_hook()) {
    return hook(wav_path, segmentation_model_path, embedding_model_path, threshold);
  }
#endif

  validate_model_paths(segmentation_model_path, embedding_model_path);
  const auto samples = load_wav_for_sherpa(wav_path);
  if (samples.empty()) {
    return {};
  }

  SherpaOnnxOfflineSpeakerDiarizationConfig config{};
  std::memset(&config, 0, sizeof(config));
  config.segmentation.pyannote.model = segmentation_model_path.c_str();
  config.segmentation.num_threads = 1;
  config.embedding.model = embedding_model_path.c_str();
  config.embedding.num_threads = 1;
  // Threshold-based clustering. num_clusters = 0 means "infer K".
  config.clustering.num_clusters = 0;
  config.clustering.threshold = threshold;
  // Skip blink-of-an-eye segments and merge tiny gaps. Defaults from sherpa
  // examples for general conversation.
  config.min_duration_on = 0.3F;
  config.min_duration_off = 0.5F;

  const SherpaOnnxOfflineSpeakerDiarization* sd =
      SherpaOnnxCreateOfflineSpeakerDiarization(&config);
  if (sd == nullptr) {
    throw std::runtime_error(
        "diarize: failed to construct sherpa-onnx diarizer (model file invalid?)");
  }

  const int32_t expected_sr = SherpaOnnxOfflineSpeakerDiarizationGetSampleRate(sd);
  if (expected_sr != static_cast<int32_t>(kRequiredSampleRate)) {
    SherpaOnnxDestroyOfflineSpeakerDiarization(sd);
    throw std::runtime_error("diarize: sherpa expects sample rate " + std::to_string(expected_sr) +
                             " but asryx wav is 16000");
  }

  const SherpaOnnxOfflineSpeakerDiarizationResult* result =
      SherpaOnnxOfflineSpeakerDiarizationProcess(sd, samples.data(),
                                                 static_cast<int32_t>(samples.size()));
  if (result == nullptr) {
    SherpaOnnxDestroyOfflineSpeakerDiarization(sd);
    throw std::runtime_error("diarize: sherpa-onnx process call failed");
  }

  const int32_t n = SherpaOnnxOfflineSpeakerDiarizationResultGetNumSegments(result);
  const SherpaOnnxOfflineSpeakerDiarizationSegment* segs =
      SherpaOnnxOfflineSpeakerDiarizationResultSortByStartTime(result);

  std::vector<DiarizationSegment> out;
  out.reserve(static_cast<size_t>(n));
  for (int32_t i = 0; i < n; ++i) {
    DiarizationSegment s;
    s.t0 = static_cast<double>(segs[i].start);
    s.t1 = static_cast<double>(segs[i].end);
    s.speaker_id = segs[i].speaker;
    out.push_back(s);
  }

  SherpaOnnxOfflineSpeakerDiarizationDestroySegment(segs);
  SherpaOnnxOfflineSpeakerDiarizationDestroyResult(result);
  SherpaOnnxDestroyOfflineSpeakerDiarization(sd);

  return out;
}

} // namespace engine
