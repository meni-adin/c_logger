#include <gtest/gtest.h>
#include "logger.h"

using namespace std;
using namespace testing;

class LoggerUnitTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }
};

TEST_F(LoggerUnitTests, Test1) {
}

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
