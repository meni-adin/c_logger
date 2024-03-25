#include <gtest/gtest.h>
#include "logger.h"
#include <stdio.h>
#include "output_redirection.h"

using namespace ::testing;

class LoggerUnitTests : public ::testing::Test
{
protected:

    OutputRedirection outputRedirection{string{} +
        UnitTest::GetInstance()->current_test_suite()->name() +
        OutputRedirection::tempFileSep +
        UnitTest::GetInstance()->current_test_info()->name()};

    virtual void SetUp() override
    {
        outputRedirection.start_redirection();
        logger_init();
    }

    virtual void TearDown() override
    {
        outputRedirection.stop_redirection();

        if (UnitTest::GetInstance()->current_test_info()->result()->Failed())
        {
            outputRedirection.print();
        }
    }
};

TEST_F(LoggerUnitTests, Test1) {
    fprintf(stdout, "C stdout\n");
    fprintf(stderr, "C stderr\n");
    cout << "C++ stdout\n";
    cerr << "C++ stderr\n";
    FAIL();
}

TEST_F(LoggerUnitTests, Test2) {
    fprintf(stdout, "worldOut");
    fprintf(stderr, "worldErr");
}

// TEST_F(LoggerUnitTests, Test3) {
//     cout << "C++ stdout 01\n";
//     cerr << "C++ stderr 01\n";

//     outputRedirection.start_redirection();
//     cout << "C++ stdout file 02\n";
//     cerr << "C++ stderr file 02\n";

//     outputRedirection.stop_redirection();
//     cout << "C++ stdout 03\n";
//     cerr << "C++ stderr 03\n";

//     outputRedirection.start_redirection();
//     cout << "C++ stdout file 04\n";
//     cerr << "C++ stderr file 04\n";
//     FAIL();
// }

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
