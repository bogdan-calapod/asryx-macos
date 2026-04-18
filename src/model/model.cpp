#include "model/model.hpp"

#include "config/config.hpp"
#include "platform/fs.hpp"
#include "platform/process.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace model {

namespace {

std::string trim(std::string value)
{
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
    value.pop_back();
  }

  size_t start = 0;
  while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0) {
    ++start;
  }

  return value.substr(start);
}

std::string read_pin_file(const std::filesystem::path& path)
{
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("missing whisper.cpp pin: " + path.string());
  }

  std::string value;
  std::getline(file, value);
  value = trim(value);

  if (value.empty()) {
    throw std::runtime_error("empty whisper.cpp pin: " + path.string());
  }

  return value;
}

std::string whisper_cpp_sha()
{
  const char* env_sha = std::getenv("ASRYX_WHISPER_SHA");
  if (env_sha != nullptr && *env_sha != '\0') {
    return trim(env_sha);
  }

  const auto installed_pin =
      platform::get_home_relative_path(".local/share/asryx/versions/whisper-cpp-sha");
  if (std::filesystem::exists(installed_pin)) {
    return read_pin_file(installed_pin);
  }

  const char* source_root = std::getenv("ASRYX_SOURCE_ROOT");
  if (source_root != nullptr && *source_root != '\0') {
    const auto source_pin = std::filesystem::path(source_root) / "versions/whisper-cpp-sha";
    if (std::filesystem::exists(source_pin)) {
      return read_pin_file(source_pin);
    }
  }

#ifdef ASRYX_SOURCE_DIR
  const auto compiled_pin = std::filesystem::path(ASRYX_SOURCE_DIR) / "versions/whisper-cpp-sha";
  if (std::filesystem::exists(compiled_pin)) {
    return read_pin_file(compiled_pin);
  }
#endif

  throw std::runtime_error("could not resolve whisper.cpp pin");
}

} // namespace

const std::vector<std::string>& get_supported_models()
{
  static const std::vector<std::string> models = {"tiny.en",  "tiny",     "base.en",   "base",
                                                  "small.en", "small",    "medium.en", "medium",
                                                  "large-v1", "large-v2", "large-v3",  "large"};
  return models;
}

std::string get_model_path(const std::string& name)
{
  return (platform::get_home_relative_path(".local/share/asryx") / ("ggml-" + name + ".bin"))
      .string();
}

bool is_model_installed(const std::string& name)
{
  return std::filesystem::exists(get_model_path(name));
}

void list_models()
{
  auto cfg = config::load_config();
  std::cout << "Available models:\n";

  for (const auto& m : get_supported_models()) {
    const bool installed = is_model_installed(m);
    const bool active = (m == cfg.model);

    std::cout << "  " << (active ? "* " : "  ") << m << (installed ? " (installed)" : "")
              << (active ? " (active)" : "") << "\n";
  }
}

void install_model(const std::string& name)
{
  const auto& supported = get_supported_models();

  if (std::find(supported.begin(), supported.end(), name) == supported.end()) {
    throw std::runtime_error("unsupported model size: " + name);
  }

  const auto path = get_model_path(name);
  if (std::filesystem::exists(path)) {
    std::cout << "Model " << name << " is already installed.\n";
    return;
  }

  const auto dir = platform::get_home_relative_path(".local/share/asryx");
  std::filesystem::create_directories(dir);

  std::cout << "Downloading model " << name << "...\n";

  const std::string url = std::string("https://huggingface.co/ggerganov/whisper.cpp/resolve/") +
                          whisper_cpp_sha() + "/ggml-" + name + ".bin";

  bool success = false;
  if (platform::command_exists("curl")) {
    success = platform::run_process_foreground({"curl", "-L", "-f", "-o", path, url});
  }
  else if (platform::command_exists("wget")) {
    success = platform::run_process_foreground({"wget", "-O", path, url});
  }
  else {
    throw std::runtime_error("neither curl nor wget is available to download models");
  }

  if (!success) {
    if (std::filesystem::exists(path)) {
      platform::safe_delete_file(path);
    }

    throw std::runtime_error("failed to download model " + name);
  }

  std::cout << "Model " << name << " installed successfully.\n";
}

void use_model(const std::string& name)
{
  const auto& supported = get_supported_models();

  if (std::find(supported.begin(), supported.end(), name) == supported.end()) {
    throw std::runtime_error("unsupported model size: " + name);
  }

  auto cfg = config::load_config();
  cfg.model = name;
  config::save_config(cfg);

  std::cout << "Using model: " << name << "\n";
}

void uninstall_model(const std::string& name)
{
  const auto path = get_model_path(name);

  if (!std::filesystem::exists(path)) {
    std::cout << "Model " << name << " is not installed.\n";
    return;
  }

  platform::safe_delete_file(path);
  std::cout << "Model " << name << " uninstalled successfully.\n";
}

} // namespace model