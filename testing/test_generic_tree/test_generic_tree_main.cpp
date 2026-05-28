// test_generic_tree_main.cpp — Custom main for GTest (optional)

#include <gtest/gtest.h>
#include <iostream>

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();

  const ::testing::UnitTest* unit_test = ::testing::UnitTest::GetInstance();

  std::cout << "\n============ test_generic_tree summary ============\n";
  std::cout << "Total tests run: " << unit_test->total_test_count()
            << std::endl;
  std::cout << "Tests passed:    " << unit_test->successful_test_count()
            << std::endl;
  std::cout << "Tests failed:    " << unit_test->failed_test_count()
            << std::endl;
  std::cout << "====================================================="
            << std::endl;

  return result;
}
