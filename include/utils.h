#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>
#include <string>
#include <vector>

using namespace std;

uint16_t crc16_calc(uint8_t* data, int len);
vector<string> splitString(const string& str, const string& delim, size_t maxCount = 0, size_t start = 0);

#endif
