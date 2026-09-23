#include "os/filelog.h"

#include <fstream>
#include <ostream>
#include <string.h>

// 0x006ee280
std::fstream g_fileLog;

// 0x006ee320
int g_bFileLogOpen;

// 0x006ee378
char g_szFileLogPath[kFileLogPathSize];

// 0x0047ddf0
void FileLogStart(const char *pszPath) {
    strcpy(g_szFileLogPath, pszPath);
    // The image passes the default protection 0664 alongside the mode.
    g_fileLog.open(g_szFileLogPath, std::ios::out);
    g_bFileLogOpen = 1;
}

// 0x0047de48
void FileLogStop() {
    if (g_bFileLogOpen != 0) {
        g_fileLog.close();
        g_bFileLogOpen = 0;
    }
}

// 0x0047de88
void FileLogAppend(const char *pszText) {
    if (g_bFileLogOpen != 0) {
        g_fileLog << pszText << std::endl;
    }
}
