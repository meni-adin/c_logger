
#ifndef OUTPUT_REDIRECTION_H
#define OUTPUT_REDIRECTION_H

#if defined(__linux__) || defined(__APPLE__)
 #include <fcntl.h>
 #include <unistd.h>
#elif defined(_WIN32)
 #include <io.h>
 #define dup _dup
 #define dup2 _dup2
 #define fileno _fileno
#endif // OS
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void OutRed_init(const char *tempFilePrefix, bool separateStderr, bool keepTempFiles);
void OutRed_deinit();
void OutRed_startRedirection();
void OutRed_stopRedirection();
void OutRed_printRedirectedData();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // OUTPUT_REDIRECTION_H


// #include <fstream>
// #include <iostream>
// #include <vector>

// using namespace std;

// class OutputRedirection
// {
// public:
//     static inline const string tempFileSep{"_"};
//     static inline const string tempFileExtension = ".log";

// private:
//     bool separateStderr, keepTempFiles, redirectionIsActive;
//     string tempFilePrefix;

//     vector<string> tempFilesNames;
//     vector<FILE*> tempFiles;
//     vector<int> tempFilesFDs;
//     vector<int> originalFDsBackups;
//     static inline const vector<int> originalFDs{1, 2};
//     static inline const vector<string> standardFilesNames{"stdout", "stderr"};

// public:
//     OutputRedirection(const string tempFilePrefix, bool separateStderr=false, bool keepTempFiles=false);
//     ~OutputRedirection();
//     void start_redirection();
//     void stop_redirection();
//     void print();

// private:
//     void flush_streams();
//     void perror_and_exit(const string &message);
//     void message_and_exit(const string &message);
// };
