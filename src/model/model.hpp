#ifndef ASRYX_MODEL_MODEL_HPP
#define ASRYX_MODEL_MODEL_HPP

#include <string>
#include <vector>

namespace config {
struct Config;
} // namespace config

namespace model {

const std::vector<std::string>& get_supported_models();
const std::vector<std::string>& get_supported_languages();
std::string get_model_path(const std::string& name);
bool is_model_installed(const std::string& name);
bool is_supported_language(const std::string& language);
bool is_english_only_model(const std::string& name);
void validate_config(const config::Config& cfg);
std::string transcription_language_for(const config::Config& cfg);
void list_models();
void install_model(const std::string& name);
void use_model(const std::string& name);
void use_language(const std::string& language);
void uninstall_model(const std::string& name);

// Speaker diarization model management. Parallel surface to the whisper model
// commands above. The diarization pipeline uses two models: a segmentation
// model and a speaker-embedding model.
namespace diarize {

struct ModelInfo
{
  std::string name;
  // Either "segmentation" or "embedding"
  std::string kind;
  // Filename on disk under ~/.local/share/asryx/diarize/
  std::string file_name;
  // Tarball flag: when true, the asset is a .tar.bz2 that contains a
  // model.onnx file; the installer extracts it and renames to file_name.
  bool is_tarball = false;
  std::string download_url;
};

const std::vector<ModelInfo>& get_supported_segmentation_models();
const std::vector<ModelInfo>& get_supported_embedding_models();

std::string get_segmentation_model_path(const std::string& name);
std::string get_embedding_model_path(const std::string& name);

bool is_segmentation_model_installed(const std::string& name);
bool is_embedding_model_installed(const std::string& name);

void list_models();
void install_model(const std::string& name);
void use_model(const std::string& name);
void uninstall_model(const std::string& name);

} // namespace diarize

} // namespace model

#endif // ASRYX_MODEL_MODEL_HPP
