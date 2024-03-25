
#include "output_redirection.h"
#include <stdio.h>
#include <stdarg.h>

#define TEMP_FILE_SEP "_"
#define TEMP_FILE_EXTENSION ".log"
#define STANDARD_STREAMS_COUNT 2
// #define MAX_FILES_NUM 2
#define MAX_FILE_NAME_LEN 200
#define TEMP_STR_BUF_SIZE 1000

typedef struct OutputRedirection
{
    char filesCount;
    char tempFilesNames[STANDARD_STREAMS_COUNT][MAX_FILE_NAME_LEN];
    FILE *tempFiles[STANDARD_STREAMS_COUNT];
    int tempFilesFDs[STANDARD_STREAMS_COUNT];
    int originalFDs[STANDARD_STREAMS_COUNT];
    int originalFDsBackups[STANDARD_STREAMS_COUNT];
    bool separateStderr      : 1;
    bool keepTempFiles       : 1;
    bool redirectionIsActive : 1;


} OutputRedirection;

enum StreamsIndices
{
    IDX_STDOUT,
    IDX_STDERR,
    IDX_STDOUT_AND_STDERR = 0,
};

static OutputRedirection g_outRed;
static char tempStrBuf[TEMP_STR_BUF_SIZE];

static void OutRed_perrorAndExit(const char *format, ...)
{
    va_list vlist;

    va_start(vlist, format);
    vsnprintf(tempStrBuf, TEMP_STR_BUF_SIZE, format, vlist);
    va_end(vlist);

    perror(tempStrBuf);
    exit(1);
}

static void OutRed_messageAndExit(const char *format, ...)
{
    va_list vlist;

    va_start(vlist, format);
    vsnprintf(tempStrBuf, TEMP_STR_BUF_SIZE, format, vlist);
    va_end(vlist);

    fprintf(stderr, tempStrBuf);
    fprintf(stderr, "\n");
    exit(1);
}

void OutRed_init(const char *tempFilePrefix, bool separateStderr, bool keepTempFiles)
{
    g_outRed = (OutputRedirection) {
        .originalFDs = {1, 2},
        .separateStderr = separateStderr,
        .keepTempFiles = keepTempFiles,
        .redirectionIsActive = redirectionIsActive,
    };

    if (separateStderr)
    {
        snprintf(g_outRed.tempFilesNames[IDX_STDOUT], MAX_FILE_NAME_LEN, "%s%s%s%s", tempFilePrefix, TEMP_FILE_SEP, "stdout", TEMP_FILE_EXTENSION);
        snprintf(g_outRed.tempFilesNames[IDX_STDERR], MAX_FILE_NAME_LEN, "%s%s%s%s", tempFilePrefix, TEMP_FILE_SEP, "stderr", TEMP_FILE_EXTENSION);
        g_outRed.filesCount = IDX_STDERR + 1;
    }
    else
    {
        snprintf(g_outRed.tempFilesNames[IDX_STDOUT_AND_STDERR], MAX_FILE_NAME_LEN, "%s%s%s%s", tempFilePrefix, TEMP_FILE_SEP, "stdoutAndStderr", TEMP_FILE_EXTENSION);
        g_outRed.filesCount = IDX_STDOUT_AND_STDERR + 1;
    }

    for (int idx = 0; idx < g_outRed.filesCount; ++idx)
    {
        g_outRed.tempFiles[idx] = fopen(g_outRed.tempFilesNames[idx], "w");
        if (!(g_outRed.tempFiles[idx]))
            OutRed_perrorAndExit("Failed to open %s for writing", g_outRed.tempFilesNames[idx]);

        g_outRed.tempFilesFDs[idx] = fileno(g_outRed.tempFiles[idx]);
        if (g_outRed.tempFilesFDs[idx] == -1)
            OutRed_perrorAndExit("Failed to get file number of opened file %s", g_outRed.tempFilesNames[idx]);
    }

    for (int idx = 0; idx < STANDARD_STREAMS_COUNT; ++idx)
    {
        g_outRed.originalFDsBackups[idx] = dup(g_outRed.originalFDs[idx]);
        if (g_outRed.originalFDsBackups[idx] == -1)
            OutRed_perrorAndExit("Failed to duplicate FD %d", g_outRed.originalFDs[idx]);
    }
}

OutRed_deinit(void)
{
    if (g_outRed.redirectionIsActive)
        OutRed_stopRedirection();

    for (size_t idx = 0; idx < g_outRed.filesCount; ++idx)
    {
        int res;

        res = fclose(g_outRed.tempFiles[idx]);
        if (res == EOF)
            OutRed_messageAndExit("Failed to close %s", g_outRed.tempFilesNames[idx]);

        res = close(g_outRed.originalFDsBackups[idx]);
        if (res == EOF)
            OutRed_messageAndExit("Failed to close backup FD %d", g_outRed.originalFDsBackups[idx]);

        if (!g_outRed.keepTempFiles)
        {
            res = remove(g_outRed.tempFilesNames[idx]);
            if (res != 0)
                OutRed_messageAndExit("Failed to remove %s", g_outRed.tempFilesNames[idx]);
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


