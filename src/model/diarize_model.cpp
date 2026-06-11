#include "config/config.hpp"
#include "constants/constants.hpp"
#include "model/model.hpp"
#include "platform/fs.hpp"
#include "platform/process.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace model::diarize {

namespace {

std::filesystem::path diarize_models_dir()
{
  return platform::get_home_relative_path(std::string(constants::diarization::diarize_dir_rel));
}

const ModelInfo* find_model(const std::vector<ModelInfo>& list, const std::string& name)
{
  for (const auto& m : list) {
    if (m.name == name) {
      return &m;
    }
  }
  return nullptr;
}

bool file_nonempty(const std::filesystem::path& path)
{
  return std::filesystem::exists(path) && std::filesystem::is_regular_file(path) &&
         !std::filesystem::is_empty(path);
}

void download_to(const std::string& url, const std::filesystem::path& dest)
{
  std::filesystem::create_directories(dest.parent_path());
  const auto tmp = dest.parent_path() / ("." + dest.filename().string() + ".tmp");
  platform::safe_delete_file(tmp);
  const bool ok = platform::run_process_blocking({"curl", "-fsSL", "-o", tmp.string(), url});
  if (!ok || !std::filesystem::exists(tmp)) {
    platform::safe_delete_file(tmp);
    throw std::runtime_error("diarize: failed to download " + url);
  }
  std::filesystem::rename(tmp, dest);
}

void extract_tarball_model(const std::filesystem::path& tarball,
                           const std::filesystem::path& target)
{
  const auto staging = target.parent_path() / (target.filename().string() + ".extract");
  platform::safe_delete_directory(staging);
  std::filesystem::create_directories(staging);
  const bool ok =
      platform::run_process_blocking({"tar", "-xjf", tarball.string(), "-C", staging.string()});
  if (!ok) {
    platform::safe_delete_directory(staging);
    throw std::runtime_error("diarize: failed to extract " + tarball.string());
  }
  // Sherpa tarballs contain a single directory with model.onnx inside.
  std::filesystem::path model_onnx;
  for (const auto& entry : std::filesystem::recursive_directory_iterator(staging)) {
    if (entry.is_regular_file() && entry.path().filename() == "model.onnx") {
      model_onnx = entry.path();
      break;
    }
  }
  if (model_onnx.empty()) {
    platform::safe_delete_directory(staging);
    throw std::runtime_error("diarize: tarball did not contain model.onnx: " + tarball.string());
  }
  std::filesystem::rename(model_onnx, target);
  platform::safe_delete_directory(staging);
  platform::safe_delete_file(tarball);
}

void install_one(const ModelInfo& info)
{
  const auto dir = diarize_models_dir();
  std::filesystem::create_directories(dir);
  const auto target = dir / info.file_name;

  if (file_nonempty(target)) {
    std::cout << "Diarize model " << info.name << " is already installed.\n";
    return;
  }

  std::cout << "Downloading diarize " << info.kind << " model: " << info.name << "\n";

  if (info.is_tarball) {
    const auto tarball = dir / (info.file_name + ".tar.bz2");
    download_to(info.download_url, tarball);
    extract_tarball_model(tarball, target);
  }
  else {
    download_to(info.download_url, target);
  }

  if (!file_nonempty(target)) {
    throw std::runtime_error("diarize: install did not produce " + target.string());
  }
  std::cout << "Diarize model " << info.name << " installed.\n";
}

} // namespace

const std::vector<ModelInfo>& get_supported_segmentation_models()
{
  static const std::vector<ModelInfo> models = {
      {"pyannote-segmentation-3-0", "segmentation", "pyannote-segmentation-3-0.onnx", true,
       "https://github.com/k2-fsa/sherpa-onnx/releases/download/"
       "speaker-segmentation-models/sherpa-onnx-pyannote-segmentation-3-0.tar.bz2"},
  };
  return models;
}

const std::vector<ModelInfo>& get_supported_embedding_models()
{
  static const std::vector<ModelInfo> models = {
      {"wespeaker-voxceleb-resnet34", "embedding", "wespeaker_en_voxceleb_resnet34.onnx",               false,
       "https://github.com/k2-fsa/sherpa-onnx/releases/download/"
       "speaker-recongition-models/wespeaker_en_voxceleb_resnet34.onnx"                           },
      {"3dspeaker-eres2net",          "embedding", "3dspeaker_speech_eres2net_sv_en_voxceleb_16k.onnx",
       false,                                                                                                  "https://github.com/k2-fsa/sherpa-onnx/releases/download/"
       "speaker-recongition-models/3dspeaker_speech_eres2net_sv_en_voxceleb_16k.onnx"},
  };
  return models;
}

