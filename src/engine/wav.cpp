#include "engine/wav.hpp"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
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

constexpr std::uint16_t wav_pcm = 1;
constexpr std::uint16_t wav_extensible = 65534;
constexpr float pcm16_scale = 32768.0F;

bool chunk_is(const std::vector<std::uint8_t>& bytes, size_t offset, const char* id)
{
  return offset + 4 <= bytes.size() && std::memcmp(bytes.data() + offset, id, 4) == 0;
}

std::uint16_t read_u16_le(const std::vector<std::uint8_t>& bytes, size_t offset)
{
  if (offset + 2 > bytes.size()) {
    throw std::runtime_error("invalid wav header");
  }
  return static_cast<std::uint16_t>(bytes[offset]) |
         static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[offset + 1]) << 8U);
}

std::uint32_t read_u32_le(const std::vector<std::uint8_t>& bytes, size_t offset)
{
  if (offset + 4 > bytes.size()) {
    throw std::runtime_error("invalid wav header");
  }
  return static_cast<std::uint32_t>(bytes[offset]) |
         (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U) |
         (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U) |
         (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U);
}

std::vector<std::uint8_t> read_file(const std::string& path)
{
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("failed to open wav file: " + path);
  }
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>{}};
}

void validate_wav_format(std::uint16_t audio_format, std::uint16_t channels,
                         std::uint32_t sample_rate, std::uint16_t bits_per_sample)
{
  const bool supported_format = audio_format == wav_pcm || audio_format == wav_extensible;
  if (!supported_format) {
    throw std::runtime_error("unsupported wav format: expected PCM");
  }
  if (channels != 1 || sample_rate != WHISPER_SAMPLE_RATE || bits_per_sample != 16) {
    throw std::runtime_error("unsupported wav format: expected 16 kHz mono s16 PCM");
  }
}

std::vector<float> decode_pcm16(const std::vector<std::uint8_t>& bytes, size_t data_offset,
                                size_t data_size)
{
  data_size -= data_size % 2U;
  std::vector<float> samples;
  samples.reserve(data_size / 2U);
  for (size_t i = data_offset; i < data_offset + data_size; i += 2) {
    const auto high = static_cast<std::uint16_t>(bytes[i + 1]);
    const auto low = static_cast<std::uint16_t>(bytes[i]);
    const auto raw = static_cast<std::uint16_t>(low | static_cast<std::uint16_t>(high << 8U));
    const auto sample = static_cast<std::int16_t>(raw);
    samples.push_back(static_cast<float>(sample) / pcm16_scale);
  }
  return samples;
}

} // namespace

std::vector<float> read_pcm16_wav(const std::string& path)
{
  const auto bytes = read_file(path);

  if (bytes.size() < 44 || !chunk_is(bytes, 0, "RIFF") || !chunk_is(bytes, 8, "WAVE")) {
    throw std::runtime_error("unsupported wav file: expected RIFF/WAVE");
  }

  bool found_fmt = false;
  bool found_data = false;

  std::uint16_t audio_format = 0;
  std::uint16_t channels = 0;
  std::uint32_t sample_rate = 0;
  std::uint16_t bits_per_sample = 0;

  size_t data_offset = 0;
  size_t data_size = 0;

  size_t offset = 12;
  while (offset + 8 <= bytes.size()) {
    const std::uint32_t declared_size = read_u32_le(bytes, offset + 4);
    const size_t chunk_data = offset + 8;
    const size_t remaining = bytes.size() - chunk_data;

    if (chunk_is(bytes, offset, "fmt ")) {
      if (declared_size > remaining || declared_size < 16) {
        throw std::runtime_error("invalid wav fmt chunk");
      }
      audio_format = read_u16_le(bytes, chunk_data);
      channels = read_u16_le(bytes, chunk_data + 2);
      sample_rate = read_u32_le(bytes, chunk_data + 4);
      bits_per_sample = read_u16_le(bytes, chunk_data + 14);
      found_fmt = true;
    }
    else if (chunk_is(bytes, offset, "data")) {
      data_offset = chunk_data;
      if (declared_size == 0 || declared_size > remaining) {
        data_size = remaining;
      }
      else {
        data_size = static_cast<size_t>(declared_size);
      }
      found_data = true;
      break;
    }

    if (declared_size > remaining) {
      break;
    }
    offset = chunk_data + declared_size + (declared_size % 2U);
  }

  if (!found_fmt || !found_data) {
    throw std::runtime_error("invalid wav file: missing fmt or data chunk");
  }

  validate_wav_format(audio_format, channels, sample_rate, bits_per_sample);
  return decode_pcm16(bytes, data_offset, data_size);
}

} // namespace engine
