#ifndef _MISC_H_
#define _MISC_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <sys/stat.h>

#define MAX_FILENAME_LENGTH FILENAME_MAX

#ifdef _WIN32
#define SLASH '\\'
#include <windows.h>
#else
#define SLASH '/'
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif


time_t GetUnixTime ();
time_t MakeUnixTime (short y, short m, short d, short h, short n, short s);
time_t StringToUnixTime (char * strTime);

#endif
