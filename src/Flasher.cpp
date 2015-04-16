#include "Flasher.h"

int Flasher::load(const string& path)
{
	return m_hexFile.load(path) ? 0 : -1;
}
int Flasher::loadData(const char* data)
{
	return m_hexFile.loadData(data) ? 0 : -1;
}

