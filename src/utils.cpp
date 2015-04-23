#include "utils.h"

uint16_t crc16_calc(uint8_t* data, int len)
{
	int i;

	uint16_t crc = 0;
	
	while (len--)
	{
		crc ^= *data++;
		for (i = 0; i < 8; ++i)
		{
			if (crc & 1)
				crc = (crc >> 1) ^ 0xA001;
			else
				crc = (crc >> 1);
		}
	}
	
	return crc;
}
vector<string> splitString(const string& str, const string& delim, size_t maxCount, size_t start)
{
	vector<std::string> parts;
	size_t idx = start, delimIdx;
	
	delimIdx = str.find(delim, idx);
	if (delimIdx == string::npos)
	{
		parts.push_back(str);
		return parts;
	}
	do
	{
		if (parts.size() == maxCount - 1)
		{
			string part = str.substr(idx);
			parts.push_back(part);
			idx = str.size();
			break;
		}
		string part = str.substr(idx, delimIdx - idx);
		parts.push_back(part);
		idx = delimIdx + delim.size();
		delimIdx = str.find(delim, idx);
	}
	while (delimIdx != string::npos && idx < str.size());
	
	if (idx < str.size())
	{
		string part = str.substr(idx);
		parts.push_back(part);
	}
	
	return parts;
}
