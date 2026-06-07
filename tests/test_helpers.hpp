#ifndef ASRYX_TESTS_TEST_HELPERS_HPP
#  define ASRYX_TESTS_TEST_HELPERS_HPP

#  include <iostream>
#  include <stdexcept>

#  define ASSERT_TRUE(expr)                                                                        \
    do {                                                                                           \
      if (!(expr)) {                                                                               \
        std::cerr << "Assertion failed: " << #expr << " at " << __FILE__ << ":" << __LINE__        \
                  << "\n";                                                                         \
        throw std::runtime_error("Assertion failed");                                              \
      }                                                                                            \
    } while (0)

#  define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))

#  define ASSERT_EQ(val1, val2)                                                                    \
    do {                                                                                           \
      if ((val1) != (val2)) {                                                                      \
        std::cerr << "Assertion failed: " << #val1 << " == " << #val2 << " (Got: " << (val1)       \
                  << ", Expected: " << (val2) << ") at " << __FILE__ << ":" << __LINE__ << "\n";   \
        throw std::runtime_error("Assertion failed");                                              \
      }                                                                                            \
    } while (0)

#endif // ASRYX_TESTS_TEST_HELPERS_HPP

void run_test_config();
void run_test_app();
void run_test_model();
void run_test_lock();
void run_test_process();
void run_test_runtime();
