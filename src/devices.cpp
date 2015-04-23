#include "devices.h"

const tFlashSector flashLayout[] =
{
	{ 0x08000000, 0x04000,  0},           /* flash sector  0 - reserved for bootloader   */
	{ 0x08004000, 0x04000,  1},           /* flash sector  1 - reserved for bootloader   */
	{ 0x08008000, 0x04000,  2},           /* flash sector  2 -  16kb                     */
	{ 0x0800c000, 0x04000,  3},           /* flash sector  3 -  16kb                     */
	{ 0x08010000, 0x10000,  4},           /* flash sector  4 -  64kb                     */
	{ 0x08020000, 0x20000,  5},           /* flash sector  5 - 128kb                     */
	{ 0x08040000, 0x20000,  6},           /* flash sector  6 - 128kb                     */
	{ 0x08060000, 0x20000,  7},           /* flash sector  7 - 128kb                     */
// #if (BOOT_NVM_SIZE_KB > 512)
	{ 0x08080000, 0x20000,  8},           /* flash sector  8 - 128kb                     */
	{ 0x080A0000, 0x20000,  9},           /* flash sector  9 - 128kb                     */
	{ 0x080C0000, 0x20000, 10},           /* flash sector 10 - 128kb                     */
	{ 0x080E0000, 0x20000, 11},           /* flash sector 11 - 128kb                     */
// #endif
// #if (BOOT_NVM_SIZE_KB > 1024)
	{ 0x08100000, 0x04000, 12},           /* flash sector 12 -  16kb                     */
	{ 0x08104000, 0x04000, 13},           /* flash sector 13 -  16kb                     */
	{ 0x08108000, 0x04000, 14},           /* flash sector 14 -  16kb                     */
	{ 0x0810c000, 0x04000, 15},           /* flash sector 15 -  16kb                     */
	{ 0x08110000, 0x10000, 16},           /* flash sector 16 -  64kb                     */
	{ 0x08120000, 0x20000, 17},           /* flash sector 17 - 128kb                     */
	{ 0x08140000, 0x20000, 18},           /* flash sector 18 - 128kb                     */
	{ 0x08160000, 0x20000, 19},           /* flash sector 19 - 128kb                     */
	{ 0x08180000, 0x20000, 20},           /* flash sector 20 - 128kb                     */
	{ 0x081A0000, 0x20000, 21},           /* flash sector 21 - 128kb                     */
	{ 0x081C0000, 0x20000, 22},           /* flash sector 22 - 128kb                     */
	{ 0x081E0000, 0x20000, 23},           /* flash sector 23 - 128kb                     */
// #endif
};
const int flashPages = sizeof(flashLayout) / sizeof(flashLayout[0]);

void parseVersion(uint32_t version, int& a, int& b, int& c, int& d)
{
	a = (version >> 24) & 0xff;
	b = (version >> 16) & 0xff;
	c = (version >>  8) & 0xff;
	d = (version >>  0) & 0xff;
}
void makeVersion(uint32_t& version, int a, int b, int c, int d)
{
	version =
	  ((uint32_t)(a & 0xff) << 24) |
	  ((uint32_t)(b & 0xff) << 16) |
	  ((uint32_t)(c & 0xff) <<  8) |
	  ((uint32_t)(d & 0xff) <<  0);
}
