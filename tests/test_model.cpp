#include "model/model.hpp"
#include "tests/test_helpers.hpp"

#include <algorithm>
#include <iostream>

void run_test_model()
{
  const auto& supported = model::get_supported_models();
  ASSERT_TRUE(!supported.empty());
  ASSERT_TRUE(std::find(supported.begin(), supported.end(), "base.en") != supported.end());

  std::string path = model::get_model_path("base.en");
  ASSERT_TRUE(!path.empty());

  // Initially it's not installed in test home
  ASSERT_FALSE(model::is_model_installed("base.en"));

  std::cout << "test_model passed\n";
}
