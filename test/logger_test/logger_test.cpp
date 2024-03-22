#include <gtest/gtest.h>
#include "logger.h"
#include <stdio.h>

#include <fcntl.h>
#include <unistd.h>

#include <fstream>

using namespace std;
using namespace testing;

#define LOGGER_MERGE_STDERR_TO_STDOUT

class OutputRedirection
{
    int stdoutBackup, stderrBackup, stdoutFile, stderrFile;
    inline static const char constStdoutFileName[] = "stdout_XXXXXX";
    inline static const char constStderrFileName[] = "stderr_XXXXXX";
    char stdoutFileName[sizeof(constStdoutFileName)];
    char stderrFileName[sizeof(constStderrFileName)];

public:
    void SetUp()
    {
        strcpy(stdoutFileName, constStdoutFileName);
        strcpy(stderrFileName, constStderrFileName);
        fflush(stdout);
        fflush(stderr);
        stdoutBackup = dup(1);
        stderrBackup = dup(2);

        stdoutFile = mkstemp(stdoutFileName);
        dup2(stdoutFile, 1);
#ifdef LOGGER_MERGE_STDERR_TO_STDOUT
        dup2(stdoutFile, 2);
#else
        stderrFile = mkstemp(stderrFileName);
        dup2(stderrFile, 2);
        close(stderrFile);
#endif // LOGGER_MERGE_STDERR_TO_STDOUT
        close(stdoutFile);
    }

    void TearDown()
    {
        fflush(stdout);
        dup2(stdoutBackup, 1);
        close(stdoutBackup);

        fflush(stderr);
        dup2(stderrBackup, 2);
        close(stderrBackup);

        if (UnitTest::GetInstance()->current_test_info()->result()->Failed())
        {
            ifstream f(stdoutFileName);
            if (f.is_open())
                cout << f.rdbuf();
        }

        remove(stdoutFileName);
#ifdef LOGGER_MERGE_STDERR_TO_STDOUT
        remove(stderrFileName);
#endif // LOGGER_MERGE_STDERR_TO_STDOUT
    }
};

class LoggerUnitTests : public ::testing::Test
{
protected:

    OutputRedirection outputRedirection{};

    virtual void SetUp() override
    {
        outputRedirection.SetUp();
        logger_init();
    }

    virtual void TearDown() override
    {
        outputRedirection.TearDown();
    }
};

TEST_F(LoggerUnitTests, Test1) {
    fprintf(stdout, "C stdout\n");
    fprintf(stderr, "C stderr\n");
    cout << "C++ stdout\n";
    cerr << "C++ stderr\n";
    FAIL();
}

// TEST_F(LoggerUnitTests, Test2) {
//     // fprintf(stdout, "worldOut");
//     // fprintf(stderr, "worldErr");
// }

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
