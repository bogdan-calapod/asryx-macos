#include "platform/process.hpp"
#include "tests/test_helpers.hpp"

#include <iostream>

void run_test_process()
{
  ASSERT_TRUE(platform::command_exists("sleep"));
  ASSERT_FALSE(platform::command_exists("nonexistent-command-xyz"));

  bool success = platform::run_process_blocking({"sleep", "0"});
  ASSERT_TRUE(success);

  std::cout << "test_process passed\n";
}
