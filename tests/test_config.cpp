#include "config/config.hpp"
#include "tests/test_helpers.hpp"

#include <iostream>

void run_test_config()
{
  config::Config cfg = config::load_config();
  ASSERT_EQ(cfg.model, std::string("base.en"));

  cfg.model = "small.en";
  config::save_config(cfg);

  config::Config cfg2 = config::load_config();
  ASSERT_EQ(cfg2.model, std::string("small.en"));

  std::cout << "test_config passed\n";
}
