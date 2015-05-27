#include "console.h"

#include <stdio.h>
#include "myFTDI.h"
#include <unistd.h>
#include <thread>

#ifdef UNIX
#include <termios.h>
#include <signal.h>
#elif WIN32
#include <windows.h>
#endif

#include <pthread.h>

static bool stop = false;

void sigHandler(int num)
{
	stop = true;
}

#include <sys/time.h>
void thread()
{
#ifdef UNIX
	struct termios oldt, newt;
	tcgetattr(fileno(stdin), &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(fileno(stdin), TCSANOW, &newt);
#elif WIN32
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode = 0;
	GetConsoleMode(hStdin, &mode);
	SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
	ch = getchar();
	SetConsoleMode(hStdin, mode | ENABLE_ECHO_INPUT);
#endif

	while (!stop)
	{
		fd_set set;
		struct timeval tv;

		tv.tv_sec = 0;
		tv.tv_usec = 10 * 1000;

		FD_ZERO(&set);
		FD_SET(fileno(stdin), &set);

		int res = select(fileno(stdin) + 1, &set, NULL, NULL, &tv);

		if (res > 0)
		{
			char data[100];
			int r = read(fileno(stdin), data, 100);
			uart_tx(data, r);
		}
	}

#ifdef UNIX
	tcsetattr(fileno(stdin), TCSANOW, &oldt);
#endif
}

int runConsole(int speed)
{
	bool res = uart_open(speed, true);
	if (!res)
		return 1;

	signal(SIGINT, &sigHandler);

	std::thread th(thread);

	while (!stop)
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

	th.join();

	uart_close();
}

