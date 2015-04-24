#include "console.h"

#include <stdio.h>
#include "myFTDI.h"

#ifdef UNIX
#include <termios.h>
#elif WIN32
#include <windows.h>
#endif

#include <pthread.h>

int mygetch()
{
	int ch;
#ifdef UNIX
	struct termios oldt, newt;
	tcgetattr(fileno(stdin), &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(fileno(stdin), TCSANOW, &newt);
	ch = getchar();
	tcsetattr(fileno(stdin), TCSANOW, &oldt);
#elif WIN32
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode = 0;
	GetConsoleMode(hStdin, &mode);
	SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
	ch = getchar();
	SetConsoleMode(hStdin, mode | ENABLE_ECHO_INPUT);
#endif
	return ch;
}

void* thread(void*)
{
	for (;;)
	{
		int c = mygetch();
		char cc = (char)(c & 0xff);
		int res = uart_tx(&cc, 1);
	}
}

void openConsole(int speed)
{
	bool res = uart_open(speed, true);
	if (!res)
		return;
		
	pthread_t t;
	pthread_create(&t, 0, thread, 0);
	
	for (;;)
	{
		int res;
		char data[1024];
		res = uart_rx_any(data, 1024);
		if (res)
		{
			for (int i = 0; i < res; i++)
				putchar(data[i]);
		}
	}
}

