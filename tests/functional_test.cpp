#include "functional.h"

#include <gtest/gtest.h>

TEST(FunctionalTest, GetCommandCodeValidString) {
    EXPECT_EQ(ioteye::GetCommandCode("ad"), 197);
}

TEST(FunctionalTest, GetCommandCodeInvalidString) {
    EXPECT_EQ(ioteye::GetCommandCode("adasdasdasd"), 0);
}