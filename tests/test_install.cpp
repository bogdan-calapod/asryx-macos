#include "install/install.hpp"
#include "platform/fs.hpp"
#include "tests/test_helpers.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

void run_test_install()
{
  auto bin_dir = platform::get_home_relative_path(".local/bin");
  std::filesystem::create_directories(bin_dir);
  auto whisper_cli_path = bin_dir / "whisper-cli";
  {
    std::ofstream whisper_cli(whisper_cli_path);
    whisper_cli << "#!/bin/sh\n"
                   "while [ \"$#\" -gt 0 ]; do\n"
                   "  if [ \"$1\" = \"-of\" ]; then shift; out=\"$1\"; fi\n"
                   "  shift\n"
                   "done\n"
                   "printf 'Test transcription.\\n' > \"${out}.txt\"\n";
  }
  std::filesystem::permissions(whisper_cli_path, std::filesystem::perms::owner_exec,
                               std::filesystem::perm_options::add);

  install::install_app();

  auto bin_path = platform::get_home_relative_path(".local/bin/asryx");
  ASSERT_TRUE(std::filesystem::exists(bin_path));

  ASSERT_TRUE(std::filesystem::exists(whisper_cli_path));

  std::cout << "test_install passed\n";
}
