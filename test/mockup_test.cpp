#include <gtest/gtest.h>

TEST(test1, test2) {
	float res = 3.0;
  ASSERT_NEAR(res, 3.0, 1.0e-11);
}
