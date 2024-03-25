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
    bool separateStderr, keepTempFiles, redirectionIsActive;
    string tempFilePrefix;

    vector<string> tempFilesNames;
    vector<FILE*> tempFiles;
    vector<int> tempFilesFDs;
    vector<int> originalFDsBackups;
    static inline const vector<int> originalFDs{1, 2};
    static inline const vector<string> standardFilesNames{"stdout", "stderr"};

public:
    OutputRedirection(const string tempFilePrefix, bool separateStderr=false, bool keepTempFiles=false) :
        tempFilePrefix{tempFilePrefix},
        separateStderr{separateStderr},
        keepTempFiles{keepTempFiles},
        redirectionIsActive(false)
    {
        if (separateStderr)
        {
            // Note: order of insertion matters
            tempFilesNames.push_back(tempFilePrefix + tempFileSep + "stdout" + tempFileExtension);
            tempFilesNames.push_back(tempFilePrefix + tempFileSep + "stderr" + tempFileExtension);
        }
        else
        {
            tempFilesNames.push_back(tempFilePrefix + tempFileSep + "stdoutAndStderr" + tempFileExtension);
        }

        for (auto tempFileName : tempFilesNames)
        {
            FILE *tempFile = fopen(tempFileName.c_str(), "w");
            if (!tempFile)
                perror_and_exit("Failed to open "s + tempFileName + " for writing"s);
            tempFiles.push_back(tempFile);

            int tempFD = fileno(tempFile);
            if (tempFD == -1)
                perror_and_exit("Failed to get file number of opened file "s + tempFileName);
            tempFilesFDs.push_back(tempFD);
        }

        for (int idx = 0; idx < originalFDs.size(); ++idx)
        {
            int FDBackup = dup(originalFDs[idx]);
            if (FDBackup == -1)
                perror_and_exit("Failed to duplicate FD "s + to_string(originalFDs[idx]));
            originalFDsBackups.push_back(FDBackup);
        }
    }

    ~OutputRedirection()
    {
        if (redirectionIsActive)
            stop_redirection();

        for (size_t idx = 0; idx < tempFilesNames.size(); ++idx)
        {
            int res;
            
            res = fclose(tempFiles[idx]);
            if (res == EOF)
                message_and_exit("Failed to close "s + tempFilesNames[idx]);

            res = close(originalFDsBackups[idx]);
            if (res == EOF)
                message_and_exit("Failed to close backup FD "s + to_string(originalFDsBackups[idx]));

            if (!keepTempFiles)
            {
                res = remove(tempFilesNames[idx].c_str());
                if (res != 0)
                    message_and_exit("Failed to remove "s + tempFilesNames[idx]);
            }
        }
    }

    void start_redirection()
    {
        int res;

        if (redirectionIsActive)
            return;

        flush_streams();
        if (separateStderr)
        {
            for (size_t idx = 0; idx < tempFilesNames.size(); ++idx)
            {
                res = dup2(tempFilesFDs[idx], originalFDs[idx]);
                if (res == -1)
                    perror_and_exit("Failed to redirect "s + standardFilesNames[idx]);
            }
        }
        else
        {
            for (size_t idx = 0; idx < originalFDs.size(); ++idx)
            {
                res = dup2(tempFilesFDs[0], originalFDs[idx]);
                if (res == -1)
                    perror_and_exit("Failed to redirect "s + standardFilesNames[idx]);
            }
        }

        redirectionIsActive = true;
    }

    void stop_redirection()
    {
        int res;

        if (!redirectionIsActive)
            return;

        flush_streams();
        for (size_t idx = 0; idx < originalFDs.size(); ++idx)
        {
            res = dup2(originalFDsBackups[idx], originalFDs[idx]);
            if (res == -1)
                perror_and_exit("Failed to restore "s + standardFilesNames[idx]);
        }

        redirectionIsActive = false;
    }

    void print()
    {
        if (redirectionIsActive)
            return;
        for (auto tempFileName : tempFilesNames)
        {
            ifstream f(tempFileName);
            if (f.is_open())
                cout << f.rdbuf();
            else
                message_and_exit("Failed to open "s + tempFileName);
        }
    }

private:
    void flush_streams()
    {
        int res;

        res = fflush(stdout);
        if (res == EOF)
            message_and_exit("Failed to flush stdout");

        res = fflush(stderr);
        if (res == EOF)
            message_and_exit("Failed to flush stderr");
    }

    void perror_and_exit(const string &message)
    {
        perror(message.c_str());
        exit(1);
    }

    void message_and_exit(const string &message)
    {
        cerr << message << endl;
        exit(1);
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
