#include "model/model.hpp"
#include "runtime/runtime.hpp"
#include "tests/test_helpers.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

void run_test_runtime()
{
  // Setup model so it passes existence check
  model::use_model("base.en");
  std::filesystem::create_directories(
      std::filesystem::path(model::get_model_path("base.en")).parent_path());
  std::ofstream model_file(model::get_model_path("base.en"));
  model_file << "fake model content";

  ASSERT_EQ(runtime::get_status(), std::string("idle"));

  // Toggle 1: start recording
  runtime::toggle();
  ASSERT_EQ(runtime::get_status(), std::string("recording"));

  // Toggle 2: stop & transcribe
  runtime::toggle();
  ASSERT_EQ(runtime::get_status(), std::string("idle"));

  std::cout << "test_runtime passed\n";
}
