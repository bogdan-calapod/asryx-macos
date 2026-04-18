#include "platform/process.hpp"
#include "tests/test_helpers.hpp"

#include <iostream>
#include <string>
#include <unistd.h>

void run_test_process()
{
  ASSERT_TRUE(platform::command_exists("sh"));
  ASSERT_TRUE(platform::run_process_blocking({"sh", "-c", "exit 0"}));
  ASSERT_FALSE(platform::run_process_blocking({"sh", "-c", "exit 7"}));
  ASSERT_TRUE(platform::run_process_with_stdin({"sh", "-c", "cat >/dev/null"}, "hello"));
  ASSERT_TRUE(platform::is_process_running(getpid()));

  std::cout << "test_process passed\n";
}
