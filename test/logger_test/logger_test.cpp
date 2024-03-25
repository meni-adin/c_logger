#include <gtest/gtest.h>
#include "logger.h"
#include <stdio.h>

#if defined(__linux__) || defined(__APPLE__)
 #include <fcntl.h>
 #include <unistd.h>
#elif defined(_WIN32)
 #include <io.h>
 #define dup _dup
 #define dup2 _dup2
 #define fileno _fileno
#endif // OS

#include <fstream>

using namespace std;
using namespace testing;


class OutputRedirection
{
public:
    static inline const string tempFileSep{"_"};
    static inline const string tempFileExtension = ".log";

private:
    bool separateStderr, keepTempFiles;

    string tempFilePrefix;

    int stdoutFDBackup, stderrFDBackup;
    int stdoutFD, stderrFD, stdoutAndStderrFD;
    string stdoutTempFileName, stderrTempFileName, stdoutAndStderrTempFileName;
    FILE *stdoutTempFile, *stderrTempFile, *stdoutAndStderrTempFile;
    static inline const string creatingTempFileMode{"w"};
    static constexpr ios_base::openmode readingTempFileMode = ios::in;

public:
    OutputRedirection(const string tempFilePrefix, bool separateStderr=false, bool keepTempFiles=false) :
        tempFilePrefix{tempFilePrefix}, separateStderr{separateStderr}, keepTempFiles{keepTempFiles}
    {
        if (separateStderr)
        {
            stdoutTempFileName = tempFilePrefix + tempFileSep + "stdout" + tempFileExtension;
            stderrTempFileName = tempFilePrefix + tempFileSep + "stderr" + tempFileExtension;

            stdoutTempFile = fopen(stdoutTempFileName.c_str(), creatingTempFileMode.c_str());
            stderrTempFile = fopen(stderrTempFileName.c_str(), creatingTempFileMode.c_str());

            stdoutFD = fileno(stdoutTempFile);
            stderrFD = fileno(stderrTempFile);
        }
        else
        {
            stdoutAndStderrTempFileName = tempFilePrefix + tempFileSep + "stdoutAndStderr" + tempFileSep + tempFileExtension;
            stdoutAndStderrTempFile = fopen(stdoutAndStderrTempFileName.c_str(), creatingTempFileMode.c_str());
            stdoutAndStderrFD = fileno(stdoutAndStderrTempFile);
        }
        if (!temp_files_are_open()) // TODO: add print from errno
            throw runtime_error("Can't open temp files");

        stdoutFDBackup = dup(1);
        stderrFDBackup = dup(2);
    }

    ~OutputRedirection()
    {
        if (separateStderr)
        {
            fclose(stdoutTempFile);
            fclose(stderrTempFile);
        }
        else
        {
            fclose(stdoutAndStderrTempFile);
        }

        if (!keepTempFiles)
        {
            if(separateStderr)
            {
                remove(stdoutTempFileName.c_str());
                remove(stderrTempFileName.c_str());
            }
            else
            {
                remove(stdoutAndStderrTempFileName.c_str());
            }
        }
    }

    void start_redirection()
    {
        fflush(stdout);
        fflush(stderr);

        if (separateStderr)
        {
            dup2(stdoutFD, 1);
            dup2(stderrFD, 2);
        }
        else
        {
            dup2(stdoutAndStderrFD, 1);
            dup2(stdoutAndStderrFD, 2);
        }
    }

    void stop_redirection()
    {
        fflush(stdout);
        fflush(stderr);
        dup2(stdoutFDBackup, 1);
        dup2(stderrFDBackup, 2);
        close(stdoutFDBackup);
        close(stderrFDBackup);
    }

    void print()
    {
        if (!separateStderr)
        {
            ifstream f(stdoutAndStderrTempFileName);
            if (f.is_open())
                cout << f.rdbuf();
        }
    }

private:
    bool temp_files_are_open() const
    {
        if (separateStderr)
            return stdoutTempFile && stderrTempFile;
        else
            return stdoutAndStderrTempFile;
    }
};

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

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
