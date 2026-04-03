#include "install/install.hpp"

#include "config/config.hpp"
#include "platform/fs.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace install {

void install_app()
{
  auto bin_dir = platform::get_home_relative_path(".local/bin");
  std::filesystem::create_directories(bin_dir);

  std::filesystem::path self;
  try {
    self = std::filesystem::read_symlink("/proc/self/exe");
  }
  catch (const std::filesystem::filesystem_error& e) {
    throw std::runtime_error("Could not resolve the current executable through /proc/self/exe: " +
                             std::string(e.what()));
  }
  auto dest = bin_dir / "asryx";

  std::cout << "Installing asryx binary to " << dest.string() << "\n";
  std::filesystem::copy_file(self, dest, std::filesystem::copy_options::overwrite_existing);
  std::filesystem::permissions(dest,
                               std::filesystem::perms::owner_exec |
                                   std::filesystem::perms::owner_read |
                                   std::filesystem::perms::owner_write,
                               std::filesystem::perm_options::add);

  auto share_dir = platform::get_home_relative_path(".local/share/asryx");
  std::filesystem::create_directories(share_dir);

  auto conf_path = platform::get_home_relative_path(".asryx.conf");
  if (!std::filesystem::exists(conf_path)) {
    config::Config cfg;
    config::save_config(cfg);
  }

  std::cout << "Installation complete.\n";
}

void uninstall_app()
{
  std::cout << "Uninstalling asryx...\n";

  auto bin_dir = platform::get_home_relative_path(".local/bin");
  auto asryx_bin = bin_dir / "asryx";
  auto whisper_cli = bin_dir / "whisper-cli";
  auto whisper_cpp = platform::get_home_relative_path(".local/opt/whisper.cpp");
  auto share_dir = platform::get_home_relative_path(".local/share/asryx");
  auto cache_dir = platform::get_home_relative_path(".cache/asryx");
  auto conf_path = platform::get_home_relative_path(".asryx.conf");
  auto runtime_dir = platform::get_runtime_directory();

  platform::safe_delete_file(asryx_bin);
  platform::safe_delete_file(whisper_cli);
  platform::safe_delete_file(conf_path);
  platform::safe_delete_directory(whisper_cpp);
  platform::safe_delete_directory(share_dir);
  platform::safe_delete_directory(cache_dir);
  platform::safe_delete_directory(runtime_dir);

  std::cout << "Uninstalled successfully.\n";
}

} // namespace install
