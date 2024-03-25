
#include "output_redirection.h"

#include <fstream>

using namespace std;

OutputRedirection::OutputRedirection(const string tempFilePrefix, bool separateStderr, bool keepTempFiles) :
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

OutputRedirection::~OutputRedirection()
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

void OutputRedirection::start_redirection()
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

void OutputRedirection::stop_redirection()
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

void OutputRedirection::print()
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

void OutputRedirection::flush_streams()
{
    int res;

    res = fflush(stdout);
    if (res == EOF)
        message_and_exit("Failed to flush stdout");

    res = fflush(stderr);
    if (res == EOF)
        message_and_exit("Failed to flush stderr");
}

void OutputRedirection::perror_and_exit(const string &message)
{
    perror(message.c_str());
    exit(1);
}

void OutputRedirection::message_and_exit(const string &message)
{
    cerr << message << endl;
    exit(1);
}
