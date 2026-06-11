#ifndef ASRYX_ENGINE_WAV_HPP
#define ASRYX_ENGINE_WAV_HPP

#include <string>
#include <vector>

namespace engine {

// Parse a RIFF/WAVE file at `path` and return its PCM samples decoded to
// floats in [-1.0, 1.0]. Throws std::runtime_error if the file is missing
// or its format is not 16 kHz mono signed-16 PCM as required by whisper.
std::vector<float> read_pcm16_wav(const std::string& path);

} // namespace engine

#endif // ASRYX_ENGINE_WAV_HPP
