#include "install/install.hpp"
#include "platform/fs.hpp"
#include "tests/test_helpers.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

void run_test_uninstall()
{
  auto bin_dir = platform::get_home_relative_path(".local/bin");
  std::filesystem::create_directories(bin_dir);
  auto whisper_cli_path = bin_dir / "whisper-cli";
  std::ofstream whisper_cli(whisper_cli_path);
  whisper_cli << "#!/bin/sh\nexit 0\n";

  install::install_app();

  // Verify paths exist
  auto share_dir = platform::get_home_relative_path(".local/share/asryx");
  auto conf_path = platform::get_home_relative_path(".asryx.conf");
  ASSERT_TRUE(std::filesystem::exists(share_dir));
  ASSERT_TRUE(std::filesystem::exists(conf_path));
  ASSERT_TRUE(std::filesystem::exists(whisper_cli_path));

  // Uninstall
  install::uninstall_app();

  // Verify paths are gone
  ASSERT_FALSE(std::filesystem::exists(share_dir));
  ASSERT_FALSE(std::filesystem::exists(conf_path));
  ASSERT_FALSE(std::filesystem::exists(whisper_cli_path));

  std::cout << "test_uninstall passed\n";
}
