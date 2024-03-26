
#include "output_redirection.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#define TEMP_FILE_SEP "_"
#define TEMP_FILE_EXTENSION ".log"
#define STANDARD_STREAMS_COUNT 2
#define MAX_FILE_NAME_LEN 200
#define STANDARD_FILE_NAME_LEN 7 // stdxxx\0
#define TEMP_STR_BUF_SIZE 1000

typedef struct OutputRedirection
{
    int   tempFilesCount; // Actual number of temp files created, depends on the value of 'separateStderr'
    FILE *tempFiles[STANDARD_STREAMS_COUNT];
    int   tempFilesFDs[STANDARD_STREAMS_COUNT];
    char  tempFilesNames[STANDARD_STREAMS_COUNT][MAX_FILE_NAME_LEN];

    FILE *standardFiles[STANDARD_STREAMS_COUNT];
    int   originalFDs[STANDARD_STREAMS_COUNT];
    char  standardFilesNames[STANDARD_STREAMS_COUNT][STANDARD_FILE_NAME_LEN];
    int   originalFDsBackups[STANDARD_STREAMS_COUNT];

    bool  separateStderr      : 1;
    bool  keepTempFiles       : 1;
    bool  redirectionIsActive : 1;
} OutputRedirection;

enum StreamsIndices
{
    IDX_STDOUT,
    IDX_STDERR,
    IDX_STDOUT_AND_STDERR = IDX_STDOUT,
};

static OutputRedirection g_outRed;
static OutputRedirection *g_ORP = &g_outRed; // Output Redirection Pointer
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

    fprintf(stderr, "%s", tempStrBuf);
    fprintf(stderr, "\n");
    exit(1);
}

static void OutRed_flushStandardStreams()
{
    int res;

    for (int idx = 0; idx < STANDARD_STREAMS_COUNT; ++idx)
    {
        res = fflush(g_ORP->standardFiles[idx]);
        if (res == EOF)
            OutRed_messageAndExit("Failed to flush %s", g_ORP->tempFilesNames[idx]);
    }
}

void OutRed_init(const char *tempFilePrefix, bool separateStderr, bool keepTempFiles)
{
    g_outRed = (OutputRedirection) {
        .standardFiles = {stdout, stderr},
        .originalFDs = {1, 2},
        .standardFilesNames = {"stdout", "stderr"},
        .separateStderr = separateStderr,
        .keepTempFiles = keepTempFiles,
        .redirectionIsActive = false,
    };

    if (separateStderr)
    {
        snprintf(g_ORP->tempFilesNames[IDX_STDOUT], MAX_FILE_NAME_LEN, "%s%s%s%s", tempFilePrefix, TEMP_FILE_SEP, "stdout", TEMP_FILE_EXTENSION);
        snprintf(g_ORP->tempFilesNames[IDX_STDERR], MAX_FILE_NAME_LEN, "%s%s%s%s", tempFilePrefix, TEMP_FILE_SEP, "stderr", TEMP_FILE_EXTENSION);
        g_ORP->tempFilesCount = IDX_STDERR + 1;
    }
    else
    {
        snprintf(g_ORP->tempFilesNames[IDX_STDOUT_AND_STDERR], MAX_FILE_NAME_LEN, "%s%s%s%s", tempFilePrefix, TEMP_FILE_SEP, "stdoutAndStderr", TEMP_FILE_EXTENSION);
        g_ORP->tempFilesCount = IDX_STDOUT_AND_STDERR + 1;
    }

    for (int idx = 0; idx < g_ORP->tempFilesCount; ++idx)
    {
        g_ORP->tempFiles[idx] = fopen(g_ORP->tempFilesNames[idx], "w");
        if (!(g_ORP->tempFiles[idx]))
            OutRed_perrorAndExit("Failed to open %s for writing", g_ORP->tempFilesNames[idx]);

        g_ORP->tempFilesFDs[idx] = fileno(g_ORP->tempFiles[idx]);
        if (g_ORP->tempFilesFDs[idx] == -1)
            OutRed_perrorAndExit("Failed to get file number of opened file %s", g_ORP->tempFilesNames[idx]);
    }

    for (int idx = 0; idx < STANDARD_STREAMS_COUNT; ++idx)
    {
        g_ORP->originalFDsBackups[idx] = dup(g_ORP->originalFDs[idx]);
        if (g_ORP->originalFDsBackups[idx] == -1)
            OutRed_perrorAndExit("Failed to duplicate FD %d", g_ORP->originalFDs[idx]);
    }
}

void OutRed_deinit()
{
    if (g_ORP->redirectionIsActive)
        OutRed_stopRedirection();

    for (int idx = 0; idx < g_ORP->tempFilesCount; ++idx)
    {
        int res;

        res = fclose(g_ORP->tempFiles[idx]);
        if (res == EOF)
            OutRed_messageAndExit("Failed to close %s", g_ORP->tempFilesNames[idx]);

        res = close(g_ORP->originalFDsBackups[idx]);
        if (res == EOF)
            OutRed_messageAndExit("Failed to close backup FD %d", g_ORP->originalFDsBackups[idx]);

        if (!g_ORP->keepTempFiles)
        {
            res = remove(g_ORP->tempFilesNames[idx]);
            if (res != 0)
                OutRed_messageAndExit("Failed to remove %s", g_ORP->tempFilesNames[idx]);
        }
    }
}

void OutRed_startRedirection()
{
    int res;

    if (g_ORP->redirectionIsActive)
        return;

    OutRed_flushStandardStreams();
    for (int idx = 0; idx < STANDARD_STREAMS_COUNT; ++idx)
    {
        if (g_ORP->separateStderr)
            res = dup2(g_ORP->tempFilesFDs[idx], g_ORP->originalFDs[idx]);
        else
            res = dup2(g_ORP->tempFilesFDs[0], g_ORP->originalFDs[idx]);

        if (res == -1)
            OutRed_perrorAndExit("Failed to redirect %s", g_ORP->standardFilesNames[idx]);
    }

    g_ORP->redirectionIsActive = true;
}

void OutRed_stopRedirection()
{
    int res;

    if (!g_ORP->redirectionIsActive)
        return;

    OutRed_flushStandardStreams();
    for (int idx = 0; idx < STANDARD_STREAMS_COUNT; ++idx)
    {
        res = dup2(g_ORP->originalFDsBackups[idx], g_ORP->originalFDs[idx]);
        if (res == -1)
            OutRed_perrorAndExit("Failed to restore %s", g_ORP->standardFilesNames[idx]);
    }

    g_ORP->redirectionIsActive = false;
}

void OutRed_printRedirectedData()
{
    FILE *fptr;
    int res;

    if (g_ORP->redirectionIsActive)
        return;

    for (int idx = 0; idx < g_ORP->tempFilesCount; ++idx)
    {
        fptr = fopen(g_ORP->tempFilesNames[idx], "r");
        if (fptr == NULL)
            OutRed_messageAndExit("Failed to open %s", g_ORP->tempFilesNames[idx]);

        while (true)
        {
            res = fgetc(fptr);
            if (res == EOF)
                break;
            // fprintf(g_ORP->standardFiles[idx], "%c", c);
            res = fputc(res, g_ORP->standardFiles[idx]);
            if (res == EOF)
                OutRed_messageAndExit("Failed to write to %s", g_ORP->tempFilesNames[idx]);
        }

        res = fclose(fptr);
        if (res == EOF)
            OutRed_messageAndExit("Failed to close %s", g_ORP->tempFilesNames[idx]);
    }
}
