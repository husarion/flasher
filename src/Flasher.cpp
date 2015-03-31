#include "Flasher.h"

int Flasher::load(const string& path)
{
	return m_hexFile.load(path) ? 0 : -1;
}