std::string get_segmentation_model_path(const std::string& name)
{
  const auto* info = find_model(get_supported_segmentation_models(), name);
  if (info == nullptr) {
    return {};
  }
  return (diarize_models_dir() / info->file_name).string();
}

std::string get_embedding_model_path(const std::string& name)
{
  const auto* info = find_model(get_supported_embedding_models(), name);
  if (info == nullptr) {
    return {};
  }
  return (diarize_models_dir() / info->file_name).string();
}

bool is_segmentation_model_installed(const std::string& name)
{
  const auto path = get_segmentation_model_path(name);
  return !path.empty() && file_nonempty(std::filesystem::path(path));
}

bool is_embedding_model_installed(const std::string& name)
{
  const auto path = get_embedding_model_path(name);
  return !path.empty() && file_nonempty(std::filesystem::path(path));
}

void list_models()
{
  const auto cfg = config::load_config();
  std::cout << "Available diarization segmentation models:\n";
  for (const auto& m : get_supported_segmentation_models()) {
    const bool installed = is_segmentation_model_installed(m.name);
    const bool active = (cfg.diarize_segmentation_model == m.name);
    std::cout << "  " << (active ? "* " : "  ") << m.name << (installed ? " (installed)" : "")
              << (active ? " (active)" : "") << "\n";
  }
  std::cout << "Available diarization embedding models:\n";
  for (const auto& m : get_supported_embedding_models()) {
    const bool installed = is_embedding_model_installed(m.name);
    const bool active = (cfg.diarize_embedding_model == m.name);
    std::cout << "  " << (active ? "* " : "  ") << m.name << (installed ? " (installed)" : "")
              << (active ? " (active)" : "") << "\n";
  }
}

void install_model(const std::string& name)
{
  if (const auto* info = find_model(get_supported_segmentation_models(), name)) {
    install_one(*info);
    return;
  }
  if (const auto* info = find_model(get_supported_embedding_models(), name)) {
    install_one(*info);
    return;
  }
  throw std::runtime_error("unknown diarize model: " + name);
}

void use_model(const std::string& name)
{
  auto cfg = config::load_config();
  if (find_model(get_supported_segmentation_models(), name) != nullptr) {
    if (!is_segmentation_model_installed(name)) {
      throw std::runtime_error("diarize: segmentation model '" + name +
                               "' is not installed; run: asryx --diarize install " + name);
    }
    cfg.diarize_segmentation_model = name;
    config::save_config(cfg);
    std::cout << "Using diarize segmentation model: " << name << "\n";
    return;
  }
  if (find_model(get_supported_embedding_models(), name) != nullptr) {
    if (!is_embedding_model_installed(name)) {
      throw std::runtime_error("diarize: embedding model '" + name +
                               "' is not installed; run: asryx --diarize install " + name);
    }
    cfg.diarize_embedding_model = name;
    config::save_config(cfg);
    std::cout << "Using diarize embedding model: " << name << "\n";
    return;
  }
  throw std::runtime_error("unknown diarize model: " + name);
}

void uninstall_model(const std::string& name)
{
  const ModelInfo* info = find_model(get_supported_segmentation_models(), name);
  if (info == nullptr) {
    info = find_model(get_supported_embedding_models(), name);
  }
  if (info == nullptr) {
    throw std::runtime_error("unknown diarize model: " + name);
  }
  const auto cfg = config::load_config();
  if ((info->kind == "segmentation" && cfg.diarize_segmentation_model == name) ||
      (info->kind == "embedding" && cfg.diarize_embedding_model == name))
  {
    throw std::runtime_error("cannot uninstall active diarize model '" + name +
                             "'; switch first with: asryx --diarize use <other>");
  }
  const auto path = diarize_models_dir() / info->file_name;
  if (!file_nonempty(path)) {
    std::cout << "Diarize model " << name << " is not installed.\n";
    return;
  }
  platform::safe_delete_file(path);
  std::cout << "Diarize model " << name << " uninstalled.\n";
}

} // namespace model::diarize
