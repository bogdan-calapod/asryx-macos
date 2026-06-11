#include "config/config.hpp"

#include "constants/constants.hpp"
#include "platform/fs.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace config {

Config load_config()
{
  Config cfg;
  auto path = platform::get_home_relative_path(std::string(constants::config::file_name));
  std::ifstream file(path);
  if (!file.is_open()) {
    return cfg;
  }

  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }

    auto pos = line.find('=');
    if (pos == std::string::npos) {
      continue;
    }

    std::string key = line.substr(0, pos);
    std::string val = line.substr(pos + 1);

    if (key == constants::config::model_key) {
      cfg.model = val;
    }
    else if (key == constants::config::language_key) {
      cfg.language = val;
    }
    else if (key == constants::config::pipe_to_key) {
      cfg.pipe_to = val;
    }
    else if (key == constants::config::mic_only_fallback_key) {
      cfg.mic_only_fallback = (val == "1" || val == "true" || val == "yes" || val == "on");
    }
    else if (key == constants::config::diarize_enabled_key) {
      cfg.diarize_enabled = !(val == "0" || val == "false" || val == "no" || val == "off");
    }
    else if (key == constants::config::diarize_segmentation_model_key) {
      cfg.diarize_segmentation_model = val;
    }
    else if (key == constants::config::diarize_embedding_model_key) {
      cfg.diarize_embedding_model = val;
    }
    else if (key == constants::config::diarize_threshold_key) {
      try {
        cfg.diarize_threshold = std::stof(val);
      }
      catch (const std::exception&) {
        // ignore malformed values; keep default
      }
    }
    else if (key == constants::config::clipboard_format_key) {
      cfg.clipboard_format = val;
    }
    else if (key == constants::config::pipe_to_format_key) {
      cfg.pipe_to_format = val;
    }
  }

  return cfg;
}

void save_config(const Config& cfg)
{
  auto path = platform::get_home_relative_path(std::string(constants::config::file_name));
  auto tmp_path = platform::get_home_relative_path(std::string(constants::config::file_name) +
                                                   std::string(constants::config::temp_suffix));

  {
    std::ofstream file(tmp_path);
    if (!file.is_open()) {
      throw std::runtime_error("Failed to open temporary config file for writing: " +
                               tmp_path.string());
    }

    file << constants::config::model_key << "=" << cfg.model << "\n";
    file << constants::config::language_key << "=" << cfg.language << "\n";
    file << constants::config::pipe_to_key << "=" << cfg.pipe_to << "\n";
    file << constants::config::mic_only_fallback_key << "="
         << (cfg.mic_only_fallback ? "true" : "false") << "\n";
    file << constants::config::diarize_enabled_key << "="
         << (cfg.diarize_enabled ? "true" : "false") << "\n";
    file << constants::config::diarize_segmentation_model_key << "="
         << cfg.diarize_segmentation_model << "\n";
    file << constants::config::diarize_embedding_model_key << "=" << cfg.diarize_embedding_model
         << "\n";
    file << constants::config::diarize_threshold_key << "=" << cfg.diarize_threshold << "\n";
    file << constants::config::clipboard_format_key << "=" << cfg.clipboard_format << "\n";
    file << constants::config::pipe_to_format_key << "=" << cfg.pipe_to_format << "\n";
  }

  std::filesystem::rename(tmp_path, path);
}

} // namespace config
