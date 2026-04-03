#include "model/model.hpp"

#include "config/config.hpp"
#include "platform/fs.hpp"
#include "platform/process.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace model {

const std::vector<std::string>& get_supported_models()
{
  static const std::vector<std::string> models = {"tiny.en",
                                                  "tiny",
                                                  "base.en",
                                                  "base",
                                                  "small.en",
                                                  "small",
                                                  "medium.en",
                                                  "medium",
                                                  "large-v1",
                                                  "large-v2",
                                                  "large-v3",
                                                  "large"};
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
    bool installed = is_model_installed(m);
    bool active = (m == cfg.model);
    std::cout << "  " << (active ? "* " : "  ") << m << (installed ? " (installed)" : "")
              << (active ? " (active)" : "") << "\n";
  }
}

void install_model(const std::string& name)
{
  const auto& supported = get_supported_models();
  if (std::find(supported.begin(), supported.end(), name) == supported.end()) {
    throw std::runtime_error("Unsupported model size: " + name);
  }
  auto path = get_model_path(name);
  if (std::filesystem::exists(path)) {
    std::cout << "Model " << name << " is already installed.\n";
    return;
  }
  auto dir = platform::get_home_relative_path(".local/share/asryx");
  std::filesystem::create_directories(dir);

  std::cout << "Downloading model " << name << "...\n";
  std::string url =
      "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-" + name + ".bin";
  bool success = false;
  if (platform::command_exists("curl")) {
    success = platform::run_process_blocking({"curl", "-L", "-f", "-o", path, url});
  }
  else if (platform::command_exists("wget")) {
    success = platform::run_process_blocking({"wget", "-O", path, url});
  }
  else {
    throw std::runtime_error("Neither curl nor wget is available to download models.");
  }
  if (!success) {
    if (std::filesystem::exists(path)) {
      platform::safe_delete_file(path);
    }
    throw std::runtime_error("Failed to download model " + name);
  }
  std::cout << "Model " << name << " installed successfully.\n";
}

void use_model(const std::string& name)
{
  const auto& supported = get_supported_models();
  if (std::find(supported.begin(), supported.end(), name) == supported.end()) {
    throw std::runtime_error("Unsupported model size: " + name);
  }
  auto cfg = config::load_config();
  cfg.model = name;
  config::save_config(cfg);
  std::cout << "Using model: " << name << "\n";
}

void uninstall_model(const std::string& name)
{
  auto path = get_model_path(name);
  if (!std::filesystem::exists(path)) {
    std::cout << "Model " << name << " is not installed.\n";
    return;
  }
  platform::safe_delete_file(path);
  std::cout << "Model " << name << " uninstalled successfully.\n";
}

} // namespace model
