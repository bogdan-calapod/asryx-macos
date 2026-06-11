#ifndef ASRYX_ENGINE_DIARIZE_HPP
#define ASRYX_ENGINE_DIARIZE_HPP

#include <string>
#include <vector>

namespace engine {

// A single speaker-attributed time range produced by sherpa-onnx.
//
// `speaker_id` is the cluster index assigned by the diarizer (`0`, `1`, `2`,
// ...). The id is stable within a single recording but not across recordings.
struct DiarizationSegment
{
  double t0 = 0.0;
  double t1 = 0.0;
  int speaker_id = 0;
};

// Run sherpa-onnx offline speaker diarization on a 16 kHz mono PCM WAV file.
// `segmentation_model_path` and `embedding_model_path` are absolute paths to
// the pyannote segmentation and speaker-embedding ONNX models respectively.
// `threshold` controls the agglomerative clustering distance threshold; higher
// values merge speakers more aggressively. Returns the diarization segments
// sorted by start time.
//
// Throws std::runtime_error if any model is missing, the WAV file is missing,
// the WAV sample rate is wrong, or sherpa fails to construct the pipeline.
std::vector<DiarizationSegment> diarize(const std::string& wav_path,
                                        const std::string& segmentation_model_path,
                                        const std::string& embedding_model_path, float threshold);

#ifdef ASRYX_TESTING
namespace testing {

using DiarizeHook = std::vector<DiarizationSegment> (*)(const std::string& wav_path,
                                                        const std::string& segmentation_model_path,
                                                        const std::string& embedding_model_path,
                                                        float threshold);

void set_diarize_hook(DiarizeHook hook);
DiarizeHook diarize_hook();

} // namespace testing
#endif

} // namespace engine

#endif // ASRYX_ENGINE_DIARIZE_HPP
