#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>
#include <string>
#include <vector>

using namespace std;

uint16_t crc16_calc(const uint8_t* data, int len);
vector<string> splitString(const string& str, const string& delim, size_t maxCount = 0, size_t start = 0);

extern int log_debug;
#define LOG_NICE(x,...) \
	do { if (!log_debug) fprintf(stderr, x, ##__VA_ARGS__); } while (0)
#define LOG_DEBUG(x,...) \
	do { if (log_debug) fprintf(stderr, x "\r\n", ##__VA_ARGS__); } while (0)
#define LOG(x,...) \
	do { fprintf(stderr, x, ##__VA_ARGS__); } while (0)

#endif
