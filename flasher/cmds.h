#ifndef __CMDS_H__
#define __CMDS_H__

#include <vector>

using namespace std;

int hard_getVersion();
int hard_getCommand();
int hard_getID();
int hard_readoutUnprotect();
int hard_writeUnprotect();
int hard_readoutProtect();
int hard_erase();

#endif
